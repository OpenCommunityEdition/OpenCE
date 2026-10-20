/*
MOD_UNIT_HUDS.C

The unit HUDs of mods (mods.c): each of a map's unit HUD interfaces (the
HUD of a unit: the shield and health meters, the motion sensor, the
auxiliary meters and overlays, the warning sounds) that a mod has a tag file
of (tags\...\name.unit_hud_interface) is drawn from that file instead of the
map's, and mods_reload reads it again.

A tag file is Halo PC's (big-endian, version 1), as the Halo Editing Kit and
Invader write it: the interface, then, in the order of its fields, the
elements of each block (and, for each element, what it holds in turn), and
after each tag reference the name of the tag it names. Every count and size
is checked against the file, which must end where its last name does; every
real must be a number; every value the HUD indexes with (corners, blend
functions, effector sources) must be one it draws.

A HUD is only what a player sees and hears on this machine, and a mod cannot
add tags, so every bitmap and sound the file names is a tag the map has: a
name the map has not got leaves the file out, and the map's HUD is drawn. The
placements are the file's as they are, drawn as the map's own are (the
Xbox's, and a Custom Edition map's converted ones): a mod's bitmaps of a
larger size scale them (mod_bitmaps.c).

The map's interface takes the file's values in place, so everything that
holds the tag keeps holding it, and its own come back when the file goes. A
HUD made is kept until the map goes, as the HUD's placements may have been
scaled for the mods' bitmaps (mod_bitmaps_hud_rescale) and remembered where.
*/

/* ---------- headers */

#include "cseries.h"
#include "errors.h"
#include "cache/cache_files.h"
#include "interface/hud_definitions.h"
#include "interface/unit_hud_interface_definition.h"
#include "main/console.h"
#include "tag_files/tag_groups.h"
#include "mods.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* the game's (port) */
char *tag_get_name(long tag_index);
unsigned long tag_get_group_tag(long tag_index);

/* ---------- constants */

/* a unit HUD interface tag's file: tags\<its name>.unit_hud_interface */
#define TAG_FILE_EXTENSION ".unit_hud_interface"
#define UNIT_HUD_INTERFACE_TAG_VERSION 1

/* the sizes of a tag file's tag reference and tag block, which are as
this build's */
#define TAG_REFERENCE_SIZE 0x10
#define TAG_BLOCK_SIZE 0x0C

/* the most of a tag reference's name (the tools' 255) */
#define MAXIMUM_NAME_LENGTH 255

/* the editing kit's block limits, and a little over */
#define MAXIMUM_WARNING_SOUNDS 16
#define MAXIMUM_AUXILARY_OVERLAYS 32
#define MAXIMUM_AUXILARY_METERS 32
#define MAXIMUM_MULTITEXTURE_OVERLAYS 64
#define MAXIMUM_OVERLAY_EFFECTORS 64

/* the sizes of the structures a tag file has that this file lays out */
#define HUD_SOUND_SIZE 0x38
#define MULTITEXTURE_OVERLAY_SIZE 0x1E0
#define OVERLAY_EFFECTOR_SIZE 0xDC

/* hud_draw.c's enums, which its drawing indexes by */
enum
{
	NUMBER_OF_FRAMEBUFFER_BLEND_FUNCTIONS = 8,
	NUMBER_OF_MAP_BLEND_FUNCTIONS = 5,
	NUMBER_OF_EFFECTOR_TYPES = 4,
	_effector_type_alpha = 3,
	NUMBER_OF_EFFECTOR_DESTINATIONS = 4,
	NUMBER_OF_EFFECTOR_SOURCES = 8,
	/* (the type of an auxilary overlay is a bit of a word of flags) */
	NUMBER_OF_AUXILARY_OVERLAY_TYPES = 16,
};

/* ---------- structures */

/* the multitexture overlay and its effector of a HUD element, as hud_draw.c
has them (it has them to itself) */
struct overlay_effector
{
	long unused0[16];
	short destination_type;
	short destination;
	short source;
	word pad46;
	real in_bounds[2];
	real out_bounds[2];
	long unused58[16];
	real_rgb_color tint_color_lower_bounds;
	real_rgb_color tint_color_upper_bounds;
	short periodic_function;
	word padB2;
	real periodic_function_period;
	real periodic_function_phase;
	long unusedBC[8];
};

