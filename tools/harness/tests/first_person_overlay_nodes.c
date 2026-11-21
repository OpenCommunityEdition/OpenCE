/*
FIRST_PERSON_OVERLAY_NODES.C (test)

The real first_person_weapon_overlay_nodes (first_person_weapons.c): the nodes
an overlay animation moves (it has their rotation, translation or scale), with
their children wherever they come in the graph, as a bit vector, which the
first-person interpolation draws as the tick has them when the count the
overlay shows changes (render_interpolation.c). test_first_person_overlay_nodes.py
takes the constants, the node structure and the code from the sources.
*/

#include <stdarg.h>
#include "harness.h"

#include "config.inc"

#define csmemset memset
#define VALID_INDEX(i, n) ((i) >= 0 && (i) < (n))
#define TAG_BLOCK_GET_ELEMENT(block, index, type) (&((type *)(block)->address)[index])

/* (the fields of model_animation_definitions.h's that the code reads) */
struct animation
{
	short node_count;
	unsigned long nodes_with_translation_flags[2];
	unsigned long nodes_with_rotation_flags[2];
	unsigned int nodes_with_scale_flags[2];
};
struct tag_block { long count; void *address; };
struct animation_graph { struct tag_block nodes; };

#include "under_test.inc"

/* a graph of 40 nodes: the gun (0), its magazine (1) with two rounds in it (2,
3), the hand (4), and the rest the gun's; node 5 is the child of node 36,
which is the magazine's, and comes before it (a graph written out of order) */
#define NODE_COUNT 40
static struct animation_graph_node graph_nodes[NODE_COUNT];
static struct animation_graph graph = { { NODE_COUNT, graph_nodes } };
static struct animation overlay;
static unsigned long nodes[BIT_VECTOR_SIZE_IN_LONGS(MAXIMUM_NODES_PER_ANIMATION)];
static short parents[MAXIMUM_NODES_PER_ANIMATION];

static void reset(void)
{
	short node;

	memset(graph_nodes, 0, sizeof(graph_nodes));
	memset(&overlay, 0, sizeof(overlay));
	for (node = 0; node < NODE_COUNT; node++)
		graph_nodes[node].parent_node_index = node ? 0 : NONE;
	graph_nodes[2].parent_node_index = 1;
	graph_nodes[3].parent_node_index = 1;
	graph_nodes[36].parent_node_index = 1;
	graph_nodes[5].parent_node_index = 36;
	overlay.node_count = NODE_COUNT;
	memset(nodes, 0xFF, sizeof(nodes));
}

static void flag(unsigned long *flags, short node)
{
	flags[node >> LONG_BITS_BITS] |= 1UL << (node & (LONG_BITS - 1));
}

/* exactly the nodes listed (ending with NONE) are in the vector */
static void expect_only(char const *what, ...)
{
	boolean listed[NODE_COUNT + 24] = { 0 };
	short node;
	va_list nodes_listed;

	va_start(nodes_listed, what);
	for (node = (short)va_arg(nodes_listed, int); node != NONE; node = (short)va_arg(nodes_listed, int))
		listed[node] = TRUE;
	va_end(nodes_listed);
	for (node = 0; node < MAXIMUM_NODES_PER_ANIMATION; node++)
	{
		CHECK(!BIT_VECTOR_TEST_FLAG(nodes, node) == !listed[node], "%s: node %d %s", what, node,
			listed[node] ? "left out" : "taken in");
	}
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* each of the three kinds of data an overlay can have for a node, in
	both of the flags' words */
	CASE("moved")
	{
		reset();
		flag(overlay.nodes_with_rotation_flags, 2);
		flag(overlay.nodes_with_translation_flags, 3);
		flag((unsigned long *)overlay.nodes_with_scale_flags, 37);
		first_person_weapon_overlay_nodes(&graph, &overlay, NODE_COUNT, nodes, parents);
		expect_only("a round turned, a round moved, a node scaled", 2, 3, 37, NONE);
	}
	/* the magazine moved: the rounds in it, and node 36's child before it, go with it */
	CASE("children")
	{
		reset();
		flag(overlay.nodes_with_translation_flags, 1);
		first_person_weapon_overlay_nodes(&graph, &overlay, NODE_COUNT, nodes, parents);
		expect_only("the magazine and what it holds", 1, 2, 3, 36, 5, NONE);
		CHECK(parents[0] == NONE && parents[2] == 1 && parents[3] == 1 && parents[5] == 36 && parents[4] == 0,
			"the parents given: %d %d %d %d %d", parents[0], parents[2], parents[3], parents[5], parents[4]);
	}
	/* nodes past the graph's count or the animation's, and a parent out of the graph */
	CASE("out of range")
	{
		reset();
		flag(overlay.nodes_with_rotation_flags, 38);
		flag(overlay.nodes_with_rotation_flags, 33);
		flag(overlay.nodes_with_rotation_flags, 30);
		overlay.node_count = 31;
		graph_nodes[6].parent_node_index = 50;
		first_person_weapon_overlay_nodes(&graph, &overlay, 36, nodes, parents);
		expect_only("within the animation and the graph", 30, NONE);
		CHECK(parents[6] == NONE && parents[5] == NONE, "a parent out of the graph given: %d %d", parents[6], parents[5]);
	}
	return 0;
}
