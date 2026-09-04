/*
COLLISION_BSP.C

symbols in this file:
00136C00 0060:
	_collision_surface_edge_count (0000)
00136C60 00e0:
	_collision_surface_polygon (0000)
00136D40 0030:
	_add_feature (0000)
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
	_collision_surface_test_sphere (0000)
00137A90 0130:
	_code_00137a90 (0000)
00137BC0 00d0:
	_code_00137bc0 (0000)
00137C90 02a0:
	_code_00137c90 (0000)
00137F30 00a0:
	_bsp2d_test_sphere_recursive (0000)
00137FD0 0190:
	_code_00137fd0 (0000)
00138160 0210:
	_code_00138160 (0000)
00138370 0070:
	_collision_bsp_test_pill_new (0000)
001383E0 0320:
	_bsp3d_test_sphere_recursive (0000)
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
	_collision_bsp_usage_times (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "collision_bsp.h"
#include "collision_bsp_definitions.h"
#include "collision_usage.h"
#include "render/render_debug.h"
#include "scenario/scenario.h"
#include "tag_files/tag_groups.h"

/* ---------- constants */

enum
{
	MAXIMUM_BSP3D_DEPTH = 128,
};

enum collision_surface_flags
{
	_collision_surface_two_sided_bit,
	_collision_surface_invisible_bit,
	_collision_surface_climbable_bit,
	_collision_surface_breakable_bit,
};

/* ---------- macros */

/* ---------- structures */

struct collision_bsp_test_pill_new_context
{
	struct collision_bsp const *bsp;
	short breakable_surface_count;
	short pad;
	byte const *breakable_surface_flags;
	real_point3d const *point;
	real_vector3d const *vector;
	real radius;
	real *t;
	real_vector3d *normal;
	long surface_index;
	byte flags;
	byte pad2[3];
	long leaf_index;
};

typedef char collision_bsp_test_pill_new_context_size_assert[
	sizeof(struct collision_bsp_test_pill_new_context) == 0x2C ? 1 : -1];

struct collision_bsp_test_pill_context
{
	struct collision_bsp const *bsp;
	real_point3d const *point;
	real_vector3d const *vector;
	real radius;
	struct collision_bsp_test_pill_result *result;
	long stack_depth;
	long traversal_stack[133];
};

typedef char collision_bsp_test_pill_context_size_assert[
	sizeof(struct collision_bsp_test_pill_context) == 0x22C ? 1 : -1];

struct collision_bsp_test_sphere_context
{
	struct collision_bsp const *bsp;
	short breakable_surface_count;
	short pad;
	byte const *breakable_surface_flags;
	real_point3d const *center;
	real radius;
	struct collision_bsp_test_sphere_result *result;
	long stack_depth;
	long plane_stack[MAXIMUM_BSP3D_DEPTH];
	short projection_axis;
	boolean projection_sign;
	byte pad2;
	real_point2d center2d;
};

typedef char collision_bsp_test_sphere_context_size_assert[
	sizeof(struct collision_bsp_test_sphere_context) == 0x228 ? 1 : -1];
typedef char collision_bsp_test_sphere_context_plane_stack_offset_assert[
	offsetof(struct collision_bsp_test_sphere_context, plane_stack) == 0x1C ? 1 : -1];
typedef char collision_bsp_test_sphere_context_projection_axis_offset_assert[
	offsetof(struct collision_bsp_test_sphere_context, projection_axis) == 0x21C ? 1 : -1];
typedef char collision_bsp_test_sphere_context_center2d_offset_assert[
	offsetof(struct collision_bsp_test_sphere_context, center2d) == 0x220 ? 1 : -1];

struct collision_leaf
{
	word flags;
	short bsp2d_reference_count;
	long first_bsp2d_reference_index;
};

struct bsp2d_reference
{
	long plane_designator;
	long root_index;
};

typedef char collision_leaf_size_assert[
	sizeof(struct collision_leaf) == 0x08 ? 1 : -1];
typedef char bsp2d_reference_size_assert[
	sizeof(struct bsp2d_reference) == 0x08 ? 1 : -1];