struct multitexture_overlay
{
	word flags;
	short type;
	short framebuffer_blend_function;
	word pad06;
	long unused08[8];
	word map_flags[3];
	short map_blending_function[2];
	short pad32;
	real_vector2d map_scale[3];
	real_vector2d map_offset[3];
	struct tag_reference map[3];
	short map_clamp[3];
	short pad9A;
	long unused9C[46];
	struct tag_block functions;
	long unused160[32];
};

/* hud_sounds.c's (it has it to itself) */
struct warning_sound
{
	struct tag_reference sound;
	unsigned long state_flags;
	real scale;
	byte reserved[32];
};

typedef char verify_long_size[sizeof(unsigned long) == 4 ? 1 : -1];
typedef char verify_tag_reference_size[sizeof(struct tag_reference) == TAG_REFERENCE_SIZE ? 1 : -1];
typedef char verify_tag_block_size[sizeof(struct tag_block) == TAG_BLOCK_SIZE ? 1 : -1];
typedef char verify_overlay_effector_size[sizeof(struct overlay_effector) == OVERLAY_EFFECTOR_SIZE ? 1 : -1];
typedef char verify_multitexture_overlay_size[sizeof(struct multitexture_overlay) == MULTITEXTURE_OVERLAY_SIZE ? 1 : -1];
typedef char verify_warning_sound_size[sizeof(struct warning_sound) == HUD_SOUND_SIZE ? 1 : -1];

/* ---------- layouts */

/* What a tag file's structure holds, in the order of its fields: the tag
file is big-endian, so each number is swapped, and each tag reference and
block is followed by what the file keeps of it. A field not listed (a byte, a
run of bytes) is kept as it is. */
enum field_kind
{
	_field_u16,
	_field_u32,
	_field_real,
	_field_reference,
	_field_block,
	_field_struct,
	_field_end,
};

struct layout;

struct layout_field
{
	short kind;
	unsigned short offset;
	/* how many there are, one after another */
	unsigned short count;
	/* a block's most elements */
	short maximum;
	/* a block's element, or a structure inline */
	struct layout const *child;
	/* a reference's groups (0 ends them) */
	unsigned long const *groups;
};

struct layout
{
	unsigned short size;
	struct layout_field const *fields;
};

#define U16(offset, count) { _field_u16, offset, count, 0, NULL, NULL }
#define U32(offset, count) { _field_u32, offset, count, 0, NULL, NULL }
#define REAL(offset, count) { _field_real, offset, count, 0, NULL, NULL }
#define REFERENCE(offset, count, groups) { _field_reference, offset, count, 0, NULL, groups }
#define BLOCK(offset, child, maximum) { _field_block, offset, 1, maximum, &child, NULL }
#define STRUCTURE(offset, child) { _field_struct, offset, 1, 0, &child, NULL }
#define END { _field_end, 0, 0, 0, NULL, NULL }

/* (the groups a HUD draws and plays: hud_sounds.c plays a sound or a looping
sound and asserts for any other) */
static unsigned long const bitmap_groups[] = { 'bitm', 0 };
static unsigned long const sound_groups[] = { 'snd!', 'lsnd', 0 };

/* (hud_definitions.h's structures) */
static struct layout_field const absolute_placement_fields[] =
{
	U16(0x00, 2), U32(0x04, 8), END
};
static struct layout const absolute_placement_layout = { 0x24, absolute_placement_fields };

static struct layout_field const placement_fields[] =
{
	U16(0x00, 2), REAL(0x04, 2), U16(0x0C, 2), U32(0x10, 5), END
};
static struct layout const placement_layout = { 0x24, placement_fields };

static struct layout_field const color_fields[] =
{
	U32(0x00, 2), REAL(0x08, 2), U16(0x10, 2), REAL(0x14, 1), U32(0x18, 2), END
};
static struct layout const color_layout = { 0x20, color_fields };

/* (a multitexture overlay's effector) */
static struct layout_field const effector_fields[] =
{
	U32(0x00, 16), U16(0x40, 4), REAL(0x48, 4), U32(0x58, 16), REAL(0x98, 6), U16(0xB0, 2), REAL(0xB4, 2),
	U32(0xBC, 8), END
};
static struct layout const effector_layout = { OVERLAY_EFFECTOR_SIZE, effector_fields };

