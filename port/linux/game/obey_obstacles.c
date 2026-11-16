/*
OBEY_OBSTACLES.C

An actor that follows a command list (obey: the scripted runs of the
cinematics and set pieces) takes each go-to whose path cannot be made as
reached, and goes on to the next; when none can be made the list ends and
the actor stands where it is. A path is made around the bipeds and objects
within 4 world units (path_avoid_obstacles), and the bipeds ahead that move
its way are passed over (optional, path_obstacles.c) only when they move
faster than 2 world units a second: a squad running together, slower, shuts
the way for as long as one is in it. In c10's insertion two of the four
grunts that run from the Pelican were in the gap between two rocks as the
last one's path was made again (tick 52): no path could be made, to any of
its points, and with its list ended it guarded where it stood, some 7
seconds, as the others ran on. On the Xbox, whose arithmetic differs from
the port's, the squad was elsewhere at that tick and it ran with them.

When no path is made for an actor that obeys, path_avoid_obstacles calls
obey_obstacles_clear_way, which takes from the obstacles the bipeds ahead
that move its way (at more than OBEY_OBSTACLES_MINIMUM_SPEED), and the path
is made once more: the actor follows them as they clear the way, and its
path is made again with them at its next refresh. Nothing changes for a
path the original makes, or for an actor that does not obey (its choice
is made again on its own), and the AI runs on the host alone: nothing the
machines send or agree on changes.

In a cinematic a squad that marches together plans round its own: in d40's
ending (x70, launch_1) the flood forms flood_2 and flood_3 walk some 0.6
world units behind flood_1, on nearly its line and at its pace, and each
time their paths were made again (every 16 ticks) they were given a point
just to its side. Each turned for it, took it as reached as it came within
its stopping distance, and turned back for its goal, behind flood_1 again:
their facing swung some 20 degrees twice a second as they walked. So in a
cinematic, before any path of an actor that obeys is made,
obey_obstacles_follow takes from the obstacles the bipeds ahead that move
its way, within 45 degrees of it (OBEY_OBSTACLES_FOLLOW_ANGLE_COSINE_SQUARED):
it walks on behind them, as one of a column, kept from them by their bodies'
collision as before. One crossing its way, or one ahead that stands, is
still planned round, and nothing changes outside a cinematic.
*/

/* ---------- headers */

#include "cseries.h"
#include "math/real_math.h"
#include "memory/data.h"
#include "objects/objects.h"
#include "units/units.h"
#include "ai/actors.h"
#include "ai/path.h"
#include "cutscene/cinematics.h"

/* ---------- constants */

/* the least speed along the actor's way, in world units a tick, of a
biped passed over (a standing biped's is none) */
#define OBEY_OBSTACLES_MINIMUM_SPEED (0.3f / TICKS_PER_SECOND)
/* in a cinematic, the most a biped followed may move off the actor's way
(obey_obstacles_follow): 45 degrees, as the square of its cosine */
#define OBEY_OBSTACLES_FOLLOW_ANGLE_COSINE_SQUARED 0.5f

/* ---------- private code */

/* whether a biped, its disc at center and moving at velocity, is ahead of
the actor at start on its way (movement_direction, a unit vector) and moves
along it */
static boolean obey_obstacles_clears_way(
	real_point2d const *start,
	real_vector3d const *movement_direction,
	real_point2d const *center,
	real_vector3d const *velocity)
{
	real ahead = (center->x - start->x) * movement_direction->i + (center->y - start->y) * movement_direction->j;

	return ahead > 0.f && dot_product3d(velocity, movement_direction) > OBEY_OBSTACLES_MINIMUM_SPEED;
}

/* whether a biped that clears the way (obey_obstacles_clears_way) moves
along it, within OBEY_OBSTACLES_FOLLOW_ANGLE_COSINE_SQUARED, not across it */
static boolean obey_obstacles_moves_along(
	real_vector3d const *movement_direction,
	real_vector3d const *velocity)
{
	real along = dot_product3d(velocity, movement_direction);

	return along * along > OBEY_OBSTACLES_FOLLOW_ANGLE_COSINE_SQUARED * magnitude_squared3d(velocity);
}

/* takes the discs marked in clears from the obstacles, keeping the rest in
order, and counts the optional ones again: whether any was taken */
static boolean obey_obstacles_remove(
	struct obstacles *obstacles,
	boolean const *clears)
{
	short disc_index;
	short kept_count = 0;
	short optional_count = 0;

	for (disc_index = 0; disc_index < obstacles->disc_count; disc_index++)
	{
		if (!clears[disc_index])
		{
			obstacles->discs[kept_count] = obstacles->discs[disc_index];
			if (TEST_FLAG(obstacles->discs[kept_count].flags, _disc_optional_bit))
			{
				optional_count++;
			}
			kept_count++;
		}
	}
	if (kept_count == obstacles->disc_count)
	{
		return FALSE;
	}
	obstacles->disc_count = kept_count;
	obstacles->disc_optional_count = optional_count;

	return TRUE;
}

/* whether the object a path is made for is a unit whose actor obeys */
static boolean obey_obstacles_obeys(
	long source_object_index)
{
	struct object_datum *object = source_object_index != NONE ? object_try_and_get_and_verify_type(source_object_index, _object_mask_unit) : NULL;
	long actor_index = object ? ((struct unit_datum *)object)->unit.actor_index : NONE;

	return actor_index != NONE && actor_get(actor_index)->state.action == _actor_action_obey;
}

/* takes from the obstacles the bipeds ahead of the actor at start_point
that clear its way (obey_obstacles_clears_way) and, if along_only, move
along it (obey_obstacles_moves_along): whether any was taken */
static boolean obey_obstacles_take(
	struct obstacles *obstacles,
	real_point3d const *start_point,
	real_vector3d const *movement_direction,
	boolean along_only)
{
	boolean clears[MAXIMUM_DISC_COUNT];
	short disc_index;

	for (disc_index = 0; disc_index < obstacles->disc_count; disc_index++)
	{
		struct obstacle_disc const *disc = &obstacles->discs[disc_index];
		struct object_datum *object = disc->object_index != NONE ?
			object_try_and_get_and_verify_type(disc->object_index, _object_mask_biped) :
			NULL;

		clears[disc_index] = object &&
			obey_obstacles_clears_way((real_point2d const *)start_point, movement_direction, &disc->center,
				&object->object.translational_velocity) &&
			(!along_only || obey_obstacles_moves_along(movement_direction, &object->object.translational_velocity));
	}

	return obey_obstacles_remove(obstacles, clears);
}

/* ---------- public code */

int obey_obstacles_clear_way(
	struct obstacles *obstacles,
	long source_object_index,
	real_point3d const *start_point,
	real_vector3d const *movement_direction)
{
	if (!obey_obstacles_obeys(source_object_index))
	{
		return FALSE;
	}

	return obey_obstacles_take(obstacles, start_point, movement_direction, FALSE);
}

int obey_obstacles_follow(
	struct obstacles *obstacles,
	long source_object_index,
	real_point3d const *start_point,
	real_vector3d const *movement_direction)
{
	if (!cinematic_in_progress() || !obey_obstacles_obeys(source_object_index))
	{
		return FALSE;
	}

	return obey_obstacles_take(obstacles, start_point, movement_direction, TRUE);
}
