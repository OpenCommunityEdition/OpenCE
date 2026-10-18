/*
MENU_PC_ART.C

The menus' pictures that the player's own Halo PC, Halo for Mac (MacSoft) or
Halo Custom Edition install has, read from its ui.map (and bitmaps.map, which
holds the pixels) when the menus draw them. The 13 frames that neither the
Xbox's ui.map nor the redraws of this port have (the preview pictures of the
Edit Profile Settings, multiplayer type and gametype lists) are drawn from
the install's when there is one: nothing of them is shipped, written into
the game's files or kept: each is read and decoded when its texture is made.

The install is looked for, in this order:
	paths.pc_data (a folder: ui.map in it, or in its maps or GameData/Maps); when
	it is set, only it is looked in
	on the Mac, /Applications/Halo.app (and ~/Applications/Halo.app)
	paths.custom_edition, a Custom Edition install
	the data root's custom_maps folder, where Custom Edition's files may be
	(docs/custom_edition_caches.md)

A cache of version 7 (Halo PC and Halo for Mac) or 609 (Custom Edition) keeps
its tags at one address, and a bitmap tag's frames in a block of 0x30-byte
records; a frame whose "in a resource map" flag is set has its pixels in
bitmaps.map, at the offset the record gives, else in ui.map. The layouts are
those of cache_file_formats.c and of the Mac port's retail_cache.c. The map
files are untrusted input: every field is read little-endian from the bytes
that were read of the file, and every offset, count and size is checked
against them before it is used.

Standalone C with fixed-width types: the Linux, Windows, Android and web
builds compile it as it is, and the Mac's (which has no SDL) too.
*/

/* ---------- headers */

/* the host's own file functions, real paths (as the platform layer's files
have them: port/linux/include/stdio.h) */
#ifndef HALO_LINUX_PLATFORM_LAYER
#define HALO_LINUX_PLATFORM_LAYER
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "port_config.h"
#include "halo_menus.h"

/* the platform's data root: ending in a separator or not (xbox_files.c;
the Mac build has none) */
extern const char *platform_data_root(void);

/* ---------- constants */

#define HEADER_BYTES 0x800
#define HEADER_HEAD 0x68656164UL
#define HEADER_FOOT 0x666f6f74UL
#define TAG_INDEX_SIGNATURE 0x74616773UL
#define BITMAP_GROUP 0x6269746dUL
#define VERSION_PC 7
#define VERSION_CUSTOM_EDITION 609

/* where every tag's data is, in a PC cache's tag data: as the file's
tag data offset 0 (cache_file_formats.h, CUSTOM_EDITION_TAG_CACHE_ADDRESS) */
#define TAG_BASE 0x40440000UL
#define TAG_INSTANCE_BYTES 32
#define BITMAP_GROUP_BYTES 0x6c
#define BITMAP_BLOCK_OFFSET 0x60
#define BITMAP_DATA_BYTES 0x30

/* a ui.map is 2 to 3 MB; this is a generous limit on what is read of one */
#define UI_MAP_MAXIMUM_BYTES 0x04000000UL
#define PICTURE_MAXIMUM 2048
#define PATH_BYTES 1100
#define NAME_BYTES 256

enum
{
	_format_r5g6b5 = 6,
	_format_a1r5g5b5 = 8,
	_format_a4r4g4b4 = 9,
	_format_x8r8g8b8 = 10,
	_format_a8r8g8b8 = 11,
	_format_dxt1 = 14,
	_format_dxt3 = 15,
	_format_dxt5 = 16
};

enum
{
	_flag_compressed = 1 << 1,
	_flag_palettized = 1 << 2,
	_flag_swizzled = 1 << 3,
	_flag_in_resource_map = 1 << 8
};

/* ---------- the frames */

