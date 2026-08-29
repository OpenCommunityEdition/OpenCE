/*
PATH_OBSTACLE_AVOIDANCE.C

symbols in this file:
0004E6E0 0050:
	_obstacles_get_disc (0000)
0004E730 0030:
	_obstacle_from_disc (0000)
0004E760 0050:
	_path_get_step (0000)
0004E7B0 0060:
	_path_get_step_index (0000)
0004E810 0040:
	_heap_parent_index (0000)
0004E850 0010:
	_heap_left_index (0000)
0004E860 0010:
	_heap_right_index (0000)
0004E870 0060:
	_heap_cost (0000)
0004E8D0 00d0:
	_code_0004e8d0 (0000)
0004E9A0 01b0:
	_code_0004e9a0 (0000)
0004EB50 0190:
	_code_0004eb50 (0000)
0004ECE0 02a0:
	_code_0004ece0 (0000)
0004EF80 0060:
	_code_0004ef80 (0000)
0004EFE0 0070:
	_code_0004efe0 (0000)
0004F050 0260:
	_render_debug_path (0000)
0004F2B0 0040:
	_valid_real_point2d (0000)
0004F2F0 0220:
	_code_0004f2f0 (0000)
0004F510 01e0:
	_code_0004f510 (0000)
0004F6F0 0200:
	_code_0004f6f0 (0000)
0004F8F0 0330:
	_code_0004f8f0 (0000)
0004FC20 0130:
	_code_0004fc20 (0000)
0004FD50 0070:
	_code_0004fd50 (0000)
0004FDC0 0550:
	_path_avoid_obstacles (0000)
00050310 00e0:
	_render_debug_obstacle_path (0000)
0024E4E0 005f:
	??_C@_0FP@EDEGFDAA@disc_index?$DO?$DN0?5?$CG?$CG?5disc_index?$DMobst@ (0000)
0024E540 0019:
	??_C@_0BJ@LMDHFJFI@c?3?2halo?2source?2ai?2path?4h?$AA@ (0000)
0024E560 0063:
	??_C@_0GD@BIALPJBH@step_index?$DO?$DN0?5?$CG?$CG?5step_index?$DMpath@ (0000)
0024E5C4 002c:
	??_C@_0CM@IMJGFEPB@c?3?2halo?2SOURCE?2ai?2path_obstacle_@ (0000)
0024E5F0 0063:
	??_C@_0GD@EOCGPJDJ@heap_index?$DO?$DN0?5?$CG?$CG?5heap_index?$DMpath@ (0000)
0024E654 000d:
	??_C@_0N@FHNAKGNC@heap_index?$DO0?$AA@ (0000)
0024E664 0010:
	??_C@_0BA@OJJDPBG@?$CF3d?4?5?$CF?412g?5?$CI?$CFx?$CJ?$AA@ (0000)
0024E674 0025:
	??_C@_0CF@NHJKJPIK@please?5give?5this?5debug?4txt?5to?5ch@ (0000)
0024E69C 0018:
	??_C@_0BI@GONMKDOC@heap_verify?5failed?5?$CI?$CFs?$CJ?$AA@ (0000)
0024E6B4 0013:
	??_C@_0BD@OPIGCKBC@heap_insert?5output?$AA@ (0000)
0024E6C8 0012:
	??_C@_0BC@FFIJKMBF@heap_insert?5input?$AA@ (0000)
0024E6DC 0013:
	??_C@_0BD@JEBDACBO@heap_remove?5output?$AA@ (0000)
0024E6F0 0012:
	??_C@_0BC@HBIKAONP@heap_remove?5input?$AA@ (0000)
0024E704 0005:
	??_C@_04PBNHMBFG@goal?$AA@ (0000)
0024E70C 0026:
	??_C@_0CG@LBHAPEOA@?$CFs?3?5assert_valid_real_point2d?$CI?$CFf@ (0000)
0024E734 0006:
	??_C@_05FAGFPHJG@start?$AA@ (0000)
0024E73C 0021:
	??_C@_0CB@CJEIPHLL@?$CFs?3?5assert_valid_real?$CI0x?$CF08X?5?$CFf?$CJ@ (0000)
0024E760 001d:
	??_C@_0BN@KOMMMEHH@stack_top?$DMMAXIMUM_DISC_COUNT?$AA@ (0000)
0024E780 0046:
	??_C@_0EG@PAHGCCFL@result?4disc_index?$DO?$DN0?5?$CG?$CG?5result?4d@ (0000)
0024E7C8 0042:
	??_C@_0EC@PIPIECEP@seed_disc_index?$DO?$DN0?5?$CG?$CG?5seed_disc_@ (0000)
0024E810 0052:
	??_C@_0FC@HKCAOOKO@path?9?$DOobstacles?9?$DOdisc_count?$DO?$DN0?5?$CG@ (0000)
0024E864 0017:
	??_C@_0BH@FKFIAHK@obstacle?5path?5failure?$CB?$AA@ (0000)
0024E880 0050:
	??_C@_0FA@MIKGHDND@?$CIobstacle_step_index?5?$DO?$DN?50?$CJ?5?$CG?$CG?5?$CIo@ (0000)
0024E8D0 0030:
	??_C@_0DA@BINHGJIJ@input_step_count?5?$DM?$DN?5MAXIMUM_SMOO@ (0000)
00319D08 2142:
	_bss_00319d08 (0000)
	_debug_obstacle_path_on_failure (2140)
	_debug_obstacle_path (2141)
*/

