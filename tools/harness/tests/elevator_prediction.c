/*
ELEVATOR_PREDICTION.C (test)

The host's taking of a co-op client's player where the client says
(port/linux/game/network_distributed.c's distributed_take_prediction), on a
moving elevator. The client's elevator follows the host's, and the client's
word comes a round trip after it, so the height it gives is where the host's
elevator was a round trip before. Going up, taken whole, it puts the host's
copy under its own elevator's floor, where the game's copy falls through and
the client's player dies after it. Taken across only, the copy rides the
host's elevator through movement, stops and reversals. After leaving a
stopped elevator, the client's word is taken whole. test_elevator_prediction.py takes the functions
(under_test.inc) and the bound's structure and tolerance, and the drift
figures the function notes for the tests (config.inc); what
they call besides is stubbed: the speed and height limits pass every move
(the cases move no faster than a player may), and applying a state puts the
unit there.
*/

#include "harness.h"

#include <limits.h>
#include <math.h>

/* ---------- the fake world */

#define _object_mask_biped 1
#define _object_mask_device 2
#define _object_mask_machine 2
#define _biped_airborne_bit 0

/* the objects: a player's unit (a biped) and an elevator (a device) */
struct fake_object
{
	long mask;
	struct
	{
		real_point3d position;
	} object;
	struct
	{
		long elevator_object_index;
		long support_surface_index;
		char elevator_ticks;
		unsigned flags;
	} biped;
	struct
	{
		real position_velocity;
	} device;
};
#define object_datum fake_object
#define biped_datum fake_object
#define device_datum fake_object

enum
{
	UNIT = 1,
	ELEVATOR = 2,
	NUMBER_OF_OBJECTS
};
static struct fake_object objects[NUMBER_OF_OBJECTS];

static void *object_try_and_get_and_verify_type(long object_index, long mask)
{
	return object_index >= 0 && object_index < NUMBER_OF_OBJECTS && (objects[object_index].mask & mask) ?
		&objects[object_index] : NULL;
}
#define object_get(object_index) (&objects[object_index])

static long game_time;
#define game_time_get() game_time

/* a client's player's word, as the host has it */
struct distributed_unit_state
{
	real_point3d position;
};

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define MAXIMUM_TRACKED_PLAYERS 1
#define PREDICTION_ANCHOR_TICKS 30
#define PREDICTION_JITTER_TICKS 2
#define HOST_BLEND_DISTANCE 0.5f

#include "config.inc"

static struct
{
	boolean valid;
	long time;
	struct distributed_unit_state state;
	long taken_time;
	long taken_host_time;
} distributed_predictions[MAXIMUM_TRACKED_PLAYERS];
static struct
{
	boolean valid;
	long unit_index;
	long time;
	long host_time;
	real_point3d position;
	long taken_host_time;
	real_point3d taken_host_position;
} distributed_accepted[MAXIMUM_TRACKED_PLAYERS];
static struct
{
	real ground_height;
	real top_height;
	long ground_time;
} distributed_host_speeds[MAXIMUM_TRACKED_PLAYERS];

/* (every move within the limits; no higher than a player may be) */
#define distributed_on_foot_move_valid(bound, from, to, ticks) TRUE
#define distributed_on_foot_ceiling(player_index, bound) ((bound)->rise)
#define distributed_on_foot_fall_speed(bound, height) 0.0f

/* the unit put where the host takes it to be */
static boolean apply(long unit_index, real_point3d const *position)
{
	objects[unit_index].object.position = *position;
	return TRUE;
}
#define distributed_apply_state(unit_index, state, position, tolerance, blend, speed, fall) apply(unit_index, position)

#include "under_test.inc"

/* ---------- a ride */

/* the host's elevator, a tick: it moves, and carries its rider as
device_machines.c's does */
static void elevator_tick(real *floor, real rise)
{
	*floor += rise;
	if (objects[UNIT].biped.elevator_object_index == ELEVATOR)
		objects[UNIT].object.position.z += rise;
}

/* a player on the elevator (NONE: on the ground) as it rises rise a tick
for three seconds, and the client's word on them each tick: a step across,
at the height the elevator had round_trip ticks before. The host's copy is
never under its own elevator's floor, and is across where the client
says. */
static void ride(long elevator_index, real rise, long round_trip)
{
	struct distributed_on_foot_bound bound;
	real floor = 10.0f;
	real client_x = 0.0f;
	long tick;

	memset(&bound, 0, sizeof(bound));
	bound.rise = 1000.0f;
	objects[UNIT].mask = _object_mask_biped;
	objects[UNIT].biped.elevator_object_index = elevator_index;
	objects[UNIT].biped.support_surface_index = 1;
	objects[UNIT].biped.elevator_ticks = 60;
	objects[UNIT].object.position.z = floor;
	objects[ELEVATOR].mask = _object_mask_device;
	objects[ELEVATOR].device.position_velocity = rise;
	for (tick = 1; tick <= 90; tick++)
	{
		game_time = tick;
		elevator_tick(&floor, rise);
		client_x += 0.05f;
		distributed_predictions[0].valid = TRUE;
		distributed_predictions[0].time = tick;
		distributed_predictions[0].state.position.x = client_x;
		distributed_predictions[0].state.position.y = 0.0f;
		distributed_predictions[0].state.position.z = floor - rise * (real)round_trip;
		distributed_take_prediction(0, UNIT, &bound);
		CHECK(objects[UNIT].object.position.x == client_x, "tick %ld: across at %f, not the client's %f", tick,
			objects[UNIT].object.position.x, client_x);
		CHECK(objects[UNIT].object.position.z >= floor - 0.001f,
			"tick %ld: %f under the elevator's floor at %f, to fall through it", tick,
			objects[UNIT].object.position.z, floor);
	}
}

