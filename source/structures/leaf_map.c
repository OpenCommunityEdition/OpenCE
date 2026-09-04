/*
LEAF_MAP.C

symbols in this file:
00181320 0050:
	_code_00181320 (0000)
00181370 0040:
	_code_00181370 (0000)
001813B0 0040:
	_code_001813b0 (0000)
001813F0 0030:
	_leaf_map_delete (0000)
00181420 00a0:
	_code_00181420 (0000)
001814C0 0040:
	_code_001814c0 (0000)
00181500 00d0:
	_leaf_map_close_portal (0000)
001815D0 0060:
	_leaf_map_leaf_is_closed (0000)
00181630 00b0:
	_code_00181630 (0000)
001816E0 0160:
	_render_debug_leaf_portal (0000)
00181840 0060:
	_render_debug_leaf_portals (0000)
001818A0 0340:
	_code_001818a0 (0000)
00181BE0 0310:
	_leaf_map_get_leaf_bounds (0000)
00181EF0 0070:
	_leaf_map_closure (0000)
00181F60 0290:
	_leaf_map_leaf_spans_polygon (0000)
001821F0 00b0:
	_code_001821f0 (0000)
001822A0 0160:
	_render_debug_leaf_faces (0000)
00182400 0080:
	_code_00182400 (0000)
00182480 0170:
	_code_00182480 (0000)
001825F0 01e0:
	_code_001825f0 (0000)
001827D0 0260:
	_code_001827d0 (0000)
00182A30 00f0:
	_code_00182a30 (0000)
00182B20 0070:
	_code_00182b20 (0000)
00182B90 00e0:
	_code_00182b90 (0000)
00182C70 0130:
	_leaf_map_initialize_from_bsp (0000)
002A0C84 0012:
	??_C@_0BC@EGCHAPG@front?5leaf?5index?$CK?$AA@ (0000)
002A0C98 0011:
	??_C@_0BB@LINKKPJD@back?5leaf?5index?$CK?$AA@ (0000)
002A0CAC 000d:
	??_C@_0N@KPMCHLJG@plane?5index?$CK?$AA@ (0000)
002A0CBC 0019:
	??_C@_0BJ@FMOPLLFD@global_leaf_portal_block?$AA@ (0000)
002A0CD8 0019:
	??_C@_0BJ@IBJPLNPO@leaf_portal_vertex_block?$AA@ (0000)
002A0CF4 0010:
	??_C@_0BA@IDKJGEOA@portal?5indices?$CK?$AA@ (0000)
002A0D04 0007:
	??_C@_06KCOLJFCM@faces?$CK?$AA@ (0000)
002A0D0C 0016:
	??_C@_0BG@MIFDMMKE@global_map_leaf_block?$AA@ (0000)
002A0D24 000c:
	??_C@_0M@MIPIKCKP@node?5index?$CK?$AA@ (0000)
002A0D30 0014:
	??_C@_0BE@JGACHFLM@map_leaf_face_block?$AA@ (0000)
002A0D44 000e:
	??_C@_0O@GPDFDPME@portal?5index?$CK?$AA@ (0000)
002A0D54 001c:
	??_C@_0BM@OMCNINBM@map_leaf_portal_index_block?$AA@ (0000)
002A0D70 0008:
	??_C@_07FKKFMDPH@vertex?$CK?$AA@ (0000)
002A0D78 001b:
	??_C@_0BL@MDBPGNIK@map_leaf_face_vertex_block?$AA@ (0000)
002A0D94 0014:
	??_C@_0BE@CCPHNOCB@leaf_map_initialize?$AA@ (0000)
002A0DA8 003b:
	??_C@_0DL@KLHAKAHD@leaf_map_globals?4node_stack_coun@ (0000)
002A0DE4 0025:
	??_C@_0CF@JDBJPHEO@c?3?2halo?2SOURCE?2structures?2leaf_m@ (0000)
002A0E0C 0024:
	??_C@_0CE@INFHLEED@leaf_map_globals?4node_stack_coun@ (0000)
002A0E30 003c:
	??_C@_0DM@JGHBMKCF@levels_up?$DO?$DN0?5?$CG?$CG?5levels_up?$DMleaf_m@ (0000)
002A0E6C 0038:
	??_C@_0DI@KHGJOGO@portal_designator_index?$CB?$DNleaf?9?$DOp@ (0000)
002A0EA4 002e:
	??_C@_0CO@EGKEMBBF@couldn?8t?5allocate?5leaf?5map?5porta@ (0000)
002A0ED4 0022:
	??_C@_0CC@GKACLMKE@couldn?8t?5allocate?5leaf?5map?5porta@ (0000)
002A0EF8 0004:
	__real@3b23d70a (0000)
002A0EFC 001c:
	??_C@_0BM@NDHGCOBM@too?5many?5vertices?5in?5portal?$AA@ (0000)
002A0F18 0019:
	??_C@_0BJ@NHOJIIIJ@leaf_index0?$CB?$DNleaf_index1?$AA@ (0000)
002A0F34 0009:
	??_C@_08JHHHNNE@map?9?$DObsp?$AA@ (0000)
002A0F40 0026:
	??_C@_0CG@HMEMFMPL@reference_plane?9?$DOn?4n?$FLprojection?$FN@ (0000)
002A0F68 004f:
	??_C@_0EP@LAJCFBMA@ancestor_node_index?$CB?$DNNONE?5?$HM?$HM?5ind@ (0000)
002A0FB8 001d:
	??_C@_0BN@LDAFHDBA@couldn?8t?5allocate?5leaf?5face?4?$AA@ (0000)
002A0FD8 0021:
	??_C@_0CB@LPMMAOJD@couldn?8t?5allocate?5leaf?5vertices?4@ (0000)
002A0FFC 001a:
	??_C@_0BK@OEBNDAAE@result?4vertex_count?$CB?$DNNONE?$AA@ (0000)
002A1018 0023:
	??_C@_0CD@KPKMOOLF@couldn?8t?5allocate?5leaf_map?5leave@ (0000)
002A103C 0025:
	??_C@_0CF@CKHDENNA@leaf_map_globals?4node_stack_coun@ (0000)
002A1064 0004:
	??_C@_03DGBMLAJE@bsp?$AA@ (0000)
002A1068 0009:
	??_C@_08NNODDDAN@leaf_map?$AA@ (0000)
0030E970 09d0:
	_data_0030e970 (0000)
	_global_map_leaf_block (08f8)
	_global_leaf_portal_block (09a4)
004C08B0 040a:
	_bss_004c08b0 (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "structures/leaf_map.h"
#include "cache/cache_files.h"
#include "physics/bsp3d.h"
#include "math/geometry.h"
#include "render/render_debug_geometry.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct map_leaf_face
{
	long node_index;
	struct tag_block vertices;			// real_point2d
};

struct map_leaf
{
	struct tag_block faces;				// map_leaf_face
	struct tag_block portal_designators;		// long
};

typedef char map_leaf_face_size_assert[
	sizeof(struct map_leaf_face) == 0x10 ? 1 : -1];
typedef char map_leaf_size_assert[
	sizeof(struct map_leaf) == 0x18 ? 1 : -1];

/* ---------- prototypes */

