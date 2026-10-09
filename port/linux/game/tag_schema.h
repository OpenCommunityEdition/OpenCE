/*
TAG_SCHEMA.H

What a map's tags are checked by before the game uses them (tag_validate.c):
a schema of each tag group, written with the game's own structures so that
every offset and size is the compiler's.

A definition is a structure (a tag's root, a tag block's element or a
structure inside one) and its fields: only the fields that hold a pointer, a
count, an index, an enum, a tag or a runtime value, which are what the game
trusts. Plain numbers and flags are not listed. A field names its place with
TAG_SCHEMA_OFFSET, which fails to compile if the field is not of the type the
schema says it is.

What happens to a field that is wrong:
- a block or data whose bytes leave the tags, or overlap other tags' (so that
  a runtime value written to one would land in another): the map is refused,
  as nothing after it can be trusted;
- a block with more elements than the game has room for: its count is cut to
  that maximum;
- a tag reference or tag index that is not a tag of the right group: NONE;
- a block index past its block: NONE where the game takes NONE, otherwise 0
  (NONE if the block is empty);
- an enum past its values: its first value (NONE where the game takes NONE);
- a string without its terminator: terminated;
- a runtime value (one the game sets as it runs): reset, always.
Each correction is logged. The retail maps need none (tools/map_validate.c).
*/

#ifndef __TAG_SCHEMA_H
#define __TAG_SCHEMA_H
#pragma once

/* ---------- headers */

#include "tag_files/tag_groups.h"

#include <stddef.h>

/* ---------- constants */

enum tag_schema_field_type
{
	/* a struct tag_block: its elements are definition's */
	_tag_schema_block,
	/* a struct tag_data in the tags: its bytes are at most maximum */
	_tag_schema_data,
	/* a struct tag_data whose bytes are in the map file (a bitmap's pixels, a
	sound's samples), at its file offset: they are at most maximum, in the
	file */
	_tag_schema_file_data,
	/* a struct tag_reference to a tag of one of groups */
	_tag_schema_reference,
	/* a long tag index alone, of one of groups */
	_tag_schema_tag_index,
	/* an index into a block (see target_level) */
	_tag_schema_block_index,
	/* an enum of maximum values */
	_tag_schema_enum,
	/* a string of size bytes, terminated */
	_tag_schema_string,
	/* a runtime value of size bytes, reset to value */
	_tag_schema_reset,
	/* a structure inline: definition's */
	_tag_schema_struct,
	/* a check the schema cannot say: check is called on the element once
	every tag has been through the schema, so it may look at other tags */
	_tag_schema_check,
	_tag_schema_terminator,
	NUMBER_OF_TAG_SCHEMA_FIELD_TYPES
};

enum tag_schema_field_flags
{
	/* an index, enum, tag or tag index that may be NONE */
	_tag_schema_none_bit = 0,
	/* (set by the macros) an index or enum of an unsigned type, whose NONE
	is all ones */
	_tag_schema_unsigned_bit,
	/* a block whose maximum is the Xbox editing kit's, not the length of an
	array of the game's: a Custom Edition map's tools went past it, and the
	game takes the block as long as it is */
	_tag_schema_tool_maximum_bit,
	NUMBER_OF_TAG_SCHEMA_FIELD_FLAGS
};

enum
{
	/* a block index's block is in the tag's root */
	TAG_SCHEMA_ROOT = -1,
	/* a block index's block is in the structure (_tag_schema_struct) the
	index is in */
	TAG_SCHEMA_STRUCTURE = -2,
	/* (0 is the block element the index is in, 1 the one its block is in,
	and so on) */

	MAXIMUM_TAG_SCHEMA_GROUPS_PER_FIELD = 15,

	/* the largest tag cache a map's tags are checked in: a Custom Edition
	map's (cache_file_formats.h, CUSTOM_EDITION_TAG_CACHE_BYTES) */
	TAG_VALIDATE_MAXIMUM_TAG_CACHE_SIZE = 0x01700000,
	/* the file ranges a Custom Edition map's bitmap pixels and sound samples
	may be in (tag_validate_custom_edition_tags) */
	MAXIMUM_TAG_VALIDATE_FILE_RANGES = 4,
};

/* ---------- macros */

/* where field is in type, if it is a field_type (it fails to compile
otherwise) */
#define TAG_SCHEMA_OFFSET(type, field, field_type) \
	(offsetof(type, field) + \
		0 * sizeof(char[__builtin_types_compatible_p(__typeof__(((type *)0)->field), field_type) ? 1 : -1]))