struct collision_bsp_test_vector_context
{
	unsigned long flags;
	struct collision_bsp const *bsp;
	short breakable_surface_count;
	short pad;
	byte const *breakable_surface_flags;
	real_point3d const *point;
	real_vector3d const *vector;
	struct collision_bsp_test_vector_result *result;
	long surface_index;
	byte result_flags;
	byte pad2[3];
	long leaf_index;
};

typedef char collision_bsp_test_vector_context_size_assert[
	sizeof(struct collision_bsp_test_vector_context) == 0x28 ? 1 : -1];

struct collision_bsp_usage_times
{
	LARGE_INTEGER vector;
	LARGE_INTEGER sphere;
};

/* ---------- prototypes */

boolean code_00137c90(
	struct collision_bsp_test_pill_new_context *context,
	long node_index,
	long stack_depth,
	real maximum_t);
boolean code_00138ed0(
	struct collision_bsp_test_pill_context *context,
	long node_index);
static void add_feature(
	long *count,
	long *indices,
	long index);
static void collision_surface_test_sphere(
	struct collision_bsp_test_sphere_context *data,
	long surface_index);
static void bsp2d_test_sphere_recursive(
	struct collision_bsp_test_sphere_context *data,
	long child_index);
static void bsp3d_test_sphere_recursive(
	struct collision_bsp_test_sphere_context *data,
	long node_index);
boolean code_00138700(
	struct collision_bsp_test_vector_context *context,
	long node_index,
	long stack_depth,
	real maximum_t);
void render_debug_collision_edge(
	struct collision_bsp *bsp,
	long edge_index,
	real_matrix4x3 const *matrix,
	real_argb_color const *color);

/* ---------- globals */

struct collision_bsp_usage_times collision_bsp_usage_times = { 0 };

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

void render_debug_collision_vertex(
	struct collision_bsp *bsp,
	long vertex_index,
	real_matrix4x3 const *matrix,
	real scale,
	real_argb_color const *color)
{
	struct collision_vertex const *vertex;
	real_point3d const *point;
	real_point3d transformed_point;

	vertex = TAG_BLOCK_GET_ELEMENT(
		&bsp->vertices,
		vertex_index,
		struct collision_vertex);
	point = &vertex->point;

	if (matrix)
	{
		point = matrix4x3_transform_point(
			matrix,
			point,
			&transformed_point);
	}

	render_debug_point(TRUE, point, scale, color);

	return;
}

void render_debug_collision_edge(
	struct collision_bsp *bsp,
	long edge_index,
	real_matrix4x3 const *matrix,
	real_argb_color const *color)
{
	struct collision_edge const *edge;
	struct collision_vertex const *vertex0;
	struct collision_vertex const *vertex1;
	real_point3d const *point0;
	real_point3d const *point1;
	real_point3d transformed_point0;
	real_point3d transformed_point1;

	edge = TAG_BLOCK_GET_ELEMENT(
		&bsp->edges,
		edge_index,
		struct collision_edge);
	vertex0 = TAG_BLOCK_GET_ELEMENT(
		&bsp->vertices,
		edge->vertex_indices[0],
		struct collision_vertex);
	vertex1 = TAG_BLOCK_GET_ELEMENT(
		&bsp->vertices,
		edge->vertex_indices[1],
		struct collision_vertex);
	point0 = &vertex0->point;
	point1 = &vertex1->point;

	if (matrix)
	{
		point0 = matrix4x3_transform_point(
			matrix,
			point0,
			&transformed_point0);
		point1 = matrix4x3_transform_point(
			matrix,
			point1,
			&transformed_point1);
	}

	render_debug_line(TRUE, point0, point1, color);

	return;
}

void render_debug_collision_surface(
	struct collision_bsp *bsp,
	long surface_index,
	real_matrix4x3 const *matrix,
	real_argb_color const *color)
{
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

		render_debug_collision_edge(
			bsp,
			edge_index,
			matrix,
			color);
		edge_index = edge->edge_indices[reverse];
	}
	while (edge_index != first_edge_index);

	return;
}

