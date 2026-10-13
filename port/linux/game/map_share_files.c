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
	/* the maps waiting to be hashed for their names, and those hashed or
	given up on (not hashed again while their files stay the same; those
	hashed kept in NAMED_FILE for the next runs) */
	MAXIMUM_NAMING_QUEUE = 64,
	/* (every map the map lists hold) */
	MAXIMUM_NAMED = CUSTOM_EDITION_MAPS_MAXIMUM,
	/* the longest map file name the map lists take (custom_edition_maps.c:
	the level name, custom_maps\ and it, kept whole by the game engine) */
	MAXIMUM_MAP_NAME_LENGTH = CUSTOM_EDITION_MAXIMUM_LEVEL_NAME_LENGTH - (sizeof(CUSTOM_EDITION_LEVEL_NAME_PREFIX) - 1),
	/* the largest file hashed (the combined offset space's resource maps
	are smaller: custom_edition_cache.c) */
	MAXIMUM_FILE_BYTES = 0x7FFFFFFF,
};

/* the maps hashed for their names, kept from one run to the next (in the
save root: a line a file, its naming_key, size, time and digits, in hexadecimal
and decimal), so that a map is hashed once as long as its file stays the
same, as when the maps were renamed for their hash */
#define NAMED_FILE "z:\\map_hashes.txt"
#define NAMED_FILE_FORMAT "# OpenCE map hashes 1"

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
	/* the file's size and time before it was hashed: its digits are kept
	only if they are the same after (a file being copied over is hashed
	again once it is whole) */
	unsigned long size;
	unsigned long modified;
};

/* a map file hashed for its name: by its folder and name (naming_key), its
size and when it was written then, and the digits it is named by (none:
it could not be read) */
struct named
{
	unsigned long key;
	unsigned long size;
	unsigned long modified;
	char digits[MAP_SHARE_NAME_DIGITS + 1];
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
	struct named named[MAXIMUM_NAMED];
	boolean named_loaded;
	/* digits worked out since NAMED_FILE was written */
	boolean named_dirty;
	/* a map the queue had no room for: the map lists offer theirs again
	once it is empty */
	boolean naming_overflowed;

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

/* the maps hashed in earlier runs (NAMED_FILE), once */
static void named_load(
	void)
{
	struct map_share_files_globals *globals = &map_share_files_globals;
	char line[128];
	FILE *stream;

	if (globals->named_loaded)
		return;
	globals->named_loaded = TRUE;
	stream = fopen(NAMED_FILE, "rb");
	if (!stream)
		return;
	if (fgets(line, sizeof(line), stream) && !csstrncmp(line, NAMED_FILE_FORMAT, csstrlen(NAMED_FILE_FORMAT)))
	{
		while (globals->named_count < MAXIMUM_NAMED && fgets(line, sizeof(line), stream))
		{
			struct named *named = &globals->named[globals->named_count];
			char digits[16];
			short index;

			csmemset(named, 0, sizeof(*named));
			if (sscanf(line, "%lx %lu %lu %15s", &named->key, &named->size, &named->modified, digits) != 4 ||
				csstrlen(digits) != MAP_SHARE_NAME_DIGITS)
			{
				continue;
			}
			for (index = 0; index < MAP_SHARE_NAME_DIGITS; index++)
			{
				if (!((digits[index] >= '0' && digits[index] <= '9') || (digits[index] >= 'a' && digits[index] <= 'f')))
					break;
			}
			if (index < MAP_SHARE_NAME_DIGITS)
				continue;
			csstrcpy(named->digits, digits);
			globals->named_count++;
		}
	}
	fclose(stream);
}

/* the maps hashed kept for the next runs (NAMED_FILE): those given up on
are tried again then */
static void named_save(
	void)
{
	struct map_share_files_globals *globals = &map_share_files_globals;
	FILE *stream = fopen(NAMED_FILE, "wb");
	long index;

	if (!stream)
		return;
	fprintf(stream, "%s\n", NAMED_FILE_FORMAT);
	for (index = 0; index < globals->named_count; index++)
	{
		struct named const *named = &globals->named[index];

		if (named->digits[0])
			fprintf(stream, "%08lx %lu %lu %s\n", named->key, named->size, named->modified, named->digits);
	}
	fclose(stream);
}

/* the map file's entry, if it was hashed (or given up on) as it is now
(its size and time the same) */
static struct named *naming_find(
	char const *folder,
	char const *name)
{
	struct map_share_files_globals *globals = &map_share_files_globals;
	unsigned long key = naming_key(folder, name);
	char path[MAP_SHARE_PATH_SIZE];
	unsigned long size, modified;
	long index;

	named_load();
	if (csstrlen(folder) + csstrlen(name) + 4 >= MAP_SHARE_PATH_SIZE)
		return NULL;
	csprintf(path, "%s%s.map", folder, name);
	if (!file_stamp(path, &size, &modified))
		return NULL;
	for (index = 0; index < globals->named_count; index++)
	{
		struct named *named = &globals->named[index];

		if (named->key == key && named->size == size && named->modified == modified)
			return named;
	}
	return NULL;
}

/* the hash's digits of the map file <folder><name>.map remembered (none:
it could not be read), as the file was when hashed (size, modified): its
entry of before (another version) replaced */
static void naming_remember(
	char const *folder,
	char const *name,
	unsigned long size,
	unsigned long modified,
	char const *digits)
{
	struct map_share_files_globals *globals = &map_share_files_globals;
	unsigned long key = naming_key(folder, name);
	struct named *named = NULL;
	long index;

	named_load();
	for (index = 0; index < globals->named_count && !named; index++)
	{
		if (globals->named[index].key == key)
			named = &globals->named[index];
	}
	if (!named)
	{
		/* (when full, the oldest goes) */
		if (globals->named_count == MAXIMUM_NAMED)
		{
			csmemmove(&globals->named[0], &globals->named[1], (MAXIMUM_NAMED - 1) * sizeof(globals->named[0]));
			globals->named_count--;
		}
		named = &globals->named[globals->named_count++];
		csmemset(named, 0, sizeof(*named));
		named->key = key;
	}
	named->size = size;
	named->modified = modified;
	csstrncpy(named->digits, digits, MAP_SHARE_NAME_DIGITS);
	named->digits[MAP_SHARE_NAME_DIGITS] = 0;
	if (named->digits[0])
		globals->named_dirty = TRUE;
}

/* the naming's file's size and time now, into it; FALSE if it is not there */
static boolean naming_stamp(
	struct naming *naming)
{
	char path[MAP_SHARE_PATH_SIZE];

