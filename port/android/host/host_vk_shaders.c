/*
HOST_VK_SHADERS.C

The Vulkan renderer's shader and pipeline services: glslang in the backend, the
shaders the game's draws need compiled on a thread of its own and kept on the device, and the pipelines made from
them, kept in a VkPipelineCache per driver.

The guest (port/android/guest/d3d8_vk.c) never sends a key here, only a 64-bit hash of one and GLSL, through the
imports host_vk_shader_find and host_vk_shader_compile; a draw's pipeline arrives in the command stream
(VK_COMMAND_PIPELINE). Nothing here reads guest memory but during the import call that names it (the GLSL is copied
inside host_vk_shader_compile).

On the device (the app's internal storage, which Android loads libraries from and the player does not see):

    vk_cache/spirv/<glslang tag>-<generator version>/<v|f><hash, 16 hex>.spv     the SPIR-V (the driver's does not matter)
    vk_cache/pipelines/<cache UUID>-<driver id>-<driver version>.bin              a VkPipelineCache, one per driver
    vk_cache/failed/<name>.vert|.frag                                              the GLSL of a shader that did not compile

Every file is written as .tmp and renamed when complete, so that a write cut short leaves no half file. A file that
does not check (its header, its size, a CRC of its contents) is said, deleted and made again.

Threads. The game thread (the guest's) calls the imports and runs the commands; one compile thread takes the queue
(shaders first, then pipelines), compiles with glslang (one at a time, whoever calls it), writes the cache file and
makes the shader module or the pipeline. VkDevice's creation functions may be called from any thread; the one
VkPipelineCache is internally synchronised (Vulkan, "Threading Behavior"), so the thread makes pipelines in it while
the game thread saves it. The tables are guarded by one mutex, never held across glslang or a driver call that makes
a pipeline (a module made from a file by find is, briefly: it takes fractions of a millisecond).
*/

#include "host.h"
#include "host_vk.h"

#include "glslang/Include/glslang_c_interface.h"
#include "glslang/Public/resource_limits_c.h"

#include <SDL3/SDL.h>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#ifndef HOST_VK_GLSLANG_TAG
#define HOST_VK_GLSLANG_TAG "unknown"
#endif

#define B host_vkb

static uint64_t now_ns(void)
{
	struct timespec time;

	clock_gettime(CLOCK_MONOTONIC, &time);
	return (uint64_t)time.tv_sec * 1000000000ull + (uint64_t)time.tv_nsec;
}

/* ---------- glslang: loaded with dlopen (libhalo_glslang.so), never linked; one compile at a time */

static struct
{
	void *library;
	int initialized;
	pthread_mutex_t lock;
	int (*initialize_process)(void);
	glslang_shader_t *(*shader_create)(const glslang_input_t *);
	void (*shader_delete)(glslang_shader_t *);
	int (*shader_preprocess)(glslang_shader_t *, const glslang_input_t *);
	int (*shader_parse)(glslang_shader_t *, const glslang_input_t *);
	const char *(*shader_info_log)(glslang_shader_t *);
	glslang_program_t *(*program_create)(void);
	void (*program_delete)(glslang_program_t *);
	void (*program_add_shader)(glslang_program_t *, glslang_shader_t *);
	int (*program_link)(glslang_program_t *, int);
	void (*program_spirv_generate)(glslang_program_t *, glslang_stage_t);
	size_t (*program_spirv_size)(glslang_program_t *);
	unsigned int *(*program_spirv_ptr)(glslang_program_t *);
	const char *(*program_info_log)(glslang_program_t *);
	const glslang_resource_t *(*default_resource)(void);
} G = { .lock = PTHREAD_MUTEX_INITIALIZER };

/* G.lock is held */
static int glslang_load(char *message, size_t message_size)
{
	if (G.initialized)
		return 1;
	if (G.library)
	{
		snprintf(message, message_size, "glslang did not load");
		return 0;
	}
	G.library = dlopen("libhalo_glslang.so", RTLD_NOW | RTLD_LOCAL);
	if (!G.library)
	{
		host_logf(HOST_LOG_ERROR, "vk: libhalo_glslang.so did not load: %s", dlerror());
		G.library = (void *)1;
		snprintf(message, message_size, "libhalo_glslang.so did not load");
		return 0;
	}
#define LOAD(field, name) \
	if (!(*(void **)&G.field = dlsym(G.library, name))) \
	{ \
		host_logf(HOST_LOG_ERROR, "vk: glslang has no %s", name); \
		snprintf(message, message_size, "glslang has no %s", name); \
		return 0; \
	}
	LOAD(initialize_process, "glslang_initialize_process")
	LOAD(shader_create, "glslang_shader_create")
	LOAD(shader_delete, "glslang_shader_delete")
	LOAD(shader_preprocess, "glslang_shader_preprocess")
	LOAD(shader_parse, "glslang_shader_parse")
	LOAD(shader_info_log, "glslang_shader_get_info_log")
	LOAD(program_create, "glslang_program_create")
	LOAD(program_delete, "glslang_program_delete")
	LOAD(program_add_shader, "glslang_program_add_shader")
	LOAD(program_link, "glslang_program_link")
	LOAD(program_spirv_generate, "glslang_program_SPIRV_generate")
	LOAD(program_spirv_size, "glslang_program_SPIRV_get_size")
	LOAD(program_spirv_ptr, "glslang_program_SPIRV_get_ptr")
	LOAD(program_info_log, "glslang_program_get_info_log")
	LOAD(default_resource, "glslang_default_resource")
#undef LOAD
	if (!G.initialize_process())
	{
		host_logf(HOST_LOG_ERROR, "vk: glslang_initialize_process failed");
		snprintf(message, message_size, "glslang_initialize_process failed");
		return 0;
	}
	G.initialized = 1;
	return 1;
}

