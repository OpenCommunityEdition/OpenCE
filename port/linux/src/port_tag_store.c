/*
PORT_TAG_STORE.C

How a Tag Injector profile's folder keeps its tags' bundles
(port_tag_store.h). A bundle's tags are a few hundred kilobytes; its file
data, the pixels, samples and vertices its tags name, is the rest, tens of
megabytes, and the tags of one map name much of the same (their effects,
sounds, shaders and bitmaps): a bundle each held a copy. Here a bundle's
tags go in a tag file of its own, and each region of its file data (a
bitmap's pixels, a sound's samples, a model part's vertices or strip:
tag_bundle.h) in the folder's pack, once, found by its contents: its size,
its 64-bit FNV-1a hash and its CRC-32, which the tag file names it by. The
tags and each region are kept compressed by zlib when that makes them an
eighth smaller (a Custom Edition map's uncompressed bitmaps are; its DXT
bitmaps and ADPCM samples hardly).

	tag file (`name`.fbrtag): 'FABR', version, the bundle's size, its tags'
	  size (where its file data starts), their stored size, their flags,
	  the region count, 0; each region (its offset in file data, size, hash
	  low, high, CRC, 0); then the tags, stored
	pack (faber_vault.bin): 'FABV', version, its generation, 0; then each
	  region: a record (size, hash low, high, CRC, stored size, flags, 0, 0)
	  and its stored bytes
	index (faber_vault.idx): 'FABX', version, the pack's generation, the entry
	  count; then each region the pack has (size, hash low, high, CRC, where
	  its stored bytes are, stored size, flags, 0), in their keys' order

All little-endian, each field four bytes. The pack is what is kept; the
index is where its regions are, written after the pack, and read again from
the pack (its records) when it is not the pack's (another generation, or a
region past the pack's end): a pack written again (port_tag_store_collect)
replaces the old, then the index is written. The bytes between a bundle's
regions are zeros, as the bundle's writer pads them (tag_bundle.c).

Compression is the game's zlib's (source/memory/zlib, 1.1.3: the port's
has inflate alone), inflation the port's (port/third_party/zlib): the
files are untrusted, and may come from elsewhere.
*/

#include "platform.h"
#include "port_tag_profiles.h"
#include "port_tag_store.h"
#include "posix.h"

#include "zlib_prefixed.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* the game's zlib's (source/memory/zlib: its names are not prefixed, where
zlib_prefixed.h makes this one z_compress2, which the port's has not got) */
#undef compress2
int compress2(Bytef *destination, uLongf *destination_length, const Bytef *source, uLong source_length, int level);

/* ---------- constants */

/* "FABR", "FABV", "FABX" */
#define TAG_FILE_SIGNATURE 0x52424146UL
#define PACK_SIGNATURE 0x56424146UL
#define INDEX_SIGNATURE 0x58424146UL
#define STORE_VERSION 1

#define TAG_FILE_HEADER_BYTES 32
#define TAG_FILE_REGION_BYTES 24
#define PACK_HEADER_BYTES 16
#define PACK_RECORD_BYTES 32
#define INDEX_HEADER_BYTES 16
#define INDEX_ENTRY_BYTES 32

#define PACK_NAME "faber_vault.bin"
#define INDEX_NAME "faber_vault.idx"

/* stored by zlib (else as it is) */
#define STORED_DEFLATED 1
/* zlib's level: its default, a few times faster than its best for a few
percent */
#define COMPRESSION_LEVEL 6

/* a pack, a tag file and a region at most (a bundle is at most 2 GB:
port_tag_profiles.c) */
#define PACK_LIMIT 0x7F000000UL
#define TAG_FILE_LIMIT 0x10000000UL
#define REGION_LIMIT 0x7F000000UL

#define MAXIMUM_STORE_HANDLES 64
#define MAXIMUM_STORE_FOLDERS 4
#define MAXIMUM_PATH_LENGTH 1024
#define COPY_CHUNK 0x10000

/* ---------- structures */

/* a region by its contents */
struct store_key
{
	unsigned long size;
	unsigned long hash_low;
	unsigned long hash_high;
	unsigned long crc;
};

/* a region the pack has: where its stored bytes are */
struct store_entry
{
	struct store_key key;
	unsigned long offset;
	unsigned long stored_size;
	unsigned long flags;
};

/* a folder's pack and index, while bundles of it are open to read */
struct store_folder
{
	char path[MAXIMUM_PATH_LENGTH];
	FILE *pack;
	struct store_entry *entries;
	long entry_count;
	long users;
};

/* a region of an open bundle's file data, and its entry in its folder's */
struct store_place
{
	unsigned long offset;
	unsigned long size;
	long entry;
};

struct store_handle
{
	int used;
	long folder;
	unsigned long size;
	unsigned long tags_size;
	unsigned long tags_flags;
	/* the tag file's bytes: the tags stored at `tags` in them */
	unsigned char *file;
	unsigned char const *tags;
	unsigned long tags_stored_size;
	struct store_place *places;
	long place_count;
};

/* ---------- globals */

static struct
{
	struct store_folder folders[MAXIMUM_STORE_FOLDERS];
	struct store_handle handles[MAXIMUM_STORE_HANDLES];
	/* the region inflated last (most reads are a region whole: a bitmap's
	pixels, a sound's samples), and the handle whose tags were */
	unsigned char *region;
	unsigned long region_capacity;
	long region_folder;
	long region_entry;
	unsigned char *tags;
	unsigned long tags_capacity;
	long tags_handle;
	/* what stored bytes are read into, to be inflated */
	unsigned char *stored;
	unsigned long stored_capacity;
} port_tag_store = { { { { 0 } } }, { { 0 } }, NULL, 0, -1, -1, NULL, 0, -1, NULL, 0 };

