/*
XISO.C

The maps folder out of an Xbox disc image, for the desktop games (Linux and
Windows). When no data root has a maps folder, sdl_platform.c asks the player
for their disc image of Halo and this copies the image's maps folder next to
the executable. (The Android app's copy of this is XisoExtractor.java, the
web page's xiso-worker.js.)

Written from a description of the Xbox's disc file system, XDVDFS
(xboxdevwiki's XDVDFS page, https://xboxdevwiki.net/XDVDFS, and the format
facts OpenCE's readers rely on). What this file relies on:

- Sectors are 2048 bytes and integers little-endian. Sector numbers count
  from the start of the game partition: an image of the partition alone has
  it at 0, an image of a whole disc further in (0x0FD90000, 0x02080000 or
  0x18300000 for XGD2, XGD3 and XGD1 discs). They are tried in that order.
- The volume descriptor is the partition's sector 32 (at 0x10000). It holds
  "MICROSOFT*XBOX*MEDIA" at its start and again at 0x7EC, and the root
  directory table's start sector (u32 at 0x14) and size in bytes (u32 at
  0x18).
- A directory table is a binary tree of entries whose root is the entry at
  offset 0. An entry is a left and a right subtree link (u16 each, in 4-byte
  units from the table's start, 0 for none), the start sector and the size
  of the file or of the subdirectory's table (u32 each), the attributes (u8,
  0x10 for a directory), the name's length (u8) and the name. Unused bytes of
  a table are 0xFF, so a left link of 0xFFFF is padding, not an entry.
- A file is one contiguous run of bytes from the start of its sector.

The image is the player's file and nothing read from it is trusted. Only the
root's table and the maps folder's are read, each whole and at most 4 MB;
the walk of a table visits each offset once, goes at most 64 links deep and
looks at most 4096 entries; and a file is written only under a plain name
that no file before it had (ignoring case). The root's default.xbe, read
whole and at most 64 MB, is searched for the loading screen's picture
(xiso_write_loading_picture) when the maps folder has no loading.tga.

The game takes a data root as having data once it has a maps folder, so the
copy is written to maps.partial (emptied first of what an earlier attempt
left) and given the name maps only when every file is complete.

The disc's bink folder (the movies: intro.bik, the menu's attract loop, the
credits) is copied beside maps once the maps are in place, the .bik files the
destination lacks, each as <name>.partial first (bink_ffmpeg.c plays them, in
a build made with configure.py --with-ffmpeg); a disc without the folder, or a
movie that cannot be copied, does not fail the maps.
*/

#include "platform.h"
#include "xiso.h"

#if !defined(HALO_ANDROID) && !defined(HALO_WEB)

#include "posix.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* (images are larger than 4 GB; Windows has neither flag) */
#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif
#ifndef O_NOFOLLOW
#define O_NOFOLLOW 0
#endif

#define XISO_SECTOR_SIZE 2048
#define XISO_DESCRIPTOR_OFFSET 0x10000
#define XISO_MAGIC "MICROSOFT*XBOX*MEDIA"
#define XISO_MAGIC_LENGTH 20
#define XISO_SECOND_MAGIC_OFFSET 0x7EC
#define XISO_ENTRY_FIXED_SIZE 14
#define XISO_ATTRIBUTE_DIRECTORY 0x10
#define XISO_PADDING_LINK 0xFFFF

/* directory tables are a few sectors; anything much larger is not one */
#define XISO_MAXIMUM_TABLE_SIZE (4 * 1024 * 1024)
#define XISO_MAXIMUM_DEPTH 64
#define XISO_MAXIMUM_VISITS 4096
#define XISO_MAXIMUM_ENTRIES 256
#define XISO_MAXIMUM_PATH 4096
#define XISO_COPY_CHUNK (1024 * 1024)

#define XISO_PARTIAL_DIRECTORY "maps.partial"

