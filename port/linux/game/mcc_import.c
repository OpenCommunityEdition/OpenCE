/*
MCC_IMPORT.C

halo --import-mcc <output folder> [<map>...]: writes the Master Chief
Collection's Halo 1 maps, in the data root's maps folder, as the Xbox's
cache files (version 5) in the output folder, so that the game loads them as
it loads the disc's maps (.local/step3_writer_design.md; a prototype, not yet
the importer the menus run).

Each map is loaded as the game loads an MCC map to play it
(custom_edition_cache_tags_load: its tags moved and converted, its models'
vertices compressed), with custom_edition_geometry_import set, so that its
geometry is kept for the file instead of being given to the renderer. Then
the map's file is written as the disc's are: the structure BSPs, each made the
Xbox's (moved to the top of the tag cache, a header and buffer arrays before
it); each bitmap's pixels, made the Xbox's by the texture cache's own
rasterizer_xbox_bitmap_rebuild_hardware_format; and the tags last, copied by
their schemas into the Xbox's tag cache at 0x803A6000 with the models' vertex
and index buffers before them, then checked by tag_validate_tags as the game
checks a map's. The body is deflated by the game's own zlib after the
header, as the disc's maps are. Sounds are silent: their samples are in MCC's
sound banks, which are not read yet.
*/

/* ---------- headers */

#include "cseries.h"
#include "cseries/cseries_windows.h"
#include "errors.h"
#include "tag_files/tag_groups.h"
#include "bitmaps/bitmap_group.h"
#include "hs/hs.h"
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
	MCC_IMPORT_MAXIMUM_BSPS = 32,
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
	long placeholders;
	/* loaded tags kept after the disc's (mcc_import_keep) */
	long kept;
	/* the disc's tags, the first of those written */
	long disc_count;
	/* the structure BSPs written (for the fingerprint) */
	struct mcc_fingerprint_bsp bsps[MCC_IMPORT_MAXIMUM_BSPS];
	long bsp_count;
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

/* Until the patch names the disc's tags in their place, the loaded tags the
written ones name (by reference, tag index or script) are written too, after
the disc's; the rest of what MCC added is left out. */
static boolean mcc_import_keep(
	void)
{
	long out_index;

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
(d:\mcc_patches\<map>.p2, the prototype's: .local/mcc_check/patch_make.c),
whose `order` line names every tag of the disc's map as group:path (a group's
spaces written as underscores); else the loaded tags as they are. */
static boolean mcc_import_order(
	char const *map_name)
{
	char path[256];
	char *text = NULL;
	char *order = NULL;
	FILE *file;
	long index, count;

	snprintf(path, sizeof(path), "d:\\mcc_patches\\%s.p2", map_name);
	file = fopen(path, "rb");
	if (file)
	{
		long size;

		fseek(file, 0, SEEK_END);
		size = ftell(file);
		fseek(file, 0, SEEK_SET);
		text = size > 0 ? mcc_import_calloc((size_t)size + 1, 1) : NULL;
		if (text && fread(text, 1, (size_t)size, file) != (size_t)size)
		{
			mcc_import_free(text);
			text = NULL;
		}
		fclose(file);
	}
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
		mcc_import_free(text);
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
		mcc_import_error("mcc import: %s: no tag order (%s): the loaded tags written as they are", map_name, path);
		mcc_import_free(text);
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
	csmemset(space->bytes + offset, 0, size);
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
				/* a bitmap's pixels are in the body already (their offset
				set in the tag); a sound's samples are in MCC's sound banks,
				which are not read yet: silent */
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
			custom_edition_cache_read(tag_index, bitmap->pixels_offset, bitmap->pixels_size, pixels);
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
			bitmap->tag_index = mcc_import_new_handle(mcc_import_globals.old_to_new[tag_index]);
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
		mcc_import_copy_element(space, schema->definition, source->base_address, mcc_import_at(space, root));
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
(mcc_import_fingerprint.c) the disc map's, every part of it, from
d:\mcc_patches\<map>.fp (the disc's maps', made by the prototype's
sim_fingerprint). Without that file it cannot be shown. Logs the parts that
differ. */
static boolean mcc_import_stock(
	char const *map_name,
	struct mcc_import_space const *tags)
{
	char const *lines = mcc_import_fingerprint(tags->bytes, tags->used, mcc_import_globals.disc_count,
		mcc_import_globals.bsps, mcc_import_globals.bsp_count);
	char path[256];
	char *expected = NULL;
	char const *line;
	FILE *file;
	long differing = 0, parts = 0;

	snprintf(path, sizeof(path), "d:\\mcc_patches\\%s.fp", map_name);
	file = fopen(path, "rb");
	if (file)
	{
		long size;

		fseek(file, 0, SEEK_END);
		size = ftell(file);
		fseek(file, 0, SEEK_SET);
		/* (a newline before the first line, so that every line is found
		between two) */
		expected = size > 0 ? mcc_import_calloc((size_t)size + 3, 1) : NULL;
		if (expected && fread(expected + 1, 1, (size_t)size, file) != (size_t)size)
		{
			mcc_import_free(expected);
			expected = NULL;
		}
		if (expected)
		{
			long from, to = 1;

			expected[0] = '\n';
			/* (without carriage returns: a file written on Windows has them) */
			for (from = 1; from <= size; from++)
			{
				if (expected[from] != '\r')
					expected[to++] = expected[from];
			}
			expected[to] = '\n';
			expected[to + 1] = 0;
		}
		fclose(file);
	}
	if (!expected)
	{
		mcc_import_error("mcc import: %s: no fingerprint of the disc's map (%s): an MCC map", map_name, path);
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

static boolean mcc_import_map(
	char const *map_name,
	char const *output_folder)
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

	csmemset(&mcc_import_globals, 0, sizeof(mcc_import_globals));
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
		mcc_import_error("mcc import: %s: no operations (d:\\mcc_patches\\%s.p2): MCC's values kept", map_name, map_name);
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
	if (deflateInit(&body->stream, Z_DEFAULT_COMPRESSION) != Z_OK)
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
	success = success && mcc_import_bitmaps(body);
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
		fprintf(stdout, "mcc import: %s: %d structure bsps, %ld bitmaps, %ld tags (0x%lX bytes; %ld empty in the "
			"disc's place, %ld of MCC's kept after the disc's, %ld without a schema; %ld references to tags left out), 0x%lX bytes before compression\n",
			map_name, (int)references->count, mcc_import_globals.bitmaps, mcc_import_globals.out_count, tags.used,
			mcc_import_globals.placeholders, mcc_import_globals.kept, mcc_import_globals.schemaless, mcc_import_globals.references_dropped,
			(unsigned long)header.file_length);
	}
	deflateEnd(&body->stream);
	fclose(body->file);
	mcc_import_free(body);
	mcc_import_free(mcc_import_globals.name_addresses);
	mcc_import_free(mcc_import_globals.out_sources);
	mcc_import_free(mcc_import_globals.out_groups);
	mcc_import_free(mcc_import_globals.out_names);
	mcc_import_free(mcc_import_globals.old_to_new);
	mcc_import_free(mcc_import_globals.out_names_text);
	mcc_import_free(mcc_import_globals.table.instances);
	for (bsp_index = 0; bsp_index < mcc_import_globals.bsp_count; bsp_index++)
		mcc_import_free(mcc_import_globals.bsps[bsp_index].bytes);
	mcc_import_operations_dispose();
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
	for (index = 0; index < map_count; index++)
	{
		if (!mcc_import_map(map_names[index], output_folder))
			failures++;
	}
	custom_edition_geometry_import = FALSE;

	return failures ? 1 : 0;
}
