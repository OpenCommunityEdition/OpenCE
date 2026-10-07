/*
TAG_VALIDATE.C

A map's tags checked against their groups' schemas (tag_schema.h) as they
load, before anything else reads them (scenario_tags_load, and each
structure bsp as it loads: cache_files.c), so that the game can trust what
it reads in them as it trusted its own maps.

The game reads a map's tag data straight into the tag cache and uses it as
its structures: every pointer, count, index and enum in it is the map's,
and the game writes runtime values into tags as it runs. So each tag is
walked once through its group's schema:
- every block's and data's bytes must lie in the tags (or the bsp) and
  overlap no other's, which a map of the game's tools never does: a map
  whose tags overlap could have a runtime value the game writes to one tag
  change a pointer in another. Each one's bytes are marked as they are
  checked (a bit a byte, the claims below), so that the walk also never
  visits a byte twice: a map whose blocks point at each other cannot make
  it take longer than its size;
- counts are cut to what the game has room for, indices, enums and tags
  past what they name are corrected, runtime values reset (tag_schema.h);
- then, once every tag has been through it, the checks that look at other
  tags or at graphs (a bsp's nodes, a model's) run.
A map is refused if any pointer is wrong; anything else is corrected and
logged, and the map plays.

The validator is the game's only reader of a map that is not the game's own
data (a map made by other tools, downloaded, or converted from another
format): it reads nothing but the tags it is given, so tools/map_validate.c
runs it alone on a map file.

A Halo Custom Edition map's tags are checked the same way, once its loader
has read them into their own tag cache and made them this build's where only
their bytes differ (port/linux/game/cache_file_formats.c): its tag header has
no vertex or index buffers, its pixels and samples are in several files, and
its models are gbxmodels, whose parts keep their geometry in the map's model
data and which the game takes as models (tag_schema_custom_edition_groups).
*/

/* ---------- headers */

#include "cseries.h"
#include "cache/physical_memory_map.h"
#include "tag_schema.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* ---------- constants */

enum
{
	TAG_HEADER_SIGNATURE = 'tags',
	STRUCTURE_BSP_HEADER_SIGNATURE = 'sbsp',
	STRUCTURE_BSP_GROUP_TAG = 'sbsp',

	/* a vertex or index buffer in a header (a D3DResource: Common, Data,
	Lock) */
	BUFFER_SIZE = 12,

	/* how deep blocks go in a schema (the deepest are 6 or 7) */
	MAXIMUM_VALIDATION_DEPTH = 32,
	/* how many corrections are logged one by one, for each map or bsp */
	MAXIMUM_LOGGED_CORRECTIONS = 64,
	MAXIMUM_MESSAGE_LENGTH = 512,

	/* the claims: a bit for each byte of the largest tag cache */
	CLAIM_BITS = 32,
	CLAIM_WORDS = TAG_VALIDATE_MAXIMUM_TAG_CACHE_SIZE / CLAIM_BITS,

	/* a Custom Edition map's models (cache_file_formats.c) */
	GBXMODEL_GROUP_TAG = 'mod2',
	MODEL_GROUP_TAG = 'mode',
	TRANSPARENT_CHICAGO_EXTENDED_GROUP_TAG = 'scex',
	TRANSPARENT_CHICAGO_GROUP_TAG = 'schi',
};

/* the passes of the walk over a tag */
enum
{
	/* blocks' and data's counts and bytes */
	_pass_extents,
	/* indices, enums, tags, strings and runtime values */
	_pass_values,
	/* (into each block's elements, which take the same passes) */
	/* the checks (tag_schema_check_proc), after every tag's other passes */
	_pass_checks,
};

/* ---------- structures */

/* a tag in the header's table (cache_files.c's struct
cache_file_tag_instance) */
struct tag_validate_instance
{
	unsigned long group_tag;
	unsigned long parent_group_tags[2];
	long tag_index;
	char *name;
	void *base_address;
	unsigned long unused[2];
};

/* the tags' header (cache_files.c's struct cache_file_tag_header) */
struct tag_validate_header
{
	struct tag_validate_instance *instances;
	long scenario_tag_index;
	unsigned long checksum;
	long tag_count;
	long vertex_buffer_count;
	byte *vertex_buffers;
	long index_buffer_count;
	byte *index_buffers;
	unsigned long signature;
};

/* a bsp's header (cache_files.c's struct cache_file_structure_bsp_header):
its second buffers are its lightmaps' vertex buffers
(structure_bsp_header_register_vertex_buffers), which the validator keeps
as a header's index buffers */
struct tag_validate_structure_bsp_header
{
	void *base_address;
	long vertex_buffer_count;
	byte *vertex_buffers;
	long index_buffer_count;
	byte *index_buffers;
	unsigned long signature;
};

/* a Custom Edition map's tags' header: its model data is in the file, and it
has no buffers */
struct tag_validate_custom_edition_header
{
	struct tag_validate_instance *instances;
	long scenario_tag_index;
	unsigned long checksum;
	long tag_count;
	long model_part_count;
	long model_vertex_data_offset;
	long model_part_count_again;
	long model_index_data_offset;
	long model_data_size;
	unsigned long signature;
};

typedef char verify_tag_validate_maximum_tag_cache_size[TAG_CACHE_SIZE <= TAG_VALIDATE_MAXIMUM_TAG_CACHE_SIZE ? 1 : -1];
typedef char verify_tag_validate_instance_size[sizeof(struct tag_validate_instance) == 0x20 ? 1 : -1];
typedef char verify_tag_validate_custom_edition_header_size[
	sizeof(struct tag_validate_custom_edition_header) == 0x28 ? 1 : -1];