static struct layout_field const overlay_fields[] =
{
	U16(0x00, 4), U32(0x08, 8), U16(0x28, 6), REAL(0x34, 12), REFERENCE(0x64, 3, bitmap_groups), U16(0x94, 4),
	U32(0x9C, 46), BLOCK(0x154, effector_layout, MAXIMUM_OVERLAY_EFFECTORS), U32(0x160, 32), END
};
static struct layout const overlay_layout = { MULTITEXTURE_OVERLAY_SIZE, overlay_fields };

/* (unit_hud_interface_definition.h's structures) */
static struct layout_field const static_element_fields[] =
{
	STRUCTURE(0x00, placement_layout), REFERENCE(0x24, 1, bitmap_groups), STRUCTURE(0x34, color_layout), U16(0x54, 2),
	BLOCK(0x58, overlay_layout, MAXIMUM_MULTITEXTURE_OVERLAYS), U32(0x64, 1), END
};
static struct layout const static_element_layout = { 0x68, static_element_fields };

static struct layout_field const meter_element_fields[] =
{
	STRUCTURE(0x00, placement_layout), REFERENCE(0x24, 1, bitmap_groups), U32(0x34, 4), U16(0x46, 1), U16(0x4A, 1),
	REAL(0x4C, 2), U32(0x54, 1), BLOCK(0x58, overlay_layout, MAXIMUM_MULTITEXTURE_OVERLAYS), U32(0x64, 1), END
};
static struct layout const meter_element_layout = { 0x68, meter_element_fields };

static struct layout_field const metered_panel_fields[] =
{
	STRUCTURE(0x00, static_element_layout), STRUCTURE(0x68, meter_element_layout), U32(0xD0, 8), END
};
static struct layout const metered_panel_layout = { 0xF0, metered_panel_fields };

static struct layout_field const motion_sensor_fields[] =
{
	STRUCTURE(0x00, static_element_layout), STRUCTURE(0x68, static_element_layout), U32(0xD0, 8), END
};
static struct layout const motion_sensor_layout = { 0xF0, motion_sensor_fields };

static struct layout_field const auxilary_overlay_fields[] =
{
	STRUCTURE(0x00, static_element_layout), U16(0x68, 2), U32(0x6C, 6), END
};
static struct layout const auxilary_overlay_layout = { 0x84, auxilary_overlay_fields };

static struct layout_field const auxilary_panel_fields[] =
{
	STRUCTURE(0x00, absolute_placement_layout), BLOCK(0x24, auxilary_overlay_layout, MAXIMUM_AUXILARY_OVERLAYS),
	U32(0x30, 4), END
};
static struct layout const auxilary_panel_layout = { 0x40, auxilary_panel_fields };

static struct layout_field const auxilary_meter_fields[] =
{
	U16(0x00, 2), U32(0x04, 4), STRUCTURE(0x14, metered_panel_layout), U32(0x104, 16), END
};
static struct layout const auxilary_meter_layout = { 0x144, auxilary_meter_fields };

static struct layout_field const warning_sound_fields[] =
{
	REFERENCE(0x00, 1, sound_groups), U32(0x10, 1), REAL(0x14, 1), END
};
static struct layout const warning_sound_layout = { HUD_SOUND_SIZE, warning_sound_fields };

static struct layout_field const unit_hud_fields[] =
{
	STRUCTURE(0x000, absolute_placement_layout), STRUCTURE(0x024, static_element_layout),
	STRUCTURE(0x08C, metered_panel_layout), STRUCTURE(0x17C, metered_panel_layout),
	STRUCTURE(0x26C, motion_sensor_layout), STRUCTURE(0x35C, placement_layout),
	STRUCTURE(0x380, auxilary_panel_layout),
	BLOCK(0x3C0, warning_sound_layout, MAXIMUM_WARNING_SOUNDS),
	BLOCK(0x3CC, auxilary_meter_layout, MAXIMUM_AUXILARY_METERS), U32(0x3D8, 101), END
};
static struct layout const unit_hud_layout = { 0x56C, unit_hud_fields };

typedef char verify_unit_hud_size[sizeof(struct unit_hud_interface_definition) == 0x56C ? 1 : -1];

/* a block of elements the file made for the tag, in a chain to free */
struct chunk
{
	struct chunk *next;
	double alignment;
};

