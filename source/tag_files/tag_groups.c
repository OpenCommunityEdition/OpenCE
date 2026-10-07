/*
TAG_GROUPS.C
*/

/* ---------- headers */

#include "cseries.h"
#include "errors.h"
#include "tag_files.h"
#include "byte_swapping.h"
#include "tag_groups.h"
#include "custom_edition_cache.h" /* port: port/linux/game/custom_edition_cache.c */

/* ---------- constants */

enum
{
	/* port: the most bytes of the empty data (tag_empty_data): more than
	any tag's root or any block's element */
	TAG_EMPTY_DATA_SIZE = 0x10000,
};

/* ---------- globals */

/* port: (tag_empty_data) */
static unsigned long tag_empty_data_bytes[TAG_EMPTY_DATA_SIZE / sizeof(unsigned long)];

/* ---------- private code */

/* port: an index past what it indexes, logged once */
static void tag_index_error(
	char const *what,
	long index,
	long count)
{
	static boolean logged = FALSE;

	if (!logged)
	{
		logged = TRUE;
		error(_error_silent, "#%ld is not a %s index in [#0,#%ld): an empty one is used", index, what, count);
	}

	return;
}

/* ---------- public code */

/* port: what an index into a tag block, a tag's data or the tags that is
not one gives (tag_block_get_element_with_size, tag_data_get_pointer,
tag_get): TAG_EMPTY_DATA_SIZE bytes of zeros, zeroed again each time, in
place of whatever lies past the block, the data or the tags. Whatever
reads it reads an element or tag with nothing in it (no elements in its
blocks, no tags referenced, every index 0); whatever writes it writes
nowhere that matters */
void *tag_empty_data(
	void)
{
	csmemset(tag_empty_data_bytes, 0, sizeof(tag_empty_data_bytes));

	return tag_empty_data_bytes;
}

/* port: (cache_files.c) */
boolean tag_index_is_group(long tag_index, long group_tag);

long verify_tag_reference(
	const struct tag_reference *reference)
{
	long index;

	match_assert("c:\\halo\\SOURCE\\tag_files\\tag_groups.c", 3055, reference);
	/* port: a protected Custom Edition map has its tag names replaced and
	its references' names emptied, so a reference is taken by its index
	(port/linux/game/custom_edition_cache.c) */
	if (custom_edition_cache_tags_loaded())
		return tag_index_is_group(reference->index, reference->group_tag) ? reference->index : NONE;
	index = tag_loaded(reference->group_tag, reference->name);
	
	match_vassert(
		"c:\\halo\\SOURCE\\tag_files\\tag_groups.c", 3061, reference->index==index,
		csprintf(temporary,
			"tag reference \"%s\" and actual index do not match: is %08lX but should be %08lX",
			reference->name,
			reference->index,
			index));

	return index;
}

void* tag_data_get_pointer(
	const struct tag_data *data,
	long offset, 
	long size) 
{
	/* port: Halo PC reads a Custom Edition map's tags unchecked, and maps
	made for it can hold an offset past a tag data's end, which never
	stopped a game there: it gets the empty data below without an
	assertion. This build's maps keep theirs */
	if (!custom_edition_cache_tags_loaded())
	{
		match_assert("c:\\halo\\SOURCE\\tag_files\\tag_groups.c", 3073, size>=0);
		match_assert("c:\\halo\\SOURCE\\tag_files\\tag_groups.c", 3074, offset>=0 && offset+size<=data->size);
	}
	/* port: bytes past the data are the empty data's (tag_empty_data), as
	far as they go */
	if (size < 0 || offset < 0 || offset > data->size || size > data->size - offset || (size && !data->address))
	{
		tag_index_error("data", offset, data->size);
		return size <= TAG_EMPTY_DATA_SIZE ? tag_empty_data() : NULL;
	}

	return (void *)((byte *)data->address + offset);
}

void *tag_block_get_element_with_size(
	const struct tag_block *block,
	long index, 
	long element_size) 
{
	match_assert("c:\\halo\\SOURCE\\tag_files\\tag_groups.c", 3084, block);
	match_assert("c:\\halo\\SOURCE\\tag_files\\tag_groups.c", 3085, block->count>=0);
	match_assert("c:\\halo\\SOURCE\\tag_files\\tag_groups.c", 3086, !block->definition || block->definition->element_size==element_size);

	match_vassert("c:\\halo\\SOURCE\\tag_files\\tag_groups.c", 3089, index>=0 && index<block->count,
		csprintf(temporary,
			"#%d is not a valid %s index in [#0,#%d)",
			index,
			block->definition ? block->definition->name : "<unknown>", block->count));
	match_assert("c:\\halo\\SOURCE\\tag_files\\tag_groups.c", 3090, block->address);
	/* port: an element past the block (an index a map's data gave, which
	nothing checked) is the empty data (tag_empty_data), not whatever lies
	past the block */
	if (index < 0 || index >= block->count || !block->address)
	{
		tag_index_error("block element", index, block->count);
		return element_size <= TAG_EMPTY_DATA_SIZE ? tag_empty_data() : NULL;
	}

	return (void *)((byte *)block->address + (index * element_size));
}