/* ---------- private code */

static unsigned long read_u32(const unsigned char *bytes)
{
	return (unsigned long)bytes[0] | (unsigned long)bytes[1] << 8 | (unsigned long)bytes[2] << 16 |
		(unsigned long)bytes[3] << 24;
}

static void write_u32(unsigned char *bytes, unsigned long value)
{
	bytes[0] = (unsigned char)value;
	bytes[1] = (unsigned char)(value >> 8);
	bytes[2] = (unsigned char)(value >> 16);
	bytes[3] = (unsigned char)(value >> 24);
}

static void folder_file(const char *folder, const char *name, char *path, size_t size)
{
	snprintf(path, size, "%s/%s", folder, name);
}

/* room for `size` bytes in a buffer kept for reuse; 0 if there is none */
static int buffer_room(unsigned char **buffer, unsigned long *capacity, unsigned long size)
{
	unsigned char *grown;

	if (size <= *capacity && *buffer)
		return 1;
	grown = malloc(size ? size : 1);
	if (!grown)
		return 0;
	free(*buffer);
	*buffer = grown;
	*capacity = size;
	return 1;
}

static void key_make(const unsigned char *bytes, unsigned long size, struct store_key *key)
{
	unsigned long long hash = 14695981039346656037ULL;
	unsigned long index;

	for (index = 0; index < size; index++)
		hash = (hash ^ bytes[index]) * 1099511628211ULL;
	key->size = size;
	key->hash_low = (unsigned long)(hash & 0xFFFFFFFFUL);
	key->hash_high = (unsigned long)(hash >> 32 & 0xFFFFFFFFUL);
	key->crc = (unsigned long)crc32(0L, bytes, (uInt)size) & 0xFFFFFFFFUL;
}

static int key_compare(const struct store_key *first, const struct store_key *second)
{
	if (first->hash_high != second->hash_high)
		return first->hash_high < second->hash_high ? -1 : 1;
	if (first->hash_low != second->hash_low)
		return first->hash_low < second->hash_low ? -1 : 1;
	if (first->crc != second->crc)
		return first->crc < second->crc ? -1 : 1;
	if (first->size != second->size)
		return first->size < second->size ? -1 : 1;
	return 0;
}

static int entry_less(const void *first, const void *second)
{
	return key_compare(&((const struct store_entry *)first)->key, &((const struct store_entry *)second)->key);
}

static int entry_offset_less(const void *first, const void *second)
{
	unsigned long a = ((const struct store_entry *)first)->offset;
	unsigned long b = ((const struct store_entry *)second)->offset;

	return a < b ? -1 : a > b;
}

/* the entry of a key among `count` sorted ones, or -1 */
static long entry_find(const struct store_entry *entries, long count, const struct store_key *key)
{
	long low = 0;
	long high = count;

	while (low < high)
	{
		long middle = low + (high - low) / 2;
		int compared = key_compare(&entries[middle].key, key);

		if (!compared)
			return middle;
		if (compared < 0)
			low = middle + 1;
		else
			high = middle;
	}
	return -1;
}

static void key_read(const unsigned char *bytes, struct store_key *key)
{
	key->size = read_u32(bytes);
	key->hash_low = read_u32(bytes + 4);
	key->hash_high = read_u32(bytes + 8);
	key->crc = read_u32(bytes + 12);
}

static void key_write(unsigned char *bytes, const struct store_key *key)
{
	write_u32(bytes, key->size);
	write_u32(bytes + 4, key->hash_low);
	write_u32(bytes + 8, key->hash_high);
	write_u32(bytes + 12, key->crc);
}

/* a file's bytes whole (at most `limit` of them); NULL if it cannot be read */
static unsigned char *file_read(const char *path, unsigned long limit, unsigned long *size)
{
	struct posix_file_information information;
	unsigned char *bytes;
	FILE *file;

	*size = 0;
	if (posix_stat(path, &information) != 0 || (information.flags & _posix_file_is_directory) ||
		information.size_high || information.size_low > limit)
		return NULL;
	file = fopen(path, "rb");
	if (!file)
		return NULL;
	bytes = malloc(information.size_low ? information.size_low : 1);
	if (bytes && fread(bytes, 1, information.size_low, file) != information.size_low)
	{
		free(bytes);
		bytes = NULL;
	}
	fclose(file);
	if (bytes)
		*size = information.size_low;
	return bytes;
}

static unsigned long file_size(const char *path)
{
	struct posix_file_information information;

	return posix_stat(path, &information) == 0 && !information.size_high &&
		!(information.flags & _posix_file_is_directory) ? information.size_low : 0;
}

/* an open file's size, measured by the stream and not by its path: the
Switch's stat opens the file to measure it, which fails while the file is
open to be written (the pack, as a bundle is written), and a pack of 0 bytes
had the next bundle's regions written over all of the others' */
static unsigned long stream_size(FILE *file)
{
	long size;

	if (fseek(file, 0, SEEK_END) != 0 || (size = ftell(file)) < 0)
		return 0;
	return (unsigned long)size;
}

/* a pack's header: its generation; 0 if it is not a pack */
static int pack_header_read(FILE *pack, unsigned long *generation)
{
	unsigned char header[PACK_HEADER_BYTES];

	if (fseek(pack, 0, SEEK_SET) != 0 || fread(header, 1, sizeof(header), pack) != sizeof(header) ||
		read_u32(header) != PACK_SIGNATURE || read_u32(header + 4) != STORE_VERSION)
		return 0;
	*generation = read_u32(header + 8);
	return 1;
}

