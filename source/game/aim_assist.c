/*
AIM_ASSIST.C

symbols in this file:
00093C00 0050:
	_compute_attenuation (0000)
00093C50 0030:
	_compute_composite_attenuation (0000)
00093C80 00f0:
	_code_00093c80 (0000)
00093D70 00a0:
	_code_00093d70 (0000)
00093E10 0010:
	_reciprocal_square_root (0000)
00093E20 0060:
	_limit3d (0000)
00093E80 0020:
	_set_real_euler_angles2d (0000)
00093EA0 00f0:
	_aim_assist_clear_line_of_sight (0000)
00093F90 01a0:
	_code_00093f90 (0000)
00094130 01a0:
	_aim_assist_compute_target (0000)
000942D0 0110:
	_autoaim_compute_target (0000)
000943E0 0190:
	_code_000943e0 (0000)
00094570 0130:
	_code_00094570 (0000)
000946A0 0100:
	_aim_assist (0000)
000947A0 0340:
	_player_aim_projectile (0000)
00094AE0 0170:
	_local_player_aim_assist (0000)
0025AC3C 0021:
	??_C@_0CB@CDDNCJJI@c?3?2halo?2SOURCE?2game?2aim_assist?4c@ (0000)
0025AC60 0004:
	__real@43000000 (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "collision_usage.h"

/* ---------- constants */

enum
{
	_collision_result_object = 3,
	_collision_test_for_line_of_sight_flags = 0xC2AD,
};

/* ---------- macros */

/* ---------- structures */

typedef float real;

struct real_euler_angles2d
{
	real yaw;
	real pitch;
};
typedef struct real_euler_angles2d real_euler_angles2d;

struct real_point3d
{
	real x;
	real y;
	real z;
};
typedef struct real_point3d real_point3d;

struct real_vector3d
{
	real i;
	real j;
	real k;
};
typedef struct real_vector3d real_vector3d;

struct real_plane3d
{
	real_vector3d n;
	real d;
};

struct location
{
	long leaf_index;
	short cluster_index;
	word bonus;
};

struct collision_result
{
	short type;
	struct location start_location;
	struct location location;
	real t;
	real_point3d point;
	struct real_plane3d plane;
	short material_type;
	word material_type_pad;
	long object_index;
	short region_index;
	short node_index;
	short bsp_index;
	long surface_index;
	long plane_designator;
	byte flags;
	byte breakable_surface_index;
	short material_index;
};

typedef char aim_assist_collision_result_size_assert[
	sizeof(struct collision_result) == 0x50 ? 1 : -1];
typedef char aim_assist_collision_result_object_index_offset_assert[
	offsetof(struct collision_result, object_index) == 0x38 ? 1 : -1];

/* ---------- prototypes */

long object_get_ultimate_parent(long object_index);
boolean collision_test_vector(
	unsigned long flags,
	real_point3d const *point,
	real_vector3d const *vector,
	long ignore_object_index,
	struct collision_result *collision);

/* ---------- globals */

/* ---------- public code */

real_euler_angles2d *set_real_euler_angles2d(
	real_euler_angles2d *angles,
	real pitch,
	real yaw)
{
	angles->pitch = pitch;
	angles->yaw = yaw;
	return angles;
}

boolean aim_assist_clear_line_of_sight(
	real_point3d const *point0,
	real_point3d const *point1,
	long ignore_object_index,
	long target_object_index)
{
	boolean clear = FALSE;
	real_vector3d vector;
	struct collision_result collision;
	long ignore_parent_index;

	match_assert(
		"c:\\halo\\SOURCE\\game\\aim_assist.c",
		350,
		global_current_collision_user_depth < MAXIMUM_COLLISION_USER_STACK_DEPTH);
	global_current_collision_users[global_current_collision_user_depth++] = _collision_user_aim_assist;

	ignore_parent_index = object_get_ultimate_parent(ignore_object_index);
	vector.i = point1->x - point0->x;
	vector.j = point1->y - point0->y;
	vector.k = point1->z - point0->z;

	if (!collision_test_vector(
		_collision_test_for_line_of_sight_flags,
		point0,
		&vector,
		ignore_parent_index,
		&collision))
	{
		clear = TRUE;
	}
	else if (collision.type == _collision_result_object &&
		object_get_ultimate_parent(collision.object_index) ==
		object_get_ultimate_parent(target_object_index))
	{
		clear = TRUE;
	}

	match_assert(
		"c:\\halo\\SOURCE\\game\\aim_assist.c",
		367,
		global_current_collision_user_depth > 1);
	--global_current_collision_user_depth;

	return clear;
}

/* ---------- private code */
