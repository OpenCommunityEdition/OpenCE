/*
PATH_STRUCTURE_BSP.C

symbols in this file:
00051D80 01e0:
	_structure_test_ray2d (0000)
00051F60 0080:
	_structure_surfaces_are_equivalent (0000)
00051FE0 0070:
	_clip_empty_interval_by_solid_interval (0000)
00052050 0450:
	_structure_test_line2d (0000)
000524A0 0060:
	_structure_surface_index_from_point (0000)
00052500 0270:
	_structure_test_pill2d (0000)
0024EC0C 0040:
	_rdata_0024ec0c (0000)
0024EC4C 0027:
	??_C@_0CH@JLKIDLBA@c?3?2halo?2SOURCE?2ai?2path_structure@ (0000)
0024EC78 0008:
	__real@3fa99999a0000000 (0000)
0024EC80 0004:
	__real@3c000000 (0000)
0024EC88 0048:
	??_C@_0EI@IFBLLPMB@surface_index?$DO?$DN0?5?$CG?$CG?5surface_inde@ (0000)
0024ECD0 0007:
	??_C@_06KJFADLGN@result?$AA@ (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "path_structure_bsp.h"

#include "math/real_math.h"
#include "physics/breakable_surfaces.h"
#include "physics/collision_bsp.h"
#include "physics/collision_bsp_definitions.h"
#include "structures/structure_bsp_definitions.h"

/* ---------- constants */

enum
{
	_pathfinding_surface_walkable_bit = 6,
	_pathfinding_surface_breakable_bit = 7,
};

enum
{
	_collision_surface_breakable_bit = 3,
};

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

boolean structure_test_ray2d(
	struct structure_bsp const *structure,
	boolean ignore_broken_surfaces,
	real_point2d const *point,
	long surface_index,
	real_vector2d const *direction,
	real distance,
	struct structure_test_ray2d_result *result)
{
	struct collision_surface_test_line2d_result surface_result;
	struct collision_bsp const *bsp;
	byte *breakable_surface_flags;
	byte const *pathfinding_surfaces;

	bsp = TAG_BLOCK_GET_ELEMENT(&structure->collision_bsp, 0, struct collision_bsp);
	breakable_surface_flags = breakable_surface_flags_get();
	pathfinding_surfaces = structure->pathfinding_surfaces.address;

	collision_surface_test_line2d(
		bsp,
		surface_index,
		_z,
		TRUE,
		point,
		direction,
		&surface_result);

	while (TRUE)
	{
		long next_surface_index;
		byte pathfinding_surface_flags;
		boolean surface_passable;

		if (distance < surface_result.enter_t)
		{
			pathfinding_surface_flags = pathfinding_surfaces[surface_result.enter_surface_index];
			if (pathfinding_surface_flags)
			{
				surface_passable = TRUE;
				if (!ignore_broken_surfaces &&
					TEST_FLAG(pathfinding_surface_flags, _pathfinding_surface_breakable_bit))
				{
					struct collision_surface const *collision_surface;

					collision_surface = TAG_BLOCK_GET_ELEMENT(
						&bsp->surfaces,
						surface_result.enter_surface_index,
						struct collision_surface);
					match_assert(
						"c:\\halo\\SOURCE\\ai\\path_structure_bsp.c",
						105,
						TEST_FLAG(collision_surface->flags, _collision_surface_breakable_bit));
					surface_passable = BIT_VECTOR_TEST_FLAG(
						(long *)breakable_surface_flags,
						collision_surface->breakable_surface_index);
				}

				if (surface_passable && surface_result.enter_surface_index != NONE)
				{
					next_surface_index = surface_result.enter_surface_index;
					goto continue_from_surface;
				}
			}
		}

		if (distance > surface_result.exit_t)
		{
			pathfinding_surface_flags = pathfinding_surfaces[surface_result.exit_surface_index];
			if (pathfinding_surface_flags)
			{
				surface_passable = TRUE;
				if (!ignore_broken_surfaces &&
					TEST_FLAG(pathfinding_surface_flags, _pathfinding_surface_breakable_bit))
				{
					struct collision_surface const *collision_surface;

					collision_surface = TAG_BLOCK_GET_ELEMENT(
						&bsp->surfaces,
						surface_result.exit_surface_index,
						struct collision_surface);
					match_assert(
						"c:\\halo\\SOURCE\\ai\\path_structure_bsp.c",
						126,
						TEST_FLAG(collision_surface->flags, _collision_surface_breakable_bit));
					surface_passable = BIT_VECTOR_TEST_FLAG(
						(long *)breakable_surface_flags,
						collision_surface->breakable_surface_index);
				}

				if (surface_passable && surface_result.exit_surface_index != NONE)
				{
					next_surface_index = surface_result.exit_surface_index;
					goto continue_from_surface;
				}
			}
		}

		break;

continue_from_surface:
		collision_surface_test_line2d(
			bsp,
			surface_index = next_surface_index,
			_z,
			TRUE,
			point,
			direction,
			&surface_result);
	}

