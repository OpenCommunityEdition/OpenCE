/*
MCC_IMPORT.C

halo --import-mcc <output folder> [--from <halo1 folder> | steam] [<map>...]:
writes the Master Chief Collection's Halo 1 maps (the data root's maps
folder's, or those of the folder --from names, read as m:\) as the Xbox's
cache files (version 5) in the output folder, so that the game loads them as
it loads the disc's maps (docs/mcc_caches.md). The first start's import
(port/linux/src/mcc_install.c) runs it in a second copy of the game.

Each map is loaded as a Custom Edition map is (custom_edition_cache_tags_load:
its tags moved and converted, its models' vertices compressed), with
custom_edition_geometry_import set, so that its geometry is kept for the file
instead of being given to the renderer, and given the disc's values and tag
order (mcc_import_patch.c). Then the map's file is written as the disc's are:
the structure BSPs, each made the Xbox's (moved to the top of the tag cache,
a header and buffer arrays before it); each bitmap's pixels, made the Xbox's
by the texture cache's own rasterizer_xbox_bitmap_rebuild_hardware_format;
the sounds, from MCC's banks (mcc_import_sounds.c); and the tags last, copied
by their schemas into the Xbox's tag cache at 0x803A6000 with the models'
vertex and index buffers before them, then checked by tag_validate_tags as
the game checks a map's. The written map's simulation data is fingerprinted
and compared with the disc's (mcc_import_fingerprint.c), which decides
whether it is written as the disc's map or as MCC's. The body is deflated by
the game's own zlib after the header, as the disc's maps are (at its fastest
level).
*/

/* ---------- headers */

#include "cseries.h"
#include "cseries/cseries_windows.h"
#include "errors.h"
#include "tag_files/tag_groups.h"
#include "bitmaps/bitmap_group.h"
#include "hs/hs.h"
#include "hs/hs_scenario_definitions.h"
#include "models/model_definitions.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/rasterizer_geometry.h"
#include "rasterizer/rasterizer_swizzle.h"
#include "scenario/scenario_definitions.h"
#include "structures/structure_bsp_definitions.h"
#include "cache/physical_memory_map.h"
#include "cache_file_formats.h"
#include "custom_edition_cache.h"
#include "tag_schema.h"
#include "mcc_import.h"
#include "memory/zlib/zlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- constants */

enum
{
	V5_HEADER_BYTES = 0x800,
	V5_TAG_ADDRESS = 0x803A6000,
	V5_TAG_CACHE_BYTES = 0x1600000,
	/* a BSP is loaded below the top of the tag cache */
	V5_BSP_TOP = V5_TAG_ADDRESS + V5_TAG_CACHE_BYTES,
	V5_BSP_HEADER_BYTES = 0x18,
	V5_BUFFER_ENTRY_BYTES = 12,
	V5_SCENARIO_STRUCTURE_BSPS_OFFSET = 0x5A4,
	V5_SCENARIO_TYPE_OFFSET = 0x3C,
	V5_BITMAP_CACHED_BIT = 7,
	/* what draws a HUD bitmap (mcc_import_hud) */
	MCC_IMPORT_HUD_PLACEMENT = 1,
	MCC_IMPORT_HUD_ICONS,
	MCC_IMPORT_HUD_MASK,
	MCC_IMPORT_MAXIMUM_BSPS = 32,
	/* the HUD sprite sheets written in the disc's layout (the operations'
	sprite_layout: the meters'), and each one's bitmaps and sprites */
	MCC_IMPORT_MAXIMUM_SPRITE_LAYOUTS = 4,
	MCC_IMPORT_MAXIMUM_LAYOUT_BITMAPS = 8,
	MCC_IMPORT_MAXIMUM_LAYOUT_SPRITES = 64,
	/* (bitmap_group's flags) the digits drawn at half their size */
	V5_BITMAP_GROUP_HALF_HUD_SCALE_BIT = 4,
	/* the scenario's script syntax data: a data array of nodes */
	V5_SCENARIO_SCRIPT_SYNTAX_DATA_OFFSET = 0x474,
	V5_SCRIPT_NODE_COUNT_OFFSET = 0x2E,
	V5_SCRIPT_NODES_OFFSET = 0x38,
	V5_SCRIPT_NODE_BYTES = 0x14,
	V5_IO_BYTES = 1 << 16,
};

#define V5_BUILD "01.10.12.2276"

/* ---------- structures */

/* this build's model geometry and part (as custom_edition_geometry.c, which
has why) */
struct mcc_import_model_geometry
{
	byte reserved[0x24];
	struct tag_block parts;
};

struct mcc_import_model_part
{
	unsigned long flags;
	short shader_index;
	char previous_part_index;
	char next_part_index;
	short centroid_primary_node_index;
	short centroid_secondary_node_index;
	real centroid_primary_node_weight;
	real centroid_secondary_node_weight;
	real_point3d centroid;
	struct tag_block uncompressed_vertices;
	struct tag_block compressed_vertices;
	struct tag_block triangles;
	struct triangle_buffer triangle_buffer;
	struct vertex_buffer vertex_buffer;
};

typedef char mcc_import_model_part_size[sizeof(struct mcc_import_model_part) == 0x68 ? 1 : -1];

/* the Xbox cache's header, tag header, tag instance and structure BSP
header, as cache_files.c defines them for itself (no header declares them) */
struct mcc_import_header
{
	unsigned long header_signature;
	long version;
	long file_length;
	byte reserved0C[4];
	long tag_data_offset;
	long tag_data_size;
	byte reserved18[8];
	char name[0x20];
	char build[0x20];
	short scenario_type;
	word pad62;
	unsigned long checksum;
	byte reserved68[0x794];
	unsigned long footer_signature;
};


struct mcc_import_tag_header
{
	struct mcc_import_tag_instance *tag_instances;
	long scenario_tag_index;
	unsigned long checksum;
	long tag_count;
	long vertex_buffer_count;
	void *vertex_buffers;
	long index_buffer_count;
	void *index_buffers;
	unsigned long signature;
};

struct mcc_import_structure_bsp_header
{
	void *base_address;
	long vertex_buffer_count;
	void *vertex_buffers;
	long lightmap_vertex_buffer_count;
	void *lightmap_vertex_buffers;
	unsigned long signature;
};

typedef char mcc_import_header_size[sizeof(struct mcc_import_header) == 0x800 ? 1 : -1];
typedef char mcc_import_tag_header_size[sizeof(struct mcc_import_tag_header) == 0x24 ? 1 : -1];
typedef char mcc_import_structure_bsp_header_size[sizeof(struct mcc_import_structure_bsp_header) == 0x18 ? 1 : -1];

/* a buffer as Xbox caches carry them (Direct3D's resource: its common word,
its data, its lock; the game writes the first when it registers it) */
struct mcc_import_buffer_entry
{
	unsigned long common;
	unsigned long data;
	unsigned long lock;
};

/* the space a copy is made into: `bytes` seen at `address` */
struct mcc_import_space
{
	byte *bytes;
	unsigned long address;
	unsigned long size;
	unsigned long used;
	boolean overflowed;
};

/* the body of the file being written, deflated as it goes */
struct mcc_import_body
{
	FILE *file;
	z_stream stream;
	unsigned long length;
	unsigned long crc;
	boolean failed;
	byte output[V5_IO_BYTES];
};

/* a HUD sprite sheet written in the disc's layout (mcc_import_sprite_layout_plan):
where MCC's pixels of each sprite are, and where the disc has the sprite */
struct mcc_import_sprite_layout
{
	long tag_index;
	/* MCC's bitmaps, as loaded */
	short source_count;
	struct
	{
		short width;
		short height;
		long pixels_offset;
		long pixels_size;
	} sources[MCC_IMPORT_MAXIMUM_LAYOUT_BITMAPS];
	/* every sprite, in the order of the sequences: MCC's rectangle and the
	disc's (left, top, right, bottom, in pixels of their bitmaps) */
	short sprite_count;
	struct
	{
		short source;
		short target;
		real source_rectangle[4];
		long target_rectangle[4];
	} sprites[MCC_IMPORT_MAXIMUM_LAYOUT_SPRITES];
	/* how much larger MCC's sprites are than the disc's */
	real ratio;
};

/* ---------- globals */

static struct
{
	struct mcc_import_tag_instance *instances;
	long tag_count;
	/* the loaded tag table, copied (the operations add tags to it) */
	struct mcc_import_tags table;
	boolean operations;
	/* each tag's name in the copy */
	unsigned long *name_addresses;
	/* an empty string in the copy, the name of a reference to no tag (the
	checks give one outside the tags theirs, which differs every run) */
	unsigned long empty_name_address;
	long schemaless;
	long bitmaps;
	long sounds_silenced;
	/* the tags written, in the disc's order (mcc_import_order): each the
	loaded tag it is (NONE: a tag of the disc's MCC has none of, written
	empty), and each loaded tag's place among them (NONE: one MCC added) */
	long out_count;
	long *out_sources;
	unsigned long *out_groups;
	char **out_names;
	long *old_to_new;
	long references_dropped;
	/* the tag being copied (for the trace) */
	char const *copying;
	long placeholders;
	/* loaded tags kept after the disc's (mcc_import_keep) */
	long kept;
	/* the disc's tags, the first of those written */
	long disc_count;
	/* the structure BSPs written (for the fingerprint) */
	struct mcc_fingerprint_bsp bsps[MCC_IMPORT_MAXIMUM_BSPS];
	long bsp_count;
	/* each HUD bitmap's factor: written at 1/factor of MCC's size
	(mcc_import_hud), and the message icons' and the scopes' masks' */
	short *hud_factor;
	/* where the operations give the disc's placements (their scales for the
	disc's bitmaps), how much they are scaled for each bitmap as written: the
	disc's width over its own */
	real *hud_scale;
	short icon_factor;
	short mask_factor;
	long hud_bitmaps_halved;
	/* the HUD sprite sheets written in the disc's layout */
	struct mcc_import_sprite_layout sprite_layouts[MCC_IMPORT_MAXIMUM_SPRITE_LAYOUTS];
	short sprite_layout_count;
	/* (a second import of a map larger than its cache file: mcc_import_run)
	the bytes its bitmaps are to give up, and the bitmaps halved for them */
	long fit_budget;
	long fitted;
	/* the sounds written, from MCC's banks */
	long sounds;
	char *out_names_text;
} mcc_import_globals;

/* ---------- private code: messages */

/* to the console as well as the log: the import runs before the log is
open */
#define mcc_import_error(...) (error(_error_silent, __VA_ARGS__), fprintf(stderr, __VA_ARGS__), fputc(10, stderr))

/* ---------- private code: memory */

/* zeroed memory from the game's malloc, which the game's free takes (calloc
is the C library's) */
static void *mcc_import_calloc(
	size_t count,
	size_t size)
{
	void *memory = malloc(count * size + 1);

	if (memory)
		csmemset(memory, 0, count * size);

	return memory;
}

/* the game's free, which does not take NULL */
static void mcc_import_free(
	void *memory)
{
	if (memory)
		free(memory);

	return;
}

/* ---------- private code: the schemas */

/* a meter ('metr', the HUD's shield and body meters), which the checks have
no schema of (the game reads it as it is): its two bitmap references and its
encoded stencil, as the disc's maps lay it out (0xAC bytes, the data after
it) */
static struct tag_schema_field const mcc_import_meter_fields[] =
{
	{ _tag_schema_reference, sizeof(struct tag_reference), 1, 0, 0x04, 0, 0, 0, NULL, NULL, "stencil_bitmaps" },
	{ _tag_schema_reference, sizeof(struct tag_reference), 1, 0, 0x14, 0, 0, 0, NULL, NULL, "source_bitmap" },
	{ _tag_schema_data, sizeof(struct tag_data), 1, 0, 0x98, 0x10000, 0, 0, NULL, NULL, "encoded_stencil" },
	TAG_SCHEMA_END
};

static struct tag_schema_definition const mcc_import_meter_definition =
{
	"meter", 0xAC, mcc_import_meter_fields
};

static struct tag_schema_group const mcc_import_meter_group =
{
	'metr', { NONE, NONE }, &mcc_import_meter_definition
};

static struct tag_schema_group const *mcc_import_schema(
	unsigned long group_tag)
{
	struct tag_schema_group const *const *list;
	struct tag_schema_group const *group;

	if (group_tag == 'metr')
		return &mcc_import_meter_group;

	for (list = tag_schema_group_lists; *list; list++)
	{
		for (group = *list; group->group_tag; group++)
		{
			if (group->group_tag == group_tag)
				return group;
		}
	}

	return NULL;
}

/* ---------- private code: the order */

/* the tag index of the tag written `new_index`th, as the Xbox's tools number
them */
static long mcc_import_new_handle(
	long new_index)
{
	return (long)(((unsigned long)(0xE174 + new_index) << 16) | (unsigned long)new_index);
}

/* a loaded tag's index as the written tag's (NONE for a tag left out) */
static long mcc_import_renumber(
	long handle)
{
	long old_index;

	if (handle == NONE)
		return NONE;
	old_index = handle & 0xFFFF;
	if (old_index >= mcc_import_globals.tag_count || mcc_import_globals.old_to_new[old_index] == NONE)
	{
		if (getenv("HALO_MCC_IMPORT_TRACE") && old_index < mcc_import_globals.tag_count)
		{
			fprintf(stderr, "mcc import: a reference to '%s' (%.4s) left out, in '%s'\n",
				mcc_import_globals.instances[old_index].name, (char const *)&mcc_import_globals.instances[old_index].group_tag,
				mcc_import_globals.copying ? mcc_import_globals.copying : "a structure bsp, or the header");
		}
		mcc_import_globals.references_dropped++;
		return NONE;
	}

	return mcc_import_new_handle(mcc_import_globals.old_to_new[old_index]);
}

static void mcc_import_keep_walk(
	struct tag_schema_definition const *definition,
	byte const *element);

/* a loaded tag that a written one names, written too (after the disc's
tags, which keep their indices) if the order left it out */
static void mcc_import_keep_tag(
	long handle)
{
	long old_index;

	if (handle == NONE)
		return;
	old_index = handle & 0xFFFF;
	if (old_index >= mcc_import_globals.tag_count || mcc_import_globals.old_to_new[old_index] != NONE)
		return;
	mcc_import_globals.old_to_new[old_index] = mcc_import_globals.out_count;
	mcc_import_globals.out_sources[mcc_import_globals.out_count] = old_index;
	mcc_import_globals.out_groups[mcc_import_globals.out_count] =
		(unsigned long)mcc_import_globals.instances[old_index].group_tag;
	mcc_import_globals.out_names[mcc_import_globals.out_count] = mcc_import_globals.instances[old_index].name;
	mcc_import_globals.out_count++;
	mcc_import_globals.kept++;

	return;
}

