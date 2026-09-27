/*
CUSTOM_EDITION_BITMAPS.C

The bitmaps of Halo Custom Edition maps, drawn by this build's texture cache
(custom_edition_cache.h).

A Custom Edition bitmap is described as an Xbox one is, and the texture
cache loads its pixels as it loads an Xbox bitmap's: the loader has made
their offsets count in the space custom_edition_cache_read serves. The
pixels are laid out for Halo PC, though: as the tag stores them, levels one
after another, unswizzled, each level of a cube map holding its six faces.
Xbox caches hold them as the hardware wants them, which this build's
rasterizer_xbox_bitmap_rebuild_hardware_format makes of the tag layout: every
level swizzled, cube maps face by face, linear rows padded to 64 bytes and
the whole padded to 128. Every Custom Edition bitmap's pixels go through it
as they arrive.
*/

/* ---------- headers */

#include "cseries.h"
#include "errors.h"
#include "tag_files/tag_groups.h"
#include "bitmaps/bitmaps.h"
#include "bitmaps/bitmaps_internal.h"
#include "bitmaps/bitmaps_mipmap.h"
#include "bitmaps/bitmap_group.h"
#include "rasterizer/rasterizer_swizzle.h"
#include "cache_file_formats.h"
#include "custom_edition_cache.h"

/* ---------- constants */

/* bitmaps.c's bitmap types, formats and flags */
enum
{
	_bitmap_type_2d,
	_bitmap_type_3d,
	_bitmap_type_cube_map,
};

enum
{
	_bitmap_format_a8,
	_bitmap_format_y8,
	_bitmap_format_ay8,
	_bitmap_format_a8y8,
	_bitmap_format_unused1,
	_bitmap_format_unused2,
	_bitmap_format_r5g6b5,
	_bitmap_format_unused3,
	_bitmap_format_a1r5g5b5,
	_bitmap_format_a4r4g4b4,
	_bitmap_format_x8r8g8b8,
	_bitmap_format_a8r8g8b8,
	_bitmap_format_unused4,
	_bitmap_format_unused5,
	_bitmap_format_dxt1,
	_bitmap_format_dxt3,
	_bitmap_format_dxt5,
	_bitmap_format_p8_bump,
};

enum
{
	_bitmap_has_power_of_two_dimensions_bit,
	_bitmap_compressed_bit,
	_bitmap_palettized_bit,
	_bitmap_swizzled_bit,
	_bitmap_linear_bit,
};

/* xbox_texture_cache.c makes linear textures with the row pitch in units
of this, rounded down */
#define LINEAR_TEXTURE_PITCH_ALIGNMENT 64

/* ---------- private code */

static boolean power_of_two(
	long value)
{
	return value > 0 && !(value & (value - 1));
}

/* Whether xbox_texture_cache.c has a hardware texture format for `format`
(its bitmap_d3d_format_tables): every format but the unused ones, and for
linear bitmaps only the uncompressed ones. */
static boolean bitmap_format_drawable(
	short format,
	boolean linear)
{
	switch (format)
	{
	case _bitmap_format_a8:
	case _bitmap_format_y8:
	case _bitmap_format_ay8:
	case _bitmap_format_a8y8:
	case _bitmap_format_r5g6b5:
	case _bitmap_format_a1r5g5b5:
	case _bitmap_format_a4r4g4b4:
	case _bitmap_format_x8r8g8b8:
	case _bitmap_format_a8r8g8b8:
		return TRUE;
	case _bitmap_format_dxt1:
	case _bitmap_format_dxt3:
	case _bitmap_format_dxt5:
	case _bitmap_format_p8_bump:
		return !linear;
	default:
		return FALSE;
	}
}

/* Whether the texture cache can draw `bitmap` from pixels laid out as its
tag lays them out, as rasterizer_xbox_bitmap_rebuild_hardware_format and
the texture cache assume without checking: the game's own bitmap_verify, a
format with a hardware texture, a compressed flag that says whether the
format is compressed, power-of-two dimensions for the swizzle (or else a
linear 2D bitmap), square cube maps, and every pixel its levels need. */
static boolean custom_edition_bitmap_drawable(
	struct bitmap_data *bitmap)
{
	boolean linear = TEST_FLAG(bitmap->flags, _bitmap_linear_bit);
	boolean compressed_format = bitmap->format >= _bitmap_format_dxt1 && bitmap->format <= _bitmap_format_dxt5;