/* GLSL to SPIR-V for Vulkan 1.0 (SPIR-V 1.0, as phases 0 to 4 compile); *words is malloc'd */
int host_vk_glslang_compile(const char *source, int fragment, uint32_t **words, size_t *count, char *message,
	size_t message_size)
{
	glslang_input_t input;
	glslang_shader_t *shader;
	glslang_program_t *program;
	glslang_stage_t stage = fragment ? GLSLANG_STAGE_FRAGMENT : GLSLANG_STAGE_VERTEX;
	const int messages = GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT;
	int ok = 0;

	*words = NULL;
	*count = 0;
	message[0] = 0;
	pthread_mutex_lock(&G.lock);
	if (!glslang_load(message, message_size))
	{
		pthread_mutex_unlock(&G.lock);
		return 0;
	}
	memset(&input, 0, sizeof(input));
	input.language = GLSLANG_SOURCE_GLSL;
	input.stage = stage;
	input.client = GLSLANG_CLIENT_VULKAN;
	input.client_version = GLSLANG_TARGET_VULKAN_1_0;
	input.target_language = GLSLANG_TARGET_SPV;
	input.target_language_version = GLSLANG_TARGET_SPV_1_0;
	input.code = source;
	input.default_version = 450;
	input.default_profile = GLSLANG_NO_PROFILE;
	input.messages = (glslang_messages_t)messages;
	input.resource = G.default_resource();
	shader = G.shader_create(&input);
	if (!G.shader_preprocess(shader, &input) || !G.shader_parse(shader, &input))
	{
		snprintf(message, message_size, "glslang: %s", G.shader_info_log(shader));
		G.shader_delete(shader);
		pthread_mutex_unlock(&G.lock);
		return 0;
	}
	program = G.program_create();
	G.program_add_shader(program, shader);
	if (G.program_link(program, messages))
	{
		G.program_spirv_generate(program, stage);
		*count = G.program_spirv_size(program);
		if (*count)
		{
			*words = malloc(*count * 4);
			if (*words)
			{
				memcpy(*words, G.program_spirv_ptr(program), *count * 4);
				ok = 1;
			}
		}
		else
		{
			snprintf(message, message_size, "glslang made no SPIR-V");
		}
	}
	else
	{
		snprintf(message, message_size, "glslang: %s", G.program_info_log(program));
	}
	G.program_delete(program);
	G.shader_delete(shader);
	pthread_mutex_unlock(&G.lock);
	return ok;
}

/* ---------- files */

static uint32_t crc32_of(const void *data, size_t size)
{
	static uint32_t table[256];
	const unsigned char *bytes = data;
	uint32_t crc = 0xffffffffu;
	size_t index;

	if (!table[1])
	{
		uint32_t n, k;

		for (n = 0; n < 256; n++)
		{
			uint32_t c = n;

			for (k = 0; k < 8; k++)
				c = c & 1 ? 0xedb88320u ^ (c >> 1) : c >> 1;
			table[n] = c;
		}
	}
	for (index = 0; index < size; index++)
		crc = table[(crc ^ bytes[index]) & 0xff] ^ (crc >> 8);
	return ~crc;
}

/* a folder and the ones above it, as far as they are missing */
static void make_path(const char *path)
{
	char partial[768];
	size_t index;

	snprintf(partial, sizeof(partial), "%s", path);
	for (index = 1; partial[index]; index++)
	{
		if (partial[index] == '/')
		{
			partial[index] = 0;
			mkdir(partial, 0770);
			partial[index] = '/';
		}
	}
	mkdir(partial, 0770);
}

/* the file's contents, malloc'd; NULL if it is not there */
static void *file_read(const char *path, size_t *size)
{
	FILE *file = fopen(path, "rb");
	void *data;
	long length;

	if (!file)
		return NULL;
	fseek(file, 0, SEEK_END);
	length = ftell(file);
	fseek(file, 0, SEEK_SET);
	if (length < 0 || !(data = malloc((size_t)length + 1)))
	{
		fclose(file);
		return NULL;
	}
	if (fread(data, 1, (size_t)length, file) != (size_t)length)
	{
		fclose(file);
		free(data);
		return NULL;
	}
	fclose(file);
	((char *)data)[length] = 0;
	*size = (size_t)length;
	return data;
}

/* written as <path>.tmp and renamed: 1 if it is there complete */
static int file_write(const char *path, const void *first, size_t first_size, const void *second, size_t second_size)
{
	char temporary[800];
	FILE *file;
	int ok;

	snprintf(temporary, sizeof(temporary), "%s.tmp", path);
	file = fopen(temporary, "wb");
	if (!file)
		return 0;
	ok = fwrite(first, 1, first_size, file) == first_size &&
		(!second_size || fwrite(second, 1, second_size, file) == second_size);
	ok = fclose(file) == 0 && ok;
	if (ok && rename(temporary, path) != 0)
		ok = 0;
	if (!ok)
		unlink(temporary);
	return ok;
}

/* a folder's files, and the folder (the cache's folders hold files only) */
static void remove_folder(const char *path)
{
	DIR *directory = opendir(path);
	struct dirent *entry;

	if (directory)
	{
		while ((entry = readdir(directory)) != NULL)
		{
			char file[800];

			if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
				continue;
			snprintf(file, sizeof(file), "%s/%s", path, entry->d_name);
			unlink(file);
		}
		closedir(directory);
	}
	rmdir(path);
}

/* ---------- the services' state */

enum
{
	STATE_NONE, /* known to the table without being anything: a file that proved bad */
	STATE_DISK, /* its SPIR-V is in the cache, its module not made */
	STATE_QUEUED,
	STATE_COMPILING,
	STATE_MADE,
	STATE_FAILED,
};

struct shader
{
	uint64_t hash;
	uint32_t stage;
	int state;
	uint32_t handle;
	VkShaderModule module;
	char *glsl; /* while queued or compiling */
	struct shader *next_in_bucket;
	struct shader *next_queued;
};

enum
{
	PIPELINE_QUEUED,
	PIPELINE_MAKING,
	PIPELINE_READY,
	PIPELINE_FAILED,
};

struct pipeline
{
	uint64_t vertex_hash, pixel_hash, state_hash;
	uint32_t vertex, pixel;
	struct vk_pipeline_state state;
	int status;
	VkPipeline pipeline;
	struct pipeline *next_in_bucket;
	struct pipeline *next_queued;
};

#define BUCKETS 4096
#define HANDLE_LIMIT 65536

static struct
{
	pthread_mutex_t lock;
	pthread_cond_t wake;
	/* signalled as each pipeline is made or fails: a draw waiting for one the thread is making (host_vk_pipeline_find) */
	pthread_cond_t pipeline_done;
	int started, thread_started;
	char root[640], spirv_folder[768], pipelines_folder[768], failed_folder[768], cache_file[900];
	struct shader *buckets[BUCKETS];
	struct shader *handles[HANDLE_LIMIT];
	uint32_t handle_count;
	struct shader *shader_head, *shader_tail;
	struct pipeline *pipeline_buckets[BUCKETS];
	struct pipeline *pipeline_head, *pipeline_tail;
	/* counted since the start, and the thread's since its last line */
	unsigned from_disk, compiled, failed, shaders_queued, pipelines_made, pipelines_failed, pipelines_queued;
	struct
	{
		unsigned shaders, pipelines;
		double glslang_ms, module_ms, pipeline_ms;
		uint64_t last;
	} thread;
	unsigned long pipeline_cache_size;
	unsigned made_since_save;
	uint64_t last_save;
	int first_find_logged, first_compile_logged;
	/* pipelines a draw made itself rather than wait for the thread, since the start */
	unsigned pipelines_made_at_draw;
} S = { .lock = PTHREAD_MUTEX_INITIALIZER, .wake = PTHREAD_COND_INITIALIZER, .pipeline_done = PTHREAD_COND_INITIALIZER };

