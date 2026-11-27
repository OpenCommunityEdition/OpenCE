/*
MOD_BITMAPS.C

The bitmaps of mods (mods.c): each of a map's bitmap tags that a mod has a
tag file of (tags\...\name.bitmap) is drawn from that file instead of the
map, its textures, sprites and HUD pictures, and mods_reload reads them
again.

A tag file is Halo PC's (big-endian, version 7): the bitmap, then its
compressed color plate, its processed pixel data, its sequences, each
sequence's sprites, and its bitmaps, as the Halo Editing Kit's tool and
Invader write them (bitmap_group.c's fields). All but the pixels is read
when the map loads, and checked: every count and offset against the file,
which must end where its bitmaps do, every bitmap as the texture cache can
draw it from pixels laid out as Halo PC's are (the Custom Edition maps'
check, custom_edition_bitmap_drawable), and every sequence's and sprite's
bitmap among its bitmaps. A non-power-of-two 2D bitmap is drawn linear, as
a Custom Edition map's is. A file is also left out if what the map's other
tags ask of the bitmap it replaces is not there: as many bitmaps of the same
types, as many sequences, and each with as many sprites (a mod's bitmap is
the map's redrawn, larger or not).

The HUD draws a bitmap at its own size (hud_draw.c), so a bitmap a redraw
made larger, every one of its sprites (or, with none, its bitmaps) by the
same ratio, has each HUD placement that draws it
(custom_edition_hud_placements_walk) scaled by the inverse: the HUD keeps
its size, sharper. A bitmap the HUD draws that is not the map's redrawn so
(its sprites made to other sizes: Halo PC's HUD is not the Xbox's) is left
out, and the map's drawn. The HUD's digits are drawn as the digits tag
says, whose bitmap's half HUD scale flag halves their size
(hud_draw_numbers): a mod's digits twice the map's size get it, of the
map's size lose it, and of any other are left out. Menus draw their bitmaps
in their widgets' bounds, whatever their size.

A bitmap's pixels are read from the file as the texture cache loads them
(mod_bitmaps_read, xbox_texture_cache.c), into its cache block, and laid out
as the hardware wants them as a Custom Edition map's are
(rasterizer_xbox_bitmap_rebuild_hardware_format); the renderer is told
which keep their channels where Halo PC does (multipurpose maps and HUD
meters: custom_edition_bitmaps.c), unless the mod's mod.toml says its keep
them where the Xbox does (channel_order = "xbox": bitmaps taken from an Xbox
map). A bitmap whose pixels are more than a quarter of the texture cache is
left out: it could not be drawn with what the map draws around it. A bitmap is drawn, not simulated: nothing a network
game's machines agree on reads one.

The map's bitmap tag takes the file's values in place, so everything that
holds the tag keeps holding it, and its own come back when the file goes.
When a reload changes any, the texture cache lets go of every texture first;
a bitmap made is kept until the map goes, as anything may still hold it.
*/

/* ---------- headers */

#include "cseries.h"
#include "errors.h"
#include "bitmaps/bitmaps.h"
#include "bitmaps/bitmap_group.h"
#include "cache/cache_files.h"
#include "cache/texture_cache.h"
#include "interface/hud_definitions.h"
#include "interface/unit_hud_interface_definition.h"
#include "main/console.h"
#include "rasterizer/rasterizer_swizzle.h"
#include "tag_files/tag_groups.h"
#include "cache_file_formats.h"
#include "custom_edition_cache.h"
#include "mods.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* the platform layer's (port/linux/src) */
void halo_custom_edition_texels_channels(void const *texels, unsigned char channel_order);

/* the game's (port) */
char *tag_get_name(long tag_index);

/* ---------- constants */

/* a bitmap tag's file: tags\<its name>.bitmap */
#define TAG_FILE_EXTENSION ".bitmap"
/* (bitmap_group.c's bitmap_group) */
#define BITMAP_TAG_VERSION 7

/* the sizes of a tag file's bitmap, sequence, sprite and bitmap data
(bitmap_group.h's structures, as the file has them) */
#define BITMAP_GROUP_SIZE 0x6C
#define SEQUENCE_SIZE 0x40
#define SPRITE_SIZE 0x20
#define BITMAP_DATA_SIZE 0x30

/* the tool's limits (bitmap_group.c's blocks) */
#define MAXIMUM_SEQUENCES 256
#define MAXIMUM_SPRITES_PER_SEQUENCE 64
#define MAXIMUM_BITMAPS 2048
/* the device's (d3d8_gl.c, D3DDevice_GetDeviceCaps) */
#define MAXIMUM_BITMAP_SIZE 4096
#define MAXIMUM_VOLUME_BITMAP_SIZE 512
/* the most of the texture cache one bitmap's pixels may take */
#define MAXIMUM_BITMAP_PIXELS_SIZE (HALO_PORT_TEXTURE_CACHE_SIZE / 4)

/* bitmaps.c's bitmap types and flags, and bitmap_group.c's group types */
enum
{
	_bitmap_type_2d,
	_bitmap_type_3d,
	_bitmap_type_cube_map,
	NUMBER_OF_BITMAP_TYPES,
};

enum
{
	_bitmap_format_dxt1 = 14,
	_bitmap_format_dxt5 = 16,
	NUMBER_OF_BITMAP_FORMATS = 18,
};

enum
{
	_bitmap_has_power_of_two_dimensions_bit,
	_bitmap_compressed_bit,
	_bitmap_palettized_bit,
	_bitmap_swizzled_bit,
	_bitmap_linear_bit,
	_bitmap_v16u16_bit,
	_bitmap_allocated_bit,
	_bitmap_cached_bit,
};

enum
{
	_bitmap_group_type_sprites = 3,
	NUMBER_OF_BITMAP_GROUP_TYPES = 5,

	/* (hud_draw.c's) */
	_bitmap_group_half_hud_scale_bit = 4,
};

/* the HUD's digits' tag (hud_draw.c): their bitmap first */
#define HUD_NUMBER_GROUP_TAG 'hud#'

typedef char verify_bitmap_group_size[sizeof(struct bitmap_group) == BITMAP_GROUP_SIZE ? 1 : -1];
typedef char verify_bitmap_group_sequence_size[sizeof(struct bitmap_group_sequence) == SEQUENCE_SIZE ? 1 : -1];
typedef char verify_bitmap_group_sprite_size[sizeof(struct bitmap_group_sprite) == SPRITE_SIZE ? 1 : -1];
typedef char verify_bitmap_data_size[sizeof(struct bitmap_data) == BITMAP_DATA_SIZE ? 1 : -1];

