/*
MAP_SHARE_FILES.C

The files map sharing hashes (map_share.h): a map's name made from its
hash, and the hashes of the files this machine shares, worked out on a
thread of their own a file at a time, so that a map of hundreds of
megabytes costs the game no frames.

A file's hash is BLAKE2b's (32 bytes) of a label, its size and its pieces'
hashes (each piece MAP_SHARE_PIECE_BYTES of it, the last shorter, hashed with
BLAKE2b): a client that knows a file's hash, from the host, can check the
pieces' hashes any player sends it, and each piece as it arrives.
*/

/* ---------- headers */

#include "cseries.h"
#include "errors.h"
#include "tag_files/tag_files.h"
#include "bungie_net/common/thread.h"
#include "cache_file_formats.h"
#include "custom_edition_cache.h"
#include "custom_edition_maps.h"
#include "map_share.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xtl.h>

/* (the C library's allocator, not the game's tracked one (cseries.h): the
hashing thread allocates what the game's thread frees) */
#undef malloc
#undef free
#undef realloc

/* (p2p_crypto.c's) */
void p2p_blake2b_256(const void *data, unsigned long size, unsigned char *digest);

/* ---------- constants */

enum
{
	/* the files hashed for sharing at once: a game's map and its three
	resource maps, a download's, and the last few games' */
	MAXIMUM_SHARE_FILES = 16,
	/* the maps waiting to be named, and those named or given up on this
	run (not looked at again) */
	MAXIMUM_NAMING_QUEUE = 64,
	MAXIMUM_NAMED = 1024,
	/* the longest map file name the map lists take (custom_edition_maps.c:
	the level name, custom_maps\ and it, kept whole by the game engine) */
	MAXIMUM_MAP_NAME_LENGTH = CUSTOM_EDITION_MAXIMUM_LEVEL_NAME_LENGTH - (sizeof(CUSTOM_EDITION_LEVEL_NAME_PREFIX) - 1),
	/* the largest file hashed (the combined offset space's resource maps
	are smaller: custom_edition_cache.c) */
	MAXIMUM_FILE_BYTES = 0x7FFFFFFF,
};

/* what a hash is of: these bytes, the file's size (4 bytes, little-endian)
and its pieces' hashes */
static char const file_hash_label[] = "OpenCE map 1";
#define FILE_HASH_LABEL_BYTES 12

enum
{
	_hash_job_none,
	/* a catalog file's (map_share_file_hash) */
	_hash_job_file,
	/* a map to name for its hash (map_share_name_map) */
	_hash_job_name,
	/* a download's pieces (map_share_pieces_hash) */
	_hash_job_pieces,
};

/* ---------- structures */

/* the hashing thread's work: the file it reads, and what it finds */
struct hash_job
{
	char path[MAP_SHARE_PATH_SIZE];
	unsigned long size;
	long piece_count;
	byte *piece_hashes;
	byte hash[MAP_SHARE_HASH_BYTES];
	/* written by the thread: done (ready or failed) when it has exited */
	volatile long failed;
};

struct naming
{
	char folder[MAP_SHARE_PATH_SIZE];
	char name[MAP_SHARE_PATH_SIZE];
};

struct map_share_files_globals
{
	struct map_share_file files[MAXIMUM_SHARE_FILES];
	/* the file catalog's next to replace, when full */
	short next_file;

	/* the job running (_hash_job_*), its thread, and whose it is (a
	catalog index, or the pieces' owner) */
	short job_type;
	struct thread_reference *thread;
	struct hash_job job;
	long job_owner;
	struct naming job_naming;

	/* the maps to name, and those looked at already (folder and name) */
	short naming_count;
	struct naming naming_queue[MAXIMUM_NAMING_QUEUE];
	long named_count;
	unsigned long named_hashes[MAXIMUM_NAMED];

	/* the pieces asked for, and what was found for their owner */
	boolean pieces_wanted;
	char pieces_path[MAP_SHARE_PATH_SIZE];
	long pieces_owner;
	boolean pieces_done;
	long pieces_done_owner;
	byte *pieces_hashes;
	long pieces_count;
	unsigned long pieces_size;
};

