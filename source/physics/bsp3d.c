/*
BSP3D.C

symbols in this file:
00136600 0080:
	_bsp3d_test_point (0000)
00136680 0280:
	_bsp3d_clip_line_to_leaves (0000)
00136900 02d0:
	_code_00136900 (0000)
00136BD0 0030:
	_bsp3d_clip_polygon_to_leaves (0000)
0028AF28 000f:
	??_C@_0P@HJMKGDLK@t?$DO0?4f?5?$CG?$CG?5t?$DM1?4f?$AA@ (0000)
0028AF38 001f:
	??_C@_0BP@OECNNHLF@c?3?2halo?2SOURCE?2physics?2bsp3d?4c?$AA@ (0000)
0028AF58 0004:
	__real@39800000 (0000)
0028AF5C 0004:
	__real@b9800000 (0000)
0028AF60 0033:
	??_C@_0DD@IKBLKABF@clipped_counts?$FL0?$FN?$CB?$DNNONE?5?$CG?$CG?5clipp@ (0000)
0028AF94 0032:
	??_C@_0DC@PNNMCKKO@vertex_count?$DMMAXIMUM_VERTICES_PE@ (0000)
0028AFC8 002e:
	??_C@_0CO@JLDBGGIN@vertex_count?$DO?$DNNUMBER_OF_VERTICES@ (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "bsp3d.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

long bsp3d_test_point(
	struct bsp3d const *bsp,
	long node_index,
	union real_point3d const *point)
{
	do
	{
		struct bsp3d_node const *node = TAG_BLOCK_GET_ELEMENT(
			&bsp->nodes,
			node_index,
			struct bsp3d_node);
		real_plane3d const *plane = TAG_BLOCK_GET_ELEMENT(
			&bsp->planes,
			node->plane_designator,
			real_plane3d);

		node_index = node->children[plane3d_distance_to_point(plane, point) >= 0.f];
	}
	while (!(node_index & LONG_MIN));

	if (node_index != NONE)
		return node_index & LONG_MAX;

	return NONE;
}

/* ---------- private code */
