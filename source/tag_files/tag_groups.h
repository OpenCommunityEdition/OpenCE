/*
TAG_GROUPS.H

header included in hcex build.
*/

#ifndef __TAG_GROUPS_H
#define __TAG_GROUPS_H
#pragma once

/* ---------- headers */

#include "memory/byte_swapping.h"

/* ---------- constants */

enum tag_field_type
{
	_tag_field_string = 0,
	_tag_field_char_integer = 1,
	_tag_field_short_integer = 2,
	_tag_field_long_integer = 3,
	_tag_field_enum = 6,
	_tag_field_word_flags = 8,
	_tag_field_byte_flags = 9,
	_tag_field_real_point3d = 17,
	_tag_field_real_plane2d = 23,
	_tag_field_real_plane3d = 24,
	_tag_field_tag_reference = 33,
	_tag_field_block = 34,
	_tag_field_data = 37,
	_tag_field_pad = 40,
	_tag_field_terminator = 44,
};

/* ---------- macros */

#define TAG_BLOCK_GET_ELEMENT(block_address, index, type) ((type *)tag_block_get_element_with_size((block_address), (index), sizeof(type)))

/* ---------- structures */

typedef void (*byte_swap_block_proc)(void *);
typedef boolean (*postprocess_block_proc)(void *, boolean);
typedef byte *(*format_block_proc)(long, struct tag_block *, long, byte *);
typedef void (*delete_block_proc)(struct tag_block *, long);
typedef void (*byte_swap_data_proc)(void *, void *, long);

struct tag_enum_definition
{
	long count;
	char **names;
	void *unused;
};

struct tag_field
{
	short type;
	word pad;
	char *name;
	void *definition;
};

struct tag_data_definition
{
	char *name;
	unsigned long flags;
	long maximum_size;
	byte_swap_data_proc byte_swap_data;
};

struct tag_block_definition
{
	char *name;
	unsigned long flags;
	long maximum_element_count;
	long element_size;
	void *default_element;
	struct tag_field *fields;
	byte_swap_block_proc byte_swap_block;
	postprocess_block_proc postprocess_block;
	format_block_proc format_block;
	delete_block_proc delete_block;
	byte_swap_code *byte_swap_codes;
};

struct tag_block
{
	long count;
	void *address;
	struct tag_block_definition *definition;
};

struct tag_reference
{
	unsigned long group_tag;
	char *name;
	long name_length;
	long index;
};

struct tag_reference_definition
{
	unsigned long flags;
	unsigned long group_tag;
	unsigned long *group_tags;
};

typedef char tag_reference_definition_size_assert[
	sizeof(struct tag_reference_definition) == 0xC ? 1 : -1];

struct tag_data
{
	long size;
	unsigned long pad;
	long file_offset;
	void *address;
	struct tag_data_definition *definition;
};

/* ---------- prototypes/TAG_GROUPS.C */

long verify_tag_reference(struct tag_reference const *reference);
void *tag_data_get_pointer(struct tag_data const *data, long offset, long size);
void *tag_block_get_element_with_size(struct tag_block const *block, long index, long element_size);

/* Where a tag_data's or tag_block's data is in this process. A map file names
these as the window the game was linked for, so a port that puts the window
elsewhere has to move them (port/linux/include/halo_port_window.h); a read of
->address goes through here, or the game follows a link-time address. Each
macro takes the tag_data or tag_block structure itself, so a caller writes
TAG_DATA_ADDRESS(foo->bar).

Where the window is where the game expects it these are the field itself, so
the original code and the byte-matching build are unaffected. */
#ifdef HALO_ANDROID
void *tag_data_address(struct tag_data const *data);
void *tag_block_address(struct tag_block const *block);
#define TAG_DATA_ADDRESS(data) tag_data_address(&(data))
#define TAG_BLOCK_ADDRESS(block) tag_block_address(&(block))
#else
#define TAG_DATA_ADDRESS(data) ((data).address)
#define TAG_BLOCK_ADDRESS(block) ((block).address)
#endif

/* the same, where the block is already a pointer: a macro that took the
structure by value cannot be used on one */
#ifdef HALO_ANDROID
#define TAG_BLOCK_ADDRESS_AT(block) tag_block_address(block)
#else
#define TAG_BLOCK_ADDRESS_AT(block) ((block)->address)
#endif

/* A tag reference names the tag it points at, with a pointer to a string
held in the tag data, which the map wrote as an address in the window the
game was linked for. A read of that name has to go through here for the
same reason ->address does. */
#ifdef HALO_ANDROID
char const *tag_reference_name(struct tag_reference const *reference);
#define TAG_REFERENCE_NAME(reference) tag_reference_name(&(reference))
#else
#define TAG_REFERENCE_NAME(reference) ((reference).name)
#endif

/* ---------- prototypes/CACHE_FILES.C */

long tag_loaded(long group_tag, const char *name);

void *tag_get(long group_tag, long tag_index);

/* ---------- globals */

/* ---------- public code */

#endif // __TAG_GROUPS_H
