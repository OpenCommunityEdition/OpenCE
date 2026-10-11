/* Remember generated shader sources and the pairs actually linked, so the
next visit compiles them before play. GL stays on the rendering thread;
only immutable source records reach the file writer. */
#include "web_shader_cache.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define CACHE_BUCKETS 1024
#define CACHE_SOURCES 1024
#define CACHE_PROGRAMS 4096
#define CACHE_SOURCE_BYTES (64 * 1024)
#define CACHE_TOTAL_BYTES (8 * 1024 * 1024)

struct cached_source
{
	struct cached_source *next;
	GLenum type;
	GLuint shader;
	uint32_t size;
	char text[1];
};

struct cached_program
{
	uint32_t vertex, fragment;
};

static struct cached_source *source_buckets[CACHE_BUCKETS];
static struct cached_source *sources[CACHE_SOURCES];
static struct cached_program programs[CACHE_PROGRAMS];
static uint32_t source_count, program_count, source_bytes;
static int initialized, writer_enabled;
static char cache_path[512], temporary_path[520];
static pthread_mutex_t writer_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t writer_changed = PTHREAD_COND_INITIALIZER;
static uint32_t published_sources, published_programs;
static unsigned long generation, written_generation;

static uint32_t checksum_bytes(uint32_t hash, const void *data, size_t size)
{
	const unsigned char *bytes = data;

	while (size--)
		hash = (hash ^ *bytes++) * UINT32_C(16777619);
	return hash;
}

/* Publish counts under the lock. Entries below those counts never change,
so the writer can release the lock before copying or doing any file I/O. */
static void cache_changed(void)
{
	if (!writer_enabled)
		return;
	pthread_mutex_lock(&writer_lock);
	published_sources = source_count;
	published_programs = program_count;
	generation++;
	pthread_cond_signal(&writer_changed);
	pthread_mutex_unlock(&writer_lock);
}

GLuint web_shader_cache_get(GLenum type, const char *source, const char *what)
{
	struct cached_source **bucket, *entry;
	size_t size = strlen(source);
	uint32_t hash = checksum_bytes(UINT32_C(2166136261) ^ type, source, size);
	GLuint shader;

	bucket = &source_buckets[hash % CACHE_BUCKETS];
	for (entry = *bucket; entry; entry = entry->next)
	{
		if (entry->type == type && !strcmp(entry->text, source))
			return entry->shader;
	}
	shader = xgpu_compile_shader(type, source, what);
	if (size > CACHE_SOURCE_BYTES || source_count == CACHE_SOURCES ||
		size > CACHE_TOTAL_BYTES - source_bytes)
		return shader;
	entry = malloc(sizeof(*entry) + size);
	if (!entry)
		return shader;
	entry->type = type;
	entry->shader = shader;
	entry->size = (uint32_t)size;
	memcpy(entry->text, source, size + 1);
	entry->next = *bucket;
	*bucket = entry;
	sources[source_count++] = entry;
	source_bytes += entry->size;
	cache_changed();
	return shader;
}

void web_shader_cache_linked(GLuint vertex_shader, GLuint fragment_shader)
{
	uint32_t index, vertex = CACHE_SOURCES, fragment = CACHE_SOURCES;

	if (!vertex_shader || !fragment_shader || program_count == CACHE_PROGRAMS)
		return;
	for (index = 0; index < source_count; index++)
	{
		if (sources[index]->type == GL_VERTEX_SHADER && sources[index]->shader == vertex_shader)
			vertex = index;
		if (sources[index]->type == GL_FRAGMENT_SHADER && sources[index]->shader == fragment_shader)
			fragment = index;
	}
	if (vertex == CACHE_SOURCES || fragment == CACHE_SOURCES)
		return;
	for (index = 0; index < program_count; index++)
	{
		if (programs[index].vertex == vertex && programs[index].fragment == fragment)
			return;
	}
	programs[program_count].vertex = vertex;
	programs[program_count++].fragment = fragment;
	cache_changed();
}