/* the regions of a pack, from its records (its index not being its): its
records up to the first that is not one (a write cut short), sorted; *end
where they end. 0 if there is no memory */
static int pack_scan(FILE *pack, unsigned long pack_size, struct store_entry **entries, long *count,
	unsigned long *end)
{
	unsigned long offset = PACK_HEADER_BYTES;
	long capacity = 0;

	*entries = NULL;
	*count = 0;
	while (offset <= pack_size && pack_size - offset >= PACK_RECORD_BYTES)
	{
		unsigned char record[PACK_RECORD_BYTES];
		struct store_entry entry;

		if (fseek(pack, (long)offset, SEEK_SET) != 0 || fread(record, 1, sizeof(record), pack) != sizeof(record))
			break;
		key_read(record, &entry.key);
		entry.stored_size = read_u32(record + 16);
		entry.flags = read_u32(record + 20);
		entry.offset = offset + PACK_RECORD_BYTES;
		if (entry.key.size > REGION_LIMIT || entry.flags & ~(unsigned long)STORED_DEFLATED ||
			read_u32(record + 24) || read_u32(record + 28) ||
			(!(entry.flags & STORED_DEFLATED) && entry.stored_size != entry.key.size) ||
			entry.stored_size > pack_size - entry.offset)
			break;
		if (*count == capacity)
		{
			long grown_capacity = capacity ? capacity * 2 : 256;
			struct store_entry *grown = realloc(*entries, (size_t)grown_capacity * sizeof(*grown));

			if (!grown)
			{
				free(*entries);
				*entries = NULL;
				*count = 0;
				return 0;
			}
			*entries = grown;
			capacity = grown_capacity;
		}
		(*entries)[(*count)++] = entry;
		offset = entry.offset + entry.stored_size;
	}
	*end = offset;
	if (*count)
		qsort(*entries, (size_t)*count, sizeof(**entries), entry_less);
	return 1;
}

/* A folder's regions, from its index when it is its pack's, else from the
pack (and the index written again): sorted; *generation the pack's, *end
where its regions end. With no pack, none (and a generation of 0). 0 if
the pack cannot be read */
static int folder_entries(const char *folder, FILE *pack, struct store_entry **entries, long *count,
	unsigned long *generation, unsigned long *end)
{
	char path[MAXIMUM_PATH_LENGTH];
	unsigned long pack_size;
	unsigned long index_size;
	unsigned char *index;
	long entry_count;
	long entry;
	int fits;

	*entries = NULL;
	*count = 0;
	*generation = 0;
	*end = PACK_HEADER_BYTES;
	if (!pack)
		return 1;
	folder_file(folder, PACK_NAME, path, sizeof(path));
	pack_size = stream_size(pack);
	if (!pack_header_read(pack, generation))
	{
		platform_log("tag injector: %s is not a pack of tags", path);
		return 0;
	}
	folder_file(folder, INDEX_NAME, path, sizeof(path));
	index = file_read(path, PACK_LIMIT, &index_size);
	entry_count = index && index_size >= INDEX_HEADER_BYTES ? (long)read_u32(index + 12) : -1;
	fits = index && index_size >= INDEX_HEADER_BYTES && read_u32(index) == INDEX_SIGNATURE &&
		read_u32(index + 4) == STORE_VERSION && read_u32(index + 8) == *generation &&
		entry_count >= 0 && (unsigned long)entry_count <= (index_size - INDEX_HEADER_BYTES) / INDEX_ENTRY_BYTES &&
		index_size == INDEX_HEADER_BYTES + (unsigned long)entry_count * INDEX_ENTRY_BYTES;
	if (fits && entry_count)
	{
		*entries = malloc((size_t)entry_count * sizeof(**entries));
		fits = *entries != NULL;
	}
	for (entry = 0; fits && entry < entry_count; entry++)
	{
		const unsigned char *bytes = index + INDEX_HEADER_BYTES + entry * INDEX_ENTRY_BYTES;
		struct store_entry *found = &(*entries)[entry];

		key_read(bytes, &found->key);
		found->offset = read_u32(bytes + 16);
		found->stored_size = read_u32(bytes + 20);
		found->flags = read_u32(bytes + 24);
		fits = found->key.size <= REGION_LIMIT && !(found->flags & ~(unsigned long)STORED_DEFLATED) &&
			(found->flags & STORED_DEFLATED || found->stored_size == found->key.size) &&
			found->offset >= PACK_HEADER_BYTES + PACK_RECORD_BYTES && found->offset <= pack_size &&
			found->stored_size <= pack_size - found->offset &&
			(!entry || key_compare(&(*entries)[entry - 1].key, &found->key) < 0);
		if (fits && found->offset + found->stored_size > *end)
			*end = found->offset + found->stored_size;
	}
	if (fits)
	{
		free(index);
		*count = entry_count;
		return 1;
	}
	/* (another pack's, or none: the pack's own records) */
	free(*entries);
	*entries = NULL;
	*end = PACK_HEADER_BYTES;
	if (index || pack_size > PACK_HEADER_BYTES)
		platform_log("tag injector: %s is not %s's: its regions are found again", path, PACK_NAME);
	free(index);
	return pack_scan(pack, pack_size, entries, count, end);
}