/* the loading screen's picture (source/interface/progress_bar.c), out of
default.xbe; the executable is a few MB */
#define XISO_LOADING_WIDTH 320
#define XISO_LOADING_HEIGHT 240
#define XISO_LOADING_PIXELS ((unsigned long)XISO_LOADING_WIDTH * XISO_LOADING_HEIGHT)
#define XISO_TGA_HEADER_SIZE 18
#define XISO_LOADING_TGA_SIZE (XISO_TGA_HEADER_SIZE + XISO_LOADING_PIXELS * 3)
#define XISO_MAXIMUM_XBE_SIZE (64UL * 1024 * 1024)

/* where the game partition may start, in the order they are tried */
static const unsigned long xiso_partition_starts[] =
{
	0x00000000, /* an image of the game partition alone */
	0x0FD90000, /* a whole XGD2 disc */
	0x02080000, /* a whole XGD3 disc */
	0x18300000, /* a whole XGD1 disc (the original Xbox's) */
};

struct xiso_image
{
	int descriptor;
	unsigned long long partition;
	/* the image's size in bytes, or ~0 when it could not be found */
	unsigned long long size;
};

/* a file or folder found in a directory table */
struct xiso_entry
{
	char name[256];
	unsigned long sector;
	unsigned long size;
};

struct xiso_walk
{
	const unsigned char *table;
	unsigned long size;
	/* a bit for each 4-byte offset of the table, set once it is visited */
	unsigned char *visited;
	int visits;
	int stopped;
	/* whether folders are collected (or else files) */
	int directories;
	struct xiso_entry *entries;
	int entry_count;
};

/* ---------- errors */

static void xiso_error(char *error, int error_size, const char *format, ...)
{
	va_list arguments;

	if (!error || error_size <= 0)
		return;
	va_start(arguments, format);
	vsnprintf(error, (size_t)error_size, format, arguments);
	va_end(arguments);
	error[error_size - 1] = 0;
}

/* ---------- names */

static int xiso_lower(int character)
{
	return character >= 'A' && character <= 'Z' ? character - 'A' + 'a' : character;
}

/* whether a name of length bytes is name (a C string), ignoring ASCII case */
static int xiso_name_is(const char *bytes, int length, const char *name)
{
	int index;

	if ((int)strlen(name) != length)
		return 0;
	for (index = 0; index < length; index++)
	{
		if (xiso_lower((unsigned char)bytes[index]) != xiso_lower((unsigned char)name[index]))
			return 0;
	}
	return 1;
}

/* no path, no . or .., and only printable ASCII that every desktop file
system takes as a plain name */
static int xiso_plain_name(const unsigned char *name, int length)
{
	int index;

	if (length <= 0 || (length == 1 && name[0] == '.') || (length == 2 && name[0] == '.' && name[1] == '.'))
		return 0;
	for (index = 0; index < length; index++)
	{
		if (name[index] < 0x20 || name[index] > 0x7E || strchr("/\\:*?\"<>|", name[index]))
			return 0;
	}
	return 1;
}

/* ---------- the image */

static unsigned long xiso_u16(const unsigned char *bytes)
{
	return (unsigned long)bytes[0] | (unsigned long)bytes[1] << 8;
}

static unsigned long xiso_u32(const unsigned char *bytes)
{
	return xiso_u16(bytes) | xiso_u16(bytes + 2) << 16;
}

/* reads size bytes at offset (from the start of the image file); returns
nonzero if all of them were read */
static int xiso_read(struct xiso_image *image, unsigned long long offset, void *buffer, unsigned long size)
{
	posix_ulong position_low, position_high;
	unsigned long count = 0;

	if (posix_seek(image->descriptor, (posix_long)(offset & 0xFFFFFFFFUL), (posix_long)(offset >> 32), SEEK_SET,
		&position_low, &position_high) != 0)
	{
		return 0;
	}
	while (count < size)
	{
		int result = (int)read(image->descriptor, (char *)buffer + count, (unsigned int)(size - count));

		if (result < 0 && errno == EINTR)
			continue;
		if (result <= 0)
			return 0;
		count += (unsigned long)result;
	}
	return 1;
}

