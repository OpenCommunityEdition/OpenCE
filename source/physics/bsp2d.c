/*
BSP2D.C

symbols in this file:
00136590 0070:
	_bsp2d_test_point (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "cseries/errors.h" /* port: error */
#include "bsp2d.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* port: whether a map's malformed bsp2d was reported (once) */
static boolean warned_about_bsp2d_nodes;

/* ---------- public code */

long bsp2d_test_point(
	struct tag_block const *nodes,
	real_point2d const *point,
	long node_index)
{
	short depth = 0;

	while (!(node_index & LONG_MIN))
	{
		struct bsp2d_node const *node;
		real distance;

		/* port: a node (from the map) that is no node, or deeper than
		MAXIMUM_BSP2D_TRAVERSAL_DEPTH, is in no surface */
		if (node_index >= nodes->count ||
			depth++ >= MAXIMUM_BSP2D_TRAVERSAL_DEPTH)
		{
			if (!warned_about_bsp2d_nodes)
			{
				error(_error_silent, "a bsp2d node is not one of the bsp's, or is more than %d deep",
					MAXIMUM_BSP2D_TRAVERSAL_DEPTH);
				warned_about_bsp2d_nodes = TRUE;
			}
			return NONE;
		}
		node = TAG_BLOCK_GET_ELEMENT(nodes, node_index, struct bsp2d_node);
		distance = (node->plane.n.i * point->x + node->plane.n.j * point->y) - node->plane.d;
		node_index = node->child_indices[distance >= 0.0f];
	}

	return node_index != NONE ? node_index & LONG_MAX : NONE;
}

/* ---------- private code */
