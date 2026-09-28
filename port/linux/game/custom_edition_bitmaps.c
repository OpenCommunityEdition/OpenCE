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

Halo PC keeps what some textures hold in other channels than this build
reads it from (enum custom_edition_channel_order, cache_file_formats.h;
docs/custom_edition_caches.md): a model shader's multipurpose masks, and
a HUD meter's shape and the order it fills in. Reordering them in the pixels
would mean decompressing them, which the texture cache has no room for, so
the renderer is told where Halo PC keeps the channels of each bitmap's
pixels as they arrive, and samples them in this build's order
(port/linux/src/xbox_textures.c).
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
#include "shaders/shader_definitions.h"
#include "interface/unit_hud_interface_definition.h"
#include "cache_file_formats.h"
#include "custom_edition_cache.h"

#include <stdlib.h>

/* ---------- constants */

enum
{
	SHADER_MODEL_GROUP_TAG = 'soso',
	WEAPON_HUD_INTERFACE_GROUP_TAG = 'wphi',
};

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

/* ---------- structures */

/* A model shader, to its reflection cube map: the bitmaps it draws with.
rasterizer_xbox_models.c defines this build's shader_model for itself (no
header declares it) and asserts where the indices of its base,
multipurpose and detail maps lie; Custom Edition lays it out the same
(OpenSauce shader_definitions.hpp, s_shader_model_definition), and in the
Custom Edition maps examined each of these references names a bitmap or
nothing. */
struct shader_model_maps
{
	struct shader shader;
	byte reserved28[0x7C];
	struct tag_reference base_map;
	byte reservedB4[0x8];
	struct tag_reference multipurpose_map;
	byte reservedCC[0x10];
	struct tag_reference detail_map;
	byte reservedEC[0x78];
	struct tag_reference reflection_cube_map;
};

typedef char verify_shader_model_maps_base_map_index_offset[
	offsetof(struct shader_model_maps, base_map.index) == 0xB0 ? 1 : -1];
typedef char verify_shader_model_maps_multipurpose_map_index_offset[
	offsetof(struct shader_model_maps, multipurpose_map.index) == 0xC8 ? 1 : -1];
typedef char verify_shader_model_maps_detail_map_index_offset[
	offsetof(struct shader_model_maps, detail_map.index) == 0xE8 ? 1 : -1];
typedef char verify_shader_model_maps_reflection_cube_map_offset[
	offsetof(struct shader_model_maps, reflection_cube_map) == 0x164 ? 1 : -1];

/* A weapon HUD interface, to its static and meter elements, as hud_weapon.c
defines it for itself (no header declares it; OpenSauce
weapon_hud_interface_definition.hpp agrees). */
struct weapon_hud_interface_elements
{
	byte reserved00[0x60];
	struct tag_block statics;
	struct tag_block meters;
};

struct weapon_hud_static_element
{
	byte header[0x24];
	struct static_hud_element_definition static_element;
	byte unused[0x28];
};

struct weapon_hud_meter_element
{
	byte header[0x24];
	struct meter_hud_element_definition meter_element;
	byte unused[0x28];
};

typedef char verify_weapon_hud_interface_elements_meters_offset[
	offsetof(struct weapon_hud_interface_elements, meters) == 0x6C ? 1 : -1];
typedef char verify_weapon_hud_static_element_size[
	sizeof(struct weapon_hud_static_element) == 0xB4 ? 1 : -1];
typedef char verify_weapon_hud_meter_element_size[
	sizeof(struct weapon_hud_meter_element) == 0xB4 ? 1 : -1];

/* a bitmap whose channels Halo PC keeps elsewhere than this build reads
them from */
struct reordered_bitmap
{
	long handle;
	/* an enum custom_edition_channel_order */
	byte channel_order;
	/* also drawn in another order, or as a texture whose channels are this
	build's: the renderer has one order for each texture, so this one keeps
	its channels as they are */
	boolean drawn_otherwise;
};

struct custom_edition_bitmaps_globals
{
	struct reordered_bitmap *reordered_bitmaps;
	long reordered_bitmap_count;
	long reordered_bitmap_capacity;
};

/* ---------- globals */

static struct custom_edition_bitmaps_globals custom_edition_bitmaps_globals;

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

