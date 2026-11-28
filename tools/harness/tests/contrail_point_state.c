#include "harness.h"

/* a contrail definition's states block, and a point's state index, as the
function reads them */
struct tag_block { long count; void *address; void *definition; };
struct contrail_point_state { real width; };
struct contrail_definition { struct tag_block states; };
struct contrail_point_datum { short state_index; };

/* (the game's element accessor asserts an index in the block: here a
check, as a debug build halts) */
static void *element(struct tag_block const *block, long index, long size)
{
	CHECK(index >= 0 && index < block->count, "#%ld is not a valid index in [#0,#%ld)", index, block->count);
	return (char *)block->address + index * size;
}
#define TAG_BLOCK_GET_ELEMENT(block, index, type) ((type *)element((block), (index), sizeof(type)))

#include "under_test.inc"

int main(int argc, char **argv)
{
	char const *mode = argv[1];
	struct contrail_point_state states[2] = { { 1.0f }, { 2.0f } };
	struct contrail_definition definition = { { 2, states, NULL } };
	struct contrail_point_datum point;

	if (!strcmp(mode, "new"))
	{
		/* (added between ticks: no state yet) */
		point.state_index = NONE;
		CHECK(contrail_point_state_get(&definition, &point) == &states[0], "drawn in its first state");
		return 0;
	}
	if (!strcmp(mode, "aged"))
	{
		point.state_index = 1;
		CHECK(contrail_point_state_get(&definition, &point) == &states[1], "drawn in its own state");
		return 0;
	}
	fprintf(stderr, "no case %s\n", mode);
	return 2;
}
