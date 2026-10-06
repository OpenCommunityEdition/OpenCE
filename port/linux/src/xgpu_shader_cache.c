/*
XGPU_SHADER_CACHE.C

The renderer's shaders and programs by their GLSL text (xgpu.h), so that
nothing the game has drawn before is compiled or linked in the middle of a
frame. A draw's shaders are generated from the game's vertex programs and
pixel shader state at first use (d3d8_gl.c vertex_shader_get,
fragment_shader_get), and each pair is linked (program_get). Linking costs
milliseconds (about 20 ms a program on NVIDIA's driver, with its own cache
cold), so a new area, a new weapon or a new effect stopped the game for a
frame or more, hundreds of times a level.

Shaders are kept by a hash of their text, so a text compiled before is not
compiled again. Every text, and every pair linked, goes into shader_cache.bin
in the save root; at start-up (xgpu_shader_cache_warm, from gl_initialize)
the file's texts are all compiled and its pairs all linked, every request
made before any result is asked for, so that a driver that compiles in
parallel (GL_KHR_parallel_shader_compile, Mesa's and NVIDIA's) works on
them at once. A draw then finds its shaders and program made.

The file is a header, then records appended as they come: a text
('S', its GL shader type, hash and length, the text) or a pair ('P', the two
texts' hashes). A record cut short (a crash while writing) ends the file;
it is written again whole, through a temporary file renamed over it. Past
SHADER_CACHE_MAXIMUM_BYTES, nothing more is added.
*/

#include "xgpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SHADER_CACHE_FILE "shader_cache.bin"
/* (a new header for a new layout: the old file is then started again) */
#define SHADER_CACHE_HEADER "HXSC0001"
#define SHADER_CACHE_MAXIMUM_BYTES (48UL * 1024 * 1024)
#define SHADER_BUCKETS 1024
#define PAIR_BUCKETS 1024

struct cached_shader
{
	struct cached_shader *next;
	unsigned long long hash;
	GLenum type;
	GLuint shader;
	/* the text, while it waits to be compiled (warming) */
	char *source;
	unsigned long length;
	/* in the file already */
	BOOL saved;
};

struct cached_pair
{
	struct cached_pair *next;
	unsigned long long vertex_hash, fragment_hash;
};

static struct
{
	BOOL initialized;
	char path[1024];
	FILE *file;
	unsigned long file_bytes;
	BOOL full;
	struct cached_shader *shaders[SHADER_BUCKETS];
	struct cached_pair *pairs[PAIR_BUCKETS];
	/* a shader's GL name to its entry, for the pairs program_get links */
	struct cached_shader **by_name;
	unsigned long by_name_count;
} cache;

static unsigned long long milliseconds(void)
{
	struct timespec now;

	clock_gettime(CLOCK_MONOTONIC, &now);
	return (unsigned long long)now.tv_sec * 1000ULL + (unsigned long long)now.tv_nsec / 1000000ULL;
}

/* FNV-1a, 64 bits, of the text, the type mixed in */
static unsigned long long text_hash(GLenum type, const char *text, unsigned long length)
{
	unsigned long long hash = 14695981039346656037ULL ^ (unsigned long long)type;
	unsigned long index;

	for (index = 0; index < length; index++)
		hash = (hash ^ (unsigned char)text[index]) * 1099511628211ULL;
	return hash;
}

static struct cached_shader *shader_find(unsigned long long hash)
{
	struct cached_shader *entry;

	for (entry = cache.shaders[hash % SHADER_BUCKETS]; entry; entry = entry->next)
	{
		if (entry->hash == hash)
			return entry;
	}
	return NULL;
}

static struct cached_shader *shader_add(unsigned long long hash, GLenum type)
{
	struct cached_shader *entry = calloc(1, sizeof(*entry));

	if (!entry)
		return NULL;
	entry->hash = hash;
	entry->type = type;
	entry->next = cache.shaders[hash % SHADER_BUCKETS];
	cache.shaders[hash % SHADER_BUCKETS] = entry;
	return entry;
}

