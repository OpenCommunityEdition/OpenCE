/*
TAG_GROUPS.C
*/

/* ---------- headers */

#include "cseries.h"
#include "tag_files.h"
#include "byte_swapping.h"
#include "tag_groups.h"
#ifdef HALO_LINUX
/* where the window is in this process (port/linux/include/halo_port_window.h) */
#include "halo_port_window.h"
#endif
#ifdef HALO_ANDROID
#include "platform.h"
#endif

/* ---------- public code */

long verify_tag_reference(
	const struct tag_reference *reference)
{
	long index;

	match_assert("c:\\halo\\SOURCE\\tag_files\\tag_groups.c", 3055, reference);
#ifdef HALO_ANDROID
	/* the map data names this string as the window was linked */
	index = tag_loaded(reference->group_tag, (char const *)PORT_WINDOW_REBASE(reference->name));
#else
	index = tag_loaded(reference->group_tag, reference->name);
#endif
	
	match_vassert(
		"c:\\halo\\SOURCE\\tag_files\\tag_groups.c", 3061, reference->index==index,
		csprintf(temporary,
			"tag reference \"%s\" and actual index do not match: is %08lX but should be %08lX",
			reference->name,
			reference->index,
			index));

	return index;
}

#ifdef HALO_ANDROID
void *tag_data_address(
	struct tag_data const *data)
{
	return PORT_WINDOW_REBASE(data->address);
}

void *tag_block_address(
	struct tag_block const *block)
{
	return PORT_WINDOW_REBASE(block->address);
}
#endif

void* tag_data_get_pointer(
	const struct tag_data *data,
	long offset, 
	long size) 
{
	match_assert("c:\\halo\\SOURCE\\tag_files\\tag_groups.c", 3073, size>=0);
	match_assert("c:\\halo\\SOURCE\\tag_files\\tag_groups.c", 3074, offset>=0 && offset+size<=data->size);

	return (void *)((byte *)TAG_DATA_ADDRESS(*data) + offset);
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

	return (void *)((byte *)TAG_BLOCK_ADDRESS(*block) + (index * element_size));
}