/* ---------- globals */

static struct map_share_files_globals map_share_files_globals;

/* ---------- names */

static boolean hex_digit(
	char character)
{
	return (character >= '0' && character <= '9') || (character >= 'a' && character <= 'f');
}

boolean map_share_name_hashed(
	char const *name)
{
	long length = (long)csstrlen(name);
	long index;

	if (length < MAP_SHARE_NAME_DIGITS + 2 || name[length - MAP_SHARE_NAME_DIGITS - 1] != '_')
		return FALSE;
	for (index = length - MAP_SHARE_NAME_DIGITS; index < length; index++)
	{
		if (!hex_digit(name[index]))
			return FALSE;
	}
	return TRUE;
}

boolean map_share_name_safe(
	char const *name)
{
	long length = (long)csstrlen(name);
	long index;

	if (!length || name[0] == '.' || name[0] == ' ' || name[length - 1] == '.' || name[length - 1] == ' ')
		return FALSE;
	for (index = 0; index < length; index++)
	{
		unsigned char character = (unsigned char)name[index];

		if (character < ' ' || character > '~' || strchr("/\\:*?\"<>|", character))
			return FALSE;
	}
	return TRUE;
}

short map_share_name_base_length(
	char const *name)
{
	short length = (short)csstrlen(name);

	return map_share_name_hashed(name) ? (short)(length - MAP_SHARE_NAME_DIGITS - 1) : length;
}

void map_share_hash_digits(
	byte const *hash,
	char *digits)
{
	static char const hex[] = "0123456789abcdef";
	short index;

	for (index = 0; index < MAP_SHARE_NAME_DIGITS / 2; index++)
	{
		digits[index * 2] = hex[hash[index] >> 4];
		digits[index * 2 + 1] = hex[hash[index] & 15];
	}
	digits[MAP_SHARE_NAME_DIGITS] = 0;
}

/* ---------- hashes */

long map_share_piece_count(
	unsigned long size)
{
	return (long)((size + MAP_SHARE_PIECE_BYTES - 1) / MAP_SHARE_PIECE_BYTES);
}

void map_share_file_hash_of_pieces(
	unsigned long size,
	byte const *piece_hashes,
	long piece_count,
	byte *hash)
{
	unsigned long bytes = FILE_HASH_LABEL_BYTES + 4 + (unsigned long)piece_count * MAP_SHARE_HASH_BYTES;
	byte *data = malloc(bytes);
	short index;

	if (!data)
	{
		csmemset(hash, 0, MAP_SHARE_HASH_BYTES);
		return;
	}
	csmemcpy(data, file_hash_label, FILE_HASH_LABEL_BYTES);
	for (index = 0; index < 4; index++)
		data[FILE_HASH_LABEL_BYTES + index] = (byte)(size >> (index * 8));
	if (piece_count)
		csmemcpy(data + FILE_HASH_LABEL_BYTES + 4, piece_hashes, (unsigned long)piece_count * MAP_SHARE_HASH_BYTES);
	p2p_blake2b_256(data, bytes, hash);
	free(data);
}

/* the hashing thread: the job's file read a piece at a time */
static unsigned long __stdcall hash_job_run(
	void *input)
{
	struct hash_job *job = input;
	byte *piece = malloc(MAP_SHARE_PIECE_BYTES);
	FILE *stream = fopen(job->path, "rb");
	long size = -1;
	long index;

	job->piece_hashes = NULL;
	if (!piece || !stream || fseek(stream, 0, SEEK_END) != 0 || (size = ftell(stream)) < 0 ||
		fseek(stream, 0, SEEK_SET) != 0)
	{
		size = -1;
	}
	if (size >= 0)
	{
		job->size = (unsigned long)size;
		job->piece_count = map_share_piece_count(job->size);
		job->piece_hashes = malloc((unsigned long)MAX(job->piece_count, 1) * MAP_SHARE_HASH_BYTES);
	}
	for (index = 0; job->piece_hashes && index < job->piece_count; index++)
	{
		unsigned long bytes = MIN(MAP_SHARE_PIECE_BYTES, job->size - (unsigned long)index * MAP_SHARE_PIECE_BYTES);

		if (fread(piece, 1, bytes, stream) != bytes)
		{
			free(job->piece_hashes);
			job->piece_hashes = NULL;
			break;
		}
		p2p_blake2b_256(piece, bytes, job->piece_hashes + index * MAP_SHARE_HASH_BYTES);
	}
	if (stream)
		fclose(stream);
	free(piece);
	if (job->piece_hashes)
		map_share_file_hash_of_pieces(job->size, job->piece_hashes, job->piece_count, job->hash);
	else
		job->failed = TRUE;

	return 0;
}