/* ---------- headers */

#include "cseries/cseries.h"

#define valid_real_point2d valid_real_point2d_inline
#include "math/real_math.h"
#undef valid_real_point2d

#undef memcpy
#include <stddef.h>
#include <string.h>

/* ---------- constants */

enum
{
	MAXIMUM_DISC_COUNT = 128,
	MAXIMUM_OBSTACLE_AVOIDANCE_STEPS = 128,
};

/* ---------- macros */

/* ---------- structures */

struct structure_bsp;

struct obstacle_disc
{
	short flags;
	short obstacle_index;
	long object_index;
	real_point2d center;
	real radius;
	byte reserved14[4];
};

struct obstacles
{
	short obstacle_count;
	short disc_count;
	short disc_optional_count;
	byte reserved6[2];
	struct obstacle_disc discs[MAXIMUM_DISC_COUNT];
};

struct obstacle_path_step
{
	real_point2d point;
	long surface_index;
	real_vector2d direction;
	real distance;
	short obstacle_index;
	byte obstacle_direction_index;
	byte reserved1B;
	short obstructed_goal_step_indices[2];
	real total_distance;
	short previous_step_index;
	byte reserved26[2];
};

struct obstacle_path
{
	real radius;
	boolean ignore_broken_surfaces;
	byte reserved5[3];
	struct obstacles const *obstacles;
	struct structure_bsp const *structure;
	real_point2d goal;
	long goal_surface_index;
	short goal_obstacle_index;
	short goal_step_index;
	short best_goal_blocked_step_index;
	byte reserved22[2];
	real best_goal_blocked_distance;
	boolean goal_found_exactly;
	boolean finishing;
	boolean ignore_optional;
	byte reserved2B;
	short step_count;
	byte reserved2E[2];
	struct obstacle_path_step steps[MAXIMUM_OBSTACLE_AVOIDANCE_STEPS];
	short heap_count;
	short heap[MAXIMUM_OBSTACLE_AVOIDANCE_STEPS];
	byte reserved1532[2];
};

typedef char obstacle_path_step_size_assert[
	sizeof(struct obstacle_path_step) == 0x28 ? 1 : -1];
typedef char obstacle_disc_size_assert[
	sizeof(struct obstacle_disc) == 0x18 ? 1 : -1];
typedef char obstacle_disc_obstacle_index_offset_assert[
	offsetof(struct obstacle_disc, obstacle_index) == 0x2 ? 1 : -1];
typedef char obstacles_size_assert[
	sizeof(struct obstacles) == 0xC08 ? 1 : -1];