static unsigned bucket_of(uint32_t stage, uint64_t hash)
{
	return (unsigned)((hash ^ (hash >> 29) ^ stage) % BUCKETS);
}

/* S.lock is held */
static struct shader *shader_lookup(uint32_t stage, uint64_t hash)
{
	struct shader *shader;

	for (shader = S.buckets[bucket_of(stage, hash)]; shader; shader = shader->next_in_bucket)
	{
		if (shader->stage == stage && shader->hash == hash)
			return shader;
	}
	return NULL;
}

static struct shader *shader_add(uint32_t stage, uint64_t hash, int state)
{
	struct shader *shader = calloc(1, sizeof(*shader));
	unsigned bucket = bucket_of(stage, hash);

	if (!shader)
		return NULL;
	shader->stage = stage;
	shader->hash = hash;
	shader->state = state;
	shader->next_in_bucket = S.buckets[bucket];
	S.buckets[bucket] = shader;
	return shader;
}

static void shader_name(char *name, size_t size, uint32_t stage, uint64_t hash)
{
	snprintf(name, size, "%c%016llx", stage == VK_SHADER_STAGE_VERTEX ? 'v' : 'f', (unsigned long long)hash);
}

/* ---------- the SPIR-V files */

#define SPIRV_MAGIC 0x56505348u /* 'HSPV' */

struct spirv_header
{
	uint32_t magic;
	uint32_t version;
	uint64_t hash;
	uint32_t size;
	uint32_t crc;
};

static void spirv_path(char *path, size_t size, uint32_t stage, uint64_t hash)
{
	char name[40];

	shader_name(name, sizeof(name), stage, hash);
	snprintf(path, size, "%s/%s.spv", S.spirv_folder, name);
}

static int spirv_write(uint32_t stage, uint64_t hash, const uint32_t *words, size_t count)
{
	struct spirv_header header;
	char path[900];

	header.magic = SPIRV_MAGIC;
	header.version = 1;
	header.hash = hash;
	header.size = (uint32_t)(count * 4);
	header.crc = crc32_of(words, count * 4);
	spirv_path(path, sizeof(path), stage, hash);
	return file_write(path, &header, sizeof(header), words, count * 4);
}

/* the SPIR-V of a file that checks, malloc'd (words); 0 and the file deleted if it does not */
static int spirv_read(uint32_t stage, uint64_t hash, uint32_t **words, size_t *count)
{
	char path[900];
	size_t size;
	unsigned char *data;
	struct spirv_header header;

	spirv_path(path, sizeof(path), stage, hash);
	data = file_read(path, &size);
	if (!data)
		return 0;
	if (size < sizeof(header))
		goto bad;
	memcpy(&header, data, sizeof(header));
	if (header.magic != SPIRV_MAGIC || header.version != 1 || header.hash != hash || header.size == 0 ||
		(header.size & 3) || (size_t)header.size != size - sizeof(header) ||
		crc32_of(data + sizeof(header), header.size) != header.crc)
		goto bad;
	*words = malloc(header.size);
	if (!*words)
	{
		free(data);
		return 0;
	}
	memcpy(*words, data + sizeof(header), header.size);
	*count = header.size / 4;
	free(data);
	return 1;
bad:
	host_logf(HOST_LOG_WARN, "vk: the cached shader %s is damaged (%zu bytes); deleted, to be compiled again", path, size);
	free(data);
	unlink(path);
	return 0;
}

static VkShaderModule module_make(const uint32_t *words, size_t count)
{
	VkShaderModuleCreateInfo info = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	VkShaderModule module = VK_NULL_HANDLE;

	info.codeSize = count * 4;
	info.pCode = words;
	if (!HOST_VK_CHECK(vkCreateShaderModule(B.device, &info, NULL, &module)))
		return VK_NULL_HANDLE;
	return module;
}

/* S.lock is held: the shader is made, with a handle */
static int shader_made(struct shader *shader, VkShaderModule module)
{
	if (S.handle_count >= HANDLE_LIMIT)
	{
		vkDestroyShaderModule(B.device, module, NULL);
		return 0;
	}
	shader->module = module;
	shader->handle = ++S.handle_count;
	S.handles[shader->handle - 1] = shader;
	shader->state = STATE_MADE;
	return 1;
}

/* ---------- the imports */

uint32_t host_vk_shader_find(uint32_t stage, uint64_t hash, uint32_t status_out)
{
	uint32_t *status = (uint32_t *)(uintptr_t)status_out;
	struct shader *shader;
	uint32_t handle = 0, result = VK_SHADER_STATUS_UNKNOWN;

	if (!S.first_find_logged)
	{
		S.first_find_logged = 1;
		host_logf(HOST_LOG_INFO, "vk: the first host_vk_shader_find: stage %u, hash %016llx (the guest logs its own)",
			(unsigned)stage, (unsigned long long)hash);
	}
	if (stage > VK_SHADER_STAGE_PIXEL)
		return 0;
	if (!host_vk_backend_ensure() || !S.started)
	{
		if (status)
			*status = VK_SHADER_STATUS_NOT_READY;
		return 0;
	}
	pthread_mutex_lock(&S.lock);
	shader = shader_lookup(stage, hash);
	if (shader)
	{
		switch (shader->state)
		{
		case STATE_MADE:
			handle = shader->handle;
			break;
		case STATE_DISK:
		{
			uint32_t *words;
			size_t count;

			/* the file is read and the module made now: fractions of a millisecond (0.4 to 1.8 measured) */
			if (spirv_read(stage, hash, &words, &count))
			{
				VkShaderModule module = module_make(words, count);

				free(words);
				if (module && shader_made(shader, module))
				{
					S.from_disk++;
					handle = shader->handle;
					break;
				}
				shader->state = STATE_FAILED;
				S.failed++;
				result = VK_SHADER_STATUS_FAILED;
				break;
			}
			shader->state = STATE_NONE;
			break;
		}
		case STATE_QUEUED:
			result = VK_SHADER_STATUS_QUEUED;
			break;
		case STATE_COMPILING:
			result = VK_SHADER_STATUS_COMPILING;
			break;
		case STATE_FAILED:
			result = VK_SHADER_STATUS_FAILED;
			break;
		default:
			break;
		}
	}
	pthread_mutex_unlock(&S.lock);
	if (status && !handle)
		*status = result;
	return handle;
}

