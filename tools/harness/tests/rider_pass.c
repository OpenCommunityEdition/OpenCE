/*
RIDER_PASS.C (test)

objects.c's update loops over a fake object list: a biped in a vehicle's
window updates after the main loop, so it always comes after its vehicle;
the counts give each carry the vehicle's updates it spans.
test_rider_pass.py takes the code (under_test.inc) and the enums
(enums.inc) from the sources.
*/

#include "harness.h"
#include "enums.inc"

#define DATUM_INDEX_NEW(i, s) ((long)(((unsigned long)(unsigned short)(s) << 16) | (unsigned short)(i)))
#define SHORT_BITS 16

struct object_header_datum
{
	short identifier;
	byte flags;
	byte type;
};

struct object_datum
{
	struct { long parent_object_index, next_object_index; } object;
	struct { long player_index; } unit;
};

enum { OBJECTS = MAXIMUM_OBJECTS_PER_MAP };
static struct data_array *object_header_data;
static struct object_datum objects[OBJECTS];
static long *object_update_counts;
/* the riding switch, and how often the pass reads it */
static boolean riding_enabled = TRUE;
static long enabled_reads;
static boolean vehicle_riding_enabled(void)
{
	enabled_reads++;
	return riding_enabled;
}
#define object_get(index) (&objects[DATUM_INDEX_TO_ABSOLUTE_INDEX(index)])
#define unit_get(index) object_get(index)

static struct object_header_datum *object_header_try_and_get(long index)
{
	struct object_header_datum *header = datum_get(object_header_data, index);

	return header->identifier && header->identifier == (short)(index >> 16) ? header : NULL;
}

#include "under_test.inc"

/* the world: slot 0 a player's biped, slot 1 its vehicle, slot 2 an AI */
static long rider, vehicle, actor;
static long order[MAXIMUM_OBJECTS_PER_MAP];
static int order_count;
static boolean rider_deferred, actor_deferred;
static boolean all_deferred;
static boolean rider_dies, deactivate_rider, attach_rider;

static long object_new(byte type, long player_index)
{
	long index = datum_new(object_header_data);
	struct object_header_datum *header = datum_get(object_header_data, index);

	header->flags = FLAG(_object_header_active_bit);
	header->type = type;
	object_get(index)->object.parent_object_index = NONE;
	object_get(index)->unit.player_index = player_index;
	return index;
}

static long deferred_calls;

static boolean deferred(long object_index)
{
	deferred_calls++;
	return all_deferred || (object_index == rider ? rider_deferred : object_index == actor ? actor_deferred : FALSE);
}

static boolean update(long object_index)
{
	order[order_count++] = object_index;
	object_count_update(object_index);
	if (object_index == vehicle && rider_dies)
		rider_deferred = FALSE;
	if (object_index == vehicle && deactivate_rider)
		((struct object_header_datum *)datum_get(object_header_data, rider))->flags = 0;
	if (object_index == vehicle && attach_rider)
		object_get(rider)->object.parent_object_index = vehicle;
	return TRUE;
}