static void shader_name_set(struct cached_shader *entry)
{
	unsigned long name = entry->shader;

	/* (GL names are small numbers, given out in turn) */
	if (!name || name > 0x100000)
		return;
	if (name >= cache.by_name_count)
	{
		unsigned long count = cache.by_name_count ? cache.by_name_count : 1024;
		struct cached_shader **grown;

		while (count <= name)
			count *= 2;
		grown = realloc(cache.by_name, count * sizeof(*grown));
		if (!grown)
			return;
		memset(grown + cache.by_name_count, 0, (count - cache.by_name_count) * sizeof(*grown));
		cache.by_name = grown;
		cache.by_name_count = count;
	}
	cache.by_name[name] = entry;
}

static struct cached_shader *shader_by_name(GLuint name)
{
	return name < cache.by_name_count ? cache.by_name[name] : NULL;
}

static struct cached_pair **pair_bucket(unsigned long long vertex_hash, unsigned long long fragment_hash)
{
	return &cache.pairs[(vertex_hash * 31 + fragment_hash) % PAIR_BUCKETS];
}

static BOOL pair_known(unsigned long long vertex_hash, unsigned long long fragment_hash)
{
	struct cached_pair *pair;

	for (pair = *pair_bucket(vertex_hash, fragment_hash); pair; pair = pair->next)
	{
		if (pair->vertex_hash == vertex_hash && pair->fragment_hash == fragment_hash)
			return TRUE;
	}
	return FALSE;
}

static void pair_add(unsigned long long vertex_hash, unsigned long long fragment_hash)
{
	struct cached_pair *pair = calloc(1, sizeof(*pair));
	struct cached_pair **bucket = pair_bucket(vertex_hash, fragment_hash);

	if (!pair)
		return;
	pair->vertex_hash = vertex_hash;
	pair->fragment_hash = fragment_hash;
	pair->next = *bucket;
	*bucket = pair;
}

/* ---------- the file */

static BOOL write_shader_record(FILE *file, const struct cached_shader *entry, const char *source)
{
	unsigned char kind = 'S';
	unsigned int type = (unsigned int)entry->type, length = (unsigned int)entry->length;

	return fwrite(&kind, 1, 1, file) == 1 && fwrite(&type, sizeof(type), 1, file) == 1 &&
		fwrite(&entry->hash, sizeof(entry->hash), 1, file) == 1 && fwrite(&length, sizeof(length), 1, file) == 1 &&
		fwrite(source, 1, entry->length, file) == entry->length;
}

static BOOL write_pair_record(FILE *file, unsigned long long vertex_hash, unsigned long long fragment_hash)
{
	unsigned char kind = 'P';

	return fwrite(&kind, 1, 1, file) == 1 && fwrite(&vertex_hash, sizeof(vertex_hash), 1, file) == 1 &&
		fwrite(&fragment_hash, sizeof(fragment_hash), 1, file) == 1;
}

/* the whole file again, from what was read whole (its texts still held,
before warming), through a temporary file renamed over it */
static void file_rewrite(void)
{
	char temporary[1100];
	FILE *file;
	unsigned long bucket;
	BOOL success;

	snprintf(temporary, sizeof(temporary), "%s.tmp", cache.path);
	file = fopen(temporary, "wb");
	if (!file)
		return;
	success = fwrite(SHADER_CACHE_HEADER, 1, 8, file) == 8;
	for (bucket = 0; success && bucket < SHADER_BUCKETS; bucket++)
	{
		struct cached_shader *entry;

		for (entry = cache.shaders[bucket]; success && entry; entry = entry->next)
		{
			if (entry->source)
				success = write_shader_record(file, entry, entry->source);
		}
	}
	for (bucket = 0; success && bucket < PAIR_BUCKETS; bucket++)
	{
		struct cached_pair *pair;

		for (pair = cache.pairs[bucket]; success && pair; pair = pair->next)
			success = write_pair_record(file, pair->vertex_hash, pair->fragment_hash);
	}
	success = fclose(file) == 0 && success;
	if (success && rename(temporary, cache.path) == 0)
		return;
	remove(temporary);
}