/* the byte offset of a sector of the game partition */
static unsigned long long xiso_sector_offset(struct xiso_image *image, unsigned long sector)
{
	return image->partition + (unsigned long long)sector * XISO_SECTOR_SIZE;
}

/* finds the game partition: the first start whose volume descriptor has
both magics. Returns the root table's sector and size */
static int xiso_find_volume(struct xiso_image *image, unsigned long *root_sector, unsigned long *root_size)
{
	unsigned char descriptor[XISO_SECTOR_SIZE];
	int index;

	for (index = 0; index < (int)(sizeof(xiso_partition_starts) / sizeof(xiso_partition_starts[0])); index++)
	{
		unsigned long long start = xiso_partition_starts[index];

		if (!xiso_read(image, start + XISO_DESCRIPTOR_OFFSET, descriptor, sizeof(descriptor)) ||
			memcmp(descriptor, XISO_MAGIC, XISO_MAGIC_LENGTH) != 0 ||
			memcmp(descriptor + XISO_SECOND_MAGIC_OFFSET, XISO_MAGIC, XISO_MAGIC_LENGTH) != 0)
		{
			continue;
		}
		image->partition = start;
		*root_sector = xiso_u32(descriptor + 0x14);
		*root_size = xiso_u32(descriptor + 0x18);
		return 1;
	}
	return 0;
}

/* reads a directory table whole into a new allocation; returns NULL if it
is empty, too large or cannot be read, setting *out_of_memory when that is
the reason */
static unsigned char *xiso_read_table(struct xiso_image *image, unsigned long sector, unsigned long size,
	int *out_of_memory)
{
	unsigned char *table;

	*out_of_memory = 0;
	if (size == 0 || size > XISO_MAXIMUM_TABLE_SIZE)
		return NULL;
	table = malloc(size);
	if (!table)
	{
		*out_of_memory = 1;
		return NULL;
	}
	if (!xiso_read(image, xiso_sector_offset(image, sector), table, size))
	{
		free(table);
		return NULL;
	}
	return table;
}

/* ---------- directory tables */

static void xiso_walk_collect(struct xiso_walk *walk, const unsigned char *entry)
{
	struct xiso_entry *collected;
	int name_length = entry[13];
	int index;

	if (((entry[12] & XISO_ATTRIBUTE_DIRECTORY) != 0) != (walk->directories != 0))
		return;
	if (walk->entry_count >= XISO_MAXIMUM_ENTRIES || !xiso_plain_name(entry + XISO_ENTRY_FIXED_SIZE, name_length))
		return;
	/* (the destination's file system may ignore case) */
	for (index = 0; index < walk->entry_count; index++)
	{
		if (xiso_name_is((const char *)entry + XISO_ENTRY_FIXED_SIZE, name_length, walk->entries[index].name))
			return;
	}
	collected = &walk->entries[walk->entry_count++];
	memcpy(collected->name, entry + XISO_ENTRY_FIXED_SIZE, (size_t)name_length);
	collected->name[name_length] = 0;
	collected->sector = xiso_u32(entry + 4);
	collected->size = xiso_u32(entry + 8);
}