/* a HUD made from a tag file, and the map's it plays over */
struct mod_unit_hud
{
	struct mod_unit_hud *next;
	long tag_index;
	/* the file, kept to tell whether it changed */
	byte *file;
	unsigned long file_size;
	struct unit_hud_interface_definition definition;
	struct chunk *chunks;
	unsigned long kept_bytes;
};

/* a map's unit HUD, and the one shown over it (none: its own) */
struct mod_unit_hud_override
{
	long tag_index;
	struct mod_unit_hud *hud;
	struct unit_hud_interface_definition map_definition;
};

/* a file being made into a HUD */
struct build
{
	byte const *file;
	unsigned long size;
	unsigned long cursor;
	struct chunk *chunks;
	unsigned long chunk_bytes;
	char const *reason;
	char reason_text[320];
};

/* ---------- globals */

static struct
{
	struct mod_unit_hud_override *overrides;
	long override_count;
	/* every HUD made while the map is loaded */
	struct mod_unit_hud *huds;
} mod_unit_huds_globals;

static char empty_name[] = "";
/* why the last file was left out, kept for its caller */
static char refusal_text[320];

/* ---------- private code */

static void chunks_free(
	struct chunk *chunk)
{
	while (chunk)
	{
		struct chunk *next = chunk->next;

		free(chunk);
		chunk = next;
	}

	return;
}

static void mod_unit_hud_free(
	struct mod_unit_hud *hud)
{
	chunks_free(hud->chunks);
	/* (the game's free, debug_free, does not take NULL) */
	if (hud->file)
		free(hud->file);
	free(hud);

	return;
}

/* zeroed memory for `count` elements of `size` bytes, kept in the build's
chain; NULL when there is none */
static void *build_allocate(
	struct build *build,
	unsigned long count,
	unsigned long size)
{
	struct chunk *chunk = malloc(sizeof(*chunk) + (size_t)count * size);

	if (!chunk)
	{
		build->reason = "out of memory";
		return NULL;
	}
	memset(chunk, 0, sizeof(*chunk) + (size_t)count * size);
	chunk->next = build->chunks;
	build->chunks = chunk;
	build->chunk_bytes += (unsigned long)sizeof(*chunk) + count * size;

	return chunk + 1;
}

static void write_u16(
	byte *destination,
	unsigned long value)
{
	unsigned short native = (unsigned short)value;

	memcpy(destination, &native, sizeof(native));

	return;
}

static void write_u32(
	byte *destination,
	unsigned long value)
{
	memcpy(destination, &value, 4);

	return;
}

/* The numbers of a structure of the file at `source`, made this machine's
at `destination`, where the structure's bytes have been copied: FALSE for a
real that is not a number. */
static boolean fields_swap(
	struct build *build,
	struct layout const *layout,
	byte *destination,
	byte const *source)
{
	struct layout_field const *field;

	for (field = layout->fields; field->kind != _field_end; field++)
	{
		unsigned long index;

		for (index = 0; index < field->count; index++)
		{
			unsigned long offset;

			switch (field->kind)
			{
			case _field_u16:
				offset = field->offset + 2 * index;
				write_u16(destination + offset, mods_read_be16(source + offset));
				break;

			case _field_u32:
				offset = field->offset + 4 * index;
				write_u32(destination + offset, mods_read_be32(source + offset));
				break;

			case _field_real:
				offset = field->offset + 4 * index;
				if (!mods_finite_real(mods_read_real(source + offset)))
				{
					build->reason = "it has a real that is not a number";
					return FALSE;
				}
				write_u32(destination + offset, mods_read_be32(source + offset));
				break;

			case _field_reference:
				{
					struct tag_reference *reference =
						(struct tag_reference *)(destination + field->offset + index * TAG_REFERENCE_SIZE);
					byte const *file = source + field->offset + index * TAG_REFERENCE_SIZE;

					reference->group_tag = mods_read_be32(file);
					reference->name = NULL;
					reference->name_length = (long)mods_read_be32(file + 8);
					reference->index = NONE;
				}
				break;

			case _field_block:
				{
					struct tag_block *block = (struct tag_block *)(destination + field->offset);

					block->count = (long)mods_read_be32(source + field->offset);
					block->address = NULL;
					block->definition = NULL;
				}
				break;

			case _field_struct:
				if (!fields_swap(build, field->child, destination + field->offset, source + field->offset))
					return FALSE;
				break;
			}
		}
	}

	return TRUE;
}