/* writes a folder's index: its regions (sorted) of the pack's generation */
static int index_write(const char *folder, const struct store_entry *entries, long count, unsigned long generation)
{
	char path[MAXIMUM_PATH_LENGTH];
	unsigned long size = INDEX_HEADER_BYTES + (unsigned long)count * INDEX_ENTRY_BYTES;
	unsigned char *bytes = malloc(size);
	long entry;
	int written;

	if (!bytes)
		return 0;
	memset(bytes, 0, size);
	write_u32(bytes, INDEX_SIGNATURE);
	write_u32(bytes + 4, STORE_VERSION);
	write_u32(bytes + 8, generation);
	write_u32(bytes + 12, (unsigned long)count);
	for (entry = 0; entry < count; entry++)
	{
		unsigned char *place = bytes + INDEX_HEADER_BYTES + entry * INDEX_ENTRY_BYTES;

		key_write(place, &entries[entry].key);
		write_u32(place + 16, entries[entry].offset);
		write_u32(place + 20, entries[entry].stored_size);
		write_u32(place + 24, entries[entry].flags);
	}
	folder_file(folder, INDEX_NAME, path, sizeof(path));
	written = port_tag_profiles_file_write(path, bytes, size);
	free(bytes);
	if (!written)
		platform_log("tag injector: %s cannot be written", path);
	return written;
}

/* `size` bytes compressed into *compressed (kept for reuse), when that
makes them an eighth smaller: their stored size, and their flags */
static unsigned long store_bytes(const unsigned char *bytes, unsigned long size, unsigned char **compressed,
	unsigned long *capacity, unsigned long *flags)
{
	/* (zlib 1.1.3's bound: a thousandth more, and 12 bytes) */
	unsigned long bound = size + size / 1000 + 64;
	uLongf length = (uLongf)bound;

	*flags = 0;
	if (size < 64 || !buffer_room(compressed, capacity, bound) ||
		compress2(*compressed, &length, bytes, (uLong)size, COMPRESSION_LEVEL) != Z_OK ||
		(unsigned long)length > size - size / 8)
		return size;
	*flags = STORED_DEFLATED;
	return (unsigned long)length;
}

/* inflates `stored_size` bytes into `size` at destination; 0 unless they
are exactly that many */
static int inflate_bytes(const unsigned char *stored, unsigned long stored_size, unsigned char *destination,
	unsigned long size)
{
	uLongf length = (uLongf)size;

	return uncompress(destination, &length, stored, (uLong)stored_size) == Z_OK && (unsigned long)length == size;
}

/* a folder's pack and index, read for its bundles to be (the one of that
folder already, else one of none); -1 if they cannot be */
static long folder_open(const char *folder)
{
	char path[MAXIMUM_PATH_LENGTH];
	struct store_folder *found = NULL;
	unsigned long generation;
	unsigned long end;
	long index;
	long free_index = -1;

	for (index = 0; index < MAXIMUM_STORE_FOLDERS; index++)
	{
		if (port_tag_store.folders[index].users && !strcmp(port_tag_store.folders[index].path, folder))
		{
			port_tag_store.folders[index].users++;
			return index;
		}
		if (!port_tag_store.folders[index].users && free_index < 0)
			free_index = index;
	}
	if (free_index < 0)
	{
		platform_log("tag injector: the bundles of more than %d profiles are open: %s's are left out",
			MAXIMUM_STORE_FOLDERS, folder);
		return -1;
	}
	found = &port_tag_store.folders[free_index];
	memset(found, 0, sizeof(*found));
	snprintf(found->path, sizeof(found->path), "%s", folder);
	folder_file(folder, PACK_NAME, path, sizeof(path));
	found->pack = fopen(path, "rb");
	if (!found->pack ||
		!folder_entries(folder, found->pack, &found->entries, &found->entry_count, &generation, &end))
	{
		if (found->pack)
			fclose(found->pack);
		platform_log("tag injector: %s cannot be read", path);
		memset(found, 0, sizeof(*found));
		return -1;
	}
	found->users = 1;
	return free_index;
}

static void folder_release(long folder)
{
	struct store_folder *found = &port_tag_store.folders[folder];

	if (--found->users > 0)
		return;
	if (found->pack)
		fclose(found->pack);
	free(found->entries);
	memset(found, 0, sizeof(*found));
	if (port_tag_store.region_folder == folder)
		port_tag_store.region_folder = -1;
	/* (what the last level read is let go with it) */
	for (folder = 0; folder < MAXIMUM_STORE_FOLDERS && !port_tag_store.folders[folder].users; folder++)
	{
	}
	if (folder == MAXIMUM_STORE_FOLDERS)
	{
		free(port_tag_store.region);
		free(port_tag_store.stored);
		free(port_tag_store.tags);
		port_tag_store.region = port_tag_store.stored = port_tag_store.tags = NULL;
		port_tag_store.region_capacity = port_tag_store.stored_capacity = port_tag_store.tags_capacity = 0;
		port_tag_store.region_folder = -1;
		port_tag_store.tags_handle = -1;
	}
}

/* an entry's bytes, inflated (kept until another's are); NULL if they cannot
be read */
static const unsigned char *region_bytes(long folder, long entry)
{
	struct store_folder *found = &port_tag_store.folders[folder];
	const struct store_entry *wanted = &found->entries[entry];

	if (port_tag_store.region_folder == folder && port_tag_store.region_entry == entry)
		return port_tag_store.region;
	port_tag_store.region_folder = -1;
	if (!buffer_room(&port_tag_store.region, &port_tag_store.region_capacity, wanted->key.size) ||
		fseek(found->pack, (long)wanted->offset, SEEK_SET) != 0)
		return NULL;
	if (wanted->flags & STORED_DEFLATED)
	{
		if (!buffer_room(&port_tag_store.stored, &port_tag_store.stored_capacity, wanted->stored_size) ||
			fread(port_tag_store.stored, 1, wanted->stored_size, found->pack) != wanted->stored_size ||
			!inflate_bytes(port_tag_store.stored, wanted->stored_size, port_tag_store.region, wanted->key.size))
			return NULL;
	}
	else if (fread(port_tag_store.region, 1, wanted->key.size, found->pack) != wanted->key.size)
		return NULL;
	port_tag_store.region_folder = folder;
	port_tag_store.region_entry = entry;
	return port_tag_store.region;
}

