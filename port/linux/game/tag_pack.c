/*
TAG_PACK.C

The tag packs (tag_pack.h): maps/<name>.pack, one for each manifest in
port/assets/tag_packs (port/linux/src/port_tag_packs.c), written by the game
itself at the main menu from the maps the manifest names, and read by the
tag importer (tag_import.c) as a level loads, which never reads a map for
its tags. tools/tag_pack.py writes the same file on a computer.

At start-up the main menu renames each pack's legacy files (a pack once
called otherwise, its manifest's legacy_file_names), then
asks for each pack its manifest wants then ("always", a setting on, or a
condition of tag_pack_conditions.c's) that is not there, is stale (its
header's content hash is not its manifest's) or no longer matches its maps;
code asks for one later with tag_pack_request (a setting turned on, a game
joined that wants it). A notice says so first; the pack is written once it
is dismissed, behind a dialog whose text each frame drawn brings up to
date, and kept. A pack none of whose maps is there is not asked for.

A pack is 'rpak', format 2 (TAG_PACK_FORMAT_VERSION), its content hash and
how many maps, then a 56-byte record per map (tag_pack.h); then each map's
pixels and samples, its inflated tag section and its table of ranges.
Every place is an offset in the file, so the parts may come in any order.
Each distinct blob of pixels or samples is kept once.

A disc map is a plain 0x800-byte header and a zlib stream, and its tag
section is at the end, so each map is inflated twice: once for its tags,
which say which ranges to keep, and again to copy those ranges as the
stream passes them. A pack of one map takes some seconds; one of ten, a
few minutes and about 500 MB, once. The file is written as <name>.pack.partial and renamed when it
is whole, so a stop part-way leaves no pack that looks complete.
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "cseries.h"
#include "cseries/errors.h"
#include "cseries/cseries_windows.h"
#include "cache/cache_files.h"
#include "main/main.h"
#include "tag_files/tag_files.h"

#include "halo_port_limits.h"
#include "tag_pack.h"
#include "../src/port_tag_packs.h"

#include <xtl.h>
/* the port's zlib, the same one that unpacks a map while precaching it */
#include "../../third_party/zlib/zlib_prefixed.h"

/* the addresses a map file gives its tag pointers (halo_port_window.h) */
#define PACK_WINDOW 0x803A6000UL
#define MAP_HEADER_SIZE 0x800
#define PACK_LIMIT 0x7F000000L
#define RANGE_LIMIT (64L * 1024L * 1024L)
#define IO_CHUNK 0x40000
#define SEEN_SLOTS 0x20000
#define PROGRESS_MILLISECONDS 100

extern int config_boolean(char const *name);
extern void display_error_text_deferred(wchar_t const *text, short local_player_index);
extern boolean ui_widget_port_error_showing(void);
extern boolean ui_widget_port_error_open(void);

/* bitmap_group's bitmaps block, and a bitmap_data's fields */
#define BITMAPS_COUNT 0x60
#define BITMAPS_ADDRESS 0x64
#define BITMAP_DATA_SIZE 0x30
/* sound_definition's pitch ranges, a range's permutations, a permutation's samples */
#define PITCH_COUNT 0x98
#define PITCH_ADDRESS 0x9C
#define PITCH_SIZE 0x48
#define PERMUTATION_COUNT 0x3C
#define PERMUTATION_ADDRESS 0x40
#define PERMUTATION_SIZE 0x7C
#define SAMPLES_SIZE 0x40
#define SAMPLES_OFFSET 0x48

struct pack_range
{
	long offset;
	long size;
	long data_at;
};

struct pack_record
{
	char leaf[TAG_PACK_MAP_NAME_SIZE];
	long file_length;
	long tag_offset;
	long tag_size;
	long tags_at;
	long range_count;
	long ranges_at;
};

struct pack_stream
{
	HANDLE file;
	int compressed;
	int inflating;
	z_stream zlib;
	unsigned char *input;
	long produced;
};

struct pack_seen
{
	unsigned long first;
	unsigned long second;
	long size;
	long data_at;
};

static struct
{
	/* the manifest being written (port_tag_packs.h) */
	long pack;
	HANDLE out;
	long cursor;
	unsigned char *chunk;
	struct pack_seen *seen;
	long seen_count;
	double done;
	double total;
	unsigned long shown_at;
	/* a write refused: the card is full, or the pack is too large */
	int failed;
} build;

/* bitmaps.c bitmap_format_bits_per_pixel_table, through p8-bump */
static short const bitmap_bits[] = { 8, 8, 8, 16, 0, 0, 16, 0, 16, 16, 32, 32, 0, 0, 4, 8, 8, 8 };

static unsigned long read_u32(unsigned char const *bytes)
{
	return (unsigned long)bytes[0] | ((unsigned long)bytes[1] << 8) |
		((unsigned long)bytes[2] << 16) | ((unsigned long)bytes[3] << 24);
}

static long read_s32(unsigned char const *bytes)
{
	unsigned long value = read_u32(bytes);

	return (value & 0x80000000UL) ? -(long)(0xFFFFFFFFUL - value) - 1 : (long)value;
}

static short read_s16(unsigned char const *bytes)
{
	return (short)((unsigned short)bytes[0] | ((unsigned short)bytes[1] << 8));
}