static void leaf_map_family_mark(
	const struct leaf_map *leaf_map,
	long *marked_leaves,
	long leaf_index);

static void leaf_face_get_vertex3d(
	const struct leaf_map *map,
	struct map_leaf_face *face,
	short vertex_index,
	real_point3d *result);

/* ---------- globals */

/* ---------- public code */

void leaf_map_delete(
	struct leaf_map *leaf_map)
{
	tag_block_resize(&leaf_map->leaves, 0);
	tag_block_resize(&leaf_map->portals, 0);
	return;
}

void leaf_map_close_portal(
	struct leaf_map *leaf_map,
	long portal_index)
{
	struct leaf_portal *portal = TAG_BLOCK_GET_ELEMENT(&leaf_map->portals, portal_index, struct leaf_portal);
	long side;

	for (side = 0; side < 2; side++)
	{
		struct map_leaf *leaf = TAG_BLOCK_GET_ELEMENT(&leaf_map->leaves, portal->leaf_indices[side] & LONG_MAX, struct map_leaf);
		short portal_designator_index;

		for (portal_designator_index = 0;
			portal_designator_index < leaf->portal_designators.count;
			portal_designator_index++)
		{
			long *portal_designator = TAG_BLOCK_GET_ELEMENT(&leaf->portal_designators, portal_designator_index, long);

			if ((*portal_designator & LONG_MAX) == portal_index)
			{
				*portal_designator |= LONG_MIN;
				break;
			}
		}

		match_assert(
			"c:\\halo\\SOURCE\\structures\\leaf_map.c",
			673,
			portal_designator_index!=leaf->portal_designators.count);
	}

	return;
}