/* a player at 10 on the elevator (none: on the ground), moving at
velocity, and the client's word on its height, once: where the host has
it after */
static real taken_height(long elevator_index, real velocity, real client_z)
{
	struct distributed_on_foot_bound bound;

	memset(&bound, 0, sizeof(bound));
	bound.rise = 1000.0f;
	objects[UNIT].mask = _object_mask_biped;
	objects[UNIT].biped.elevator_object_index = elevator_index;
	objects[UNIT].biped.support_surface_index = 1;
	objects[UNIT].biped.elevator_ticks = 60;
	objects[UNIT].object.position.z = 10.0f;
	objects[ELEVATOR].mask = _object_mask_device;
	objects[ELEVATOR].device.position_velocity = velocity;
	game_time = 1;
	distributed_predictions[0].valid = TRUE;
	distributed_predictions[0].time = 1;
	distributed_predictions[0].state.position.z = client_z;
	distributed_take_prediction(0, UNIT, &bound);
	return objects[UNIT].object.position.z;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* going up, the client's word a round trip (200 ms) behind: the host's
	copy rides the host's elevator, and goes across as the client says */
	CASE("riding-up")
	{
		ride(ELEVATOR, 0.1f, 6);
		return 0;
	}
	/* An elevator at rest still protects its rider from a delayed height. */
	CASE("elevator-at-rest")
	{
		real height = taken_height(ELEVATOR, 0.0f, 9.8f);

		CHECK(height == 10.0f, "at rest, delayed height moved the rider below the floor: %f", height);
		return 0;
	}
	/* on foot on the ground, no elevator: the client's height taken */
	CASE("on-foot")
	{
		real height = taken_height(NONE, 0.0f, 10.4f);

		CHECK(height == 10.4f, "on foot, the client's height 10.4 not taken: %f", height);
		return 0;
	}
	CASE("jump-at-rest")
	{
		objects[UNIT].biped.flags = FLAG(_biped_airborne_bit);
		CHECK(taken_height(ELEVATOR, 0.0f, 10.4f) == 10.4f, "a jump was clamped to the stopped floor");
		return 0;
	}
	CASE("left-platform")
	{
		(void)taken_height(ELEVATOR, 0.0f, 10.0f);
		objects[UNIT].biped.elevator_ticks = 59;
		game_time = 2;
		distributed_predictions[0].time = 2;
		distributed_predictions[0].state.position.z = 9.8f;
		{
			struct distributed_on_foot_bound bound = {0};
			bound.rise = 1000.0f;
			distributed_take_prediction(0, UNIT, &bound);
		}
		CHECK(objects[UNIT].object.position.z == 9.8f, "remembered attachment overrode walking off");
		return 0;
	}
	if (!strcmp(case_name, "reversals-100ms") || !strcmp(case_name, "reversals-200ms"))
	{
		long delay = !strcmp(case_name, "reversals-100ms") ? 3 : 6;
		real floors[181];
		long tick;
		struct distributed_on_foot_bound bound = {0};
		bound.rise = 1000.0f;
		(void)taken_height(ELEVATOR, 0.0f, 10.0f);
		floors[0] = 10.0f;
		for (tick = 1; tick <= 180; tick++)
		{
			long phase = tick % 45;
			real step = phase < 15 ? 0.1f : phase < 20 ? 0.0f : phase < 35 ? -0.1f : 0.0f;
			floors[tick] = floors[tick - 1];
			elevator_tick(&floors[tick], step);
			objects[ELEVATOR].device.position_velocity = step;
			game_time = tick;
			distributed_predictions[0].time = tick;
			distributed_predictions[0].state.position.z = floors[MAX(0, tick - delay)];
			distributed_take_prediction(0, UNIT, &bound);
			CHECK(fabsf(objects[UNIT].object.position.z - floors[tick]) < 0.001f,
				"%ld ms: tick %ld rider %f, floor %f", delay * 1000 / 30, tick,
				objects[UNIT].object.position.z, floors[tick]);
		}
		return 0;
	}
	fprintf(stderr, "no case %s\n", case_name);
	return 2;
}