typedef char verify_tag_validate_header_size[sizeof(struct tag_validate_header) == 0x24 ? 1 : -1];
typedef char verify_tag_validate_structure_bsp_header_size[
	sizeof(struct tag_validate_structure_bsp_header) == 0x18 ? 1 : -1];

/* an element being walked: its bytes, its definition, and where it is (for
messages) */
struct tag_validation_frame
{
	byte *base;
	struct tag_schema_definition const *definition;
	char const *field_name;
	long element_index;
};

struct tag_validation
{
	/* what the pointers being checked may point into */
	byte *region;
	unsigned long region_size;
	/* the header whose vertex and index buffers models (or the bsp) use */
	byte *vertex_buffers;
	long vertex_buffer_count;
	byte *index_buffers;
	long index_buffer_count;

	long tag_index;
	short pass;
	boolean refused;
	long corrections;

	struct tag_validation_frame frames[MAXIMUM_VALIDATION_DEPTH];
	short depth;
	/* the structure the field being checked is in (TAG_SCHEMA_STRUCTURE) */
	byte *structure;
	char const *field_name;
};

/* ---------- globals */

static struct
{
	/* the tags last checked (tag_validate_tags), in a tag cache of
	tag_cache_size bytes */
	struct tag_validate_header *header;
	long tag_data_size;
	unsigned long tag_cache_size;
	/* where their data in files may be: a map's file, or a Custom Edition
	map's files */
	struct tag_validate_file_range file_ranges[MAXIMUM_TAG_VALIDATE_FILE_RANGES];
	short file_range_count;
	boolean custom_edition;
	char map_name[64];
	long corrections;
} tag_validate_globals;

/* a bit for each byte of the tag cache that a tag's root, block or data
holds */
static unsigned long tag_validate_claims[CLAIM_WORDS];

static char const tag_validate_empty_name[] = "";

/* ---------- prototypes */

static void validate_element(struct tag_validation *validation, byte *base,
	struct tag_schema_definition const *definition, char const *field_name, long element_index);
static void validate_fields(struct tag_validation *validation, byte *base,
	struct tag_schema_definition const *definition);

/* ---------- private code */

static struct tag_schema_group const *schema_group_get(
	unsigned long group_tag)
{
	struct tag_schema_group const *const *list;

	/* (a Custom Edition map's groups laid out otherwise come first) */
	if (tag_validate_globals.custom_edition)
	{
		struct tag_schema_group const *group;

		for (group = tag_schema_custom_edition_groups; group->group_tag; group++)
		{
			if (group->group_tag == group_tag)
				return group;
		}
	}

	for (list = tag_schema_group_lists; *list; list++)
	{
		struct tag_schema_group const *group;

		for (group = *list; group->group_tag; group++)
		{
			if (group->group_tag == group_tag)
				return group;
		}
	}

	return NULL;
}

static char *tag_to_text(
	unsigned long group_tag,
	char text[5])
{
	short index;

	for (index = 0; index < 4; index++)
	{
		char c = (char)(group_tag >> (8 * (3 - index)));

		text[index] = c >= ' ' && c < 0x7F ? c : '?';
	}
	text[4] = 0;

	return text;
}

/* the name of tag_index, if it has one in the tags */
static char const *tag_name(
	long tag_index)
{
	struct tag_validate_header *header = tag_validate_globals.header;
	short absolute_index = (short)tag_index;
	char const *name;
	unsigned long offset;

	if (!header || absolute_index < 0 || absolute_index >= header->tag_count)
		return "?";
	name = header->instances[absolute_index].name;
	if (name == tag_validate_empty_name)
		return name;
	offset = (unsigned long)name - (unsigned long)header;
	if ((unsigned long)name < (unsigned long)header || offset >= (unsigned long)tag_validate_globals.tag_data_size ||
		!memchr(name, 0, tag_validate_globals.tag_data_size - offset))
	{
		return "?";
	}

	return name;
}

/* where the field being checked is: tag 'name' (grou): block[3].field */
static void validation_where(
	struct tag_validation *validation,
	char *where,
	size_t size)
{
	struct tag_validate_header *header = tag_validate_globals.header;
	short absolute_index = (short)validation->tag_index;
	char group[5] = "?";
	size_t length;
	short depth;

	if (header && absolute_index >= 0 && absolute_index < header->tag_count)
		tag_to_text(header->instances[absolute_index].group_tag, group);
	snprintf(where, size, "tag '%s' (%s): ", tag_name(validation->tag_index), group);
	for (depth = 1; depth < validation->depth; depth++)
	{
		length = strlen(where);
		snprintf(where + length, size - length, "%s[%ld].",
			validation->frames[depth].field_name, validation->frames[depth].element_index);
	}
	length = strlen(where);
	snprintf(where + length, size - length, "%s", validation->field_name ? validation->field_name : "");
}

static void validation_message(
	struct tag_validation *validation,
	char const *what,
	char const *format,
	va_list arguments)
{
	char message[MAXIMUM_MESSAGE_LENGTH];
	size_t length;

	snprintf(message, sizeof(message), "the map '%s' %s: ", tag_validate_globals.map_name, what);
	length = strlen(message);
	validation_where(validation, message + length, sizeof(message) - length);
	length = strlen(message);
	snprintf(message + length, sizeof(message) - length, " ");
	length = strlen(message);
	vsnprintf(message + length, sizeof(message) - length, format, arguments);
	tag_validate_report(message);
}

/* whether size bytes at address lie in the region being checked */
static boolean region_contains(
	struct tag_validation *validation,
	void const *address,
	unsigned long size)
{
	unsigned long offset = (unsigned long)address - (unsigned long)validation->region;

	return (unsigned long)address >= (unsigned long)validation->region &&
		offset <= validation->region_size &&
		size <= validation->region_size - offset;
}