static void mcc_import_keep_walk(
	struct tag_schema_definition const *definition,
	byte const *element)
{
	struct tag_schema_field const *field;

	for (field = definition->fields; field->type != _tag_schema_terminator; field++)
	{
		short element_index;

		for (element_index = 0; element_index < field->count; element_index++)
		{
			byte const *at = element + field->offset + element_index * field->size;

			if (field->type == _tag_schema_reference)
			{
				mcc_import_keep_tag(((struct tag_reference const *)at)->index);
			}
			else if (field->type == _tag_schema_tag_index)
			{
				mcc_import_keep_tag(*(long const *)at);
			}
			else if (field->type == _tag_schema_struct)
			{
				mcc_import_keep_walk(field->definition, at);
			}
			else if (field->type == _tag_schema_block)
			{
				struct tag_block const *block = (struct tag_block const *)at;
				struct tag_schema_definition const *block_definition = field->definition;
				long index;

				if (block->count <= 0 || !block->address)
					continue;
				for (index = 0; index < block->count; index++)
					mcc_import_keep_walk(block_definition, (byte const *)block->address + index * block_definition->size);
			}
		}
	}

	return;
}

/* the loaded tag (its index) that is the disc's object of the name of the
loaded object `handle` names, where that one is not the disc's (MCC's
scripts take a10's and d20's pelican for scenery, the disc's for the vehicle
it is), or NONE */
static long mcc_import_disc_object(
	long handle)
{
	static unsigned long const object_groups[] =
	{
		'bipd', 'vehi', 'weap', 'eqip', 'garb', 'proj', 'scen', 'mach', 'ctrl', 'lifi', 'plac', 'ssce'
	};
	long old_index = handle == NONE ? NONE : handle & 0xFFFF;
	long index;

	if (old_index == NONE || old_index >= mcc_import_globals.tag_count || mcc_import_globals.old_to_new[old_index] != NONE)
		return NONE;
	for (index = 0; index < mcc_import_globals.tag_count; index++)
	{
		struct mcc_import_tag_instance const *instance = &mcc_import_globals.instances[index];
		short group;

		if (mcc_import_globals.old_to_new[index] == NONE ||
			strcmp(instance->name, mcc_import_globals.instances[old_index].name))
		{
			continue;
		}
		for (group = 0; group < NUMBEROF(object_groups); group++)
		{
			if ((unsigned long)instance->group_tag == object_groups[group])
				return index;
		}
	}

	return NONE;
}

/* the objects MCC's scripts name under another group than the disc's (the
scenario's script references and syntax nodes), named as the disc's: the
other is then not written, and the disc's tags are all the map simulates */
static void mcc_import_scripts_disc_objects(
	void)
{
	struct scenario *scenario = mcc_import_globals.instances[mcc_import_globals.table.scenario_index].base_address;
	struct tag_data const *syntax_data;
	byte *syntax;
	short node_count, node_index;
	long index;

	if (!scenario)
		return;
	for (index = 0; scenario->hs_references.address && index < scenario->hs_references.count; index++)
	{
		struct tag_reference *reference = &((struct hs_reference *)scenario->hs_references.address)[index].reference;
		long disc_index = mcc_import_disc_object(reference->index);

		if (disc_index != NONE)
		{
			struct mcc_import_tag_instance const *instance = &mcc_import_globals.instances[disc_index];

			reference->group_tag = (unsigned long)instance->group_tag;
			reference->name = instance->name;
			reference->index = instance->tag_index;
		}
	}
	syntax_data = (struct tag_data const *)((byte const *)scenario + V5_SCENARIO_SCRIPT_SYNTAX_DATA_OFFSET);
	syntax = syntax_data->address;
	node_count = syntax && syntax_data->size >= V5_SCRIPT_NODES_OFFSET ? *(short const *)(syntax + V5_SCRIPT_NODE_COUNT_OFFSET) : 0;
	for (node_index = 0; node_index < node_count &&
		V5_SCRIPT_NODES_OFFSET + (node_index + 1) * V5_SCRIPT_NODE_BYTES <= syntax_data->size; node_index++)
	{
		byte *node = syntax + V5_SCRIPT_NODES_OFFSET + node_index * V5_SCRIPT_NODE_BYTES;

		if (*(short const *)node && (*(short const *)(node + 6) & 1) && *(short const *)(node + 4) == _hs_type_object_definition)
		{
			long disc_index = mcc_import_disc_object(*(long const *)(node + 0x10));

			if (disc_index != NONE)
				*(long *)(node + 0x10) = mcc_import_globals.instances[disc_index].tag_index;
		}
	}

	return;
}

/* Until the patch names the disc's tags in their place, the loaded tags the
written ones name (by reference, tag index or script) are written too, after
the disc's; the rest of what MCC added is left out. */
static boolean mcc_import_keep(
	void)
{
	long out_index;

	mcc_import_scripts_disc_objects();
	/* the structure BSPs' first (they are read only to be written: read here
	too, as they are then, for the tags they name) */
	{
		struct tag_schema_group const *bsp_schema = mcc_import_schema('sbsp');
		byte const *scenario = mcc_import_globals.instances[mcc_import_globals.table.scenario_index].base_address;
		struct tag_block const *references = scenario ?
			(struct tag_block const *)(scenario + V5_SCENARIO_STRUCTURE_BSPS_OFFSET) : NULL;
		long bsp_index;

		for (bsp_index = 0; bsp_schema && references && bsp_index < references->count; bsp_index++)
		{
			struct scenario_structure_bsp_reference *reference =
				(struct scenario_structure_bsp_reference *)references->address + bsp_index;

			custom_edition_cache_read(NONE, reference->file_offset, reference->file_size, reference->base_address);
			if (custom_edition_structure_bsp_read(reference))
			{
				mcc_import_keep_walk(bsp_schema->definition,
					((struct mcc_import_structure_bsp_header const *)reference->base_address)->base_address);
			}
		}
	}
	/* (the list grows as it is walked: a kept tag's own are kept) */
	for (out_index = 0; out_index < mcc_import_globals.out_count; out_index++)
	{
		long old_index = mcc_import_globals.out_sources[out_index];
		struct mcc_import_tag_instance const *instance;
		struct tag_schema_group const *schema;

		if (old_index == NONE)
			continue;
		instance = &mcc_import_globals.instances[old_index];
		schema = mcc_import_schema((unsigned long)instance->group_tag);
		if (!instance->base_address || !schema || !schema->definition)
			continue;
		mcc_import_keep_walk(schema->definition, instance->base_address);
		if (instance->group_tag == 'scnr')
		{
			struct tag_data const *syntax_data = (struct tag_data const *)((byte const *)instance->base_address +
				V5_SCENARIO_SCRIPT_SYNTAX_DATA_OFFSET);
			byte const *syntax = syntax_data->address;
			short node_count = syntax && syntax_data->size >= V5_SCRIPT_NODES_OFFSET ?
				*(short const *)(syntax + V5_SCRIPT_NODE_COUNT_OFFSET) : 0;
			short node_index;

			for (node_index = 0; node_index < node_count &&
				V5_SCRIPT_NODES_OFFSET + (node_index + 1) * V5_SCRIPT_NODE_BYTES <= syntax_data->size; node_index++)
			{
				byte const *node = syntax + V5_SCRIPT_NODES_OFFSET + node_index * V5_SCRIPT_NODE_BYTES;
				short type = *(short const *)(node + 4);

				if (*(short const *)node && (*(short const *)(node + 6) & 1) && type >= _hs_type_sound &&
					type <= _hs_type_object_definition)
				{
					mcc_import_keep_tag(*(long const *)(node + 0x10));
				}
			}
		}
	}

	return TRUE;
}

/* The tags to write and their order: the disc's, from the map's operations
(mcc_import_data_read's <map>.p2), whose `order` line names every tag of the
disc's map as group:path (a group's spaces written as underscores); else the
loaded tags as they are. */
static boolean mcc_import_order(
	char const *map_name)
{
	char name[80];
	char *text;
	char *order = NULL;
	long index, count, size;

	snprintf(name, sizeof(name), "%s.p2", map_name);
	text = mcc_import_data_read(name, &size);
	if (text)
	{
		char *line = text;

		while (line && *line)
		{
			if (!strncmp(line, "order\t-\t-\t", 10))
			{
				order = line + 10;
				break;
			}
			line = strchr(line, '\n');
			if (line)
				line++;
		}
		if (order && strchr(order, '\n'))
			*strchr(order, '\n') = 0;
		/* (a file written on Windows ends its lines with a carriage return) */
		if (order && strchr(order, '\r'))
			*strchr(order, '\r') = 0;
	}

	count = order ? 1 : mcc_import_globals.tag_count;
	for (index = 0; order && order[index]; index++)
		count += order[index] == '|';
	mcc_import_globals.out_count = count;
	mcc_import_globals.disc_count = count;
	/* (room for the loaded tags kept after the disc's: mcc_import_keep) */
	mcc_import_globals.out_sources = mcc_import_calloc((size_t)(count + mcc_import_globals.tag_count), sizeof(long));
	mcc_import_globals.out_groups = mcc_import_calloc((size_t)(count + mcc_import_globals.tag_count), sizeof(unsigned long));
	mcc_import_globals.out_names = mcc_import_calloc((size_t)(count + mcc_import_globals.tag_count), sizeof(char *));
	mcc_import_globals.old_to_new = mcc_import_calloc((size_t)mcc_import_globals.tag_count, sizeof(long));
	if (!mcc_import_globals.out_sources || !mcc_import_globals.out_groups || !mcc_import_globals.out_names ||
		!mcc_import_globals.old_to_new)
	{
		mcc_import_data_free(text);
		return FALSE;
	}
	for (index = 0; index < mcc_import_globals.tag_count; index++)
		mcc_import_globals.old_to_new[index] = order ? NONE : index;
	if (!order)
	{
		for (index = 0; index < count; index++)
		{
			mcc_import_globals.out_sources[index] = index;
			mcc_import_globals.out_groups[index] = (unsigned long)mcc_import_globals.instances[index].group_tag;
			mcc_import_globals.out_names[index] = mcc_import_globals.instances[index].name;
		}
		mcc_import_error("mcc import: %s: no tag order (%s): the loaded tags written as they are", map_name, name);
		mcc_import_data_free(text);
		return TRUE;
	}

	for (index = 0; index < count; index++)
	{
		char *end = strchr(order, '|');
		unsigned long group = 0;
		long old_index;
		short character;

		if (end)
			*end = 0;
		for (character = 0; character < 4 && order[character]; character++)
			group = group << 8 | (unsigned char)(order[character] == '_' ? ' ' : order[character]);
		mcc_import_globals.out_groups[index] = group;
		mcc_import_globals.out_names[index] = order + 5;
		mcc_import_globals.out_sources[index] = NONE;
		for (old_index = 0; old_index < mcc_import_globals.tag_count; old_index++)
		{
			struct mcc_import_tag_instance const *instance = &mcc_import_globals.instances[old_index];

			if ((unsigned long)instance->group_tag == group && mcc_import_globals.old_to_new[old_index] == NONE &&
				!strcmp(instance->name, order + 5))
			{
				mcc_import_globals.out_sources[index] = old_index;
				mcc_import_globals.old_to_new[old_index] = index;
				break;
			}
		}
		if (mcc_import_globals.out_sources[index] == NONE)
			mcc_import_globals.placeholders++;
		if (!end)
			break;
		order = end + 1;
	}
	/* (the names stay in `text`, which lives as long as the import of the
	map: freed with the order) */
	mcc_import_globals.out_names_text = text;

	return mcc_import_keep();
}

/* ---------- private code: the space */

static unsigned long mcc_import_allocate(
	struct mcc_import_space *space,
	unsigned long size,
	unsigned long alignment)
{
	unsigned long offset = (space->used + alignment - 1) & ~(alignment - 1);

	if (offset > space->size || size > space->size - offset)
	{
		space->overflowed = TRUE;
		return 0;
	}
	/* (the alignment's gap too: the space is reused from map to map, and
	what an earlier map left there would be written) */
	csmemset(space->bytes + space->used, 0, offset + size - space->used);
	space->used = offset + size;

	return space->address + offset;
}

static void *mcc_import_at(
	struct mcc_import_space const *space,
	unsigned long address)
{
	return space->bytes + (address - space->address);
}

/* ---------- private code: the copy */

/* a stand-in's references (zeros: mcc_import_tags), named as the copy names
a reference to no tag, with the map's empty name; else tag_validate_tags
gives them its own, an address of the game that imports, which the map
would keep */
static void mcc_import_empty_references(
	struct tag_schema_definition const *definition,
	byte *element)
{
	struct tag_schema_field const *field;

	for (field = definition->fields; field->type != _tag_schema_terminator; field++)
	{
		short element_index;

		for (element_index = 0; element_index < field->count; element_index++)
		{
			byte *address = element + field->offset + element_index * field->size;

			if (field->type == _tag_schema_reference)
			{
				struct tag_reference *reference = (struct tag_reference *)address;

				reference->name = (char *)mcc_import_globals.empty_name_address;
				reference->index = NONE;
			}
			else if (field->type == _tag_schema_struct)
			{
				mcc_import_empty_references(field->definition, address);
			}
		}
	}

	return;
}

static void mcc_import_copy_pointers(
	struct mcc_import_space *space,
	struct tag_schema_definition const *definition,
	byte const *source,
	byte *copy);

/* the element at `source` of `definition`, and everything its blocks and
data hold, into `space`, at `copy` */
static void mcc_import_copy_element(
	struct mcc_import_space *space,
	struct tag_schema_definition const *definition,
	byte const *source,
	byte *copy)
{
	csmemcpy(copy, source, definition->size);
	mcc_import_copy_pointers(space, definition, source, copy);

	return;
}