/* where field is in type, if it is an integer (of 1, 2 or 4 bytes) */
#define TAG_SCHEMA_INTEGER_OFFSET(type, field) \
	(offsetof(type, field) + \
		0 * sizeof(char[(sizeof(((type *)0)->field) == 1 || sizeof(((type *)0)->field) == 2 || \
			sizeof(((type *)0)->field) == 4) ? 1 : -1]))

#define TAG_SCHEMA_FIELD_SIZE(type, field) ((short)sizeof(((type *)0)->field))

/* _tag_schema_unsigned_bit if field is of an unsigned type */
#define TAG_SCHEMA_UNSIGNED(type, field) \
	((__typeof__(((type *)0)->field))-1 > 0 ? FLAG(_tag_schema_unsigned_bit) : 0)

/* a definition: its name, its structure and its fields, which end with
TAG_SCHEMA_END */
#define TAG_SCHEMA_DEFINITION(name, type, fields) \
	{ #name, sizeof(type), (fields) }

#define TAG_SCHEMA_BLOCK(type, field, definition, maximum) \
	{ _tag_schema_block, sizeof(struct tag_block), 1, 0, \
		TAG_SCHEMA_OFFSET(type, field, struct tag_block), (maximum), 0, 0, &(definition), NULL, #field }

/* ... whose maximum is the editing kit's (_tag_schema_tool_maximum_bit) */
#define TAG_SCHEMA_TOOL_BLOCK(type, field, definition, maximum) 	{ _tag_schema_block, sizeof(struct tag_block), 1, FLAG(_tag_schema_tool_maximum_bit), 		TAG_SCHEMA_OFFSET(type, field, struct tag_block), (maximum), 0, 0, &(definition), NULL, #field }

#define TAG_SCHEMA_DATA(type, field, maximum) \
	{ _tag_schema_data, sizeof(struct tag_data), 1, 0, \
		TAG_SCHEMA_OFFSET(type, field, struct tag_data), (maximum), 0, 0, NULL, NULL, #field }

#define TAG_SCHEMA_FILE_DATA(type, field, maximum) \
	{ _tag_schema_file_data, sizeof(struct tag_data), 1, 0, \
		TAG_SCHEMA_OFFSET(type, field, struct tag_data), (maximum), 0, 0, NULL, NULL, #field }

/* groups: a list of group tags ending with 0, e.g. TAG_SCHEMA_GROUPS('obje') */
#define TAG_SCHEMA_GROUPS(...) ((unsigned long const []){ __VA_ARGS__, 0 })

/* a reference may always be NONE */
#define TAG_SCHEMA_REFERENCE(type, field, groups) \
	{ _tag_schema_reference, sizeof(struct tag_reference), 1, FLAG(_tag_schema_none_bit), \
		TAG_SCHEMA_OFFSET(type, field, struct tag_reference), 0, 0, 0, (groups), NULL, #field }

#define TAG_SCHEMA_REFERENCE_ARRAY(type, field, groups) \
	{ _tag_schema_reference, sizeof(struct tag_reference), \
		(short)(sizeof(((type *)0)->field) / sizeof(struct tag_reference)), FLAG(_tag_schema_none_bit), \
		TAG_SCHEMA_OFFSET(type, field[0], struct tag_reference), 0, 0, 0, (groups), NULL, #field }

#define TAG_SCHEMA_TAG_INDEX(type, field, groups) \
	{ _tag_schema_tag_index, 4, 1, FLAG(_tag_schema_none_bit), \
		TAG_SCHEMA_OFFSET(type, field, long), 0, 0, 0, (groups), NULL, #field }

/* an index into the block at target_offset in the element or structure
target_level names; flags are _tag_schema_none_bit's or 0 */
#define TAG_SCHEMA_BLOCK_INDEX(type, field, target_level, target_offset, flags) \
	{ _tag_schema_block_index, TAG_SCHEMA_FIELD_SIZE(type, field), 1, (flags) | TAG_SCHEMA_UNSIGNED(type, field), \
		TAG_SCHEMA_INTEGER_OFFSET(type, field), 0, (target_level), (target_offset), NULL, NULL, #field }

/* an array of them */
#define TAG_SCHEMA_BLOCK_INDEX_ARRAY(type, field, target_level, target_offset, flags) \
	{ _tag_schema_block_index, TAG_SCHEMA_FIELD_SIZE(type, field[0]), \
		(short)(sizeof(((type *)0)->field) / sizeof(((type *)0)->field[0])), \
		(flags) | TAG_SCHEMA_UNSIGNED(type, field[0]), \
		TAG_SCHEMA_INTEGER_OFFSET(type, field[0]), 0, (target_level), (target_offset), NULL, NULL, #field }

#define TAG_SCHEMA_ENUM(type, field, count, flags) \
	{ _tag_schema_enum, TAG_SCHEMA_FIELD_SIZE(type, field), 1, (flags) | TAG_SCHEMA_UNSIGNED(type, field), \
		TAG_SCHEMA_INTEGER_OFFSET(type, field), (count), 0, 0, NULL, NULL, #field }

#define TAG_SCHEMA_ENUM_ARRAY(type, field, count, flags) \
	{ _tag_schema_enum, TAG_SCHEMA_FIELD_SIZE(type, field[0]), \
		(short)(sizeof(((type *)0)->field) / sizeof(((type *)0)->field[0])), \
		(flags) | TAG_SCHEMA_UNSIGNED(type, field[0]), \
		TAG_SCHEMA_INTEGER_OFFSET(type, field[0]), (count), 0, 0, NULL, NULL, #field }

#define TAG_SCHEMA_STRING(type, field) \
	{ _tag_schema_string, TAG_SCHEMA_FIELD_SIZE(type, field), 1, 0, \
		offsetof(type, field), 0, 0, 0, NULL, NULL, #field }

/* a runtime value of the field's size (up to 4 bytes) reset to value */
#define TAG_SCHEMA_RESET(type, field, value) \
	{ _tag_schema_reset, TAG_SCHEMA_FIELD_SIZE(type, field), 1, 0, \
		TAG_SCHEMA_INTEGER_OFFSET(type, field), 0, 0, (value), NULL, NULL, #field }

/* a runtime structure (any size) zeroed */
#define TAG_SCHEMA_ZERO(type, field) \
	{ _tag_schema_reset, TAG_SCHEMA_FIELD_SIZE(type, field), 1, 0, \
		offsetof(type, field), 0, 0, 0, NULL, NULL, #field }

#define TAG_SCHEMA_STRUCT(type, field, definition) \
	{ _tag_schema_struct, TAG_SCHEMA_FIELD_SIZE(type, field), 1, 0, \
		offsetof(type, field), 0, 0, 0, &(definition), NULL, #field }

#define TAG_SCHEMA_STRUCT_ARRAY(type, field, definition) \
	{ _tag_schema_struct, TAG_SCHEMA_FIELD_SIZE(type, field[0]), \
		(short)(sizeof(((type *)0)->field) / sizeof(((type *)0)->field[0])), 0, \
		offsetof(type, field[0]), 0, 0, 0, &(definition), NULL, #field }

#define TAG_SCHEMA_CHECK(check) \
	{ _tag_schema_check, 0, 1, 0, 0, 0, 0, 0, NULL, (check), #check }

#define TAG_SCHEMA_END \
	{ _tag_schema_terminator, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }

/* ---------- structures */

struct tag_validation;
struct tag_schema_definition;

/* a check on one element (base) of the definition it is in: FALSE refuses
the map (tag_validate_refuse says why) */
typedef boolean (*tag_schema_check_proc)(
	struct tag_validation *validation,
	void *base);

struct tag_schema_field
{
	short type;
	/* the integer's bytes; a block's, reference's or structure's stride */
	short size;
	/* how many there are, one after another (an array) */
	short count;
	short flags;
	long offset;
	/* a block's most elements, data's most bytes, an enum's values */
	long maximum;
	/* a block index's block: where it is (TAG_SCHEMA_ROOT,
	TAG_SCHEMA_STRUCTURE, or how many elements up) and its offset there */
	long target_level;
	/* a block index's block's offset; a reset's value */
	long target_offset;
	/* a block's or structure's definition; a reference's or tag index's
	groups */
	void const *definition;
	tag_schema_check_proc check;
	char const *name;
};

struct tag_schema_definition
{
	char const *name;
	long size;
	struct tag_schema_field const *fields;
};

struct tag_schema_group
{
	unsigned long group_tag;
	unsigned long parent_group_tags[2];
	struct tag_schema_definition const *definition;
};

/* bytes a tag's data in a file may be at: offset to offset + size (a
Custom Edition map's offsets count in one space of several files:
custom_edition_cache.c) */
struct tag_validate_file_range
{
	unsigned long offset;
	unsigned long size;
};

/* ---------- prototypes/TAG_SCHEMA_*.C */

/* the groups the validator knows: lists of them (each ending with a 0 group
tag), one for each tag_schema_*.c, ending with NULL */
extern struct tag_schema_group const *const tag_schema_group_lists[];

extern struct tag_schema_group const tag_schema_object_groups[];
extern struct tag_schema_group const tag_schema_model_groups[];
extern struct tag_schema_group const tag_schema_collision_groups[];
extern struct tag_schema_group const tag_schema_render_groups[];
extern struct tag_schema_group const tag_schema_effect_groups[];
extern struct tag_schema_group const tag_schema_scenario_groups[];
/* the groups a Custom Edition map's tags are of where they are laid out
otherwise than this build's, as they are checked (tag_schema_models.c:
gbxmodels, 'mod2', which the game takes as models, 'mode') */
extern struct tag_schema_group const tag_schema_custom_edition_groups[];

/* ---------- prototypes/TAG_VALIDATE.C */

/* the tags at tag_header (tag_data_size bytes read from a map file of
file_length bytes): FALSE if they cannot be used. Before anything else
reads them; corrections are made in place */
boolean tag_validate_tags(
	void *tag_header,
	long tag_data_size,
	long file_length,
	char const *map_name);

/* the tags of a Custom Edition map, as custom_edition_cache_load loaded and
custom_edition_cache_convert converted them (loaded_size bytes at
tag_header, the start of a tag cache of tag_cache_size bytes), whose bitmap
pixels and sound samples are in file_ranges: FALSE if they cannot be used.
They are checked as tag_validate_tags checks a map's, but for what Custom
Edition lays out otherwise (tag_schema_custom_edition_groups; a structure
bsp's material vertices, uncompressed, tag_validate_custom_edition) */
boolean tag_validate_custom_edition_tags(
	void *tag_header,
	long loaded_size,
	unsigned long tag_cache_size,
	struct tag_validate_file_range const *file_ranges,
	short file_range_count,
	char const *map_name);

/* a structure bsp just read (size bytes at base, its header there) as tag
tag_index: FALSE if it cannot be used */
boolean tag_validate_structure_bsp(
	long tag_index,
	void *base,
	long size);

/* how many corrections the last validation made (none for a retail map) */
long tag_validate_corrections(
	void);
/* whether the last validation found address to be in a tag's root, block or
data (tools/map_validate.c) */
boolean tag_validate_claimed(
	void const *address);
/* whether any of the size bytes at address are in a tag's root, block or
data (TRUE too when they are not all in the tag cache): what the game draws
from must not be bytes it writes to as it runs */
boolean tag_validate_any_claimed(
	void const *address,
	unsigned long size);

/* for checks: */

/* the map is refused: why, printf style */
void tag_validate_refuse(
	struct tag_validation *validation,
	char const *format,
	...);
/* a correction was made: what, printf style */
void tag_validate_correct(
	struct tag_validation *validation,
	char const *format,
	...);
/* A length the game takes as one (a radius, a width: point_physics_update's
radius): none or more, a value below zero or not a number made 0, as the
correction `name` says. */
void tag_validate_non_negative(
	struct tag_validation *validation,
	char const *name,
	real *value);
/* whether size bytes at offset in the map's file are in it (or, for a
Custom Edition map, in one of the files its offsets count in), as data in a
file (_tag_schema_file_data) must be */
boolean tag_validate_file_contains(
	struct tag_validation *validation,
	long offset,
	long size);
/* the root of the tag being checked (the element a check is on may be one
of its blocks') */
void *tag_validate_root(
	struct tag_validation *validation);
/* whether the tags being checked are a Custom Edition map's */
boolean tag_validate_custom_edition(
	struct tag_validation *validation);
/* whether size bytes at address are in the tags (or the bsp) being checked */
boolean tag_validate_contains(
	struct tag_validation *validation,
	void const *address,
	unsigned long size);
/* the root of tag tag_index if it is a tag of group (or inherits from it),
otherwise NULL */
void *tag_validate_tag_get(
	struct tag_validation *validation,
	long tag_index,
	unsigned long group_tag);
/* the vertex or index buffer (D3DVertexBuffer, D3DIndexBuffer) a model's or
bsp's buffer points at, if it is one of its header's: its data's address,
otherwise NULL. A bsp's "index buffers" are its lightmaps' vertex buffers
(the second array of its header). The data is only read: it is not a block
of the tags, and may lie where another tag is */
void *tag_validate_vertex_buffer_data(
	struct tag_validation *validation,
	void const *buffer);
void *tag_validate_index_buffer_data(
	struct tag_validation *validation,
	void const *buffer);

/* ---------- prototypes/where the validator's messages go (tag_validate_report.c) */

void tag_validate_report(
	char const *message);

#endif // __TAG_SCHEMA_H
