/*
COLLISION_BSP.C

symbols in this file:
00136C00 0060:
	_collision_surface_edge_count (0000)
00136C60 00e0:
	_collision_surface_polygon (0000)
00136D40 0030:
	_code_00136d40 (0000)
00136D70 0050:
	_render_debug_collision_vertex (0000)
00136DC0 0080:
	_render_debug_collision_edge (0000)
00136E40 0070:
	_render_debug_collision_surface (0000)
00136EB0 0040:
	_render_debug_collision_bsp (0000)
00136EF0 0070:
	_collision_edge_length (0000)
00136F60 00e0:
	_collision_surface_perimeter (0000)
00137040 01a0:
	_collision_surface_area (0000)
001371E0 0050:
	_collision_surface_project_point2d (0000)
00137230 0100:
	_collision_surface_test_point2d (0000)
00137330 0230:
	_collision_surface_find_closest_point2d (0000)
00137560 01c0:
	_collision_surface_test_line2d (0000)
00137720 0370:
	_code_00137720 (0000)
00137A90 0130:
	_code_00137a90 (0000)
00137BC0 00d0:
	_code_00137bc0 (0000)
00137C90 02a0:
	_code_00137c90 (0000)
00137F30 00a0:
	_code_00137f30 (0000)
00137FD0 0190:
	_code_00137fd0 (0000)
00138160 0210:
	_code_00138160 (0000)
00138370 0070:
	_collision_bsp_test_pill_new (0000)
001383E0 0320:
	_code_001383e0 (0000)
00138700 0320:
	_code_00138700 (0000)
00138A20 01e0:
	_code_00138a20 (0000)
00138C00 00d0:
	_collision_bsp_test_sphere (0000)
00138CD0 00f0:
	_collision_bsp_test_vector (0000)
00138DC0 0110:
	_code_00138dc0 (0000)
00138ED0 05e0:
	_code_00138ed0 (0000)
001394B0 0080:
	_collision_bsp_test_pill (0000)
0028AFF8 0033:
	??_C@_0DD@MEMDNOLF@point_count?$DMMAXIMUM_VERTICES_PER@ (0000)
0028B02C 0027:
	??_C@_0CH@BOKFNJDK@c?3?2halo?2SOURCE?2physics?2collision@ (0000)
0028B054 003e:
	??_C@_0DO@KILIPDDJ@data?9?$DOstack_depth?$DO?$DN0?5?$CG?$CG?5data?9?$DOst@ (0000)
0028B094 0004:
	__real@39000000 (0000)
00456EB0 0010:
	_bss_00456eb0 (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "collision_bsp.h"
#include "collision_bsp_definitions.h"
#include "tag_files/tag_groups.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

short collision_surface_edge_count(
	struct collision_bsp const *bsp,
	long surface_index)
{
	short edge_count = 0;
	struct collision_surface const *surface;
	long first_edge_index;
	long edge_index;

	surface = TAG_BLOCK_GET_ELEMENT(
		&bsp->surfaces,
		surface_index,
		struct collision_surface);
	first_edge_index = surface->first_edge_index;
	edge_index = first_edge_index;

	do
	{
		struct collision_edge const *edge = TAG_BLOCK_GET_ELEMENT(
			&bsp->edges,
			edge_index,
			struct collision_edge);
		boolean const reverse = edge->surface_indices[1] == surface_index;

		edge_count++;
		edge_index = edge->edge_indices[reverse];
	}
	while (edge_index != first_edge_index);

	return edge_count;
}

short collision_surface_polygon(
	struct collision_bsp const *bsp,
	long surface_index,
	real_point3d *points)
{
	short point_count = 0;
	struct collision_surface const *surface;
	long first_edge_index;
	long edge_index;

	surface = TAG_BLOCK_GET_ELEMENT(
		&bsp->surfaces,
		surface_index,
		struct collision_surface);
	first_edge_index = surface->first_edge_index;
	edge_index = first_edge_index;

	do
	{
		struct collision_edge const *edge = TAG_BLOCK_GET_ELEMENT(
			&bsp->edges,
			edge_index,
			struct collision_edge);
		boolean const reverse = edge->surface_indices[1] == surface_index;
		struct collision_vertex const *vertex = TAG_BLOCK_GET_ELEMENT(
			&bsp->vertices,
			edge->vertex_indices[reverse],
			struct collision_vertex);

		match_assert(
			"c:\\halo\\SOURCE\\physics\\collision_bsp.c",
			0xe1,
			point_count<MAXIMUM_VERTICES_PER_COLLISION_SURFACE);

		points[point_count++] = vertex->point;
		edge_index = edge->edge_indices[reverse];
	}
	while (edge_index != first_edge_index);

	return point_count;
}

real collision_edge_length(
	struct collision_bsp const *bsp,
	long edge_index)
{
	struct collision_edge const *edge = TAG_BLOCK_GET_ELEMENT(
		&bsp->edges,
		edge_index,
		struct collision_edge);
	struct collision_vertex const *vertex0 = TAG_BLOCK_GET_ELEMENT(
		&bsp->vertices,
		edge->vertex_indices[0],
		struct collision_vertex);
	struct collision_vertex const *vertex1 = TAG_BLOCK_GET_ELEMENT(
		&bsp->vertices,
		edge->vertex_indices[1],
		struct collision_vertex);

	return distance3d(&vertex0->point, &vertex1->point);
}

real collision_surface_perimeter(
	struct collision_bsp const *bsp,
	long surface_index)
{
	real perimeter = 0.f;
	struct collision_surface const *surface = TAG_BLOCK_GET_ELEMENT(
		&bsp->surfaces,
		surface_index,
		struct collision_surface);
	long const first_edge_index = surface->first_edge_index;
	long edge_index = first_edge_index;

	do
	{
		struct collision_edge const *edge = TAG_BLOCK_GET_ELEMENT(
			&bsp->edges,
			edge_index,
			struct collision_edge);
		boolean const reverse = edge->surface_indices[1] == surface_index;
		struct collision_vertex const *vertex0 = TAG_BLOCK_GET_ELEMENT(
			&bsp->vertices,
			edge->vertex_indices[reverse],
			struct collision_vertex);
		struct collision_vertex const *vertex1 = TAG_BLOCK_GET_ELEMENT(
			&bsp->vertices,
			edge->vertex_indices[!reverse],
			struct collision_vertex);

		perimeter += distance3d(&vertex0->point, &vertex1->point);
		edge_index = edge->edge_indices[reverse];
	}
	while (edge_index != first_edge_index);

	return perimeter;
}

real collision_surface_area(
	struct collision_bsp const *bsp,
	long surface_index)
{
	real_plane3d plane;
	real_vector3d p_vector;
	real_vector3d q_vector;
	real_vector3d cross;
	struct collision_vertex const *anchor;
	struct collision_vertex const *p;
	struct collision_vertex const *q;
	struct collision_surface const *surface;
	struct collision_edge const *edge;
	boolean reverse;
	boolean owner;
	real area = 0.f;

	surface = TAG_BLOCK_GET_ELEMENT(
		&bsp->surfaces,
		surface_index,
		struct collision_surface);
	edge = TAG_BLOCK_GET_ELEMENT(
		&bsp->edges,
		surface->first_edge_index,
		struct collision_edge);
	reverse = edge->surface_indices[1] == surface_index;
	anchor = TAG_BLOCK_GET_ELEMENT(
		&bsp->vertices,
		edge->vertex_indices[reverse],
		struct collision_vertex);
	bsp3d_get_plane_from_designator(
		&bsp->bsp3d,
		surface->plane_designator,
		&plane);
	edge = TAG_BLOCK_GET_ELEMENT(
		&bsp->edges,
		edge->edge_indices[reverse],
		struct collision_edge);
	owner = edge->surface_indices[1] == surface_index;
	reverse = owner;

	if (edge->edge_indices[reverse] != surface->first_edge_index)
	{
		do
		{
			p = TAG_BLOCK_GET_ELEMENT(
				&bsp->vertices,
				edge->vertex_indices[reverse],
				struct collision_vertex);
			q = TAG_BLOCK_GET_ELEMENT(
				&bsp->vertices,
				edge->vertex_indices[!owner],
				struct collision_vertex);
			vector_from_points3d(&anchor->point, &p->point, &p_vector);
			vector_from_points3d(&anchor->point, &q->point, &q_vector);
			cross_product3d(&p_vector, &q_vector, &cross);
			area += dot_product3d(&cross, &plane.n);

			edge = TAG_BLOCK_GET_ELEMENT(
				&bsp->edges,
				edge->edge_indices[reverse],
				struct collision_edge);
			owner = edge->surface_indices[1] == surface_index;
			reverse = owner;
		}
		while (edge->edge_indices[reverse] != surface->first_edge_index);

		if (area > 0.f)
		{
			return area;
		}
	}

	return 0.f;
}

real_point3d *collision_surface_project_point2d(
	struct collision_bsp const *bsp,
	long surface_index,
	short projection,
	boolean sign,
	real_point2d const *point,
	real_point3d *result)
{
	struct collision_surface const *surface = TAG_BLOCK_GET_ELEMENT(
		&bsp->surfaces,
		surface_index,
		struct collision_surface);
	real_plane3d const *plane = TAG_BLOCK_GET_ELEMENT(
		&bsp->bsp3d.planes,
		surface->plane_designator & LONG_MAX,
		real_plane3d);

	project_point2d(point, plane, projection, sign, result);
	return result;
}

boolean collision_surface_test_point2d(
	struct collision_bsp const *bsp,
	long surface_index,
	short projection,
	boolean sign,
	real_point2d const *point)
{
	struct collision_surface const *surface = TAG_BLOCK_GET_ELEMENT(
		&bsp->surfaces,
		surface_index,
		struct collision_surface);
	long const first_edge_index = surface->first_edge_index;
	long edge_index = first_edge_index;

	do
	{
		struct collision_edge const *edge = TAG_BLOCK_GET_ELEMENT(
			&bsp->edges,
			edge_index,
			struct collision_edge);
		boolean const reverse = edge->surface_indices[1] == surface_index;
		struct collision_vertex const *vertex0 = TAG_BLOCK_GET_ELEMENT(
			&bsp->vertices,
			edge->vertex_indices[reverse],
			struct collision_vertex);
		struct collision_vertex const *vertex1 = TAG_BLOCK_GET_ELEMENT(
			&bsp->vertices,
			edge->vertex_indices[!reverse],
			struct collision_vertex);
		real_point2d point0;
		real_point2d point1;
		real_vector2d vector0;
		real_vector2d vector1;

		project_point3d(&vertex0->point, projection, sign, &point0);
		project_point3d(&vertex1->point, projection, sign, &point1);
		vector_from_points2d(&point0, point, &vector0);
		vector_from_points2d(&point1, point, &vector1);

		if (cross_product2d(&vector0, &vector1) > 0.f)
		{
			return FALSE;
		}

		edge_index = edge->edge_indices[reverse];
	}
	while (edge_index != first_edge_index);

	return TRUE;
}

/* ---------- private code */