static void world(void)
{
	object_header_data = game_state_data_new("object", OBJECTS, sizeof(struct object_header_datum));
	/* (one slot past the end, to see a count written there) */
	object_update_counts = calloc(MAXIMUM_OBJECTS_PER_MAP + 1, sizeof(long));
	rider = object_new(_object_type_biped, 0);
	vehicle = object_new(_object_type_vehicle, NONE);
	actor = object_new(_object_type_biped, NONE);
	order_count = 0;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	world();
	CASE("capacity")
	{
		long index;

		/* one more deferred object than the list holds: the one that does not fit updates in the main loop, ahead of the list */
		for (index = 3; index <= HALO_PORT_MAXIMUM_NETWORK_PLAYERS; index++)
			object_new(_object_type_biped, NONE);
		all_deferred = TRUE;
		objects_update_passes(FALSE, deferred, update);
		CHECK(order_count == HALO_PORT_MAXIMUM_NETWORK_PLAYERS + 1, "not every deferred object updated past capacity");
		CHECK(DATUM_INDEX_TO_ABSOLUTE_INDEX(order[0]) == HALO_PORT_MAXIMUM_NETWORK_PLAYERS,
			"the overflow rider did not update in the main loop");
		for (index = 0; index < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; index++)
			CHECK(DATUM_INDEX_TO_ABSOLUTE_INDEX(order[index + 1]) == index, "list order changed at %ld", index);
		for (index = 0; index <= HALO_PORT_MAXIMUM_NETWORK_PLAYERS; index++)
			CHECK(object_update_count(order[index]) == 1, "an object was not updated exactly once");
		return 0;
	}
	CASE("after-vehicle")
	{
		long before;
		short tick;

		/* riding, the tick it leaves, the tick it lands back: deferred in each */
		rider_deferred = TRUE;
		for (tick = 0; tick < 3; tick++)
		{
			before = object_update_count(vehicle);
			order_count = 0;
			objects_update_passes(FALSE, deferred, update);
			CHECK(order_count == 3 && order[2] == rider, "the rider not after its vehicle");
			CHECK(object_update_count(vehicle) - before == 1, "n = %ld", object_update_count(vehicle) - before);
		}
		/* not deferred: datum order */
		rider_deferred = FALSE;
		order_count = 0;
		objects_update_passes(FALSE, deferred, update);
		CHECK(order[0] == rider && order[1] == vehicle, "not in datum order");
		return 0;
	}
	CASE("off-no-deferred-calls")
	{
		/* riding off: nothing is a rider, so the callback is never asked and every object updates in datum order */
		riding_enabled = FALSE;
		rider_deferred = TRUE;
		actor_deferred = TRUE;
		objects_update_passes(FALSE, deferred, update);
		CHECK(deferred_calls == 0, "deferred asked %ld times", deferred_calls);
		CHECK(enabled_reads == 1, "the switch read %ld times, not once", enabled_reads);
		CHECK(order_count == 3 && order[0] == rider && order[1] == vehicle && order[2] == actor, "not updated in datum order");
		return 0;
	}
	CASE("dies-between-loops")
	{
		rider_deferred = TRUE;
		rider_dies = TRUE;
		objects_update_passes(FALSE, deferred, update);
		CHECK(order_count == 3 && order[2] == rider, "a rider that died in between not updated");
		return 0;
	}
	CASE("deactivated-between-loops")
	{
		rider_deferred = TRUE;
		deactivate_rider = TRUE;
		objects_update_passes(FALSE, deferred, update);
		CHECK(order_count == 2, "a deactivated rider updated");
		return 0;
	}
	CASE("parented-between-loops")
	{
		/* entered a seat during the vehicle's update: a child, updated by its parent */
		rider_deferred = TRUE;
		attach_rider = TRUE;
		objects_update_passes(FALSE, deferred, update);
		CHECK(order_count == 2, "a rider that became a child updated alone");
		return 0;
	}
	CASE("double-speed")
	{
		/* an odd tick: the player updates, the vehicle and the AI not; a deferred
		biped without a player not either */
		rider_deferred = TRUE;
		actor_deferred = TRUE;
		objects_update_passes(TRUE, deferred, update);
		CHECK(order_count == 1 && order[0] == rider, "%d updated", order_count);
		return 0;
	}
	CASE("counts")
	{
		long *counts = object_update_counts;

		object_count_update(DATUM_INDEX_NEW(MAXIMUM_OBJECTS_PER_MAP, 1));
		CHECK(counts[MAXIMUM_OBJECTS_PER_MAP] == 0, "counted past the end");
		object_update_counts = NULL;
		object_count_update(vehicle);
		CHECK(object_update_count(vehicle) == 0, "a count without counts");
		object_update_counts = counts;
		object_count_update(vehicle);
		CHECK(object_update_count(vehicle) == 1, "not counted");
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