/* the map's tag a reference of the file names (the groups of the field,
the file's own first) */
static boolean reference_resolve(
	struct build *build,
	struct layout_field const *field,
	struct tag_reference *reference)
{
	long length = reference->name_length;
	char const *name;
	unsigned long file_group = reference->group_tag;
	long tag_index = NONE;
	long group;

	if (length < 0 || length > MAXIMUM_NAME_LENGTH)
	{
		build->reason = "a tag reference has a name that is too long";
		return FALSE;
	}
	if (!length)
	{
		/* (none) */
		reference->group_tag = (unsigned long)NONE;
		reference->name = empty_name;
		reference->name_length = 0;
		reference->index = NONE;
		return TRUE;
	}
	name = (char const *)build->file + build->cursor;
	if (!mods_in_file(build->cursor, (unsigned long)length + 1, build->size) || name[length] != 0 ||
		memchr(name, 0, (size_t)length))
	{
		build->reason = "the name of a tag reference is not in the file";
		return FALSE;
	}
	build->cursor += (unsigned long)length + 1;
	for (group = 0; tag_index == NONE && field->groups[group]; group++)
	{
		if (field->groups[group] == file_group)
			tag_index = tag_loaded((long)file_group, name);
	}
	for (group = 0; tag_index == NONE && field->groups[group]; group++)
	{
		tag_index = tag_loaded((long)field->groups[group], name);
	}
	if (tag_index == NONE)
	{
		snprintf(build->reason_text, sizeof(build->reason_text),
			"it names %s, which the map has not got (a mod cannot add tags)", name);
		build->reason = build->reason_text;
		return FALSE;
	}
	reference->group_tag = tag_get_group_tag(tag_index);
	reference->index = tag_index;
	reference->name = tag_get_name(tag_index);
	reference->name_length = (long)strlen(reference->name);

	return TRUE;
}

/* What follows a structure in the file, which holds it at `destination`
(made this machine's): the names of its references, and the elements of its
blocks and what follows each. */
static boolean fields_walk(
	struct build *build,
	struct layout const *layout,
	byte *destination)
{
	struct layout_field const *field;

	for (field = layout->fields; field->kind != _field_end; field++)
	{
		unsigned long index;

		if (field->kind == _field_struct)
		{
			if (!fields_walk(build, field->child, destination + field->offset))
				return FALSE;
		}
		else if (field->kind == _field_reference)
		{
			for (index = 0; index < field->count; index++)
			{
				if (!reference_resolve(build, field,
						(struct tag_reference *)(destination + field->offset + index * TAG_REFERENCE_SIZE)))
					return FALSE;
			}
		}
		else if (field->kind == _field_block)
		{
			struct tag_block *block = (struct tag_block *)(destination + field->offset);
			unsigned long element_size = field->child->size;
			byte *elements;
			long element_index;

			if (block->count < 0 || block->count > field->maximum)
			{
				build->reason = "a block has more elements than the HUD has room for";
				return FALSE;
			}
			if (!block->count)
				continue;
			if (!mods_in_file(build->cursor, (unsigned long)block->count * element_size, build->size))
			{
				build->reason = "the elements of a block are not in the file";
				return FALSE;
			}
			elements = build_allocate(build, (unsigned long)block->count, element_size);
			if (!elements)
				return FALSE;
			for (element_index = 0; element_index < block->count; element_index++)
			{
				byte *element = elements + (unsigned long)element_index * element_size;
				byte const *source = build->file + build->cursor + (unsigned long)element_index * element_size;

				memcpy(element, source, element_size);
				if (!fields_swap(build, field->child, element, source))
					return FALSE;
			}
			build->cursor += (unsigned long)block->count * element_size;
			block->address = elements;
			for (element_index = 0; element_index < block->count; element_index++)
			{
				if (!fields_walk(build, field->child, elements + (unsigned long)element_index * element_size))
					return FALSE;
			}
		}
	}

	return TRUE;
}

/* ---------- checks */

static boolean overlays_check(
	struct build *build,
	struct tag_block const *overlays)
{
	long overlay_index;

