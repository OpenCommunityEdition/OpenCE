/*
PATH_OBSTACLES.C

symbols in this file:
000503F0 0020:
	_obstacles_new (0000)
00050410 0020:
	_set_real_point2d (0000)
00050430 0030:
	_rotate_vector2d (0000)
00050460 0090:
	_project_point3d (0000)
000504F0 0040:
	_point_in_circle (0000)
00050530 0050:
	_point_in_sphere (0000)
00050580 0090:
	_code_00050580 (0000)
00050610 0080:
	_code_00050610 (0000)
00050690 00d0:
	_obstacles_add_disc (0000)
00050760 0320:
	_obstacles_get_discs_in_sphere (0000)
00050A80 00a0:
	_obstacles_test_circle (0000)
00050B20 00f0:
	_obstacles_test_pill (0000)
00050C10 00e0:
	_obstacles_disc_tangents (0000)
00050CF0 0230:
	_obstacles_disc_neighborhood (0000)
00050F20 00b0:
	_obstacles_recompute (0000)
00050FD0 01c0:
	_render_debug_obstacles (0000)
0024E900 000b:
	??_C@_0L@LKENDBEE@?$HO?$CIsign?$CG?$HO1?$CJ?$AA@ (0000)
0024E90C 0021:
	??_C@_0CB@FEGCJDLB@projection?$DO?$DN_x?5?$CG?$CG?5projection?$DM?$DN_z@ (0000)
0024E930 0014:
	??_C@_0BE@DKJODJEH@?4?4?2math?2real_math?4h?$AA@ (0000)
0024E948 0051:
	??_C@_0FB@IMJPHNIA@obstacles?9?$DOobstacle_count?$DO?$DN0?5?$CG?$CG?5@ (0000)
0024E9A0 0046:
	??_C@_0EG@PILJPJIJ@obstacles?9?$DOdisc_count?$DO?$DN0?5?$CG?$CG?5obst@ (0000)
0024E9E8 0023:
	??_C@_0CD@MNFANEEP@c?3?2halo?2SOURCE?2ai?2path_obstacles@ (0000)
0024EA0C 000d:
	??_C@_0N@IDCAPIB@matt?5is?5bad?4?$AA@ (0000)
0024EA1C 0004:
	__real@3b800000 (0000)
0024EA20 003c:
	??_C@_0DM@HAPAFMPD@seed_disc_index?$DO?$DN0?5?$CG?$CG?5seed_disc_@ (0000)
0024EA5C 0004:
	__real@c0000000 (0000)
0024EA60 004a:
	??_C@_0EK@GINAEHH@disc?9?$DOobstacle_index?$DO?$DN0?5?$CG?$CG?5disc?9@ (0000)
002B7578 0800:
	_data_002b7578 (0000)
*/

/* ---------- headers */

#define set_real_point2d set_real_point2d_inline
#define project_point3d project_point3d_inline
#define distance_squared2d distance_squared2d_inline
#define point_in_circle point_in_circle_inline
#define distance_squared3d distance_squared3d_inline
#define point_in_sphere point_in_sphere_inline
#include "cseries.h"
#include "path.h"
#undef set_real_point2d
#undef project_point3d
#undef distance_squared2d
#undef point_in_circle
#undef distance_squared3d
#undef point_in_sphere

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

void obstacles_new(
	struct obstacles *obstacles)
{
	obstacles->obstacle_count = 0;
	obstacles->disc_count = 0;
	obstacles->disc_optional_count = 0;
	return;
}

real_point2d *set_real_point2d(
	real_point2d *p,
	real x,
	real y)
{
	p->x = x;
	p->y = y;
	return p;
}

real_point2d *project_point3d(
	real_point3d const *p3d,
	short projection,
	boolean sign,
	real_point2d *p2d)
{
	match_assert("..\\math\\real_math.h", 859, projection>=_x && projection<=_z);
	match_assert("..\\math\\real_math.h", 860, ~(sign&~1));

	set_real_point2d_inline(
		p2d,
		p3d->n[global_projection3d_mappings[projection][sign][0]],
		p3d->n[global_projection3d_mappings[projection][sign][1]]);
	return p2d;
}

boolean point_in_circle(
	real_point2d const *point,
	real_point2d const *center,
	real radius)
{
	return distance_squared2d_inline(point, center) <= (radius * radius);
}

boolean point_in_sphere(
	real_point3d const *point,
	real_point3d const *center,
	real radius)
{
	return distance_squared3d_inline(point, center) <= (radius * radius);
}

boolean obstacles_add_disc(
	struct obstacles *obstacles,
	long object_index,
	short flags,
	real_point3d const *center,
	real radius)
{
	struct obstacle_disc *disc;

	match_assert(
		"c:\\halo\\SOURCE\\ai\\path_obstacles.c",
		104,
		obstacles->disc_count>=0 && obstacles->disc_count<=MAXIMUM_DISC_COUNT);
	match_assert(
		"c:\\halo\\SOURCE\\ai\\path_obstacles.c",
		105,
		obstacles->obstacle_count>=0 && obstacles->obstacle_count<=obstacles->disc_count);
	if (obstacles->disc_count == MAXIMUM_DISC_COUNT)
	{
		return FALSE;
	}

	disc = &obstacles->discs[obstacles->disc_count++];
	if (TEST_FLAG(flags, _disc_optional_bit))
	{
		obstacles->disc_optional_count++;
	}
	disc->flags = flags;
	disc->object_index = object_index;
	disc->obstacle_index = NONE;
	project_point3d(center, _z, TRUE, &disc->center);
	disc->radius = radius;
	disc->height = center->z;

	return TRUE;
}

/* ---------- private code */