/* The file is little-endian records, followed by a checksum of every byte
before it. No pointers, GL handles, or host structure padding are saved. */
static int cache_bytes(FILE *file, void *bytes, size_t size, uint32_t *hash, int writing)
{
	if ((writing ? fwrite(bytes, 1, size, file) : fread(bytes, 1, size, file)) != size)
		return 0;
	*hash = checksum_bytes(*hash, bytes, size);
	return 1;
}

static int cache_word(FILE *file, uint32_t *word, uint32_t *hash, int writing)
{
	unsigned char bytes[4];

	if (writing)
	{
		bytes[0] = (unsigned char)*word;
		bytes[1] = (unsigned char)(*word >> 8);
		bytes[2] = (unsigned char)(*word >> 16);
		bytes[3] = (unsigned char)(*word >> 24);
	}
	if (!cache_bytes(file, bytes, sizeof(bytes), hash, writing))
		return 0;
	if (!writing)
		*word = (uint32_t)bytes[0] | (uint32_t)bytes[1] << 8 |
			(uint32_t)bytes[2] << 16 | (uint32_t)bytes[3] << 24;
	return 1;
}

static void cache_load(void (*prepare_program)(GLuint, GLuint))
{
	struct cached_source **loaded = NULL;
	struct cached_program *pairs = NULL;
	FILE *file = fopen(cache_path, "rb");
	char magic[8];
	uint32_t hash = UINT32_C(2166136261), saved_hash, ignored = 0;
	uint32_t count = 0, pair_count, bytes = 0, index;
	int valid = 0;

	if (!file)
	{
		platform_log("shader warm cache: no saved cache");
		return;
	}
	if (!cache_bytes(file, magic, sizeof(magic), &hash, 0) || memcmp(magic, "OCEGLSC1", 8) ||
		!cache_word(file, &count, &hash, 0) || count > CACHE_SOURCES ||
		!cache_word(file, &pair_count, &hash, 0) || pair_count > CACHE_PROGRAMS)
		goto done;
	loaded = calloc(count ? count : 1, sizeof(*loaded));
	pairs = calloc(pair_count ? pair_count : 1, sizeof(*pairs));
	if (!loaded || !pairs)
		goto done;
	for (index = 0; index < count; index++)
	{
		uint32_t type, size;

		if (!cache_word(file, &type, &hash, 0) ||
			(type != GL_VERTEX_SHADER && type != GL_FRAGMENT_SHADER) ||
			!cache_word(file, &size, &hash, 0) || size > CACHE_SOURCE_BYTES ||
			size > CACHE_TOTAL_BYTES - bytes)
			goto done;
		loaded[index] = calloc(1, sizeof(**loaded) + size);
		if (!loaded[index])
			goto done;
		loaded[index]->type = type;
		loaded[index]->size = size;
		bytes += size;
		if (!cache_bytes(file, loaded[index]->text, size, &hash, 0) ||
			memchr(loaded[index]->text, 0, size))
			goto done;
	}
	for (index = 0; index < pair_count; index++)
	{
		if (!cache_word(file, &pairs[index].vertex, &hash, 0) ||
			!cache_word(file, &pairs[index].fragment, &hash, 0) ||
			pairs[index].vertex >= count || pairs[index].fragment >= count ||
			loaded[pairs[index].vertex]->type != GL_VERTEX_SHADER ||
			loaded[pairs[index].fragment]->type != GL_FRAGMENT_SHADER)
			goto done;
	}
	if (!cache_word(file, &saved_hash, &ignored, 0) || saved_hash != hash || fgetc(file) != EOF || ferror(file))
		goto done;
	valid = 1;
	/* Validate the whole file before allowing any of it to reach GL. */
	for (index = 0; index < count; index++)
		loaded[index]->shader = web_shader_cache_get(loaded[index]->type, loaded[index]->text, "cached shader");
	for (index = 0; index < pair_count; index++)
	{
		GLuint vertex = loaded[pairs[index].vertex]->shader;
		GLuint fragment = loaded[pairs[index].fragment]->shader;

		if (vertex && fragment)
			prepare_program(vertex, fragment);
	}
	platform_log("shader warm cache: prepared %u sources and %u program pairs", (unsigned)count, (unsigned)pair_count);
	done:
	if (!valid)
		platform_log("shader warm cache: ignored invalid cache");
	fclose(file);
	if (loaded)
	{
		for (index = 0; index < count; index++)
			free(loaded[index]);
	}
	free(loaded);
	free(pairs);
}