/* where `handle` is in the list of reordered bitmaps, or NONE */
static long reordered_bitmap_index(
	long handle)
{
	struct custom_edition_bitmaps_globals *globals = &custom_edition_bitmaps_globals;
	long index;

	for (index = 0; index < globals->reordered_bitmap_count; index++)
	{
		if (globals->reordered_bitmaps[index].handle == handle)
		{
			return index;
		}
	}

	return NONE;
}

/* Lists the bitmap `handle` names, when it names one, as drawn in the
channel order `channel_order`; FALSE after logging why when there is no
memory for it. */
static boolean reordered_bitmap_add(
	byte *tag_cache,
	unsigned long loaded_bytes,
	long handle,
	byte channel_order)
{
	struct custom_edition_bitmaps_globals *globals = &custom_edition_bitmaps_globals;
	long index;
	struct reordered_bitmap *bitmap;

	if (!custom_edition_cache_tag_get(tag_cache, loaded_bytes, (unsigned long)handle, BITMAP_GROUP_TAG, sizeof(struct bitmap_group)))
	{
		return TRUE;
	}
	index = reordered_bitmap_index(handle);
	if (index != NONE)
	{
		if (globals->reordered_bitmaps[index].channel_order != channel_order)
		{
			globals->reordered_bitmaps[index].drawn_otherwise = TRUE;
		}
		return TRUE;
	}
	if (globals->reordered_bitmap_count == globals->reordered_bitmap_capacity)
	{
		long capacity = globals->reordered_bitmap_capacity ? 2 * globals->reordered_bitmap_capacity : 64;
		struct reordered_bitmap *bitmaps = realloc(globals->reordered_bitmaps, capacity * sizeof(*bitmaps));

		if (!bitmaps)
		{
			error(_error_silent, "custom edition: no memory to list %ld bitmaps whose channels are reordered", capacity);
			return FALSE;
		}
		globals->reordered_bitmaps = bitmaps;
		globals->reordered_bitmap_capacity = capacity;
	}
	bitmap = &globals->reordered_bitmaps[globals->reordered_bitmap_count++];
	bitmap->handle = handle;
	bitmap->channel_order = channel_order;
	bitmap->drawn_otherwise = FALSE;

	return TRUE;
}

/* the bitmap `handle` names is drawn as a texture whose channels are this
build's */
static void reordered_bitmap_drawn_otherwise(
	long handle)
{
	long index = reordered_bitmap_index(handle);

	if (index != NONE)
	{
		custom_edition_bitmaps_globals.reordered_bitmaps[index].drawn_otherwise = TRUE;
	}

	return;
}

/* the bitmaps of the HUD meters of a unit HUD interface */
static boolean unit_hud_meters_add(
	byte *tag_cache,
	unsigned long loaded_bytes,
	struct unit_hud_interface_definition *hud)
{
	boolean added =
		reordered_bitmap_add(tag_cache, loaded_bytes, hud->shield_meter.meter.meter_bitmap.index, _custom_edition_channels_hud_meter) &&
		reordered_bitmap_add(tag_cache, loaded_bytes, hud->health_meter.meter.meter_bitmap.index, _custom_edition_channels_hud_meter);
	long meter_index;

	for (meter_index = 0; added && meter_index < hud->auxilary_meters.count; meter_index++)
	{
		struct auxilary_meter_definition *meter = custom_edition_cache_block_element(
			tag_cache,
			loaded_bytes,
			&hud->auxilary_meters,
			meter_index,
			sizeof(*meter));

		/* none when the block does not lie within the tag cache */
		if (!meter)
		{
			break;
		}
		added = reordered_bitmap_add(tag_cache, loaded_bytes, meter->panel.meter.meter_bitmap.index, _custom_edition_channels_hud_meter);
	}

	return added;
}

/* the bitmaps a unit HUD interface draws as they are */
static void unit_hud_statics_note(
	byte *tag_cache,
	unsigned long loaded_bytes,
	struct unit_hud_interface_definition *hud)
{
	long element_index;

