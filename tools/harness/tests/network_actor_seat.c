/* idle seated actors' actual publisher and serializer, with transport captured */
#include "harness.h"
#include <math.h>
typedef unsigned short word;
typedef struct { real i, j; } real_vector2d;
struct distributed_message_header { byte header[8]; };
#define csmemset memset
#define PIN(value, low, high) ((value) < (low) ? (low) : (value) > (high) ? (high) : (value))
#define SET_FLAG(flags, bit, value) ((flags) = (value) ? (flags) | FLAG(bit) : (flags) & ~FLAG(bit))
#define NO_IMPULSE 255
#define NO_TEAM 255
#define ONE_SHOT_CONTROL_FLAGS 0
#define NUMBER_OF_UNIT_CONTROL_FLAGS 16
#define DISTRIBUTED_UNIT_SCALE 32767.f
#define DISTRIBUTED_VELOCITY_SCALE 1024.f
#define _unit_running_blindly_bit 0
#define _unit_active_camouflaged_bit 1
#define _unit_super_camouflaged_bit 2
#define _object_dead_bit 0
#define _object_mask_unit 1
#define _distributed_message_actor_states 1
#define DATAGRAM_ENTRIES(type) 8
#include "actor_seat_types.inc"
struct unit_datum
{
	struct
	{
		long parent_object_index;
		unsigned long damage_flags;
		short owner_team_index;
		real_point3d position;
		real_vector3d translational_velocity, forward, up;
	} object;
	struct
	{
		long player_index;
		short parent_seat_index;
		unsigned long flags, control_flags;
		real active_camouflage, primary_trigger, run_blindly_angle;
		short persistent_control_timer;
	} unit;
};
struct damage_network_state
{
	boolean shield_depleted, shield_charging, shield_over_charging;
	real body_vitality, shield_vitality;
};
static struct host_actor host_actors[MAXIMUM_NETWORK_ACTORS];
static short host_actor_count;
static struct unit_datum units[MAXIMUM_NETWORK_ACTORS];
static boolean present[MAXIMUM_NETWORK_ACTORS];
static long now;
static short machine_count = 1;
static short period = 1;
static long sent[HALO_PORT_MAXIMUM_NETWORK_MACHINES];
static struct distributed_actor_state last_state;
static boolean check_full_seats;
static boolean riding_enabled;
static boolean vehicle_riding_enabled(void) { return riding_enabled; }
static struct unit_datum *unit_get(long index)
{
	long slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(index);
	CHECK(slot < MAXIMUM_NETWORK_ACTORS && present[slot], "accessed an absent unit");
	return &units[slot];
}
static void *object_try_and_get_and_verify_type(long index, long mask)
{
	long slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(index);
	return slot < MAXIMUM_NETWORK_ACTORS && present[slot] ? &units[slot] : NULL;
}
static void damage_get_network_state(long index, struct damage_network_state *state) { memset(state, 0, sizeof(*state)); }
static word distributed_vitality_pack(real value) { return 0; }
static short distributed_angle_pack(real value) { return 0; }
static void angles_pack(real_vector3d const *vector, short *angles) { angles[0] = angles[1] = 0; }
static void distributed_vector_pack(real_vector3d const *vector, real scale, struct distributed_vector *result) { memset(result, 0, sizeof(*result)); }
static short distributed_client_machines(long *indices, short maximum)
{
	short index;
	CHECK(machine_count <= maximum, "client list exceeds publisher capacity");
	for (index = 0; index < machine_count; index++) indices[index] = index;
	return machine_count;
}
static long game_time_get(void) { return now; }
static void host_send_damage(void) { }
static short network_objects_send_period(long machine, real_point3d const *position) { return period; }
static void distributed_send_to_machine(long machine, void const *data, short type, short count, word size)
{
	struct distributed_actor_state_message const *message = data;
	short index;
	CHECK(count > 0 && count <= 8 && type == _distributed_message_actor_states, "invalid actor-state batch");
	CHECK(size == sizeof(message->header) + count * sizeof(message->states[0]), "wrong wire size");
	for (index = 0; index < count; index++)
	{
		last_state = message->states[index];
		if (check_full_seats)
		{
			CHECK(TEST_FLAG(last_state.flags, _distributed_actor_rides_bit) &&
				last_state.vehicle_index == 5000 && last_state.seat_index == 2,
				"full-capacity publisher lost a seat");
		}
	}
	sent[machine] += count;
}
#include "actor_seat_under_test.inc"
static void seed(short count)
{
	short index;
	memset(host_actors, 0, sizeof(host_actors));
	memset(units, 0, sizeof(units));
	memset(present, 0, sizeof(present));
	memset(sent, 0, sizeof(sent));
	host_actor_count = count;
	for (index = 0; index < count; index++)
	{
		host_actors[index].unit_index = 0x10000 + index;
		host_actors[index].impulse = NONE;
		host_actors[index].speech_sound = NONE;
		host_actors[index].user_animation_graph = NONE;
		present[index] = TRUE;
		units[index].unit.player_index = NONE;
		units[index].unit.parent_seat_index = 2;
		units[index].object.parent_object_index = 5000;
	}
}
int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	short index;
	CASE("actor-seat-idle-riding-off")
	{
		seed(1);
		riding_enabled = FALSE;
		host_actors[0].noted = TRUE;
		network_actors_host_tick();
		CHECK(sent[0] == 1 && host_actor_count == 1, "riding off dropped fresh controls");
		CHECK(!host_actors[0].noted, "test accidentally supplies fresh controls");
		network_actors_host_tick();
		CHECK(sent[0] == 1 && host_actor_count == 0, "riding off retained an idle seated actor");
		seed(1);
		units[0].unit.flags = FLAG(_unit_running_blindly_bit);
		network_actors_host_tick();
		CHECK(sent[0] == 1 && host_actor_count == 1, "riding off dropped a fleeing actor");
		return 0;
	}
	CASE("actor-seat-idle-published")
	{
		riding_enabled = TRUE;
		seed(1);
		host_actors[0].noted = TRUE;
		network_actors_host_tick();
		CHECK(sent[0] == 1, "initial control did not publish");
		for (now = 1; now <= 5; now++)
		{
			CHECK(!host_actors[0].noted, "test accidentally supplies fresh controls");
			network_actors_host_tick();
			CHECK(sent[0] == now + 1 && host_actor_count == 1,
				"seated actor without fresh control stopped publishing its seat");
			CHECK(last_state.unit_index == 0x10000 && last_state.vehicle_index == 5000 &&
				last_state.seat_index == 2 && TEST_FLAG(last_state.flags, _distributed_actor_rides_bit),
				"idle actor's wire entry lost its salted unit, vehicle, seat or riding flag");
		}
		/* (retention must survive a tick on which distance scheduling skips the send) */
		period = 4;
		now = 6;
		network_actors_host_tick();
		CHECK(sent[0] == 6 && host_actor_count == 1, "idle actor bypassed scheduling or was dropped");
		now = 8;
		network_actors_host_tick();
		CHECK(sent[0] == 7, "idle actor did not publish on its next scheduled tick");
		/* (the old non-seated, invalid and dead-unit filters still apply) */
		period = 1;
		seed(5);
		units[0].unit.parent_seat_index = NONE;
		units[1].object.parent_object_index = NONE;
		units[2].object.damage_flags = FLAG(_object_dead_bit);
		present[3] = FALSE;
		units[4].unit.player_index = 0;
		network_actors_host_tick();
		CHECK(host_actor_count == 0 && sent[0] == 0, "idle retention kept invalid or unseated actors");
		seed(1);
		units[0].object.parent_object_index = NONE;
		units[0].unit.parent_seat_index = NONE;
		units[0].unit.flags = FLAG(_unit_running_blindly_bit);
		network_actors_host_tick();
		CHECK(host_actor_count == 1 && sent[0] == 1, "running-blindly retention changed");
		/* (the largest tables still publish all seats, with no extra per-client scan) */
		seed(MAXIMUM_NETWORK_ACTORS);
		machine_count = HALO_PORT_MAXIMUM_NETWORK_MACHINES;
		check_full_seats = TRUE;
		network_actors_host_tick();
		CHECK(host_actor_count == MAXIMUM_NETWORK_ACTORS, "full actor table was pruned");
		for (index = 0; index < machine_count; index++)
			CHECK(sent[index] == MAXIMUM_NETWORK_ACTORS, "full-capacity client lost actor states");
		return 0;
	}
	return 2;
}