/* a compile asked for and not queued: the guest asks a few times and then stops, so its draws would be skipped for the
run with nothing in the log to say why; this says it, the first few times */
static void compile_dropped(uint32_t stage, uint64_t hash, const char *why)
{
	static unsigned said;

	if (said++ < 8)
		host_logf(HOST_LOG_ERROR, "vk: the %s shader %016llx was not queued (%s); its draws are skipped",
			stage == VK_SHADER_STAGE_VERTEX ? "vertex" : "pixel", (unsigned long long)hash, why);
}

void host_vk_shader_compile(uint32_t stage, uint64_t hash, uint32_t glsl_address, uint32_t glsl_size)
{
	const char *glsl = (const char *)(uintptr_t)glsl_address;
	struct shader *shader;
	char *copy;

	if (!S.first_compile_logged)
	{
		S.first_compile_logged = 1;
		host_logf(HOST_LOG_INFO, "vk: the first host_vk_shader_compile: stage %u, hash %016llx, %u bytes of GLSL (the guest "
			"logs its own)", (unsigned)stage, (unsigned long long)hash, (unsigned)glsl_size);
	}
	if (stage > VK_SHADER_STAGE_PIXEL || !glsl || !glsl_size || !S.started)
		return;
	/* copied during the call: the guest's memory is not read again */
	copy = malloc((size_t)glsl_size + 1);
	if (!copy)
	{
		compile_dropped(stage, hash, "out of memory for its GLSL");
		return;
	}
	memcpy(copy, glsl, glsl_size);
	copy[glsl_size] = 0;
	pthread_mutex_lock(&S.lock);
	shader = shader_lookup(stage, hash);
	if (shader && shader->state != STATE_NONE)
	{
		pthread_mutex_unlock(&S.lock);
		free(copy);
		return;
	}
	if (!shader)
		shader = shader_add(stage, hash, STATE_QUEUED);
	if (!shader)
	{
		pthread_mutex_unlock(&S.lock);
		free(copy);
		compile_dropped(stage, hash, "the shader table is full or out of memory");
		return;
	}
	shader->state = STATE_QUEUED;
	shader->glsl = copy;
	shader->next_queued = NULL;
	if (S.shader_tail)
		S.shader_tail->next_queued = shader;
	else
		S.shader_head = shader;
	S.shader_tail = shader;
	S.shaders_queued++;
	pthread_cond_signal(&S.wake);
	pthread_mutex_unlock(&S.lock);
}

/* ---------- pipelines */

static uint64_t state_hash_of(const struct vk_pipeline_state *state)
{
	return vk_hash_mix(vk_hash_init(), state, sizeof(*state));
}

static struct pipeline *pipeline_lookup(uint64_t vertex_hash, uint64_t pixel_hash, uint64_t state_hash)
{
	struct pipeline *pipeline;
	uint64_t key = vertex_hash * 31 + pixel_hash * 17 + state_hash;

	for (pipeline = S.pipeline_buckets[key % BUCKETS]; pipeline; pipeline = pipeline->next_in_bucket)
	{
		if (pipeline->vertex_hash == vertex_hash && pipeline->pixel_hash == pixel_hash &&
			pipeline->state_hash == state_hash)
			return pipeline;
	}
	return NULL;
}

/* the pipeline for two shader handles and a state: made on the compile thread when new, and the VkPipeline once it is ready,
else VK_NULL_HANDLE (the draw is skipped, counted). B.lock is held. */
static void compile_pipeline_item(struct pipeline *pipeline);
static void pipeline_unqueue(struct pipeline *pipeline);

VkPipeline host_vk_pipeline_find(uint32_t vertex_handle, uint32_t pixel_handle, const struct vk_pipeline_state *state)
{
	struct shader *vertex, *pixel;
	struct pipeline *pipeline;
	uint64_t state_hash;
	VkPipeline ready = VK_NULL_HANDLE;

	if (!S.started)
		return VK_NULL_HANDLE;
	/* a draw whose shader is not ready asks for nothing (it is skipped, counted) */
	if (!vertex_handle || !pixel_handle || vertex_handle > HANDLE_LIMIT || pixel_handle > HANDLE_LIMIT)
	{
		B.counts.draws_skipped_shader++;
		return VK_NULL_HANDLE;
	}
	state_hash = state_hash_of(state);
	pthread_mutex_lock(&S.lock);
	vertex = S.handles[vertex_handle - 1];
	pixel = S.handles[pixel_handle - 1];
	if (!vertex || !pixel)
	{
		pthread_mutex_unlock(&S.lock);
		B.counts.draws_skipped_shader++;
		return VK_NULL_HANDLE;
	}
	pipeline = pipeline_lookup(vertex->hash, pixel->hash, state_hash);
	if (!pipeline)
	{
		uint64_t key = vertex->hash * 31 + pixel->hash * 17 + state_hash;

		pipeline = calloc(1, sizeof(*pipeline));
		if (pipeline)
		{
			pipeline->vertex_hash = vertex->hash;
			pipeline->pixel_hash = pixel->hash;
			pipeline->state_hash = state_hash;
			pipeline->vertex = vertex_handle;
			pipeline->pixel = pixel_handle;
			pipeline->state = *state;
			pipeline->status = PIPELINE_QUEUED;
			pipeline->next_in_bucket = S.pipeline_buckets[key % BUCKETS];
			S.pipeline_buckets[key % BUCKETS] = pipeline;
			if (S.pipeline_tail)
				S.pipeline_tail->next_queued = pipeline;
			else
				S.pipeline_head = pipeline;
			S.pipeline_tail = pipeline;
			S.pipelines_queued++;
			pthread_cond_signal(&S.wake);
		}
	}
	/* Not made yet: made now, by the draw that needs it, rather than the draw left out until the thread gets to it. Its
	shaders are made (the draw has their handles), and with the driver's cache warm a pipeline takes well under a
	millisecond, where leaving the draw out showed as the sky and whole passes flashing for the first seconds of a map,
	and after each new effect. One the thread is making already is waited for. */
	if (pipeline && pipeline->status == PIPELINE_QUEUED)
	{
		pipeline_unqueue(pipeline);
		compile_pipeline_item(pipeline);
		S.pipelines_made_at_draw++;
	}
	while (pipeline && pipeline->status == PIPELINE_MAKING)
		pthread_cond_wait(&S.pipeline_done, &S.lock);
	if (pipeline && pipeline->status == PIPELINE_READY)
	{
		B.counts.draws_ready++;
		ready = pipeline->pipeline;
	}
	else
		B.counts.draws_skipped_pipeline++;
	pthread_mutex_unlock(&S.lock);
	return ready;
}

/* a pipeline asked for in the stream by itself (the draw record does it now) */
void host_vk_pipeline_command(const struct vk_command_pipeline *command)
{
	host_vk_pipeline_find(command->vertex_shader, command->pixel_shader, &command->state);
}