/* an open bundle's tags, inflated (kept until another's are) */
static const unsigned char *handle_tags(long handle)
{
	struct store_handle *found = &port_tag_store.handles[handle];

	if (!(found->tags_flags & STORED_DEFLATED))
		return found->tags;
	if (port_tag_store.tags_handle == handle)
		return port_tag_store.tags;
	port_tag_store.tags_handle = -1;
	if (!buffer_room(&port_tag_store.tags, &port_tag_store.tags_capacity, found->tags_size) ||
		!inflate_bytes(found->tags, found->tags_stored_size, port_tag_store.tags, found->tags_size))
		return NULL;
	port_tag_store.tags_handle = handle;
	return port_tag_store.tags;
}

/* a tag file's header and regions, checked: its regions' places and keys
(which the caller frees); 0 if it is not one this build reads */
static int tag_file_parse(const unsigned char *bytes, unsigned long size, unsigned long *bundle_size,
	unsigned long *tags_size, unsigned long *tags_stored_size, unsigned long *tags_flags,
	struct store_place **places, struct store_key **keys, long *count)
{
	unsigned long region_count;
	unsigned long file_data_size;
	unsigned long end = 0;
	unsigned long index;

	*places = NULL;
	*keys = NULL;
	*count = 0;
	if (size < TAG_FILE_HEADER_BYTES || read_u32(bytes) != TAG_FILE_SIGNATURE || read_u32(bytes + 4) != STORE_VERSION)
		return 0;
	*bundle_size = read_u32(bytes + 8);
	*tags_size = read_u32(bytes + 12);
	*tags_stored_size = read_u32(bytes + 16);
	*tags_flags = read_u32(bytes + 20);
	region_count = read_u32(bytes + 24);
	if (*bundle_size > 0x7FFFFFFFUL || *tags_size > *bundle_size || *tags_flags & ~(unsigned long)STORED_DEFLATED ||
		(!(*tags_flags & STORED_DEFLATED) && *tags_stored_size != *tags_size) ||
		region_count > (size - TAG_FILE_HEADER_BYTES) / TAG_FILE_REGION_BYTES ||
		size != TAG_FILE_HEADER_BYTES + region_count * TAG_FILE_REGION_BYTES + *tags_stored_size)
		return 0;
	file_data_size = *bundle_size - *tags_size;
	*places = malloc((region_count ? region_count : 1) * sizeof(**places));
	*keys = malloc((region_count ? region_count : 1) * sizeof(**keys));
	if (!*places || !*keys)
	{
		free(*places);
		free(*keys);
		*places = NULL;
		*keys = NULL;
		return 0;
	}
	for (index = 0; index < region_count; index++)
	{
		const unsigned char *region = bytes + TAG_FILE_HEADER_BYTES + index * TAG_FILE_REGION_BYTES;
		struct store_place *place = &(*places)[index];

		place->offset = read_u32(region);
		key_read(region + 4, &(*keys)[index]);
		place->size = (*keys)[index].size;
		place->entry = -1;
		/* (in order, apart, and in the file data) */
		if (place->offset < end || place->size > file_data_size || place->offset > file_data_size - place->size ||
			read_u32(region + 20))
		{
			free(*places);
			free(*keys);
			*places = NULL;
			*keys = NULL;
			return 0;
		}
		end = place->offset + place->size;
	}
	*count = (long)region_count;
	return 1;
}

/* ---------- public code */