/* marks size bytes at address (in the tag cache, as the region is) as a
tag's: FALSE if any of them already were */
static boolean claim(
	void const *address,
	unsigned long size)
{
	byte const *tag_cache = (byte const *)tag_validate_globals.header;
	unsigned long first = (unsigned long)((byte const *)address - tag_cache);
	unsigned long end = first + size;
	unsigned long bit;

	if (!size)
		return TRUE;
	if (first > tag_validate_globals.tag_cache_size || size > tag_validate_globals.tag_cache_size - first)
		return FALSE;

	/* (whole words where they can be) */
	bit = first;
	while (bit < end)
	{
		unsigned long *word = &tag_validate_claims[bit / CLAIM_BITS];

		if (bit % CLAIM_BITS == 0 && end - bit >= CLAIM_BITS)
		{
			if (*word)
				return FALSE;
			*word = 0xFFFFFFFFUL;
			bit += CLAIM_BITS;
		}
		else
		{
			unsigned long mask = 1UL << (bit % CLAIM_BITS);

			if (*word & mask)
				return FALSE;
			*word |= mask;
			bit++;
		}
	}

	return TRUE;
}

/* unclaims the size bytes at the tag cache's offset first (a bsp's, as
another loads in its place) */
static void unclaim(
	unsigned long first,
	unsigned long size)
{
	unsigned long end = first + size;
	unsigned long bit;

	if (first > tag_validate_globals.tag_cache_size || size > tag_validate_globals.tag_cache_size - first)
		return;
	for (bit = first; bit < end; )
	{
		if (bit % CLAIM_BITS == 0 && end - bit >= CLAIM_BITS)
		{
			tag_validate_claims[bit / CLAIM_BITS] = 0;
			bit += CLAIM_BITS;
		}
		else
		{
			tag_validate_claims[bit / CLAIM_BITS] &= ~(1UL << (bit % CLAIM_BITS));
			bit++;
		}
	}
}

/* whether a string ends in the region, or in the tags (where a bsp's tag
references' names are) */
static boolean string_valid(
	struct tag_validation *validation,
	char const *string)
{
	byte const *tags = (byte const *)tag_validate_globals.header;
	unsigned long offset = (unsigned long)string - (unsigned long)validation->region;
	unsigned long tags_offset = (unsigned long)string - (unsigned long)tags;

	/* (the name a nameless tag or reference is given) */
	if (string == tag_validate_empty_name)
		return TRUE;
	if (region_contains(validation, string, 1))
		return memchr(string, 0, validation->region_size - offset) != NULL;
	if (tags && (unsigned long)string >= (unsigned long)tags &&
		tags_offset < (unsigned long)tag_validate_globals.tag_data_size)
	{
		return memchr(string, 0, tag_validate_globals.tag_data_size - tags_offset) != NULL;
	}

	return FALSE;
}

/* the tag tag_index names, if it is one of the map's: NULL otherwise */
static struct tag_validate_instance *instance_get(
	long tag_index)
{
	struct tag_validate_header *header = tag_validate_globals.header;
	short absolute_index = (short)tag_index;
	struct tag_validate_instance *instance;

	if (!header || tag_index == NONE || absolute_index < 0 || absolute_index >= header->tag_count)
		return NULL;
	instance = &header->instances[absolute_index];

	return instance->tag_index == tag_index ? instance : NULL;
}

/* whether a tag is of one of groups (a list ending with 0; NULL for any) */
static boolean instance_in_groups(
	struct tag_validate_instance const *instance,
	unsigned long const *groups)
{
	if (!groups)
		return TRUE;
	for (; *groups; groups++)
	{
		/* (a Custom Edition map's gbxmodels are the game's models) */
		if (tag_validate_globals.custom_edition && instance->group_tag == GBXMODEL_GROUP_TAG && *groups == MODEL_GROUP_TAG)
			return TRUE;
		if (instance->group_tag == *groups ||
			instance->parent_group_tags[0] == *groups ||
			instance->parent_group_tags[1] == *groups)
		{
			return TRUE;
		}
	}

	return FALSE;
}

static char const *groups_text(
	unsigned long const *groups,
	char *text,
	size_t size)
{
	char group[5];

	text[0] = 0;
	if (!groups)
		return "any";
	for (; *groups && strlen(text) + 6 < size; groups++)
	{
		if (text[0])
			strcat(text, ",");
		strcat(text, tag_to_text(*groups, group));
	}

	return text;
}

static long integer_get(
	byte const *address,
	short size,
	boolean is_unsigned)
{
	switch (size)
	{
	case 1:
		return is_unsigned ? (long)*address : (long)*(signed char const *)address;
	case 2:
		return is_unsigned ? (long)*(unsigned short const *)address : (long)*(short const *)address;
	default:
		return *(long const *)address;
	}
}

static void integer_set(
	byte *address,
	short size,
	long value)
{
	switch (size)
	{
	case 1:
		*address = (byte)value;
		break;
	case 2:
		*(short *)address = (short)value;
		break;
	default:
		*(long *)address = value;
		break;
	}
}

/* NONE in an integer of size bytes, as it reads (an unsigned byte's NONE is
0xFF) */
static long integer_none(
	short size,
	boolean is_unsigned)
{
	if (!is_unsigned)
		return NONE;

	return size == 1 ? 0xFF : size == 2 ? 0xFFFF : NONE;
}

/* the block a block index indexes */
static struct tag_block const *block_index_target(
	struct tag_validation *validation,
	struct tag_schema_field const *field)
{
	byte *base;

	if (field->target_level == TAG_SCHEMA_ROOT)
		base = validation->frames[0].base;
	else if (field->target_level == TAG_SCHEMA_STRUCTURE)
		base = validation->structure;
	else if (field->target_level >= 0 && field->target_level < validation->depth)
		base = validation->frames[validation->depth - 1 - field->target_level].base;
	else
		return NULL;