	if (distance < surface_result.enter_t)
	{
		result->distance = surface_result.enter_t;
		result->surface_index = surface_index;
		result->edge_index = surface_result.enter_edge_index;
		return TRUE;
	}

	if (distance > surface_result.exit_t)
	{
		result->distance = surface_result.exit_t;
		result->surface_index = surface_index;
		result->edge_index = surface_result.exit_edge_index;
		return TRUE;
	}

	result->distance = distance;
	result->surface_index = surface_index;
	result->edge_index = NONE;

	return FALSE;
}

boolean structure_surfaces_are_equivalent(
	struct structure_bsp const *structure,
	real_point2d const *destination_point,
	long destination_surface_index,
	long test_surface_index)
{
	struct collision_bsp const *bsp;
	real_point3d destination_point3d;
	real_point3d test_point3d;
	boolean result;

	bsp = TAG_BLOCK_GET_ELEMENT(&structure->collision_bsp, 0, struct collision_bsp);
	result = FALSE;

	if (destination_surface_index != NONE && test_surface_index != NONE)
	{
		collision_surface_project_point2d(
			bsp,
			destination_surface_index,
			_z,
			TRUE,
			destination_point,
			&destination_point3d);
		collision_surface_project_point2d(
			bsp,
			test_surface_index,
			_z,
			TRUE,
			destination_point,
			&test_point3d);
		result = fabs(destination_point3d.z - test_point3d.z) < 0.05f;
	}

	return result;
}

boolean structure_test_line2d(
	struct structure_bsp const *structure,
	boolean ignore_broken_surfaces,
	real_point2d const *p0,
	long p0_surface_index,
	real_point2d const *p1,
	long p1_surface_index,
	struct path_collision_result *result)
{
	struct collision_bsp const *bsp;
	byte const *pathfinding_surfaces;
	long const *breakable_surface_flags;
	boolean recursed;
	boolean reached_target;
	long surface_index;
	real_vector2d delta;

	bsp = TAG_BLOCK_GET_ELEMENT(&structure->collision_bsp, 0, struct collision_bsp);
	pathfinding_surfaces = structure->pathfinding_surfaces.address;
	breakable_surface_flags = (long const *)breakable_surface_flags_get();
	surface_index = p0_surface_index;
	recursed = FALSE;
	match_assert("c:\\halo\\SOURCE\\ai\\path_structure_bsp.c", 217, result);

	delta.i = p1->x - p0->x;
	delta.j = p1->y - p0->y;