	for (overlay_index = 0; overlay_index < overlays->count; overlay_index++)
	{
		struct multitexture_overlay const *overlay = (struct multitexture_overlay const *)overlays->address + overlay_index;
		short map_index;
		long function_index;

		if (overlay->framebuffer_blend_function < 0 ||
			overlay->framebuffer_blend_function >= NUMBER_OF_FRAMEBUFFER_BLEND_FUNCTIONS)
		{
			build->reason = "a multitexture overlay has a blend function the HUD does not draw";
			return FALSE;
		}
		for (map_index = 0; map_index < 2; map_index++)
		{
			if (overlay->map_blending_function[map_index] < 0 ||
				overlay->map_blending_function[map_index] >= NUMBER_OF_MAP_BLEND_FUNCTIONS)
			{
				build->reason = "a multitexture overlay has a map blend function the HUD does not draw";
				return FALSE;
			}
		}
		for (function_index = 0; function_index < overlay->functions.count; function_index++)
		{
			struct overlay_effector const *effector =
				(struct overlay_effector const *)overlay->functions.address + function_index;

			if (effector->destination_type < 0 || effector->destination_type >= NUMBER_OF_EFFECTOR_TYPES ||
				effector->destination < 0 || effector->destination >= NUMBER_OF_EFFECTOR_DESTINATIONS ||
				effector->source < 0 || effector->source >= NUMBER_OF_EFFECTOR_SOURCES)
			{
				build->reason = "a multitexture overlay's effector has a type, destination or source the HUD does not draw";
				return FALSE;
			}
			/* (the HUD asserts an alpha is a fraction) */
			if (effector->destination_type == _effector_type_alpha &&
				(effector->out_bounds[0] < 0.f || effector->out_bounds[0] > 1.f ||
				effector->out_bounds[1] < 0.f || effector->out_bounds[1] > 1.f))
			{
				build->reason = "a multitexture overlay's effector sets an alpha that is not from 0 to 1";
				return FALSE;
			}
		}
	}

	return TRUE;
}

static boolean static_element_check(
	struct build *build,
	struct static_hud_element_definition const *element)
{
	return overlays_check(build, &element->multitexture_overlays);
}

static boolean metered_panel_check(
	struct build *build,
	struct metered_panel_definition const *panel)
{
	return static_element_check(build, &panel->background) && overlays_check(build, &panel->meter.multitexture_overlays);
}

static boolean corner_check(
	struct build *build,
	struct hud_absolute_placement_definition const *placement)
{
	if (placement->corner < 0 || placement->corner >= NUMBER_OF_HUD_ANCHORS)
	{
		build->reason = "an anchor is not one of the HUD's corners or its center";
		return FALSE;
	}

	return TRUE;
}

/* the values the HUD indexes by, or draws asserting */
static boolean unit_hud_check(
	struct build *build,
	struct unit_hud_interface_definition const *hud)
{
	long index;

	if (!corner_check(build, &hud->absolute_placement) ||
		!corner_check(build, &hud->auxilary_panel.absolute_placement) ||
		!static_element_check(build, &hud->background) ||
		!metered_panel_check(build, &hud->shield_meter) ||
		!metered_panel_check(build, &hud->health_meter) ||
		!static_element_check(build, &hud->motion_sensor.background) ||
		!static_element_check(build, &hud->motion_sensor.foreground))
	{
		return FALSE;
	}
	for (index = 0; index < hud->auxilary_panel.auxilary_overlays.count; index++)
	{
		struct auxilary_overlay_definition const *overlay =
			(struct auxilary_overlay_definition const *)hud->auxilary_panel.auxilary_overlays.address + index;

		/* (the type is a bit of a word of flags) */
		if (overlay->type < 0 || overlay->type >= NUMBER_OF_AUXILARY_OVERLAY_TYPES)
		{
			build->reason = "an auxilary overlay has a type the HUD does not draw";
			return FALSE;
		}
		if (!static_element_check(build, &overlay->static_element))
			return FALSE;
	}
	for (index = 0; index < hud->auxilary_meters.count; index++)
	{
		struct auxilary_meter_definition const *meter =
			(struct auxilary_meter_definition const *)hud->auxilary_meters.address + index;

		if (!metered_panel_check(build, &meter->panel))
			return FALSE;
	}

	return TRUE;
}