/* the menu files' PNG of a frame, and the PC version's bitmap tag and frame
it has the picture in (tools/ce_menus.py writes the PNGs: a frame's name is
the tag's, under ce/shell, with __<frame>.png). Each folder is written as a
PNG's path and as a tag path. */
#define PICTURE(folder, tag_folder, name, frame) \
	{ "ce/shell/" folder "/" name "__" #frame ".png", "ui\\shell\\" tag_folder "\\" name, frame }
#define MP "main_menu/multiplayer_type_select"
#define MP_TAG "main_menu\\multiplayer_type_select"
#define GAMETYPE "main_menu/settings_select/multiplayer_setup/playlist_edit"
#define GAMETYPE_TAG "main_menu\\settings_select\\multiplayer_setup\\playlist_edit"

struct picture
{
	char const *png;
	char const *tag;
	long frame;
};

/* (not the profile settings' pictures: the user-drawn ones in
   port/assets/menus/art are drawn instead of the install's) */
static struct picture const pictures[] =
{
	PICTURE(MP, MP_TAG, "mp_options", 0),
	PICTURE(MP, MP_TAG, "mp_options", 1),
	PICTURE(GAMETYPE, GAMETYPE_TAG, "gametype_options", 0),
	PICTURE(GAMETYPE, GAMETYPE_TAG, "gametype_options", 3),
	PICTURE(GAMETYPE, GAMETYPE_TAG, "gametype_options", 4),
	PICTURE(GAMETYPE, GAMETYPE_TAG, "gametype_options", 6),
	PICTURE(GAMETYPE, GAMETYPE_TAG, "gametype_options", 7),
};
#define PICTURE_COUNT ((long)(sizeof(pictures) / sizeof(*pictures)))

/* where a picture's pixels are, read from ui.map's tags */
struct located
{
	int found;
	uint32_t offset;
	uint32_t bytes;
	uint32_t width;
	uint32_t height;
	uint32_t format;
	int in_bitmaps_map;
};

static struct
{
	int searched;
	int present;
	char ui_map[PATH_BYTES];
	char bitmaps_map[PATH_BYTES];
	struct located located[sizeof(pictures) / sizeof(*pictures)];
} install;

/* ---------- reading */

static uint32_t read_u16(uint8_t const *bytes)
{
	return (uint32_t)bytes[0] | (uint32_t)bytes[1] << 8;
}

static uint32_t read_u32(uint8_t const *bytes)
{
	return (uint32_t)bytes[0] | (uint32_t)bytes[1] << 8 | (uint32_t)bytes[2] << 16 | (uint32_t)bytes[3] << 24;
}

/* whether [offset, offset + bytes) is inside limit bytes */
static int fits(uint32_t offset, uint32_t bytes, uint32_t limit)
{
	return offset <= limit && bytes <= limit - offset;
}

/* the size of a file, or 0 (and none over 2 GB: its offsets are longs) */
static uint32_t file_size(char const *path)
{
	FILE *file = fopen(path, "rb");
	long size = 0;

	if (!file)
		return 0;
	if (fseek(file, 0, SEEK_END) == 0)
		size = ftell(file);
	fclose(file);

	return size > 0 && size <= 0x7fffffffL ? (uint32_t)size : 0;
}

/* bytes [offset, offset + bytes) of a file into buffer: all or nothing */
static int file_read_range(char const *path, uint32_t offset, uint32_t bytes, uint8_t *buffer)
{
	FILE *file;
	int result = 0;

	if (offset > 0x7fffffffUL || bytes > 0x7fffffffUL)
		return 0;
	file = fopen(path, "rb");
	if (!file)
		return 0;
	if (fseek(file, (long)offset, SEEK_SET) == 0 && fread(buffer, 1, bytes, file) == bytes)
		result = 1;
	fclose(file);

	return result;
}

/* ---------- the cache */

/* a span of the cache's tag data, by the address its tags are given at, or
NULL (the address, and the size from it, are inside the tag data) */
static uint8_t const *tag_span(uint8_t const *tags, uint32_t tag_bytes, uint32_t address, uint32_t bytes)
{
	if (address < TAG_BASE || !fits(address - (uint32_t)TAG_BASE, bytes, tag_bytes))
		return NULL;

	return tags + (address - (uint32_t)TAG_BASE);
}

/* whether the tag's name (a NUL-terminated string in the tag data) is this
tag path */
static int tag_name_matches(uint8_t const *tags, uint32_t tag_bytes, uint32_t address, char const *name)
{
	size_t length = strlen(name);
	uint8_t const *start = tag_span(tags, tag_bytes, address, (uint32_t)length + 1);

	return start && !memcmp(start, name, length) && !start[length];
}

/* where a frame's pixels are: the cache's bitmap tag of this name (its data
local, not held whole by a resource map) and a frame of it that is an
uncompressed or DXT picture of a size and format this decodes */
static int locate(uint8_t const *map, uint32_t map_bytes, struct picture const *picture, struct located *located)
{
	uint8_t const *tags;
	uint8_t const *instances;
	uint32_t tag_offset = read_u32(map + 16);
	uint32_t tag_bytes = read_u32(map + 20);
	uint32_t first_instance;
	uint32_t tag_count;
	uint32_t index;

	if (tag_offset < HEADER_BYTES || tag_bytes < 40 || !fits(tag_offset, tag_bytes, map_bytes))
		return 0;
	tags = map + tag_offset;
	if (read_u32(tags + 36) != TAG_INDEX_SIGNATURE)
		return 0;
	tag_count = read_u32(tags + 12);
	if (!tag_count || tag_count > 65536UL || read_u32(tags) < TAG_BASE)
		return 0;
	first_instance = read_u32(tags) - (uint32_t)TAG_BASE;
	if (!fits(first_instance, tag_count * TAG_INSTANCE_BYTES, tag_bytes))
		return 0;
	instances = tags + first_instance;
	for (index = 0; index < tag_count; index++)
	{
		uint8_t const *instance = instances + index * TAG_INSTANCE_BYTES;
		uint8_t const *root;
		uint8_t const *frames;
		uint8_t const *data;
		uint32_t frame_count;
		uint32_t flags;
		uint32_t format;
		uint32_t width;
		uint32_t height;
		uint32_t need;

		if (read_u32(instance) != BITMAP_GROUP || !tag_name_matches(tags, tag_bytes, read_u32(instance + 16), picture->tag))
			continue;
		/* (a tag held whole by bitmaps.map has no definition here) */
		if (read_u32(instance + 24))
			return 0;
		root = tag_span(tags, tag_bytes, read_u32(instance + 20), BITMAP_GROUP_BYTES);
		if (!root)
			return 0;
		frame_count = read_u32(root + BITMAP_BLOCK_OFFSET);
		if ((uint32_t)picture->frame >= frame_count || frame_count > tag_bytes / BITMAP_DATA_BYTES)
			return 0;
		frames = tag_span(tags, tag_bytes, read_u32(root + BITMAP_BLOCK_OFFSET + 4), frame_count * BITMAP_DATA_BYTES);
		if (!frames)
			return 0;
		data = frames + (uint32_t)picture->frame * BITMAP_DATA_BYTES;
		width = read_u16(data + 4);
		height = read_u16(data + 6);
		format = read_u16(data + 12);
		flags = read_u16(data + 14);
		/* (a 2D picture of one level, stored as the PC's tags store them) */
		if (!width || !height || width > PICTURE_MAXIMUM || height > PICTURE_MAXIMUM ||
			read_u16(data + 8) != 1 || read_u16(data + 10) != 0 ||
			(flags & (_flag_swizzled | _flag_palettized)))
		{
			return 0;
		}
		switch (format)
		{
		case _format_r5g6b5:
		case _format_a1r5g5b5:
		case _format_a4r4g4b4:
			need = width * height * 2;
			break;
		case _format_x8r8g8b8:
		case _format_a8r8g8b8:
			need = width * height * 4;
			break;
		case _format_dxt1:
			need = ((width + 3) / 4) * ((height + 3) / 4) * 8;
			break;
		case _format_dxt3:
		case _format_dxt5:
			need = ((width + 3) / 4) * ((height + 3) / 4) * 16;
			break;
		default:
			return 0;
		}
		located->offset = read_u32(data + 24);
		located->bytes = read_u32(data + 28);
		located->width = width;
		located->height = height;
		located->format = format;
		located->in_bitmaps_map = (flags & _flag_in_resource_map) != 0;
		/* (the first level: the rest are mipmaps) */
		if (located->bytes < need)
			return 0;
		located->bytes = need;
		located->found = 1;

		return 1;
	}

	return 0;
}

/* the install's ui.map: read, checked and looked in for each picture */
static int install_read(void)
{
	FILE *file = fopen(install.ui_map, "rb");
	uint8_t *map;
	uint32_t map_bytes;
	uint32_t bitmaps_bytes = 0;
	long size = 0;
	long index;
	int any = 0;

	if (!file)
		return 0;
	if (fseek(file, 0, SEEK_END) == 0)
		size = ftell(file);
	if (size < HEADER_BYTES + 40 || size > (long)UI_MAP_MAXIMUM_BYTES || fseek(file, 0, SEEK_SET) != 0)
	{
		fclose(file);
		return 0;
	}
	map_bytes = (uint32_t)size;
	map = (uint8_t *)malloc(map_bytes);
	if (!map || fread(map, 1, map_bytes, file) != map_bytes)
	{
		free(map);
		fclose(file);
		return 0;
	}
	fclose(file);
	/* (not a cache: the Xbox's ui.map is version 5, a cache compressed by a
	tool has a length at 12, and the footer ends the header) */
	if (read_u32(map) != HEADER_HEAD || read_u32(map + HEADER_BYTES - 4) != HEADER_FOOT ||
		(read_u32(map + 4) != VERSION_PC && read_u32(map + 4) != VERSION_CUSTOM_EDITION) ||
		read_u32(map + 8) > map_bytes || read_u32(map + 8) < HEADER_BYTES || read_u32(map + 12))
	{
		free(map);
		return 0;
	}
	for (index = 0; index < PICTURE_COUNT; index++)
	{
		struct located *located = &install.located[index];

		memset(located, 0, sizeof(*located));
		if (locate(map, read_u32(map + 8), &pictures[index], located))
		{
			if (located->in_bitmaps_map)
			{
				if (!bitmaps_bytes)
					bitmaps_bytes = file_size(install.bitmaps_map);
				if (!fits(located->offset, located->bytes, bitmaps_bytes))
					located->found = 0;
			}
			else if (!fits(located->offset, located->bytes, read_u32(map + 8)))
			{
				located->found = 0;
			}
			any |= located->found;
		}
	}
	free(map);

	return any;
}

/* ---------- the install */

/* a folder given as a path: its ui.map's path, in path, if it has one in it
or in the places a Halo install keeps its maps */
static int folder_find(char const *folder, char *path, char *bitmaps)
{
	static char const *const places[] =
	{
		"", "/maps", "/Maps", "/GameData/Maps", "/GameData/maps", "/Contents/Resources/GameData/Maps",
	};
	size_t length = strlen(folder);
	long place;

	if (!length || length > 600)
		return 0;
	/* (a data root's name may end in a separator) */
	while (length && (folder[length - 1] == '/' || folder[length - 1] == '\\'))
		length--;
	if (!length)
		return 0;
	for (place = 0; place < (long)(sizeof(places) / sizeof(*places)); place++)
	{
		FILE *file;

		sprintf(path, "%.*s%s/ui.map", (int)length, folder, places[place]);
		file = fopen(path, "rb");
		if (file)
		{
			fclose(file);
			sprintf(bitmaps, "%.*s%s/bitmaps.map", (int)length, folder, places[place]);

			return 1;
		}
	}

	return 0;
}

static void install_find(void)
{
	char folder[PATH_BYTES];
#ifdef HALO_ANDROID
	/* (the app has no such setting: its Custom Edition maps are in the data folder) */
	char const *configured = "";
#else
	char const *configured = config_string("paths.pc_data");
#endif
	char const *home = getenv("HOME");
	int step;

	install.searched = 1;
	for (step = 0; step < 5 && !install.present; step++)
	{
		/* (a folder that paths.pc_data names is the only one looked in: an
		empty one means no install) */
		if (step && configured && *configured)
			break;
		folder[0] = 0;
		switch (step)
		{
		case 0:
			snprintf(folder, sizeof(folder), "%s", configured ? configured : "");
			break;
#ifdef __APPLE__
		/* MacSoft's Halo: where its disc image puts it */
		case 1:
			snprintf(folder, sizeof(folder), "/Applications/Halo.app/Contents/Resources/GameData");
			break;
		case 2:
			if (home && *home)
				snprintf(folder, sizeof(folder), "%s/Applications/Halo.app/Contents/Resources/GameData", home);
			break;
#endif
		case 3:
			snprintf(folder, sizeof(folder), "%s", config_string("paths.custom_edition"));
			break;
#ifndef __APPLE__
		/* (the folder custom_edition_cache.c looks in first) */
		case 4:
			{
				char const *root = platform_data_root();

				if (root && strlen(root) < 500)
					snprintf(folder, sizeof(folder), "%s/custom_maps", root);
			}
			break;
#endif
		default:
			break;
		}
		(void)home;
		if (folder[0] && folder_find(folder, install.ui_map, install.bitmaps_map))
		{
			/* (an install that has none of the pictures goes on to the next) */
			if (install_read())
				install.present = 1;
		}
	}
}

/* ---------- decoding */

static void expand565(uint32_t value, uint8_t *rgba)
{
	uint32_t r = value >> 11, g = (value >> 5) & 63, b = value & 31;

	rgba[0] = (uint8_t)((r << 3) | (r >> 2));
	rgba[1] = (uint8_t)((g << 2) | (g >> 4));
	rgba[2] = (uint8_t)((b << 3) | (b >> 2));
	rgba[3] = 255;
}

/* the picture's RGBA texels, top row first, from its stored bytes (exactly
what locate counted) */
static void decode(struct located const *located, uint8_t const *source, uint8_t *rgba)
{
	uint32_t width = located->width, height = located->height, format = located->format;
	uint32_t texel, block_x, block_y, x, y;

	if (format == _format_dxt1 || format == _format_dxt3 || format == _format_dxt5)
	{
		uint32_t blocks_wide = (width + 3) / 4;
		uint32_t block_bytes = format == _format_dxt1 ? 8 : 16;

		for (block_y = 0; block_y < (height + 3) / 4; block_y++)
		{
			for (block_x = 0; block_x < blocks_wide; block_x++)
			{
				uint8_t const *block = source + (block_y * blocks_wide + block_x) * block_bytes;
				uint8_t const *colors = block + (format == _format_dxt1 ? 0 : 8);
				uint32_t c0 = read_u16(colors), c1 = read_u16(colors + 2), choices = read_u32(colors + 4);
				uint8_t palette[4][4];
				uint8_t alpha[8];
				uint64_t alpha_bits = 0;
				int four_colors = format != _format_dxt1 || c0 > c1;
				uint32_t channel;

				expand565(c0, palette[0]);
				expand565(c1, palette[1]);
				for (channel = 0; channel < 3; channel++)
				{
					palette[2][channel] = (uint8_t)(four_colors ?
						(2 * palette[0][channel] + palette[1][channel]) / 3 : (palette[0][channel] + palette[1][channel]) / 2);
					palette[3][channel] = (uint8_t)(four_colors ? (palette[0][channel] + 2 * palette[1][channel]) / 3 : 0);
				}
				palette[2][3] = 255;
				palette[3][3] = four_colors ? 255 : 0;
				if (format == _format_dxt5)
				{
					uint32_t step;

					alpha[0] = block[0];
					alpha[1] = block[1];
					if (alpha[0] > alpha[1])
					{
						for (step = 2; step < 8; step++)
							alpha[step] = (uint8_t)(((8 - step) * alpha[0] + (step - 1) * alpha[1]) / 7);
					}
					else
					{
						for (step = 2; step < 6; step++)
							alpha[step] = (uint8_t)(((6 - step) * alpha[0] + (step - 1) * alpha[1]) / 5);
						alpha[6] = 0;
						alpha[7] = 255;
					}
					for (step = 0; step < 6; step++)
						alpha_bits |= (uint64_t)block[2 + step] << (step * 8);
				}
				for (y = 0; y < 4; y++)
				{
					for (x = 0; x < 4; x++)
					{
						uint32_t k = y * 4 + x;
						uint8_t *out;

						if (block_x * 4 + x >= width || block_y * 4 + y >= height)
							continue;
						out = rgba + ((block_y * 4 + y) * width + block_x * 4 + x) * 4;
						memcpy(out, palette[(choices >> (k * 2)) & 3], 4);
						if (format == _format_dxt3)
							out[3] = (uint8_t)(((block[k / 2] >> ((k & 1) * 4)) & 15) * 17);
						else if (format == _format_dxt5)
							out[3] = alpha[(uint32_t)(alpha_bits >> (k * 3)) & 7];
					}
				}
			}
		}
		return;
	}
	for (texel = 0; texel < width * height; texel++)
	{
		uint8_t *out = rgba + texel * 4;

		if (format == _format_r5g6b5)
		{
			expand565(read_u16(source + texel * 2), out);
		}
		else if (format == _format_a1r5g5b5)
		{
			uint32_t value = read_u16(source + texel * 2), r = (value >> 10) & 31, g = (value >> 5) & 31, b = value & 31;

			out[0] = (uint8_t)((r << 3) | (r >> 2));
			out[1] = (uint8_t)((g << 3) | (g >> 2));
			out[2] = (uint8_t)((b << 3) | (b >> 2));
			out[3] = (value & 0x8000) ? 255 : 0;
		}
		else if (format == _format_a4r4g4b4)
		{
			uint32_t value = read_u16(source + texel * 2);

			out[0] = (uint8_t)(((value >> 8) & 15) * 17);
			out[1] = (uint8_t)(((value >> 4) & 15) * 17);
			out[2] = (uint8_t)((value & 15) * 17);
			out[3] = (uint8_t)((value >> 12) * 17);
		}
		else
		{
			/* (B, G, R, A in the file) */
			out[0] = source[texel * 4 + 2];
			out[1] = source[texel * 4 + 1];
			out[2] = source[texel * 4];
			out[3] = format == _format_a8r8g8b8 ? source[texel * 4 + 3] : 255;
		}
	}
}

/* ---------- public code */

unsigned char *halo_menus_pc_art(char const *png, long *width, long *height)
{
	long index;
	struct located const *located;
	uint8_t *source;
	uint8_t *rgba;

	if (!png)
		return NULL;
	for (index = 0; index < PICTURE_COUNT; index++)
	{
		if (!strcmp(png, pictures[index].png))
			break;
	}
	if (index == PICTURE_COUNT)
		return NULL;
	if (!install.searched)
		install_find();
	if (!install.present)
		return NULL;
	located = &install.located[index];
	if (!located->found)
		return NULL;
	source = (uint8_t *)malloc(located->bytes);
	rgba = (uint8_t *)malloc((size_t)located->width * located->height * 4);
	if (!source || !rgba ||
		!file_read_range(located->in_bitmaps_map ? install.bitmaps_map : install.ui_map, located->offset, located->bytes, source))
	{
		free(source);
		free(rgba);
		return NULL;
	}
	decode(located, source, rgba);
	free(source);
	*width = (long)located->width;
	*height = (long)located->height;

	return rgba;
}

char const *halo_menus_pc_art_source(void)
{
	if (!install.searched)
		install_find();

	return install.present ? install.ui_map : NULL;
}