/* the subtree whose root entry is at offset: its left subtree, the entry,
its right subtree. A link to padding, past the table or back to an entry
already visited ends that branch */
static void xiso_walk_entry(struct xiso_walk *walk, unsigned long offset, int depth)
{
	const unsigned char *entry;
	unsigned long left, right, name_length;

	if (walk->stopped || depth > XISO_MAXIMUM_DEPTH)
		return;
	if (offset >= walk->size || walk->size - offset < XISO_ENTRY_FIXED_SIZE)
		return;
	entry = walk->table + offset;
	left = xiso_u16(entry);
	right = xiso_u16(entry + 2);
	if (left == XISO_PADDING_LINK)
		return;
	/* (links are in 4-byte units, so every offset is a multiple of 4) */
	if (walk->visited[offset >> 5] & (1 << ((offset >> 2) & 7)))
		return;
	walk->visited[offset >> 5] |= (unsigned char)(1 << ((offset >> 2) & 7));
	if (walk->visits >= XISO_MAXIMUM_VISITS)
	{
		walk->stopped = 1;
		return;
	}
	walk->visits++;

	if (left)
		xiso_walk_entry(walk, left * 4, depth + 1);
	name_length = entry[13];
	if (name_length && name_length <= walk->size - offset - XISO_ENTRY_FIXED_SIZE)
		xiso_walk_collect(walk, entry);
	if (right)
		xiso_walk_entry(walk, right * 4, depth + 1);
}

/* collects the folders (or the files) of a table, with plain names, into
entries (XISO_MAXIMUM_ENTRIES of them); returns how many, or -1 when out of
memory */
static int xiso_walk_table(const unsigned char *table, unsigned long size, int directories,
	struct xiso_entry *entries)
{
	struct xiso_walk walk;

	memset(&walk, 0, sizeof(walk));
	walk.table = table;
	walk.size = size;
	walk.directories = directories;
	walk.entries = entries;
	walk.visited = calloc(size / 32 + 1, 1);
	if (!walk.visited)
		return -1;
	xiso_walk_entry(&walk, 0, 0);
	free(walk.visited);
	return walk.entry_count;
}

/* ---------- the copy */

/* removes what an earlier, interrupted copy left in the folder at path */
static void xiso_empty_directory(const char *path, char *file_path, size_t file_path_size)
{
	void *directory = posix_directory_open(path);
	char name[260];

	if (!directory)
		return;
	while (posix_directory_next(directory, name, sizeof(name)))
	{
		int length = snprintf(file_path, file_path_size, "%s/%s", path, name);

		if (length > 0 && (size_t)length < file_path_size)
			remove(file_path);
	}
	posix_directory_close(directory);
}

/* copies one file of the image to path, adding to *done as it goes */
static int xiso_copy_file(struct xiso_image *image, const struct xiso_entry *entry, const char *path,
	unsigned char *buffer, unsigned long long *done, unsigned long long total, xiso_progress_proc progress,
	void *context, char *error, int error_size)
{
	unsigned long long offset = xiso_sector_offset(image, entry->sector);
	unsigned long remaining = entry->size;
	int output = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC | O_NOFOLLOW, 0644);

	if (output < 0)
	{
		xiso_error(error, error_size, "Could not write %s.", path);
		return 0;
	}
	while (remaining)
	{
		unsigned long count = remaining < XISO_COPY_CHUNK ? remaining : XISO_COPY_CHUNK;
		unsigned long written = 0;

		if (!xiso_read(image, offset, buffer, count))
		{
			close(output);
			xiso_error(error, error_size, "Could not read %s from the disc image (is it complete?).", entry->name);
			return 0;
		}
		while (written < count)
		{
			int result = (int)write(output, buffer + written, (unsigned int)(count - written));

			if (result < 0 && errno == EINTR)
				continue;
			if (result <= 0)
			{
				close(output);
				xiso_error(error, error_size, "Could not write %s (is the disk full?).", path);
				return 0;
			}
			written += (unsigned long)result;
		}
		offset += count;
		remaining -= count;
		*done += count;
		if (progress)
			progress(context, entry->name, *done, total);
	}
	if (close(output) != 0)
	{
		xiso_error(error, error_size, "Could not write %s (is the disk full?).", path);
		return 0;
	}
	return 1;
}