static void mcc_import_copy_pointers(
	struct mcc_import_space *space,
	struct tag_schema_definition const *definition,
	byte const *source,
	byte *copy)
{
	struct tag_schema_field const *field;

	for (field = definition->fields; field->type != _tag_schema_terminator; field++)
	{
		short element_index;

		for (element_index = 0; element_index < field->count; element_index++)
		{
			long offset = field->offset + element_index * field->size;
			byte const *source_field = source + offset;
			byte *copy_field = copy + offset;

			switch (field->type)
			{
			case _tag_schema_block:
			{
				struct tag_block const *block = (struct tag_block const *)source_field;
				struct tag_block *copied = (struct tag_block *)copy_field;
				struct tag_schema_definition const *element = field->definition;
				unsigned long address = 0;
				long index;

				if (block->count > 0 && block->address)
				{
					address = mcc_import_allocate(space, (unsigned long)block->count * element->size, 4);
				}
				copied->address = (void *)address;
				copied->definition = NULL;
				if (!address)
				{
					copied->count = 0;
					break;
				}
				for (index = 0; index < block->count; index++)
				{
					mcc_import_copy_element(
						space,
						element,
						(byte const *)block->address + index * element->size,
						(byte *)mcc_import_at(space, address) + index * element->size);
				}
				break;
			}
			case _tag_schema_data:
			{
				struct tag_data const *data = (struct tag_data const *)source_field;
				struct tag_data *copied = (struct tag_data *)copy_field;
				unsigned long address = 0;
				unsigned long source_address = (unsigned long)data->address;

				/* (a tag of a group the move had no schema of, a meter, still
				names MCC's addresses) */
				if (source_address - MCC_TAG_DATA_ADDRESS < CUSTOM_EDITION_TAG_CACHE_BYTES)
					source_address = source_address - MCC_TAG_DATA_ADDRESS + CUSTOM_EDITION_TAG_CACHE_ADDRESS;
				if (data->size > 0 && source_address - CUSTOM_EDITION_TAG_CACHE_ADDRESS < CUSTOM_EDITION_TAG_CACHE_BYTES &&
					(unsigned long)data->size <= CUSTOM_EDITION_TAG_CACHE_BYTES - (source_address - CUSTOM_EDITION_TAG_CACHE_ADDRESS))
				{
					address = mcc_import_allocate(space, (unsigned long)data->size, 4);
					if (address)
						csmemcpy(mcc_import_at(space, address), (void const *)source_address, data->size);
				}
				else if (data->size > 0 && data->address)
				{
					/* (the BSPs' compressed vertices, made for the import
					outside the tags) */
					address = mcc_import_allocate(space, (unsigned long)data->size, 4);
					if (address)
						csmemcpy(mcc_import_at(space, address), data->address, data->size);
				}
				copied->address = (void *)address;
				copied->definition = NULL;
				if (!address)
					copied->size = 0;
				break;
			}
			case _tag_schema_file_data:
			{
				/* a bitmap's pixels and a sound's samples are in the body
				already (their offsets set in the tag: mcc_import_bitmaps,
				mcc_import_sounds_write) */
				struct tag_data *copied = (struct tag_data *)copy_field;

				copied->address = NULL;
				copied->definition = NULL;
				break;
			}
			case _tag_schema_reference:
			{
				struct tag_reference *copied = (struct tag_reference *)copy_field;

				copied->index = mcc_import_renumber(copied->index);
				copied->name = copied->index != NONE ?
					(char *)mcc_import_globals.name_addresses[copied->index & 0xFFFF] :
					(char *)mcc_import_globals.empty_name_address;
				copied->name_length = 0;
				break;
			}
			case _tag_schema_tag_index:
				*(long *)copy_field = mcc_import_renumber(*(long const *)source_field);
				break;
			case _tag_schema_struct:
				mcc_import_copy_pointers(space, field->definition, source_field, copy_field);
				break;
			default:
				break;
			}
		}
	}

	return;
}

/* the bytes the copy of the element at `source` takes beyond its own */
static unsigned long mcc_import_measure(
	struct tag_schema_definition const *definition,
	byte const *source)
{
	struct tag_schema_field const *field;
	unsigned long total = 0;

	for (field = definition->fields; field->type != _tag_schema_terminator; field++)
	{
		short element_index;

		for (element_index = 0; element_index < field->count; element_index++)
		{
			byte const *source_field = source + field->offset + element_index * field->size;

			if (field->type == _tag_schema_block)
			{
				struct tag_block const *block = (struct tag_block const *)source_field;
				struct tag_schema_definition const *element = field->definition;
				long index;

				if (block->count <= 0 || !block->address)
					continue;
				total = ((total + 3) & ~3UL) + (unsigned long)block->count * element->size;
				for (index = 0; index < block->count; index++)
					total += mcc_import_measure(element, (byte const *)block->address + index * element->size);
			}
			else if (field->type == _tag_schema_data)
			{
				struct tag_data const *data = (struct tag_data const *)source_field;

				if (data->size > 0 && data->address)
					total = ((total + 3) & ~3UL) + (unsigned long)data->size;
			}
			else if (field->type == _tag_schema_struct)
			{
				total += mcc_import_measure(field->definition, source_field);
			}
		}
	}

	return total + 4;
}

/* ---------- private code: the body */

static void mcc_import_body_deflate(
	struct mcc_import_body *body,
	int flush)
{
	do
	{
		size_t produced;

		body->stream.next_out = body->output;
		body->stream.avail_out = sizeof(body->output);
		if (deflate(&body->stream, flush) == Z_STREAM_ERROR)
		{
			body->failed = TRUE;
			return;
		}
		produced = sizeof(body->output) - body->stream.avail_out;
		if (produced && fwrite(body->output, 1, produced, body->file) != produced)
		{
			body->failed = TRUE;
			return;
		}
	} while (body->stream.avail_out == 0);

	return;
}

static void mcc_import_body_write(
	struct mcc_import_body *body,
	void const *bytes,
	unsigned long size)
{
	if (!size || body->failed)
		return;
	body->crc = crc32(body->crc, (Bytef const *)bytes, (uInt)size);
	body->stream.next_in = (Bytef *)bytes;
	body->stream.avail_in = (uInt)size;
	mcc_import_body_deflate(body, Z_NO_FLUSH);
	body->length += size;

	return;
}

/* zeros up to a multiple of `alignment`: the body's offset after them */
static unsigned long mcc_import_body_align(
	struct mcc_import_body *body,
	unsigned long alignment)
{
	static byte const zeros[0x800];
	unsigned long padding = (alignment - body->length % alignment) % alignment;

	mcc_import_body_write(body, zeros, padding);

	return body->length;
}

/* the offset in the file of the body's next byte */
static unsigned long mcc_import_body_file_offset(
	struct mcc_import_body const *body)
{
	return V5_HEADER_BYTES + body->length;
}

/* (mcc_import_sound_write_proc) a sound's samples into the body, at a
multiple of 0x200 as the disc's are: their offset in the file */
static unsigned long mcc_import_body_sound(
	void *context,
	void const *bytes,
	unsigned long size)
{
	struct mcc_import_body *body = context;
	unsigned long offset;

	mcc_import_body_align(body, 0x200);
	offset = mcc_import_body_file_offset(body);
	mcc_import_body_write(body, bytes, size);

	return offset;
}

/* (mcc_sounds_muted_proc, of exactly its type) */
static void mcc_import_sound_muted(
	uint8_t *sound,
	int32_t pitch_range_count)
{
	mcc_import_sounds_muted(sound, (long)pitch_range_count);

	return;
}

/* ---------- private code: the structure BSPs */

/* The BSP of `reference`, read and converted as the game does to draw it,
written to the body as an Xbox map's are: its header, its materials' buffer
arrays and the BSP copied by its schema after them, at the top of the tag
cache. The reference then names it in the file. */
static boolean mcc_import_structure_bsp(
	struct mcc_import_body *body,
	struct scenario_structure_bsp_reference *reference,
	short bsp_index)
{
	struct tag_schema_group const *schema = mcc_import_schema('sbsp');
	struct structure_bsp *structure_bsp;
	struct mcc_import_structure_bsp_header const *loaded_header;
	struct mcc_import_space space;
	long material_count = 0;
	long lightmap_index;
	unsigned long size, address, root, arrays;
	byte *copy_header;
	boolean success = FALSE;

	if (!schema)
	{
		mcc_import_error("mcc import: no structure bsp schema");
		return FALSE;
	}
	/* read as scenario_structure_bsp_load reads it: its bytes, then (the
	MCC load) its vertex region over the bytes after it */
	custom_edition_cache_read(NONE, reference->file_offset, reference->file_size, reference->base_address);
	if (!custom_edition_structure_bsp_read(reference))
	{
		mcc_import_error("mcc import: structure bsp %d cannot be read", bsp_index);
		return FALSE;
	}
	loaded_header = (struct mcc_import_structure_bsp_header const *)reference->base_address;
	structure_bsp = (struct structure_bsp *)loaded_header->base_address;
	if (!custom_edition_structure_bsp_load(structure_bsp))
	{
		mcc_import_error("mcc import: structure bsp %d cannot be converted", bsp_index);
		return FALSE;
	}
	/* the BSP's operations (the disc's values) */
	if (mcc_import_globals.operations && reference->structure_bsp.index != NONE &&
		(reference->structure_bsp.index & 0xFFFF) < mcc_import_globals.tag_count)
	{
		char name[300];

		snprintf(name, sizeof(name), "sbsp:%s", mcc_import_globals.instances[reference->structure_bsp.index & 0xFFFF].name);
		mcc_import_operations_apply(&mcc_import_globals.table, name, structure_bsp);
	}
	/* the Xbox's: the materials' compressed vertices only (their
	uncompressed ones are left out: custom_edition_structure_bsp_load put the
	compressed ones in compressed_vertex_data) */
	for (lightmap_index = 0; lightmap_index < structure_bsp->lightmaps.count; lightmap_index++)
	{
		struct structure_lightmap *lightmap = TAG_BLOCK_GET_ELEMENT(&structure_bsp->lightmaps, lightmap_index,
			struct structure_lightmap);
		long material_index;

		for (material_index = 0; material_index < lightmap->materials.count; material_index++)
		{
			struct structure_material *material = TAG_BLOCK_GET_ELEMENT(&lightmap->materials, material_index,
				struct structure_material);

			material->uncompressed_vertex_data.size = 0;
			material->uncompressed_vertex_data.address = NULL;
			material_count++;
		}
	}

	size = V5_BSP_HEADER_BYTES + 2 * material_count * V5_BUFFER_ENTRY_BYTES + 4 +
		schema->definition->size + mcc_import_measure(schema->definition, (byte const *)structure_bsp);
	size = (size + 0x7FF) & ~0x7FFUL;
	/* (the measure is generous, 4 bytes an element: the BSP is copied once
	to learn its size, so that it starts where the game reads it, the top of
	the tag cache less that) */
	space.bytes = mcc_import_calloc(1, size);
	space.address = V5_BSP_TOP - size;
	space.size = size;
	space.used = 0;
	space.overflowed = FALSE;
	if (space.bytes)
	{
		long dropped = mcc_import_globals.references_dropped;

		mcc_import_allocate(&space, V5_BSP_HEADER_BYTES, 4);
		mcc_import_allocate(&space, 2 * material_count * V5_BUFFER_ENTRY_BYTES, 4);
		root = mcc_import_allocate(&space, schema->definition->size, 4);
		if (root)
		{
			mcc_import_copy_element(&space, schema->definition, (byte const *)structure_bsp,
				(byte *)mcc_import_at(&space, root));
		}
		if (!space.overflowed && root)
			size = (space.used + 0x7FF) & ~0x7FFUL;
		mcc_import_globals.references_dropped = dropped;
		mcc_import_free(space.bytes);
	}
	address = V5_BSP_TOP - size;
	space.bytes = mcc_import_calloc(1, size);
	space.address = address;
	space.size = size;
	space.used = 0;
	space.overflowed = FALSE;
	if (space.bytes)
	{
		struct mcc_import_buffer_entry *vertex_entries, *lightmap_entries;
		long entry = 0;

		mcc_import_allocate(&space, V5_BSP_HEADER_BYTES, 4);
		arrays = mcc_import_allocate(&space, 2 * material_count * V5_BUFFER_ENTRY_BYTES, 4);
		root = mcc_import_allocate(&space, schema->definition->size, 4);
		if (root)
		{
			struct structure_bsp *copied;

			mcc_import_copy_element(&space, schema->definition, (byte const *)structure_bsp,
				(byte *)mcc_import_at(&space, root));
			copied = (struct structure_bsp *)mcc_import_at(&space, root);
			vertex_entries = (struct mcc_import_buffer_entry *)mcc_import_at(&space, arrays);
			lightmap_entries = vertex_entries + material_count;
			for (lightmap_index = 0; lightmap_index < copied->lightmaps.count; lightmap_index++)
			{
				struct structure_lightmap *lightmap = (struct structure_lightmap *)mcc_import_at(&space,
					(unsigned long)copied->lightmaps.address) + lightmap_index;
				long material_index;

				for (material_index = 0; material_index < lightmap->materials.count; material_index++, entry++)
				{
					struct structure_material *material = (struct structure_material *)mcc_import_at(&space,
						(unsigned long)lightmap->materials.address) + material_index;
					unsigned long data = (unsigned long)material->compressed_vertex_data.address;

					vertex_entries[entry].data = data;
					lightmap_entries[entry].data = data + material->vertices.count *
						rasterizer_geometry_get_vertex_size(_rasterizer_vertex_type_environment_compressed);
					material->vertices.base_address = (void *)vertex_entries[entry].data;
					material->vertices.hardware_format = (void *)(arrays + entry * V5_BUFFER_ENTRY_BYTES);
					material->lightmap_vertices.base_address = (void *)lightmap_entries[entry].data;
					material->lightmap_vertices.hardware_format = (void *)(arrays +
						(material_count + entry) * V5_BUFFER_ENTRY_BYTES);
				}
			}
			copy_header = space.bytes;
			*(unsigned long *)(copy_header + 0x00) = root;
			*(unsigned long *)(copy_header + 0x04) = (unsigned long)material_count;
			*(unsigned long *)(copy_header + 0x08) = arrays;
			*(unsigned long *)(copy_header + 0x0C) = (unsigned long)material_count;
			*(unsigned long *)(copy_header + 0x10) = arrays + material_count * V5_BUFFER_ENTRY_BYTES;
			*(unsigned long *)(copy_header + 0x14) = 'sbsp';
		}
		if (!space.overflowed && root)
		{
			reference->file_offset = (long)mcc_import_body_file_offset(body);
			reference->file_size = (long)size;
			reference->base_address = (void *)address;
			mcc_import_body_write(body, space.bytes, size);
			success = !body->failed;
		}
		else
		{
			mcc_import_error("mcc import: structure bsp %d does not fit its measure", bsp_index);
		}
		/* (kept for the fingerprint, made once the tags are written) */
		if (success && bsp_index < MCC_IMPORT_MAXIMUM_BSPS)
		{
			mcc_import_globals.bsps[bsp_index].address = address;
			mcc_import_globals.bsps[bsp_index].bytes = space.bytes;
			mcc_import_globals.bsps[bsp_index].size = size;
			if (bsp_index >= mcc_import_globals.bsp_count)
				mcc_import_globals.bsp_count = bsp_index + 1;
		}
		else
			mcc_import_free(space.bytes);
	}
	custom_edition_structure_bsp_unload();

	return success;
}

/* ---------- private code: the HUD */

/* the bytes a pixel takes in the uncompressed formats halved (A8, Y8, AY8: 1;
A8Y8: 2; X8R8G8B8, A8R8G8B8: 4), else 0 */
static short mcc_import_pixel_bytes(
	short format)
{
	switch (format)
	{
	case 0:
	case 1:
	case 2:
		return 1;
	case 3:
		return 2;
	case 10:
	case 11:
		return 4;
	default:
		return 0;
	}
}

/* the bytes of a 4 by 4 block of a compressed format (DXT1, DXT3, DXT5), else 0 */
static short mcc_import_block_bytes(
	short format)
{
	return format == 14 ? 8 : format == 15 || format == 16 ? 16 : 0;
}

