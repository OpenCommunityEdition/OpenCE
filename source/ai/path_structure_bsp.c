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
#include "physics/collision_bsp.h"
#include "physics/collision_bsp_definitions.h"
#include "structures/structure_bsp_definitions.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

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