void render_debug_collision_bsp(
	struct collision_bsp *bsp,
	real_matrix4x3 const *matrix)
{
	long edge_index;

	for (edge_index = 0; edge_index < bsp->edges.count; edge_index++)
	{
		render_debug_collision_edge(
			bsp,
			edge_index,
			matrix,
			global_real_argb_green);
	}

	return;
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

boolean collision_surface_find_closest_point2d(
	struct collision_bsp const *bsp,
	long surface_index,
	short projection,
	boolean sign,
	real_point2d const *point,
	real_point2d *result)
{
	struct collision_surface const *surface = TAG_BLOCK_GET_ELEMENT(
		&bsp->surfaces,
		surface_index,
		struct collision_surface);
	long first_edge_index = surface->first_edge_index;
	long edge_index = first_edge_index;
	boolean previous_before;
	boolean previous_after;
	boolean first_before;
	boolean first_after;
	boolean before;
	boolean after;

	do
	{
		struct collision_edge const *edge = TAG_BLOCK_GET_ELEMENT(
			&bsp->edges,
			edge_index,
			struct collision_edge);
		boolean reverse = edge->surface_indices[1] == surface_index;
		struct collision_vertex const *origin = TAG_BLOCK_GET_ELEMENT(
			&bsp->vertices,
			edge->vertex_indices[reverse],
			struct collision_vertex);
		struct collision_vertex const *target = TAG_BLOCK_GET_ELEMENT(
			&bsp->vertices,
			edge->vertex_indices[!reverse],
			struct collision_vertex);
		real_point2d origin2d;
		real_point2d target2d;
		real_vector2d edge_vector;
		real_vector2d point_vector;
		real dot;

		before = FALSE;
		after = FALSE;
		project_point3d(&origin->point, projection, sign, &origin2d);
		project_point3d(&target->point, projection, sign, &target2d);
		point_vector.i = point->x-origin2d.x;
		point_vector.j = point->y-origin2d.y;
		edge_vector.i = target2d.x-origin2d.x;
		edge_vector.j = target2d.y-origin2d.y;

		if (edge_vector.j*point_vector.i-edge_vector.i*point_vector.j > 0.f)
		{
			dot = dot_product2d(&point_vector, &edge_vector);
			if (dot < 0.f)
			{
				before = TRUE;
			}
			else
			{
				real length_squared = magnitude_squared2d(&edge_vector);

				if (dot > length_squared)
				{
					after = TRUE;
				}
				else
				{
					point_from_line2d(
						&origin2d,
						&edge_vector,
						dot/length_squared,
						result);
					return FALSE;
				}
			}
		}

		if (edge_index != first_edge_index)
		{
			if ((previous_after && (before || !after)) ||
				(before && (previous_after || !previous_before)))
			{
				*result = origin2d;
				return FALSE;
			}
		}
		else
		{
			first_before = before;
			first_after = after;
		}

		previous_before = before;
		previous_after = after;
		edge_index = edge->edge_indices[reverse];
	}
	while (edge_index != first_edge_index);

	if ((after && (first_before || !first_after)) ||
		(first_before && (after || !before)))
	{
		struct collision_edge const *edge = TAG_BLOCK_GET_ELEMENT(
			&bsp->edges,
			edge_index,
			struct collision_edge);
		boolean reverse = edge->surface_indices[1] == surface_index;
		struct collision_vertex const *origin = TAG_BLOCK_GET_ELEMENT(
			&bsp->vertices,
			edge->vertex_indices[reverse],
			struct collision_vertex);

		project_point3d(&origin->point, projection, sign, result);
		return FALSE;
	}

	*result = *point;
	return TRUE;
}

boolean collision_surface_test_line2d(
	struct collision_bsp const *bsp,
	long surface_index,
	short projection,
	boolean sign,
	real_point2d const *point,
	real_vector2d const *direction,
	struct collision_surface_test_line2d_result *result)
{
	struct collision_surface const *surface = TAG_BLOCK_GET_ELEMENT(
		&bsp->surfaces,
		surface_index,
		struct collision_surface);
	long const first_edge_index = surface->first_edge_index;
	long edge_index = first_edge_index;

	result->enter_t = REAL_MIN;
	result->enter_edge_index = NONE;
	result->enter_surface_index = NONE;
	result->exit_t = REAL_MAX;
	result->exit_edge_index = NONE;
	result->exit_surface_index = NONE;

	do
	{
		struct collision_edge const *edge = TAG_BLOCK_GET_ELEMENT(
			&bsp->edges,
			edge_index,
			struct collision_edge);
		boolean const reverse = edge->surface_indices[1] == surface_index;
		struct collision_vertex const *vertex0 = TAG_BLOCK_GET_ELEMENT(
			&bsp->vertices,
			edge->vertex_indices[0],
			struct collision_vertex);
		struct collision_vertex const *vertex1 = TAG_BLOCK_GET_ELEMENT(
			&bsp->vertices,
			edge->vertex_indices[1],
			struct collision_vertex);
		real_vector2d edge_vector;
		real_vector2d point_vector;
		volatile real edge_cross;
		real point_cross;

		edge_vector.i = vertex1->point.x-vertex0->point.x;
		edge_vector.j = vertex1->point.y-vertex0->point.y;
		point_vector.i = point->x-vertex0->point.x;
		point_vector.j = point->y-vertex0->point.y;
		edge_cross = cross_product2d(direction, &edge_vector);
		point_cross = cross_product2d(&edge_vector, &point_vector);

		if (edge_cross != 0.f)
		{
			point_cross /= edge_cross;
			if ((edge_cross < 0.f) != reverse)
			{
				if (point_cross > result->enter_t)
				{
					result->enter_t = point_cross;
					result->enter_edge_index = edge_index;
					result->enter_surface_index = edge->surface_indices[!reverse];
				}
			}
			else if (point_cross < result->exit_t)
			{
				result->exit_t = point_cross;
				result->exit_edge_index = edge_index;
				result->exit_surface_index = edge->surface_indices[!reverse];
			}
		}
		else if ((point_cross < 0.f) != reverse)
		{
			result->enter_t = REAL_MAX;
			result->enter_edge_index = edge_index;
			result->enter_surface_index = edge->surface_indices[!reverse];
			result->exit_t = REAL_MIN;
			result->exit_edge_index = edge_index;
			result->exit_surface_index = edge->surface_indices[!reverse];
		}

		edge_index = edge->edge_indices[reverse];
	}
	while (edge_index != first_edge_index);

	return result->enter_t > result->exit_t;
}

boolean collision_bsp_test_pill_new(
	struct collision_bsp const *bsp,
	short breakable_surface_count,
	byte const *breakable_surface_flags,
	real_point3d const *point,
	real_vector3d const *vector,
	real radius,
	real *t,
	real_vector3d *normal)
{
	struct collision_bsp_test_pill_new_context context;

	context.bsp = bsp;
	context.breakable_surface_count = breakable_surface_count;
	context.breakable_surface_flags = breakable_surface_flags;
	context.point = point;
	context.vector = vector;
	context.radius = radius;
	context.t = t;
	context.normal = normal;
	context.surface_index = NONE;
	context.flags = 0;
	context.leaf_index = NONE;
	*t = REAL_MAX;

	return code_00137c90(&context, 0, 0, 1.f);
}

boolean collision_bsp_test_pill(
	struct collision_bsp const *bsp,
	real_point3d const *point,
	real_vector3d const *vector,
	real radius,
	real maximum_t,
	struct collision_bsp_test_pill_result *result)
{
	struct collision_bsp_test_pill_context context;

	context.bsp = bsp;
	context.point = point;
	context.vector = vector;
	context.radius = radius;
	context.result = result;
	context.stack_depth = 0;
	result->t = maximum_t < 0.f ? 0.f : maximum_t;
	result->leaf_count = 0;

	return code_00138ed0(&context, 0);
}

boolean collision_bsp_test_sphere(
	struct collision_bsp const *bsp,
	short breakable_surface_count,
	byte const *breakable_surface_flags,
	real_point3d const *center,
	real radius,
	struct collision_bsp_test_sphere_result *result)
{
	struct collision_bsp_test_sphere_context context;
	short collision_function = 6 + (bsp == global_collision_bsp);

	collision_log_usage(collision_function);
	collision_log_start_time(&collision_bsp_usage_times.sphere);

	context.bsp = bsp;
	context.breakable_surface_count = breakable_surface_count;
	context.breakable_surface_flags = breakable_surface_flags;
	context.center = center;
	context.radius = radius;
	context.result = result;
	context.stack_depth = 0;
	result->leaf_count = 0;
	result->surface_count = 0;
	result->edge_count = 0;
	result->vertex_count = 0;

	bsp3d_test_sphere_recursive(&context, 0);
	collision_log_end_time(
		collision_function,
		collision_bsp_usage_times.sphere.QuadPart);

	return result->surface_count > 0 || result->edge_count > 0;
}

boolean collision_bsp_test_vector(
	unsigned long flags,
	struct collision_bsp const *bsp,
	short breakable_surface_count,
	byte const *breakable_surface_flags,
	real_point3d const *point,
	real_vector3d const *vector,
	real maximum_t,
	struct collision_bsp_test_vector_result *result)
{
	struct collision_bsp_test_vector_context context;
	short collision_function = 4 + (bsp == global_collision_bsp);
	boolean return_value;
	real t;

	collision_log_usage(collision_function);
	collision_log_start_time(&collision_bsp_usage_times.vector);

	context.flags = flags;
	context.bsp = bsp;
	context.breakable_surface_count = breakable_surface_count;
	context.breakable_surface_flags = breakable_surface_flags;
	context.point = point;
	context.vector = vector;
	context.result = result;
	result->t = maximum_t < 0.f ? 0.f : maximum_t;
	context.surface_index = NONE;
	context.leaf_index = NONE;
	result->leaf_count = 0;
	context.result_flags = 0;

	t = PIN(maximum_t, 0.f, 1.f);
	return_value = code_00138700(&context, 0, 0, t);
	collision_log_end_time(
		collision_function,
		collision_bsp_usage_times.vector.QuadPart);

	return return_value;
}

/* ---------- private code */

static void add_feature(
	long *count,
	long *indices,
	long index)
{
	short feature_index;

	for (feature_index = 0; feature_index < *count; feature_index++)
	{
		if (indices[feature_index] == index)
			return;
	}

	if (*count < 256)
	{
		indices[*count] = index;
		(*count)++;
	}

	return;
}

static void collision_surface_test_sphere(
	struct collision_bsp_test_sphere_context *data,
	long surface_index)
{
	struct collision_surface const *surface = TAG_BLOCK_GET_ELEMENT(
		&data->bsp->surfaces,
		surface_index,
		struct collision_surface);
	byte breakable_surface_index;
	long edge_index;
	real radius_squared;
	boolean hit_feature = FALSE;

	if (TEST_FLAG(surface->flags, _collision_surface_breakable_bit))
	{
		breakable_surface_index = surface->breakable_surface_index;
		if (breakable_surface_index < data->breakable_surface_count &&
			!BIT_VECTOR_TEST_FLAG(
				(long const *)data->breakable_surface_flags,
				breakable_surface_index))
		{
			return;
		}
	}

	radius_squared = data->radius * data->radius;
	edge_index = surface->first_edge_index;
	do
	{
		struct collision_edge const *edge = TAG_BLOCK_GET_ELEMENT(
			&data->bsp->edges,
			edge_index,
			struct collision_edge);
		boolean reverse = edge->surface_indices[1] == surface_index;
		long vertex_index = edge->vertex_indices[reverse];
		struct collision_vertex const *vertex = TAG_BLOCK_GET_ELEMENT(
			&data->bsp->vertices,
			vertex_index,
			struct collision_vertex);
		real delta_x = vertex->point.x - data->center->x;
		real delta_y = vertex->point.y - data->center->y;
		real delta_z = vertex->point.z - data->center->z;
		real distance_squared =
			delta_x * delta_x +
			delta_y * delta_y +
			delta_z * delta_z;

		if (distance_squared <= radius_squared)
		{
			add_feature(
				&data->result->vertex_count,
				data->result->vertex_indices,
				vertex_index);
			hit_feature = TRUE;
		}

		edge_index = edge->edge_indices[reverse];
	}
	while (edge_index != surface->first_edge_index);

	edge_index = surface->first_edge_index;
	do
	{
		struct collision_edge const *edge = TAG_BLOCK_GET_ELEMENT(
			&data->bsp->edges,
			edge_index,
			struct collision_edge);
		boolean reverse = edge->surface_indices[1] == surface_index;
		struct collision_vertex const *vertex0 = TAG_BLOCK_GET_ELEMENT(
			&data->bsp->vertices,
			edge->vertex_indices[reverse],
			struct collision_vertex);
		struct collision_vertex const *vertex1 = TAG_BLOCK_GET_ELEMENT(
			&data->bsp->vertices,
			edge->vertex_indices[!reverse],
			struct collision_vertex);
		real_vector3d edge_vector;

		vector_from_points3d(&vertex0->point, &vertex1->point, &edge_vector);
		if (fast_vector_intersects_sphere(
			&vertex0->point,
			&edge_vector,
			data->center,
			data->radius))
		{
			add_feature(
				&data->result->edge_count,
				data->result->edge_indices,
				edge_index);
			hit_feature = TRUE;
		}

		edge_index = edge->edge_indices[reverse];
	}
	while (edge_index != surface->first_edge_index);

	if (!hit_feature)
	{
		edge_index = surface->first_edge_index;
		do
		{
			struct collision_edge const *edge = TAG_BLOCK_GET_ELEMENT(
				&data->bsp->edges,
				edge_index,
				struct collision_edge);
			boolean reverse = edge->surface_indices[1] == surface_index;
			struct collision_vertex const *vertex0 = TAG_BLOCK_GET_ELEMENT(
				&data->bsp->vertices,
				edge->vertex_indices[reverse],
				struct collision_vertex);
			struct collision_vertex const *vertex1 = TAG_BLOCK_GET_ELEMENT(
				&data->bsp->vertices,
				edge->vertex_indices[!reverse],
				struct collision_vertex);
			real_point2d point0;
			real_point2d point1;
			real_vector2d vector0;
			real_vector2d vector1;

			project_point3d(
				&vertex0->point,
				data->projection_axis,
				data->projection_sign,
				&point0);
			project_point3d(
				&vertex1->point,
				data->projection_axis,
				data->projection_sign,
				&point1);
			vector_from_points2d(&data->center2d, &point0, &vector0);
			vector_from_points2d(&data->center2d, &point1, &vector1);
			if (cross_product2d(&vector0, &vector1) < 0.f)
				return;

			edge_index = edge->edge_indices[reverse];
		}
		while (edge_index != surface->first_edge_index);
	}

	add_feature(
		&data->result->surface_count,
		data->result->surface_indices,
		surface_index);

	return;
}

static void bsp2d_test_sphere_recursive(
	struct collision_bsp_test_sphere_context *data,
	long child_index)
{
	real_point2d const *center2d = &data->center2d;

	while (!(child_index & LONG_MIN))
	{
		struct bsp2d_node const *node = TAG_BLOCK_GET_ELEMENT(
			&data->bsp->bsp2d.nodes,
			child_index,
			struct bsp2d_node);
		real distance =
			node->plane.n.j * center2d->y +
			center2d->x * node->plane.n.i -
			node->plane.d;
		boolean reaches_first_child = distance <= data->radius;
		boolean reaches_second_child = distance >= -data->radius;

		if (reaches_first_child)
			bsp2d_test_sphere_recursive(data, node->child_indices[0]);
		if (!reaches_second_child)
			return;

		child_index = node->child_indices[1];
	}

	collision_surface_test_sphere(data, child_index & LONG_MAX);

	return;
}

static void bsp3d_test_sphere_recursive(
	struct collision_bsp_test_sphere_context *data,
	long node_index)
{
	while (!(node_index & LONG_MIN))
	{
		struct bsp3d_node const *node = TAG_BLOCK_GET_ELEMENT(
			&data->bsp->bsp3d.nodes,
			node_index,
			struct bsp3d_node);
		real_plane3d const *plane = TAG_BLOCK_GET_ELEMENT(
			&data->bsp->bsp3d.planes,
			node->plane_designator,
			real_plane3d);
		real distance = plane3d_distance_to_point(plane, data->center);
		boolean reaches_second_child = distance < data->radius;
		boolean child_index = distance > -data->radius;

		if (child_index && reaches_second_child)
		{
			match_assert(
				"c:\\halo\\SOURCE\\physics\\collision_bsp.c",
				0x206,
				data->stack_depth>=0 && data->stack_depth<MAXIMUM_BSP3D_DEPTH);
			data->plane_stack[data->stack_depth++] =
				node->plane_designator | LONG_MIN;
			bsp3d_test_sphere_recursive(data, node->children[0]);

			data->stack_depth--;
			match_assert(
				"c:\\halo\\SOURCE\\physics\\collision_bsp.c",
				0x210,
				data->stack_depth>=0 && data->stack_depth<MAXIMUM_BSP3D_DEPTH);
			data->plane_stack[data->stack_depth++] =
				node->plane_designator & LONG_MAX;
			bsp3d_test_sphere_recursive(data, node->children[1]);
			data->stack_depth--;

			return;
		}

		node_index = node->children[child_index];
	}

	if (node_index != NONE)
	{
		long leaf_index = node_index & LONG_MAX;
		struct collision_leaf const *leaf = TAG_BLOCK_GET_ELEMENT(
			&data->bsp->leaves,
			leaf_index,
			struct collision_leaf);
		long reference_index;

		if (data->result->leaf_count < 256)
		{
			data->result->leaf_indices[data->result->leaf_count] = leaf_index;
			data->result->leaf_count++;
		}

		for (
			reference_index = leaf->first_bsp2d_reference_index;
			reference_index <
				leaf->first_bsp2d_reference_index + leaf->bsp2d_reference_count;
			reference_index++)
		{
			struct bsp2d_reference const *reference = TAG_BLOCK_GET_ELEMENT(
				&data->bsp->bsp2d_references,
				reference_index,
				struct bsp2d_reference);
			short stack_index;

			for (stack_index = 0; stack_index < data->stack_depth; stack_index++)
			{
				if (data->plane_stack[stack_index] == reference->plane_designator)
				{
					real_plane3d const *plane = TAG_BLOCK_GET_ELEMENT(
						&data->bsp->bsp3d.planes,
						reference->plane_designator & LONG_MAX,
						real_plane3d);
					real plane_distance = -(
						plane->n.i * data->center->x +
						(
							data->center->z * plane->n.k +
							data->center->y * plane->n.j) -
						plane->d);
					real_point3d projected_center;
					real absolute_i;
					real absolute_j;
					real absolute_k;
					short projection;
					boolean projection_sign;

					projected_center.x =
						plane->n.i * plane_distance + data->center->x;
					projected_center.y =
						plane->n.j * plane_distance + data->center->y;
					projected_center.z =
						plane->n.k * plane_distance + data->center->z;
					absolute_i = fabs(plane->n.i);
					absolute_j = fabs(plane->n.j);
					absolute_k = fabs(plane->n.k);
					if (absolute_k >= absolute_j && absolute_k >= absolute_i)
						projection = _z;
					else
						projection = absolute_j >= absolute_i ? _y : _x;
					data->projection_axis = projection;
					match_assert(
						"..\\math\\real_math.h",
						848,
						projection>=_x && projection<=_z);
					projection_sign = plane->n.n[projection] > 0.f;
					data->projection_sign = projection_sign !=
						(reference->plane_designator & LONG_MIN ? TRUE : FALSE);
					project_point3d(
						&projected_center,
						data->projection_axis,
						data->projection_sign,
						&data->center2d);
					bsp2d_test_sphere_recursive(data, reference->root_index);
					break;
				}
			}
		}
	}

	return;
}
