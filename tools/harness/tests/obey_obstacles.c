/*
OBEY_OBSTACLES.C (test)

The real obey_obstacles_clear_way and its helpers (obey_obstacles.c): an
actor that follows a command list takes a go-to whose path cannot be made
as reached, and when none can be the list ends and it stands. In c10's
insertion two grunts of a squad, running its way slower than the 2 world
units a second that make a biped optional, were in the gap between two
rocks as the last one's path was made again: no way round was found and it
stood as the others ran on. For an actor that obeys, the bipeds ahead
that move its way are taken from the obstacles and the path is made once
more; a standing biped, one behind or one coming at it, an object that is
no biped, and an actor that does not obey are left as they were.
test_obey_obstacles.py takes the code from the sources (under_test.inc).
*/

#include "harness.h"
#include <math.h>

#include "config.inc"

/* the objects and actors the code reads, by index */
struct object_datum
{
	struct { real_vector3d translational_velocity; } object;
	struct { long actor_index; } unit;
};
struct unit_datum
{
	struct { real_vector3d translational_velocity; } object;
	struct { long actor_index; } unit;
};
struct actor_datum
{
	struct { short action; } state;
};

static struct object_datum test_objects[8];
static unsigned long test_object_types[8];
static struct actor_datum test_actors[2];

static void *object_try_and_get_and_verify_type(long object_index, unsigned long valid_type_flags)
{
	return object_index >= 0 && object_index < 8 && (test_object_types[object_index] & valid_type_flags) ?
		&test_objects[object_index] :
		NULL;
}

#define actor_get(index) (&test_actors[(index)])

#include "under_test.inc"

/* c10's: the grunt at the start, the way it was going, and two of its
squad in the gap ahead (tick 52) */
static real_point3d const start = { -112.552f, -66.268f, 39.44f };
static real_vector3d const way = { -0.718f, 0.690f, -0.091f };

static void object_set(long object_index, unsigned long types, real i, real j, real k)
{
	test_object_types[object_index] = types;
	test_objects[object_index].object.translational_velocity.i = i;
	test_objects[object_index].object.translational_velocity.j = j;
	test_objects[object_index].object.translational_velocity.k = k;
	test_objects[object_index].unit.actor_index = NONE;
}

static void disc_add(struct obstacles *obstacles, long object_index, short flags, real x, real y)
{
	struct obstacle_disc *disc = &obstacles->discs[obstacles->disc_count++];

	memset(disc, 0, sizeof(*disc));
	disc->object_index = object_index;
	disc->flags = flags;
	disc->obstacle_index = NONE;
	disc->center.x = x;
	disc->center.y = y;
	disc->radius = 0.2f;
	if (TEST_FLAG(flags, _disc_optional_bit))
		obstacles->disc_optional_count++;
}