	for (;;)
	{
		struct collision_surface const *surface;
		long edge_index;
		short edge_count;
		boolean crossed_any;
		real_point3d point_in_surface;

		surface = TAG_BLOCK_GET_ELEMENT(&bsp->surfaces, surface_index, struct collision_surface);
		edge_index = surface->first_edge_index;
		point_in_surface = *global_origin3d;
		edge_count = 0;
		crossed_any = FALSE;
		reached_target = FALSE;

		do
		{
			struct collision_edge const *edge;
			boolean on_right_side;
			struct collision_vertex const *vertex_a;
			struct collision_vertex const *vertex_b;
			real_vector2d a_to_p0;
			real_vector2d edge_vector;
			real_vector2d b_to_p1;
			real_vector2d b_to_p0;

			edge = TAG_BLOCK_GET_ELEMENT(&bsp->edges, edge_index, struct collision_edge);
			on_right_side = surface_index == edge->surface_indices[1];
			vertex_a = TAG_BLOCK_GET_ELEMENT(
				&bsp->vertices,
				edge->vertex_indices[!on_right_side],
				struct collision_vertex);
			vertex_b = TAG_BLOCK_GET_ELEMENT(
				&bsp->vertices,
				edge->vertex_indices[on_right_side],
				struct collision_vertex);

			edge_vector.i = vertex_b->point.x - vertex_a->point.x;
			edge_vector.j = vertex_b->point.y - vertex_a->point.y;
			b_to_p1.i = p1->x - vertex_a->point.x;
			b_to_p1.j = p1->y - vertex_a->point.y;
			b_to_p0.i = vertex_a->point.x - p0->x;
			b_to_p0.j = vertex_a->point.y - p0->y;
			a_to_p0.i = vertex_b->point.x - p0->x;
			a_to_p0.j = vertex_b->point.y - p0->y;

			if (edge->surface_indices[!on_right_side] == p1_surface_index)
				reached_target = TRUE;

			point_in_surface.x += vertex_a->point.x;
			point_in_surface.y += vertex_a->point.y;
			point_in_surface.z += vertex_a->point.z;
			edge_count++;

			if (cross_product2d(&edge_vector, &b_to_p1) > 0.0f)
			{
				crossed_any = TRUE;
				if (cross_product2d(&delta, &b_to_p0) > 0.0f &&
					cross_product2d(&a_to_p0, &delta) > 0.0f)
				{
					long neighbor_surface_index;
					byte pathfinding_surface_flags;
					boolean passable;

					neighbor_surface_index = edge->surface_indices[!on_right_side];
					pathfinding_surface_flags = pathfinding_surfaces[neighbor_surface_index];
					passable = TEST_FLAG(
						pathfinding_surface_flags,
						_pathfinding_surface_walkable_bit);
					if (!ignore_broken_surfaces &&
						passable &&
						TEST_FLAG(
							pathfinding_surface_flags,
							_pathfinding_surface_breakable_bit))
					{
						struct collision_surface const *collision_surface;

						collision_surface = TAG_BLOCK_GET_ELEMENT(
							&bsp->surfaces,
							neighbor_surface_index,
							struct collision_surface);
						match_assert(
							"c:\\halo\\SOURCE\\ai\\path_structure_bsp.c",
							274,
							TEST_FLAG(collision_surface->flags, _collision_surface_breakable_bit));
						passable = BIT_VECTOR_TEST_FLAG(
							breakable_surface_flags,
							collision_surface->breakable_surface_index);
					}

					if (passable)
					{
						surface_index = neighbor_surface_index;
						goto continue_outer;
					}

					{
						real edge_length;
						real t;
						real_point2d hit_point;

						edge_length = magnitude2d(&edge_vector);
						t = (cross_product2d(&edge_vector, &b_to_p0) -
							edge_length * (1.0f / 128.0f)) /
							cross_product2d(&edge_vector, &delta);
						hit_point.x = delta.i * t + p0->x;
						hit_point.y = delta.j * t + p0->y;
						collision_surface_project_point2d(
							bsp,
							surface_index,
							_z,
							TRUE,
							&hit_point,
							&result->point);
						result->surface_index = surface_index;
						result->edge_index = edge_index;
						result->collision = TRUE;
						result->t = t;
						return TRUE;
					}
				}
			}

			edge_index = edge->edge_indices[on_right_side];
		}
		while (edge_index != surface->first_edge_index);

		if (crossed_any)
		{
			real scale;

			match_assert(
				"c:\\halo\\SOURCE\\ai\\path_structure_bsp.c",
				316,
				surface_index >= 0 && surface_index < structure->pathfinding_surfaces.count);
			scale = 1.0f / edge_count;
			point_in_surface.x *= scale;
			point_in_surface.y *= scale;

			if (!recursed && pathfinding_surfaces[surface_index])
			{
				struct path_collision_result p0_result;

				if (!structure_test_line2d(
					structure,
					ignore_broken_surfaces,
					(real_point2d const *)&point_in_surface,
					surface_index,
					p0,
					NONE,
					&p0_result))
				{
					recursed = TRUE;
					surface_index = p0_result.surface_index;
					continue;
				}
			}

			goto blocked_at_start;
		}

		break;

continue_outer:
		;
	}

	if (surface_index == p1_surface_index || reached_target || p1_surface_index == NONE)
	{
		collision_surface_project_point2d(
			bsp,
			surface_index,
			_z,
			TRUE,
			p1,
			&result->point);
		result->surface_index = surface_index;
		result->edge_index = NONE;
		result->collision = FALSE;
		result->t = 1.0f;
		return FALSE;
	}

blocked_at_start:
	collision_surface_project_point2d(
		bsp,
		p0_surface_index,
		_z,
		TRUE,
		p0,
		&result->point);
	result->surface_index = NONE;
	result->edge_index = NONE;
	result->collision = TRUE;
	result->t = 0.0f;

	return TRUE;
}

boolean clip_empty_interval_by_solid_interval(
	real *empty_t0,
	real *empty_t1,
	real solid_t0,
	real solid_t1)
{
	real clipped_solid_t0;
	real clipped_solid_t1;

	if (*empty_t0 > solid_t1)
		clipped_solid_t1 = *empty_t0;
	else
		clipped_solid_t1 = solid_t1;
	solid_t1 = clipped_solid_t1;

	clipped_solid_t0 = MIN(solid_t0, *empty_t1);

	if (*empty_t1 - solid_t1 > clipped_solid_t0 - *empty_t0)
		*empty_t0 = solid_t1;
	else
		*empty_t1 = clipped_solid_t0;

	return *empty_t0 > *empty_t1;
}

long structure_surface_index_from_point(
	struct structure_bsp const *structure,
	boolean ignore_broken_surfaces,
	real_point2d const *known_point,
	long known_surface_index,
	real_point2d *point)
{
	struct path_collision_result result;

	if (known_surface_index != NONE)
	{
		structure_test_line2d(
			structure,
			ignore_broken_surfaces,
			known_point,
			known_surface_index,
			point,
			NONE,
			&result);

		point->x = result.point.x;
		point->y = result.point.y;

		if (result.surface_index == NONE)
			return known_surface_index;

		return result.surface_index;
	}

	return NONE;
}

/* ---------- private code */