/* whether a bitmap can be halved: two-dimensional, of an uncompressed format
and at least 2 by 2, or of a compressed one with mipmaps (once: its next
level), at least 8 by 8 */
static boolean mcc_import_bitmap_halvable(
	struct bitmap_data const *bitmap)
{
	if (bitmap->type != 0 || bitmap->mipmap_count < 0 || bitmap->depth > 1)
		return FALSE;
	/* (a compressed one by its mipmaps alone: its next level is its half, of
	whole blocks) */
	if (mcc_import_block_bytes(bitmap->format))
	{
		return bitmap->mipmap_count > 0 && bitmap->width >= 8 && bitmap->height >= 8 &&
			!(bitmap->width % 8) && !(bitmap->height % 8);
	}
	return mcc_import_pixel_bytes(bitmap->format) && bitmap->width >= 2 && bitmap->height >= 2 &&
		!(bitmap->width & 1) && !(bitmap->height & 1);
}

/* The bitmap's pixels (linear, as MCC's bitmaps.map holds them) at half its
size, each pixel the average of four, its colour weighted by its alpha (so
that the colour of what is not drawn does not run into what is): FALSE if it
cannot be halved. */
static boolean mcc_import_bitmap_halve(
	struct bitmap_data *bitmap,
	byte *pixels)
{
	short bytes = mcc_import_pixel_bytes(bitmap->format);
	long width = bitmap->width / 2, height = bitmap->height / 2;
	long x, y;

	if (!mcc_import_bitmap_halvable(bitmap))
		return FALSE;
	/* (with mipmaps, its next level is the half: the first left out) */
	if (bitmap->mipmap_count > 0)
	{
		long top = mcc_import_block_bytes(bitmap->format) ?
			(long)(bitmap->width / 4) * (bitmap->height / 4) * mcc_import_block_bytes(bitmap->format) :
			(long)bitmap->width * bitmap->height * bytes;

		if (top >= bitmap->pixels_size)
			return FALSE;
		memmove(pixels, pixels + top, (size_t)(bitmap->pixels_size - top));
		bitmap->width = (short)width;
		bitmap->height = (short)height;
		bitmap->registration_point.x /= 2;
		bitmap->registration_point.y /= 2;
		bitmap->pixels_size -= top;
		bitmap->mipmap_count--;
		return TRUE;
	}
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			byte const *source[4];
			byte *out = pixels + (y * width + x) * bytes;
			short corner;

			source[0] = pixels + ((2 * y) * bitmap->width + 2 * x) * bytes;
			source[1] = source[0] + bytes;
			source[2] = source[0] + bitmap->width * bytes;
			source[3] = source[2] + bytes;
			if (bytes == 1)
			{
				long sum = 0;

				for (corner = 0; corner < 4; corner++)
					sum += source[corner][0];
				out[0] = (byte)((sum + 2) / 4);
			}
			else
			{
				/* the alpha is the last byte (A8Y8: Y, A; A8R8G8B8: B, G, R, A) */
				short alpha_byte = bytes - 1, channel;
				long alpha_sum = 0;
				byte result[4];

				for (corner = 0; corner < 4; corner++)
					alpha_sum += source[corner][alpha_byte];
				for (channel = 0; channel < alpha_byte; channel++)
				{
					long sum = 0, weighted = 0;

					for (corner = 0; corner < 4; corner++)
					{
						sum += source[corner][channel];
						weighted += source[corner][channel] * source[corner][alpha_byte];
					}
					result[channel] = (byte)(alpha_sum ? (weighted + alpha_sum / 2) / alpha_sum : (sum + 2) / 4);
				}
				result[alpha_byte] = (byte)((alpha_sum + 2) / 4);
				/* (the four read before the first written over: out is never
				past source[0]) */
				csmemcpy(out, result, (unsigned long)bytes);
			}
		}
	}
	bitmap->width = (short)width;
	bitmap->height = (short)height;
	bitmap->registration_point.x /= 2;
	bitmap->registration_point.y /= 2;
	bitmap->pixels_size = width * height * bytes;

	return TRUE;
}

/* ---------- private code: HUD sprite sheets in the disc's layout

MCC drew the meters again, at about twice the disc's size, in sheets of its
own layout (more bitmaps, the sprites elsewhere). Where the operations give
the disc's layout of such a sheet (sprite_layout), it is written in it: the
disc's bitmaps, each sprite where the disc has it, its pixels MCC's,
resampled into the disc's rectangle. The disc's placements then draw it at
their own scale, and the high-res HUD's texture, drawn for that layout
(port/linux/game/hud_hires_tags.c), stands for it. */

/* the sheet `tag_index` written in the disc's layout, or NULL */
static struct mcc_import_sprite_layout const *mcc_import_sprite_layout_find(
	long tag_index)
{
	short index;

	for (index = 0; index < mcc_import_globals.sprite_layout_count; index++)
	{
		if (mcc_import_globals.sprite_layouts[index].tag_index == tag_index)
			return &mcc_import_globals.sprite_layouts[index];
	}

	return NULL;
}

/* how much larger MCC's sprites of the sheet `tag_index` are than those it is
written with: 1 for any sheet not written in the disc's layout */
static real mcc_import_sprite_layout_ratio(
	long tag_index)
{
	struct mcc_import_sprite_layout const *layout = mcc_import_sprite_layout_find(tag_index);

	return layout ? layout->ratio : 1.0f;
}

/* the next number of a layout's text, and the character after it in *end
(FALSE if there is none) */
static boolean mcc_import_sprite_layout_number(
	char const **cursor,
	real *value)
{
	char *end;
	double number = strtod(*cursor, &end);

	if (end == *cursor || !(number >= -65536.0 && number <= 65536.0))
		return FALSE;
	*value = (real)number;
	*cursor = end;

	return TRUE;
}

/* The sheet `tag_index` (a sprite sheet only the HUD's placements draw) planned
to be written in the disc's layout, from the operations' sprite_layout line,
and its tag given that layout: its bitmaps the disc's (their pixels made by
mcc_import_sprite_layout_pixels), its sprites where the disc has them. Not
where the sheet already has the disc's layout, or where it does not have the
disc's sequences and sprites, in uncompressed bitmaps without mipmaps. */
static void mcc_import_sprite_layout_plan(
	long tag_index)
{
	struct mcc_import_tag_instance const *instance = &mcc_import_globals.instances[tag_index];
	struct bitmap_group *group = (struct bitmap_group *)instance->base_address;
	struct mcc_import_sprite_layout layout;
	short target_widths[MCC_IMPORT_MAXIMUM_LAYOUT_BITMAPS], target_heights[MCC_IMPORT_MAXIMUM_LAYOUT_BITMAPS];
	real target_registrations[MCC_IMPORT_MAXIMUM_LAYOUT_SPRITES][2];
	real ratios[MCC_IMPORT_MAXIMUM_LAYOUT_SPRITES * 2];
	char const *bitmaps_text, *sequences_text, *cursor;
	short target_count = 0, ratio_count = 0, format;
	boolean same = TRUE;
	long index, sequence_index;

	if (mcc_import_globals.sprite_layout_count >= MCC_IMPORT_MAXIMUM_SPRITE_LAYOUTS || !group ||
		!mcc_import_operations_sprite_layout(instance->name, &bitmaps_text, &sequences_text))
	{
		return;
	}
	if (group->type != 3 || group->bitmaps.count <= 0 || group->bitmaps.count > MCC_IMPORT_MAXIMUM_LAYOUT_BITMAPS)
		return;
	csmemset(&layout, 0, sizeof(layout));
	layout.tag_index = tag_index;
	format = TAG_BLOCK_GET_ELEMENT(&group->bitmaps, 0, struct bitmap_data)->format;
	for (index = 0; index < group->bitmaps.count; index++)
	{
		struct bitmap_data const *bitmap = TAG_BLOCK_GET_ELEMENT(&group->bitmaps, index, struct bitmap_data);

		if (bitmap->type != 0 || bitmap->depth > 1 || bitmap->mipmap_count != 0 || bitmap->format != format ||
			!mcc_import_pixel_bytes(bitmap->format) || bitmap->width <= 0 || bitmap->height <= 0 ||
			bitmap->pixels_size < (long)bitmap->width * bitmap->height * mcc_import_pixel_bytes(bitmap->format))
		{
			return;
		}
		layout.sources[index].width = bitmap->width;
		layout.sources[index].height = bitmap->height;
		layout.sources[index].pixels_offset = bitmap->pixels_offset;
		layout.sources[index].pixels_size = bitmap->pixels_size;
	}
	layout.source_count = (short)group->bitmaps.count;
	/* the disc's bitmaps: "<width>x<height>", by commas (powers of two, as the
	Xbox's swizzled textures are) */
	for (cursor = bitmaps_text; *cursor; )
	{
		real width, height;

		if (target_count >= layout.source_count || !mcc_import_sprite_layout_number(&cursor, &width) ||
			*cursor++ != 'x' || !mcc_import_sprite_layout_number(&cursor, &height) ||
			width < 1.0f || width > 2048.0f || height < 1.0f || height > 2048.0f ||
			((long)width & ((long)width - 1)) || ((long)height & ((long)height - 1)))
		{
			return;
		}
		target_widths[target_count] = (short)width;
		target_heights[target_count] = (short)height;
		target_count++;
		if (*cursor == ',')
			cursor++;
		else if (*cursor)
			return;
	}
	if (!target_count)
		return;
	if (target_count != layout.source_count)
		same = FALSE;
	for (index = 0; index < target_count; index++)
	{
		if (target_widths[index] != layout.sources[index].width || target_heights[index] != layout.sources[index].height)
			same = FALSE;
	}
	/* the disc's sprites: the sequences by "|", their sprites by ";", each
	"<bitmap>,<left>,<top>,<right>,<bottom>,<registration x>,<registration y>"
	in pixels; as many as MCC's, sequence by sequence */
	cursor = sequences_text;
	for (sequence_index = 0; sequence_index < group->sequences.count; sequence_index++)
	{
		struct bitmap_group_sequence const *sequence =
			TAG_BLOCK_GET_ELEMENT(&group->sequences, sequence_index, struct bitmap_group_sequence);
		long sprite_index;

		if (sequence_index > 0 && *cursor++ != '|')
			return;
		for (sprite_index = 0; sprite_index < sequence->sprites.count; sprite_index++)
		{
			struct bitmap_group_sprite const *sprite =
				TAG_BLOCK_GET_ELEMENT(&sequence->sprites, sprite_index, struct bitmap_group_sprite);
			real values[7];
			short value_index, target;

			if (layout.sprite_count >= MCC_IMPORT_MAXIMUM_LAYOUT_SPRITES || (sprite_index > 0 && *cursor++ != ';'))
				return;
			for (value_index = 0; value_index < 7; value_index++)
			{
				if ((value_index > 0 && *cursor++ != ',') || !mcc_import_sprite_layout_number(&cursor, &values[value_index]))
					return;
			}
			target = (short)values[0];
			if (values[0] != (real)target || target < 0 || target >= target_count ||
				!(values[1] >= 0.0f && values[1] < values[3] && values[3] <= (real)target_widths[target] + 0.01f) ||
				!(values[2] >= 0.0f && values[2] < values[4] && values[4] <= (real)target_heights[target] + 0.01f))
			{
				return;
			}
			/* (MCC's, in its bitmap) */
			if (sprite->bitmap_index < 0 || sprite->bitmap_index >= layout.source_count ||
				!(sprite->bounds.x0 >= 0.0f && sprite->bounds.x0 < sprite->bounds.x1 && sprite->bounds.x1 <= 1.0f) ||
				!(sprite->bounds.y0 >= 0.0f && sprite->bounds.y0 < sprite->bounds.y1 && sprite->bounds.y1 <= 1.0f))
			{
				return;
			}
			layout.sprites[layout.sprite_count].source = sprite->bitmap_index;
			layout.sprites[layout.sprite_count].target = target;
			layout.sprites[layout.sprite_count].source_rectangle[0] =
				sprite->bounds.x0 * layout.sources[sprite->bitmap_index].width;
			layout.sprites[layout.sprite_count].source_rectangle[1] =
				sprite->bounds.y0 * layout.sources[sprite->bitmap_index].height;
			layout.sprites[layout.sprite_count].source_rectangle[2] =
				sprite->bounds.x1 * layout.sources[sprite->bitmap_index].width;
			layout.sprites[layout.sprite_count].source_rectangle[3] =
				sprite->bounds.y1 * layout.sources[sprite->bitmap_index].height;
			for (value_index = 0; value_index < 4; value_index++)
			{
				long pixel = (long)(values[1 + value_index] + 0.5f);

				layout.sprites[layout.sprite_count].target_rectangle[value_index] = pixel;
				if (sprite->bitmap_index != target ||
					(long)(layout.sprites[layout.sprite_count].source_rectangle[value_index] + 0.5f) != pixel)
				{
					same = FALSE;
				}
			}
			target_registrations[layout.sprite_count][0] = values[5] / (real)target_widths[target];
			target_registrations[layout.sprite_count][1] = values[6] / (real)target_heights[target];
			ratios[ratio_count++] = (layout.sprites[layout.sprite_count].source_rectangle[2] -
				layout.sprites[layout.sprite_count].source_rectangle[0]) / (values[3] - values[1]);
			ratios[ratio_count++] = (layout.sprites[layout.sprite_count].source_rectangle[3] -
				layout.sprites[layout.sprite_count].source_rectangle[1]) / (values[4] - values[2]);
			layout.sprite_count++;
		}
	}
	if (*cursor || same || !layout.sprite_count)
		return;
	/* (the median of the widths' and heights' ratios: MCC drew some a little
	larger or smaller) */
	{
		short i, j;

		for (i = 1; i < ratio_count; i++)
		{
			for (j = i; j > 0 && ratios[j - 1] > ratios[j]; j--)
			{
				real swap = ratios[j];

				ratios[j] = ratios[j - 1];
				ratios[j - 1] = swap;
			}
		}
		layout.ratio = ratio_count % 2 ? ratios[ratio_count / 2] :
			0.5f * (ratios[ratio_count / 2 - 1] + ratios[ratio_count / 2]);
	}
	if (!(layout.ratio > 0.0f))
		return;

	/* the tag given the disc's layout */
	group->bitmaps.count = target_count;
	for (index = 0; index < target_count; index++)
	{
		struct bitmap_data *bitmap = TAG_BLOCK_GET_ELEMENT(&group->bitmaps, index, struct bitmap_data);

		bitmap->width = target_widths[index];
		bitmap->height = target_heights[index];
		bitmap->registration_point.x = 0;
		bitmap->registration_point.y = 0;
		bitmap->pixels_size = (long)bitmap->width * bitmap->height * mcc_import_pixel_bytes(bitmap->format);
	}
	index = 0;
	for (sequence_index = 0; sequence_index < group->sequences.count; sequence_index++)
	{
		struct bitmap_group_sequence *sequence =
			TAG_BLOCK_GET_ELEMENT(&group->sequences, sequence_index, struct bitmap_group_sequence);
		long sprite_index;

		for (sprite_index = 0; sprite_index < sequence->sprites.count; sprite_index++, index++)
		{
			struct bitmap_group_sprite *sprite =
				TAG_BLOCK_GET_ELEMENT(&sequence->sprites, sprite_index, struct bitmap_group_sprite);
			short target = layout.sprites[index].target;

			sprite->bitmap_index = target;
			sprite->bounds.x0 = (real)layout.sprites[index].target_rectangle[0] / (real)target_widths[target];
			sprite->bounds.y0 = (real)layout.sprites[index].target_rectangle[1] / (real)target_heights[target];
			sprite->bounds.x1 = (real)layout.sprites[index].target_rectangle[2] / (real)target_widths[target];
			sprite->bounds.y1 = (real)layout.sprites[index].target_rectangle[3] / (real)target_heights[target];
			sprite->registration_point.x = target_registrations[index][0];
			sprite->registration_point.y = target_registrations[index][1];
		}
	}
	/* (the disc's meters have no half HUD scale, which only the digits read) */
	group->flags &= ~FLAG(V5_BITMAP_GROUP_HALF_HUD_SCALE_BIT);
	mcc_import_globals.sprite_layouts[mcc_import_globals.sprite_layout_count++] = layout;
	if (getenv("HALO_MCC_IMPORT_TRACE"))
	{
		fprintf(stderr, "mcc import: hud %s: written in the disc's layout, %d sprites, MCC's %g times as large\n",
			instance->name, (int)layout.sprite_count, (double)layout.ratio);
	}

	return;
}