/* reads the file's records; FALSE if it ended inside one, or was not ours */
static BOOL file_read(void)
{
	FILE *file = fopen(cache.path, "rb");
	char header[8];
	BOOL whole = TRUE;

	if (!file)
		return TRUE;
	if (fread(header, 1, 8, file) != 8 || memcmp(header, SHADER_CACHE_HEADER, 8))
	{
		fclose(file);
		return FALSE;
	}
	for (;;)
	{
		unsigned char kind;

		if (fread(&kind, 1, 1, file) != 1)
			break;
		if (kind == 'S')
		{
			unsigned int type, length;
			unsigned long long hash;
			char *source;
			struct cached_shader *entry;

			if (fread(&type, sizeof(type), 1, file) != 1 || fread(&hash, sizeof(hash), 1, file) != 1 ||
				fread(&length, sizeof(length), 1, file) != 1 || length > 1024 * 1024 ||
				(type != GL_VERTEX_SHADER && type != GL_FRAGMENT_SHADER))
			{
				whole = FALSE;
				break;
			}
			source = malloc(length + 1);
			if (!source || fread(source, 1, length, file) != length ||
				text_hash(type, source, length) != hash)
			{
				free(source);
				whole = FALSE;
				break;
			}
			source[length] = 0;
			if (shader_find(hash) || !(entry = shader_add(hash, type)))
			{
				free(source);
				continue;
			}
			entry->source = source;
			entry->length = length;
			entry->saved = TRUE;
		}
		else if (kind == 'P')
		{
			unsigned long long vertex_hash, fragment_hash;

			if (fread(&vertex_hash, sizeof(vertex_hash), 1, file) != 1 ||
				fread(&fragment_hash, sizeof(fragment_hash), 1, file) != 1)
			{
				whole = FALSE;
				break;
			}
			if (!pair_known(vertex_hash, fragment_hash))
				pair_add(vertex_hash, fragment_hash);
		}
		else
		{
			whole = FALSE;
			break;
		}
	}
	fclose(file);
	return whole;
}

static void file_open_for_appending(void)
{
	cache.file = fopen(cache.path, "ab");
	if (!cache.file)
		return;
	fseek(cache.file, 0, SEEK_END);
	cache.file_bytes = (unsigned long)ftell(cache.file);
	if (cache.file_bytes == 0 && fwrite(SHADER_CACHE_HEADER, 1, 8, cache.file) == 8)
		cache.file_bytes = 8;
}

/* (records reach the file at once: a crash keeps what came before it) */
static void file_flush(void)
{
	if (cache.file)
		fflush(cache.file);
}

static BOOL file_has_room(unsigned long bytes)
{
	if (!cache.file || cache.full)
		return FALSE;
	if (cache.file_bytes + bytes > SHADER_CACHE_MAXIMUM_BYTES)
	{
		cache.full = TRUE;
		platform_log("shader cache: %s is full (%lu MB): new shaders are no longer added", cache.path,
			SHADER_CACHE_MAXIMUM_BYTES >> 20);
		return FALSE;
	}
	cache.file_bytes += bytes;
	return TRUE;
}

static void cache_initialize(void)
{
	const char *root;

	if (cache.initialized)
		return;
	cache.initialized = TRUE;
	root = platform_save_root();
	if (!root || !*root)
		return;
	snprintf(cache.path, sizeof(cache.path), "%s/" SHADER_CACHE_FILE, root);
	if (!file_read())
	{
		/* (cut short, or another layout: what was read whole is kept) */
		file_rewrite();
	}
	file_open_for_appending();
}

/* ---------- the renderer's side (xgpu.h) */

GLuint xgpu_shader_cache_compile(GLenum type, const char *source, const char *what)
{
	unsigned long length = (unsigned long)strlen(source);
	unsigned long long hash = text_hash(type, source, length);
	struct cached_shader *entry;

	cache_initialize();
	entry = shader_find(hash);
	if (entry && entry->shader)
		return entry->shader;
	if (!entry && !(entry = shader_add(hash, type)))
		return xgpu_compile_shader(type, source, what);
	entry->length = length;
	entry->shader = xgpu_compile_shader(type, source, what);
	shader_name_set(entry);
	if (entry->shader && !entry->saved && file_has_room(17 + length) && write_shader_record(cache.file, entry, source))
	{
		entry->saved = TRUE;
		file_flush();
	}
	return entry->shader;
}