int port_tag_store_write(const char *folder, const char *file, const unsigned char *bundle, unsigned long size,
	unsigned long tags_size, const struct port_tag_store_region *regions, long region_count, unsigned long *stored)
{
	char path[MAXIMUM_PATH_LENGTH];
	struct store_entry *entries = NULL;
	struct store_key *keys = NULL;
	unsigned char *compressed = NULL;
	unsigned long compressed_capacity = 0;
	unsigned char *tag_file = NULL;
	long entry_count = 0;
	long sorted_count;
	unsigned long generation;
	unsigned long end;
	unsigned long tags_stored;
	unsigned long tags_flags;
	unsigned long tag_file_size;
	unsigned long previous_end = 0;
	FILE *pack;
	long region;
	int written = 0;

	*stored = 0;
	if (!bundle || tags_size > size || size > 0x7FFFFFFFUL || region_count < 0)
		return 0;
	for (region = 0; region < region_count; region++)
	{
		if (regions[region].offset < previous_end || regions[region].size > size - tags_size ||
			regions[region].offset > size - tags_size - regions[region].size)
		{
			platform_log("tag injector: %s's regions are not its file data's: it is not written", file);
			return 0;
		}
		previous_end = regions[region].offset + regions[region].size;
	}

	/* the pack, made when there is none, and its regions */
	folder_file(folder, PACK_NAME, path, sizeof(path));
	pack = fopen(path, "r+b");
	if (!pack)
	{
		unsigned char header[PACK_HEADER_BYTES];

		memset(header, 0, sizeof(header));
		write_u32(header, PACK_SIGNATURE);
		write_u32(header + 4, STORE_VERSION);
		write_u32(header + 8, 1);
		pack = fopen(path, "wb");
		if (pack && fwrite(header, 1, sizeof(header), pack) != sizeof(header))
		{
			fclose(pack);
			pack = NULL;
		}
		if (pack)
		{
			fclose(pack);
			pack = fopen(path, "r+b");
		}
		*stored += PACK_HEADER_BYTES;
	}
	if (!pack || !folder_entries(folder, pack, &entries, &entry_count, &generation, &end))
	{
		platform_log("tag injector: %s cannot be read or made", path);
		if (pack)
			fclose(pack);
		return 0;
	}
	sorted_count = entry_count;
	keys = malloc((size_t)(region_count ? region_count : 1) * sizeof(*keys));
	if (keys && region_count)
	{
		struct store_entry *grown = realloc(entries, (size_t)(entry_count + region_count) * sizeof(*grown));

		if (!grown)
		{
			free(keys);
			keys = NULL;
		}
		else
			entries = grown;
	}
	if (!keys)
		goto done;

	/* each region the pack has not got, after its last (over a write cut
	short) */
	for (region = 0; region < region_count; region++)
	{
		const unsigned char *bytes = bundle + tags_size + regions[region].offset;
		unsigned char record[PACK_RECORD_BYTES];
		struct store_entry *added;
		unsigned long flags;
		unsigned long stored_size;
		long other;

		key_make(bytes, regions[region].size, &keys[region]);
		if (entry_find(entries, sorted_count, &keys[region]) >= 0)
			continue;
		for (other = sorted_count; other < entry_count && key_compare(&entries[other].key, &keys[region]); other++)
		{
		}
		if (other < entry_count)
			continue;
		stored_size = store_bytes(bytes, regions[region].size, &compressed, &compressed_capacity, &flags);
		if (end > PACK_LIMIT - PACK_RECORD_BYTES || stored_size > PACK_LIMIT - PACK_RECORD_BYTES - end)
		{
			platform_log("tag injector: %s would be past %lu MB: %s is not written", path, PACK_LIMIT >> 20, file);
			goto done;
		}
		memset(record, 0, sizeof(record));
		key_write(record, &keys[region]);
		write_u32(record + 16, stored_size);
		write_u32(record + 20, flags);
		if (fseek(pack, (long)end, SEEK_SET) != 0 || fwrite(record, 1, sizeof(record), pack) != sizeof(record) ||
			fwrite(flags ? compressed : bytes, 1, stored_size, pack) != stored_size)
		{
			platform_log("tag injector: %s cannot be written", path);
			goto done;
		}
		added = &entries[entry_count++];
		added->key = keys[region];
		added->offset = end + PACK_RECORD_BYTES;
		added->stored_size = stored_size;
		added->flags = flags;
		end += PACK_RECORD_BYTES + stored_size;
		*stored += PACK_RECORD_BYTES + stored_size;
	}
	if (fflush(pack) != 0)
	{
		platform_log("tag injector: %s cannot be written", path);
		goto done;
	}
	if (entry_count > sorted_count || !sorted_count)
	{
		qsort(entries, (size_t)entry_count, sizeof(*entries), entry_less);
		if (!index_write(folder, entries, entry_count, generation))
			goto done;
	}

	/* the tag file */
	tags_stored = store_bytes(bundle, tags_size, &compressed, &compressed_capacity, &tags_flags);
	tag_file_size = TAG_FILE_HEADER_BYTES + (unsigned long)region_count * TAG_FILE_REGION_BYTES + tags_stored;
	tag_file = malloc(tag_file_size);
	if (!tag_file)
		goto done;
	memset(tag_file, 0, TAG_FILE_HEADER_BYTES + (unsigned long)region_count * TAG_FILE_REGION_BYTES);
	write_u32(tag_file, TAG_FILE_SIGNATURE);
	write_u32(tag_file + 4, STORE_VERSION);
	write_u32(tag_file + 8, size);
	write_u32(tag_file + 12, tags_size);
	write_u32(tag_file + 16, tags_stored);
	write_u32(tag_file + 20, tags_flags);
	write_u32(tag_file + 24, (unsigned long)region_count);
	for (region = 0; region < region_count; region++)
	{
		unsigned char *place = tag_file + TAG_FILE_HEADER_BYTES + region * TAG_FILE_REGION_BYTES;

		write_u32(place, regions[region].offset);
		key_write(place + 4, &keys[region]);
	}
	memcpy(tag_file + TAG_FILE_HEADER_BYTES + (unsigned long)region_count * TAG_FILE_REGION_BYTES,
		tags_flags ? compressed : bundle, tags_stored);
	folder_file(folder, file, path, sizeof(path));
	written = port_tag_profiles_file_write(path, tag_file, tag_file_size);
	if (!written)
		platform_log("tag injector: %s cannot be written", path);
	else
		*stored += tag_file_size;

done:
	fclose(pack);
	free(entries);
	free(keys);
	free(compressed);
	free(tag_file);
	return written;
}

unsigned long port_tag_store_bundle_size(const char *path)
{
	unsigned char header[TAG_FILE_HEADER_BYTES];
	FILE *file = fopen(path, "rb");
	int read;

	if (!file)
		return 0;
	read = fread(header, 1, sizeof(header), file) == sizeof(header);
	fclose(file);
	return read && read_u32(header) == TAG_FILE_SIGNATURE && read_u32(header + 4) == STORE_VERSION &&
		read_u32(header + 8) <= 0x7FFFFFFFUL ? read_u32(header + 8) : 0;
}