/* the grunt (object 0, actor 0, obeying), the two of its squad ahead
(objects 1, optional, and 2), a rock between them (object 3, no unit) and a moving
vehicle ahead (object 4, a unit, no biped) */
static void scene(struct obstacles *obstacles, short action)
{
	memset(obstacles, 0, sizeof(*obstacles));
	object_set(0, _object_mask_unit | _object_mask_biped, 0.f, 0.f, 0.f);
	test_objects[0].unit.actor_index = 0;
	test_actors[0].state.action = action;
	object_set(1, _object_mask_unit | _object_mask_biped, -0.0389f, 0.0439f, -0.0046f);
	object_set(2, _object_mask_unit | _object_mask_biped, -0.0489f, 0.0153f, -0.0055f);
	object_set(3, 0, 0.f, 0.f, 0.f);
	object_set(4, _object_mask_unit, -0.2f, 0.2f, 0.f);
	disc_add(obstacles, 1, FLAG(_disc_optional_bit), -113.752f, -65.083f);
	disc_add(obstacles, 3, 0, -114.624f, -66.385f);
	disc_add(obstacles, 2, 0, -114.180f, -64.311f);
	disc_add(obstacles, 4, 0, -113.934f, -62.345f);
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	real_point2d const start2d = { start.x, start.y };

	/* the squad ahead, running its way, slower than an optional biped */
	CASE("squad ahead")
	{
		real_point2d const first = { -113.752f, -65.083f };
		real_point2d const second = { -114.180f, -64.311f };
		real_vector3d const first_velocity = { -0.0389f, 0.0439f, -0.0046f };
		real_vector3d const second_velocity = { -0.0489f, 0.0153f, -0.0055f };

		CHECK(obey_obstacles_clears_way(&start2d, &way, &first, &first_velocity), "the first not passed over");
		CHECK(obey_obstacles_clears_way(&start2d, &way, &second, &second_velocity), "the second not passed over");
		return 0;
	}
	/* standing in the way, or barely moving: an obstacle */
	CASE("standing")
	{
		real_point2d const center = { -113.752f, -65.083f };
		real_vector3d const still = { 0.f, 0.f, 0.f };
		real_vector3d const shuffling = { -0.004f, 0.004f, 0.f };

		CHECK(!obey_obstacles_clears_way(&start2d, &way, &center, &still), "a standing biped passed over");
		CHECK(!obey_obstacles_clears_way(&start2d, &way, &center, &shuffling), "a shuffling biped passed over");
		return 0;
	}
	/* behind it, or coming at it: an obstacle */
	CASE("behind or coming")
	{
		real_point2d const behind = { -111.552f, -67.268f };
		real_point2d const ahead = { -113.752f, -65.083f };
		real_vector3d const along = { -0.0389f, 0.0439f, 0.f };
		real_vector3d const coming = { 0.0389f, -0.0439f, 0.f };

		CHECK(!obey_obstacles_clears_way(&start2d, &way, &behind, &along), "a biped behind passed over");
		CHECK(!obey_obstacles_clears_way(&start2d, &way, &ahead, &coming), "a biped coming at it passed over");
		return 0;
	}
	/* an actor that obeys: the two of its squad taken, the rock and the
	vehicle kept in order, the optional ones counted again (none) */
	CASE("obeys")
	{
		struct obstacles obstacles;

		scene(&obstacles, _actor_action_obey);
		CHECK(obey_obstacles_clear_way(&obstacles, 0, &start, &way), "nothing taken");
		CHECK(obstacles.disc_count == 2, "%d discs kept", obstacles.disc_count);
		CHECK(obstacles.discs[0].object_index == 3 && obstacles.discs[1].object_index == 4,
			"kept %ld and %ld", obstacles.discs[0].object_index, obstacles.discs[1].object_index);
		CHECK(obstacles.disc_optional_count == 0, "%d optional", obstacles.disc_optional_count);
		return 0;
	}
	/* an actor that does not obey: as the original */
	CASE("does not obey")
	{
		struct obstacles obstacles;

		scene(&obstacles, _actor_action_guard);
		CHECK(!obey_obstacles_clear_way(&obstacles, 0, &start, &way), "taken for an actor that guards");
		CHECK(obstacles.disc_count == 4, "%d discs kept", obstacles.disc_count);
		return 0;
	}
	/* no actor (a unit no actor drives), or nothing to take */
	CASE("no actor")
	{
		struct obstacles obstacles;

		scene(&obstacles, _actor_action_obey);
		test_objects[0].unit.actor_index = NONE;
		CHECK(!obey_obstacles_clear_way(&obstacles, 0, &start, &way), "taken with no actor");
		scene(&obstacles, _actor_action_obey);
		object_set(1, _object_mask_unit | _object_mask_biped, 0.f, 0.f, 0.f);
		object_set(2, _object_mask_unit | _object_mask_biped, 0.f, 0.f, 0.f);
		CHECK(!obey_obstacles_clear_way(&obstacles, 0, &start, &way), "taken with the squad standing");
		CHECK(obstacles.disc_count == 4 && obstacles.disc_optional_count == 1, "%d discs, %d optional",
			obstacles.disc_count, obstacles.disc_optional_count);
		return 0;
	}

	return 2;
}