	return (struct tag_block const *)(base + field->target_offset);
}

static void validate_block_extent(
	struct tag_validation *validation,
	struct tag_block *block,
	struct tag_schema_field const *field)
{
	struct tag_schema_definition const *definition = field->definition;

	block->definition = NULL;
	if (block->count < 0)
	{
		tag_validate_refuse(validation, "has %ld elements", block->count);
		return;
	}
	if (field->maximum > 0 && block->count > field->maximum)
	{
		tag_validate_correct(validation, "has %ld elements, more than the game's %ld: cut to %ld",
			block->count, field->maximum, field->maximum);
		block->count = field->maximum;
	}
	if (block->count &&
		((unsigned long)block->count > validation->region_size / (unsigned long)definition->size ||
			!region_contains(validation, block->address, (unsigned long)block->count * definition->size)))
	{
		tag_validate_refuse(validation, "has %ld elements of %ld bytes at %08lx, outside the tags",
			block->count, definition->size, (unsigned long)block->address);
		return;
	}
	if (!claim(block->address, (unsigned long)block->count * definition->size))
	{
		tag_validate_refuse(validation, "has %ld elements of %ld bytes at %08lx, which overlap another's",
			block->count, definition->size, (unsigned long)block->address);
		return;
	}
	if (!block->count)
		block->address = NULL;

	return;
}

static void validate_data_extent(
	struct tag_validation *validation,
	struct tag_data *data,
	struct tag_schema_field const *field)
{
	data->definition = NULL;
	if (data->size < 0)
	{
		tag_validate_refuse(validation, "has %ld bytes", data->size);
		return;
	}
	if (field->maximum > 0 && data->size > field->maximum)
	{
		tag_validate_correct(validation, "has %ld bytes, more than the game's %ld: cut to %ld",
			data->size, field->maximum, field->maximum);
		data->size = field->maximum;
	}
	if (field->type == _tag_schema_file_data)
	{
		if (!tag_validate_file_contains(validation, data->file_offset, data->size))
		{
			tag_validate_refuse(validation, "has %ld bytes at %08lx, outside the map's files",
				data->size, (unsigned long)data->file_offset);
		}
		return;
	}
	if (data->size &&
		(!region_contains(validation, data->address, data->size) || !claim(data->address, data->size)))
	{
		tag_validate_refuse(validation, "has %ld bytes at %08lx, outside the tags or overlapping another's",
			data->size, (unsigned long)data->address);
		return;
	}
	if (!data->size)
		data->address = NULL;

	return;
}

static void validate_tag_index(
	struct tag_validation *validation,
	long *tag_index,
	unsigned long *group_tag,
	unsigned long const *groups)
{
	struct tag_validate_instance *instance;
	char text[96];

	if (*tag_index == NONE)
		return;
	instance = instance_get(*tag_index);
	if (!instance || !instance_in_groups(instance, groups))
	{
		tag_validate_correct(validation, "names %08lx, not a tag of %s: none",
			*tag_index, groups_text(groups, text, sizeof(text)));
		*tag_index = NONE;
		return;
	}
	/* (a Custom Edition map's transparent chicago extended shaders are its
	loader's transparent chicago shaders now, cache_file_formats.c: the
	references that say so are not wrong) */
	if (group_tag && tag_validate_globals.custom_edition &&
		*group_tag == TRANSPARENT_CHICAGO_EXTENDED_GROUP_TAG && instance->group_tag == TRANSPARENT_CHICAGO_GROUP_TAG)
	{
		*group_tag = instance->group_tag;
	}
	/* (what the game takes the reference's tag to be: the tag's group) */
	if (group_tag && *group_tag != instance->group_tag)
	{
		char said[5];
		char group[5];

		tag_validate_correct(validation, "says '%s' of a '%s' tag", tag_to_text(*group_tag, said),
			tag_to_text(instance->group_tag, group));
		*group_tag = instance->group_tag;
	}

	return;
}

static void validate_value(
	struct tag_validation *validation,
	byte *address,
	struct tag_schema_field const *field)
{
	boolean is_unsigned = TEST_FLAG(field->flags, _tag_schema_unsigned_bit);
	boolean none_allowed = TEST_FLAG(field->flags, _tag_schema_none_bit);

	switch (field->type)
	{
	case _tag_schema_reference:
	{
		struct tag_reference *reference = (struct tag_reference *)address;

		/* (a reference to no tag often has a name pointer that is not one,
		in the retail maps too: only one to a tag counts as a correction) */
		if (!string_valid(validation, reference->name))
		{
			if (reference->index != NONE)
				tag_validate_correct(validation, "has a name outside the tags: none");
			reference->name = (char *)tag_validate_empty_name;
		}
		validate_tag_index(validation, &reference->index, &reference->group_tag, field->definition);
		break;
	}
	case _tag_schema_tag_index:
		validate_tag_index(validation, (long *)address, NULL, field->definition);
		break;
	case _tag_schema_block_index:
	{
		struct tag_block const *target = block_index_target(validation, field);
		long value = integer_get(address, field->size, is_unsigned);
		long none = integer_none(field->size, is_unsigned);
		long count = target ? target->count : 0;

		/* (an empty block's index is none, as below, whether or not it
		may be otherwise) */
		if ((value >= 0 && value < count && value != none) || ((none_allowed || !count) && value == none))
			break;
		tag_validate_correct(validation, "is %ld, past its block's %ld: %s",
			value, count, none_allowed || !count ? "none" : "0");
		integer_set(address, field->size, none_allowed || !count ? NONE : 0);
		break;
	}
	case _tag_schema_enum:
	{
		long value = integer_get(address, field->size, is_unsigned);
		long none = integer_none(field->size, is_unsigned);

		if ((value >= 0 && value < field->maximum && value != none) || (none_allowed && value == none))
			break;
		tag_validate_correct(validation, "is %ld, past its %ld values: %s",
			value, field->maximum, none_allowed ? "none" : "0");
		integer_set(address, field->size, none_allowed ? NONE : 0);
		break;
	}
	case _tag_schema_string:
		if (!memchr(address, 0, field->size))
		{
			tag_validate_correct(validation, "is not terminated");
			address[field->size - 1] = 0;
		}
		break;
	case _tag_schema_reset:
		if (field->size <= 4)
			integer_set(address, field->size, field->target_offset);
		else
			memset(address, 0, field->size);
		break;
	}