/* ---------- structures */

/* a bitmap tag file read, and what it makes */
struct mod_bitmap
{
	struct mod_bitmap *next;
	long tag_index;
	/* the file (its index in the mods' files of the last time they were
	found), and its path, size and modification time, to tell whether it
	changed */
	long file;
	char *path;
	unsigned long file_size;
	unsigned long stamp[2];
	/* where its processed pixel data is in the file, and how much there is */
	unsigned long pixel_data_offset;
	unsigned long pixel_data_size;
	/* the definition it makes, and its sequences, sprites and bitmaps */
	struct bitmap_group group;
	struct bitmap_group_sequence *sequences;
	struct bitmap_group_sprite *sprites;
	struct bitmap_data *bitmaps;
	unsigned long kept_bytes;
	/* the map's size of what it draws to its own, the same for every
	sprite or bitmap, by which the HUD's placements that draw it are scaled
	(1: as they are; 0: not the same, which the HUD cannot draw) */
	real hud_scale;
	/* the HUD's digits, drawn as the digits tag says (not scaled) */
	boolean digits;
};

/* a HUD placement's scale before it was scaled for a mod's bitmap */
struct hud_placement_scale
{
	real *scale;
	real original[2];
};

/* a bitmap of the map drawn from a file: the map's own definition, given
back when it goes */
struct mod_bitmap_override
{
	struct mod_bitmap *bitmap;
	struct bitmap_group map_group;
};

static struct
{
	/* by the map's tag, its override (none: the map's own) */
	struct mod_bitmap_override *overrides;
	long override_count;
	/* every bitmap made while the map is loaded */
	struct mod_bitmap *bitmaps;
	/* the HUD placements scaled for them, with their scales before */
	struct hud_placement_scale *scales;
	long scale_count;
	long scale_capacity;
	/* whether the channel orders of the mods' bitmaps were found
	(custom_edition_loaded_reordered_bitmaps_find), and a pixel read that
	failed was said */
	boolean orders_found;
	boolean read_failure_reported;
} mod_bitmaps_globals;

/* ---------- private code */

static void mod_bitmap_free(
	struct mod_bitmap *bitmap)
{
	/* (the game's free, debug_free, does not take NULL) */
	if (bitmap->path)
		free(bitmap->path);
	if (bitmap->sequences)
		free(bitmap->sequences);
	if (bitmap->sprites)
		free(bitmap->sprites);
	if (bitmap->bitmaps)
		free(bitmap->bitmaps);
	free(bitmap);

	return;
}

static boolean power_of_two(
	long value)
{
	return value > 0 && !(value & (value - 1));
}

static short floor_log2_of(
	long value)
{
	short log = 0;

	while (value > 1)
	{
		value >>= 1;
		log++;
	}

	return log;
}

/* whether a format is one bitmaps.c has (not one of the unused) */
static boolean format_known(
	short format)
{
	switch (format)
	{
	case 4: case 5: case 7: case 12: case 13:
		return FALSE;
	default:
		return format >= 0 && format < NUMBER_OF_BITMAP_FORMATS;
	}
}

/* Makes the bitmap read from `bytes` (a file's bitmap data, its pixels at
`pixels_offset` in a processed pixel data of `pixel_data_size` bytes) one
the texture cache can draw, as the tag `tag_index`'s; FALSE with *reason
saying why it cannot be. */
static boolean bitmap_data_read(
	struct bitmap_data *bitmap,
	byte const *bytes,
	long tag_index,
	unsigned long pixel_data_size,
	char const **reason)
{
	boolean compressed;
	short maximum_size;
	unsigned long end;

	memset(bitmap, 0, sizeof(*bitmap));
	bitmap->signature = mods_read_be32(bytes + offsetof(struct bitmap_data, signature));
	bitmap->width = mods_read_short(bytes + offsetof(struct bitmap_data, width));
	bitmap->height = mods_read_short(bytes + offsetof(struct bitmap_data, height));
	bitmap->depth = mods_read_short(bytes + offsetof(struct bitmap_data, depth));
	bitmap->type = mods_read_short(bytes + offsetof(struct bitmap_data, type));
	bitmap->format = mods_read_short(bytes + offsetof(struct bitmap_data, format));
	bitmap->flags = (unsigned short)mods_read_be16(bytes + offsetof(struct bitmap_data, flags));
	bitmap->registration_point.x = mods_read_short(bytes + offsetof(struct bitmap_data, registration_point));
	bitmap->registration_point.y = mods_read_short(bytes + offsetof(struct bitmap_data, registration_point) + 2);
	bitmap->mipmap_count = mods_read_short(bytes + offsetof(struct bitmap_data, mipmap_count));
	bitmap->pixels_offset = (long)mods_read_be32(bytes + offsetof(struct bitmap_data, pixels_offset));

	*reason = "a bitmap is of a type, format or size this build does not draw";
	maximum_size = bitmap->type == _bitmap_type_3d ? MAXIMUM_VOLUME_BITMAP_SIZE : MAXIMUM_BITMAP_SIZE;
	if (bitmap->signature != BITMAP_GROUP_TAG ||
		bitmap->type < 0 || bitmap->type >= NUMBER_OF_BITMAP_TYPES || !format_known(bitmap->format) ||
		bitmap->width < 1 || bitmap->width > maximum_size ||
		bitmap->height < 1 || bitmap->height > maximum_size ||
		bitmap->depth < 1 || bitmap->depth > (bitmap->type == _bitmap_type_3d ? maximum_size : 1) ||
		bitmap->mipmap_count < 0 ||
		bitmap->mipmap_count > floor_log2_of(MAX(bitmap->width, MAX(bitmap->height, bitmap->depth))))
	{
		return FALSE;
	}
	/* (what the tool says of it, the game's flags; of the rest, a map's
	build marks it cached) */
	compressed = bitmap->format >= _bitmap_format_dxt1 && bitmap->format <= _bitmap_format_dxt5;
	bitmap->flags &= FLAG(_bitmap_has_power_of_two_dimensions_bit) | FLAG(_bitmap_palettized_bit) |
		FLAG(_bitmap_linear_bit) | FLAG(_bitmap_v16u16_bit);
	SET_FLAG(bitmap->flags, _bitmap_compressed_bit, compressed);
	SET_FLAG(bitmap->flags, _bitmap_has_power_of_two_dimensions_bit,
		power_of_two(bitmap->width) && power_of_two(bitmap->height) && power_of_two(bitmap->depth));
	/* Halo PC draws a 2D bitmap of any size, this build swizzles all but
	linear ones: an uncompressed one that has not got power-of-two sides
	is drawn linear, as a Custom Edition map's is (cache_file_formats.c,
	bitmap_npot_make_linear) */
	if (bitmap->type == _bitmap_type_2d && !compressed &&
		!TEST_FLAG(bitmap->flags, _bitmap_has_power_of_two_dimensions_bit))
	{
		SET_FLAG(bitmap->flags, _bitmap_linear_bit, TRUE);
	}
	if (!bitmap_verify(bitmap, FALSE))
	{
		return FALSE;
	}
	bitmap->pixels_size = bitmap_get_pixel_data_size(bitmap);
	if (!custom_edition_bitmap_drawable(bitmap))
	{
		return FALSE;
	}
	*reason = "a bitmap's pixels are not in the file";
	if (bitmap->pixels_offset < 0 ||
		!mods_in_file((unsigned long)bitmap->pixels_offset, (unsigned long)bitmap->pixels_size, pixel_data_size))
	{
		return FALSE;
	}
	*reason = "a bitmap is larger than the texture cache can draw among others";
	/* (what the texture cache takes for it: the larger of its sizes) */
	end = (unsigned long)rasterizer_xbox_bitmap_get_pixel_data_size(bitmap);
	if (MAX(end, (unsigned long)bitmap->pixels_size) > MAXIMUM_BITMAP_PIXELS_SIZE)
	{
		return FALSE;
	}
	SET_FLAG(bitmap->flags, _bitmap_cached_bit, TRUE);
	bitmap->tag_index = tag_index;
	bitmap->cache_block_index = NONE;
	bitmap->hardware_format = NULL;
	bitmap->base_address = NULL;

	return TRUE;
}