	reordered_bitmap_drawn_otherwise(hud->background.interface_bitmap.index);
	reordered_bitmap_drawn_otherwise(hud->shield_meter.background.interface_bitmap.index);
	reordered_bitmap_drawn_otherwise(hud->health_meter.background.interface_bitmap.index);
	reordered_bitmap_drawn_otherwise(hud->motion_sensor.background.interface_bitmap.index);
	reordered_bitmap_drawn_otherwise(hud->motion_sensor.foreground.interface_bitmap.index);
	for (element_index = 0; element_index < hud->auxilary_panel.auxilary_overlays.count; element_index++)
	{
		struct auxilary_overlay_definition *overlay = custom_edition_cache_block_element(
			tag_cache,
			loaded_bytes,
			&hud->auxilary_panel.auxilary_overlays,
			element_index,
			sizeof(*overlay));

		if (!overlay)
		{
			break;
		}
		reordered_bitmap_drawn_otherwise(overlay->static_element.interface_bitmap.index);
	}
	for (element_index = 0; element_index < hud->auxilary_meters.count; element_index++)
	{
		struct auxilary_meter_definition *meter = custom_edition_cache_block_element(
			tag_cache,
			loaded_bytes,
			&hud->auxilary_meters,
			element_index,
			sizeof(*meter));

		if (!meter)
		{
			break;
		}
		reordered_bitmap_drawn_otherwise(meter->panel.background.interface_bitmap.index);
	}

	return;
}

/* the bitmaps of the HUD meters of a weapon HUD interface */
static boolean weapon_hud_meters_add(
	byte *tag_cache,
	unsigned long loaded_bytes,
	struct weapon_hud_interface_elements *hud)
{
	boolean added = TRUE;
	long meter_index;

	for (meter_index = 0; added && meter_index < hud->meters.count; meter_index++)
	{
		struct weapon_hud_meter_element *meter = custom_edition_cache_block_element(
			tag_cache,
			loaded_bytes,
			&hud->meters,
			meter_index,
			sizeof(*meter));

		if (!meter)
		{
			break;
		}
		added = reordered_bitmap_add(tag_cache, loaded_bytes, meter->meter_element.meter_bitmap.index, _custom_edition_channels_hud_meter);
	}

	return added;
}

/* the bitmaps a weapon HUD interface draws as they are */
static void weapon_hud_statics_note(
	byte *tag_cache,
	unsigned long loaded_bytes,
	struct weapon_hud_interface_elements *hud)
{
	long static_index;

	for (static_index = 0; static_index < hud->statics.count; static_index++)
	{
		struct weapon_hud_static_element *element = custom_edition_cache_block_element(
			tag_cache,
			loaded_bytes,
			&hud->statics,
			static_index,
			sizeof(*element));

		if (!element)
		{
			break;
		}
		reordered_bitmap_drawn_otherwise(element->static_element.interface_bitmap.index);
	}