void xgpu_shader_cache_linked(GLuint vertex_shader, GLuint fragment_shader)
{
	struct cached_shader *vertex = shader_by_name(vertex_shader);
	struct cached_shader *fragment = shader_by_name(fragment_shader);

	if (!vertex || !fragment || vertex->shader != vertex_shader || fragment->shader != fragment_shader ||
		!vertex->saved || !fragment->saved || pair_known(vertex->hash, fragment->hash))
	{
		return;
	}
	pair_add(vertex->hash, fragment->hash);
	if (file_has_room(17) && write_pair_record(cache.file, vertex->hash, fragment->hash))
		file_flush();
}

void xgpu_shader_cache_warm(void (*linked)(GLuint vertex_shader, GLuint fragment_shader, GLuint program))
{
	unsigned long long start = milliseconds();
	unsigned long bucket, shaders = 0, programs = 0, failed = 0, pair_count = 0, index;
	struct
	{
		struct cached_shader *vertex, *fragment;
		GLuint program;
	} *pending;

	cache_initialize();
	/* every text compiled, none of them asked about yet */
	for (bucket = 0; bucket < SHADER_BUCKETS; bucket++)
	{
		struct cached_shader *entry;

		for (entry = cache.shaders[bucket]; entry; entry = entry->next)
		{
			const char *text = entry->source;

			if (!text || entry->shader)
				continue;
			entry->shader = glCreateShader(entry->type);
			glShaderSource(entry->shader, 1, &text, NULL);
			glCompileShader(entry->shader);
			shaders++;
		}
	}
	for (bucket = 0; bucket < PAIR_BUCKETS; bucket++)
	{
		struct cached_pair *pair;

		for (pair = cache.pairs[bucket]; pair; pair = pair->next)
			pair_count++;
	}
	pending = pair_count ? calloc(pair_count, sizeof(*pending)) : NULL;
	/* every pair linked, none of them asked about yet */
	for (bucket = 0; pending && bucket < PAIR_BUCKETS; bucket++)
	{
		struct cached_pair *pair;

		for (pair = cache.pairs[bucket]; pair; pair = pair->next)
		{
			struct cached_shader *vertex = shader_find(pair->vertex_hash);
			struct cached_shader *fragment = shader_find(pair->fragment_hash);

			if (!vertex || !fragment || !vertex->shader || !fragment->shader ||
				vertex->type != GL_VERTEX_SHADER || fragment->type != GL_FRAGMENT_SHADER)
			{
				continue;
			}
			pending[programs].vertex = vertex;
			pending[programs].fragment = fragment;
			pending[programs].program = glCreateProgram();
			glAttachShader(pending[programs].program, vertex->shader);
			glAttachShader(pending[programs].program, fragment->shader);
			glLinkProgram(pending[programs].program);
			programs++;
		}
	}
	/* then the results, in turn: the driver has had all of them to work on */
	for (bucket = 0; bucket < SHADER_BUCKETS; bucket++)
	{
		struct cached_shader *entry;

		for (entry = cache.shaders[bucket]; entry; entry = entry->next)
		{
			GLint status = 0;

			if (!entry->source)
				continue;
			free(entry->source);
			entry->source = NULL;
			if (!entry->shader)
				continue;
			glGetShaderiv(entry->shader, GL_COMPILE_STATUS, &status);
			if (status)
			{
				shader_name_set(entry);
				continue;
			}
			/* (a text this driver does not take: compiled again at first
			use, which logs why) */
			glDeleteShader(entry->shader);
			entry->shader = 0;
			failed++;
		}
	}
	for (index = 0; index < programs; index++)
	{
		GLint status = 0;

		if (pending[index].vertex->shader && pending[index].fragment->shader)
			glGetProgramiv(pending[index].program, GL_LINK_STATUS, &status);
		if (status)
		{
			linked(pending[index].vertex->shader, pending[index].fragment->shader, pending[index].program);
		}
		else
		{
			glDeleteProgram(pending[index].program);
			failed++;
		}
	}
	free(pending);
	if (shaders || programs)
	{
		platform_log("shader cache: %lu shaders and %lu programs made ready in %llu ms%s (%s)", shaders, programs,
			milliseconds() - start, failed ? ", some failed" : "", cache.path);
	}
}