static void write_u32(unsigned char *bytes, unsigned long value)
{
	bytes[0] = (unsigned char)value;
	bytes[1] = (unsigned char)(value >> 8);
	bytes[2] = (unsigned char)(value >> 16);
	bytes[3] = (unsigned char)(value >> 24);
}

/* ---------- the screen

The build's dialog (tag_pack_update) shows this text, which each
frame drawn here brings up to date. The loading screen's bar is not used: it
is drawn for a loop that renders every frame, and from near nothing it stays
black for minutes */

static wchar_t status_text[200];
static char status_map[TAG_PACK_MAP_NAME_SIZE];
/* the manifest's words, as the dialogs take them (lines split by \r\n) */
static wchar_t notice_text[300];
static wchar_t building_text[160];
static wchar_t ready_text[160];
static wchar_t failed_text[200];

static void status_set(wchar_t const *first, int percent, char const *map)
{
	int out = 0;
	int index;

	for (index = 0; first[index] && out < (int)NUMBEROF(status_text) - 48; index++)
		status_text[out++] = first[index];
	if (percent >= 0)
	{
		if (percent >= 100)
			status_text[out++] = L'1';
		if (percent >= 10)
			status_text[out++] = (wchar_t)(L'0' + (percent / 10) % 10);
		status_text[out++] = (wchar_t)(L'0' + percent % 10);
		status_text[out++] = L'%';
	}
	if (map && map[0])
	{
		status_text[out++] = L' ';
		status_text[out++] = L'(';
		for (index = 0; map[index] && index < TAG_PACK_MAP_NAME_SIZE - 1; index++)
			status_text[out++] = (wchar_t)(unsigned char)map[index];
		status_text[out++] = L')';
	}
	status_text[out] = 0;
}

static void progress_show(int force)
{
	unsigned long now = system_milliseconds();
	int percent;

	if (!force && now - build.shown_at < PROGRESS_MILLISECONDS)
		return;
	build.shown_at = now;
	percent = build.total > 0.0 ? (int)(build.done * 100.0 / build.total) : 0;
	if (percent > 99)
		percent = 99;
	status_set(building_text, percent, status_map);
	main_pregame_render();
	main_present_frame();
}

/* ---------- files */

static void map_path(char *path, unsigned int size, char const *leaf)
{
	snprintf(path, size, "%s%s.map", cache_files_map_directory(), leaf);
}

static int file_at(HANDLE file, long offset)
{
	DWORD result = SetFilePointer(file, offset, NULL, FILE_BEGIN);

	return result != INVALID_SET_FILE_POINTER || GetLastError() == 0;
}

static int file_read(HANDLE file, void *buffer, unsigned long size)
{
	unsigned long done = 0;

	while (done < size)
	{
		DWORD chunk = (DWORD)(size - done);
		DWORD got = 0;

		if (chunk > IO_CHUNK)
			chunk = IO_CHUNK;
		if (!ReadFile(file, (char *)buffer + done, chunk, &got, NULL) || got == 0)
			return 0;
		done += got;
	}

	return 1;
}

static int pack_write(void const *buffer, unsigned long size)
{
	unsigned long done = 0;

	if ((unsigned long)build.cursor + size > (unsigned long)PACK_LIMIT)
	{
		error(_error_silent, "tag pack: past what a 32-bit offset can name");
		build.failed = 1;
		return 0;
	}
	while (done < size)
	{
		DWORD chunk = (DWORD)(size - done);
		DWORD wrote = 0;

		if (chunk > IO_CHUNK)
			chunk = IO_CHUNK;
		if (!WriteFile(build.out, (char const *)buffer + done, chunk, &wrote, NULL) || wrote == 0)
		{
			error(_error_silent, "tag pack: could not write (error %lu; is the card full?)",
				(unsigned long)GetLastError());
			build.failed = 1;
			return 0;
		}
		done += wrote;
	}
	build.cursor += (long)size;

	return 1;
}