static VkFormat format_of(uint32_t which, int depth)
{
	if (!which)
		return VK_FORMAT_UNDEFINED;
	return depth ? B.depth_format : B.color_format;
}

/* the pipeline made, with the state it carries (Vulkan 1.0's core dynamic states are dynamic; the rest is here) */
static VkPipeline pipeline_make(const struct pipeline *wanted, VkShaderModule vertex, VkShaderModule pixel)
{
	const struct vk_pipeline_state *state = &wanted->state;
	VkPipelineShaderStageCreateInfo stages[2] = { { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO },
		{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO } };
	VkVertexInputBindingDescription bindings[VK_PIPELINE_VERTEX_BINDINGS];
	VkVertexInputAttributeDescription attributes[VK_PIPELINE_VERTEX_ATTRIBUTES];
	VkPipelineVertexInputStateCreateInfo vertex_input = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	VkPipelineInputAssemblyStateCreateInfo assembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	VkPipelineViewportStateCreateInfo viewport = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	VkPipelineRasterizationStateCreateInfo raster = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	VkPipelineMultisampleStateCreateInfo multisample = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	VkPipelineDepthStencilStateCreateInfo depth_stencil = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	VkPipelineColorBlendAttachmentState blend_attachment;
	VkPipelineColorBlendStateCreateInfo blend = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS,
		VK_DYNAMIC_STATE_BLEND_CONSTANTS, VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK, VK_DYNAMIC_STATE_STENCIL_WRITE_MASK,
		VK_DYNAMIC_STATE_STENCIL_REFERENCE };
	VkPipelineDynamicStateCreateInfo dynamic = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	VkPipelineRenderingCreateInfo rendering = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	VkGraphicsPipelineCreateInfo info = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	VkFormat color_format = format_of(state->color_format, 0);
	VkPipeline pipeline = VK_NULL_HANDLE;
	unsigned index, binding_count = state->binding_count > VK_PIPELINE_VERTEX_BINDINGS ?
		VK_PIPELINE_VERTEX_BINDINGS : state->binding_count;

	stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = vertex;
	stages[0].pName = "main";
	stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = pixel;
	stages[1].pName = "main";
	for (index = 0; index < binding_count; index++)
	{
		bindings[index].binding = index;
		bindings[index].stride = state->bindings[index].stride;
		bindings[index].inputRate = (VkVertexInputRate)state->bindings[index].rate;
	}
	for (index = 0; index < VK_PIPELINE_VERTEX_ATTRIBUTES; index++)
	{
		/* every attribute a vertex shader declares has one (all sixteen) */
		attributes[index].location = index;
		attributes[index].binding = state->attributes[index].binding;
		attributes[index].format = (VkFormat)state->attributes[index].format;
		attributes[index].offset = state->attributes[index].offset;
	}
	vertex_input.vertexBindingDescriptionCount = binding_count;
	vertex_input.pVertexBindingDescriptions = bindings;
	vertex_input.vertexAttributeDescriptionCount = VK_PIPELINE_VERTEX_ATTRIBUTES;
	vertex_input.pVertexAttributeDescriptions = attributes;
	assembly.topology = (VkPrimitiveTopology)state->topology;
	viewport.viewportCount = 1;
	viewport.scissorCount = 1;
	/* wireframe and point fill need fillModeNonSolid (a debug mode of the game's): filled where the device lacks it */
	raster.polygonMode = B.fill_mode_non_solid ? (VkPolygonMode)state->polygon_mode : VK_POLYGON_MODE_FILL;
	raster.cullMode = (VkCullModeFlags)state->cull_mode;
	raster.frontFace = (VkFrontFace)state->front_face;
	raster.depthBiasEnable = state->depth_bias ? VK_TRUE : VK_FALSE;
	raster.lineWidth = 1.0f;
	/* the targets' samples a pixel (vk_command_targets), which the rendering it is used in has too */
	multisample.rasterizationSamples = state->samples > 1 ? (VkSampleCountFlagBits)state->samples : VK_SAMPLE_COUNT_1_BIT;
	depth_stencil.depthTestEnable = state->depth_test ? VK_TRUE : VK_FALSE;
	depth_stencil.depthWriteEnable = state->depth_write ? VK_TRUE : VK_FALSE;
	depth_stencil.depthCompareOp = (VkCompareOp)state->depth_compare_op;
	depth_stencil.stencilTestEnable = state->stencil_test ? VK_TRUE : VK_FALSE;
	depth_stencil.front.failOp = (VkStencilOp)state->stencil_front.fail_op;
	depth_stencil.front.passOp = (VkStencilOp)state->stencil_front.pass_op;
	depth_stencil.front.depthFailOp = (VkStencilOp)state->stencil_front.depth_fail_op;
	depth_stencil.front.compareOp = (VkCompareOp)state->stencil_front.compare_op;
	depth_stencil.back.failOp = (VkStencilOp)state->stencil_back.fail_op;
	depth_stencil.back.passOp = (VkStencilOp)state->stencil_back.pass_op;
	depth_stencil.back.depthFailOp = (VkStencilOp)state->stencil_back.depth_fail_op;
	depth_stencil.back.compareOp = (VkCompareOp)state->stencil_back.compare_op;
	memset(&blend_attachment, 0, sizeof(blend_attachment));
	blend_attachment.blendEnable = state->blend_enable ? VK_TRUE : VK_FALSE;
	blend_attachment.srcColorBlendFactor = (VkBlendFactor)state->source_color_factor;
	blend_attachment.dstColorBlendFactor = (VkBlendFactor)state->destination_color_factor;
	blend_attachment.colorBlendOp = (VkBlendOp)state->color_op;
	blend_attachment.srcAlphaBlendFactor = (VkBlendFactor)state->source_alpha_factor;
	blend_attachment.dstAlphaBlendFactor = (VkBlendFactor)state->destination_alpha_factor;
	blend_attachment.alphaBlendOp = (VkBlendOp)state->alpha_op;
	blend_attachment.colorWriteMask = (VkColorComponentFlags)state->color_write_mask;
	blend.attachmentCount = color_format != VK_FORMAT_UNDEFINED ? 1 : 0;
	blend.pAttachments = &blend_attachment;
	dynamic.dynamicStateCount = sizeof(dynamic_states) / sizeof(dynamic_states[0]);
	dynamic.pDynamicStates = dynamic_states;
	rendering.colorAttachmentCount = color_format != VK_FORMAT_UNDEFINED ? 1 : 0;
	rendering.pColorAttachmentFormats = &color_format;
	rendering.depthAttachmentFormat = format_of(state->depth_format, 1);
	rendering.stencilAttachmentFormat = rendering.depthAttachmentFormat;
	info.pNext = &rendering;
	info.stageCount = 2;
	info.pStages = stages;
	info.pVertexInputState = &vertex_input;
	info.pInputAssemblyState = &assembly;
	info.pViewportState = &viewport;
	info.pRasterizationState = &raster;
	info.pMultisampleState = &multisample;
	info.pDepthStencilState = &depth_stencil;
	info.pColorBlendState = &blend;
	info.pDynamicState = &dynamic;
	info.layout = B.draw_layout;
	info.basePipelineIndex = -1;
	if (!HOST_VK_CHECK(vkCreateGraphicsPipelines(B.device, B.pipeline_cache, 1, &info, NULL, &pipeline)))
		return VK_NULL_HANDLE;
	return pipeline;
}