static boolean hash_job_start(
	short type,
	char const *path,
	long owner)
{
	struct map_share_files_globals *globals = &map_share_files_globals;

	csmemset(&globals->job, 0, sizeof(globals->job));
	csstrncpy(globals->job.path, path, sizeof(globals->job.path) - 1);
	if (!create_thread(1, hash_job_run, &globals->job, &globals->thread))
	{
		globals->thread = NULL;
		return FALSE;
	}
	globals->job_type = type;
	globals->job_owner = owner;
	return TRUE;
}

/* the file's size and when it was last written, as the file system says */
static boolean file_stamp(
	char const *path,
	unsigned long *size,
	unsigned long *modified)
{
	WIN32_FILE_ATTRIBUTE_DATA data;

	if (!GetFileAttributesExA(path, GetFileExInfoStandard, &data) || data.nFileSizeHigh)
		return FALSE;
	*size = data.nFileSizeLow;
	*modified = data.ftLastWriteTime.dwLowDateTime ^ data.ftLastWriteTime.dwHighDateTime;
	return TRUE;
}

/* ---------- the catalog */

short map_share_file_hash(
	char const *path,
	short kind)
{
	struct map_share_files_globals *globals = &map_share_files_globals;
	struct map_share_file *file = NULL;
	unsigned long size;
	unsigned long modified;
	short index;

	if (!file_stamp(path, &size, &modified) || size > MAXIMUM_FILE_BYTES)
		return NONE;
	for (index = 0; index < MAXIMUM_SHARE_FILES; index++)
	{
		if (globals->files[index].used && !csstrcasecmp(globals->files[index].path, path))
		{
			file = &globals->files[index];
			break;
		}
	}
	/* (changed since: hashed again) */
	if (file && (file->ready || file->failed) && (file->size != size || file->modified != modified))
	{
		free(file->piece_hashes);
		file->piece_hashes = NULL;
		file->ready = FALSE;
		file->failed = FALSE;
	}
	if (!file)
	{
		/* a free one, else the next in turn not being hashed */
		for (index = 0; index < MAXIMUM_SHARE_FILES && globals->files[index].used; index++)
			;
		if (index == MAXIMUM_SHARE_FILES)
		{
			index = globals->next_file;
			if (globals->job_type == _hash_job_file && globals->job_owner == index)
				index = (short)((index + 1) % MAXIMUM_SHARE_FILES);
			globals->next_file = (short)((index + 1) % MAXIMUM_SHARE_FILES);
			free(globals->files[index].piece_hashes);
		}
		file = &globals->files[index];
		csmemset(file, 0, sizeof(*file));
		file->used = TRUE;
		csstrncpy(file->path, path, sizeof(file->path) - 1);
	}
	file->kind = kind;
	file->size = size;
	file->modified = modified;

	return (short)(file - globals->files);
}

short map_share_file_hashed(
	char const *path,
	short kind,
	unsigned long size,
	byte const *piece_hashes,
	long piece_count,
	byte const *hash)
{
	short index = map_share_file_hash(path, kind);
	struct map_share_file *file = map_share_file_get(index);

	if (!file || file->size != size || piece_count != map_share_piece_count(size))
		return index;
	free(file->piece_hashes);
	file->piece_hashes = malloc((unsigned long)MAX(piece_count, 1) * MAP_SHARE_HASH_BYTES);
	if (!file->piece_hashes)
		return index;
	csmemcpy(file->piece_hashes, piece_hashes, (unsigned long)piece_count * MAP_SHARE_HASH_BYTES);
	file->piece_count = piece_count;
	csmemcpy(file->hash, hash, MAP_SHARE_HASH_BYTES);
	file->ready = TRUE;
	file->failed = FALSE;
	return index;
}

