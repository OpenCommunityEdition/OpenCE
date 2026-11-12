/*
TAG_BUNDLE.C

Tag bundles (tag_bundle.h): a tag of a Custom Edition map and every tag it
names, written out of the map's checked tags.

Each tag is walked through its group's schema (tag_schema.h) twice, in the
same order. The first walk finds the tags (every reference's and tag index's
tag, added once, in the order they are first named) and lays out what each
holds: its root, its blocks' elements and its data, one after another in
the bundle's data, and the bytes of the map's files it names (a bitmap's
pixels, which its pixels_offset and pixels_size name, sound samples, a
gbxmodel's vertices and strips) in its file data.
The second walk copies them there and makes each pointer and tag the
bundle's: as it visits the same fields in the same order, the nth block or
data it meets is the nth laid out, and no address need be looked up.

The tags are those tag_validate_custom_edition_tags passed: every block,
data and name lies in the tags and overlaps no other, every file range lies
in the map's files, and every tag named is one of the map's of the group the
field takes (or none). What a tag's group has no schema for, the validator
did not walk, and neither can this: such a tag is not bundled
(_tag_bundle_status_unknown_group), nor is a tag that names one.
*/

/* ---------- headers */

#ifndef HALO_RELEASE
#define HALO_RELEASE
#endif
#include "cseries.h"
#include "tag_schema.h"
#include "tag_bundle.h"

#include <stdlib.h>
#include <string.h>

#undef malloc
#undef free
#undef realloc

/* ---------- constants */

enum
{
	/* how deep blocks and structures go (tag_validate.c's) */
	MAXIMUM_WALK_DEPTH = 32,
	/* the longest tag path kept, without its terminator */
	MAXIMUM_TAG_NAME_LENGTH = 255,
	/* the most data a bundle holds: a tag cache's (its tags can hold no
	more), and file data: no more than a map and its resource maps have */
	MAXIMUM_DATA_SIZE = TAG_VALIDATE_MAXIMUM_TAG_CACHE_SIZE,
	MAXIMUM_FILE_DATA_SIZE = 0x40000000,
	/* what the file data is copied by */
	COPY_CHUNK_SIZE = 0x10000,
	ALIGNMENT = 4,

	/* a Custom Edition gbxmodel's part (tag_schema_models.c's struct
	gbxmodel_geometry_part; custom_edition_geometry.c reads it too): its
	strip, of strip_triangle_count + 2 indices, at strip_offset in the model
	data's indices, and its vertex_count vertices at vertex_offset in the
	model data */
	GBXMODEL_PART_BYTES = 0x84,
	GBXMODEL_PART_STRIP_TRIANGLE_COUNT_OFFSET = 0x48,
	GBXMODEL_PART_STRIP_OFFSET_OFFSET = 0x4C,
	GBXMODEL_PART_VERTEX_COUNT_OFFSET = 0x58,
	GBXMODEL_PART_VERTEX_OFFSET_OFFSET = 0x64,
	GBXMODEL_VERTEX_BYTES = 0x44,
	STRIP_INDEX_BYTES = 2,

	/* a bitmap of a bitmap group (bitmap_group.h's struct bitmap_data): the
	texture cache reads its pixels_size bytes of pixels at pixels_offset in
	the map's files, which no tag_data names */
	BITMAP_DATA_BYTES = 0x30,
	BITMAP_DATA_PIXELS_OFFSET_OFFSET = 0x18,
	BITMAP_DATA_PIXELS_SIZE_OFFSET = 0x1C,
};

/* the schema definitions of a gbxmodel's part (tag_schema_models.c) and of
a bitmap (tag_schema_render.c) */
#define GBXMODEL_PART_DEFINITION_NAME "gbxmodel_geometry_part"
#define BITMAP_DATA_DEFINITION_NAME "bitmap_data"

/* the walks */
enum
{
	_walk_layout,
	_walk_copy,
};

/* ---------- structures */

/* a tag in the header's table (tag_validate.c's struct
tag_validate_instance) */
struct bundle_instance
{
	unsigned long group_tag;
	unsigned long parent_group_tags[2];
	long tag_index;
	char *name;
	void *base_address;
	/* (the first: nonzero for a tag read from a resource map) */
	unsigned long unused[2];
};

/* a Custom Edition map's tags' header (tag_validate.c's struct
tag_validate_custom_edition_header) */
struct bundle_tag_header
{
	struct bundle_instance *instances;
	long scenario_tag_index;
	unsigned long checksum;
	long tag_count;
	long model_part_count;
	unsigned long model_data_offset;
	long model_part_count_again;
	unsigned long model_index_data_offset;
	unsigned long model_data_size;
	unsigned long signature;
};

typedef char verify_bundle_instance_size[sizeof(struct bundle_instance) == 0x20 ? 1 : -1];
typedef char verify_bundle_tag_header_size[sizeof(struct bundle_tag_header) == 0x28 ? 1 : -1];

/* bytes of the tags laid out in data: from address, at offset */
struct bundle_region
{
	byte const *address;
	unsigned long size;
	unsigned long offset;
};

/* bytes of the map's files laid out in file data */
struct bundle_file_region
{
	unsigned long file_offset;
	unsigned long size;
	unsigned long offset;
};

struct bundle
{
	struct tag_bundle_source const *source;
	struct bundle_tag_header const *header;

	/* the bundle's tags (absolute indices), and each map tag's place among
	them (NONE: not one) */
	long *tags;
	long tag_count;
	long tag_capacity;
	long *bundle_indices;
	/* each bundle tag's path, in names */
	unsigned long *name_offsets;
	unsigned long names_size;

	struct bundle_region *regions;
	long region_count;
	long region_capacity;
	unsigned long data_size;
	struct bundle_file_region *file_regions;
	long file_region_count;
	long file_region_capacity;
	unsigned long file_data_size;

	/* the copy walk: the data, and the next region and file region it meets */
	byte *data;
	long region_cursor;
	long file_region_cursor;

	short walk;
	short depth;
	long tag_index;
	enum tag_bundle_status status;
	long problem_tag_index;
};

/* ---------- private code */

static unsigned long align_up(
	unsigned long value)
{
	return (value + ALIGNMENT - 1) & ~(unsigned long)(ALIGNMENT - 1);
}

static void bundle_fail(
	struct bundle *bundle,
	enum tag_bundle_status status)
{
	if (bundle->status == _tag_bundle_status_ok)
	{
		bundle->status = status;
		bundle->problem_tag_index = bundle->tag_index;
	}