/* the maps folder among the root's folders, count if none: "maps", or on a
multi-language European disc, which has a "maps_xx" for each language
instead, the one for the system's language (LC_ALL, LC_MESSAGES or LANG),
else English's, else the first */
static int xiso_maps_folder(const struct xiso_entry *entries, int count)
{
	static const char *const variables[] = { "LC_ALL", "LC_MESSAGES", "LANG" };
	char language[8] = "maps_en";
	int english = count, first = count;
	int index;

	for (index = 0; index < count; index++)
	{
		if (xiso_name_is(entries[index].name, (int)strlen(entries[index].name), "maps"))
			return index;
	}
	for (index = 0; index < (int)(sizeof(variables) / sizeof(variables[0])); index++)
	{
		const char *value = getenv(variables[index]);

		if (value && value[0])
		{
			if (value[0] >= 'a' && value[0] <= 'z' && value[1] >= 'a' && value[1] <= 'z')
			{
				language[5] = value[0];
				language[6] = value[1];
			}
			break;
		}
	}
	for (index = 0; index < count; index++)
	{
		const char *name = entries[index].name;

		if (strlen(name) != 7 || !xiso_name_is(name, 5, "maps_"))
			continue;
		if (xiso_name_is(name, 7, language))
			return index;
		if (english == count && xiso_name_is(name, 7, "maps_en"))
			english = index;
		if (first == count)
			first = index;
	}
	return english < count ? english : first;
}

/* ---------- the loading screen's picture */

/* The loading screen's picture (source/interface/progress_bar.c reads it as
maps/loading.tga). The retail disc has no such file: its default.xbe holds
the picture, 320 by 240, as runs of 4-bit greys (a byte each: the grey's
high four bits in its low nibble, the run's length in its high one) after
the width and height as u32s. It is found by those two numbers followed by
runs that fill the picture exactly (a run of none, or one past the
picture's end, is not the picture), and written as the uncompressed 24-bit
TGA progress_bar.c reads. Decodes into greys (XISO_LOADING_PIXELS bytes);
returns nonzero if it was found. (Each try stops at the next try's u32s at
the latest, whose zero bytes are runs of none, so the search reads each
byte of the executable a bounded number of times.) */
static int xiso_loading_greys(const unsigned char *xbe, unsigned long size, unsigned char *greys)
{
	unsigned long at, next, pixel = 0;

	for (at = 0; at + 8 <= size; at++)
	{
		if (xiso_u32(xbe + at) != XISO_LOADING_WIDTH || xiso_u32(xbe + at + 4) != XISO_LOADING_HEIGHT)
			continue;
		for (pixel = 0, next = at + 8; next < size && pixel < XISO_LOADING_PIXELS; next++)
		{
			unsigned long run = xbe[next] >> 4;

			if (!run || run > XISO_LOADING_PIXELS - pixel)
				break;
			memset(greys + pixel, (xbe[next] & 0x0F) << 4, run);
			pixel += run;
		}
		if (pixel == XISO_LOADING_PIXELS)
			return 1;
	}
	return 0;
}

/* writes all of size bytes to output; returns nonzero if it did */
static int xiso_write_all(int output, const unsigned char *bytes, unsigned long size)
{
	unsigned long written = 0;

	while (written < size)
	{
		int result = (int)write(output, bytes + written, (unsigned int)(size - written));

		if (result < 0 && errno == EINTR)
			continue;
		if (result <= 0)
			return 0;
		written += (unsigned long)result;
	}
	return 1;
}

