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
	_circle_intersect_ray (0000)
00050610 0080:
	_circle_tangents (0000)
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
#define rotate_vector2d rotate_vector2d_inline
#define project_point3d project_point3d_inline
#define distance_squared2d distance_squared2d_inline
#define point_in_circle point_in_circle_inline
#define distance_squared3d distance_squared3d_inline
#define point_in_sphere point_in_sphere_inline
#include "cseries.h"
#include "path.h"
#undef set_real_point2d
#undef rotate_vector2d
#undef project_point3d
#undef distance_squared2d
#undef point_in_circle
#undef distance_squared3d
#undef point_in_sphere

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

static boolean circle_intersect_ray(
	real_point2d const *center,
	real radius,
	real_point2d const *point,
	real_vector2d const *direction,
	real *distance);
static void circle_tangents(
	real_vector2d const *direction,
	real distance,
	real radius,
	real_vector2d *right_direction,
	real_vector2d *left_direction,
	real *tangent_distance);

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

real_vector2d *rotate_vector2d(
	real_vector2d const *vector,
	real sine,
	real cosine,
	real_vector2d *result)
{
	real j = sine * vector->i + cosine * vector->j;
	real i = cosine * vector->i - sine * vector->j;

	result->i = i;
	result->j = j;
	return result;
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

short obstacles_test_circle(
	struct obstacles const *obstacles,
	short ignore_disc_index,
	real_point2d const *center,
	real radius)
{
	short disc_index;

	for (disc_index = 0; disc_index < obstacles->disc_count; disc_index++)
	{
		if (disc_index != ignore_disc_index)
		{
			struct obstacle_disc const *disc = obstacles_get_disc(obstacles, disc_index);

			if (point_in_circle(center, &disc->center, disc->radius + radius))
			{
				return disc_index;
			}
		}
	}

	return NONE;
}

boolean obstacles_test_pill(
	struct obstacles const *obstacles,
	short ignore_disc_index,
	real_point2d const *point,
	real_vector2d const *direction,
	real radius,
	real distance,
	boolean ignore_optional,
	struct obstacles_test_pill_result *result)
{
	short disc_index;

	result->distance = distance;
	result->disc_index = NONE;
	result->obstacle_index = NONE;
	for (disc_index = 0; disc_index < obstacles->disc_count; disc_index++)
	{
		if (disc_index != ignore_disc_index)
		{
			struct obstacle_disc const *disc = obstacles_get_disc(obstacles, disc_index);

			if (!ignore_optional || !TEST_FLAG(disc->flags, _disc_optional_bit))
			{
				real disc_distance;

				if (circle_intersect_ray(&disc->center, disc->radius + radius,
					point, direction, &disc_distance) && result->distance > disc_distance)
				{
					result->distance = disc_distance;
					result->disc_index = disc_index;
					result->obstacle_index = disc->obstacle_index;
				}
			}
		}
	}

	return result->disc_index != NONE;
}

void obstacles_disc_tangents(
	struct obstacles const *obstacles,
	short disc_index,
	real_point2d const *point,
	real radius,
	real_vector2d *right_direction,
	real_vector2d *left_direction,
	real *tangent_distance)
{
	struct obstacle_disc const *disc = obstacles_get_disc(obstacles, disc_index);
	real_vector2d direction;
	real distance = normalize2d(vector_from_points2d(point, &disc->center, &direction));

	circle_tangents(
		&direction,
		distance,
		radius + disc->radius + 0.00390625f,
		right_direction,
		left_direction,
		tangent_distance);
	return;
}

void obstacles_disc_neighborhood(
	struct obstacles const *obstacles,
	real radius,
	short seed_disc_index,
	byte *disc_flags)
{
	short disc_stack[MAXIMUM_DISC_COUNT];
	short stack_top;
	unsigned long *disc_flag_words = (unsigned long *)disc_flags;

	match_assert(
		"c:\\halo\\SOURCE\\ai\\path_obstacles.c",
		0x183,
		obstacles->disc_count>=0 && obstacles->disc_count<=MAXIMUM_DISC_COUNT);
	csmemset(disc_flag_words, 0, BIT_VECTOR_SIZE_IN_BYTES(obstacles->disc_count));

	if (seed_disc_index != NONE)
	{
		match_assert(
			"c:\\halo\\SOURCE\\ai\\path_obstacles.c",
			0x18C,
			seed_disc_index>=0 && seed_disc_index<obstacles->disc_count);

		disc_stack[0] = seed_disc_index;
		stack_top = 1;
		BIT_VECTOR_SET_FLAG(disc_flag_words, seed_disc_index, TRUE);

		while (stack_top > 0)
		{
			short current_disc_index = disc_stack[--stack_top];
			struct obstacle_disc const *current_disc = obstacles_get_disc(obstacles, current_disc_index);
			short disc_index;

			for (disc_index = 0; disc_index < obstacles->disc_count; disc_index++)
			{
				if (!BIT_VECTOR_TEST_FLAG(disc_flag_words, disc_index))
				{
					struct obstacle_disc const *disc = obstacles_get_disc(obstacles, disc_index);

					if (point_in_circle_inline(
						&disc->center,
						&current_disc->center,
						(disc->radius + radius) + (current_disc->radius + radius)))
					{
						BIT_VECTOR_SET_FLAG(disc_flag_words, disc_index, TRUE);
						match_assert(
							"c:\\halo\\SOURCE\\ai\\path_obstacles.c",
							0x1A5,
							stack_top<MAXIMUM_DISC_COUNT);
						disc_stack[stack_top++] = disc_index;
					}
				}
			}
		}
	}

	return;
}

void obstacles_recompute(
	struct obstacles *obstacles,
	real radius)
{
	unsigned long disc_flags[BIT_VECTOR_SIZE_IN_LONGS(MAXIMUM_DISC_COUNT)];
	short disc_index;

	obstacles->obstacle_count = 0;
	for (disc_index = 0; disc_index < obstacles->disc_count; disc_index++)
		obstacles->discs[disc_index].obstacle_index = NONE;

	for (disc_index = 0; disc_index < obstacles->disc_count; disc_index++)
	{
		if (obstacles->discs[disc_index].obstacle_index == NONE)
		{
			short obstacle_index = obstacles->obstacle_count++;
			short neighborhood_disc_index;

			obstacles_disc_neighborhood(obstacles, radius, disc_index, (byte *)disc_flags);
			for (neighborhood_disc_index = 0;
				neighborhood_disc_index < obstacles->disc_count;
				neighborhood_disc_index++)
			{
				if (BIT_VECTOR_TEST_FLAG(disc_flags, neighborhood_disc_index))
					obstacles->discs[neighborhood_disc_index].obstacle_index = obstacle_index;
			}
		}
	}

	return;
}

/* ---------- private code */

static boolean circle_intersect_ray(
	real_point2d const *center,
	real radius,
	real_point2d const *point,
	real_vector2d const *direction,
	real *distance)
{
	real_vector2d offset;
	real projection = dot_product2d(direction, vector_from_points2d(point, center, &offset));

	if (projection > 0.0f)
	{
		real distance_squared_minus_radius_squared = magnitude_squared2d(&offset) - radius * radius;

		if (distance_squared_minus_radius_squared <= 0.0f)
		{
			*distance = 0.0f;
			return TRUE;
		}
		else
		{
			real discriminant = projection * projection - distance_squared_minus_radius_squared;

			if (discriminant >= 0.0f)
			{
				*distance = projection - square_root(discriminant);
				return TRUE;
			}
		}
	}

	return FALSE;
}

static void circle_tangents(
	real_vector2d const *direction,
	real distance,
	real radius,
	real_vector2d *right_direction,
	real_vector2d *left_direction,
	real *tangent_distance)
{
	real sine = MIN(radius / distance, 1.0f);
	real cosine = square_root(1.0f - sine * sine);

	rotate_vector2d(direction, -sine, cosine, right_direction);
	rotate_vector2d(direction, sine, cosine, left_direction);
	*tangent_distance = cosine * distance;
	return;
}