	return;
}

/* room in *list (of *capacity elements of element_size bytes) for one more
after count */
static boolean list_grow(
	void **list,
	long *capacity,
	long count,
	unsigned long element_size)
{
	long new_capacity;
	void *grown;

	if (count < *capacity)
		return TRUE;
	new_capacity = *capacity ? *capacity * 2 : 64;
	if ((unsigned long)new_capacity > 0x7FFFFFFFUL / element_size)
		return FALSE;
	/* (the game's realloc and free take no NULL: cseries.h) */
	grown = *list ? realloc(*list, (unsigned long)new_capacity * element_size) :
		malloc((unsigned long)new_capacity * element_size);
	if (!grown)
		return FALSE;
	*list = grown;
	*capacity = new_capacity;

	return TRUE;
}

static struct tag_schema_group const *bundle_schema_group(
	unsigned long group_tag)
{
	struct tag_schema_group const *const *list;
	struct tag_schema_group const *group;

	/* (a Custom Edition map's groups laid out otherwise first, as the
	validator takes them) */
	for (group = tag_schema_custom_edition_groups; group->group_tag; group++)
	{
		if (group->group_tag == group_tag)
			return group;
	}
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

/* the absolute index of the map's tag `tag_index` names, or NONE */
static long bundle_absolute_index(
	struct bundle const *bundle,
	long tag_index)
{
	long absolute_index = (long)(unsigned short)tag_index;

	if (tag_index == NONE || absolute_index >= bundle->header->tag_count ||
		bundle->header->instances[absolute_index].tag_index != tag_index)
	{
		return NONE;
	}

	return absolute_index;
}

/* the bundle's index of the map's tag `tag_index` names, added if it is not
one yet (the layout walk), or NONE for none */
static long bundle_tag(
	struct bundle *bundle,
	long tag_index)
{
	long absolute_index;

	if (tag_index == NONE)
		return NONE;
	absolute_index = bundle_absolute_index(bundle, tag_index);
	if (absolute_index == NONE)
	{
		bundle_fail(bundle, _tag_bundle_status_bad_pointer);
		return NONE;
	}
	if (bundle->bundle_indices[absolute_index] == NONE && bundle->walk == _walk_layout)
	{
		if (!list_grow((void **)&bundle->tags, &bundle->tag_capacity, bundle->tag_count, sizeof(long)))
		{
			bundle_fail(bundle, _tag_bundle_status_out_of_memory);
			return NONE;
		}
		bundle->bundle_indices[absolute_index] = bundle->tag_count;
		bundle->tags[bundle->tag_count++] = absolute_index;
	}

	return bundle->bundle_indices[absolute_index];
}

/* lays out size bytes at address in data (the layout walk), or gives the
copy of those it laid out there (the copy walk); NULL if it cannot */
static byte *bundle_region(
	struct bundle *bundle,
	byte const *address,
	unsigned long size)
{
	struct bundle_region *region;

	if (bundle->walk == _walk_layout)
	{
		unsigned long offset = align_up(bundle->data_size);

		if (!address || size > MAXIMUM_DATA_SIZE || offset > MAXIMUM_DATA_SIZE - size)
		{
			bundle_fail(bundle, address ? _tag_bundle_status_too_large : _tag_bundle_status_bad_pointer);
			return NULL;
		}
		if (!list_grow((void **)&bundle->regions, &bundle->region_capacity, bundle->region_count,
			sizeof(struct bundle_region)))
		{
			bundle_fail(bundle, _tag_bundle_status_out_of_memory);
			return NULL;
		}
		region = &bundle->regions[bundle->region_count++];
		region->address = address;
		region->size = size;
		region->offset = offset;
		bundle->data_size = offset + size;
		/* (nothing to write to yet) */
		return (byte *)address;
	}

	if (bundle->region_cursor >= bundle->region_count)
	{
		bundle_fail(bundle, _tag_bundle_status_bad_pointer);
		return NULL;
	}
	region = &bundle->regions[bundle->region_cursor++];
	if (region->address != address || region->size != size)
	{
		bundle_fail(bundle, _tag_bundle_status_bad_pointer);
		return NULL;
	}

	return bundle->data + region->offset;
}

/* lays out size bytes at file_offset in file data, or gives the offset it
laid them out at; NONE if it cannot */
static unsigned long bundle_file_region(
	struct bundle *bundle,
	unsigned long file_offset,
	unsigned long size)
{
	struct bundle_file_region *region;

	if (bundle->walk == _walk_layout)
	{
		unsigned long offset = align_up(bundle->file_data_size);

		if (size > MAXIMUM_FILE_DATA_SIZE || offset > MAXIMUM_FILE_DATA_SIZE - size)
		{
			bundle_fail(bundle, _tag_bundle_status_too_large);
			return (unsigned long)NONE;
		}
		if (!list_grow((void **)&bundle->file_regions, &bundle->file_region_capacity, bundle->file_region_count,
			sizeof(struct bundle_file_region)))
		{
			bundle_fail(bundle, _tag_bundle_status_out_of_memory);
			return (unsigned long)NONE;
		}
		region = &bundle->file_regions[bundle->file_region_count++];
		region->file_offset = file_offset;
		region->size = size;
		region->offset = offset;
		bundle->file_data_size = offset + size;
		return offset;
	}

	if (bundle->file_region_cursor >= bundle->file_region_count)
	{
		bundle_fail(bundle, _tag_bundle_status_bad_pointer);
		return (unsigned long)NONE;
	}
	region = &bundle->file_regions[bundle->file_region_cursor++];
	if (region->file_offset != file_offset || region->size != size)
	{
		bundle_fail(bundle, _tag_bundle_status_bad_pointer);
		return (unsigned long)NONE;
	}

	return region->offset;
}

static unsigned long read_unsigned_long(
	byte const *address)
{
	unsigned long value;

	memcpy(&value, address, sizeof(value));
	return value;
}

static void write_unsigned_long(
	byte *address,
	unsigned long value)
{
	memcpy(address, &value, sizeof(value));

	return;
}

/* a gbxmodel's part: its vertices and its strip, in the map's model data */
static void bundle_gbxmodel_part(
	struct bundle *bundle,
	byte const *part,
	byte *copy)
{
	struct bundle_tag_header const *header = bundle->header;
	unsigned long strip_count = read_unsigned_long(part + GBXMODEL_PART_STRIP_TRIANGLE_COUNT_OFFSET);
	unsigned long strip_offset = read_unsigned_long(part + GBXMODEL_PART_STRIP_OFFSET_OFFSET);
	unsigned long vertex_count = read_unsigned_long(part + GBXMODEL_PART_VERTEX_COUNT_OFFSET);
	unsigned long vertex_offset = read_unsigned_long(part + GBXMODEL_PART_VERTEX_OFFSET_OFFSET);
	unsigned long vertices;
	unsigned long strip;

	/* (the loader checked that they lie in the model data:
	cache_file_formats.c; these are its bounds again, so that nothing out of
	them is read) */
	if ((long)strip_count < 0 || (long)vertex_count < 0 ||
		strip_count > (header->model_data_size - header->model_index_data_offset) / STRIP_INDEX_BYTES - 2 ||
		vertex_count > header->model_index_data_offset / GBXMODEL_VERTEX_BYTES ||
		vertex_offset > header->model_index_data_offset - vertex_count * GBXMODEL_VERTEX_BYTES ||
		strip_offset > header->model_data_size - header->model_index_data_offset - (strip_count + 2) * STRIP_INDEX_BYTES)
	{
		bundle_fail(bundle, _tag_bundle_status_bad_pointer);
		return;
	}
	vertices = bundle_file_region(bundle, header->model_data_offset + vertex_offset,
		vertex_count * GBXMODEL_VERTEX_BYTES);
	strip = bundle_file_region(bundle, header->model_data_offset + header->model_index_data_offset + strip_offset,
		(strip_count + 2) * STRIP_INDEX_BYTES);
	if (bundle->walk == _walk_copy && bundle->status == _tag_bundle_status_ok)
	{
		write_unsigned_long(copy + GBXMODEL_PART_VERTEX_OFFSET_OFFSET, vertices);
		write_unsigned_long(copy + GBXMODEL_PART_STRIP_OFFSET_OFFSET, strip);
	}

	return;
}

/* a bitmap: its pixels, in the map's files */
static void bundle_bitmap_data(
	struct bundle *bundle,
	byte const *bitmap,
	byte *copy)
{
	unsigned long pixels_offset = read_unsigned_long(bitmap + BITMAP_DATA_PIXELS_OFFSET_OFFSET);
	unsigned long pixels_size = read_unsigned_long(bitmap + BITMAP_DATA_PIXELS_SIZE_OFFSET);
	unsigned long pixels;

	/* (the validator checked that they lie in the map's files) */
	if ((long)pixels_size < 0)
	{
		bundle_fail(bundle, _tag_bundle_status_bad_pointer);
		return;
	}
	pixels = bundle_file_region(bundle, pixels_offset, pixels_size);
	if (bundle->walk == _walk_copy && bundle->status == _tag_bundle_status_ok)
		write_unsigned_long(copy + BITMAP_DATA_PIXELS_OFFSET_OFFSET, pixels);

	return;
}

static void bundle_element(
	struct bundle *bundle,
	byte const *base,
	byte *copy,
	struct tag_schema_definition const *definition);

/* the fields of an element (or a structure in one) at base, whose copy is
at copy (the copy walk) */
static void bundle_fields(
	struct bundle *bundle,
	byte const *base,
	byte *copy,
	struct tag_schema_definition const *definition)
{
	struct tag_schema_field const *field;

	for (field = definition->fields;
		field->type != _tag_schema_terminator && bundle->status == _tag_bundle_status_ok;
		field++)
	{
		short index;

		for (index = 0; index < field->count && bundle->status == _tag_bundle_status_ok; index++)
		{
			long offset = field->offset + index * field->size;
			byte const *address = base + offset;
			byte *copied = copy ? copy + offset : NULL;

			switch (field->type)
			{
			case _tag_schema_struct:
				bundle_element(bundle, address, copied, field->definition);
				break;

			case _tag_schema_block:
			{
				struct tag_block const *block = (struct tag_block const *)address;
				struct tag_schema_definition const *element_definition = field->definition;
				byte const *elements = block->address;
				byte *element_copies = NULL;
				long element_index;

				if (block->count < 0 ||
					(block->count && (unsigned long)block->count > MAXIMUM_DATA_SIZE / (unsigned long)element_definition->size))
				{
					bundle_fail(bundle, _tag_bundle_status_bad_pointer);
					break;
				}
				if (block->count)
				{
					element_copies = bundle_region(bundle, elements,
						(unsigned long)block->count * (unsigned long)element_definition->size);
					if (!element_copies)
						break;
				}
				if (bundle->walk == _walk_copy)
				{
					struct tag_block *block_copy = (struct tag_block *)copied;

					block_copy->address = block->count ? (void *)(element_copies - bundle->data) : NULL;
					block_copy->definition = NULL;
				}
				for (element_index = 0;
					element_index < block->count && bundle->status == _tag_bundle_status_ok;
					element_index++)
				{
					bundle_element(bundle,
						elements + element_index * element_definition->size,
						bundle->walk == _walk_copy ? element_copies + element_index * element_definition->size : NULL,
						element_definition);
				}
				break;
			}

			case _tag_schema_data:
			{
				struct tag_data const *data = (struct tag_data const *)address;
				byte *data_copy = NULL;

				if (data->size < 0)
				{
					bundle_fail(bundle, _tag_bundle_status_bad_pointer);
					break;
				}
				if (data->size)
				{
					data_copy = bundle_region(bundle, data->address, (unsigned long)data->size);
					if (!data_copy)
						break;
				}
				if (bundle->walk == _walk_copy)
				{
					struct tag_data *copied_data = (struct tag_data *)copied;

					copied_data->address = data->size ? (void *)(data_copy - bundle->data) : NULL;
					copied_data->definition = NULL;
				}
				break;
			}

			case _tag_schema_file_data:
			{
				struct tag_data const *data = (struct tag_data const *)address;
				unsigned long file_offset = 0;

				if (data->size < 0)
				{
					bundle_fail(bundle, _tag_bundle_status_bad_pointer);
					break;
				}
				if (data->size)
					file_offset = bundle_file_region(bundle, (unsigned long)data->file_offset, (unsigned long)data->size);
				if (bundle->walk == _walk_copy && bundle->status == _tag_bundle_status_ok)
				{
					struct tag_data *copied_data = (struct tag_data *)copied;

					copied_data->file_offset = (long)file_offset;
					copied_data->address = NULL;
					copied_data->definition = NULL;
				}
				break;
			}

			case _tag_schema_reference:
			{
				struct tag_reference const *reference = (struct tag_reference const *)address;
				long bundle_index = bundle_tag(bundle, reference->index);

				if (bundle->walk == _walk_copy && bundle->status == _tag_bundle_status_ok)
				{
					struct tag_reference *copied_reference = (struct tag_reference *)copied;

					copied_reference->index = bundle_index;
					copied_reference->name = bundle_index == NONE ?
						(char *)NONE :
						(char *)bundle->name_offsets[bundle_index];
				}
				break;
			}

			case _tag_schema_tag_index:
			{
				long bundle_index = bundle_tag(bundle, *(long const *)address);

				if (bundle->walk == _walk_copy && bundle->status == _tag_bundle_status_ok)
					*(long *)copied = bundle_index;
				break;
			}

			default:
				break;
			}
		}
	}
	if (bundle->status == _tag_bundle_status_ok && !strcmp(definition->name, GBXMODEL_PART_DEFINITION_NAME))
	{
		if (definition->size != GBXMODEL_PART_BYTES)
			bundle_fail(bundle, _tag_bundle_status_unknown_group);
		else
			bundle_gbxmodel_part(bundle, base, copy);
	}
	else if (bundle->status == _tag_bundle_status_ok && !strcmp(definition->name, BITMAP_DATA_DEFINITION_NAME))
	{
		if (definition->size != BITMAP_DATA_BYTES)
			bundle_fail(bundle, _tag_bundle_status_unknown_group);
		else
			bundle_bitmap_data(bundle, base, copy);
	}

	return;
}

static void bundle_element(
	struct bundle *bundle,
	byte const *base,
	byte *copy,
	struct tag_schema_definition const *definition)
{
	if (bundle->depth >= MAXIMUM_WALK_DEPTH)
	{
		bundle_fail(bundle, _tag_bundle_status_bad_pointer);
		return;
	}
	bundle->depth++;
	bundle_fields(bundle, base, copy, definition);
	bundle->depth--;

	return;
}

/* one walk over every tag of the bundle (in the layout walk, also those it
finds as it goes) */
static void bundle_walk(
	struct bundle *bundle,
	short walk)
{
	long bundle_index;

	bundle->walk = walk;
	bundle->region_cursor = 0;
	bundle->file_region_cursor = 0;
	for (bundle_index = 0; bundle_index < bundle->tag_count && bundle->status == _tag_bundle_status_ok; bundle_index++)
	{
		struct bundle_instance const *instance = &bundle->header->instances[bundle->tags[bundle_index]];
		struct tag_schema_group const *group = bundle_schema_group(instance->group_tag);
		byte *root;

		bundle->tag_index = bundle->tags[bundle_index];
		bundle->depth = 0;
		if (!group || !group->definition || !instance->base_address)
		{
			bundle_fail(bundle, _tag_bundle_status_unknown_group);
			break;
		}
		root = bundle_region(bundle, instance->base_address, (unsigned long)group->definition->size);
		if (root)
			bundle_element(bundle, instance->base_address, walk == _walk_copy ? root : NULL, group->definition);
	}
	bundle->tag_index = NONE;

	return;
}

/* (the game's free takes no NULL: cseries.h) */
static void free_some(
	void *pointer)
{
	if (pointer)
		free(pointer);

	return;
}

static void bundle_dispose(
	struct bundle *bundle)
{
	free_some(bundle->tags);
	free_some(bundle->bundle_indices);
	free_some(bundle->name_offsets);
	free_some(bundle->regions);
	free_some(bundle->file_regions);
	free_some(bundle->data);
	memset(bundle, 0, sizeof(*bundle));

	return;
}

/* the walk that finds the tags and lays them out, from the map's tag of
absolute index tag_index */
static void bundle_layout(
	struct bundle *bundle,
	struct tag_bundle_source const *source,
	long tag_index)
{
	long index;

	memset(bundle, 0, sizeof(*bundle));
	bundle->source = source;
	bundle->header = source->tag_header;
	bundle->tag_index = NONE;
	bundle->problem_tag_index = NONE;
	if (!bundle->header || bundle->header->tag_count <= 0 || tag_index < 0 || tag_index >= bundle->header->tag_count)
	{
		bundle->status = _tag_bundle_status_no_such_tag;
		return;
	}
	bundle->bundle_indices = malloc((unsigned long)bundle->header->tag_count * sizeof(long));
	if (!bundle->bundle_indices)
	{
		bundle->status = _tag_bundle_status_out_of_memory;
		return;
	}
	for (index = 0; index < bundle->header->tag_count; index++)
		bundle->bundle_indices[index] = NONE;
	bundle->walk = _walk_layout;
	bundle_tag(bundle, bundle->header->instances[tag_index].tag_index);
	bundle_walk(bundle, _walk_layout);

	return;
}

/* the length of the map's tag's path kept (at most
MAXIMUM_TAG_NAME_LENGTH) */
static unsigned long bundle_name_length(
	char const *name)
{
	char const *end = name ? memchr(name, 0, MAXIMUM_TAG_NAME_LENGTH + 1) : NULL;

	if (!name)
		return 0;
	return end ? (unsigned long)(end - name) : MAXIMUM_TAG_NAME_LENGTH;
}

static unsigned long bundle_string_length(
	char const *text)
{
	return text ? (unsigned long)strlen(text) : 0;
}

static boolean bundle_write_bytes(
	struct bundle *bundle,
	tag_bundle_write_proc write,
	void *write_context,
	void const *bytes,
	unsigned long size)
{
	if (bundle->status == _tag_bundle_status_ok && size && !write(write_context, bytes, size))
		bundle_fail(bundle, _tag_bundle_status_write_failed);

	return bundle->status == _tag_bundle_status_ok;
}

/* zeros to the next ALIGNMENT, from position */
static boolean bundle_write_padding(
	struct bundle *bundle,
	tag_bundle_write_proc write,
	void *write_context,
	unsigned long position)
{
	static byte const zeros[ALIGNMENT] = { 0 };

	return bundle_write_bytes(bundle, write, write_context, zeros, align_up(position) - position);
}

/* ---------- loading */

/* a bundle being checked and loaded (tag_bundle_load) */
struct bundle_load
{
	byte const *tags;
	long tag_count;
	char const *names;
	unsigned long names_size;
	byte *data;
	unsigned long data_size;
	unsigned long file_data_size;

	/* where the next region of data and of file data must start: a bundle
	is laid out as its writer's walk lays it out, and nothing else is one */
	unsigned long cursor;
	unsigned long file_cursor;
	/* the regions of file data found, when they are listed
	(tag_bundle_load_tags_regions) */
	boolean list_regions;
	struct tag_bundle_file_region *regions;
	long region_count;
	long region_capacity;

	short depth;
	long tag_index;
	enum tag_bundle_status status;
	long problem_tag_index;
};

static void load_fail(
	struct bundle_load *load)
{
	if (load->status == _tag_bundle_status_ok)
	{
		load->status = _tag_bundle_status_bad_bundle;
		load->problem_tag_index = load->tag_index;
	}

	return;
}

/* the next region of data, of size bytes, which must be at offset: its
bytes, or NULL */
static byte *load_region(
	struct bundle_load *load,
	unsigned long offset,
	unsigned long size)
{
	unsigned long expected = align_up(load->cursor);

	if (offset != expected || expected > load->data_size || size > load->data_size - expected)
	{
		load_fail(load);
		return NULL;
	}
	load->cursor = expected + size;

	return load->data + expected;
}

/* the next region of file data, likewise */
static void load_file_region(
	struct bundle_load *load,
	unsigned long offset,
	unsigned long size)
{
	unsigned long expected = align_up(load->file_cursor);

	if (offset != expected || expected > load->file_data_size || size > load->file_data_size - expected)
	{
		load_fail(load);
		return;
	}
	load->file_cursor = expected + size;
	if (load->list_regions && size)
	{
		if (!list_grow((void **)&load->regions, &load->region_capacity, load->region_count,
			sizeof(struct tag_bundle_file_region)))
		{
			if (load->status == _tag_bundle_status_ok)
				load->status = _tag_bundle_status_out_of_memory;
			return;
		}
		load->regions[load->region_count].offset = expected;
		load->regions[load->region_count].size = size;
		load->region_count++;
	}

	return;
}

static unsigned long load_tag_field(
	struct bundle_load const *load,
	long bundle_index,
	long field_offset)
{
	return read_unsigned_long(load->tags + bundle_index * TAG_BUNDLE_TAG_BYTES + field_offset);
}

/* a gbxmodel's part: its vertices and its strip, next in file data */
static void load_gbxmodel_part(
	struct bundle_load *load,
	byte const *part)
{
	unsigned long strip_count = read_unsigned_long(part + GBXMODEL_PART_STRIP_TRIANGLE_COUNT_OFFSET);
	unsigned long vertex_count = read_unsigned_long(part + GBXMODEL_PART_VERTEX_COUNT_OFFSET);

	if (strip_count > MAXIMUM_FILE_DATA_SIZE / STRIP_INDEX_BYTES - 2 ||
		vertex_count > MAXIMUM_FILE_DATA_SIZE / GBXMODEL_VERTEX_BYTES)
	{
		load_fail(load);
		return;
	}
	load_file_region(load, read_unsigned_long(part + GBXMODEL_PART_VERTEX_OFFSET_OFFSET),
		vertex_count * GBXMODEL_VERTEX_BYTES);
	load_file_region(load, read_unsigned_long(part + GBXMODEL_PART_STRIP_OFFSET_OFFSET),
		(strip_count + 2) * STRIP_INDEX_BYTES);

	return;
}

/* a bitmap: its pixels, next in file data */
static void load_bitmap_data(
	struct bundle_load *load,
	byte const *bitmap)
{
	unsigned long pixels_size = read_unsigned_long(bitmap + BITMAP_DATA_PIXELS_SIZE_OFFSET);

	if (pixels_size > MAXIMUM_FILE_DATA_SIZE)
	{
		load_fail(load);
		return;
	}
	load_file_region(load, read_unsigned_long(bitmap + BITMAP_DATA_PIXELS_OFFSET_OFFSET), pixels_size);

	return;
}

static void load_element(
	struct bundle_load *load,
	byte *base,
	struct tag_schema_definition const *definition);

static void load_fields(
	struct bundle_load *load,
	byte *base,
	struct tag_schema_definition const *definition)
{
	struct tag_schema_field const *field;

	for (field = definition->fields;
		field->type != _tag_schema_terminator && load->status == _tag_bundle_status_ok;
		field++)
	{
		short index;

		for (index = 0; index < field->count && load->status == _tag_bundle_status_ok; index++)
		{
			byte *address = base + field->offset + index * field->size;

			switch (field->type)
			{
			case _tag_schema_struct:
				load_element(load, address, field->definition);
				break;

			case _tag_schema_block:
			{
				struct tag_block *block = (struct tag_block *)address;
				struct tag_schema_definition const *element_definition = field->definition;
				byte *elements = NULL;
				long element_index;

				if (block->count < 0 || block->definition ||
					(unsigned long)block->count > load->data_size / (unsigned long)element_definition->size ||
					(!block->count && block->address))
				{
					load_fail(load);
					break;
				}
				if (block->count)
				{
					elements = load_region(load, (unsigned long)block->address,
						(unsigned long)block->count * (unsigned long)element_definition->size);
					block->address = elements;
				}
				for (element_index = 0;
					element_index < block->count && load->status == _tag_bundle_status_ok;
					element_index++)
				{
					load_element(load, elements + element_index * element_definition->size, element_definition);
				}
				break;
			}

			case _tag_schema_data:
			{
				struct tag_data *data = (struct tag_data *)address;

				if (data->size < 0 || data->definition || (!data->size && data->address))
				{
					load_fail(load);
					break;
				}
				if (data->size)
					data->address = load_region(load, (unsigned long)data->address, (unsigned long)data->size);
				break;
			}

			case _tag_schema_file_data:
			{
				struct tag_data const *data = (struct tag_data const *)address;

				if (data->size < 0 || data->definition || data->address)
					load_fail(load);
				else if (data->size)
					load_file_region(load, (unsigned long)data->file_offset, (unsigned long)data->size);
				break;
			}

			case _tag_schema_reference:
			{
				struct tag_reference *reference = (struct tag_reference *)address;

				if (reference->index == NONE)
				{
					/* (the names' last byte: an empty name) */
					if (reference->name != (char *)NONE)
						load_fail(load);
					else
						reference->name = (char *)load->names + load->names_size - 1;
				}
				else if (reference->index < 0 || reference->index >= load->tag_count ||
					(unsigned long)reference->name != load_tag_field(load, reference->index, TAG_BUNDLE_TAG_NAME_OFFSET) ||
					reference->group_tag != load_tag_field(load, reference->index, TAG_BUNDLE_TAG_GROUP_OFFSET))
				{
					load_fail(load);
				}
				else
				{
					reference->name = (char *)load->names + (unsigned long)reference->name;
				}
				break;
			}

			case _tag_schema_tag_index:
			{
				long tag_index = *(long const *)address;

				if (tag_index != NONE && (tag_index < 0 || tag_index >= load->tag_count))
					load_fail(load);
				break;
			}

			default:
				break;
			}
		}
	}
	if (load->status == _tag_bundle_status_ok && !strcmp(definition->name, GBXMODEL_PART_DEFINITION_NAME))
	{
		if (definition->size != GBXMODEL_PART_BYTES)
			load_fail(load);
		else
			load_gbxmodel_part(load, base);
	}
	else if (load->status == _tag_bundle_status_ok && !strcmp(definition->name, BITMAP_DATA_DEFINITION_NAME))
	{
		if (definition->size != BITMAP_DATA_BYTES)
			load_fail(load);
		else
			load_bitmap_data(load, base);
	}

	return;
}

static void load_element(
	struct bundle_load *load,
	byte *base,
	struct tag_schema_definition const *definition)
{
	if (load->depth >= MAXIMUM_WALK_DEPTH)
	{
		load_fail(load);
		return;
	}
	load->depth++;
	load_fields(load, base, definition);
	load->depth--;

	return;
}

/* ---------- public code */

char const *tag_bundle_status_describe(
	enum tag_bundle_status status)
{
	static char const *const descriptions[NUMBER_OF_TAG_BUNDLE_STATUSES] =
	{
		"ok",
		"no such tag",
		"a tag whose group no schema knows the pointers of",
		"a pointer not where the walk found the tags",
		"not laid out as a bundle is",
		"too large",
		"out of memory",
		"a map file could not be read",
		"the bundle could not be written",
	};

	return (unsigned)status < NUMBER_OF_TAG_BUNDLE_STATUSES ? descriptions[status] : "unknown";
}

enum tag_bundle_status tag_bundle_tags(
	struct tag_bundle_source const *source,
	long tag_index,
	long **tag_indices,
	long *tag_count,
	long *problem_tag_index)
{
	struct bundle bundle;
	enum tag_bundle_status status;

	bundle_layout(&bundle, source, tag_index);
	status = bundle.status;
	*problem_tag_index = bundle.problem_tag_index;
	*tag_count = bundle.tag_count;
	/* (the tags found, those of a bundle that cannot be written too: what
	named the one that stopped it) */
	*tag_indices = bundle.tags;
	bundle.tags = NULL;
	bundle_dispose(&bundle);

	return status;
}

enum tag_bundle_status tag_bundle_write(
	struct tag_bundle_source const *source,
	long tag_index,
	tag_bundle_write_proc write,
	void *write_context,
	struct tag_bundle_report *report)
{
	struct bundle bundle;
	byte header[TAG_BUNDLE_HEADER_BYTES];
	byte entry[TAG_BUNDLE_TAG_BYTES];
	unsigned long tags_offset = TAG_BUNDLE_HEADER_BYTES;
	unsigned long names_offset;
	unsigned long data_offset;
	unsigned long file_data_offset;
	unsigned long map_name_offset = 0;
	unsigned long map_build_offset = 0;
	unsigned long position;
	long bundle_index;
	long region_index;
	byte *chunk = NULL;

	memset(report, 0, sizeof(*report));
	bundle_layout(&bundle, source, tag_index);

	/* the names: each tag's path, then the map's name and build */
	if (bundle.status == _tag_bundle_status_ok)
	{
		bundle.name_offsets = malloc((unsigned long)bundle.tag_count * sizeof(unsigned long));
		if (!bundle.name_offsets)
			bundle_fail(&bundle, _tag_bundle_status_out_of_memory);
	}
	if (bundle.status == _tag_bundle_status_ok)
	{
		for (bundle_index = 0; bundle_index < bundle.tag_count; bundle_index++)
		{
			bundle.name_offsets[bundle_index] = bundle.names_size;
			bundle.names_size += bundle_name_length(source->tag_header ?
				bundle.header->instances[bundle.tags[bundle_index]].name : NULL) + 1;
		}
		map_name_offset = bundle.names_size;
		bundle.names_size += bundle_string_length(source->map_name) + 1;
		map_build_offset = bundle.names_size;
		bundle.names_size += bundle_string_length(source->map_build) + 1;
	}

	/* the data, copied and made the bundle's */
	if (bundle.status == _tag_bundle_status_ok)
	{
		/* (the game's malloc: what bundle_dispose frees) */
		bundle.data = malloc(bundle.data_size ? bundle.data_size : 1);
		if (!bundle.data)
			bundle_fail(&bundle, _tag_bundle_status_out_of_memory);
		else
			memset(bundle.data, 0, bundle.data_size);
	}
	if (bundle.status == _tag_bundle_status_ok)
	{
		for (region_index = 0; region_index < bundle.region_count; region_index++)
		{
			struct bundle_region const *region = &bundle.regions[region_index];

			memcpy(bundle.data + region->offset, region->address, region->size);
		}
		bundle_walk(&bundle, _walk_copy);
		if (bundle.status == _tag_bundle_status_ok &&
			(bundle.region_cursor != bundle.region_count || bundle.file_region_cursor != bundle.file_region_count))
		{
			bundle_fail(&bundle, _tag_bundle_status_bad_pointer);
		}
	}

	names_offset = tags_offset + (unsigned long)bundle.tag_count * TAG_BUNDLE_TAG_BYTES;
	data_offset = align_up(names_offset + bundle.names_size);
	file_data_offset = align_up(data_offset + bundle.data_size);

	/* the header and the tags */
	if (bundle.status == _tag_bundle_status_ok)
	{
		memset(header, 0, sizeof(header));
		write_unsigned_long(header + TAG_BUNDLE_HEADER_SIGNATURE_OFFSET, TAG_BUNDLE_SIGNATURE);
		write_unsigned_long(header + TAG_BUNDLE_HEADER_VERSION_OFFSET, TAG_BUNDLE_VERSION);
		write_unsigned_long(header + TAG_BUNDLE_HEADER_FLAGS_OFFSET, FLAG(_tag_bundle_custom_edition_bit));
		write_unsigned_long(header + TAG_BUNDLE_HEADER_TAG_COUNT_OFFSET, (unsigned long)bundle.tag_count);
		write_unsigned_long(header + TAG_BUNDLE_HEADER_TAGS_OFFSET, tags_offset);
		write_unsigned_long(header + TAG_BUNDLE_HEADER_NAMES_OFFSET, names_offset);
		write_unsigned_long(header + TAG_BUNDLE_HEADER_NAMES_SIZE_OFFSET, bundle.names_size);
		write_unsigned_long(header + TAG_BUNDLE_HEADER_DATA_OFFSET, data_offset);
		write_unsigned_long(header + TAG_BUNDLE_HEADER_DATA_SIZE_OFFSET, bundle.data_size);
		write_unsigned_long(header + TAG_BUNDLE_HEADER_FILE_DATA_OFFSET, file_data_offset);
		write_unsigned_long(header + TAG_BUNDLE_HEADER_FILE_DATA_SIZE_OFFSET, bundle.file_data_size);
		write_unsigned_long(header + TAG_BUNDLE_HEADER_MAP_NAME_OFFSET, map_name_offset);
		write_unsigned_long(header + TAG_BUNDLE_HEADER_MAP_BUILD_OFFSET, map_build_offset);
		bundle_write_bytes(&bundle, write, write_context, header, sizeof(header));
	}
	for (bundle_index = 0; bundle_index < bundle.tag_count && bundle.status == _tag_bundle_status_ok; bundle_index++)
	{
		struct bundle_instance const *instance = &bundle.header->instances[bundle.tags[bundle_index]];
		struct tag_schema_group const *group = bundle_schema_group(instance->group_tag);

		memset(entry, 0, sizeof(entry));
		write_unsigned_long(entry + TAG_BUNDLE_TAG_GROUP_OFFSET, instance->group_tag);
		write_unsigned_long(entry + TAG_BUNDLE_TAG_PARENT_GROUPS_OFFSET, instance->parent_group_tags[0]);
		write_unsigned_long(entry + TAG_BUNDLE_TAG_PARENT_GROUPS_OFFSET + 4, instance->parent_group_tags[1]);
		write_unsigned_long(entry + TAG_BUNDLE_TAG_NAME_OFFSET, bundle.name_offsets[bundle_index]);
		/* (each tag's root is the first region its walk laid out: the tags'
		roots are found again by their places) */
		write_unsigned_long(entry + TAG_BUNDLE_TAG_ROOT_SIZE_OFFSET, (unsigned long)group->definition->size);
		write_unsigned_long(entry + TAG_BUNDLE_TAG_FLAGS_OFFSET,
			instance->unused[0] ? FLAG(_tag_bundle_tag_from_resource_map_bit) : 0);
		write_unsigned_long(entry + TAG_BUNDLE_TAG_MAP_INDEX_OFFSET, (unsigned long)bundle.tags[bundle_index]);
		for (region_index = 0; region_index < bundle.region_count; region_index++)
		{
			if (bundle.regions[region_index].address == instance->base_address)
			{
				write_unsigned_long(entry + TAG_BUNDLE_TAG_ROOT_OFFSET, bundle.regions[region_index].offset);
				break;
			}
		}
		bundle_write_bytes(&bundle, write, write_context, entry, sizeof(entry));
	}

	/* the names */
	for (bundle_index = 0; bundle_index < bundle.tag_count && bundle.status == _tag_bundle_status_ok; bundle_index++)
	{
		static byte const terminator = 0;
		char const *name = bundle.header->instances[bundle.tags[bundle_index]].name;

		bundle_write_bytes(&bundle, write, write_context, name, bundle_name_length(name));
		bundle_write_bytes(&bundle, write, write_context, &terminator, 1);
	}
	if (bundle.status == _tag_bundle_status_ok)
	{
		bundle_write_bytes(&bundle, write, write_context, source->map_name ? source->map_name : "",
			bundle_string_length(source->map_name) + 1);
		bundle_write_bytes(&bundle, write, write_context, source->map_build ? source->map_build : "",
			bundle_string_length(source->map_build) + 1);
		bundle_write_padding(&bundle, write, write_context, names_offset + bundle.names_size);
	}

	/* the data, then the file data, read a chunk at a time */
	bundle_write_bytes(&bundle, write, write_context, bundle.data, bundle.data_size);
	bundle_write_padding(&bundle, write, write_context, data_offset + bundle.data_size);
	if (bundle.status == _tag_bundle_status_ok)
	{
		chunk = malloc(COPY_CHUNK_SIZE);
		if (!chunk)
			bundle_fail(&bundle, _tag_bundle_status_out_of_memory);
	}
	position = 0;
	for (region_index = 0; region_index < bundle.file_region_count && bundle.status == _tag_bundle_status_ok; region_index++)
	{
		struct bundle_file_region const *region = &bundle.file_regions[region_index];
		unsigned long done = 0;

		bundle_write_padding(&bundle, write, write_context, position);
		position = region->offset;
		while (done < region->size && bundle.status == _tag_bundle_status_ok)
		{
			unsigned long count = region->size - done < COPY_CHUNK_SIZE ? region->size - done : COPY_CHUNK_SIZE;

			if (!source->read(source->context, region->file_offset + done, count, chunk))
			{
				bundle_fail(&bundle, _tag_bundle_status_read_failed);
				break;
			}
			bundle_write_bytes(&bundle, write, write_context, chunk, count);
			done += count;
		}
		position += region->size;
	}
	free_some(chunk);

	report->status = bundle.status;
	report->problem_tag_index = bundle.problem_tag_index;
	report->tag_count = bundle.tag_count;
	report->data_size = bundle.data_size;
	report->file_data_size = bundle.file_data_size;
	report->size = file_data_offset + bundle.file_data_size;
	bundle_dispose(&bundle);

	return report->status;
}


/* tag_bundle_load, of the bundle's first `available` bytes (all of them,
or all but its file data, which is then not read) of `size`; its file
data's regions listed into *regions when it is not NULL */
static enum tag_bundle_status bundle_load(
	byte *bundle,
	unsigned long available,
	unsigned long size,
	struct tag_bundle_loaded *loaded,
	struct tag_bundle_file_region **regions,
	long *region_count)
{
	struct bundle_load load;
	unsigned long tags_offset, names_offset, data_offset, file_data_offset;
	long bundle_index;

	memset(loaded, 0, sizeof(*loaded));
	memset(&load, 0, sizeof(load));
	load.list_regions = regions != NULL;
	if (regions)
	{
		*regions = NULL;
		*region_count = 0;
	}
	load.tag_index = NONE;
	load.problem_tag_index = NONE;
	loaded->problem_tag_index = NONE;
	if (available > size || available < TAG_BUNDLE_HEADER_BYTES ||
		read_unsigned_long(bundle + TAG_BUNDLE_HEADER_SIGNATURE_OFFSET) != TAG_BUNDLE_SIGNATURE ||
		read_unsigned_long(bundle + TAG_BUNDLE_HEADER_VERSION_OFFSET) != TAG_BUNDLE_VERSION)
	{
		return _tag_bundle_status_bad_bundle;
	}
	load.tag_count = (long)read_unsigned_long(bundle + TAG_BUNDLE_HEADER_TAG_COUNT_OFFSET);
	tags_offset = read_unsigned_long(bundle + TAG_BUNDLE_HEADER_TAGS_OFFSET);
	names_offset = read_unsigned_long(bundle + TAG_BUNDLE_HEADER_NAMES_OFFSET);
	load.names_size = read_unsigned_long(bundle + TAG_BUNDLE_HEADER_NAMES_SIZE_OFFSET);
	data_offset = read_unsigned_long(bundle + TAG_BUNDLE_HEADER_DATA_OFFSET);
	load.data_size = read_unsigned_long(bundle + TAG_BUNDLE_HEADER_DATA_SIZE_OFFSET);
	file_data_offset = read_unsigned_long(bundle + TAG_BUNDLE_HEADER_FILE_DATA_OFFSET);
	load.file_data_size = read_unsigned_long(bundle + TAG_BUNDLE_HEADER_FILE_DATA_SIZE_OFFSET);
	/* (each part where the writer puts it, and the file no longer) */
	if (load.tag_count <= 0 || (unsigned long)load.tag_count > size / TAG_BUNDLE_TAG_BYTES ||
		tags_offset != TAG_BUNDLE_HEADER_BYTES ||
		names_offset != tags_offset + (unsigned long)load.tag_count * TAG_BUNDLE_TAG_BYTES ||
		!load.names_size || load.names_size > size || names_offset > size - load.names_size ||
		data_offset != align_up(names_offset + load.names_size) ||
		load.data_size > MAXIMUM_DATA_SIZE || data_offset > size || load.data_size > size - data_offset ||
		file_data_offset != align_up(data_offset + load.data_size) || file_data_offset > size ||
		load.file_data_size != size - file_data_offset ||
		read_unsigned_long(bundle + TAG_BUNDLE_HEADER_MAP_NAME_OFFSET) >= load.names_size ||
		read_unsigned_long(bundle + TAG_BUNDLE_HEADER_MAP_BUILD_OFFSET) >= load.names_size ||
		(available != size && available != file_data_offset))
	{
		return _tag_bundle_status_bad_bundle;
	}
	load.tags = bundle + tags_offset;
	load.names = (char const *)bundle + names_offset;
	load.data = bundle + data_offset;
	if (load.names[load.names_size - 1])
		return _tag_bundle_status_bad_bundle;

	for (bundle_index = 0; bundle_index < load.tag_count && load.status == _tag_bundle_status_ok; bundle_index++)
	{
		struct tag_schema_group const *group = bundle_schema_group(load_tag_field(&load, bundle_index,
			TAG_BUNDLE_TAG_GROUP_OFFSET));
		unsigned long root_size = load_tag_field(&load, bundle_index, TAG_BUNDLE_TAG_ROOT_SIZE_OFFSET);
		byte *root;

		load.tag_index = bundle_index;
		load.depth = 0;
		if (!group || !group->definition || root_size != (unsigned long)group->definition->size ||
			load_tag_field(&load, bundle_index, TAG_BUNDLE_TAG_NAME_OFFSET) >= load.names_size)
		{
			load_fail(&load);
			break;
		}
		root = load_region(&load, load_tag_field(&load, bundle_index, TAG_BUNDLE_TAG_ROOT_OFFSET), root_size);
		if (root)
			load_element(&load, root, group->definition);
	}
	load.tag_index = NONE;
	if (load.status == _tag_bundle_status_ok &&
		(load.cursor != load.data_size || load.file_cursor != load.file_data_size))
	{
		load_fail(&load);
	}

	loaded->problem_tag_index = load.problem_tag_index;
	if (load.status == _tag_bundle_status_ok)
	{
		loaded->tag_count = load.tag_count;
		loaded->tags = load.tags;
		loaded->names = load.names;
		loaded->names_size = load.names_size;
		loaded->data = load.data;
		loaded->data_size = load.data_size;
		loaded->file_data_offset = file_data_offset;
		loaded->file_data = available == size ? bundle + file_data_offset : NULL;
		loaded->file_data_size = load.file_data_size;
	}
	if (regions && load.status == _tag_bundle_status_ok)
	{
		*regions = load.regions;
		*region_count = load.region_count;
	}
	else
		free_some(load.regions);

	return load.status;
}

enum tag_bundle_status tag_bundle_load(
	byte *bundle,
	unsigned long size,
	struct tag_bundle_loaded *loaded)
{
	return bundle_load(bundle, size, size, loaded, NULL, NULL);
}

unsigned long tag_bundle_file_data_offset(
	byte const *header)
{
	return read_unsigned_long(header + TAG_BUNDLE_HEADER_FILE_DATA_OFFSET);
}

enum tag_bundle_status tag_bundle_load_tags(
	byte *bundle,
	unsigned long tags_size,
	unsigned long size,
	struct tag_bundle_loaded *loaded)
{
	return bundle_load(bundle, tags_size, size, loaded, NULL, NULL);
}

enum tag_bundle_status tag_bundle_load_tags_regions(
	byte *bundle,
	unsigned long tags_size,
	unsigned long size,
	struct tag_bundle_loaded *loaded,
	struct tag_bundle_file_region **regions,
	long *region_count)
{
	return bundle_load(bundle, tags_size, size, loaded, regions, region_count);
}