	return bitmap_verify(bitmap, FALSE) &&
		bitmap_format_drawable(bitmap->format, linear) &&
		(TEST_FLAG(bitmap->flags, _bitmap_compressed_bit) != 0) == compressed_format &&
		(linear ?
			bitmap->type == _bitmap_type_2d :
			power_of_two(bitmap->width) && power_of_two(bitmap->height) && power_of_two(bitmap->depth)) &&
		(bitmap->type != _bitmap_type_cube_map || bitmap->width == bitmap->height) &&
		bitmap->pixels_size >= bitmap_get_pixel_data_size(bitmap);
}

/* ---------- public code */

boolean custom_edition_bitmaps_verify(
	byte *tag_cache,
	unsigned long loaded_bytes)
{
	struct bitmap_group *group;
	int32_t tag_index = NONE;
	long bitmap_count = 0;
	long misaligned_count = 0;

	while ((group = custom_edition_cache_tag_next(tag_cache, loaded_bytes, BITMAP_GROUP_TAG, sizeof(*group), &tag_index)) != NULL)
	{
		long bitmap_index;

		for (bitmap_index = 0; bitmap_index < group->bitmaps.count; bitmap_index++)
		{
			struct bitmap_data *bitmap = TAG_BLOCK_GET_ELEMENT(&group->bitmaps, bitmap_index, struct bitmap_data);

			if (!custom_edition_bitmap_drawable(bitmap))
			{
				error(
					_error_silent,
					"custom edition: bitmap %ld of '%s' (type %d, format %d, %dx%dx%d, flags 0x%X) cannot be drawn by this build",
					bitmap_index,
					custom_edition_cache_tag_name(tag_cache, loaded_bytes, tag_index),
					bitmap->type,
					bitmap->format,
					bitmap->width,
					bitmap->height,
					bitmap->depth,
					bitmap->flags);
				return FALSE;
			}
			if (TEST_FLAG(bitmap->flags, _bitmap_linear_bit) &&
				bitmap_mipmap_get_row_pitch(bitmap, 0) % LINEAR_TEXTURE_PITCH_ALIGNMENT)
			{
				/* rasterizer_xbox_bitmap_rebuild_hardware_format pads the rows,
				but the texture's header gets the unpadded pitch rounded down */
				error(
					_error_silent,
					"custom edition: bitmap %ld of '%s' is linear with %ld-byte rows, which this build draws with the wrong row pitch",
					bitmap_index,
					custom_edition_cache_tag_name(tag_cache, loaded_bytes, tag_index),
					bitmap_mipmap_get_row_pitch(bitmap, 0));
				misaligned_count++;
			}
			bitmap_count++;
		}
	}
	error(
		_error_silent,
		"custom edition: %ld bitmaps can be drawn (%ld with misaligned rows)",
		bitmap_count,
		misaligned_count);

	return TRUE;
}

void custom_edition_bitmap_pixels_arrived(
	byte *tag_cache,
	unsigned long loaded_bytes,
	long tag_index,
	long offset,
	void *pixels)
{
	struct bitmap_group *group = custom_edition_cache_tag_get(
		tag_cache,
		loaded_bytes,
		(unsigned long)tag_index,
		BITMAP_GROUP_TAG,
		sizeof(*group));
	long bitmap_index;

	if (!group)
	{
		return;
	}
	/* the texture cache sets the address its pixels go to before it reads
	them */
	for (bitmap_index = 0; bitmap_index < group->bitmaps.count; bitmap_index++)
	{
		struct bitmap_data *bitmap = TAG_BLOCK_GET_ELEMENT(&group->bitmaps, bitmap_index, struct bitmap_data);

		if (bitmap->base_address == pixels && bitmap->pixels_offset == offset)
		{
			/* on failure it logs, and the texture shows the pixels as read */
			rasterizer_xbox_bitmap_rebuild_hardware_format(bitmap);
			return;
		}
	}

	return;
}
