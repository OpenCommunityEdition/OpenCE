#include "harness.h"

static struct { void *node_of_surface, *cost, *previous, *heap, *heap_position; long link_capacity; } bots_routes;
static struct { void *nodes, *link_first, *links; long node_count, link_count; } bots_graph;
static struct { void *grid_area, *grid_height_sum; } bots_vantage;
static int frees;
static void game_free(void *memory)
{
	CHECK(memory != NULL, "the engine allocator rejects free(NULL)");
	free(memory);
	frees++;
}
#define free game_free
#include "under_test.inc"

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	bots_routes_free();
	bots_vantage_free();
	CHECK(frees == 0, "initial disposal freed memory");
	/* A failed allocation can leave only part of each structure allocated. */
	bots_routes.cost = malloc(16);
	bots_graph.nodes = malloc(16);
	bots_graph.node_count = 4;
	bots_vantage.grid_area = malloc(16);
	bots_routes_free();
	bots_vantage_free();
	CHECK(frees == 3 && !bots_routes.cost && !bots_graph.nodes && !bots_vantage.grid_area, "partial allocations leaked");
	CHECK(!bots_graph.node_count && !bots_graph.link_count && !bots_routes.link_capacity, "stale graph bounds");
	bots_routes_free();
	bots_vantage_free();
	CHECK(frees == 3, "repeated disposal");
	return 0;
}