/* writes default.xbe's picture to folder/loading.tga, if it has one; a
picture that cannot be written whole is removed (the loading screen would
read past one cut short). Nothing is written when it is not found: the
light then sweeps over black, as the game's own does */
static void xiso_write_loading_picture(struct xiso_image *image, const struct xiso_entry *xbe, const char *folder,
	char *path, size_t path_size)
{
	unsigned char *data = NULL, *greys = NULL, *tga = NULL;
	int output, x, y, whole;

	if (!xbe->size || xbe->size > XISO_MAXIMUM_XBE_SIZE ||
		xiso_sector_offset(image, xbe->sector) + xbe->size > image->size)
	{
		return;
	}
	data = malloc(xbe->size);
	greys = malloc(XISO_LOADING_PIXELS);
	tga = malloc(XISO_LOADING_TGA_SIZE);
	if (!data || !greys || !tga || !xiso_read(image, xiso_sector_offset(image, xbe->sector), data, xbe->size) ||
		!xiso_loading_greys(data, xbe->size, greys))
	{
		goto done;
	}
	memset(tga, 0, XISO_TGA_HEADER_SIZE);
	tga[2] = 2;
	tga[12] = XISO_LOADING_WIDTH & 0xFF;
	tga[13] = XISO_LOADING_WIDTH >> 8;
	tga[14] = XISO_LOADING_HEIGHT & 0xFF;
	tga[15] = XISO_LOADING_HEIGHT >> 8;
	tga[16] = 24;
	/* (rows from the bottom; blue, green and red each the grey) */
	for (y = 0; y < XISO_LOADING_HEIGHT; y++)
	{
		for (x = 0; x < XISO_LOADING_WIDTH; x++)
		{
			unsigned char *out = tga + XISO_TGA_HEADER_SIZE + ((unsigned long)y * XISO_LOADING_WIDTH + x) * 3;

			out[0] = out[1] = out[2] = greys[(unsigned long)(XISO_LOADING_HEIGHT - 1 - y) * XISO_LOADING_WIDTH + x];
		}
	}
	if (snprintf(path, path_size, "%s/loading.tga", folder) >= (int)path_size)
		goto done;
	output = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC | O_NOFOLLOW, 0644);
	if (output < 0)
		goto done;
	whole = xiso_write_all(output, tga, XISO_LOADING_TGA_SIZE);
	if (close(output) != 0)
		whole = 0;
	if (whole)
		platform_log("extracting maps/loading.tga (the loading screen's picture, from default.xbe)");
	else
		remove(path);

done:
	free(tga);
	free(greys);
	free(data);
}

/* ---------- public code */