static int cache_write(uint32_t count, uint32_t pair_count)
{
	FILE *file;
	char path[sizeof(temporary_path)];
	char magic[8] = {'O', 'C', 'E', 'G', 'L', 'S', 'C', '1'};
	uint32_t hash = UINT32_C(2166136261), ignored = 0, index;
	int descriptor, success = 0;

	memcpy(path, temporary_path, sizeof(path));
	descriptor = mkstemp(path);
	if (descriptor < 0)
		return 0;
	file = fdopen(descriptor, "wb");
	if (!file)
	{
		close(descriptor);
		unlink(path);
		return 0;
	}
	if (!cache_bytes(file, magic, sizeof(magic), &hash, 1) ||
		!cache_word(file, &count, &hash, 1) || !cache_word(file, &pair_count, &hash, 1))
		goto done;
	for (index = 0; index < count; index++)
	{
		struct cached_source *entry = sources[index];
		uint32_t type = entry->type;

		if (!cache_word(file, &type, &hash, 1) || !cache_word(file, &entry->size, &hash, 1) ||
			!cache_bytes(file, entry->text, entry->size, &hash, 1))
			goto done;
	}
	for (index = 0; index < pair_count; index++)
	{
		if (!cache_word(file, &programs[index].vertex, &hash, 1) ||
			!cache_word(file, &programs[index].fragment, &hash, 1))
			goto done;
	}
	success = cache_word(file, &hash, &ignored, 1) && !fflush(file) && !fsync(fileno(file));
	done:
	if (fclose(file))
		success = 0;
	if (success && !rename(path, cache_path))
		return 1;
	unlink(path);
	return 0;
}

static void *cache_writer(void *unused)
{
	int reported_failure = 0;

	(void)unused;
	for (;;)
	{
		struct timespec deadline;
		uint32_t count, pair_count;
		unsigned long snapshot;

		pthread_mutex_lock(&writer_lock);
		while (generation == written_generation)
			pthread_cond_wait(&writer_changed, &writer_lock);
		clock_gettime(CLOCK_REALTIME, &deadline);
		deadline.tv_sec++;
		while (!pthread_cond_timedwait(&writer_changed, &writer_lock, &deadline))
			;
		count = published_sources;
		pair_count = published_programs;
		snapshot = generation;
		pthread_mutex_unlock(&writer_lock);
		if (cache_write(count, pair_count))
			platform_log("shader warm cache: saved %u sources and %u program pairs", (unsigned)count, (unsigned)pair_count);
		else if (!reported_failure)
		{
			platform_log("shader warm cache: cannot save; rendering continues without persistent updates");
			reported_failure = 1;
		}
		pthread_mutex_lock(&writer_lock);
		written_generation = snapshot;
		pthread_mutex_unlock(&writer_lock);
	}
	return NULL;
}

void web_shader_cache_initialize(void (*prepare_program)(GLuint, GLuint))
{
	pthread_t thread;
	int length;

	if (initialized)
		return;
	initialized = 1;
	length = snprintf(cache_path, sizeof(cache_path), "%s/web-shaders-v1.bin", platform_save_root());
	if (length < 0 || (size_t)length >= sizeof(cache_path))
		return;
	snprintf(temporary_path, sizeof(temporary_path), "%s.XXXXXX", cache_path);
	cache_load(prepare_program);
	if (!pthread_create(&thread, NULL, cache_writer, NULL))
	{
		pthread_detach(thread);
		writer_enabled = 1;
	}
}