struct map_share_file *map_share_file_get(
	short index)
{
	return index >= 0 && index < MAXIMUM_SHARE_FILES && map_share_files_globals.files[index].used ?
		&map_share_files_globals.files[index] : NULL;
}

short map_share_file_find(
	byte const *hash)
{
	short index;

	for (index = 0; index < MAXIMUM_SHARE_FILES; index++)
	{
		struct map_share_file const *file = &map_share_files_globals.files[index];

		if (file->used && file->ready && !csmemcmp(file->hash, hash, MAP_SHARE_HASH_BYTES))
			return index;
	}
	return NONE;
}

/* ---------- a download's pieces */

boolean map_share_pieces_hash(
	char const *path,
	long owner)
{
	struct map_share_files_globals *globals = &map_share_files_globals;

	if (globals->pieces_wanted || (globals->job_type == _hash_job_pieces))
		return globals->pieces_owner == owner;
	globals->pieces_wanted = TRUE;
	globals->pieces_owner = owner;
	csstrncpy(globals->pieces_path, path, sizeof(globals->pieces_path) - 1);
	globals->pieces_path[sizeof(globals->pieces_path) - 1] = 0;
	return TRUE;
}

boolean map_share_pieces_hashed(
	long owner,
	byte **piece_hashes,
	long *piece_count,
	unsigned long *size)
{
	struct map_share_files_globals *globals = &map_share_files_globals;

	if (!globals->pieces_done || globals->pieces_done_owner != owner)
		return FALSE;
	*piece_hashes = globals->pieces_hashes;
	*piece_count = globals->pieces_count;
	*size = globals->pieces_size;
	globals->pieces_done = FALSE;
	globals->pieces_hashes = NULL;
	return TRUE;
}

/* ---------- naming */

/* a folder and name's own number, to know it again */
static unsigned long naming_key(
	char const *folder,
	char const *name)
{
	unsigned long key = 2166136261UL;
	char const *text;

	for (text = folder; *text; text++)
		key = (key ^ (unsigned char)(*text | 0x20)) * 16777619UL;
	for (text = name; *text; text++)
		key = (key ^ (unsigned char)(*text | 0x20)) * 16777619UL;
	return key;
}

static boolean naming_known(
	unsigned long key)
{
	struct map_share_files_globals *globals = &map_share_files_globals;
	long index;

	for (index = 0; index < globals->named_count; index++)
	{
		if (globals->named_hashes[index] == key)
			return TRUE;
	}
	return FALSE;
}

static void naming_remember(
	unsigned long key)
{
	struct map_share_files_globals *globals = &map_share_files_globals;

	if (globals->named_count < MAXIMUM_NAMED)
		globals->named_hashes[globals->named_count++] = key;
}

void map_share_name_map(
	char const *folder,
	char const *name)
{
	struct map_share_files_globals *globals = &map_share_files_globals;
	unsigned long key = naming_key(folder, name);
	short index;

	/* (the custom_maps folder's alone: an install of Custom Edition is
	left as it is, for Custom Edition) */
	if (csstrcasecmp(folder, CUSTOM_EDITION_MAP_DIRECTORY) || map_share_name_hashed(name) || naming_known(key))
		return;
	for (index = 0; index < globals->naming_count; index++)
	{
		if (!csstrcasecmp(globals->naming_queue[index].name, name) &&
			!csstrcasecmp(globals->naming_queue[index].folder, folder))
		{
			return;
		}
	}
	if (globals->job_type == _hash_job_name && !csstrcasecmp(globals->job_naming.name, name) &&
		!csstrcasecmp(globals->job_naming.folder, folder))
	{
		return;
	}
	if (globals->naming_count == MAXIMUM_NAMING_QUEUE)
		return;
	csstrncpy(globals->naming_queue[globals->naming_count].folder, folder, MAP_SHARE_PATH_SIZE - 1);
	csstrncpy(globals->naming_queue[globals->naming_count].name, name, MAP_SHARE_PATH_SIZE - 1);
	globals->naming_count++;
}