int xiso_extract_maps(const char *image_path, const char *destination, xiso_progress_proc progress, void *context,
	char *error, int error_size)
{
	struct xiso_image image;
	struct xiso_entry *entries = NULL;
	/* the root's bink folder (the movies), empty if the disc has none */
	struct xiso_entry *movies = NULL;
	/* the root's default.xbe (the loading screen's picture), size 0 if none */
	struct xiso_entry xbe;
	unsigned char *table = NULL;
	unsigned char *buffer = NULL;
	char *partial_path = NULL, *file_path = NULL, *maps_path = NULL;
	char *movies_path = NULL, *movie_path = NULL, *movie_final = NULL;
	unsigned long root_sector, root_size, maps_sector = 0, maps_size = 0;
	unsigned long movies_sector = 0, movies_size = 0;
	int movie_count = 0, movies_found = 0;
	unsigned long long done = 0, total = 0;
	posix_ulong size_low, size_high;
	int count, index, found, has_loading, out_of_memory, succeeded = 0;

	memset(&image, 0, sizeof(image));
	memset(&xbe, 0, sizeof(xbe));
	image.descriptor = open(image_path, O_RDONLY | O_LARGEFILE | O_CLOEXEC);
	if (image.descriptor < 0)
	{
		xiso_error(error, error_size, "Could not open %s.", image_path);
		return 0;
	}
	image.size = ~0ULL;
	if (posix_seek(image.descriptor, 0, 0, SEEK_END, &size_low, &size_high) == 0)
		image.size = (unsigned long long)size_high << 32 | size_low;

	entries = malloc(sizeof(*entries) * XISO_MAXIMUM_ENTRIES);
	movies = malloc(sizeof(*movies) * XISO_MAXIMUM_ENTRIES);
	buffer = malloc(XISO_COPY_CHUNK);
	partial_path = malloc(XISO_MAXIMUM_PATH);
	file_path = malloc(XISO_MAXIMUM_PATH);
	maps_path = malloc(XISO_MAXIMUM_PATH);
	movies_path = malloc(XISO_MAXIMUM_PATH);
	movie_path = malloc(XISO_MAXIMUM_PATH);
	movie_final = malloc(XISO_MAXIMUM_PATH);
	if (!entries || !movies || !buffer || !partial_path || !file_path || !maps_path ||
		!movies_path || !movie_path || !movie_final)
	{
		xiso_error(error, error_size, "Out of memory.");
		goto done;
	}

	if (!xiso_find_volume(&image, &root_sector, &root_size))
	{
		xiso_error(error, error_size, "This is not an Xbox disc image.");
		goto done;
	}

	/* the root's maps folder */
	table = xiso_read_table(&image, root_sector, root_size, &out_of_memory);
	if (!table)
	{
		xiso_error(error, error_size, out_of_memory ? "Out of memory." : "The disc image's file system is damaged.");
		goto done;
	}
	/* (its files first, for default.xbe) */
	count = xiso_walk_table(table, root_size, 0, entries);
	for (index = 0; index < count; index++)
	{
		if (xiso_name_is(entries[index].name, (int)strlen(entries[index].name), "default.xbe"))
			xbe = entries[index];
	}
	if (count >= 0)
		count = xiso_walk_table(table, root_size, 1, entries);
	free(table);
	table = NULL;
	if (count < 0)
	{
		xiso_error(error, error_size, "Out of memory.");
		goto done;
	}
	index = xiso_maps_folder(entries, count);
	found = index < count;
	if (found)
	{
		maps_sector = entries[index].sector;
		maps_size = entries[index].size;
		if (!xiso_name_is(entries[index].name, (int)strlen(entries[index].name), "maps"))
			platform_log("the disc image has no maps folder but one per language: extracting %s", entries[index].name);
	}
	else
	{
		xiso_error(error, error_size, "The disc image has no maps folder: it is not a Halo disc.");
		goto done;
	}

	/* the root's bink folder (the movies later, once the maps are in place) */
	for (index = 0; index < count; index++)
	{
		if (xiso_name_is(entries[index].name, (int)strlen(entries[index].name), "bink"))
		{
			movies_sector = entries[index].sector;
			movies_size = entries[index].size;
			movies_found = 1;
			break;
		}
	}
	if (movies_found)
	{
		table = xiso_read_table(&image, movies_sector, movies_size, &out_of_memory);
		if (table)
		{
			movie_count = xiso_walk_table(table, movies_size, 0, movies);
			free(table);
			table = NULL;
			if (movie_count < 0)
				movie_count = 0;
		}
	}

	/* its files */
	table = xiso_read_table(&image, maps_sector, maps_size, &out_of_memory);
	if (!table)
	{
		xiso_error(error, error_size, out_of_memory ? "Out of memory." : "The disc image's maps folder is damaged.");
		goto done;
	}
	count = xiso_walk_table(table, maps_size, 0, entries);
	free(table);
	table = NULL;
	if (count < 0)
	{
		xiso_error(error, error_size, "Out of memory.");
		goto done;
	}
	for (found = 0, has_loading = 0, index = 0; index < count; index++)
	{
		found |= xiso_name_is(entries[index].name, (int)strlen(entries[index].name), "ui.map");
		has_loading |= xiso_name_is(entries[index].name, (int)strlen(entries[index].name), "loading.tga");
	}
	if (!found)
	{
		xiso_error(error, error_size, "The disc image's maps folder has no ui.map: it is not a Halo disc.");
		goto done;
	}
	/* (an image that ends before a file does is refused before anything is
	written; an empty file's sector need not be anywhere) */
	for (index = 0; index < count; index++)
	{
		if (entries[index].size &&
			xiso_sector_offset(&image, entries[index].sector) + entries[index].size > image.size)
		{
			xiso_error(error, error_size, "Could not read %s from the disc image (is it complete?).",
				entries[index].name);
			goto done;
		}
		total += entries[index].size;
	}
	for (index = 0; index < movie_count; index++)
	{
		if (movies[index].size &&
			xiso_sector_offset(&image, movies[index].sector) + movies[index].size > image.size)
		{
			xiso_error(error, error_size, "Could not read %s from the disc image (is it complete?).",
				movies[index].name);
			goto done;
		}
		total += movies[index].size;
	}

	/* the copy, to maps.partial */
	if (snprintf(partial_path, XISO_MAXIMUM_PATH, "%s/" XISO_PARTIAL_DIRECTORY, destination) >= XISO_MAXIMUM_PATH ||
		snprintf(maps_path, XISO_MAXIMUM_PATH, "%s/maps", destination) >= XISO_MAXIMUM_PATH)
	{
		xiso_error(error, error_size, "Could not create %s/maps.", destination);
		goto done;
	}
	/* (an existing one is reused, emptied; a failure to make it shows as a
	failure to write the first file) */
	posix_make_directory(partial_path);
	xiso_empty_directory(partial_path, file_path, XISO_MAXIMUM_PATH);
	for (index = 0; index < count; index++)
	{
		platform_log("extracting maps/%s (%lu bytes)", entries[index].name, entries[index].size);
		if (snprintf(file_path, XISO_MAXIMUM_PATH, "%s/%s", partial_path, entries[index].name) >= XISO_MAXIMUM_PATH)
		{
			xiso_error(error, error_size, "Could not write %s/%s.", partial_path, entries[index].name);
			goto done;
		}
		if (!xiso_copy_file(&image, &entries[index], file_path, buffer, &done, total, progress, context,
			error, error_size))
		{
			goto done;
		}
	}
	if (!has_loading)
		xiso_write_loading_picture(&image, &xbe, partial_path, file_path, XISO_MAXIMUM_PATH);
	if (rename(partial_path, maps_path) != 0)
	{
		xiso_error(error, error_size, "Could not create %s.", maps_path);
		goto done;
	}
	succeeded = 1;

	/* the disc's movies, beside the maps: those the destination's bink folder
	lacks, each as <name>.partial first, so a copy that stops never leaves a
	short file the game would take for the movie (bink_ffmpeg.c plays them; a
	disc with none, or a movie that cannot be copied, does not fail the maps) */
	if (movie_count &&
		snprintf(movies_path, XISO_MAXIMUM_PATH, "%s/bink", destination) < XISO_MAXIMUM_PATH)
	{
		posix_make_directory(movies_path);
		for (index = 0; index < movie_count; index++)
		{
			char movie_error[64];
			size_t length = strlen(movies[index].name);

			if (length < 4 ||
				!xiso_name_is(movies[index].name + length - 4, 4, ".bik"))
				continue;
			if (snprintf(movie_final, XISO_MAXIMUM_PATH, "%s/%s", movies_path, movies[index].name) >=
					XISO_MAXIMUM_PATH ||
				snprintf(movie_path, XISO_MAXIMUM_PATH, "%s.partial", movie_final) >= XISO_MAXIMUM_PATH)
				continue;
			/* (one already there stays: it may be the player's) */
			{
				int existing = open(movie_final, O_RDONLY | O_CLOEXEC);

				if (existing >= 0)
				{
					close(existing);
					continue;
				}
			}
			platform_log("extracting bink/%s (%lu bytes)", movies[index].name, movies[index].size);
			if (!xiso_copy_file(&image, &movies[index], movie_path, buffer, &done, total, progress, context,
					movie_error, sizeof(movie_error)) ||
				rename(movie_path, movie_final) != 0)
			{
				platform_log("could not copy bink/%s; the game skips it", movies[index].name);
				unlink(movie_path);
				break;
			}
		}
	}

done:
	free(table);
	free(movies);
	free(maps_path);
	free(movies_path);
	free(movie_path);
	free(movie_final);
	free(file_path);
	free(partial_path);
	free(buffer);
	free(entries);
	close(image.descriptor);
	return succeeded;
}

#endif