/* The pixels (linear, as MCC's bitmaps.map holds them) of the bitmap
`bitmap_index` of a sheet written in the disc's layout: each of its sprites
MCC's, resampled into the disc's rectangle, each pixel the average of what
it covers, its colour weighted by its alpha (as mcc_import_bitmap_halve
does); clear elsewhere. FALSE if MCC's pixels cannot be read. */
static boolean mcc_import_sprite_layout_pixels(
	struct mcc_import_sprite_layout const *layout,
	struct bitmap_data const *bitmap,
	short bitmap_index,
	byte *pixels)
{
	short bytes = mcc_import_pixel_bytes(bitmap->format);
	short source_index;

	csmemset(pixels, 0, (unsigned long)bitmap->width * bitmap->height * bytes);
	for (source_index = 0; source_index < layout->source_count; source_index++)
	{
		long source_width = layout->sources[source_index].width, source_height = layout->sources[source_index].height;
		byte *source = NULL;
		short sprite_index;

		for (sprite_index = 0; sprite_index < layout->sprite_count; sprite_index++)
		{
			real const *from = layout->sprites[sprite_index].source_rectangle;
			long const *to = layout->sprites[sprite_index].target_rectangle;
			real step_x, step_y;
			long x, y;

			if (layout->sprites[sprite_index].source != source_index ||
				layout->sprites[sprite_index].target != bitmap_index)
			{
				continue;
			}
			if (!source)
			{
				source = mcc_import_calloc(1, (size_t)layout->sources[source_index].pixels_size);
				if (!source)
					return FALSE;
				custom_edition_cache_read(layout->tag_index, layout->sources[source_index].pixels_offset,
					layout->sources[source_index].pixels_size, source);
			}
			step_x = (from[2] - from[0]) / (real)(to[2] - to[0]);
			step_y = (from[3] - from[1]) / (real)(to[3] - to[1]);
			for (y = to[1]; y < to[3] && y < bitmap->height; y++)
			{
				real top = from[1] + (real)(y - to[1]) * step_y, bottom = top + step_y;

				for (x = to[0]; x < to[2] && x < bitmap->width; x++)
				{
					real left = from[0] + (real)(x - to[0]) * step_x, right = left + step_x;
					real weights = 0.0f, alpha_sum = 0.0f, sums[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
					real weighted[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
					byte *out = pixels + (y * bitmap->width + x) * bytes;
					short alpha_byte = bytes - 1, channel;
					long sx, sy;

					for (sy = (long)top; sy < source_height && (real)sy < bottom; sy++)
					{
						real cover_y = MIN(bottom, (real)(sy + 1)) - MAX(top, (real)sy);

						for (sx = (long)left; sx < source_width && (real)sx < right; sx++)
						{
							real weight = cover_y * (MIN(right, (real)(sx + 1)) - MAX(left, (real)sx));
							byte const *in = source + (sy * source_width + sx) * bytes;

							if (weight <= 0.0f)
								continue;
							weights += weight;
							alpha_sum += weight * in[alpha_byte];
							for (channel = 0; channel < bytes; channel++)
							{
								sums[channel] += weight * in[channel];
								weighted[channel] += weight * in[channel] * in[alpha_byte];
							}
						}
					}
					if (weights <= 0.0f)
						continue;
					for (channel = 0; channel < bytes; channel++)
					{
						real value = channel == alpha_byte || bytes == 1 ? sums[channel] / weights :
							alpha_sum > 0.0f ? weighted[channel] / alpha_sum : sums[channel] / weights;

						out[channel] = (byte)MIN(255.0f, value + 0.5f);
					}
				}
			}
		}
		mcc_import_free(source);
	}

	return TRUE;
}

/* (struct custom_edition_hud_walk's) a block's elements, in the loaded tags */
static uint8_t *mcc_import_hud_block_elements(
	void *context,
	uint8_t const *block,
	uint32_t element_bytes,
	int32_t *count)
{
	struct tag_block const *tag_block = (struct tag_block const *)block;

	(void)context;
	(void)element_bytes;
	*count = tag_block->count > 0 && tag_block->address ? tag_block->count : 0;

	return *count ? tag_block->address : NULL;
}

/* the bitmap references HUD placements draw (pass 0), or those placements'
scales doubled where their bitmap is halved (pass 1), or, the disc's
placements, scaled for their bitmap as written (mcc_import_hud) */
struct mcc_import_hud_context
{
	short pass;
	/* (pass 1) whether the interface walked has the disc's placements */
	boolean disc_values;
	void const **references;
	long reference_count;
	long maximum_references;
};

static void mcc_import_hud_placement(
	void *context,
	uint8_t *placement,
	uint8_t const *bitmap,
	int items)
{
	struct mcc_import_hud_context *hud = context;
	struct tag_reference const *reference = (struct tag_reference const *)bitmap;
	long tag_index;

	(void)items;
	if (!reference || reference->index == NONE)
		return;
	tag_index = reference->index & 0xFFFF;
	if (tag_index >= mcc_import_globals.tag_count)
		return;
	if (hud->pass == 0)
	{
		if (hud->reference_count < hud->maximum_references)
			hud->references[hud->reference_count++] = reference;
	}
	else
	{
		/* (MCC's own placements, of an interface the disc has not, drawing a
		sheet written in the disc's layout: by how much smaller its sprites
		are written) */
		float factor = hud->disc_values ? mcc_import_globals.hud_scale[tag_index] :
			(float)mcc_import_globals.hud_factor[tag_index] * mcc_import_sprite_layout_ratio(tag_index);
		float scale;

		if (factor == 1.0f)
			return;
		csmemcpy(&scale, placement + 4, sizeof(scale));
		scale *= factor;
		csmemcpy(placement + 4, &scale, sizeof(scale));
		csmemcpy(&scale, placement + 8, sizeof(scale));
		scale *= factor;
		csmemcpy(placement + 8, &scale, sizeof(scale));
	}

	return;
}

static void mcc_import_hud_walk(
	struct mcc_import_hud_context *hud)
{
	static unsigned long const groups[] = { 'unhi', 'wphi', 'grhi', 'hudg' };
	struct custom_edition_hud_walk walk;
	short group_index;
	long tag_index;

	walk.block_elements = mcc_import_hud_block_elements;
	walk.placement = mcc_import_hud_placement;
	walk.context = hud;
	for (group_index = 0; group_index < NUMBEROF(groups); group_index++)
	{
		for (tag_index = 0; tag_index < mcc_import_globals.tag_count; tag_index++)
		{
			struct mcc_import_tag_instance *instance = &mcc_import_globals.instances[tag_index];

			if ((unsigned long)instance->group_tag == groups[group_index] && instance->base_address &&
				custom_edition_hud_definition_bytes(groups[group_index]))
			{
				char key[300];

				snprintf(key, sizeof(key), "%c%c%c%c:%s", (char)(groups[group_index] >> 24),
					(char)(groups[group_index] >> 16), (char)(groups[group_index] >> 8), (char)groups[group_index],
					instance->name);
				hud->disc_values = hud->pass == 1 && mcc_import_globals.hud_scale &&
					mcc_import_operations_hud_values(key);
				custom_edition_hud_placements_walk(instance->base_address, groups[group_index], &walk);
			}
		}
	}

	return;
}

/* marks every bitmap a reference names that HUD placements do not draw
through (`hud` lists those they do) */
static void mcc_import_hud_others(
	struct mcc_import_hud_context const *hud,
	struct tag_schema_definition const *definition,
	byte const *element,
	boolean *others,
	boolean *candidates)
{
	struct tag_schema_field const *field;

	for (field = definition->fields; field->type != _tag_schema_terminator; field++)
	{
		short index;

		for (index = 0; index < field->count; index++)
		{
			byte const *at = element + field->offset + index * field->size;

			if (field->type == _tag_schema_reference)
			{
				struct tag_reference const *reference = (struct tag_reference const *)at;
				long listed;

				if (reference->index == NONE || reference->group_tag != 'bitm' ||
					(reference->index & 0xFFFF) >= mcc_import_globals.tag_count)
				{
					continue;
				}
				/* (the message icons and the scopes' masks, which the game
				draws at half MCC's size, an imported map's too:
				hud_messaging.c, rasterizer_xbox_screen_effect.c,
				mcc_imported_hud_loaded) */
				if (field->name && (!strcmp(field->name, "messaging.messaging_icons") ||
					!strcmp(field->name, "mask_fullscreen") || !strcmp(field->name, "mask_splitscreen")))
				{
					candidates[reference->index & 0xFFFF] = !strcmp(field->name, "messaging.messaging_icons") ?
						MCC_IMPORT_HUD_ICONS : MCC_IMPORT_HUD_MASK;
					continue;
				}
				for (listed = 0; listed < hud->reference_count; listed++)
				{
					if (hud->references[listed] == (void const *)reference)
						break;
				}
				if (listed == hud->reference_count)
					others[reference->index & 0xFFFF] = TRUE;
			}
			else if (field->type == _tag_schema_struct)
			{
				mcc_import_hud_others(hud, field->definition, at, others, candidates);
			}
			else if (field->type == _tag_schema_block)
			{
				struct tag_block const *block = (struct tag_block const *)at;
				struct tag_schema_definition const *element_definition = field->definition;
				long element_index;

				for (element_index = 0; block->address && element_index < block->count; element_index++)
				{
					mcc_import_hud_others(hud, element_definition,
						(byte const *)block->address + element_index * element_definition->size, others, candidates);
				}
			}
		}
	}

	return;
}

/* how much smaller the disc's sprites of a sprite sheet are than those it is
written with, at 1/factor of MCC's size (the median over its sequences, of
their widths and heights: MCC drew some again, a little larger or smaller),
from the disc's sizes `sizes` (mcc_import_operations_sprite_sizes); 0 when
none can be compared */
static real mcc_import_sprite_scale(
	struct bitmap_group const *group,
	short factor,
	char const *sizes)
{
	real ratios[128];
	short ratio_count = 0;
	long sequence_index;
	char const *cursor = sizes;

	for (sequence_index = 0; cursor && *cursor && sequence_index < group->sequences.count &&
		ratio_count + 2 <= NUMBEROF(ratios); sequence_index++)
	{
		struct bitmap_group_sequence const *sequence =
			TAG_BLOCK_GET_ELEMENT(&group->sequences, sequence_index, struct bitmap_group_sequence);
		char *end;
		long disc_width = strtol(cursor, &end, 10), disc_height = *end == 'x' ? strtol(end + 1, &end, 10) : 0;

		cursor = *end == ',' ? end + 1 : NULL;
		if (disc_width > 0 && disc_height > 0 && sequence->sprites.count > 0)
		{
			struct bitmap_group_sprite const *sprite =
				TAG_BLOCK_GET_ELEMENT(&sequence->sprites, 0, struct bitmap_group_sprite);
			struct bitmap_data const *bitmap = sprite->bitmap_index >= 0 && sprite->bitmap_index < group->bitmaps.count ?
				TAG_BLOCK_GET_ELEMENT(&group->bitmaps, sprite->bitmap_index, struct bitmap_data) : NULL;
			real width = bitmap ? (sprite->bounds.x1 - sprite->bounds.x0) * bitmap->width / factor : 0.0f;
			real height = bitmap ? (sprite->bounds.y1 - sprite->bounds.y0) * bitmap->height / factor : 0.0f;

			if (width >= 1.0f && height >= 1.0f)
			{
				ratios[ratio_count++] = (real)disc_width / width;
				ratios[ratio_count++] = (real)disc_height / height;
			}
		}
	}
	if (!ratio_count)
		return 0.0f;
	{
		short i, j;

		for (i = 1; i < ratio_count; i++)
		{
			for (j = i; j > 0 && ratios[j - 1] > ratios[j]; j--)
			{
				real swap = ratios[j];

				ratios[j] = ratios[j - 1];
				ratios[j - 1] = swap;
			}
		}
	}

	return ratio_count % 2 ? ratios[ratio_count / 2] : 0.5f * (ratios[ratio_count / 2 - 1] + ratios[ratio_count / 2]);
}

/* MCC's HUD bitmaps are four times the Xbox's, and its placements draw them
at a half or a quarter (mcc_import_hud_convert, custom_edition_cache_convert): a
bitmap drawn through placements alone is written at half its size, and its
placements' scales doubled, so that it draws at the same size in a quarter
of the room (the multiplayer maps' cache files hold no more). */
static void mcc_import_hud(
	void)
{
	struct mcc_import_hud_context hud;
	boolean *others, *candidates;
	long tag_index;

	/* (HALO_MCC_IMPORT_NO_HUD: the HUD's bitmaps as MCC has them, to compare) */
	if (getenv("HALO_MCC_IMPORT_NO_HUD"))
		return;
	mcc_import_globals.hud_factor = mcc_import_calloc((size_t)mcc_import_globals.tag_count, sizeof(short));
	mcc_import_globals.icon_factor = 1;
	mcc_import_globals.mask_factor = 1;
	others = mcc_import_calloc((size_t)mcc_import_globals.tag_count, sizeof(boolean));
	candidates = mcc_import_calloc((size_t)mcc_import_globals.tag_count, sizeof(boolean));
	csmemset(&hud, 0, sizeof(hud));
	hud.maximum_references = 8192;
	hud.references = mcc_import_calloc((size_t)hud.maximum_references, sizeof(void const *));
	if (!mcc_import_globals.hud_factor || !others || !candidates || !hud.references)
	{
		mcc_import_free(others);
		mcc_import_free(candidates);
		mcc_import_free((void *)hud.references);
		return;
	}
	mcc_import_hud_walk(&hud);
	for (tag_index = 0; tag_index < mcc_import_globals.tag_count; tag_index++)
	{
		struct mcc_import_tag_instance const *instance = &mcc_import_globals.instances[tag_index];
		struct tag_schema_group const *schema = mcc_import_schema((unsigned long)instance->group_tag);

		if (instance->base_address && schema && schema->definition)
			mcc_import_hud_others(&hud, schema->definition, instance->base_address, others, candidates);
	}
	for (tag_index = 0; tag_index < hud.reference_count; tag_index++)
	{
		long bitmap_tag = ((struct tag_reference const *)hud.references[tag_index])->index & 0xFFFF;

		if (!candidates[bitmap_tag])
			candidates[bitmap_tag] = MCC_IMPORT_HUD_PLACEMENT;
	}
	/* a sprite sheet only placements draw, in the disc's layout where the
	operations give it, and the disc's placements, which draw it at their
	own scale (mcc_import_sprite_layout_plan) */
	mcc_import_globals.sprite_layout_count = 0;
	for (tag_index = 0; mcc_import_operations_hud_values(NULL) && tag_index < mcc_import_globals.tag_count; tag_index++)
	{
		if (candidates[tag_index] == MCC_IMPORT_HUD_PLACEMENT && !others[tag_index] &&
			mcc_import_globals.instances[tag_index].group_tag == 'bitm')
		{
			mcc_import_sprite_layout_plan(tag_index);
		}
	}
	/* a bitmap the placements (or the icons and masks) draw, and nothing
	else: brought to the disc's size (the operations' bitmap_size), or half
	MCC's where they do not give it, if each of its images can be */
	for (tag_index = 0; tag_index < mcc_import_globals.tag_count; tag_index++)
	{
		struct mcc_import_tag_instance const *instance = &mcc_import_globals.instances[tag_index];
		struct bitmap_group const *group = candidates[tag_index] && instance->group_tag == 'bitm' &&
			!others[tag_index] ? instance->base_address : NULL;
		struct bitmap_data const *first;
		short disc_width, disc_height, factor = 2;
		long bitmap_index;

		if (!group || group->bitmaps.count <= 0)
			continue;
		first = TAG_BLOCK_GET_ELEMENT(&group->bitmaps, 0, struct bitmap_data);
		if (mcc_import_operations_bitmap_size(instance->name, &disc_width, &disc_height))
		{
			factor = first->width % disc_width == 0 ? (short)(first->width / disc_width) : 0;
			if ((factor != 1 && factor != 2 && factor != 4) || first->height != disc_height * factor)
				factor = 1;
		}
		for (bitmap_index = 0; factor > 1 && bitmap_index < group->bitmaps.count; bitmap_index++)
		{
			struct bitmap_data const *bitmap = TAG_BLOCK_GET_ELEMENT(&group->bitmaps, bitmap_index, struct bitmap_data);

			if (!mcc_import_bitmap_halvable(bitmap) || mcc_import_block_bytes(bitmap->format) ||
				bitmap->width % factor || bitmap->height % factor)
				factor = 1;
		}
		mcc_import_globals.hud_factor[tag_index] = factor;
		if (candidates[tag_index] == MCC_IMPORT_HUD_ICONS)
			mcc_import_globals.icon_factor = factor;
		else if (candidates[tag_index] == MCC_IMPORT_HUD_MASK)
			mcc_import_globals.mask_factor = factor;
	}
	/* every other bitmap too, where the operations say (bitmaps_to_disc_size:
	the menus' map, whose MCC's bitmaps of the disc's are larger, to fit its
	cache file) */
	for (tag_index = 0; mcc_import_operations_flag("bitmaps_to_disc_size") && tag_index < mcc_import_globals.tag_count;
		tag_index++)
	{
		struct mcc_import_tag_instance const *instance = &mcc_import_globals.instances[tag_index];
		struct bitmap_group const *group = instance->group_tag == 'bitm' ? instance->base_address : NULL;
		struct bitmap_data const *first;
		short disc_width, disc_height, factor;
		long bitmap_index;

		if (!group || group->bitmaps.count <= 0 || mcc_import_globals.hud_factor[tag_index] > 1 ||
			!mcc_import_operations_bitmap_size(instance->name, &disc_width, &disc_height))
		{
			continue;
		}
		first = TAG_BLOCK_GET_ELEMENT(&group->bitmaps, 0, struct bitmap_data);
		factor = first->width % disc_width == 0 ? (short)(first->width / disc_width) : 0;
		if ((factor != 2 && factor != 4) || first->height != disc_height * factor)
			continue;
		for (bitmap_index = 0; factor > 1 && bitmap_index < group->bitmaps.count; bitmap_index++)
		{
			struct bitmap_data const *bitmap = TAG_BLOCK_GET_ELEMENT(&group->bitmaps, bitmap_index, struct bitmap_data);

			if (!mcc_import_bitmap_halvable(bitmap) || mcc_import_block_bytes(bitmap->format) ||
				bitmap->width % factor || bitmap->height % factor)
				factor = 1;
		}
		mcc_import_globals.hud_factor[tag_index] = factor;
	}
	/* and, where the map came out larger than its cache file holds (a second
	import: mcc_import_run), the largest of the rest halved until it fits: the
	level's and effects' textures (drawn by their coordinates, whatever their
	size), not the HUD's or the menus' (drawn by their pixels) */
	if (mcc_import_globals.fit_budget > 0)
	{
		long budget = mcc_import_globals.fit_budget;

		while (budget > 0)
		{
			long best = NONE;
			unsigned long best_bytes = 0;

			for (tag_index = 0; tag_index < mcc_import_globals.tag_count; tag_index++)
			{
				struct mcc_import_tag_instance const *instance = &mcc_import_globals.instances[tag_index];
				struct bitmap_group const *group = instance->group_tag == 'bitm' ? instance->base_address : NULL;
				unsigned long bytes = 0;
				long bitmap_index;
				boolean halvable = group && group->bitmaps.count > 0 && mcc_import_globals.hud_factor[tag_index] <= 1 &&
					!candidates[tag_index] && strncmp(instance->name, "ui\\", 3);

				for (bitmap_index = 0; halvable && bitmap_index < group->bitmaps.count; bitmap_index++)
				{
					struct bitmap_data const *bitmap = TAG_BLOCK_GET_ELEMENT(&group->bitmaps, bitmap_index, struct bitmap_data);

					halvable = mcc_import_bitmap_halvable(bitmap) && bitmap->width >= 64 && bitmap->height >= 64 &&
						!(bitmap->width % 2) && !(bitmap->height % 2);
					bytes += (unsigned long)bitmap->pixels_size;
				}
				if (halvable && bytes > best_bytes)
				{
					best = tag_index;
					best_bytes = bytes;
				}
			}
			if (best == NONE)
				break;
			mcc_import_globals.hud_factor[best] = 2;
			mcc_import_globals.fitted++;
			budget -= (long)(best_bytes - best_bytes / 4);
		}
	}
	/* the disc's placements: scaled by how much smaller or larger than the
	disc's each bitmap they draw is written (one that cannot be halved), by
	the width of its first image, or a sprite sheet's sprites (MCC drew the
	meters' at twice the disc's size in sheets of its size) */
	if (mcc_import_operations_hud_values(NULL))
		mcc_import_globals.hud_scale = mcc_import_calloc((size_t)mcc_import_globals.tag_count, sizeof(real));
	for (tag_index = 0; mcc_import_globals.hud_scale && tag_index < mcc_import_globals.tag_count; tag_index++)
	{
		struct mcc_import_tag_instance const *instance = &mcc_import_globals.instances[tag_index];
		struct bitmap_group const *group = instance->group_tag == 'bitm' ? instance->base_address : NULL;
		short disc_width, disc_height, factor = mcc_import_globals.hud_factor[tag_index];
		struct bitmap_data const *first;
		char const *sprite_sizes;
		real sprite_scale;

		mcc_import_globals.hud_scale[tag_index] = 1.0f;
		/* (a sheet written in the disc's layout: its sprites the disc's) */
		if (!group || group->bitmaps.count <= 0 || mcc_import_sprite_layout_find(tag_index) ||
			!mcc_import_operations_bitmap_size(instance->name, &disc_width, &disc_height))
		{
			continue;
		}
		first = TAG_BLOCK_GET_ELEMENT(&group->bitmaps, 0, struct bitmap_data);
		/* (a sprite sheet: by its sprites, which MCC may lay out again) */
		sprite_sizes = mcc_import_operations_sprite_sizes(instance->name);
		sprite_scale = sprite_sizes ? mcc_import_sprite_scale(group, factor > 1 ? factor : 1, sprite_sizes) : 0.0f;
		if (sprite_scale > 0.0f)
			mcc_import_globals.hud_scale[tag_index] = sprite_scale;
		else if (first->width > 0)
			mcc_import_globals.hud_scale[tag_index] = (real)disc_width * (real)(factor > 1 ? factor : 1) / (real)first->width;
		if (getenv("HALO_MCC_IMPORT_TRACE"))
			fprintf(stderr, "mcc import: hud %s: written at 1/%d, placements scaled %g\n", instance->name,
				(int)(factor > 1 ? factor : 1), (double)mcc_import_globals.hud_scale[tag_index]);
	}
	hud.pass = 1;
	mcc_import_hud_walk(&hud);
	mcc_import_free(others);
	mcc_import_free(candidates);
	mcc_import_free((void *)hud.references);

	return;
}

/* ---------- private code: the bitmaps */

/* every bitmap's pixels as the Xbox's caches hold them, written to the body;
its tag then names them there */
static boolean mcc_import_bitmaps(
	struct mcc_import_body *body)
{
	long tag_index;

	for (tag_index = 0; tag_index < mcc_import_globals.tag_count; tag_index++)
	{
		struct mcc_import_tag_instance *instance = &mcc_import_globals.instances[tag_index];
		struct bitmap_group *group;
		long bitmap_index;

		if (instance->group_tag != 'bitm' || !instance->base_address ||
			mcc_import_globals.old_to_new[tag_index] == NONE)
		{
			continue;
		}
		group = (struct bitmap_group *)instance->base_address;
		for (bitmap_index = 0; bitmap_index < group->bitmaps.count; bitmap_index++)
		{
			struct bitmap_data *bitmap = TAG_BLOCK_GET_ELEMENT(&group->bitmaps, bitmap_index, struct bitmap_data);
			long xbox_size = rasterizer_xbox_bitmap_get_pixel_data_size(bitmap);
			long buffer_size = MAX(xbox_size, bitmap->pixels_size);
			byte *pixels;

			if (bitmap->pixels_size <= 0 || buffer_size <= 0)
				continue;
			pixels = mcc_import_calloc(1, (size_t)buffer_size);
			if (!pixels)
				return FALSE;
			/* (a HUD sprite sheet in the disc's layout: MCC's sprites moved
			into it, mcc_import_sprite_layout_plan) */
			if (mcc_import_sprite_layout_find(tag_index))
			{
				if (!mcc_import_sprite_layout_pixels(mcc_import_sprite_layout_find(tag_index), bitmap,
					(short)bitmap_index, pixels))
				{
					mcc_import_free(pixels);
					return FALSE;
				}
			}
			else
				custom_edition_cache_read(tag_index, bitmap->pixels_offset, bitmap->pixels_size, pixels);
			/* a HUD bitmap at the disc's size (mcc_import_hud) */
			if (mcc_import_globals.hud_factor && mcc_import_globals.hud_factor[tag_index] > 1)
			{
				short factor;

				for (factor = mcc_import_globals.hud_factor[tag_index]; factor > 1; factor /= 2)
					mcc_import_bitmap_halve(bitmap, pixels);
				xbox_size = rasterizer_xbox_bitmap_get_pixel_data_size(bitmap);
				mcc_import_globals.hud_bitmaps_halved++;
			}
			bitmap->base_address = pixels;
			rasterizer_xbox_bitmap_rebuild_hardware_format(bitmap);
			mcc_import_body_align(body, 0x200);
			bitmap->pixels_offset = (long)mcc_import_body_file_offset(body);
			bitmap->pixels_size = xbox_size;
			mcc_import_body_write(body, pixels, (unsigned long)xbox_size);
			mcc_import_free(pixels);
			bitmap->base_address = NULL;
			bitmap->hardware_format = NULL;
			bitmap->cache_block_index = NONE;
			/* (its own: the copy gives it the written tag's index) */
			bitmap->tag_index = instance->tag_index;
			bitmap->flags |= FLAG(V5_BITMAP_CACHED_BIT);
			mcc_import_globals.bitmaps++;
		}
	}

	return !body->failed;
}

/* ---------- private code: the tags */

/* the models' parts in the copy, given the Xbox's buffers: their
compressed vertices and strips copied to the vertex and index data, the
arrays' entries naming them */
static boolean mcc_import_model_buffers(
	struct mcc_import_space *space,
	struct mcc_import_model_part **parts,
	long part_count)
{
	unsigned long vertex_array = mcc_import_allocate(space, (unsigned long)part_count * V5_BUFFER_ENTRY_BYTES, 4);
	unsigned long index_array = mcc_import_allocate(space, (unsigned long)part_count * V5_BUFFER_ENTRY_BYTES, 4);
	struct mcc_import_tag_header *header = (struct mcc_import_tag_header *)space->bytes;
	long part_index;

	if ((part_count && (!vertex_array || !index_array)))
		return FALSE;
	for (part_index = 0; part_index < part_count; part_index++)
	{
		struct mcc_import_model_part *part = parts[part_index];
		long vertex_bytes = part->vertex_buffer.count *
			rasterizer_geometry_get_vertex_size(_rasterizer_vertex_type_model_compressed);
		long index_bytes = (part->triangle_buffer.count + 2) * (long)sizeof(word);
		unsigned long vertices = mcc_import_allocate(space, (unsigned long)vertex_bytes, 32);
		unsigned long indices = mcc_import_allocate(space, (unsigned long)index_bytes, 4);
		struct mcc_import_buffer_entry *vertex_entry =
			(struct mcc_import_buffer_entry *)mcc_import_at(space, vertex_array) + part_index;
		struct mcc_import_buffer_entry *index_entry =
			(struct mcc_import_buffer_entry *)mcc_import_at(space, index_array) + part_index;

		if (!vertices || !indices || !part->vertex_buffer.base_address || !part->triangle_buffer.base_address)
		{
			mcc_import_error("mcc import: model part %ld: %ld vertices at %p, %ld triangles at %p", part_index,
				part->vertex_buffer.count, part->vertex_buffer.base_address, part->triangle_buffer.count,
				part->triangle_buffer.base_address);
			return FALSE;
		}
		csmemcpy(mcc_import_at(space, vertices), part->vertex_buffer.base_address, vertex_bytes);
		csmemcpy(mcc_import_at(space, indices), part->triangle_buffer.base_address, index_bytes);
		vertex_entry->data = vertices;
		index_entry->data = indices;
		part->vertex_buffer.base_address = (void *)vertices;
		part->vertex_buffer.hardware_format = (void *)(vertex_array + part_index * V5_BUFFER_ENTRY_BYTES);
		part->triangle_buffer.base_address = (void *)indices;
		part->triangle_buffer.hardware_format = (void *)(index_array + part_index * V5_BUFFER_ENTRY_BYTES);
	}
	header->vertex_buffer_count = part_count;
	header->vertex_buffers = (void *)vertex_array;
	header->index_buffer_count = part_count;
	header->index_buffers = (void *)index_array;

	return TRUE;
}

/* the copied models' parts (their buffers still name the loaded geometry) */
static long mcc_import_copied_model_parts(
	struct mcc_import_space *space,
	struct mcc_import_model_part **parts,
	long maximum)
{
	struct mcc_import_tag_header const *header = (struct mcc_import_tag_header const *)space->bytes;
	struct mcc_import_tag_instance const *instances = mcc_import_at(space, (unsigned long)header->tag_instances);
	long tag_index, count = 0;

	for (tag_index = 0; tag_index < header->tag_count; tag_index++)
	{
		struct model const *model;
		long geometry_index;

		if (instances[tag_index].group_tag != 'mode' || !instances[tag_index].base_address)
			continue;
		model = mcc_import_at(space, (unsigned long)instances[tag_index].base_address);
		for (geometry_index = 0; geometry_index < model->geometries.count; geometry_index++)
		{
			struct mcc_import_model_geometry const *geometry = (struct mcc_import_model_geometry const *)
				mcc_import_at(space, (unsigned long)model->geometries.address) + geometry_index;
			long part_index;

			for (part_index = 0; part_index < geometry->parts.count && geometry->parts.address; part_index++)
			{
				if (count < maximum)
				{
					parts[count] = (struct mcc_import_model_part *)
						mcc_import_at(space, (unsigned long)geometry->parts.address) + part_index;
				}
				count++;
			}
		}
	}

	return count;
}

/* The Xbox's tag cache (`space`, at 0x803A6000) begun: its header, the
instances and their names, which references name (the structure BSPs' too,
written first) */
static unsigned long mcc_import_names(
	struct mcc_import_space *space,
	struct mcc_import_tag_header const *loaded)
{
	struct mcc_import_tag_instance *instances;
	unsigned long instances_address;
	long tag_index;

	long count = mcc_import_globals.out_count;

	(void)loaded;
	mcc_import_allocate(space, sizeof(struct mcc_import_tag_header), 4);
	instances_address = mcc_import_allocate(space, (unsigned long)count * sizeof(*instances), 4);
	if (!instances_address)
		return 0;
	instances = mcc_import_at(space, instances_address);
	mcc_import_globals.empty_name_address = mcc_import_allocate(space, 1, 1);
	mcc_import_globals.name_addresses = mcc_import_calloc((size_t)count, sizeof(unsigned long));
	if (!mcc_import_globals.name_addresses)
		return 0;
	for (tag_index = 0; tag_index < count; tag_index++)
	{
		long source_index = mcc_import_globals.out_sources[tag_index];
		char const *source_name = mcc_import_globals.out_names[tag_index];
		unsigned long length = (unsigned long)strlen(source_name) + 1;
		unsigned long name = mcc_import_allocate(space, length, 1);
		struct tag_schema_group const *schema = mcc_import_schema(mcc_import_globals.out_groups[tag_index]);

		if (!name)
			return 0;
		csmemcpy(mcc_import_at(space, name), source_name, length);
		mcc_import_globals.name_addresses[tag_index] = name;
		if (source_index != NONE)
		{
			instances[tag_index] = mcc_import_globals.instances[source_index];
		}
		else
		{
			/* a tag of the disc's MCC has none of: in its place, empty */
			csmemset(&instances[tag_index], 0, sizeof(instances[tag_index]));
			instances[tag_index].group_tag = (long)mcc_import_globals.out_groups[tag_index];
			instances[tag_index].parent_group_tags[0] = schema ? (long)schema->parent_group_tags[0] : NONE;
			instances[tag_index].parent_group_tags[1] = schema ? (long)schema->parent_group_tags[1] : NONE;
		}
		instances[tag_index].tag_index = mcc_import_new_handle(tag_index);
		instances[tag_index].name = (char *)name;
		instances[tag_index].base_address = NULL;
		instances[tag_index].unused[0] = 0;
		instances[tag_index].unused[1] = 0;
	}
	((struct mcc_import_tag_header *)space->bytes)->tag_instances = (struct mcc_import_tag_instance *)instances_address;
	((struct mcc_import_tag_header *)space->bytes)->tag_count = count;

	return instances_address;
}

/* The scenario's script nodes that are tags (sound to object definition,
_hs_type_sound.._hs_type_object_definition) given the written tags'
indices: the syntax data is copied as bytes. */
static boolean mcc_import_scripts_renumber(
	struct mcc_import_space *space)
{
	struct mcc_import_tag_header const *header = (struct mcc_import_tag_header const *)space->bytes;
	struct mcc_import_tag_instance const *instances = mcc_import_at(space, (unsigned long)header->tag_instances);
	long tag_index;

	for (tag_index = 0; tag_index < header->tag_count; tag_index++)
	{
		byte *scenario;
		struct tag_data const *syntax_data;
		byte *syntax;
		short node_count, node_index;

		if (instances[tag_index].group_tag != 'scnr' || !instances[tag_index].base_address)
			continue;
		scenario = mcc_import_at(space, (unsigned long)instances[tag_index].base_address);
		syntax_data = (struct tag_data const *)(scenario + V5_SCENARIO_SCRIPT_SYNTAX_DATA_OFFSET);
		if (syntax_data->size < V5_SCRIPT_NODES_OFFSET || !syntax_data->address)
			continue;
		syntax = mcc_import_at(space, (unsigned long)syntax_data->address);
		/* (the node array's last index, its data array's) */
		node_count = *(short *)(syntax + V5_SCRIPT_NODE_COUNT_OFFSET);
		if (V5_SCRIPT_NODES_OFFSET + node_count * V5_SCRIPT_NODE_BYTES > syntax_data->size)
		{
			mcc_import_error("mcc import: the script nodes (%d) do not fit their data", (int)node_count);
			return FALSE;
		}
		for (node_index = 0; node_index < node_count; node_index++)
		{
			byte *node = syntax + V5_SCRIPT_NODES_OFFSET + node_index * V5_SCRIPT_NODE_BYTES;
			short type = *(short *)(node + 4);
			short flags = *(short *)(node + 6);

			if (*(short *)node && (flags & 1) && type >= _hs_type_sound && type <= _hs_type_object_definition)
				*(long *)(node + 0x10) = mcc_import_renumber(*(long *)(node + 0x10));
		}
	}

	return TRUE;
}

/* The tags into the Xbox's tag cache (`space`, at 0x803A6000): its header,
the instances and their names, the models' buffers, then every tag by its
schema. */
static boolean mcc_import_tags(
	struct mcc_import_space *space,
	struct mcc_import_tag_header const *loaded,
	unsigned long instances_address)
{
	struct mcc_import_tag_header *header;
	struct mcc_import_tag_instance *instances;
	struct mcc_import_model_part **parts;
	long tag_index, part_count, copied_count;
	boolean success = TRUE;

	/* the tags, each by its schema, in the order written (a structure BSP's
	is in the body) */
	for (tag_index = 0; tag_index < mcc_import_globals.out_count && success; tag_index++)
	{
		struct mcc_import_tag_instance const *source;
		struct tag_schema_group const *schema;
		unsigned long root;

		if (mcc_import_globals.out_sources[tag_index] == NONE)
		{
			/* a tag of the disc's MCC has none of: zeros of its group's size
			in its place, for the code that asks for it by name (a stand-in,
			until the import makes or copies the tag) */
			unsigned long group = mcc_import_globals.out_groups[tag_index];

			schema = mcc_import_schema(group);
			if (group != 'sbsp')
			{
				root = mcc_import_allocate(space, schema && schema->definition ?
					(unsigned long)schema->definition->size : 0x100, 4);
				if (!root)
					return FALSE;
				if (schema && schema->definition)
					mcc_import_empty_references(schema->definition, mcc_import_at(space, root));
				instances = mcc_import_at(space, instances_address);
				instances[tag_index].base_address = (void *)root;
			}
			continue;
		}
		source = &mcc_import_globals.instances[mcc_import_globals.out_sources[tag_index]];
		if (source->group_tag == 'sbsp' || !source->base_address)
			continue;
		schema = mcc_import_schema((unsigned long)source->group_tag);
		if (!schema || !schema->definition)
		{
			mcc_import_error("mcc import: '%s' has no schema: left out", source->name);
			mcc_import_globals.schemaless++;
			continue;
		}
		root = mcc_import_allocate(space, (unsigned long)schema->definition->size, 4);
		if (!root)
			return FALSE;
		mcc_import_globals.copying = source->name;
		mcc_import_copy_element(space, schema->definition, source->base_address, mcc_import_at(space, root));
		mcc_import_globals.copying = NULL;
		instances = mcc_import_at(space, instances_address);
		instances[tag_index].base_address = (void *)root;
		success = !space->overflowed;
	}
	if (!success)
	{
		mcc_import_error("mcc import: the tags do not fit the tag cache (0x%lX bytes)", space->used);
		return FALSE;
	}

	/* the models' buffers, after the tags (the game looks them up by the
	header's arrays and the parts' fields) */
	part_count = mcc_import_copied_model_parts(space, NULL, 0);
	parts = mcc_import_calloc((size_t)part_count + 1, sizeof(*parts));
	if (!parts)
		return FALSE;
	copied_count = mcc_import_copied_model_parts(space, parts, part_count);
	success = copied_count == part_count && mcc_import_model_buffers(space, parts, part_count);
	if (!success)
		mcc_import_error("mcc import: the models' buffers cannot be written (%ld parts)", part_count);
	mcc_import_free(parts);
	success = success && mcc_import_scripts_renumber(space);

	header = (struct mcc_import_tag_header *)space->bytes;
	header->tag_instances = (struct mcc_import_tag_instance *)instances_address;
	header->scenario_tag_index = mcc_import_renumber(loaded->scenario_tag_index);
	header->checksum = loaded->checksum;
	header->tag_count = mcc_import_globals.out_count;
	header->signature = 'tags';

	return success && !space->overflowed;
}

/* ---------- private code: the fingerprint */

/* Whether the written map is the disc's: its fingerprint
(mcc_import_fingerprint.c) the disc map's, every part of it, from <map>.fp
(mcc_import_data_read: the data root's mcc_patches folder's, else
port/assets/mcc's, embedded in the build). Without that file it cannot be
shown. Logs the parts that differ. */
static boolean mcc_import_stock(
	char const *map_name,
	struct mcc_import_space const *tags)
{
	char const *lines = mcc_import_fingerprint(map_name, tags->bytes, tags->used, mcc_import_globals.disc_count,
		mcc_import_globals.bsps, mcc_import_globals.bsp_count);
	char name[80];
	char *expected = NULL, *text;
	char const *line;
	long differing = 0, parts = 0, size;

	snprintf(name, sizeof(name), "%s.fp", map_name);
	text = mcc_import_data_read(name, &size);
	/* (a newline before the first line, so that every line is found between
	two) */
	expected = text && size > 0 ? mcc_import_calloc((size_t)size + 3, 1) : NULL;
	if (expected)
	{
		long from, to = 1;

		expected[0] = '\n';
		/* (without carriage returns: a file written on Windows has them) */
		for (from = 0; from < size; from++)
		{
			if (text[from] != '\r')
				expected[to++] = text[from];
		}
		expected[to] = '\n';
		expected[to + 1] = 0;
	}
	mcc_import_data_free(text);
	if (!expected)
	{
		mcc_import_error("mcc import: %s: no fingerprint of the disc's map (%s): an MCC map", map_name, name);
		return FALSE;
	}
	for (line = lines; *line; )
	{
		char const *end = strchr(line, '\n');
		unsigned long length = end ? (unsigned long)(end - line) : 0;
		char key[700];

		if (!end)
			break;
		if (length + 3 < sizeof(key))
		{
			key[0] = '\n';
			csmemcpy(key + 1, line, length);
			key[length + 1] = '\n';
			key[length + 2] = 0;
			parts++;
			if (!strstr(expected, key))
			{
				if (differing < 12)
					fprintf(stderr, "mcc import: %s: differs from the disc's: %.*s\n", map_name, (int)length, line);
				differing++;
			}
		}
		line = end + 1;
	}
	mcc_import_free(expected);
	fprintf(stdout, "mcc import: %s: %ld of %ld parts of the fingerprint differ from the disc's: %s\n", map_name,
		differing, parts, differing || !parts ? "an MCC map (MCC's version)" : "the disc's map");

	return parts && !differing;
}

/* ---------- private code: a map */

/* the tags `tags` of the MCC map `donor_map`, as its fixes and operations
leave them, written for the maps whose donor lines name them */
static boolean mcc_import_donor_export(
	char const *donor_map,
	char const *const *tags,
	long tag_count,
	char const *for_map)
{
	static byte loaded_header[V5_HEADER_BYTES];
	struct mcc_import_tag_header *loaded;
	boolean success = TRUE;
	long index;

	csmemset(&mcc_import_globals, 0, sizeof(mcc_import_globals));
	loaded = (struct mcc_import_tag_header *)custom_edition_cache_tags_load(donor_map, loaded_header);
	if (!loaded)
	{
		fprintf(stderr, "mcc import: %s: cannot be loaded to give its tags (the log says why)\n", donor_map);
		return FALSE;
	}
	mcc_import_globals.table.maximum_count = loaded->tag_count + 64;
	mcc_import_globals.table.instances = mcc_import_calloc((size_t)mcc_import_globals.table.maximum_count,
		sizeof(struct mcc_import_tag_instance));
	if (mcc_import_globals.table.instances)
	{
		csmemcpy(mcc_import_globals.table.instances, loaded->tag_instances,
			(unsigned long)loaded->tag_count * sizeof(struct mcc_import_tag_instance));
		mcc_import_globals.table.count = loaded->tag_count;
		mcc_import_globals.table.scenario_index = loaded->scenario_tag_index & 0xFFFF;
		mcc_import_fixes_apply(&mcc_import_globals.table);
		if (mcc_import_operations_read(donor_map))
		{
			mcc_import_operations_apply(&mcc_import_globals.table, NULL, NULL);
			mcc_import_models_complete(&mcc_import_globals.table);
		}
		for (index = 0; index < tag_count; index++)
		{
			success = mcc_import_operations_export(&mcc_import_globals.table, donor_map, tags[index], for_map) && success;
			fprintf(stdout, "mcc import: %s: gave %s\n", donor_map, tags[index]);
		}
	}
	else
		success = FALSE;
	mcc_import_free(mcc_import_globals.table.instances);
	mcc_import_operations_dispose();
	custom_edition_cache_tags_unload();

	return success;
}

/* the tags the map's donor lines name (MCC has them in other maps: the
warthog for the multiplayer maps), written first where they are not */
static boolean mcc_import_donors(
	char const *map_name)
{
	enum { MAXIMUM_DONORS = 16 };
	static char donor_maps[MAXIMUM_DONORS][64];
	static char tags[MAXIMUM_DONORS][300];
	char const *missing_maps[MAXIMUM_DONORS], *missing_tags[MAXIMUM_DONORS];
	long count = 0, index;
	boolean success = TRUE;

	if (mcc_import_operations_read(map_name))
	{
		count = mcc_import_operations_donors_missing(missing_maps, missing_tags, MAXIMUM_DONORS);
		for (index = 0; index < count; index++)
		{
			snprintf(donor_maps[index], sizeof(donor_maps[index]), "%s", missing_maps[index]);
			snprintf(tags[index], sizeof(tags[index]), "%s", missing_tags[index]);
		}
	}
	mcc_import_operations_dispose();
	/* (each donor map loaded once, for all the tags it gives) */
	for (index = 0; index < count; index++)
	{
		char const *given[MAXIMUM_DONORS];
		long given_count = 0, other;

		if (!donor_maps[index][0])
			continue;
		for (other = index; other < count; other++)
		{
			if (!strcmp(donor_maps[other], donor_maps[index]))
			{
				given[given_count++] = tags[other];
				if (other != index)
					donor_maps[other][0] = 0;
			}
		}
		success = mcc_import_donor_export(donor_maps[index], given, given_count, map_name) && success;
		donor_maps[index][0] = 0;
	}

	return success;
}

/* the most a map of the scenario type `scenario_type` can be, its cache
file's size (cache_files_windows.c, cached_map_file_get_size) */
static unsigned long mcc_import_cache_file_size(
	short scenario_type)
{
	return scenario_type == _scenario_type_solo ? 0x11600000UL :
		scenario_type == _scenario_type_main_menu ? 0x02300000UL : 0x02F00000UL;
}

/* the map imported into output_folder, its bitmaps giving up fit_budget
bytes (mcc_import_hud); *overage the bytes it is larger than its cache file
holds, if it is */
static boolean mcc_import_map(
	char const *map_name,
	char const *output_folder,
	long fit_budget,
	long *overage)
{
	static byte loaded_header[V5_HEADER_BYTES];
	struct mcc_import_header header;
	struct mcc_import_tag_header *loaded;
	struct mcc_import_body *body;
	struct mcc_import_space tags;
	struct tag_block *references;
	byte *scenario;
	char path[512];
	unsigned long instances_address;
	short bsp_index;
	short scenario_type;
	boolean success = FALSE;
	boolean stock = FALSE;

	if (!mcc_import_donors(map_name))
		mcc_import_error("mcc import: %s: some tags of other maps it takes cannot be written: left out", map_name);
	csmemset(&mcc_import_globals, 0, sizeof(mcc_import_globals));
	mcc_import_globals.fit_budget = fit_budget;
	*overage = 0;
	loaded = (struct mcc_import_tag_header *)custom_edition_cache_tags_load(map_name, loaded_header);
	if (!loaded)
	{
		fprintf(stderr, "mcc import: %s: cannot be loaded (the log says why)\n", map_name);
		return FALSE;
	}
	/* the table copied, then the fixes and the map's operations, which may
	add tags to it */
	mcc_import_globals.table.maximum_count = loaded->tag_count + 64;
	mcc_import_globals.table.instances = mcc_import_calloc((size_t)mcc_import_globals.table.maximum_count,
		sizeof(struct mcc_import_tag_instance));
	if (!mcc_import_globals.table.instances)
	{
		custom_edition_cache_tags_unload();
		return FALSE;
	}
	csmemcpy(mcc_import_globals.table.instances, loaded->tag_instances,
		(unsigned long)loaded->tag_count * sizeof(struct mcc_import_tag_instance));
	mcc_import_globals.table.count = loaded->tag_count;
	mcc_import_globals.table.scenario_index = loaded->scenario_tag_index & 0xFFFF;
	mcc_import_fixes_apply(&mcc_import_globals.table);
	mcc_import_globals.operations = mcc_import_operations_read(map_name);
	if (mcc_import_globals.operations)
	{
		mcc_import_operations_apply(&mcc_import_globals.table, NULL, NULL);
		mcc_import_models_complete(&mcc_import_globals.table);
	}
	else
		mcc_import_error("mcc import: %s: no operations (%s.p2): MCC's values kept", map_name, map_name);
	mcc_import_globals.instances = mcc_import_globals.table.instances;
	mcc_import_globals.tag_count = mcc_import_globals.table.count;
	if (!mcc_import_order(map_name))
	{
		fprintf(stderr, "mcc import: %s: out of memory for the tag order\n", map_name);
		custom_edition_cache_tags_unload();
		return FALSE;
	}
	scenario = mcc_import_globals.instances[loaded->scenario_tag_index & 0xFFFF].base_address;
	scenario_type = *(short *)(scenario + V5_SCENARIO_TYPE_OFFSET);
	references = (struct tag_block *)(scenario + V5_SCENARIO_STRUCTURE_BSPS_OFFSET);

	snprintf(path, sizeof(path), "%s/%s.map", output_folder, map_name);
	body = mcc_import_calloc(1, sizeof(*body));
	tags.bytes = (byte *)V5_TAG_ADDRESS;
	tags.address = V5_TAG_ADDRESS;
	tags.size = V5_TAG_CACHE_BYTES;
	tags.used = 0;
	tags.overflowed = FALSE;
	if (!body || !(body->file = fopen(path, "wb")))
	{
		fprintf(stderr, "mcc import: %s: cannot write %s\n", map_name, path);
		mcc_import_free(body);
		custom_edition_cache_tags_unload();
		return FALSE;
	}
	csmemset(&header, 0, sizeof(header));
	fwrite(&header, 1, sizeof(header), body->file);
	body->crc = crc32(0L, Z_NULL, 0);
	/* (the fastest level: as small within a few percent, in half the time,
	the samples and pixels compressing little at any level) */
	if (deflateInit(&body->stream, Z_BEST_SPEED) != Z_OK)
		body->failed = TRUE;

	instances_address = mcc_import_names(&tags, loaded);
	success = !body->failed && instances_address;
	for (bsp_index = 0; success && bsp_index < references->count; bsp_index++)
	{
		struct scenario_structure_bsp_reference *reference =
			(struct scenario_structure_bsp_reference *)references->address + bsp_index;

		mcc_import_body_align(body, 0x800);
		success = mcc_import_structure_bsp(body, reference, bsp_index);
	}
	if (getenv("HALO_MCC_IMPORT_TRACE"))
		fprintf(stderr, "mcc import: %s: bitmaps (%d)\n", map_name, (int)success);
	mcc_import_hud();
	success = success && mcc_import_bitmaps(body);
	if (success)
	{
		/* the sounds, from MCC's banks */
		mcc_import_globals.sounds = mcc_import_sounds_write(&mcc_import_globals.table, mcc_import_globals.old_to_new,
			mcc_import_body_sound, body);
		success = !body->failed;
	}
	if (getenv("HALO_MCC_IMPORT_TRACE"))
		fprintf(stderr, "mcc import: %s: tags (%d)\n", map_name, (int)success);
	success = success && mcc_import_tags(&tags, loaded, instances_address);
	if (getenv("HALO_MCC_IMPORT_TRACE"))
		fprintf(stderr, "mcc import: %s: checks (%d)\n", map_name, (int)success);
	if (success && !tag_validate_tags(tags.bytes, (long)tags.used, 0x7FFFFFFF, map_name))
	{
		/* (written all the same for now: the game checks them again as it
		loads the map, with its log open to say why) */
		fprintf(stderr, "mcc import: %s: the written tags are refused (the game says why as it loads them)\n", map_name);
	}
	if (success)
	{
		if (getenv("HALO_MCC_IMPORT_TRACE"))
			fprintf(stderr, "mcc import: %s: fingerprint\n", map_name);
		stock = mcc_import_stock(map_name, &tags);
	}
	if (success)
	{
		unsigned long tag_data_offset = (mcc_import_body_align(body, 0x200), mcc_import_body_file_offset(body));

		mcc_import_body_write(body, tags.bytes, tags.used);
		mcc_import_body_align(body, 0x200);
		body->stream.avail_in = 0;
		mcc_import_body_deflate(body, Z_FINISH);
		success = !body->failed;

		header.header_signature = 'head';
		header.version = 5;
		header.file_length = (long)(V5_HEADER_BYTES + body->length);
		header.tag_data_offset = (long)tag_data_offset;
		header.tag_data_size = (long)tags.used;
		strncpy(header.name, map_name, sizeof(header.name) - 1);
		strncpy(header.build, V5_BUILD, sizeof(header.build) - 1);
		header.scenario_type = scenario_type;
		header.checksum = body->crc;
		header.footer_signature = 'foot';
		if ((unsigned long)header.file_length > mcc_import_cache_file_size(scenario_type))
			*overage = (long)((unsigned long)header.file_length - mcc_import_cache_file_size(scenario_type));
		/* MCC's HUD, which is drawn as an MCC map's (mcc_cache_loaded), and
		how much smaller its message icons and scopes' masks are written */
		{
			unsigned long mark = MCC_IMPORTED_HUD_MARK;
			unsigned long icon_factor = (unsigned long)mcc_import_globals.icon_factor;
			unsigned long mask_factor = (unsigned long)mcc_import_globals.mask_factor;

			csmemcpy(header.reserved68 + (MCC_IMPORTED_HUD_MARK_OFFSET - 0x68), &mark, sizeof(mark));
			csmemcpy(header.reserved68 + (MCC_IMPORTED_ICON_FACTOR_OFFSET - 0x68), &icon_factor, sizeof(icon_factor));
			csmemcpy(header.reserved68 + (MCC_IMPORTED_MASK_FACTOR_OFFSET - 0x68), &mask_factor, sizeof(mask_factor));
		}
		/* a map not shown to be the disc's plays on MCC's version
		(mcc_imported_level_name) */
		if (!stock)
		{
			unsigned long mark = MCC_MAP_VERSION;

			csmemcpy(header.reserved68 + (MCC_IMPORTED_MAP_MARK_OFFSET - 0x68), &mark, sizeof(mark));
		}
		{
			/* the deflated body padded to 0x800, as the disc's maps are */
			static byte const zeros[0x800];
			long end = ftell(body->file);

			fwrite(zeros, 1, (size_t)((0x800 - end % 0x800) % 0x800), body->file);
		}
		fseek(body->file, 0, SEEK_SET);
		success = success && fwrite(&header, 1, sizeof(header), body->file) == sizeof(header);
		fprintf(stdout, "mcc import: %s: %d structure bsps, %ld bitmaps, %ld sounds (%ld silent), %ld tags (0x%lX bytes; "
			"%ld empty in the disc's place, %ld of MCC's kept after the disc's, %ld without a schema; %ld references to "
			"tags left out; %ld HUD bitmaps halved, %ld textures halved to fit), 0x%lX bytes before compression\n",
			map_name, (int)references->count, mcc_import_globals.bitmaps, mcc_import_globals.sounds,
			mcc_import_sounds_silent(), mcc_import_globals.out_count, tags.used, mcc_import_globals.placeholders,
			mcc_import_globals.kept, mcc_import_globals.schemaless, mcc_import_globals.references_dropped,
			mcc_import_globals.hud_bitmaps_halved, mcc_import_globals.fitted, (unsigned long)header.file_length);
	}
	deflateEnd(&body->stream);
	/* (a write that failed, as on a full disk, said: nothing else says it;
	the last bytes are written as the file is closed) */
	if (fclose(body->file) != 0 || body->failed)
	{
		mcc_import_error("mcc import: %s: %s cannot be written (is the disk full?)", map_name, path);
		success = FALSE;
	}
	mcc_import_free(body);
	mcc_import_free(mcc_import_globals.name_addresses);
	mcc_import_free(mcc_import_globals.out_sources);
	mcc_import_free(mcc_import_globals.out_groups);
	mcc_import_free(mcc_import_globals.out_names);
	mcc_import_free(mcc_import_globals.old_to_new);
	mcc_import_data_free(mcc_import_globals.out_names_text);
	mcc_import_free(mcc_import_globals.table.instances);
	mcc_import_free(mcc_import_globals.hud_factor);
	mcc_import_free(mcc_import_globals.hud_scale);
	for (bsp_index = 0; bsp_index < mcc_import_globals.bsp_count; bsp_index++)
		mcc_import_free(mcc_import_globals.bsps[bsp_index].bytes);
	mcc_import_operations_dispose();
	mcc_import_sounds_map_done();
	custom_edition_cache_tags_unload();
	if (!success)
	{
		fprintf(stderr, "mcc import: %s: failed\n", map_name);
		remove(path);
	}

	return success;
}

/* ---------- public code */

int mcc_import_run(
	char const *output_folder,
	char const *const *map_names,
	int map_count)
{
	int index, failures = 0;

	/* the Xbox's memory, as main() allocates it after the command line is
	answered: the tags are written into the tag cache at 0x803A6000, where
	tag_validate_tags checks them */
	physical_memory_allocate();
	/* (the messages as they come: a failure must not lose the last) */
	setvbuf(stderr, NULL, _IONBF, 0);
	custom_edition_geometry_import = TRUE;
	mcc_import_sounds_open();
	mcc_sounds_muted_proc = mcc_import_sound_muted;
	for (index = 0; index < map_count; index++)
	{
		long overage = 0, budget = 0;
		short attempt;
		boolean success = mcc_import_map(map_names[index], output_folder, 0, &overage);

		/* (larger than its cache file holds, so that the game cannot load it:
		imported again, its largest textures halved; the room is reckoned
		before compression, as the cache file's) */
		for (attempt = 0; success && overage > 0 && attempt < 3; attempt++)
		{
			budget += overage + 0x10000;
			fprintf(stdout, "mcc import: %s: 0x%lX bytes larger than its cache file: imported again, its largest "
				"textures halved\n", map_names[index], (unsigned long)overage);
			success = mcc_import_map(map_names[index], output_folder, budget, &overage);
		}
		if (success && overage > 0)
			mcc_import_error("mcc import: %s: still 0x%lX bytes larger than its cache file: the game cannot load it",
				map_names[index], (unsigned long)overage);
		if (!success)
			failures++;
		/* (each map's lines as it is done: stdout into a pipe, as the first
		start's progress reads it, is otherwise written only when full) */
		fflush(stdout);
	}
	mcc_sounds_muted_proc = NULL;
	mcc_import_sounds_close();
	custom_edition_geometry_import = FALSE;

	return failures ? 1 : 0;
}