/* <folder><from><extension> renamed <folder><to><extension>, if it is there
and that is not */
static boolean rename_beside(
	char const *folder,
	char const *from,
	char const *to,
	char const *extension)
{
	char from_path[MAP_SHARE_PATH_SIZE];
	char to_path[MAP_SHARE_PATH_SIZE];

	if (csstrlen(folder) + csstrlen(from) + csstrlen(extension) >= MAP_SHARE_PATH_SIZE ||
		csstrlen(folder) + csstrlen(to) + csstrlen(extension) >= MAP_SHARE_PATH_SIZE)
	{
		return FALSE;
	}
	csprintf(from_path, "%s%s%s", folder, from, extension);
	csprintf(to_path, "%s%s%s", folder, to, extension);
	return MoveFileA(from_path, to_path) != 0;
}

/* a map hashed: renamed <name>_<hash>.map (the name cut short to fit the
map lists), with its picture and description */
static void naming_finish(
	struct naming const *naming,
	struct hash_job const *job)
{
	char digits[MAP_SHARE_NAME_DIGITS + 1];
	char hashed[MAP_SHARE_PATH_SIZE];
	char path[MAP_SHARE_PATH_SIZE];
	short base_length = (short)csstrlen(naming->name);
	char const *loaded = custom_edition_cache_loaded_path();

	csprintf(path, "%s%s.map", naming->folder, naming->name);
	/* (in use: named another time) */
	if (loaded && !csstrcasecmp(loaded, path))
		return;
	naming_remember(naming_key(naming->folder, naming->name));
	if (job->failed)
	{
		error(_error_silent, "map sharing: '%s' could not be read to name it for its hash", path);
		return;
	}
	map_share_hash_digits(job->hash, digits);
	base_length = (short)MIN(base_length, MAXIMUM_MAP_NAME_LENGTH - MAP_SHARE_NAME_DIGITS - 1);
	csmemcpy(hashed, naming->name, base_length);
	csprintf(hashed + base_length, "_%s", digits);
	if (!rename_beside(naming->folder, naming->name, hashed, ".map"))
	{
		error(_error_silent, "map sharing: '%s' was not renamed %s.map (that file exists, or this one is in use)",
			path, hashed);
		return;
	}
	rename_beside(naming->folder, naming->name, hashed, ".bmp");
	rename_beside(naming->folder, naming->name, hashed, ".txt");
	error(_error_silent, "map sharing: %s.map is now %s.map, named for its hash", naming->name, hashed);
	custom_edition_maps_look_again();
}

void map_share_level_name_hash(
	char *level_name,
	long size)
{
	struct naming naming;
	struct hash_job job;
	char path[MAP_SHARE_PATH_SIZE];
	char const *name;
	char const *hashed;

	if (!level_name || !custom_edition_level_name(level_name))
		return;
	name = tag_name_strip_path(level_name);
	if (map_share_name_hashed(name) || !custom_edition_map_file_path(level_name, path))
		return;
	/* (the file's own name, which may have its hash already) */
	hashed = tag_name_strip_path(path);
	if (_strnicmp(path, CUSTOM_EDITION_MAP_DIRECTORY, csstrlen(CUSTOM_EDITION_MAP_DIRECTORY)) ||
		strchr(path + csstrlen(CUSTOM_EDITION_MAP_DIRECTORY), '\\'))
	{
		return;
	}
	if (!map_share_name_hashed(name) && csstrlen(hashed) > 4 && csstrlen(hashed) - 4 == csstrlen(name) &&
		!_strnicmp(hashed, name, csstrlen(name)))
	{
		/* named now: hashed here (a map's hash takes a fraction of a second
		to work out) and renamed, unless it is loaded */
		csmemset(&naming, 0, sizeof(naming));
		csmemset(&job, 0, sizeof(job));
		csstrncpy(naming.folder, CUSTOM_EDITION_MAP_DIRECTORY, sizeof(naming.folder) - 1);
		csstrncpy(naming.name, name, sizeof(naming.name) - 1);
		csstrncpy(job.path, path, sizeof(job.path) - 1);
		hash_job_run(&job);
		naming_finish(&naming, &job);
		free(job.piece_hashes);
		if (!custom_edition_map_file_path(level_name, path))
			return;
		hashed = tag_name_strip_path(path);
	}
	/* the level name of the file found: custom_maps\<name>_<hash> */
	if (csstrlen(hashed) <= 4 || csstrlen(CUSTOM_EDITION_LEVEL_NAME_PREFIX) + csstrlen(hashed) - 4 >= (unsigned long)size)
		return;
	csprintf(level_name, "%s%.*s", CUSTOM_EDITION_LEVEL_NAME_PREFIX, (int)(csstrlen(hashed) - 4), hashed);
	if (!map_share_name_hashed(tag_name_strip_path(level_name)))
		return;
	error(_error_silent, "map sharing: the game's map is %s", level_name);
}