	if (csstrlen(naming->folder) + csstrlen(naming->name) + 4 >= MAP_SHARE_PATH_SIZE)
		return FALSE;
	csprintf(path, "%s%s.map", naming->folder, naming->name);
	return file_stamp(path, &naming->size, &naming->modified);
}

void map_share_name_map(
	char const *folder,
	char const *name)
{
	struct map_share_files_globals *globals = &map_share_files_globals;
	short index;

	/* (the maps folders': a game's level name finds an install of Custom
	Edition's maps by their hash too, though they are not shared) */
	if ((csstrcasecmp(folder, CUSTOM_EDITION_MAP_DIRECTORY) && csstrcasecmp(folder, CUSTOM_EDITION_INSTALL_MAP_DIRECTORY)) ||
		map_share_name_hashed(name) || naming_find(folder, name))
	{
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
	{
		globals->naming_overflowed = TRUE;
		return;
	}
	csmemset(&globals->naming_queue[globals->naming_count], 0, sizeof(globals->naming_queue[0]));
	csstrncpy(globals->naming_queue[globals->naming_count].folder, folder, MAP_SHARE_PATH_SIZE - 1);
	csstrncpy(globals->naming_queue[globals->naming_count].name, name, MAP_SHARE_PATH_SIZE - 1);
	if (naming_stamp(&globals->naming_queue[globals->naming_count]))
		globals->naming_count++;
}

/* a map hashed: the digits it is named by remembered (its file is not
renamed: a player's maps are theirs, and other programs use them) */
static void naming_finish(
	struct naming const *naming,
	struct hash_job const *job)
{
	char digits[MAP_SHARE_NAME_DIGITS + 1];
	struct naming now = *naming;

	if (job->failed)
	{
		error(_error_silent, "map sharing: '%s%s.map' could not be read to name it for its hash", naming->folder,
			naming->name);
		naming_remember(naming->folder, naming->name, naming->size, naming->modified, "");
		return;
	}
	/* (changed while it was hashed: hashed again when it is next asked for) */
	if (!naming_stamp(&now) || now.size != naming->size || now.modified != naming->modified)
	{
		error(_error_silent, "map sharing: '%s%s.map' changed while it was hashed", naming->folder, naming->name);
		return;
	}
	map_share_hash_digits(job->hash, digits);
	naming_remember(naming->folder, naming->name, naming->size, naming->modified, digits);
}

boolean map_share_name_digits(
	char const *folder,
	char const *name,
	char *digits)
{
	struct named *named = naming_find(folder, name);
	struct naming naming;
	struct hash_job job;

	if (!named)
	{
		/* (hashed here: a map's hash takes a fraction of a second) */
		if (csstrlen(folder) + csstrlen(name) + 4 >= MAP_SHARE_PATH_SIZE)
			return FALSE;
		csmemset(&naming, 0, sizeof(naming));
		csmemset(&job, 0, sizeof(job));
		csstrncpy(naming.folder, folder, sizeof(naming.folder) - 1);
		csstrncpy(naming.name, name, sizeof(naming.name) - 1);
		if (!naming_stamp(&naming))
			return FALSE;
		csprintf(job.path, "%s%s.map", folder, name);
		hash_job_run(&job);
		naming_finish(&naming, &job);
		free(job.piece_hashes);
		named = naming_find(folder, name);
	}
	if (!named || !named->digits[0])
		return FALSE;
	csstrcpy(digits, named->digits);
	return TRUE;
}

void map_share_level_name_hash(
	char *level_name,
	long size)
{
	char path[MAP_SHARE_PATH_SIZE];
	char file[MAP_SHARE_PATH_SIZE];
	char digits[MAP_SHARE_NAME_DIGITS + 1];
	char const *name;
	size_t length;

	if (!level_name || !custom_edition_level_name(level_name))
		return;
	name = tag_name_strip_path(level_name);
	if (map_share_name_hashed(name) || !custom_edition_map_file_path(level_name, path))
		return;
	/* (the custom_maps folder's alone: an install of Custom Edition's maps
	are not shared) */
	if (_strnicmp(path, CUSTOM_EDITION_MAP_DIRECTORY, csstrlen(CUSTOM_EDITION_MAP_DIRECTORY)) ||
		strchr(path + csstrlen(CUSTOM_EDITION_MAP_DIRECTORY), '\\'))
	{
		return;
	}
	/* the file's own name, which may have its hash already (a map downloaded,
	or renamed by an earlier build) */
	length = csstrlen(path + csstrlen(CUSTOM_EDITION_MAP_DIRECTORY));
	if (length <= 4 || length - 4 >= sizeof(file))
		return;
	csprintf(file, "%.*s", (int)(length - 4), path + csstrlen(CUSTOM_EDITION_MAP_DIRECTORY));
	if (map_share_name_hashed(file))
	{
		if (csstrlen(CUSTOM_EDITION_LEVEL_NAME_PREFIX) + csstrlen(file) < (unsigned long)size)
			csprintf(level_name, "%s%s", CUSTOM_EDITION_LEVEL_NAME_PREFIX, file);
	}
	else
	{
		/* named for its hash, the file as it is (custom_edition_cache.c finds
		<name>.map for <name>_<hash> by the hash); a name too long to be kept
		whole with the hash cut short to fit, as the maps were renamed before
		(custom_edition_cache.c finds the file whose name starts so) */
		short base_length = (short)MIN(csstrlen(file), MAXIMUM_MAP_NAME_LENGTH - MAP_SHARE_NAME_DIGITS - 1);

		if (csstrlen(CUSTOM_EDITION_LEVEL_NAME_PREFIX) + base_length + 1 + MAP_SHARE_NAME_DIGITS >= (unsigned long)size ||
			!map_share_name_digits(CUSTOM_EDITION_MAP_DIRECTORY, file, digits))
		{
			return;
		}
		csprintf(level_name, "%s%.*s_%s", CUSTOM_EDITION_LEVEL_NAME_PREFIX, (int)base_length, file, digits);
	}
	error(_error_silent, "map sharing: the game's map is %s", level_name);
}

/* ---------- the hashing thread's work */

void map_share_files_update(
	void)
{
	struct map_share_files_globals *globals = &map_share_files_globals;
	short index;

	/* (the maps hashed kept once no more are waiting, not after each; and
	the map lists asked for the maps there was no room for) */
	if (!globals->naming_count && globals->job_type != _hash_job_name)
	{
		if (globals->named_dirty)
		{
			named_save();
			globals->named_dirty = FALSE;
		}
		if (globals->naming_overflowed)
		{
			globals->naming_overflowed = FALSE;
			custom_edition_maps_look_again();
		}
	}

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