/* ---------- making a HUD */

/* The HUD the tag file `file` (`file_size` bytes) makes for the map's tag
`tag_index`, or NULL with *reason saying why not. It keeps `file`. */
static struct mod_unit_hud *mod_unit_hud_new(
	long tag_index,
	byte *file,
	unsigned long file_size,
	char const **reason)
{
	struct mod_unit_hud *hud;
	struct build build;
	unsigned long root = MOD_TAG_FILE_HEADER_SIZE;

	*reason = "not a unit HUD interface tag file of this version";
	if (file_size < root + unit_hud_layout.size ||
		!mods_tag_file_header_valid(file, UNIT_HUD_INTERFACE_DEFINITION_TAG, UNIT_HUD_INTERFACE_TAG_VERSION))
	{
		return NULL;
	}
	hud = malloc(sizeof(*hud));
	if (!hud)
	{
		*reason = "out of memory";
		return NULL;
	}
	memset(hud, 0, sizeof(*hud));
	hud->tag_index = tag_index;
	memset(&build, 0, sizeof(build));
	build.file = file;
	build.size = file_size;
	build.cursor = root + unit_hud_layout.size;
	build.reason = "it does not fit what the HUD draws";

	memcpy(&hud->definition, file + root, unit_hud_layout.size);
	if (!fields_swap(&build, &unit_hud_layout, (byte *)&hud->definition, file + root) ||
		!fields_walk(&build, &unit_hud_layout, (byte *)&hud->definition))
	{
		goto refused;
	}
	if (build.cursor != file_size)
	{
		build.reason = "the file goes on past its last name";
		goto refused;
	}
	if (!unit_hud_check(&build, &hud->definition))
	{
		goto refused;
	}
	hud->chunks = build.chunks;
	hud->file = file;
	hud->file_size = file_size;
	hud->kept_bytes = (unsigned long)sizeof(*hud) + file_size + build.chunk_bytes;

	return hud;

refused:
	/* (the reason may be in the build, which is gone) */
	strncpy(refusal_text, build.reason, sizeof(refusal_text) - 1);
	refusal_text[sizeof(refusal_text) - 1] = 0;
	*reason = refusal_text;
	chunks_free(build.chunks);
	free(hud);

	return NULL;
}

/* a HUD made before for the tag `tag_index` from a file of these bytes,
shown or not, or NULL */
static struct mod_unit_hud *hud_kept(
	long tag_index,
	byte const *file,
	unsigned long file_size)
{
	struct mod_unit_hud *hud;

	for (hud = mod_unit_huds_globals.huds; hud; hud = hud->next)
	{
		if (hud->tag_index == tag_index && hud->file_size == file_size && !memcmp(hud->file, file, file_size))
			return hud;
	}

	return NULL;
}

/* Makes the table of overrides, one for each of the map's unit HUDs; FALSE
when it cannot be. */
static boolean overrides_new(
	void)
{
	struct tag_iterator iterator;
	long tag_index;
	long count = 0;

	if (mod_unit_huds_globals.overrides)
	{
		return TRUE;
	}
	tag_iterator_new(&iterator, UNIT_HUD_INTERFACE_DEFINITION_TAG);
	while ((tag_index = tag_iterator_next(&iterator)) != NONE)
	{
		if (tag_get_group_tag(tag_index) == UNIT_HUD_INTERFACE_DEFINITION_TAG)
			count++;
	}
	if (count <= 0 ||
		!(mod_unit_huds_globals.overrides = malloc((size_t)count * sizeof(struct mod_unit_hud_override))))
	{
		return FALSE;
	}
	memset(mod_unit_huds_globals.overrides, 0, (size_t)count * sizeof(struct mod_unit_hud_override));
	count = 0;
	tag_iterator_new(&iterator, UNIT_HUD_INTERFACE_DEFINITION_TAG);
	while ((tag_index = tag_iterator_next(&iterator)) != NONE)
	{
		if (tag_get_group_tag(tag_index) == UNIT_HUD_INTERFACE_DEFINITION_TAG)
			mod_unit_huds_globals.overrides[count++].tag_index = tag_index;
	}
	mod_unit_huds_globals.override_count = count;

	return TRUE;
}

/* ---------- public code */