/* ---------- the compile thread */

static void log_thread(int force)
{
	uint64_t now = now_ns();
	unsigned queued_shaders = 0, queued_pipelines = 0;
	struct shader *shader;
	struct pipeline *pipeline;

	if (!S.thread.shaders && !S.thread.pipelines)
		return;
	if (!force && now - S.thread.last < 5000000000ull)
		return;
	for (shader = S.shader_head; shader; shader = shader->next_queued)
		queued_shaders++;
	for (pipeline = S.pipeline_head; pipeline; pipeline = pipeline->next_queued)
		queued_pipelines++;
	host_logf(HOST_LOG_INFO, "vk: compile thread: %u shaders (glslang %.2f ms, module %.2f ms on average), %u pipelines "
		"(%.2f ms on average); %u shaders and %u pipelines waiting; since the start %u from the cache, %u compiled, %u failed, "
		"%u pipelines made", S.thread.shaders, S.thread.shaders ? S.thread.glslang_ms / S.thread.shaders : 0.0,
		S.thread.shaders ? S.thread.module_ms / S.thread.shaders : 0.0, S.thread.pipelines,
		S.thread.pipelines ? S.thread.pipeline_ms / S.thread.pipelines : 0.0, queued_shaders, queued_pipelines,
		S.from_disk, S.compiled, S.failed, S.pipelines_made);
	memset(&S.thread, 0, sizeof(S.thread));
	S.thread.last = now;
}

/* a shader that did not compile: said once, its GLSL kept next to the cache for the PC check */
static void shader_failed(struct shader *shader, const char *why)
{
	char name[40], path[900];

	shader_name(name, sizeof(name), shader->stage, shader->hash);
	snprintf(path, sizeof(path), "%s/%s.%s", S.failed_folder, name, shader->stage == VK_SHADER_STAGE_VERTEX ? "vert" : "frag");
	host_logf(HOST_LOG_ERROR, "vk: the %s shader %016llx did not compile (%s); its draws are skipped for the run, its GLSL is "
		"in %s", shader->stage == VK_SHADER_STAGE_VERTEX ? "vertex" : "pixel", (unsigned long long)shader->hash, why, path);
	if (shader->glsl)
		file_write(path, shader->glsl, strlen(shader->glsl), NULL, 0);
}

/* S.lock is held on entry and on return */
static void compile_shader_item(struct shader *shader)
{
	uint32_t *words = NULL;
	size_t count = 0;
	char message[2048];
	uint64_t start, middle;
	VkShaderModule module = VK_NULL_HANDLE;
	int ok;

	shader->state = STATE_COMPILING;
	pthread_mutex_unlock(&S.lock);
	start = now_ns();
	ok = host_vk_glslang_compile(shader->glsl, shader->stage == VK_SHADER_STAGE_PIXEL, &words, &count, message,
		sizeof(message));
	middle = now_ns();
	if (ok)
	{
		if (!spirv_write(shader->stage, shader->hash, words, count))
			host_logf(HOST_LOG_WARN, "vk: the SPIR-V of shader %016llx could not be written to the cache",
				(unsigned long long)shader->hash);
		module = module_make(words, count);
		if (!module)
		{
			snprintf(message, sizeof(message), "vkCreateShaderModule failed");
			ok = 0;
		}
	}
	free(words);
	if (!ok)
		shader_failed(shader, message);
	pthread_mutex_lock(&S.lock);
	S.thread.shaders++;
	S.thread.glslang_ms += (double)(middle - start) / 1e6;
	S.thread.module_ms += (double)(now_ns() - middle) / 1e6;
	if (ok && shader_made(shader, module))
	{
		S.compiled++;
	}
	else
	{
		shader->state = STATE_FAILED;
		S.failed++;
	}
	free(shader->glsl);
	shader->glsl = NULL;
}

static void compile_pipeline_item(struct pipeline *pipeline)
{
	VkShaderModule vertex, pixel;
	uint64_t start;
	VkPipeline made;

	pipeline->status = PIPELINE_MAKING;
	vertex = S.handles[pipeline->vertex - 1]->module;
	pixel = S.handles[pipeline->pixel - 1]->module;
	pthread_mutex_unlock(&S.lock);
	start = now_ns();
	made = pipeline_make(pipeline, vertex, pixel);
	pthread_mutex_lock(&S.lock);
	S.thread.pipelines++;
	S.thread.pipeline_ms += (double)(now_ns() - start) / 1e6;
	if (made)
	{
		pipeline->pipeline = made;
		pipeline->status = PIPELINE_READY;
		S.pipelines_made++;
		S.made_since_save++;
	}
	else
	{
		pipeline->status = PIPELINE_FAILED;
		S.pipelines_failed++;
	}
	pthread_cond_broadcast(&S.pipeline_done);
}

/* the pipeline taken out of the thread's queue (S.lock is held) */
static void pipeline_unqueue(struct pipeline *pipeline)
{
	struct pipeline *previous = NULL, *at;

	for (at = S.pipeline_head; at && at != pipeline; at = at->next_queued)
		previous = at;
	if (!at)
		return;
	if (previous)
		previous->next_queued = pipeline->next_queued;
	else
		S.pipeline_head = pipeline->next_queued;
	if (S.pipeline_tail == pipeline)
		S.pipeline_tail = previous;
	pipeline->next_queued = NULL;
}

static void *compile_thread(void *unused)
{
	(void)unused;
	/* below the game thread's priority: Android lets an app raise its own niceness */
	setpriority(PRIO_PROCESS, (id_t)syscall(SYS_gettid), 10);
	pthread_mutex_lock(&S.lock);
	for (;;)
	{
		struct shader *shader = S.shader_head;
		struct pipeline *pipeline = S.pipeline_head;

		if (B.dead)
		{
			pthread_cond_wait(&S.wake, &S.lock);
			continue;
		}
		if (shader)
		{
			S.shader_head = shader->next_queued;
			if (!S.shader_head)
				S.shader_tail = NULL;
			compile_shader_item(shader);
		}
		else if (pipeline)
		{
			S.pipeline_head = pipeline->next_queued;
			if (!S.pipeline_head)
				S.pipeline_tail = NULL;
			compile_pipeline_item(pipeline);
		}
		else
		{
			log_thread(0);
			pthread_cond_wait(&S.wake, &S.lock);
			continue;
		}
		log_thread(0);
	}
	return NULL;
}