	return;
}

/* one pass over an element's fields (those of the structures in it too) */
static void validate_fields(
	struct tag_validation *validation,
	byte *base,
	struct tag_schema_definition const *definition)
{
	struct tag_schema_field const *field;
	byte *structure = validation->structure;

	validation->structure = base;
	for (field = definition->fields; field->type != _tag_schema_terminator && !validation->refused; field++)
	{
		short index;

		validation->field_name = field->name;
		for (index = 0; index < field->count && !validation->refused; index++)
		{
			byte *address = base + field->offset + index * field->size;

			switch (field->type)
			{
			case _tag_schema_struct:
				validate_fields(validation, address, field->definition);
				validation->field_name = field->name;
				break;

			case _tag_schema_block:
				if (validation->pass == _pass_extents)
				{
					validate_block_extent(validation, (struct tag_block *)address, field);
				}
				else
				{
					struct tag_block *block = (struct tag_block *)address;
					long element_index;

					/* (the elements, through every pass) */
					for (element_index = 0;
						element_index < block->count && !validation->refused;
						element_index++)
					{
						validate_element(
							validation,
							(byte *)block->address + element_index * ((struct tag_schema_definition const *)field->definition)->size,
							field->definition,
							field->name,
							element_index);
					}
					validation->field_name = field->name;
				}
				break;

			case _tag_schema_data:
			case _tag_schema_file_data:
				if (validation->pass == _pass_extents)
					validate_data_extent(validation, (struct tag_data *)address, field);
				break;

			case _tag_schema_check:
				if (validation->pass == _pass_checks && !field->check(validation, base))
				{
					if (!validation->refused)
						tag_validate_refuse(validation, "failed its check");
				}
				break;

			default:
				if (validation->pass == _pass_values)
					validate_value(validation, address, field);
				break;
			}
		}
	}
	validation->structure = structure;

	return;
}

/* an element, and the elements of its blocks: in the first walk, its
extents then its values then its blocks' elements (so that an index's
block is checked before the index); in the checks' walk, its checks then
its blocks' elements */
static void validate_element(
	struct tag_validation *validation,
	byte *base,
	struct tag_schema_definition const *definition,
	char const *field_name,
	long element_index)
{
	struct tag_validation_frame *frame;
	short pass = validation->pass;

	if (validation->depth >= MAXIMUM_VALIDATION_DEPTH)
	{
		tag_validate_refuse(validation, "is too deep");
		return;
	}
	frame = &validation->frames[validation->depth++];
	frame->base = base;
	frame->definition = definition;
	frame->field_name = field_name;
	frame->element_index = element_index;

	if (pass == _pass_checks)
	{
		/* (its checks, then its blocks': validate_fields does both) */
		validate_fields(validation, base, definition);
	}
	else
	{
		validation->pass = _pass_extents;
		validate_fields(validation, base, definition);
		if (!validation->refused)
		{
			validation->pass = _pass_values;
			validate_fields(validation, base, definition);
		}
		validation->pass = pass;
	}
	validation->depth--;

	return;
}

/* the blocks' elements are walked in the values pass (validate_fields); the
first walk starts each tag there */
static void validate_tag(
	struct tag_validation *validation,
	long tag_index,
	void *root,
	struct tag_schema_definition const *definition,
	short pass)
{
	validation->tag_index = tag_index;
	validation->depth = 0;
	validation->structure = root;
	validation->field_name = NULL;
	validation->pass = pass;
	validate_element(validation, root, definition, NULL, NONE);

	return;
}

/* whether a definition's fields (and those of the structures in it) all
lie within it, as the walk assumes: a field past its definition's size
would be read and written in bytes that another tag may hold. A mistake
in a schema, reported (once) and the map refused, never trusted */
static boolean definition_fits(
	struct tag_schema_definition const *definition,
	short depth)
{
	struct tag_schema_field const *field;

	if (depth > MAXIMUM_VALIDATION_DEPTH)
		return FALSE;
	for (field = definition->fields; field->type != _tag_schema_terminator; field++)
	{
		/* (a structure's bytes are its definition's: the last of an array
		ends there) */
		long end = field->type == _tag_schema_struct && field->count >= 1 ?
			field->offset + (long)field->size * (field->count - 1) +
				((struct tag_schema_definition const *)field->definition)->size :
			field->offset + (long)field->size * field->count;

		if (field->type != _tag_schema_check &&
			(field->offset < 0 || field->count < 1 || end > definition->size))
		{
			char message[MAXIMUM_MESSAGE_LENGTH];

			snprintf(message, sizeof(message), "the tag schema '%s' has its field '%s' at %ld..%ld, past its %ld bytes",
				definition->name, field->name, field->offset, end, definition->size);
			tag_validate_report(message);
			return FALSE;
		}
		if ((field->type == _tag_schema_struct || field->type == _tag_schema_block) &&
			!definition_fits(field->definition, depth + 1))
		{
			return FALSE;
		}
		if (field->type == _tag_schema_struct &&
			((struct tag_schema_definition const *)field->definition)->size > field->size)
		{
			char message[MAXIMUM_MESSAGE_LENGTH];

			snprintf(message, sizeof(message), "the tag schema '%s' has its structure '%s' of %ld bytes in %d",
				definition->name, field->name, ((struct tag_schema_definition const *)field->definition)->size,
				field->size);
			tag_validate_report(message);
			return FALSE;
		}
	}

	return TRUE;
}