long port_tag_store_open(const char *folder, const char *file, unsigned long *size)
{
	char path[MAXIMUM_PATH_LENGTH];
	struct store_handle *found;
	struct store_key *keys = NULL;
	unsigned long file_size_read;
	long handle;
	long place;

	*size = 0;
	for (handle = 0; handle < MAXIMUM_STORE_HANDLES && port_tag_store.handles[handle].used; handle++)
	{
	}
	if (handle == MAXIMUM_STORE_HANDLES)
	{
		platform_log("tag injector: more than %d tag files are open: %s/%s is left out", MAXIMUM_STORE_HANDLES, folder,
			file);
		return -1;
	}
	found = &port_tag_store.handles[handle];
	memset(found, 0, sizeof(*found));
	folder_file(folder, file, path, sizeof(path));
	found->file = file_read(path, TAG_FILE_LIMIT, &file_size_read);
	if (!found->file || !tag_file_parse(found->file, file_size_read, &found->size, &found->tags_size,
		&found->tags_stored_size, &found->tags_flags, &found->places, &keys, &found->place_count))
	{
		platform_log("tag injector: %s is not a tag file this build reads", path);
		free(found->file);
		memset(found, 0, sizeof(*found));
		return -1;
	}
	found->tags = found->file + TAG_FILE_HEADER_BYTES + (unsigned long)found->place_count * TAG_FILE_REGION_BYTES;
	found->folder = folder_open(folder);
	for (place = 0; found->folder >= 0 && place < found->place_count; place++)
	{
		struct store_folder const *pack = &port_tag_store.folders[found->folder];

		found->places[place].entry = entry_find(pack->entries, pack->entry_count, &keys[place]);
		if (found->places[place].entry < 0)
		{
			platform_log("tag injector: %s names a region (%lu bytes) its profile's pack has not got", path,
				keys[place].size);
			break;
		}
	}
	free(keys);
	if (found->folder < 0 || place < found->place_count)
	{
		if (found->folder >= 0)
			folder_release(found->folder);
		free(found->file);
		free(found->places);
		memset(found, 0, sizeof(*found));
		return -1;
	}
	found->used = 1;
	*size = found->size;
	return handle;
}

int port_tag_store_read(long handle, unsigned long offset, void *buffer, unsigned long size)
{
	struct store_handle *found = handle >= 0 && handle < MAXIMUM_STORE_HANDLES && port_tag_store.handles[handle].used ?
		&port_tag_store.handles[handle] : NULL;
	unsigned char *out = buffer;
	unsigned long start;
	unsigned long end;
	long low;
	long high;

	if (!found || offset > found->size || size > found->size - offset)
		return 0;
	/* the tags */
	if (offset < found->tags_size)
	{
		unsigned long count = found->tags_size - offset < size ? found->tags_size - offset : size;
		const unsigned char *tags = handle_tags(handle);

		if (!tags)
			return 0;
		memcpy(out, tags + offset, count);
		out += count;
		offset += count;
		size -= count;
	}
	if (!size)
		return 1;
	/* the file data: zeros between its regions */
	start = offset - found->tags_size;
	end = start + size;
	memset(out, 0, size);
	low = 0;
	high = found->place_count;
	while (low < high)
	{
		long middle = low + (high - low) / 2;

		if (found->places[middle].offset + found->places[middle].size <= start)
			low = middle + 1;
		else
			high = middle;
	}
	for (; low < found->place_count && found->places[low].offset < end; low++)
	{
		const struct store_place *place = &found->places[low];
		unsigned long from = place->offset > start ? place->offset : start;
		unsigned long to = place->offset + place->size < end ? place->offset + place->size : end;
		const unsigned char *bytes;

		if (from >= to)
			continue;
		bytes = region_bytes(found->folder, place->entry);
		if (!bytes)
		{
			platform_log("tag injector: a region of %lu bytes cannot be read from %s's pack", place->size,
				port_tag_store.folders[found->folder].path);
			return 0;
		}
		memcpy(out + (from - start), bytes + (from - place->offset), to - from);
	}
	return 1;
}

void port_tag_store_close(long handle)
{
	struct store_handle *found;

	if (handle < 0 || handle >= MAXIMUM_STORE_HANDLES || !port_tag_store.handles[handle].used)
		return;
	found = &port_tag_store.handles[handle];
	if (port_tag_store.tags_handle == handle)
		port_tag_store.tags_handle = -1;
	folder_release(found->folder);
	free(found->file);
	free(found->places);
	memset(found, 0, sizeof(*found));
}