/* ---------- the pipeline cache */

#define CACHE_MAGIC 0x43505648u /* 'HVPC' */

struct cache_header
{
	uint32_t magic;
	uint32_t size;
	uint32_t crc;
};

static void pipeline_cache_path(void)
{
	const unsigned char *uuid = host_vk.properties.pipelineCacheUUID;
	char hex[VK_UUID_SIZE * 2 + 1];
	unsigned index;

	for (index = 0; index < VK_UUID_SIZE; index++)
		snprintf(hex + index * 2, 3, "%02x", uuid[index]);
	snprintf(S.cache_file, sizeof(S.cache_file), "%s/%s-%u-%08x.bin", S.pipelines_folder, hex, (unsigned)host_vk.driver_id,
		(unsigned)host_vk.properties.driverVersion);
}

/* the cache the driver is started with: the file's contents if they check, and it takes them */
static void pipeline_cache_load(void)
{
	VkPipelineCacheCreateInfo info = { VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
	struct cache_header header;
	size_t size = 0;
	unsigned char *data = file_read(S.cache_file, &size);
	VkResult result;
	int from_file = 0;

	if (data)
	{
		if (size >= sizeof(header))
			memcpy(&header, data, sizeof(header));
		if (size < sizeof(header) || header.magic != CACHE_MAGIC || header.size != size - sizeof(header) ||
			crc32_of(data + sizeof(header), header.size) != header.crc)
		{
			host_logf(HOST_LOG_WARN, "vk: the pipeline cache %s is damaged (%zu bytes); deleted, a new one is started",
				S.cache_file, size);
			unlink(S.cache_file);
			free(data);
			data = NULL;
		}
		else
		{
			info.initialDataSize = header.size;
			info.pInitialData = data + sizeof(header);
			from_file = 1;
		}
	}
	result = vkCreatePipelineCache(B.device, &info, NULL, &B.pipeline_cache);
	if (result != VK_SUCCESS && from_file)
	{
		/* the driver does not take this file: another build's, or damaged in a way the checks do not see */
		host_logf(HOST_LOG_WARN, "vk: the driver did not accept the pipeline cache %s (result %d); deleted, a new one is started",
			S.cache_file, (int)result);
		unlink(S.cache_file);
		info.initialDataSize = 0;
		info.pInitialData = NULL;
		result = vkCreatePipelineCache(B.device, &info, NULL, &B.pipeline_cache);
		from_file = 0;
	}
	if (!HOST_VK_CHECK(result))
		B.pipeline_cache = VK_NULL_HANDLE;
	else
		host_logf(HOST_LOG_INFO, "vk: pipeline cache %s: %s", S.cache_file, from_file ?
			"loaded from the device" : "new (none on the device for this driver)");
	S.pipeline_cache_size = from_file ? size - sizeof(header) : 0;
	free(data);
}

/* Saves come from two threads: the game thread (the surface's loss, five minutes, the game's exit) and the activity's
thread (SDL's background events, which come twice: "will" and "did"). One save at a time: two would write the same .tmp
file at once, and the one renamed last could be a mixture of both (the CRC would then throw the whole cache away at the
next start). */
static pthread_mutex_t save_lock = PTHREAD_MUTEX_INITIALIZER;

static void pipeline_cache_save_locked(const char *why);

void host_vk_pipeline_cache_save(const char *why)
{
	pthread_mutex_lock(&save_lock);
	pipeline_cache_save_locked(why);
	pthread_mutex_unlock(&save_lock);
}

static void pipeline_cache_save_locked(const char *why)
{
	unsigned saving;
	size_t size = 0;
	void *data;
	struct cache_header header;
	VkResult result;

	if (!S.started || !B.pipeline_cache || B.dead)
		return;
	/* nothing new since the last save: the file is as it was. (The compile thread counts under S.lock; what is counted
	from here on is in the next save, so the count is taken now and only that much is cleared after the write.) */
	{
		unsigned made;

		pthread_mutex_lock(&S.lock);
		made = S.made_since_save;
		pthread_mutex_unlock(&S.lock);
		if (!made)
			return;
		saving = made;
	}
	if (!HOST_VK_CHECK(vkGetPipelineCacheData(B.device, B.pipeline_cache, &size, NULL)) || !size)
		return;
	data = malloc(size);
	if (!data)
		return;
	result = vkGetPipelineCacheData(B.device, B.pipeline_cache, &size, data);
	if (result != VK_SUCCESS)
	{
		/* VK_INCOMPLETE: it grew between the two calls; the next save has it */
		free(data);
		return;
	}
	header.magic = CACHE_MAGIC;
	header.size = (uint32_t)size;
	header.crc = crc32_of(data, size);
	if (file_write(S.cache_file, &header, sizeof(header), data, size))
	{
		pthread_mutex_lock(&S.lock);
		S.made_since_save -= saving < S.made_since_save ? saving : S.made_since_save;
		pthread_mutex_unlock(&S.lock);
		S.last_save = now_ns();
		S.pipeline_cache_size = size;
		host_logf(HOST_LOG_INFO, "vk: pipeline cache saved (%s): %zu bytes", why, size);
	}
	else
	{
		host_logf(HOST_LOG_WARN, "vk: the pipeline cache could not be written to %s", S.cache_file);
	}
	free(data);
}

void host_vk_services_tick(void)
{
	if (!S.started)
		return;
	/* (read without the lock: a stale count only moves the save to the next tick) */
	if (S.made_since_save && now_ns() - S.last_save > 300ull * 1000000000ull)
		host_vk_pipeline_cache_save("five minutes");
}

void host_vk_exit(void)
{
	if (S.started)
	{
		pthread_mutex_lock(&S.lock);
		log_thread(1);
		pthread_mutex_unlock(&S.lock);
		host_vk_pipeline_cache_save("the game's exit");
	}
}

/* ---------- the app goes to the background

Android stops the game thread then (the surface's loss is found only when it returns), and may end the process while it is
away: the cache is saved from SDL's own event, which is sent on the thread that runs the activity's lifecycle. */

static bool lifecycle_watch(void *unused, SDL_Event *event)
{
	(void)unused;
	if (event->type == SDL_EVENT_WILL_ENTER_BACKGROUND || event->type == SDL_EVENT_DID_ENTER_BACKGROUND)
		host_vk_pipeline_cache_save("the app went to the background");
	return true;
}

/* ---------- start */

int host_vk_backend_ensure(void)
{
	int ready;

	if (!host_vk.instance)
		return 0;
	pthread_mutex_lock(&B.lock);
	if (B.state == 0)
		host_vk_device_ensure_locked();
	ready = B.state == 1 && !B.dead;
	pthread_mutex_unlock(&B.lock);
	return ready;
}

void host_vk_services_statistics(char *text, size_t size)
{
	unsigned queued_shaders = 0, queued_pipelines = 0;
	struct shader *shader;
	struct pipeline *pipeline;

	if (!S.started)
	{
		text[0] = 0;
		return;
	}
	pthread_mutex_lock(&S.lock);
	for (shader = S.shader_head; shader; shader = shader->next_queued)
		queued_shaders++;
	for (pipeline = S.pipeline_head; pipeline; pipeline = pipeline->next_queued)
		queued_pipelines++;
	snprintf(text, size, "shaders %u from the cache, %u compiled, %u failed, %u queued; pipelines %u made (%u at their draw), "
		"%u failed, %u queued; draws: %u ready, %u would be skipped for a shader, %u for a pipeline; pipeline cache %lu KB",
		S.from_disk, S.compiled, S.failed, queued_shaders, S.pipelines_made, S.pipelines_made_at_draw, S.pipelines_failed,
		queued_pipelines,
		B.counts.draws_ready, B.counts.draws_skipped_shader, B.counts.draws_skipped_pipeline, S.pipeline_cache_size / 1024);
	pthread_mutex_unlock(&S.lock);
}

/* the cache's folders, the files already there listed once into the table (they are opened only when asked for), the
other glslang versions' and generators' folders removed */
static void cache_scan(void)
{
	char spirv_root[700], keep[200];
	DIR *directory;
	struct dirent *entry;
	unsigned found = 0;

	snprintf(spirv_root, sizeof(spirv_root), "%s/spirv", S.root);
	snprintf(keep, sizeof(keep), "%s-%d", HOST_VK_GLSLANG_TAG, VK_SHADER_GENERATOR_VERSION);
	make_path(spirv_root);
	directory = opendir(spirv_root);
	if (directory)
	{
		while ((entry = readdir(directory)) != NULL)
		{
			char path[900];

			if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..") || !strcmp(entry->d_name, keep))
				continue;
			snprintf(path, sizeof(path), "%s/%s", spirv_root, entry->d_name);
			host_logf(HOST_LOG_INFO, "vk: removing the cache of another glslang or generator version: %s", path);
			remove_folder(path);
		}
		closedir(directory);
	}
	snprintf(S.spirv_folder, sizeof(S.spirv_folder), "%s/%s", spirv_root, keep);
	make_path(S.spirv_folder);
	directory = opendir(S.spirv_folder);
	if (directory)
	{
		while ((entry = readdir(directory)) != NULL)
		{
			unsigned long long hash;
			char letter;
			size_t length = strlen(entry->d_name);

			if (length != 21 || strcmp(entry->d_name + 17, ".spv") ||
				sscanf(entry->d_name, "%c%16llx", &letter, &hash) != 2 || (letter != 'v' && letter != 'f'))
				continue;
			if (shader_add(letter == 'v' ? VK_SHADER_STAGE_VERTEX : VK_SHADER_STAGE_PIXEL, hash, STATE_DISK))
				found++;
		}
		closedir(directory);
	}
	host_logf(HOST_LOG_INFO, "vk: shader cache %s: %u shaders on the device", S.spirv_folder, found);
}

/* the descriptor set layout and pipeline layout every draw's pipeline is made with (vk_shaders.h's bindings), the pipeline
cache, and the compile thread; called when the device is made, with B.lock held */
void host_vk_services_start(void)
{
	VkDescriptorSetLayoutBinding bindings[7];
	VkDescriptorSetLayoutCreateInfo set_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	VkPipelineLayoutCreateInfo layout_info = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	const char *internal = SDL_GetAndroidInternalStoragePath();
	pthread_attr_t attributes;
	pthread_t thread;
	unsigned index;

	if (S.started)
		return;
	if (!internal)
	{
		host_logf(HOST_LOG_ERROR, "vk: no internal storage for the shader cache (%s); shaders and pipelines are not made",
			SDL_GetError());
		return;
	}
	memset(bindings, 0, sizeof(bindings));
	bindings[0].binding = VK_BINDING_VERTEX_CONSTANTS;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[0].descriptorCount = 1;
	bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	bindings[1] = bindings[0];
	bindings[1].binding = VK_BINDING_VERTEX_PARAMETERS;
	bindings[2] = bindings[0];
	bindings[2].binding = VK_BINDING_PIXEL_PARAMETERS;
	bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	for (index = 0; index < 4; index++)
	{
		bindings[3 + index].binding = VK_BINDING_TEXTURE0 + index;
		bindings[3 + index].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bindings[3 + index].descriptorCount = 1;
		bindings[3 + index].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	}
	set_info.bindingCount = 7;
	set_info.pBindings = bindings;
	if (!HOST_VK_CHECK(vkCreateDescriptorSetLayout(B.device, &set_info, NULL, &B.draw_set_layout)))
		return;
	layout_info.setLayoutCount = 1;
	layout_info.pSetLayouts = &B.draw_set_layout;
	if (!HOST_VK_CHECK(vkCreatePipelineLayout(B.device, &layout_info, NULL, &B.draw_layout)))
		return;
	snprintf(S.root, sizeof(S.root), "%s/vk_cache", internal);
	snprintf(S.pipelines_folder, sizeof(S.pipelines_folder), "%s/pipelines", S.root);
	snprintf(S.failed_folder, sizeof(S.failed_folder), "%s/failed", S.root);
	make_path(S.pipelines_folder);
	make_path(S.failed_folder);
	pthread_mutex_lock(&S.lock);
	cache_scan();
	pthread_mutex_unlock(&S.lock);
	pipeline_cache_path();
	pipeline_cache_load();
	S.thread.last = now_ns();
	S.last_save = now_ns();
	pthread_attr_init(&attributes);
	pthread_attr_setstacksize(&attributes, 2 * 1024 * 1024);
	pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
	if (pthread_create(&thread, &attributes, compile_thread, NULL) != 0)
	{
		host_logf(HOST_LOG_ERROR, "vk: the compile thread could not be started; shaders and pipelines are not made");
		pthread_attr_destroy(&attributes);
		return;
	}
	pthread_attr_destroy(&attributes);
	S.started = 1;
	SDL_AddEventWatch(lifecycle_watch, NULL);
}