static real difference(
	real first,
	real second)
{
	return first > second ? first - second : second - first;
}

/* the ratio of the size the map's bitmap `map_group` draws a sprite at to
the size the bitmap read, `bitmap`, draws it (both of the same sequences and
sprites: a sheet the tool made again lays them out anew), or of their
bitmaps, the same for all of them; 0 if it is not the same */
static real hud_scale_find(
	struct bitmap_group const *map_group,
	struct mod_bitmap const *bitmap)
{
	real scale = 0.f;
	boolean sprites = FALSE;
	long sequence_index;
	long index;

	for (sequence_index = 0; sequence_index < map_group->sequences.count; sequence_index++)
	{
		struct bitmap_group_sequence const *map_sequence =
			TAG_BLOCK_GET_ELEMENT(&map_group->sequences, sequence_index, struct bitmap_group_sequence);
		struct bitmap_group_sequence const *sequence = &bitmap->sequences[sequence_index];

		for (index = 0; index < map_sequence->sprites.count; index++)
		{
			struct bitmap_group_sprite const *map_sprite =
				TAG_BLOCK_GET_ELEMENT(&map_sequence->sprites, index, struct bitmap_group_sprite);
			struct bitmap_group_sprite const *sprite = (struct bitmap_group_sprite const *)sequence->sprites.address + index;
			struct bitmap_data const *sprite_bitmap = &bitmap->bitmaps[sprite->bitmap_index];
			struct bitmap_data const *map_bitmap =
				TAG_BLOCK_GET_ELEMENT(&map_group->bitmaps, map_sprite->bitmap_index, struct bitmap_data);
			real map_width = (map_sprite->bounds.x1 - map_sprite->bounds.x0) * map_bitmap->width;
			real map_height = (map_sprite->bounds.y1 - map_sprite->bounds.y0) * map_bitmap->height;
			real width = (sprite->bounds.x1 - sprite->bounds.x0) * sprite_bitmap->width;
			real height = (sprite->bounds.y1 - sprite->bounds.y0) * sprite_bitmap->height;

			/* (a sprite of no size draws nothing) */
			if (map_width < 1.f || map_height < 1.f || width < 1.f || height < 1.f)
			{
				continue;
			}
			sprites = TRUE;
			if (!scale)
			{
				scale = map_width / width;
			}
			/* (to the pixel the tool rounds a sprite's bounds to) */
			if (difference(map_width, width * scale) > 1.5f * MAX(scale, 1.f) ||
				difference(map_height, height * scale) > 1.5f * MAX(scale, 1.f))
			{
				return 0.f;
			}
		}
	}
	if (sprites)
	{
		return scale;
	}
	for (index = 0; index < map_group->bitmaps.count; index++)
	{
		struct bitmap_data const *map_bitmap = TAG_BLOCK_GET_ELEMENT(&map_group->bitmaps, index, struct bitmap_data);
		struct bitmap_data const *mod_bitmap = &bitmap->bitmaps[index];

		if (!scale)
		{
			scale = (real)map_bitmap->width / (real)mod_bitmap->width;
		}
		if ((real)map_bitmap->width != (real)mod_bitmap->width * scale ||
			(real)map_bitmap->height != (real)mod_bitmap->height * scale)
		{
			return 0.f;
		}
	}

	return scale ? scale : 1.f;
}