int port_tag_store_collect(const char *folder, const char *const *files, long file_count)
{
	char path[MAXIMUM_PATH_LENGTH];
	char new_path[MAXIMUM_PATH_LENGTH];
	struct store_entry *entries = NULL;
	struct store_entry *live = NULL;
	unsigned char *alive = NULL;
	unsigned char *chunk = NULL;
	unsigned long generation;
	unsigned long end;
	unsigned long dead_bytes = 0;
	unsigned long pack_size;
	unsigned long offset;
	long entry_count = 0;
	long live_count = 0;
	long tag_files = 0;
	long index;
	FILE *pack;
	FILE *written_pack = NULL;
	int collected = 0;

	folder_file(folder, PACK_NAME, path, sizeof(path));
	pack = fopen(path, "rb");
	if (!pack)
		return 1;
	pack_size = stream_size(pack);
	if (!folder_entries(folder, pack, &entries, &entry_count, &generation, &end))
	{
		fclose(pack);
		return 0;
	}
	alive = malloc((size_t)(entry_count ? entry_count : 1));
	if (!alive)
		goto done;
	memset(alive, 0, (size_t)(entry_count ? entry_count : 1));
	/* (the regions each tag file names) */
	for (index = 0; index < file_count; index++)
	{
		char file_path[MAXIMUM_PATH_LENGTH];
		unsigned char *bytes;
		unsigned long size;
		unsigned long bundle_size, tags_size, tags_stored_size, tags_flags;
		struct store_place *places;
		struct store_key *keys;
		long count;
		long place;
		size_t length = strlen(files[index]);

		if (length < strlen(PORT_TAG_STORE_EXTENSION) ||
			strcmp(files[index] + length - strlen(PORT_TAG_STORE_EXTENSION), PORT_TAG_STORE_EXTENSION))
			continue;
		folder_file(folder, files[index], file_path, sizeof(file_path));
		bytes = file_read(file_path, TAG_FILE_LIMIT, &size);
		if (!bytes)
			continue;
		if (!tag_file_parse(bytes, size, &bundle_size, &tags_size, &tags_stored_size, &tags_flags, &places, &keys,
			&count))
		{
			/* (one this build cannot read: what it names is not known, so
			nothing goes) */
			free(bytes);
			platform_log("tag injector: %s is not a tag file this build reads: %s is kept whole", file_path, path);
			collected = 1;
			goto done;
		}
		tag_files++;
		for (place = 0; place < count; place++)
		{
			long found = entry_find(entries, entry_count, &keys[place]);

			if (found >= 0)
				alive[found] = 1;
		}
		free(bytes);
		free(places);
		free(keys);
	}
	if (!tag_files)
	{
		fclose(pack);
		pack = NULL;
		remove(path);
		folder_file(folder, INDEX_NAME, path, sizeof(path));
		remove(path);
		platform_log("tag injector: %s's pack is gone with its last tag", folder);
		collected = 1;
		goto done;
	}
	for (index = 0; index < entry_count; index++)
	{
		if (!alive[index])
			dead_bytes += PACK_RECORD_BYTES + entries[index].stored_size;
	}
	/* (kept while they are less than a quarter of it: a region let go may
	be added again) */
	if (dead_bytes <= pack_size / 4)
	{
		collected = 1;
		goto done;
	}

	/* the pack written again with the regions named, a generation later */
	live = malloc((size_t)entry_count * sizeof(*live));
	chunk = malloc(COPY_CHUNK);
	if (!live || !chunk)
		goto done;
	for (index = 0; index < entry_count; index++)
	{
		if (alive[index])
			live[live_count++] = entries[index];
	}
	qsort(live, (size_t)live_count, sizeof(*live), entry_offset_less);
	snprintf(new_path, sizeof(new_path), "%s.new", path);
	written_pack = fopen(new_path, "wb");
	if (!written_pack)
		goto done;
	{
		unsigned char header[PACK_HEADER_BYTES];

		memset(header, 0, sizeof(header));
		write_u32(header, PACK_SIGNATURE);
		write_u32(header + 4, STORE_VERSION);
		write_u32(header + 8, generation + 1);
		if (fwrite(header, 1, sizeof(header), written_pack) != sizeof(header))
			goto done;
	}
	offset = PACK_HEADER_BYTES;
	for (index = 0; index < live_count; index++)
	{
		unsigned char record[PACK_RECORD_BYTES];
		unsigned long done_bytes = 0;

		memset(record, 0, sizeof(record));
		key_write(record, &live[index].key);
		write_u32(record + 16, live[index].stored_size);
		write_u32(record + 20, live[index].flags);
		if (fwrite(record, 1, sizeof(record), written_pack) != sizeof(record) ||
			fseek(pack, (long)live[index].offset, SEEK_SET) != 0)
			goto done;
		while (done_bytes < live[index].stored_size)
		{
			unsigned long count = live[index].stored_size - done_bytes < COPY_CHUNK ?
				live[index].stored_size - done_bytes : COPY_CHUNK;

			if (fread(chunk, 1, count, pack) != count || fwrite(chunk, 1, count, written_pack) != count)
				goto done;
			done_bytes += count;
		}
		live[index].offset = offset + PACK_RECORD_BYTES;
		offset += PACK_RECORD_BYTES + live[index].stored_size;
	}
	if (fclose(written_pack) != 0)
	{
		written_pack = NULL;
		goto done;
	}
	written_pack = NULL;
	fclose(pack);
	pack = NULL;
	/* (the pack first: an index of the old one's generation is found again
	from its records, should what follows not be done) */
	if (!port_tag_profiles_file_replace(new_path, path))
	{
		platform_log("tag injector: %s cannot replace %s", new_path, path);
		goto done;
	}
	qsort(live, (size_t)live_count, sizeof(*live), entry_less);
	index_write(folder, live, live_count, generation + 1);
	platform_log("tag injector: %s written again without %lu bytes no tag has (%lu bytes now)", path, dead_bytes,
		offset);
	collected = 1;

done:
	if (written_pack)
	{
		fclose(written_pack);
		remove(new_path);
	}
	if (pack)
		fclose(pack);
	free(entries);
	free(live);
	free(alive);
	free(chunk);
	return collected;
}

unsigned long port_tag_store_pack_size(const char *folder)
{
	char path[MAXIMUM_PATH_LENGTH];
	unsigned long size;

	folder_file(folder, PACK_NAME, path, sizeof(path));
	size = file_size(path);
	folder_file(folder, INDEX_NAME, path, sizeof(path));
	return size + file_size(path);
}