int tag_pack_map_header(char const *leaf, long *file_length, long *tag_offset, long *tag_size, int *compressed)
{
	char path[512];
	unsigned char header[0x18];
	HANDLE file;
	DWORD physical;
	int good;

	map_path(path, sizeof(path), leaf);
	file = CreateFileA(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (file == INVALID_HANDLE_VALUE)
		return 0;
	good = file_read(file, header, sizeof(header));
	physical = GetFileSize(file, NULL);
	CloseHandle(file);
	if (!good)
		return 0;
	*file_length = read_s32(header + 8);
	*tag_offset = read_s32(header + 0x10);
	*tag_size = read_s32(header + 0x14);
	if (read_u32(header) != 'head' || read_s32(header + 4) != 5 ||
		*file_length < MAP_HEADER_SIZE || *file_length > 0x11600000L ||
		*tag_offset < MAP_HEADER_SIZE || *tag_size < 36 || *tag_size > (long)HALO_PORT_TAG_CACHE_SIZE ||
		*tag_size > *file_length || *tag_offset > *file_length - *tag_size)
		return 0;
	if (compressed)
		*compressed = physical == INVALID_FILE_SIZE || (long)physical < *file_length;

	return 1;
}

/* ---------- a map's bytes, front to back */

static int stream_begin(struct pack_stream *stream, HANDLE file, int compressed)
{
	memset(stream, 0, sizeof(*stream));
	stream->file = file;
	stream->compressed = compressed;
	stream->produced = MAP_HEADER_SIZE;
	if (!file_at(file, MAP_HEADER_SIZE))
		return 0;
	if (!compressed)
		return 1;
	stream->input = malloc(IO_CHUNK);
	if (!stream->input || inflateInit(&stream->zlib) != Z_OK)
		return 0;
	stream->inflating = 1;

	return 1;
}

static void stream_end(struct pack_stream *stream)
{
	if (stream->inflating)
		inflateEnd(&stream->zlib);
	if (stream->input)
		free(stream->input);
	memset(stream, 0, sizeof(*stream));
}

/* the next `size` bytes, into `out`, or passed over when it is NULL */
static int stream_take(struct pack_stream *stream, long size, unsigned char *out)
{
	while (size > 0)
	{
		unsigned char *into = out ? out : build.chunk;
		long want = size > IO_CHUNK ? IO_CHUNK : size;
		long filled;

		if (!stream->compressed)
		{
			if (out ? !file_read(stream->file, into, (unsigned long)want) :
				!file_at(stream->file, stream->produced + want))
				return 0;
			filled = want;
		}
		else
		{
			int result;

			stream->zlib.next_out = (Bytef *)into;
			stream->zlib.avail_out = (uInt)want;
			while (stream->zlib.avail_out > 0)
			{
				if (stream->zlib.avail_in == 0)
				{
					DWORD got = 0;

					if (!ReadFile(stream->file, stream->input, IO_CHUNK, &got, NULL) || got == 0)
						return 0;
					stream->zlib.next_in = (Bytef *)stream->input;
					stream->zlib.avail_in = got;
				}
				result = inflate(&stream->zlib, Z_NO_FLUSH);
				if (result == Z_STREAM_END && stream->zlib.avail_out > 0)
					return 0;
				if (result != Z_OK && result != Z_STREAM_END && result != Z_BUF_ERROR)
					return 0;
			}
			filled = want;
		}
		stream->produced += filled;
		size -= filled;
		if (out)
			out += filled;
		build.done += (double)filled;
		progress_show(0);
	}

	return 1;
}

/* ---------- what a map's tags point at */

static long locate(unsigned char const *blob, long blob_size, unsigned long address)
{
	if (address < PACK_WINDOW || address - PACK_WINDOW >= (unsigned long)blob_size)
		return -1;

	return (long)(address - PACK_WINDOW);
}

static char const *tag_name(unsigned char const *blob, long blob_size, long instance)
{
	long at = locate(blob, blob_size, read_u32(blob + instance + 0x10));
	long room;

	if (at < 0)
		return "";
	room = blob_size - at;
	if (room > 256)
		room = 256;
	if (!memchr(blob + at, 0, (size_t)room))
		return "";

	return (char const *)blob + at;
}

static long bitmap_bytes(short width, short height, short depth, short kind, short format, unsigned short flags,
	short mipmaps)
{
	long pixels = 0;
	short level;

	if (format < 0 || format >= (short)NUMBEROF(bitmap_bits) || !bitmap_bits[format])
		return 0;
	if (mipmaps < 0 || mipmaps > 16 || width <= 0 || height <= 0 || depth <= 0 ||
		width > 4096 || height > 4096 || depth > 4096)
		return 0;
	for (level = 0; level <= mipmaps; level++)
	{
		long wide = width >> level;
		long high = height >> level;
		long deep = depth >> level;

		if (wide < 1)
			wide = 1;
		if (high < 1)
			high = 1;
		if (deep < 1)
			deep = 1;
		if (flags & 2)
		{
			wide = (wide + 3) & ~3L;
			high = (high + 3) & ~3L;
		}
		pixels += wide * high * deep * (kind == 2 ? 6 : 1);
		if (pixels > 64L * 1024L * 1024L)
			return 0;
	}

	return pixels / 8 * bitmap_bits[format] + (pixels % 8) * bitmap_bits[format] / 8;
}

struct range_list
{
	struct pack_range *ranges;
	long count;
	long capacity;
};

static int range_add(struct range_list *list, long file_length, long offset, long size)
{
	if (size <= 0 || size > RANGE_LIMIT || offset < 0 || offset > file_length - size)
		return 1;
	if (list->count == list->capacity)
	{
		long capacity = list->capacity ? list->capacity * 2 : 4096;
		struct pack_range *grown = malloc((unsigned long)capacity * sizeof(*grown));

		if (!grown)
			return 0;
		if (list->ranges)
		{
			memcpy(grown, list->ranges, (unsigned long)list->count * sizeof(*grown));
			free(list->ranges);
		}
		list->ranges = grown;
		list->capacity = capacity;
	}
	list->ranges[list->count].offset = offset;
	list->ranges[list->count].size = size;
	list->ranges[list->count].data_at = 0;
	list->count++;

	return 1;
}

static int range_less(void const *left, void const *right)
{
	struct pack_range const *a = left;
	struct pack_range const *b = right;

	if (a->offset != b->offset)
		return a->offset < b->offset ? -1 : 1;

	return a->size < b->size ? -1 : a->size > b->size;
}

static void bitmap_ranges(struct range_list *list, unsigned char const *blob, long blob_size, long base,
	long file_length, int *ok)
{
	long count;
	long at;
	long index;

	if (base + BITMAPS_ADDRESS + 4 > blob_size)
		return;
	count = read_s32(blob + base + BITMAPS_COUNT);
	at = locate(blob, blob_size, read_u32(blob + base + BITMAPS_ADDRESS));
	if (at < 0 || count <= 0 || count > 512 || at + count * BITMAP_DATA_SIZE > blob_size)
		return;
	for (index = 0; index < count && *ok; index++)
	{
		unsigned char const *record = blob + at + index * BITMAP_DATA_SIZE;
		long pixels_offset = read_s32(record + 0x18);
		long need = read_s32(record + 0x1C);
		long read;

		if (pixels_offset < 0)
			continue;
		read = bitmap_bytes(read_s16(record + 4), read_s16(record + 6), read_s16(record + 8),
			read_s16(record + 0x0A), read_s16(record + 0x0C), (unsigned short)read_s16(record + 0x0E),
			read_s16(record + 0x14));
		if (read > need)
			need = read;
		*ok = range_add(list, file_length, pixels_offset, need);
	}
}

static void sound_ranges(struct range_list *list, unsigned char const *blob, long blob_size, long base,
	long file_length, int *ok)
{
	long pitch_count;
	long pitch_at;
	long pitch;

	if (base + PITCH_ADDRESS + 4 > blob_size)
		return;
	pitch_count = read_s32(blob + base + PITCH_COUNT);
	pitch_at = locate(blob, blob_size, read_u32(blob + base + PITCH_ADDRESS));
	if (pitch_at < 0 || pitch_count <= 0 || pitch_count > 32 || pitch_at + pitch_count * PITCH_SIZE > blob_size)
		return;
	for (pitch = 0; pitch < pitch_count && *ok; pitch++)
	{
		long at = pitch_at + pitch * PITCH_SIZE;
		long count = read_s32(blob + at + PERMUTATION_COUNT);
		long permutations = locate(blob, blob_size, read_u32(blob + at + PERMUTATION_ADDRESS));
		long index;

		if (permutations < 0 || count <= 0 || count > 256 || permutations + count * PERMUTATION_SIZE > blob_size)
			continue;
		for (index = 0; index < count && *ok; index++)
		{
			unsigned char const *sample = blob + permutations + index * PERMUTATION_SIZE;

			*ok = range_add(list, file_length, read_s32(sample + SAMPLES_OFFSET), read_s32(sample + SAMPLES_SIZE));
		}
	}
}

/* the ranges a map's bitmaps and sounds read, in order, the overlapping
ones joined. 0 when the tags are not a campaign map's */
static int collect(struct range_list *list, unsigned char const *blob, long blob_size, long file_length)
{
	long count;
	long instances;
	long index;
	long bitmaps = 0;
	long sounds = 0;
	long kept;
	int ok = 1;

	if (blob_size < 36 || read_u32(blob + 0x20) != 'tags')
		return 0;
	count = read_s32(blob + 0x0C);
	instances = locate(blob, blob_size, read_u32(blob));
	if (count <= 0 || count > 100000 || instances < 0 || instances + count * 0x20 > blob_size)
		return 0;
	for (index = 0; index < count && ok; index++)
	{
		long instance = instances + index * 0x20;
		unsigned long group = read_u32(blob + instance);
		long base = locate(blob, blob_size, read_u32(blob + instance + 0x14));

		if (base < 0 || (group != 'bitm' && group != 'snd!'))
			continue;
		if (!port_tag_packs_keep(build.pack, group, tag_name(blob, blob_size, instance)))
			continue;
		if (group == 'bitm')
		{
			if (base + BITMAPS_ADDRESS + 4 <= blob_size)
				bitmaps++;
			bitmap_ranges(list, blob, blob_size, base, file_length, &ok);
		}
		else
		{
			if (base + PITCH_ADDRESS + 4 <= blob_size)
				sounds++;
			sound_ranges(list, blob, blob_size, base, file_length, &ok);
		}
	}
	if (!ok || bitmaps < port_tag_packs_minimum_bitmaps(build.pack) || sounds < port_tag_packs_minimum_sounds(build.pack))
		return 0;
	qsort(list->ranges, (unsigned long)list->count, sizeof(*list->ranges), range_less);
	kept = 0;
	for (index = 0; index < list->count; index++)
	{
		struct pack_range *range = list->ranges + index;

		if (kept && range->offset <= list->ranges[kept - 1].offset + list->ranges[kept - 1].size)
		{
			struct pack_range *last = list->ranges + kept - 1;
			long end = range->offset + range->size;

			if (end > last->offset + last->size)
				last->size = end - last->offset;
		}
		else
			list->ranges[kept++] = *range;
	}
	list->count = kept;

	return 1;
}

/* ---------- each blob once */

static unsigned long rotate(unsigned long value, int bits)
{
	value &= 0xFFFFFFFFUL;

	return ((value << bits) | (value >> (32 - bits))) & 0xFFFFFFFFUL;
}

static unsigned long finish_hash(unsigned long value)
{
	value &= 0xFFFFFFFFUL;
	value ^= value >> 16;
	value = (value * 0x85EBCA6BUL) & 0xFFFFFFFFUL;
	value ^= value >> 13;
	value = (value * 0xC2B2AE35UL) & 0xFFFFFFFFUL;
	value ^= value >> 16;

	return value;
}

/* two 32-bit lanes, whole words. A blob is hashed in IO_CHUNK pieces, a
multiple of four, so only its last piece has a tail */
static void hash_bytes(unsigned long *first, unsigned long *second, unsigned char const *bytes, long size)
{
	long index;

	for (index = 0; index + 4 <= size; index += 4)
	{
		unsigned long word = read_u32(bytes + index);

		*first ^= rotate(word * 0xCC9E2D51UL, 15) * 0x1B873593UL;
		*first = rotate(*first, 13) * 5 + 0xE6546B64UL;
		*second ^= rotate(word * 0x85EBCA6BUL, 17) * 0xC2B2AE35UL;
		*second = rotate(*second, 11) * 9 + 0x52DCE729UL;
	}
	for (; index < size; index++)
	{
		*first = rotate(*first ^ bytes[index], 7) * 0x01000193UL;
		*second = rotate(*second ^ bytes[index], 9) * 0x0100019DUL;
	}
}

static struct pack_seen *seen_slot(unsigned long first, unsigned long second, long size)
{
	unsigned long slot = (first ^ (second * 31)) & (SEEN_SLOTS - 1);

	for (;;)
	{
		struct pack_seen *seen = build.seen + slot;

		if (!seen->size || (seen->first == first && seen->second == second && seen->size == size))
			return seen;
		slot = (slot + 1) & (SEEN_SLOTS - 1);
	}
}

/* ---------- a map */

/* the range's bytes, at the end of the pack unless the same bytes already
are in it. Those under the map's header are its own, not the stream's */
static int copy_range(struct pack_stream *stream, unsigned char const *header, struct pack_range *range)
{
	long start = build.cursor;
	long offset = range->offset;
	long left = range->size;
	unsigned long first = 0x52414E44UL;
	unsigned long second = 0x4F4D495AUL;
	struct pack_seen *seen;

	if (offset < MAP_HEADER_SIZE)
	{
		long below = MAP_HEADER_SIZE - offset;

		if (below > left)
			below = left;
		hash_bytes(&first, &second, header + offset, below);
		if (!pack_write(header + offset, (unsigned long)below))
			return 0;
		offset += below;
		left -= below;
	}
	if (left > 0 && (offset < stream->produced || !stream_take(stream, offset - stream->produced, NULL)))
		return 0;
	while (left > 0)
	{
		long piece = left > IO_CHUNK ? IO_CHUNK : left;

		if (!stream_take(stream, piece, build.chunk))
			return 0;
		hash_bytes(&first, &second, build.chunk, piece);
		if (!pack_write(build.chunk, (unsigned long)piece))
			return 0;
		left -= piece;
	}
	first = finish_hash(first ^ (unsigned long)range->size);
	second = finish_hash(second + (unsigned long)range->size);
	if (build.seen_count >= SEEN_SLOTS / 4 * 3)
	{
		range->data_at = start;
		return 1;
	}
	seen = seen_slot(first, second, range->size);
	if (seen->size)
	{
		/* the next blob writes over this copy */
		range->data_at = seen->data_at;
		build.cursor = start;
		return file_at(build.out, start);
	}
	seen->first = first;
	seen->second = second;
	seen->size = range->size;
	seen->data_at = start;
	build.seen_count++;
	range->data_at = start;

	return 1;
}

static int pack_map(char const *leaf, struct pack_record *record)
{
	char path[512];
	unsigned char header[MAP_HEADER_SIZE];
	unsigned char *blob = NULL;
	struct range_list list;
	struct pack_stream stream;
	HANDLE file;
	long file_length, tag_offset, tag_size;
	long index;
	long resource_bytes = 0;
	double started = build.done;
	int compressed;
	int ok = 0;

	memset(&list, 0, sizeof(list));
	memset(&stream, 0, sizeof(stream));
	snprintf(status_map, sizeof(status_map), "%s", leaf);
	if (!tag_pack_map_header(leaf, &file_length, &tag_offset, &tag_size, &compressed))
	{
		error(_error_silent, "tag pack: no usable %s.map", leaf);
		return 0;
	}
	map_path(path, sizeof(path), leaf);
	file = CreateFileA(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (file == INVALID_HANDLE_VALUE)
		return 0;
	blob = malloc((unsigned long)tag_size);
	if (!blob || !file_at(file, 0) || !file_read(file, header, sizeof(header)))
		goto done;
	/* the tags, at the stream's end */
	if (!stream_begin(&stream, file, compressed) ||
		!stream_take(&stream, tag_offset - MAP_HEADER_SIZE, NULL) ||
		!stream_take(&stream, tag_size, blob))
	{
		error(_error_silent, "tag pack: %s.map ends early (a half-copied map?)", leaf);
		goto done;
	}
	stream_end(&stream);
	if (!collect(&list, blob, tag_size, file_length))
	{
		error(_error_silent, "tag pack: %s.map's tags are not the map %s wants", leaf,
			port_tag_packs_name(build.pack));
		goto done;
	}
	/* the pixels and samples, from the start again */
	build.done = started + (double)file_length;
	if (!stream_begin(&stream, file, compressed))
		goto done;
	for (index = 0; index < list.count; index++)
	{
		if (!copy_range(&stream, header, list.ranges + index))
		{
			error(_error_silent, "tag pack: could not copy %s.map's range at %ld", leaf,
				list.ranges[index].offset);
			goto done;
		}
		resource_bytes += list.ranges[index].size;
	}
	stream_end(&stream);
	memset(record, 0, sizeof(*record));
	snprintf(record->leaf, sizeof(record->leaf), "%s", leaf);
	record->file_length = file_length;
	record->tag_offset = tag_offset;
	record->tag_size = tag_size;
	record->tags_at = build.cursor;
	if (!pack_write(blob, (unsigned long)tag_size))
		goto done;
	record->ranges_at = build.cursor;
	record->range_count = list.count;
	for (index = 0; index < list.count; index++)
	{
		unsigned char entry[12];

		write_u32(entry, (unsigned long)list.ranges[index].offset);
		write_u32(entry + 4, (unsigned long)list.ranges[index].size);
		write_u32(entry + 8, (unsigned long)list.ranges[index].data_at);
		if (!pack_write(entry, sizeof(entry)))
			goto done;
	}
	error(_error_silent, "tag pack: %s.map, %ld ranges, %ld resource bytes, pack now %ld bytes",
		leaf, list.count, resource_bytes, build.cursor);
	ok = 1;
done:
	stream_end(&stream);
	CloseHandle(file);
	build.done = started + 2.0 * (double)file_length;
	if (blob)
		free(blob);
	if (list.ranges)
		free(list.ranges);

	return ok;
}

static int pack_finish(struct pack_record const *records, int count, long directory_size)
{
	unsigned char *directory = malloc((unsigned long)directory_size);
	int index;
	int ok;

	if (!directory)
		return 0;
	memset(directory, 0, (unsigned long)directory_size);
	write_u32(directory, 'rpak');
	write_u32(directory + 4, TAG_PACK_FORMAT_VERSION);
	write_u32(directory + 8, port_tag_packs_content(build.pack));
	write_u32(directory + 12, (unsigned long)count);
	for (index = 0; index < count; index++)
	{
		unsigned char *entry = directory + TAG_PACK_HEADER_SIZE + index * TAG_PACK_RECORD_SIZE;

		memcpy(entry, records[index].leaf, TAG_PACK_MAP_NAME_SIZE - 1);
		write_u32(entry + 32, (unsigned long)records[index].file_length);
		write_u32(entry + 36, (unsigned long)records[index].tag_offset);
		write_u32(entry + 40, (unsigned long)records[index].tag_size);
		write_u32(entry + 44, (unsigned long)records[index].tags_at);
		write_u32(entry + 48, (unsigned long)records[index].range_count);
		write_u32(entry + 52, (unsigned long)records[index].ranges_at);
	}
	/* a copy written over at the end leaves bytes past it */
	ok = file_at(build.out, build.cursor) && SetEndOfFile(build.out) && file_at(build.out, 0);
	build.cursor = 0;
	ok = ok && pack_write(directory, (unsigned long)directory_size);
	free(directory);

	return ok;
}

static char const *pack_path(long pack, char *path, unsigned int size)
{
	snprintf(path, size, "%s%s.pack", cache_files_map_directory(), port_tag_packs_name(pack));
	return path;
}

static int pack_build(long pack)
{
	char path[512];
	char partial[520];
	struct pack_record records[TAG_PACK_MAPS];
	long map_count = port_tag_packs_map_count(pack);
	long directory_size;
	int count = 0;
	int level;
	int ok = 0;

	if (map_count > TAG_PACK_MAPS)
		map_count = TAG_PACK_MAPS;
	directory_size = TAG_PACK_HEADER_SIZE + map_count * TAG_PACK_RECORD_SIZE;
	pack_path(pack, path, sizeof(path));
	snprintf(partial, sizeof(partial), "%s.partial", path);
	memset(&build, 0, sizeof(build));
	build.pack = pack;
	for (level = 0; level < map_count; level++)
	{
		long file_length, tag_offset, tag_size;

		if (tag_pack_map_header(port_tag_packs_map(pack, level), &file_length, &tag_offset, &tag_size, NULL))
			build.total += 2.0 * (double)file_length;
	}
	build.chunk = malloc(IO_CHUNK);
	build.seen = malloc(SEEN_SLOTS * sizeof(*build.seen));
	if (!build.chunk || !build.seen || build.total <= 0.0)
		goto done;
	memset(build.seen, 0, SEEN_SLOTS * sizeof(*build.seen));
	build.out = CreateFileA(partial, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	if (build.out == INVALID_HANDLE_VALUE)
	{
		error(_error_silent, "tag pack: could not write %s (error %lu)", partial, (unsigned long)GetLastError());
		build.out = NULL;
		goto done;
	}
	error(_error_silent, "tag pack: writing %s (once)", path);
	progress_show(1);
	memset(records, 0, sizeof(records));
	{
		unsigned char *blank = malloc((unsigned long)directory_size);

		if (!blank)
			goto done;
		memset(blank, 0, (unsigned long)directory_size);
		ok = pack_write(blank, (unsigned long)directory_size);
		free(blank);
		if (!ok)
			goto done;
		ok = 0;
	}
	for (level = 0; level < map_count; level++)
	{
		if (pack_map(port_tag_packs_map(pack, level), records + count))
			count++;
		else if (build.failed)
			goto done;
	}
	ok = count > 0 && pack_finish(records, count, directory_size);
done:
	if (build.out)
		CloseHandle(build.out);
	build.out = NULL;
	if (ok)
	{
		DeleteFileA(path);
		ok = MoveFileA(partial, path);
		if (!ok)
			error(_error_silent, "tag pack: could not rename %s (error %lu)", partial, (unsigned long)GetLastError());
	}
	if (!ok)
		DeleteFileA(partial);
	else
		error(_error_silent, "tag pack: wrote %s (%d maps, %ld distinct pixel and sample blobs)",
			path, count, build.seen_count);
	status_map[0] = 0;
	if (build.chunk)
		free(build.chunk);
	if (build.seen)
		free(build.seen);
	memset(&build, 0, sizeof(build));

	return ok;
}

/* ---------- the packs */

/* a manifest's words as a dialog takes them: each line ended \r\n */
static void text_widen(wchar_t *text, int size, char const *source)
{
	int out = 0;

	for (; *source && out < size - 2; source++)
	{
		if (*source == '\n')
			text[out++] = L'\r';
		text[out++] = (wchar_t)(unsigned char)*source;
	}
	text[out] = 0;
}

static void texts_load(long pack)
{
	text_widen(notice_text, (int)NUMBEROF(notice_text), port_tag_packs_text(pack, "notice"));
	text_widen(building_text, (int)NUMBEROF(building_text), port_tag_packs_text(pack, "building"));
	text_widen(ready_text, (int)NUMBEROF(ready_text), port_tag_packs_text(pack, "ready"));
	text_widen(failed_text, (int)NUMBEROF(failed_text), port_tag_packs_text(pack, "failed"));
	if (!notice_text[0])
		text_widen(notice_text, (int)NUMBEROF(notice_text), "Some data needs to be built\nfrom the maps.\nIt takes a moment.");
	if (!building_text[0])
		text_widen(building_text, (int)NUMBEROF(building_text), "Building data:\n");
	if (!ready_text[0])
		text_widen(ready_text, (int)NUMBEROF(ready_text), "The data is ready.");
	if (!failed_text[0])
		text_widen(failed_text, (int)NUMBEROF(failed_text), "The data could not\nbe built. Is the card full?");
}

/* whether any map of the pack is there to write it from */
static int pack_buildable(long pack)
{
	long level;

	for (level = 0; level < port_tag_packs_map_count(pack); level++)
	{
		long file_length, tag_offset, tag_size;

		if (tag_pack_map_header(port_tag_packs_map(pack, level), &file_length, &tag_offset, &tag_size, NULL))
			return 1;
	}
	return 0;
}

/* the pack is there, of this format and content, and each map it has is
the one in maps/ */
static int pack_ready(long pack)
{
	char path[512];
	unsigned char head[TAG_PACK_HEADER_SIZE];
	char leaves[TAG_PACK_MAPS][TAG_PACK_MAP_NAME_SIZE];
	HANDLE file;
	unsigned long count;
	unsigned long index;
	long level;
	int ready;

	if (pack < 0)
		return 0;
	memset(leaves, 0, sizeof(leaves));
	file = CreateFileA(pack_path(pack, path, sizeof(path)), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (file == INVALID_HANDLE_VALUE)
		return 0;
	ready = file_read(file, head, sizeof(head)) && read_u32(head) == 'rpak' &&
		read_u32(head + 4) == TAG_PACK_FORMAT_VERSION && read_u32(head + 8) == port_tag_packs_content(pack);
	count = ready ? read_u32(head + 12) : 0;
	if (!count || count > TAG_PACK_MAPS)
		ready = 0;
	for (index = 0; ready && index < count; index++)
	{
		unsigned char record[TAG_PACK_RECORD_SIZE];
		long file_length, tag_offset, tag_size;

		if (!file_read(file, record, sizeof(record)))
		{
			ready = 0;
			break;
		}
		memcpy(leaves[index], record, TAG_PACK_MAP_NAME_SIZE - 1);
		leaves[index][TAG_PACK_MAP_NAME_SIZE - 1] = 0;
		/* (a map no longer there is read from nowhere: the pack's is as good) */
		if (tag_pack_map_header(leaves[index], &file_length, &tag_offset, &tag_size, NULL) &&
			(file_length != read_s32(record + 32) || tag_offset != read_s32(record + 36) ||
				tag_size != read_s32(record + 40)))
			ready = 0;
	}
	CloseHandle(file);
	/* (and each of the manifest's maps there is, in it: a pack written
	before one of its maps was copied in has not got it) */
	for (level = 0; ready && level < port_tag_packs_map_count(pack) && level < TAG_PACK_MAPS; level++)
	{
		long file_length, tag_offset, tag_size;
		char const *leaf = port_tag_packs_map(pack, level);

		if (!tag_pack_map_header(leaf, &file_length, &tag_offset, &tag_size, NULL))
			continue;
		for (index = 0; index < count && _stricmp(leaves[index], leaf); index++)
			;
		if (index == count)
			ready = 0;
	}

	return ready;
}

/* what the main menu is doing about the pack: the notice is up; the build's
dialog has been asked for; it is up, and the pack is written */
enum
{
	_pack_idle = 0,
	_pack_notice,
	_pack_dialog_asked,
	_pack_dialog_up
};

static int pack_step;
/* the pack being written (NONE), and those asked for after it, in turn */
static long pack_current = NONE;
static long pack_queue[TAG_PACK_MAPS];
static int pack_queued;

static void pack_request(long pack)
{
	int index;

	if (pack < 0 || pack_current == pack)
		return;
	for (index = 0; index < pack_queued; index++)
	{
		if (pack_queue[index] == pack)
			return;
	}
	if (pack_queued >= (int)NUMBEROF(pack_queue) || pack_ready(pack) || !pack_buildable(pack))
		return;
	pack_queue[pack_queued++] = pack;
	error(_error_silent, "tag pack: %s wanted; written at the main menu, after its notice", port_tag_packs_name(pack));
}

/* a pack once called otherwise: renamed to the pack's own name, or, when
the pack is there already, deleted (it is only ever written again) */
static void legacy_rename(long pack)
{
	char path[512];
	long index;

	pack_path(pack, path, sizeof(path));
	for (index = 0; index < port_tag_packs_legacy_count(pack); index++)
	{
		char legacy[512];
		char partial[520];
		HANDLE file;

		snprintf(legacy, sizeof(legacy), "%s%s", cache_files_map_directory(), port_tag_packs_legacy(pack, index));
		snprintf(partial, sizeof(partial), "%s.partial", legacy);
		DeleteFileA(partial);
		file = CreateFileA(legacy, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (file == INVALID_HANDLE_VALUE)
			continue;
		CloseHandle(file);
		file = CreateFileA(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (file != INVALID_HANDLE_VALUE)
		{
			CloseHandle(file);
			if (DeleteFileA(legacy))
				error(_error_silent, "tag pack: deleted %s, which %s.pack is now", legacy, port_tag_packs_name(pack));
		}
		else if (MoveFileA(legacy, path))
			error(_error_silent, "tag pack: renamed %s to %s", legacy, path);
		else
			error(_error_silent, "tag pack: could not rename %s to %s (error %lu)", legacy, path,
				(unsigned long)GetLastError());
	}
}

/* whether a manifest wants its pack at start-up */
static int pack_wanted(long pack)
{
	long index;

	for (index = 0; index < port_tag_packs_wanted_count(pack); index++)
	{
		char const *wanted = port_tag_packs_wanted(pack, index);

		if (!strcmp(wanted, "always"))
			return 1;
		if (!strncmp(wanted, "setting:", 8) && config_boolean(wanted + 8))
			return 1;
		if (!strncmp(wanted, "condition:", 10))
		{
			int condition;

			for (condition = 0; tag_pack_conditions[condition].name; condition++)
			{
				if (!strcmp(tag_pack_conditions[condition].name, wanted + 10) &&
					tag_pack_conditions[condition].wanted())
					return 1;
			}
		}
	}
	return 0;
}

char const *tag_pack_file_name(char const *name)
{
	static char path[512];
	long pack = port_tag_packs_find(name);

	return pack < 0 ? NULL : pack_path(pack, path, sizeof(path));
}

unsigned long tag_pack_content(char const *name)
{
	return port_tag_packs_content(port_tag_packs_find(name));
}

int tag_pack_ready(char const *name)
{
	return pack_ready(port_tag_packs_find(name));
}

void tag_pack_request(char const *name)
{
	long pack = port_tag_packs_find(name);

	if (pack < 0)
		error(_error_silent, "tag pack: no manifest is named %s", name ? name : "(none)");
	pack_request(pack);
}

int tag_pack_busy(void)
{
	return pack_current != NONE || pack_queued > 0;
}

void tag_pack_update(boolean main_menu)
{
	static int checked;

	if (!main_menu)
		return;
	if (!checked)
	{
		long pack;

		checked = 1;
		for (pack = 0; pack < port_tag_packs_count(); pack++)
			legacy_rename(pack);
		for (pack = 0; pack < port_tag_packs_count(); pack++)
		{
			if (pack_wanted(pack))
				pack_request(pack);
		}
	}
	switch (pack_step)
	{
	case _pack_idle:
		if (!pack_queued || ui_widget_port_error_showing())
			return;
		pack_current = pack_queue[0];
		pack_queued--;
		memmove(pack_queue, pack_queue + 1, (unsigned long)pack_queued * sizeof(*pack_queue));
		texts_load(pack_current);
		display_error_text_deferred(notice_text, 0);
		pack_step = _pack_notice;
		return;
	case _pack_notice:
		if (ui_widget_port_error_showing())
			return;
		status_set(building_text, 0, "");
		display_error_text_deferred(status_text, 0);
		pack_step = _pack_dialog_asked;
		return;
	case _pack_dialog_asked:
		/* (it opens as the widgets update, after this; the pack is written
		only once it is drawn over the menu) */
		if (ui_widget_port_error_open())
			pack_step = _pack_dialog_up;
		else if (!ui_widget_port_error_showing())
		{
			pack_step = _pack_idle;
			pack_current = NONE;
		}
		return;
	case _pack_dialog_up:
		pack_step = _pack_idle;
		if (pack_build(pack_current))
			status_set(ready_text, -1, NULL);
		else
			status_set(failed_text, -1, NULL);
		pack_current = NONE;
		return;
	}
}