/* The bitmap the tag file `file` makes for the map's bitmap tag
`tag_index`, whose definition is `map_group`, or NULL with *reason saying
why not. */
static struct mod_bitmap *mod_bitmap_new(
	long tag_index,
	long file,
	struct bitmap_group const *map_group,
	char const **reason)
{
	byte head[MOD_TAG_FILE_HEADER_SIZE + BITMAP_GROUP_SIZE];
	byte const *root = head + MOD_TAG_FILE_HEADER_SIZE;
	struct mod_bitmap *bitmap;
	struct bitmap_group *group;
	byte *tail = NULL;
	unsigned long tail_size;
	unsigned long cursor = sizeof(head);
	unsigned long color_plate_size;
	unsigned long sequence_count;
	unsigned long bitmap_count;
	unsigned long sprite_count = 0;
	unsigned long tail_cursor;
	unsigned long index;

	*reason = "it cannot be read";
	bitmap = malloc(sizeof(*bitmap));
	if (!bitmap)
	{
		*reason = "out of memory";
		return NULL;
	}
	memset(bitmap, 0, sizeof(*bitmap));
	bitmap->tag_index = tag_index;
	bitmap->file = file;
	group = &bitmap->group;
	bitmap->path = malloc(strlen(port_mods_file_path(file)) + 1);
	if (!bitmap->path)
	{
		*reason = "out of memory";
		goto refused;
	}
	strcpy(bitmap->path, port_mods_file_path(file));
	if (!port_mods_file_stamp(file, &bitmap->file_size, bitmap->stamp))
	{
		goto refused;
	}
	*reason = "not a bitmap tag file of this version";
	if (bitmap->file_size < sizeof(head) || !port_mods_file_read(file, 0, head, sizeof(head)) ||
		!mods_tag_file_header_valid(head, BITMAP_GROUP_TAG, BITMAP_TAG_VERSION))
	{
		goto refused;
	}

	/* the bitmap's values */
	group->type = mods_read_short(root + offsetof(struct bitmap_group, type));
	group->format = mods_read_short(root + offsetof(struct bitmap_group, format));
	group->usage = mods_read_short(root + offsetof(struct bitmap_group, usage));
	group->flags = (unsigned short)mods_read_be16(root + offsetof(struct bitmap_group, flags));
	group->detail_fade = mods_read_real(root + offsetof(struct bitmap_group, detail_fade));
	group->sharpen_amount = mods_read_real(root + offsetof(struct bitmap_group, sharpen_amount));
	group->bump_height = mods_read_real(root + offsetof(struct bitmap_group, bump_height));
	group->sprite_budget_size = mods_read_short(root + offsetof(struct bitmap_group, sprite_budget_size));
	group->sprite_budget_count = (unsigned short)mods_read_be16(root + offsetof(struct bitmap_group, sprite_budget_count));
	group->import_width = mods_read_short(root + offsetof(struct bitmap_group, import_width));
	group->import_height = mods_read_short(root + offsetof(struct bitmap_group, import_height));
	group->smoothing_filter_size = mods_read_real(root + offsetof(struct bitmap_group, smoothing_filter_size));
	group->alpha_bias = mods_read_real(root + offsetof(struct bitmap_group, alpha_bias));
	group->mipmap_count = mods_read_short(root + offsetof(struct bitmap_group, mipmap_count));
	group->sprite_usage = mods_read_short(root + offsetof(struct bitmap_group, sprite_usage));
	group->sprite_spacing = mods_read_short(root + offsetof(struct bitmap_group, sprite_spacing));
	color_plate_size = mods_read_be32(root + offsetof(struct bitmap_group, import_bitmap.size));
	bitmap->pixel_data_size = mods_read_be32(root + offsetof(struct bitmap_group, pixel_data.size));
	sequence_count = mods_read_be32(root + offsetof(struct bitmap_group, sequences.count));
	bitmap_count = mods_read_be32(root + offsetof(struct bitmap_group, bitmaps.count));
	*reason = "a value of the bitmap is out of range";
	if (group->type < 0 || group->type >= NUMBER_OF_BITMAP_GROUP_TYPES ||
		sequence_count > MAXIMUM_SEQUENCES || bitmap_count < 1 || bitmap_count > MAXIMUM_BITMAPS)
	{
		goto refused;
	}

	/* the color plate (not read), the pixels (read as they are drawn), then
	the sequences, their sprites and the bitmaps, read whole */
	*reason = "its pixels are not in the file";
	if (!mods_in_file(cursor, color_plate_size, bitmap->file_size))
	{
		goto refused;
	}
	cursor += color_plate_size;
	if (!mods_in_file(cursor, bitmap->pixel_data_size, bitmap->file_size))
	{
		goto refused;
	}
	bitmap->pixel_data_offset = cursor;
	cursor += bitmap->pixel_data_size;
	tail_size = bitmap->file_size - cursor;
	*reason = "its sequences and bitmaps are not in the file";
	if (tail_size < sequence_count * SEQUENCE_SIZE + bitmap_count * BITMAP_DATA_SIZE ||
		tail_size > MAXIMUM_SEQUENCES * (SEQUENCE_SIZE + MAXIMUM_SPRITES_PER_SEQUENCE * SPRITE_SIZE) +
			MAXIMUM_BITMAPS * BITMAP_DATA_SIZE)
	{
		goto refused;
	}
	tail = malloc(tail_size);
	if (!tail)
	{
		*reason = "out of memory";
		goto refused;
	}
	*reason = "it cannot be read";
	if (!port_mods_file_read(file, cursor, tail, tail_size))
	{
		goto refused;
	}

	/* the sequences, then each one's sprites */
	*reason = "out of memory";
	bitmap->sequences = malloc(MAX(sequence_count, 1) * sizeof(*bitmap->sequences));
	bitmap->bitmaps = malloc(bitmap_count * sizeof(*bitmap->bitmaps));
	if (!bitmap->sequences || !bitmap->bitmaps)
	{
		goto refused;
	}
	memset(bitmap->sequences, 0, MAX(sequence_count, 1) * sizeof(*bitmap->sequences));
	*reason = "its sprites are not in the file";
	for (index = 0; index < sequence_count; index++)
	{
		byte const *bytes = tail + index * SEQUENCE_SIZE;
		unsigned long count = mods_read_be32(bytes + offsetof(struct bitmap_group_sequence, sprites.count));

		if (count > MAXIMUM_SPRITES_PER_SEQUENCE)
		{
			goto refused;
		}
		sprite_count += count;
	}
	tail_cursor = sequence_count * SEQUENCE_SIZE;
	if (!mods_in_file(tail_cursor, sprite_count * SPRITE_SIZE + bitmap_count * BITMAP_DATA_SIZE, tail_size))
	{
		goto refused;
	}
	*reason = "out of memory";
	bitmap->sprites = malloc(MAX(sprite_count, 1) * sizeof(*bitmap->sprites));
	if (!bitmap->sprites)
	{
		goto refused;
	}
	sprite_count = 0;
	for (index = 0; index < sequence_count; index++)
	{
		byte const *bytes = tail + index * SEQUENCE_SIZE;
		struct bitmap_group_sequence *sequence = &bitmap->sequences[index];
		unsigned long count = mods_read_be32(bytes + offsetof(struct bitmap_group_sequence, sprites.count));
		unsigned long sprite_index;

		memcpy(sequence->name, bytes, sizeof(sequence->name));
		sequence->name[sizeof(sequence->name) - 1] = 0;
		sequence->first_bitmap_index = mods_read_short(bytes + offsetof(struct bitmap_group_sequence, first_bitmap_index));
		sequence->bitmap_count = mods_read_short(bytes + offsetof(struct bitmap_group_sequence, bitmap_count));
		/* (a sprites group's sequences are their sprites, whatever the tool
		left in these: bitmap_group.c, postprocess_bitmap_group) */
		if (group->type == _bitmap_group_type_sprites)
		{
			sequence->first_bitmap_index = 0;
			sequence->bitmap_count = 0;
		}
		sequence->sprites.count = (long)count;
		sequence->sprites.address = count ? bitmap->sprites + sprite_count : NULL;
		sequence->sprites.definition = NULL;
		*reason = "a sequence names bitmaps it has not got";
		if (sequence->first_bitmap_index < 0 || sequence->bitmap_count < 0 ||
			(unsigned long)sequence->first_bitmap_index + (unsigned long)sequence->bitmap_count > bitmap_count)
		{
			goto refused;
		}
		for (sprite_index = 0; sprite_index < count; sprite_index++)
		{
			byte const *sprite_bytes = tail + tail_cursor;
			struct bitmap_group_sprite *sprite = &bitmap->sprites[sprite_count++];

			memset(sprite, 0, sizeof(*sprite));
			sprite->bitmap_index = mods_read_short(sprite_bytes + offsetof(struct bitmap_group_sprite, bitmap_index));
			sprite->bounds.x0 = mods_read_real(sprite_bytes + offsetof(struct bitmap_group_sprite, bounds.x0));
			sprite->bounds.x1 = mods_read_real(sprite_bytes + offsetof(struct bitmap_group_sprite, bounds.x1));
			sprite->bounds.y0 = mods_read_real(sprite_bytes + offsetof(struct bitmap_group_sprite, bounds.y0));
			sprite->bounds.y1 = mods_read_real(sprite_bytes + offsetof(struct bitmap_group_sprite, bounds.y1));
			sprite->registration_point.x = mods_read_real(sprite_bytes + offsetof(struct bitmap_group_sprite, registration_point.x));
			sprite->registration_point.y = mods_read_real(sprite_bytes + offsetof(struct bitmap_group_sprite, registration_point.y));
			tail_cursor += SPRITE_SIZE;
			*reason = "a sprite is of a bitmap it has not got, or out of range";
			if (sprite->bitmap_index < 0 || (unsigned long)sprite->bitmap_index >= bitmap_count ||
				!mods_finite_real(sprite->bounds.x0) || !mods_finite_real(sprite->bounds.x1) ||
				!mods_finite_real(sprite->bounds.y0) || !mods_finite_real(sprite->bounds.y1) ||
				!mods_finite_real(sprite->registration_point.x) || !mods_finite_real(sprite->registration_point.y))
			{
				goto refused;
			}
		}
	}

	/* the bitmaps, which must end the file */
	for (index = 0; index < bitmap_count; index++)
	{
		if (!bitmap_data_read(&bitmap->bitmaps[index], tail + tail_cursor, tag_index, bitmap->pixel_data_size, reason))
		{
			goto refused;
		}
		tail_cursor += BITMAP_DATA_SIZE;
	}
	*reason = "the file goes on past its last bitmap";
	if (tail_cursor != tail_size)
	{
		goto refused;
	}
	free(tail);
	tail = NULL;

	/* what the map's other tags ask of the bitmap (as many sequences: what
	draws it keeps a sequence's index, which must still be one when the map's
	bitmap comes back, weather_particle_systems.c) */
	*reason = "it has fewer bitmaps or sprites than the map's, other sequences, or bitmaps of other types";
	if ((long)bitmap_count < map_group->bitmaps.count || (long)sequence_count != map_group->sequences.count)
	{
		goto refused;
	}
	for (index = 0; index < (unsigned long)map_group->bitmaps.count; index++)
	{
		struct bitmap_data const *map_bitmap = TAG_BLOCK_GET_ELEMENT(&map_group->bitmaps, index, struct bitmap_data);

		if (map_bitmap->type != bitmap->bitmaps[index].type)
		{
			goto refused;
		}
	}
	for (index = 0; index < (unsigned long)map_group->sequences.count; index++)
	{
		struct bitmap_group_sequence const *map_sequence =
			TAG_BLOCK_GET_ELEMENT(&map_group->sequences, index, struct bitmap_group_sequence);
		struct bitmap_group_sequence const *sequence = &bitmap->sequences[index];

		if (sequence->sprites.count < map_sequence->sprites.count ||
			(map_sequence->bitmap_count > 0 && sequence->bitmap_count <= 0))
		{
			goto refused;
		}
	}

	/* (the HUD draws it at the map's size if what it draws, its sprites or
	its bitmaps, is all larger by one ratio) */
	bitmap->hud_scale = hud_scale_find(map_group, bitmap);

	group->import_bitmap.size = 0;
	group->import_bitmap.address = NULL;
	group->pixel_data.size = (long)bitmap->pixel_data_size;
	group->pixel_data.file_offset = 0;
	group->pixel_data.address = NULL;
	group->sequences.count = (long)sequence_count;
	group->sequences.address = sequence_count ? bitmap->sequences : NULL;
	group->bitmaps.count = (long)bitmap_count;
	group->bitmaps.address = bitmap->bitmaps;
	bitmap->kept_bytes = sizeof(*bitmap) + MAX(sequence_count, 1) * sizeof(*bitmap->sequences) +
		MAX(sprite_count, 1) * sizeof(*bitmap->sprites) + bitmap_count * sizeof(*bitmap->bitmaps);

	return bitmap;

refused:
	if (tail)
		free(tail);
	mod_bitmap_free(bitmap);

	return NULL;
}