/* whether every group's schema fits (definition_fits), found once */
static boolean schemas_fit(
	void)
{
	static short fit = NONE;

	if (fit == NONE)
	{
		struct tag_schema_group const *const *list;

		fit = TRUE;
		for (list = tag_schema_group_lists; *list && fit; list++)
		{
			struct tag_schema_group const *group;

			for (group = *list; group->group_tag && fit; group++)
			{
				if (group->definition && !definition_fits(group->definition, 0))
					fit = FALSE;
			}
		}
	}

	return (boolean)fit;
}

/* a tag of the table through a pass of its group's schema, if it has one */
static void validate_instance(
	struct tag_validation *validation,
	struct tag_validate_instance *instance,
	short pass)
{
	struct tag_schema_group const *group = schema_group_get(instance->group_tag);

	if (group && group->definition && instance->base_address)
		validate_tag(validation, instance->tag_index, instance->base_address, group->definition, pass);

	return;
}

static void validation_new(
	struct tag_validation *validation,
	void *region,
	unsigned long region_size)
{
	memset(validation, 0, sizeof(*validation));
	validation->region = region;
	validation->region_size = region_size;
	validation->tag_index = NONE;

	return;
}

/* a header's vertex and index buffers: in the region and claimed, their
data in the region */
static boolean validate_buffers(
	struct tag_validation *validation,
	byte *vertex_buffers,
	long vertex_buffer_count,
	byte *index_buffers,
	long index_buffer_count)
{
	long index;

	validation->vertex_buffers = vertex_buffers;
	validation->vertex_buffer_count = vertex_buffer_count;
	validation->index_buffers = index_buffers;
	validation->index_buffer_count = index_buffer_count;
	if (vertex_buffer_count < 0 || index_buffer_count < 0 ||
		(unsigned long)vertex_buffer_count > validation->region_size / BUFFER_SIZE ||
		(unsigned long)index_buffer_count > validation->region_size / BUFFER_SIZE ||
		(vertex_buffer_count && !region_contains(validation, vertex_buffers, vertex_buffer_count * BUFFER_SIZE)) ||
		(index_buffer_count && !region_contains(validation, index_buffers, index_buffer_count * BUFFER_SIZE)) ||
		!claim(vertex_buffers, vertex_buffer_count * BUFFER_SIZE) ||
		!claim(index_buffers, index_buffer_count * BUFFER_SIZE))
	{
		tag_validate_refuse(validation, "has its vertex or index buffers outside the tags");
		return FALSE;
	}
	for (index = 0; index < vertex_buffer_count + index_buffer_count; index++)
	{
		byte *buffer = index < vertex_buffer_count ?
			vertex_buffers + index * BUFFER_SIZE :
			index_buffers + (index - vertex_buffer_count) * BUFFER_SIZE;
		/* (the buffer's Data, its bytes' address before it is registered) */
		void *data = *(void **)(buffer + 4);

		if (!region_contains(validation, data, 1))
		{
			tag_validate_refuse(validation, "has a vertex or index buffer's data at %08lx, outside the tags",
				(unsigned long)data);
			return FALSE;
		}
	}

	return TRUE;
}

/* ---------- public code */

/* the tags at tag_header (tag_data_size bytes of a tag cache), whose header
the caller has checked and claimed: each tag's table entry, then every tag
through its schema, then every tag's checks */
static boolean validate_tag_table(
	struct tag_validation *validation,
	struct tag_validate_instance *instances,
	long tag_count)
{
	long absolute_index;

	/* the tag table: each tag's index, name, groups and root */
	for (absolute_index = 0; absolute_index < tag_count && !validation->refused; absolute_index++)
	{
		struct tag_validate_instance *instance = &instances[absolute_index];
		struct tag_schema_group const *group = schema_group_get(instance->group_tag);
		unsigned long parent_group_tags[2];

		validation->tag_index = instance->tag_index;
		if ((short)instance->tag_index != absolute_index)
		{
			tag_validate_refuse(validation, "has tag %08lx in the table's place %ld", instance->tag_index,
				absolute_index);
			break;
		}
		if (!string_valid(validation, instance->name))
		{
			instance->name = (char *)tag_validate_empty_name;
			tag_validate_correct(validation, "has no name");
		}
		/* (a group's parents are what the game asks a tag's group by
		(tag_get): they are the group's, not the map's) */
		parent_group_tags[0] = group ? group->parent_group_tags[0] : NONE;
		parent_group_tags[1] = group ? group->parent_group_tags[1] : NONE;
		if (instance->parent_group_tags[0] != parent_group_tags[0] ||
			instance->parent_group_tags[1] != parent_group_tags[1])
		{
			char text[5];

			tag_validate_correct(validation, "has the wrong parent groups for '%s'",
				tag_to_text(instance->group_tag, text));
			instance->parent_group_tags[0] = parent_group_tags[0];
			instance->parent_group_tags[1] = parent_group_tags[1];
		}
		/* (a structure bsp's root is where it loads, set as it does) */
		if (instance->group_tag == STRUCTURE_BSP_GROUP_TAG)
		{
			instance->base_address = NULL;
			continue;
		}
		if (!instance->base_address)
		{
			tag_validate_refuse(validation, "has no data");
			break;
		}
		if (group && group->definition &&
			(!region_contains(validation, instance->base_address, group->definition->size) ||
				!claim(instance->base_address, group->definition->size)))
		{
			tag_validate_refuse(validation, "has its data at %08lx, outside the tags or overlapping another's",
				(unsigned long)instance->base_address);
			break;
		}
	}

	/* every tag through its schema, then every tag's checks */
	for (absolute_index = 0; absolute_index < tag_count && !validation->refused; absolute_index++)
		validate_instance(validation, &instances[absolute_index], _pass_values);
	for (absolute_index = 0; absolute_index < tag_count && !validation->refused; absolute_index++)
		validate_instance(validation, &instances[absolute_index], _pass_checks);

	tag_validate_globals.corrections = validation->corrections;

	return !validation->refused;
}