/* ---------- the hashing thread's work */

void map_share_files_update(
	void)
{
	struct map_share_files_globals *globals = &map_share_files_globals;
	short index;

	if (globals->thread)
	{
		if (!thread_has_exited(globals->thread))
			return;
		dispose_thread(globals->thread);
		globals->thread = NULL;
		switch (globals->job_type)
		{
		case _hash_job_file:
		{
			struct map_share_file *file = map_share_file_get((short)globals->job_owner);

			/* (replaced meanwhile: the catalog's file is another's now) */
			if (file && !csstrcasecmp(file->path, globals->job.path))
			{
				/* (changed meanwhile: hashed again) */
				if (globals->job.failed || globals->job.size != file->size)
				{
					file->failed = globals->job.failed;
					free(globals->job.piece_hashes);
				}
				else
				{
					file->piece_count = globals->job.piece_count;
					file->piece_hashes = globals->job.piece_hashes;
					csmemcpy(file->hash, globals->job.hash, MAP_SHARE_HASH_BYTES);
					file->ready = TRUE;
				}
			}
			else
			{
				free(globals->job.piece_hashes);
			}
			break;
		}
		case _hash_job_name:
			naming_finish(&globals->job_naming, &globals->job);
			free(globals->job.piece_hashes);
			break;
		case _hash_job_pieces:
			free(globals->pieces_hashes);
			globals->pieces_done = TRUE;
			globals->pieces_done_owner = globals->job_owner;
			globals->pieces_hashes = globals->job.failed ? NULL : globals->job.piece_hashes;
			globals->pieces_count = globals->job.failed ? 0 : globals->job.piece_count;
			globals->pieces_size = globals->job.size;
			break;
		}
		globals->job.piece_hashes = NULL;
		globals->job_type = _hash_job_none;
	}

	/* the next: a download's pieces, a file to share, then a map to name */
	if (globals->pieces_wanted)
	{
		globals->pieces_wanted = FALSE;
		if (!hash_job_start(_hash_job_pieces, globals->pieces_path, globals->pieces_owner))
		{
			globals->pieces_done = TRUE;
			globals->pieces_done_owner = globals->pieces_owner;
			globals->pieces_hashes = NULL;
			globals->pieces_count = 0;
		}
		return;
	}
	for (index = 0; index < MAXIMUM_SHARE_FILES; index++)
	{
		struct map_share_file *file = &globals->files[index];

		if (file->used && !file->ready && !file->failed)
		{
			if (!hash_job_start(_hash_job_file, file->path, index))
				file->failed = TRUE;
			return;
		}
	}
	while (globals->naming_count > 0)
	{
		char path[MAP_SHARE_PATH_SIZE];

		globals->job_naming = globals->naming_queue[0];
		globals->naming_count--;
		csmemmove(&globals->naming_queue[0], &globals->naming_queue[1],
			globals->naming_count * sizeof(globals->naming_queue[0]));
		if (csstrlen(globals->job_naming.folder) + csstrlen(globals->job_naming.name) + 4 >= MAP_SHARE_PATH_SIZE)
			continue;
		csprintf(path, "%s%s.map", globals->job_naming.folder, globals->job_naming.name);
		if (hash_job_start(_hash_job_name, path, NONE))
			return;
	}
}