typedef char obstacles_disc_count_offset_assert[
	offsetof(struct obstacles, disc_count) == 0x2 ? 1 : -1];
typedef char obstacles_discs_offset_assert[
	offsetof(struct obstacles, discs) == 0x8 ? 1 : -1];
typedef char obstacle_path_size_assert[
	sizeof(struct obstacle_path) == 0x1534 ? 1 : -1];
typedef char obstacle_path_step_count_offset_assert[
	offsetof(struct obstacle_path, step_count) == 0x2C ? 1 : -1];
typedef char obstacle_path_steps_offset_assert[
	offsetof(struct obstacle_path, steps) == 0x30 ? 1 : -1];
typedef char obstacle_path_heap_count_offset_assert[
	offsetof(struct obstacle_path, heap_count) == 0x1430 ? 1 : -1];
typedef char obstacle_path_heap_offset_assert[
	offsetof(struct obstacle_path, heap) == 0x1432 ? 1 : -1];

/* ---------- prototypes */

struct obstacle_disc const *obstacles_get_disc(
	struct obstacles const *obstacles,
	short disc_index);

/* ---------- globals */

/* ---------- public code */

/* ---------- private code */

struct obstacle_disc const *obstacles_get_disc(
	struct obstacles const *obstacles,
	short disc_index)
{
	match_assert(
		"c:\\halo\\source\\ai\\path.h",
		0x18C,
		disc_index>=0 && disc_index<obstacles->disc_count && obstacles->disc_count<=MAXIMUM_DISC_COUNT);

	return &obstacles->discs[disc_index];
}

long obstacle_from_disc(
	struct obstacles const *obstacles,
	short disc_index)
{
	if (disc_index != NONE)
		return obstacles_get_disc(obstacles, disc_index)->obstacle_index;

	return NONE;
}

struct obstacle_path_step *path_get_step(
	struct obstacle_path *path,
	short step_index)
{
	match_assert("c:\\halo\\SOURCE\\ai\\path_obstacle_avoidance.c", 40, step_index>=0 && step_index<path->step_count && path->step_count<=MAXIMUM_OBSTACLE_AVOIDANCE_STEPS);

	return &path->steps[step_index];
}

unsigned short path_get_step_index(
	struct obstacle_path *path,
	short heap_index)
{
	match_assert(
		"c:\\halo\\SOURCE\\ai\\path_obstacle_avoidance.c",
		0x31,
		heap_index>=0 && heap_index<path->heap_count && path->heap_count<=MAXIMUM_OBSTACLE_AVOIDANCE_STEPS);

	return path->heap[heap_index];
}

short heap_parent_index(
	short heap_index)
{
	match_assert("c:\\halo\\SOURCE\\ai\\path_obstacle_avoidance.c", 57, heap_index>0);

	return (short)((heap_index - 1) >> 1);
}

short heap_left_index(
	short heap_index)
{
	short result = 2 * heap_index + 1;

	return result;
}

short heap_right_index(
	short heap_index)
{
	short result = 2 * (heap_index + 1);

	return result;
}

real heap_cost(
	struct obstacle_path *path,
	short heap_index)
{
	short step_index = path_get_step_index(path, heap_index);

	match_assert(
		"c:\\halo\\SOURCE\\ai\\path_obstacle_avoidance.c",
		0x28,
		step_index>=0 && step_index<path->step_count && path->step_count<=MAXIMUM_OBSTACLE_AVOIDANCE_STEPS);

	return path->steps[step_index].total_distance;
}

boolean valid_real_point2d(
	real_point2d const *point)
{
	real x_value = point->x;
	real y_value;
	unsigned long x_bits;
	unsigned long y_bits;

	memcpy(&x_bits, &x_value, sizeof(x_bits));

	return (x_bits & 0x7F800000) != 0x7F800000 &&
		(y_value = point->y,
		memcpy(&y_bits, &y_value, sizeof(y_bits)),
		(y_bits & 0x7F800000) != 0x7F800000);
}