/* the override of the bitmap tag `tag_index` */
static struct mod_bitmap_override *override_get(
	long tag_index)
{
	long absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(tag_index);

	return absolute_index >= 0 && absolute_index < mod_bitmaps_globals.override_count ?
		&mod_bitmaps_globals.overrides[absolute_index] : NULL;
}

/* Makes the table of overrides, one for each of the map's tags up to its
last bitmap tag; FALSE when it cannot be. */
static boolean overrides_new(
	void)
{
	struct tag_iterator iterator;
	long tag_index;
	long count = 0;

	if (mod_bitmaps_globals.overrides)
	{
		return TRUE;
	}
	tag_iterator_new(&iterator, BITMAP_GROUP_TAG);
	while ((tag_index = tag_iterator_next(&iterator)) != NONE)
	{
		count = MAX(count, DATUM_INDEX_TO_ABSOLUTE_INDEX(tag_index) + 1);
	}
	if (count <= 0 || !(mod_bitmaps_globals.overrides = malloc((size_t)count * sizeof(struct mod_bitmap_override))))
	{
		return FALSE;
	}
	memset(mod_bitmaps_globals.overrides, 0, (size_t)count * sizeof(struct mod_bitmap_override));
	mod_bitmaps_globals.override_count = count;

	return TRUE;
}