/* the globals for checking tags in a tag cache of tag_cache_size bytes, and
the validation of the tag_data_size bytes at tag_header */
static void validation_begin(
	struct tag_validation *validation,
	void *tag_header,
	long tag_data_size,
	unsigned long tag_cache_size,
	boolean custom_edition,
	char const *map_name)
{
	memset(&tag_validate_globals, 0, sizeof(tag_validate_globals));
	tag_validate_globals.header = tag_header;
	tag_validate_globals.tag_data_size = tag_data_size;
	tag_validate_globals.tag_cache_size = tag_cache_size;
	tag_validate_globals.custom_edition = custom_edition;
	snprintf(tag_validate_globals.map_name, sizeof(tag_validate_globals.map_name), "%s", map_name);
	memset(tag_validate_claims, 0, sizeof(tag_validate_claims));
	validation_new(validation, tag_header, (unsigned long)tag_data_size);

	return;
}

boolean tag_validate_tags(
	void *tag_header,
	long tag_data_size,
	long file_length,
	char const *map_name)
{
	struct tag_validate_header *header = tag_header;
	struct tag_validation validation;

	validation_begin(&validation, tag_header, tag_data_size, TAG_CACHE_SIZE, FALSE, map_name);
	tag_validate_globals.file_ranges[0].offset = 0;
	tag_validate_globals.file_ranges[0].size = file_length < 0 ? 0 : (unsigned long)file_length;
	tag_validate_globals.file_range_count = 1;

	if (!schemas_fit())
	{
		tag_validate_refuse(&validation, "cannot be checked: a tag schema is wrong");
		return FALSE;
	}
	if (tag_data_size < (long)sizeof(*header) || tag_data_size > TAG_CACHE_SIZE ||
		header->signature != TAG_HEADER_SIGNATURE ||
		header->tag_count <= 0 || header->tag_count > UNSIGNED_SHORT_MAX ||
		!region_contains(&validation, header->instances, header->tag_count * sizeof(struct tag_validate_instance)))
	{
		tag_validate_refuse(&validation, "has a damaged tag header");
		return FALSE;
	}
	if (!claim(header, sizeof(*header)) ||
		!claim(header->instances, header->tag_count * sizeof(struct tag_validate_instance)) ||
		!validate_buffers(&validation, header->vertex_buffers, header->vertex_buffer_count,
			header->index_buffers, header->index_buffer_count))
	{
		if (!validation.refused)
			tag_validate_refuse(&validation, "has a damaged tag header");
		return FALSE;
	}

	return validate_tag_table(&validation, header->instances, header->tag_count);
}

boolean tag_validate_custom_edition_tags(
	void *tag_header,
	long loaded_size,
	unsigned long tag_cache_size,
	struct tag_validate_file_range const *file_ranges,
	short file_range_count,
	char const *map_name)
{
	struct tag_validate_custom_edition_header *header = tag_header;
	struct tag_validation validation;
	short range_index;

	validation_begin(&validation, tag_header, loaded_size, tag_cache_size, TRUE, map_name);
	for (range_index = 0; range_index < file_range_count && range_index < MAXIMUM_TAG_VALIDATE_FILE_RANGES; range_index++)
		tag_validate_globals.file_ranges[range_index] = file_ranges[range_index];
	tag_validate_globals.file_range_count = range_index;

	if (!schemas_fit())
	{
		tag_validate_refuse(&validation, "cannot be checked: a tag schema is wrong");
		return FALSE;
	}
	if (tag_cache_size > TAG_VALIDATE_MAXIMUM_TAG_CACHE_SIZE ||
		loaded_size < (long)sizeof(*header) || (unsigned long)loaded_size > tag_cache_size ||
		header->signature != TAG_HEADER_SIGNATURE ||
		header->tag_count <= 0 || header->tag_count > UNSIGNED_SHORT_MAX ||
		!region_contains(&validation, header->instances, header->tag_count * sizeof(struct tag_validate_instance)) ||
		!claim(header, sizeof(*header)) ||
		!claim(header->instances, header->tag_count * sizeof(struct tag_validate_instance)))
	{
		tag_validate_refuse(&validation, "has a damaged tag header");
		return FALSE;
	}
	/* (no buffers: its models and bsps are given theirs as they are made
	this build's, custom_edition_geometry.c) */
	validate_buffers(&validation, NULL, 0, NULL, 0);

	return validate_tag_table(&validation, header->instances, header->tag_count);
}