long mod_unit_huds_update(
	boolean remove_all,
	boolean at_load,
	boolean *changed)
{
	long loaded_count = 0;
	long removed_count = 0;
	long refused_count = 0;
	long played_count = 0;
	long index;

	*changed = FALSE;
	if (!mods_map_loaded() || !overrides_new())
	{
		return 0;
	}

	for (index = 0; index < mod_unit_huds_globals.override_count; index++)
	{
		struct mod_unit_hud_override *override = &mod_unit_huds_globals.overrides[index];
		long tag_index = override->tag_index;
		struct unit_hud_interface_definition *definition =
			(struct unit_hud_interface_definition *)tag_get(UNIT_HUD_INTERFACE_DEFINITION_TAG, tag_index);
		long file_index = remove_all ? NONE : mods_tag_file_find(tag_index, TAG_FILE_EXTENSION);
		struct mod_unit_hud *made = NULL;
		byte *file = NULL;
		unsigned long file_size = 0;
		char const *reason = NULL;

		if (!definition)
		{
			continue;
		}
		if (file_index != NONE)
		{
			file = mods_tag_file_read(file_index, &file_size, &reason);
			/* (one made before from the same file, shown or not) */
			made = file ? hud_kept(tag_index, file, file_size) : NULL;
			if (made)
			{
				free(file);
				if (made == override->hud)
					continue;
			}
			else if (file)
			{
				made = mod_unit_hud_new(tag_index, file, file_size, &reason);
				if (made && !mods_memory_take(made->kept_bytes))
				{
					reason = "the mods' tags already hold too much memory";
					made->file = NULL;
					mod_unit_hud_free(made);
					made = NULL;
				}
				if (!made)
				{
					free(file);
				}
			}
			if (!made)
			{
				error(_error_silent, "mods: %s left out: %s", port_mods_file_path(file_index), reason);
				console_warning("mods: %s%s of %s left out: %s", tag_get_name(tag_index), TAG_FILE_EXTENSION,
					port_mods_file_mod(file_index), reason);
				refused_count++;
				/* (one refused leaves what was shown before) */
				continue;
			}
		}
		else if (!override->hud)
		{
			continue;
		}

		if (override->hud)
		{
			memcpy(definition, &override->map_definition, sizeof(*definition));
			override->hud = NULL;
			if (!made)
				removed_count++;
		}
		if (made)
		{
			memcpy(&override->map_definition, definition, sizeof(*definition));
			memcpy(definition, &made->definition, sizeof(*definition));
			override->hud = made;
			if (!hud_kept(tag_index, made->file, made->file_size))
			{
				made->next = mod_unit_huds_globals.huds;
				mod_unit_huds_globals.huds = made;
			}
			loaded_count++;
		}
		*changed = TRUE;
	}

	for (index = 0; index < mod_unit_huds_globals.override_count; index++)
	{
		if (mod_unit_huds_globals.overrides[index].hud)
			played_count++;
	}
	if (!at_load || loaded_count || refused_count)
	{
		error(_error_silent, "mods: %ld unit HUDs from their files (%ld new), %ld given back, %ld left out",
			played_count, loaded_count, removed_count, refused_count);
		if (!at_load && mods_say())
		{
			console_printf(FALSE, "mods: %ld unit HUDs from their files (%ld new), %ld given back, %ld left out",
				played_count, loaded_count, removed_count, refused_count);
		}
	}

	return played_count;
}

void mod_unit_huds_tags_unloaded(
	boolean give_back)
{
	struct mod_unit_hud *hud = mod_unit_huds_globals.huds;
	long index;

	/* (the map's tags are still there to take their own back) */
	for (index = 0; give_back && index < mod_unit_huds_globals.override_count; index++)
	{
		struct mod_unit_hud_override *override = &mod_unit_huds_globals.overrides[index];

		if (override->hud)
			memcpy(tag_get(UNIT_HUD_INTERFACE_DEFINITION_TAG, override->tag_index), &override->map_definition,
				sizeof(override->map_definition));
	}
	while (hud)
	{
		struct mod_unit_hud *next = hud->next;

		mod_unit_hud_free(hud);
		hud = next;
	}
	if (mod_unit_huds_globals.overrides)
		free(mod_unit_huds_globals.overrides);
	memset(&mod_unit_huds_globals, 0, sizeof(mod_unit_huds_globals));

	return;
}