/* the bitmap made that `bitmap` is one of the bitmaps of, played over its
tag's, or NULL (a bitmap names its tag: bitmap_data_read) */
static struct mod_bitmap *bitmap_of(
	struct bitmap_data const *bitmap)
{
	struct mod_bitmap_override const *override = override_get(bitmap->tag_index);
	struct mod_bitmap *made = override ? override->bitmap : NULL;

	return made && bitmap >= made->bitmaps && bitmap < made->bitmaps + made->group.bitmaps.count ? made : NULL;
}

/* Gives the map's bitmap tag `tag_index` its own definition back, and has
the bitmap made that was played over it not drawn (nothing should name it
now; a stale pointer to one is a bitmap with no texture). */
static void bitmap_unplug(
	long tag_index,
	struct mod_bitmap_override *override)
{
	struct mod_bitmap *made = override->bitmap;
	long index;

	memcpy(bitmap_group_get(tag_index), &override->map_group, sizeof(override->map_group));
	override->bitmap = NULL;
	for (index = 0; made && index < made->group.bitmaps.count; index++)
	{
		SET_FLAG(made->bitmaps[index].flags, _bitmap_cached_bit, FALSE);
		made->bitmaps[index].hardware_format = NULL;
	}

	return;
}

/* Plays the bitmap made `made` over the map's bitmap tag `tag_index`. */
static void bitmap_plug(
	long tag_index,
	struct mod_bitmap_override *override,
	struct mod_bitmap *made)
{
	struct bitmap_group *definition = bitmap_group_get(tag_index);
	long index;

	for (index = 0; index < made->group.bitmaps.count; index++)
	{
		SET_FLAG(made->bitmaps[index].flags, _bitmap_cached_bit, TRUE);
		made->bitmaps[index].cache_block_index = NONE;
		made->bitmaps[index].hardware_format = NULL;
		made->bitmaps[index].base_address = NULL;
	}
	memcpy(&override->map_group, definition, sizeof(*definition));
	memcpy(definition, &made->group, sizeof(*definition));
	override->bitmap = made;

	return;
}

/* Lets go of a bitmap made that was never drawn (the HUD checks', as it is
made), and of the memory it was counted. */
static void bitmap_forget(
	struct mod_bitmap *made)
{
	struct mod_bitmap **reference = &mod_bitmaps_globals.bitmaps;

	while (*reference && *reference != made)
		reference = &(*reference)->next;
	if (*reference)
		*reference = made->next;
	mods_memory_give(made->kept_bytes);
	mod_bitmap_free(made);

	return;
}

/* a bitmap made before for the tag `tag_index` from the file at `path` as
it is now (`size` bytes, modified at `stamp`), played or not, or NULL */
static struct mod_bitmap *bitmap_kept(
	long tag_index,
	char const *path,
	unsigned long size,
	unsigned long const stamp[2])
{
	struct mod_bitmap *made;

	for (made = mod_bitmaps_globals.bitmaps; made; made = made->next)
	{
		if (made->tag_index == tag_index && made->file_size == size && made->stamp[0] == stamp[0] &&
			made->stamp[1] == stamp[1] && !strcmp(made->path, path))
		{
			return made;
		}
	}

	return NULL;
}

/* the scale of the HUD placements that draw the bitmap tag `handle` names */
static real hud_scale_of(
	long handle)
{
	struct mod_bitmap_override const *override = handle != NONE ? override_get(handle) : NULL;

	return override && override->bitmap && override->bitmap->tag_index == handle && override->bitmap->hud_scale > 0.f &&
		!override->bitmap->digits ? override->bitmap->hud_scale : 1.f;
}

static uint8_t *hud_block_elements(
	void *context,
	uint8_t const *block,
	uint32_t element_bytes,
	int32_t *count)
{
	struct tag_block const *elements = (struct tag_block const *)block;

	(void)context;
	(void)element_bytes;
	*count = elements->count;

	return elements->count > 0 && elements->address ? elements->address : NULL;
}

/* a placement drawing a mod's larger bitmap, scaled to draw it at the
map's size */
static void hud_placement_scale(
	void *context,
	uint8_t *placement,
	uint8_t const *bitmap,
	int items)
{
	struct hud_placement_definition *definition = (struct hud_placement_definition *)placement;
	real scale = bitmap ? hud_scale_of(((struct tag_reference const *)bitmap)->index) : 1.f;
	struct hud_placement_scale *scaled;

	(void)context;
	(void)items;
	if (scale == 1.f)
	{
		return;
	}
	if (mod_bitmaps_globals.scale_count == mod_bitmaps_globals.scale_capacity)
	{
		long capacity = mod_bitmaps_globals.scale_capacity ? 2 * mod_bitmaps_globals.scale_capacity : 64;
		struct hud_placement_scale *grown = malloc(capacity * sizeof(*grown));

		if (!grown)
		{
			return;
		}
		if (mod_bitmaps_globals.scales)
		{
			memcpy(grown, mod_bitmaps_globals.scales, mod_bitmaps_globals.scale_count * sizeof(*grown));
			free(mod_bitmaps_globals.scales);
		}
		mod_bitmaps_globals.scales = grown;
		mod_bitmaps_globals.scale_capacity = capacity;
	}
	scaled = &mod_bitmaps_globals.scales[mod_bitmaps_globals.scale_count++];
	scaled->scale = &definition->scale.i;
	scaled->original[0] = definition->scale.i;
	scaled->original[1] = definition->scale.j;
	definition->scale.i *= scale;
	definition->scale.j *= scale;

	return;
}