boolean leaf_map_leaf_is_closed(
	const struct leaf_map *leaf_map,
	long leaf_index)
{
	struct map_leaf *leaf = TAG_BLOCK_GET_ELEMENT(&leaf_map->leaves, leaf_index & LONG_MAX, struct map_leaf);
	short portal_designator_index;

	for (portal_designator_index = 0;
		portal_designator_index < leaf->portal_designators.count;
		portal_designator_index++)
	{
		long portal_designator = *TAG_BLOCK_GET_ELEMENT(&leaf->portal_designators, portal_designator_index, long);

		if (!(portal_designator & LONG_MIN))
		{
			return FALSE;
		}
	}

	return (boolean)leaf->portal_designators.count;
}

void render_debug_leaf_portal(
	const struct leaf_map *leaf_map,
	long portal_designator)
{
	struct leaf_portal *portal = TAG_BLOCK_GET_ELEMENT(&leaf_map->portals, portal_designator & LONG_MAX, struct leaf_portal);
	boolean closed = (portal_designator & LONG_MIN) != 0;
	real_argb_color colors[2] = {{{0.1f, 0.0f, 1.0f, 0.0f}}, {{0.1f, 1.0f, 0.0f, 0.0f}}};
	short vertex_index;

	for (vertex_index = 2; vertex_index < portal->vertices.count; vertex_index++)
	{
		render_debug_triangle(
			TRUE,
			TAG_BLOCK_GET_ELEMENT(&portal->vertices, 0, real_point3d),
			TAG_BLOCK_GET_ELEMENT(&portal->vertices, vertex_index - 1, real_point3d),
			TAG_BLOCK_GET_ELEMENT(&portal->vertices, vertex_index, real_point3d),
			&colors[closed]);
		render_debug_line(
			TRUE,
			TAG_BLOCK_GET_ELEMENT(&portal->vertices, vertex_index - 1, real_point3d),
			TAG_BLOCK_GET_ELEMENT(&portal->vertices, vertex_index, real_point3d),
			global_real_argb_red);
	}

	render_debug_line(
		TRUE,
		TAG_BLOCK_GET_ELEMENT(&portal->vertices, 0, real_point3d),
		TAG_BLOCK_GET_ELEMENT(&portal->vertices, 1, real_point3d),
		global_real_argb_red);
	render_debug_line(
		TRUE,
		TAG_BLOCK_GET_ELEMENT(&portal->vertices, 0, real_point3d),
		TAG_BLOCK_GET_ELEMENT(&portal->vertices, portal->vertices.count - 1, real_point3d),
		global_real_argb_red);

	return;
}

