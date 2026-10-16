/*
MAP_SHARE_FILES.C

The files map sharing hashes (map_share.h): a map's version (its hash's
first digits), the hashes of the files this machine shares, worked out on a
thread of their own a file at a time, so that a map of hundreds of
megabytes costs the game no frames, and the names a build of before gave
maps (<name>_<hash>.map) given back.

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
	/* the maps' hashes kept this run (map_share_map_hash) */
	MAXIMUM_KNOWN_HASHES = 64,
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
	/* a map named for its hash, to give its name back (map_share_map_found) */
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

/* a map's hash, found this run: its file by its path, size and time
written, so that it is read once */
struct known_hash
{
	boolean used;
	char path[MAP_SHARE_PATH_SIZE];
	unsigned long size;
	unsigned long modified;
	byte hash[MAP_SHARE_HASH_BYTES];
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

	/* the maps' hashes found, and the next of them to replace */
	struct known_hash known[MAXIMUM_KNOWN_HASHES];
	short next_known;
	/* the level name a host's game was last given, and its map's version
	(map_share_level_name_choose) */
	char chosen_level[MAP_SHARE_PATH_SIZE];
	unsigned long chosen_version;

	/* the maps to give their names back, and those looked at already
	(folder and name) */
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

/* whether Windows takes a file of this name for a device: its name before
the first dot, spaces after it aside, is CON, PRN, AUX, NUL, CONIN$, CONOUT$,
or COM or LPT and a digit, in any case (CON.x_1a2b3c4d.map.part is the
console) */
static boolean name_device(
	char const *name)
{
	static char const *const devices[] = { "con", "prn", "aux", "nul", "conin$", "conout$" };
	long length = 0;
	short index;

	while (name[length] && name[length] != '.')
		length++;
	while (length && name[length - 1] == ' ')
		length--;
	if (length == 4 && (!_strnicmp(name, "com", 3) || !_strnicmp(name, "lpt", 3)) && name[3] >= '0' && name[3] <= '9')
		return TRUE;
	for (index = 0; index < (short)(sizeof(devices) / sizeof(devices[0])); index++)
	{
		if ((long)csstrlen(devices[index]) == length && !_strnicmp(name, devices[index], length))
			return TRUE;
	}
	return FALSE;
}

boolean map_share_name_safe(
	char const *name)
{
	long length = (long)csstrlen(name);
	long index;

	if (!length || name[0] == '.' || name[0] == ' ' || name[length - 1] == '.' || name[length - 1] == ' ' ||
		name_device(name))
	{
		return FALSE;
	}
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

/* ---------- maps' hashes */

static struct known_hash *known_hash_find(
	char const *path,
	unsigned long size,
	unsigned long modified)
{
	struct map_share_files_globals *globals = &map_share_files_globals;
	short index;

	for (index = 0; index < MAXIMUM_KNOWN_HASHES; index++)
	{
		struct known_hash *known = &globals->known[index];

		if (known->used && known->size == size && known->modified == modified && !csstrcasecmp(known->path, path))
			return known;
	}
	return NULL;
}

void map_share_map_hash_remember(
	char const *path,
	byte const *hash)
{
	struct map_share_files_globals *globals = &map_share_files_globals;
	struct known_hash *known;
	unsigned long size;
	unsigned long modified;
	short index;

	if (!file_stamp(path, &size, &modified))
		return;
	known = known_hash_find(path, size, modified);
	if (!known)
	{
		/* (the same path's of before, else the next in turn) */
		for (index = 0; index < MAXIMUM_KNOWN_HASHES; index++)
		{
			if (globals->known[index].used && !csstrcasecmp(globals->known[index].path, path))
				break;
		}
		if (index == MAXIMUM_KNOWN_HASHES)
		{
			index = globals->next_known;
			globals->next_known = (short)((index + 1) % MAXIMUM_KNOWN_HASHES);
		}
		known = &globals->known[index];
		csmemset(known, 0, sizeof(*known));
		known->used = TRUE;
		csstrncpy(known->path, path, sizeof(known->path) - 1);
		known->size = size;
		known->modified = modified;
	}
	csmemcpy(known->hash, hash, MAP_SHARE_HASH_BYTES);
}

boolean map_share_map_hash(
	char const *path,
	byte *hash)
{
	struct map_share_files_globals *globals = &map_share_files_globals;
	struct known_hash const *known;
	struct hash_job job;
	unsigned long size;
	unsigned long modified;
	short index;

	if (!file_stamp(path, &size, &modified) || size > MAXIMUM_FILE_BYTES)
		return FALSE;
	known = known_hash_find(path, size, modified);
	if (known)
	{
		csmemcpy(hash, known->hash, MAP_SHARE_HASH_BYTES);
		return TRUE;
	}
	/* (one the catalog hashed for sharing) */
	for (index = 0; index < MAXIMUM_SHARE_FILES; index++)
	{
		struct map_share_file const *file = &globals->files[index];

		if (file->used && file->ready && file->size == size && file->modified == modified &&
			!csstrcasecmp(file->path, path))
		{
			csmemcpy(hash, file->hash, MAP_SHARE_HASH_BYTES);
			map_share_map_hash_remember(path, hash);
			return TRUE;
		}
	}
	/* (read here: a map takes a fraction of a second) */
	csmemset(&job, 0, sizeof(job));
	csstrncpy(job.path, path, sizeof(job.path) - 1);
	hash_job_run(&job);
	free(job.piece_hashes);
	if (job.failed || job.size != size)
		return FALSE;
	csmemcpy(hash, job.hash, MAP_SHARE_HASH_BYTES);
	map_share_map_hash_remember(path, hash);
	return TRUE;
}

unsigned long map_share_hash_version(
	byte const *hash)
{
	unsigned long version = (unsigned long)hash[0] << 24 | (unsigned long)hash[1] << 16 |
		(unsigned long)hash[2] << 8 | (unsigned long)hash[3];

	/* (0 is a version a client checks nothing for) */
	return version ? version : 1;
}

boolean map_share_map_version(
	char const *path,
	unsigned long *version)
{
	byte hash[MAP_SHARE_HASH_BYTES];

	if (!map_share_map_hash(path, hash))
		return FALSE;
	*version = map_share_hash_version(hash);
	return TRUE;
}

/* ---------- names given back */

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

void map_share_map_found(
	char const *folder,
	char const *name)
{
	struct map_share_files_globals *globals = &map_share_files_globals;
	unsigned long key = naming_key(folder, name);
	char path[MAP_SHARE_PATH_SIZE];
	short base_length = map_share_name_base_length(name);
	short index;

	/* (the custom_maps folder's alone, a name ending in a hash, and none of
	its name without it there: one of that name is another version) */
	if (csstrcasecmp(folder, CUSTOM_EDITION_MAP_DIRECTORY) || !map_share_name_hashed(name) || naming_known(key) ||
		csstrlen(folder) + base_length + 4 >= MAP_SHARE_PATH_SIZE)
	{
		return;
	}
	csprintf(path, "%s%.*s.map", folder, (int)base_length, name);
	if (GetFileAttributesA(path) != (unsigned long)-1)
	{
		naming_remember(key);
		return;
	}
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

/* a map named for its hash hashed: given its own name back, with its
picture and description, if the hash is its name's (a name this game gave
it) */
static void naming_finish(
	struct naming const *naming,
	struct hash_job const *job)
{
	char digits[MAP_SHARE_NAME_DIGITS + 1];
	char base[MAP_SHARE_PATH_SIZE];
	char path[MAP_SHARE_PATH_SIZE];
	short base_length = map_share_name_base_length(naming->name);
	char const *loaded = custom_edition_cache_loaded_path();
	short index;

	csprintf(path, "%s%s.map", naming->folder, naming->name);
	/* (in use, loaded or being sent: named another time) */
	if (loaded && !csstrcasecmp(loaded, path))
		return;
	for (index = 0; index < MAXIMUM_SHARE_FILES; index++)
	{
		if (map_share_files_globals.files[index].used && !csstrcasecmp(map_share_files_globals.files[index].path, path))
			return;
	}
	naming_remember(naming_key(naming->folder, naming->name));
	if (job->failed)
	{
		error(_error_silent, "map sharing: '%s' could not be read to give it its name back", path);
		return;
	}
	map_share_hash_digits(job->hash, digits);
	if (csstrcmp(naming->name + base_length + 1, digits))
		return;
	csmemcpy(base, naming->name, base_length);
	base[base_length] = 0;
	if (!rename_beside(naming->folder, naming->name, base, ".map"))
	{
		error(_error_silent, "map sharing: '%s' was not renamed %s.map (that file exists, or this one is in use)",
			path, base);
		return;
	}
	rename_beside(naming->folder, naming->name, base, ".bmp");
	rename_beside(naming->folder, naming->name, base, ".txt");
	csprintf(path, "%s%s.map", naming->folder, base);
	map_share_map_hash_remember(path, job->hash);
	error(_error_silent, "map sharing: %s.map has its name back, %s.map", naming->name, base);
	custom_edition_maps_look_again();
}

/* ---------- a host's level name */

void map_share_level_name_choose(
	char *level_name,
	long size)
{
	struct map_share_files_globals *globals = &map_share_files_globals;
	char path[MAP_SHARE_PATH_SIZE];
	char name[MAP_SHARE_PATH_SIZE];
	char digits[MAP_SHARE_NAME_DIGITS + 1];
	byte hash[MAP_SHARE_HASH_BYTES];
	long length;

	globals->chosen_level[0] = 0;
	if (!level_name || !custom_edition_level_name(level_name) || !custom_edition_map_file_named(level_name, path) ||
		!map_share_map_hash(path, hash))
	{
		return;
	}
	/* the map's own name: its file's, without a hash it ends in (another
	version's, or one of before) if that is its hash's */
	csstrncpy(name, tag_name_strip_path(path), sizeof(name) - 1);
	name[sizeof(name) - 1] = 0;
	length = (long)csstrlen(name) - 4;
	if (length <= 0)
		return;
	name[length] = 0;
	map_share_hash_digits(hash, digits);
	if (map_share_name_hashed(name) && !csstrcmp(name + length - MAP_SHARE_NAME_DIGITS, digits))
		length -= MAP_SHARE_NAME_DIGITS + 1;
	if (length <= 0 || csstrlen(CUSTOM_EDITION_LEVEL_NAME_PREFIX) + length >= (unsigned long)size ||
		csstrlen(CUSTOM_EDITION_LEVEL_NAME_PREFIX) + length >= sizeof(globals->chosen_level))
	{
		return;
	}
	csprintf(level_name, "%s%.*s", CUSTOM_EDITION_LEVEL_NAME_PREFIX, (int)length, name);
	csstrncpy(globals->chosen_level, level_name, sizeof(globals->chosen_level) - 1);
	globals->chosen_version = map_share_hash_version(hash);
	error(_error_silent, "map sharing: the game's map is %s (%s, version %08lx)", level_name, path,
		globals->chosen_version);
}

boolean map_share_level_name_version(
	char const *level_name,
	unsigned long *version)
{
	struct map_share_files_globals *globals = &map_share_files_globals;

	if (!level_name || !globals->chosen_level[0] || csstrcasecmp(globals->chosen_level, level_name))
		return FALSE;
	*version = globals->chosen_version;
	return TRUE;
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
					map_share_map_hash_remember(file->path, file->hash);
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

	/* the next: a download's pieces, a file to share, then a map to give
	its name back */
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