/* a placement drawing a mod's bitmap that is not the map's redrawn to one
ratio: the map's bitmap comes back (the texture cache holds no bitmap) */
static void hud_placement_bitmap_check(
	void *context,
	uint8_t *placement,
	uint8_t const *bitmap,
	int items)
{
	long handle = bitmap ? ((struct tag_reference const *)bitmap)->index : NONE;
	struct mod_bitmap_override *override = handle != NONE ? override_get(handle) : NULL;
	long *given_back = context;

	(void)placement;
	(void)items;
	if (override && override->bitmap && override->bitmap->tag_index == handle && override->bitmap->hud_scale <= 0.f &&
		!override->bitmap->digits)
	{
		struct mod_bitmap *made = override->bitmap;

		error(_error_silent, "mods: %s left out: the HUD draws it, and it is not the map's redrawn larger by one ratio",
			made->path);
		console_warning("mods: %s.bitmap left out: the HUD draws it, and it is not the map's redrawn larger by one ratio",
			tag_get_name(handle));
		bitmap_unplug(handle, override);
		bitmap_forget(made);
		(*given_back)++;
	}

	return;
}

/* every placement of the HUD's definitions, walked */
static void hud_placements_walk(
	struct custom_edition_hud_walk const *walk)
{
	static unsigned long const groups[] =
	{
		UNIT_HUD_INTERFACE_DEFINITION_TAG, 'wphi', 'grhi', 'hudg',
	};
	long index;

	for (index = 0; index < (long)NUMBEROF(groups); index++)
	{
		struct tag_iterator iterator;
		long tag_index;

		tag_iterator_new(&iterator, groups[index]);
		while ((tag_index = tag_iterator_next(&iterator)) != NONE)
		{
			if (tag_get_group_tag(tag_index) == groups[index])
				custom_edition_hud_placements_walk(tag_get(groups[index], tag_index), groups[index], walk);
		}
	}

	return;
}

/* Gives the map's bitmaps back where the HUD cannot draw a mod's at the
map's size: how many. */
static long hud_bitmaps_check(
	void)
{
	struct custom_edition_hud_walk walk;
	long given_back = 0;

	walk.block_elements = hud_block_elements;
	walk.placement = hud_placement_bitmap_check;
	walk.context = &given_back;
	hud_placements_walk(&walk);

	return given_back;
}

/* Gives the map's digits back where the HUD cannot draw a mod's at the
map's size, and has it draw the others at it, once each: how many were
given back. The map draws its digits at their size, or half of it with
the flag (a Custom Edition map's halved digits); a mod's twice the size get
the flag where the map's have not, a mod's of the same size the map's. */
static long hud_digits_check(
	void)
{
	struct tag_iterator iterator;
	long tag_index;
	long given_back = 0;

	tag_iterator_new(&iterator, HUD_NUMBER_GROUP_TAG);
	while ((tag_index = tag_iterator_next(&iterator)) != NONE)
	{
		struct tag_reference const *digits = tag_get(HUD_NUMBER_GROUP_TAG, tag_index);
		long handle = digits ? digits->index : NONE;
		struct mod_bitmap_override *override = handle != NONE ? override_get(handle) : NULL;
		struct mod_bitmap *made = override ? override->bitmap : NULL;

		boolean map_half;

		if (!made || made->tag_index != handle || made->digits)
		{
			continue;
		}
		map_half = TEST_FLAG(override->map_group.flags, _bitmap_group_half_hud_scale_bit);
		if (difference(made->hud_scale, 1.f) < 0.01f || (difference(made->hud_scale, 0.5f) < 0.01f && !map_half))
		{
			boolean half = map_half || difference(made->hud_scale, 0.5f) < 0.01f;

			SET_FLAG(made->group.flags, _bitmap_group_half_hud_scale_bit, half);
			SET_FLAG(bitmap_group_get(handle)->flags, _bitmap_group_half_hud_scale_bit, half);
			/* (its placements are not scaled: the digits' are their own) */
			made->digits = TRUE;
		}
		else
		{
			error(_error_silent, "mods: %s left out: the HUD draws digits at the map's size or twice it, not this one's",
				made->path);
			console_warning("mods: %s.bitmap left out: the HUD draws digits at the map's size or twice it, not this one's",
				tag_get_name(handle));
			bitmap_unplug(handle, override);
			bitmap_forget(made);
			given_back++;
		}
	}

	return given_back;
}

/* Gives the HUD's placements their scales back, then scales those that
draw the mods' bitmaps now played. */
static long hud_placements_rescale(
	void)
{
	struct custom_edition_hud_walk walk;
	long index;

	for (index = mod_bitmaps_globals.scale_count - 1; index >= 0; index--)
	{
		mod_bitmaps_globals.scales[index].scale[0] = mod_bitmaps_globals.scales[index].original[0];
		mod_bitmaps_globals.scales[index].scale[1] = mod_bitmaps_globals.scales[index].original[1];
	}
	mod_bitmaps_globals.scale_count = 0;
	walk.block_elements = hud_block_elements;
	walk.placement = hud_placement_scale;
	walk.context = NULL;
	hud_placements_walk(&walk);

	return mod_bitmaps_globals.scale_count;
}

/* ---------- public code */