boolean leaf_map_leaf_spans_polygon(
	const struct leaf_map *leaf_map,
	long leaf_index,
	const real_plane3d *plane,
	short projection,
	boolean sign,
	long vertex_count,
	const real_point2d *vertices)
{
	struct map_leaf *leaf = TAG_BLOCK_GET_ELEMENT(&leaf_map->leaves, leaf_index & LONG_MAX, struct map_leaf);
	short face_index;

	for (face_index = 0; face_index < leaf->faces.count; face_index++)
	{
		struct map_leaf_face *face = TAG_BLOCK_GET_ELEMENT(&leaf->faces, face_index, struct map_leaf_face);
		struct bsp3d_node *node = TAG_BLOCK_GET_ELEMENT(&leaf_map->bsp->nodes, face->node_index, struct bsp3d_node);
		real_plane3d *face_plane = TAG_BLOCK_GET_ELEMENT(&leaf_map->bsp->planes, node->plane_designator, real_plane3d);
		short face_projection = projection_from_vector3d(&face_plane->n);
		boolean face_sign = projection_sign_from_vector3d(&face_plane->n, face_projection);
		short vertex_index;

		for (vertex_index = 0; vertex_index < face->vertices.count; vertex_index++)
		{
			real_point2d point;
			real_point3d point0;
			real_point3d point1;
			real_point3d intersection;
			real_vector3d delta;
			long next_vertex_index;
			real distance0, distance1;

			project_point2d(
				TAG_BLOCK_GET_ELEMENT(&face->vertices, vertex_index, real_point2d),
				face_plane,
				face_projection,
				face_sign,
				&point0);

			next_vertex_index = (vertex_index + 1 == face->vertices.count) ? 0 : vertex_index + 1;

			project_point2d(
				TAG_BLOCK_GET_ELEMENT(&face->vertices, next_vertex_index, real_point2d),
				face_plane,
				face_projection,
				face_sign,
				&point1);

			distance0 = plane3d_distance_to_point(plane, &point0);
			distance1 = plane3d_distance_to_point(plane, &point1);

			if ((distance0 < -0.03f && distance1 > 0.03f) ||
				(distance0 > 0.03f && distance1 < -0.03f))
			{
				real intersection_t;

				vector_from_points3d(&point0, &point1, &delta);
				intersection_t= vector_intersect_plane3d(&point0, &delta, plane);
				/* Preserve January's scalar expansion without emitting a
				 * point_from_line3d COMDAT. */
				intersection.x= delta.i*intersection_t + point0.x;
				intersection.y= delta.j*intersection_t + point0.y;
				intersection.z= delta.k*intersection_t + point0.z;
				project_point3d(&intersection, projection, sign, &point);

				if (convex_hull2d_test_point(vertex_count, vertices, &point, 0.05f))
				{
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

void leaf_map_get_leaf_bounds(
	const struct leaf_map *leaf_map,
	long leaf_index,
	real_point3d *center,
	real *radius)
{
	struct map_leaf *leaf = TAG_BLOCK_GET_ELEMENT(&leaf_map->leaves, leaf_index & LONG_MAX, struct map_leaf);
	real_rectangle3d bounds = *global_null_rectangle3d;
	real half_width, half_height, half_depth;

	if (leaf->portal_designators.count != 0)
	{
		short portal_designator_index;

		for (portal_designator_index = 0;
			portal_designator_index < leaf->portal_designators.count;
			portal_designator_index++)
		{
			struct leaf_portal *portal = TAG_BLOCK_GET_ELEMENT(&leaf_map->portals,
				*TAG_BLOCK_GET_ELEMENT(&leaf->portal_designators, portal_designator_index, long) & LONG_MAX,
				struct leaf_portal);
			short vertex_index;

			for (vertex_index = 0; vertex_index < portal->vertices.count; vertex_index++)
			{
				real_point3d *vertex = TAG_BLOCK_GET_ELEMENT(&portal->vertices, vertex_index, real_point3d);

				if (vertex->x < bounds.x0)
				{
					bounds.x0 = vertex->x;
				}
				if (vertex->x > bounds.x1)
				{
					bounds.x1 = vertex->x;
				}
				if (vertex->y < bounds.y0)
				{
					bounds.y0 = vertex->y;
				}
				if (vertex->y > bounds.y1)
				{
					bounds.y1 = vertex->y;
				}
				if (vertex->z < bounds.z0)
				{
					bounds.z0 = vertex->z;
				}
				if (vertex->z > bounds.z1)
				{
					bounds.z1 = vertex->z;
				}
			}
		}
	}
	else if (leaf->faces.count != 0)
	{
		short face_index;

		for (face_index = 0; face_index < leaf->faces.count; face_index++)
		{
			struct map_leaf_face *face = TAG_BLOCK_GET_ELEMENT(&leaf->faces, face_index, struct map_leaf_face);
			struct bsp3d_node *node = TAG_BLOCK_GET_ELEMENT(&leaf_map->bsp->nodes, face->node_index, struct bsp3d_node);
			real_plane3d *plane = TAG_BLOCK_GET_ELEMENT(&leaf_map->bsp->planes, node->plane_designator, real_plane3d);
			short projection = projection_from_vector3d(&plane->n);
			boolean sign = projection_sign_from_vector3d(&plane->n, projection);
			real_point3d point;
			short vertex_index;

			for (vertex_index = 0; vertex_index < face->vertices.count; vertex_index++)
			{
				project_point2d(
					TAG_BLOCK_GET_ELEMENT(&face->vertices, vertex_index, real_point2d),
					plane,
					projection,
					sign,
					&point);

				if (point.x < bounds.x0)
				{
					bounds.x0 = point.x;
				}
				if (point.x > bounds.x1)
				{
					bounds.x1 = point.x;
				}
				if (point.y < bounds.y0)
				{
					bounds.y0 = point.y;
				}
				if (point.y > bounds.y1)
				{
					bounds.y1 = point.y;
				}
				if (point.z < bounds.z0)
				{
					bounds.z0 = point.z;
				}
				if (point.z > bounds.z1)
				{
					bounds.z1 = point.z;
				}
			}
		}
	}

	center->x = (bounds.x1 + bounds.x0) * 0.5f;
	center->y = (bounds.y1 + bounds.y0) * 0.5f;
	center->z = (bounds.z1 + bounds.z0) * 0.5f;

	half_width = bounds.x1 - center->x;
	half_height = bounds.y1 - center->y;
	half_depth = bounds.z1 - center->z;

	*radius = square_root(half_width * half_width + half_height * half_height + half_depth * half_depth);

	return;
}

void render_debug_leaf_portals(
	const struct leaf_map *leaf_map,
	long leaf_index)
{
	struct map_leaf *leaf = TAG_BLOCK_GET_ELEMENT(&leaf_map->leaves, leaf_index & LONG_MAX, struct map_leaf);
	short portal_designator_index;

	for (portal_designator_index = 0;
		portal_designator_index < leaf->portal_designators.count;
		portal_designator_index++)
	{
		long portal_designator = *TAG_BLOCK_GET_ELEMENT(&leaf->portal_designators, portal_designator_index, long);

		render_debug_leaf_portal(leaf_map, portal_designator);
	}

	return;
}

void render_debug_leaf_faces(
	const struct leaf_map *map,
	long leaf_index)
{
	struct map_leaf *leaf = TAG_BLOCK_GET_ELEMENT(&map->leaves, leaf_index & LONG_MAX, struct map_leaf);
	real_argb_color color = {{0.2f, 1.0f, 0.0f, 0.0f}};
	real_point3d first_vertex;
	real_point3d second_vertex;
	real_point3d next_vertex;
	real_point3d previous_vertex;
	short face_index;

	match_assert(
		"c:\\halo\\SOURCE\\structures\\leaf_map.c",
		940,
		map->bsp);

	for (face_index = 0; face_index < leaf->faces.count; face_index++)
	{
		struct map_leaf_face *face = TAG_BLOCK_GET_ELEMENT(&leaf->faces, face_index, struct map_leaf_face);
		short vertex_index;

		leaf_face_get_vertex3d(map, face, 0, &first_vertex);
		leaf_face_get_vertex3d(map, face, 1, &second_vertex);
		previous_vertex = second_vertex;
		render_debug_line(TRUE, &first_vertex, &second_vertex, global_real_argb_white);

		for (vertex_index = 2; vertex_index < face->vertices.count; vertex_index++)
		{
			leaf_face_get_vertex3d(map, face, vertex_index, &next_vertex);
			render_debug_triangle(TRUE, &first_vertex, &previous_vertex, &next_vertex, &color);
			render_debug_line(TRUE, &previous_vertex, &next_vertex, global_real_argb_white);
			previous_vertex = next_vertex;
		}
	}

	return;
}

boolean leaf_map_closure(
	const struct leaf_map *leaf_map,
	long *marked_leaves,
	long *result)
{
	long leaf_index;

	if (marked_leaves != result)
	{
		csmemcpy(result, marked_leaves, BIT_VECTOR_SIZE_IN_BYTES(leaf_map->leaves.count));
	}

	for (leaf_index = 0; leaf_index < leaf_map->leaves.count; leaf_index++)
	{
		if (BIT_VECTOR_TEST_FLAG(result, leaf_index))
		{
			leaf_map_family_mark(leaf_map, result, leaf_index);
		}
	}

	return TRUE;
}

/* ---------- private code */

static void leaf_face_get_vertex3d(
	const struct leaf_map *map,
	struct map_leaf_face *face,
	short vertex_index,
	real_point3d *result)
{
	struct bsp3d_node *node = TAG_BLOCK_GET_ELEMENT(&map->bsp->nodes, face->node_index, struct bsp3d_node);
	real_plane3d *plane = TAG_BLOCK_GET_ELEMENT(&map->bsp->planes, node->plane_designator, real_plane3d);
	short projection = projection_from_vector3d(&plane->n);
	boolean sign = projection_sign_from_vector3d(&plane->n, projection);

	project_point2d(
		TAG_BLOCK_GET_ELEMENT(&face->vertices, vertex_index, real_point2d),
		plane,
		projection,
		sign,
		result);

	return;
}

static void leaf_map_family_mark(
	const struct leaf_map *leaf_map,
	long *marked_leaves,
	long leaf_index)
{
	struct map_leaf *leaf = TAG_BLOCK_GET_ELEMENT(&leaf_map->leaves, leaf_index & LONG_MAX, struct map_leaf);
	short portal_designator_index;

	for (portal_designator_index = 0;
		portal_designator_index < leaf->portal_designators.count;
		portal_designator_index++)
	{
		struct leaf_portal *portal = TAG_BLOCK_GET_ELEMENT(&leaf_map->portals,
			*TAG_BLOCK_GET_ELEMENT(&leaf->portal_designators, portal_designator_index, long) & LONG_MAX,
			struct leaf_portal);
		long neighbor_leaf_index = (portal->leaf_indices[0] != leaf_index)
			? portal->leaf_indices[0]
			: portal->leaf_indices[1];

		if (!BIT_VECTOR_TEST_FLAG(marked_leaves, neighbor_leaf_index))
		{
			BIT_VECTOR_SET_FLAG(marked_leaves, neighbor_leaf_index, TRUE);
			leaf_map_family_mark(leaf_map, marked_leaves, neighbor_leaf_index);
		}
	}

	return;
}