	return;
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

boolean custom_edition_reordered_bitmaps_find(
	byte *tag_cache,
	unsigned long loaded_bytes)
{
	struct custom_edition_bitmaps_globals *globals = &custom_edition_bitmaps_globals;
	struct shader_model_maps *shader;
	struct unit_hud_interface_definition *unit_hud;
	struct weapon_hud_interface_elements *weapon_hud;
	int32_t tag_index;
	long multipurpose_count = 0;
	long meter_count = 0;
	long kept_count = 0;
	long bitmap_index;

	/* the bitmaps drawn in Halo PC's orders: model shaders' multipurpose
	maps and HUD meters */
	tag_index = NONE;
	while ((shader = custom_edition_cache_tag_next(tag_cache, loaded_bytes, SHADER_MODEL_GROUP_TAG, sizeof(*shader), &tag_index)) != NULL)
	{
		if (!reordered_bitmap_add(tag_cache, loaded_bytes, shader->multipurpose_map.index, _custom_edition_channels_multipurpose))
		{
			return FALSE;
		}
	}
	tag_index = NONE;
	while ((unit_hud = custom_edition_cache_tag_next(tag_cache, loaded_bytes, UNIT_HUD_INTERFACE_DEFINITION_TAG, sizeof(*unit_hud), &tag_index)) != NULL)
	{
		if (!unit_hud_meters_add(tag_cache, loaded_bytes, unit_hud))
		{
			return FALSE;
		}
	}
	tag_index = NONE;
	while ((weapon_hud = custom_edition_cache_tag_next(tag_cache, loaded_bytes, WEAPON_HUD_INTERFACE_GROUP_TAG, sizeof(*weapon_hud), &tag_index)) != NULL)
	{
		if (!weapon_hud_meters_add(tag_cache, loaded_bytes, weapon_hud))
		{
			return FALSE;
		}
	}

	/* and those drawn as they are too: a model shader's other maps and a
	HUD's static elements */
	tag_index = NONE;
	while ((shader = custom_edition_cache_tag_next(tag_cache, loaded_bytes, SHADER_MODEL_GROUP_TAG, sizeof(*shader), &tag_index)) != NULL)
	{
		reordered_bitmap_drawn_otherwise(shader->base_map.index);
		reordered_bitmap_drawn_otherwise(shader->detail_map.index);
		reordered_bitmap_drawn_otherwise(shader->reflection_cube_map.index);
	}
	tag_index = NONE;
	while ((unit_hud = custom_edition_cache_tag_next(tag_cache, loaded_bytes, UNIT_HUD_INTERFACE_DEFINITION_TAG, sizeof(*unit_hud), &tag_index)) != NULL)
	{
		unit_hud_statics_note(tag_cache, loaded_bytes, unit_hud);
	}
	tag_index = NONE;
	while ((weapon_hud = custom_edition_cache_tag_next(tag_cache, loaded_bytes, WEAPON_HUD_INTERFACE_GROUP_TAG, sizeof(*weapon_hud), &tag_index)) != NULL)
	{
		weapon_hud_statics_note(tag_cache, loaded_bytes, weapon_hud);
	}

	/* which keep their channels as they are */
	bitmap_index = 0;
	while (bitmap_index < globals->reordered_bitmap_count)
	{
		struct reordered_bitmap *bitmap = &globals->reordered_bitmaps[bitmap_index];

		if (bitmap->drawn_otherwise)
		{
			error(
				_error_silent,
				"custom edition: '%s' is drawn with Halo PC's %s channels and otherwise too, and keeps its channels as they are",
				custom_edition_cache_tag_name(tag_cache, loaded_bytes, DATUM_INDEX_TO_ABSOLUTE_INDEX(bitmap->handle)),
				bitmap->channel_order == _custom_edition_channels_multipurpose ? "multipurpose map" : "HUD meter");
			*bitmap = globals->reordered_bitmaps[--globals->reordered_bitmap_count];
			kept_count++;
		}
		else
		{
			if (bitmap->channel_order == _custom_edition_channels_multipurpose)
			{
				multipurpose_count++;
			}
			else
			{
				meter_count++;
			}
			bitmap_index++;
		}
	}
	error(
		_error_silent,
		"custom edition: %ld multipurpose maps and %ld HUD meters are drawn with their channels in this build's order (%ld drawn otherwise too are not)",
		multipurpose_count,
		meter_count,
		kept_count);

	return TRUE;
}

void custom_edition_bitmaps_dispose(
	void)
{
	struct custom_edition_bitmaps_globals *globals = &custom_edition_bitmaps_globals;

	/* the game's free stops on NULL (cseries.h), and a map can fail before
	its bitmaps are listed */
	if (globals->reordered_bitmaps)
	{
		free(globals->reordered_bitmaps);
	}
	globals->reordered_bitmaps = NULL;
	globals->reordered_bitmap_count = 0;
	globals->reordered_bitmap_capacity = 0;
	/* the pixels it was told of are the texture cache's to reuse */
	halo_custom_edition_texels_forget();

	return;
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
			long reordered_index = reordered_bitmap_index(tag_index);

			/* the texture cache reuses its memory for other bitmaps, so the
			renderer is told of every bitmap arriving */
			halo_custom_edition_texels_channels(
				pixels,
				reordered_index != NONE ?
					custom_edition_bitmaps_globals.reordered_bitmaps[reordered_index].channel_order :
					_custom_edition_channels_xbox);
			/* on failure it logs, and the texture shows the pixels as read */
			rasterizer_xbox_bitmap_rebuild_hardware_format(bitmap);
			return;
		}
	}

	return;
}