long mod_bitmaps_update(
	boolean remove_all,
	boolean at_load)
{
	struct tag_iterator iterator;
	long tag_index;
	long loaded_count = 0;
	long removed_count = 0;
	long refused_count = 0;
	long played_count = 0;
	boolean flushed = FALSE;
	/* (whether a bitmap played changed: the HUD's placements and the
	channels' orders are found again) */
	boolean changed = FALSE;

	if (!mods_map_loaded() || !overrides_new())
	{
		return 0;
	}

	tag_iterator_new(&iterator, BITMAP_GROUP_TAG);
	while ((tag_index = tag_iterator_next(&iterator)) != NONE)
	{
		struct mod_bitmap_override *override = override_get(tag_index);
		long file = remove_all ? NONE : mods_tag_file_find(tag_index, TAG_FILE_EXTENSION);
		struct bitmap_group *definition = bitmap_group_get(tag_index);
		struct mod_bitmap *made = NULL;
		boolean made_now = FALSE;
		char const *reason = NULL;

		if (!override || !definition)
		{
			continue;
		}
		if (file != NONE)
		{
			unsigned long size;
			unsigned long stamp[2];

			/* (one made before from the file as it is: only where it is in the
			mods' files changes) */
			if (port_mods_file_stamp(file, &size, stamp))
				made = bitmap_kept(tag_index, port_mods_file_path(file), size, stamp);
			if (made)
				made->file = file;
			if (made && made == override->bitmap)
				continue;
		}
		if (file != NONE && !made)
		{
			made = mod_bitmap_new(tag_index, file, override->bitmap ? &override->map_group : definition, &reason);
			if (made && !mods_memory_take(made->kept_bytes))
			{
				reason = "the mods' tags already hold too much memory";
				mod_bitmap_free(made);
				made = NULL;
			}
			if (!made)
			{
				error(_error_silent, "mods: %s left out: %s", port_mods_file_path(file), reason);
				console_warning("mods: %s.bitmap of %s left out: %s", tag_get_name(tag_index), port_mods_file_mod(file),
					reason);
				refused_count++;
			}
			made_now = made != NULL;
		}
		/* (a bitmap whose file is gone or changed, read again or not, is the
		map's until another is made: its pixels are read from its file) */
		if (!made && !override->bitmap)
		{
			continue;
		}

		/* a change: no texture holds a bitmap while it is made */
		if (!flushed && !at_load)
		{
			texture_cache_flush();
			flushed = TRUE;
		}
		if (override->bitmap)
		{
			bitmap_unplug(tag_index, override);
			changed = TRUE;
			if (!made)
				removed_count++;
		}
		if (made)
		{
			bitmap_plug(tag_index, override, made);
			changed = TRUE;
			if (made_now)
			{
				made->next = mod_bitmaps_globals.bitmaps;
				mod_bitmaps_globals.bitmaps = made;
			}
			loaded_count++;
		}
	}

	/* (those the HUD cannot draw at the map's size go: no texture holds
	them yet) */
	if (loaded_count)
	{
		long given_back = hud_digits_check() + hud_bitmaps_check();

		loaded_count -= given_back;
		refused_count += given_back;
	}
	for (tag_index = 0; tag_index < mod_bitmaps_globals.override_count; tag_index++)
	{
		if (mod_bitmaps_globals.overrides[tag_index].bitmap)
			played_count++;
	}
	/* which of them keep their channels where Halo PC does, found again
	with them in the map's tags (a Custom Edition map's own bitmaps, all Halo
	PC's, are found again too: they are drawn by the same table) */
	if (changed && (played_count || custom_edition_cache_tags_loaded()))
	{
		mod_bitmaps_globals.orders_found = custom_edition_loaded_reordered_bitmaps_find();
	}
	if (changed)
	{
		long scaled_count = hud_placements_rescale();

		if (scaled_count)
			error(_error_silent, "mods: %ld HUD placements scaled to draw larger bitmaps at the map's size", scaled_count);
	}
	if (!at_load || loaded_count || refused_count)
	{
		error(_error_silent, "mods: %ld bitmaps from their files (%ld new), %ld given back, %ld left out",
			played_count, loaded_count, removed_count, refused_count);
		if (!at_load && mods_say())
		{
			console_printf(FALSE, "mods: %ld bitmaps from their files (%ld new), %ld given back, %ld left out",
				played_count, loaded_count, removed_count, refused_count);
		}
	}

	return played_count;
}

/* (once the texture cache is closed: no block holds a bitmap) */
void mod_bitmaps_tags_unloaded(
	void)
{
	struct mod_bitmap *made = mod_bitmaps_globals.bitmaps;

	while (made)
	{
		struct mod_bitmap *next = made->next;

		mod_bitmap_free(made);
		made = next;
	}
	if (mod_bitmaps_globals.overrides)
		free(mod_bitmaps_globals.overrides);
	if (mod_bitmaps_globals.scales)
		free(mod_bitmaps_globals.scales);
	/* (a Custom Edition map's are its own, which it forgets as it goes) */
	if (mod_bitmaps_globals.orders_found && !custom_edition_cache_tags_loaded())
		custom_edition_bitmaps_dispose();
	memset(&mod_bitmaps_globals, 0, sizeof(mod_bitmaps_globals));

	return;
}

boolean mod_bitmaps_contain(
	struct bitmap_data const *bitmap)
{
	return bitmap_of(bitmap) != NULL;
}

boolean mod_bitmaps_read(
	struct bitmap_data *bitmap,
	void *buffer)
{
	struct mod_bitmap *made = bitmap_of(bitmap);
	unsigned long size;
	unsigned long stamp[2];

	if (!made)
	{
		/* (the renderer keeps each address's channel order until it is told
		another: an Xbox map's bitmap where a mod's was has the Xbox's) */
		if (mod_bitmaps_globals.orders_found && !custom_edition_cache_tags_loaded())
			halo_custom_edition_texels_channels(buffer, _custom_edition_channels_xbox);
		return FALSE;
	}
	/* (a file changed since it was read is read again by mods_reload: until
	then its bitmaps are black) */
	if (!port_mods_file_stamp(made->file, &size, stamp) || size != made->file_size ||
		stamp[0] != made->stamp[0] || stamp[1] != made->stamp[1] ||
		!port_mods_file_read(made->file, made->pixel_data_offset + (unsigned long)bitmap->pixels_offset, buffer,
			(unsigned long)bitmap->pixels_size))
	{
		if (!mod_bitmaps_globals.read_failure_reported)
		{
			error(_error_silent, "mods: the pixels of %s cannot be read from %s (mods_reload reads it again)",
				tag_get_name(made->tag_index), made->path);
			mod_bitmaps_globals.read_failure_reported = TRUE;
		}
		memset(buffer, 0, (size_t)bitmap->pixels_size);
	}
	halo_custom_edition_texels_channels(
		buffer,
		!port_mods_file_xbox_channels(made->file) &&
		(mod_bitmaps_globals.orders_found || custom_edition_cache_tags_loaded()) ?
			(unsigned char)custom_edition_bitmap_channel_order(made->tag_index, (long)(bitmap - made->bitmaps)) :
			_custom_edition_channels_xbox);
	/* (on failure it logs, and the texture shows the pixels as read) */
	rasterizer_xbox_bitmap_rebuild_hardware_format(bitmap);

	return TRUE;
}