boolean tag_validate_structure_bsp(
	long tag_index,
	void *base,
	long size)
{
	struct tag_validate_header *header = tag_validate_globals.header;
	struct tag_validate_structure_bsp_header *bsp_header = base;
	struct tag_validate_instance *instance = instance_get(tag_index);
	struct tag_schema_group const *group = schema_group_get(STRUCTURE_BSP_GROUP_TAG);
	struct tag_validation validation;
	unsigned long tag_cache_offset = (unsigned long)((byte *)base - (byte *)header);

	validation_new(&validation, base, (unsigned long)size);
	validation.tag_index = tag_index;
	if (!header || !instance || instance->group_tag != STRUCTURE_BSP_GROUP_TAG ||
		size < (long)sizeof(*bsp_header) ||
		tag_cache_offset < (unsigned long)tag_validate_globals.tag_data_size ||
		tag_cache_offset > tag_validate_globals.tag_cache_size ||
		(unsigned long)size > tag_validate_globals.tag_cache_size - tag_cache_offset)
	{
		tag_validate_refuse(&validation, "has a structure bsp outside the tag cache");
		return FALSE;
	}
	/* (another bsp may have been where this one is) */
	unclaim(tag_validate_globals.tag_data_size, tag_validate_globals.tag_cache_size - tag_validate_globals.tag_data_size);

	if (bsp_header->signature != STRUCTURE_BSP_HEADER_SIGNATURE ||
		!claim(bsp_header, sizeof(*bsp_header)) ||
		!validate_buffers(&validation, bsp_header->vertex_buffers, bsp_header->vertex_buffer_count,
			bsp_header->index_buffers, bsp_header->index_buffer_count))
	{
		if (!validation.refused)
			tag_validate_refuse(&validation, "has a damaged structure bsp header");
		return FALSE;
	}
	if (group && group->definition)
	{
		if (!region_contains(&validation, bsp_header->base_address, group->definition->size) ||
			!claim(bsp_header->base_address, group->definition->size))
		{
			tag_validate_refuse(&validation, "has its structure bsp at %08lx, outside it",
				(unsigned long)bsp_header->base_address);
			return FALSE;
		}
		validate_tag(&validation, tag_index, bsp_header->base_address, group->definition, _pass_values);
		if (!validation.refused)
			validate_tag(&validation, tag_index, bsp_header->base_address, group->definition, _pass_checks);
	}
	tag_validate_globals.corrections += validation.corrections;

	return !validation.refused;
}

long tag_validate_corrections(
	void)
{
	return tag_validate_globals.corrections;
}

/* (for tools/map_validate.c's fuzzing: which bytes the last validation
found to be a tag's root, block or data) */
boolean tag_validate_claimed(
	void const *address)
{
	unsigned long offset = (unsigned long)address - (unsigned long)tag_validate_globals.header;

	if (!tag_validate_globals.header || (unsigned long)address < (unsigned long)tag_validate_globals.header ||
		offset >= tag_validate_globals.tag_cache_size)
	{
		return FALSE;
	}

	return (tag_validate_claims[offset / CLAIM_BITS] & (1UL << (offset % CLAIM_BITS))) != 0;
}

void tag_validate_refuse(
	struct tag_validation *validation,
	char const *format,
	...)
{
	va_list arguments;

	validation->refused = TRUE;
	va_start(arguments, format);
	validation_message(validation, "cannot be played", format, arguments);
	va_end(arguments);

	return;
}

void tag_validate_correct(
	struct tag_validation *validation,
	char const *format,
	...)
{
	va_list arguments;

	validation->corrections++;
	if (validation->corrections <= MAXIMUM_LOGGED_CORRECTIONS)
	{
		va_start(arguments, format);
		validation_message(validation, "is corrected", format, arguments);
		va_end(arguments);
	}
	else if (validation->corrections == MAXIMUM_LOGGED_CORRECTIONS + 1)
	{
		char message[MAXIMUM_MESSAGE_LENGTH];

		snprintf(message, sizeof(message), "the map '%s' has more corrections, not logged",
			tag_validate_globals.map_name);
		tag_validate_report(message);
	}

	return;
}

boolean tag_validate_file_contains(
	struct tag_validation *validation,
	long offset,
	long size)
{
	short range_index;

	(void)validation;
	if (offset < 0 || size < 0)
		return FALSE;
	for (range_index = 0; range_index < tag_validate_globals.file_range_count; range_index++)
	{
		struct tag_validate_file_range const *range = &tag_validate_globals.file_ranges[range_index];

		if ((unsigned long)offset >= range->offset && (unsigned long)offset - range->offset <= range->size &&
			(unsigned long)size <= range->size - ((unsigned long)offset - range->offset))
		{
			return TRUE;
		}
	}

	return FALSE;
}

void *tag_validate_root(
	struct tag_validation *validation)
{
	return validation->depth > 0 ? validation->frames[0].base : NULL;
}

boolean tag_validate_custom_edition(
	struct tag_validation *validation)
{
	(void)validation;

	return tag_validate_globals.custom_edition;
}

boolean tag_validate_contains(
	struct tag_validation *validation,
	void const *address,
	unsigned long size)
{
	return region_contains(validation, address, size);
}

void *tag_validate_tag_get(
	struct tag_validation *validation,
	long tag_index,
	unsigned long group_tag)
{
	struct tag_validate_instance *instance = instance_get(tag_index);
	unsigned long groups[2];

	(void)validation;
	groups[0] = group_tag;
	groups[1] = 0;
	if (!instance || !instance->base_address || !instance_in_groups(instance, groups))
		return NULL;

	return instance->base_address;
}

static void *buffer_data(
	byte *buffers,
	long count,
	void const *buffer)
{
	unsigned long offset = (unsigned long)buffer - (unsigned long)buffers;

	if ((unsigned long)buffer < (unsigned long)buffers || offset % BUFFER_SIZE ||
		offset / BUFFER_SIZE >= (unsigned long)count)
	{
		return NULL;
	}

	return *(void **)((byte const *)buffer + 4);
}

void *tag_validate_vertex_buffer_data(
	struct tag_validation *validation,
	void const *buffer)
{
	return buffer_data(validation->vertex_buffers, validation->vertex_buffer_count, buffer);
}

void *tag_validate_index_buffer_data(
	struct tag_validation *validation,
	void const *buffer)
{
	return buffer_data(validation->index_buffers, validation->index_buffer_count, buffer);
}
