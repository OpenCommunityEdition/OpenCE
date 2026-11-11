/* a vehicle state from the host, taken by the real network_objects_handle_states against fakes */
#include "harness.h"
#include <math.h>

typedef unsigned short word;
typedef struct { real i, j; } real_vector2d;
#include "types.inc"

#define SET_FLAG(v,b,s) ((v)=(s)?((v)|FLAG(b)):((v)&~FLAG(b)))
#define _network_vehicle_write_carry 1
#define _network_vehicle_write_state 0
#define VEHICLE 0x12340007L
#define DRIVER 0x12350008L
#define game_players_are_double_speed() FALSE
enum { _object_at_rest_bit, _object_connected_to_map_bit };
enum { _object_type_biped, _object_type_vehicle, _object_type_item };
enum { _unit_integrated_light_on_bit, _item_attached_to_unit_bit };
#define _object_mask_unit (FLAG(_object_type_biped) | FLAG(_object_type_vehicle))
#define _object_mask_item FLAG(_object_type_item)
#define _object_mask_vehicle FLAG(_object_type_vehicle)
#define _object_mask_all 0xFF
struct object_datum
{
	struct
	{
		short type;
		real_point3d position;
		real_vector3d forward, up, translational_velocity, angular_velocity;
		long parent_object_index;
		unsigned long flags;
	} object;
	struct
	{
		long driver_object_index;
		short player_index;
		unsigned long flags;
	} unit;
	struct { unsigned long flags; } item;
};
#define unit_datum object_datum
struct object_iterator { long index; int visited; };
static struct object_datum vehicle, driver;
static long tick;
static boolean enabled = TRUE, unsteered = FALSE;
static int sets, moves, writes, carries, corrections, own_corrections, notes;
static real_vector3d const zero = {0, 0, 0};
static real_vector3d const *global_zero_vector3d = &zero;
#include "config.inc"

static struct object_datum *object_get(long index) { CHECK(index == VEHICLE, "stale salt"); return &vehicle; }
static struct object_datum *unit_get(long index) { return index == DRIVER ? &driver : &vehicle; }
static struct object_datum *item_get(long index) { return &vehicle; }
static struct object_datum *object_try_and_get_and_verify_type(long index, unsigned long mask) { return &vehicle; }
static long game_time_get(void) { return tick; }
boolean vehicle_riding_enabled(void) { return enabled; }
static boolean distributed_vehicle_unsteered(long index) { return unsteered; }
static boolean distributed_object_follows_host(long index) { return TRUE; }
static boolean distributed_object_index_valid(long index) { return TRUE; }
static boolean network_objects_client_has(long index) { return TRUE; }
static boolean distributed_unit_driven_here(long index) { return FALSE; }
static boolean distributed_player_is_local(short player) { return player == 0; }
static boolean distributed_client_correct_own_vehicle(long vehicle_index, short player, void const *state)
{
	own_corrections++;
	return FALSE;
}
/* (upstream's path of an unsteered vehicle, tested apart in unsteered_path.c: here only that its word is its whole take) */
static void distributed_client_note_unsteered_path(long index, real_point3d const *p, real_vector3d const *f,
	real_vector3d const *u, long host_time, boolean at_rest)
{
	notes++;
}
static void distributed_body_set_at_rest(long index, boolean at_rest) { }
static void distributed_client_dead_biped(long index) { }
static void distributed_client_dead_vehicle(long index) { }
static void distributed_count_correction(void) { corrections++; }
static void object_reconnect_to_map(long index, void *location) { }
static void object_set_visibility(long index, boolean visible) { }
static void distributed_body_unfix_pose(long index) { }
static void render_interpolation_correct_object(long index, real_vector3d const *offset) { }
static void object_set_position(long index, real_point3d const *p, real_vector3d const *f, real_vector3d const *u)
{
	struct object_datum *object = object_get(index);
	sets++;
	object->object.position = *p;
	object->object.forward = *f;
	object->object.up = *u;
}
static void network_objects_vehicle_writing(long index, short kind) { }
static void network_objects_vehicle_written(long index, real_point3d const *p,
	real_vector3d const *f, real_vector3d const *u, short kind)
{
	if (kind == _network_vehicle_write_carry) carries++; else writes++;
}
static boolean distributed_transform_valid(real_point3d const *p, real_vector3d const *f, real_vector3d const *u,
	real_vector3d const *v, real_vector3d const *a, real_vector3d *vf, real_vector3d *vu)
{
	*vf = *f;
	*vu = *u;
	return TRUE;
}
static void distributed_object_move(long index, real_point3d const *p, real_vector3d const *f,
	real_vector3d const *u, real_vector3d const *v, real_vector3d const *a, short kind)
{
	struct object_datum *object = object_get(index);
	moves++;
	object->object.position = *p;
	object->object.forward = *f;
	object->object.up = *u;
	if (v) object->object.translational_velocity = *v;
	if (a) object->object.angular_velocity = *a;
}
static void distributed_vector_unpack(struct distributed_vector const *p, real scale, real_vector3d *v)
{
	v->i = p->i / scale; v->j = p->j / scale; v->k = p->k / scale;
}
static void object_iterator_new(struct object_iterator *it, unsigned long mask, long flags) { it->visited = 0; it->index = VEHICLE; }
static struct object_datum *object_iterator_next(struct object_iterator *it) { return it->visited++ ? NULL : &vehicle; }
static void matrix4x3_rotation_from_axis_and_angle(real_matrix4x3 *m, real_vector3d const *v, real s, real c) { CHECK(FALSE, "unexpected rotation"); }
static void matrix4x3_transform_vector(real_matrix4x3 const *m, real_vector3d const *v, real_vector3d *out) { CHECK(FALSE, "unexpected rotation"); }
/* (the co-op restore recovery of a dead client copy: not under test) */
static void network_objects_expect_live_unit(long index) { }
#include "under_test.inc"

static short pack(real value, real scale) { return (short)floorf(value * scale + 0.5f); }
static void pack_vector(real i, real j, real k, real scale, struct distributed_vector *out)
{
	out->i = pack(i, scale); out->j = pack(j, scale); out->k = pack(k, scale);
}
static void near(real got, real want, real limit, char const *what)
{
	CHECK(fabsf(got - want) <= limit, "%s: got %.9f want %.9f", what, got, want);
}
static void vector_is(real_vector3d const *got, real i, real j, real k, real limit, char const *what)
{
	near(got->i, i, limit, what); near(got->j, j, limit, what); near(got->k, k, limit, what);
}
static void state_make(struct distributed_object_state *state, real_point3d const *position,
	real_vector3d const *v, real_vector3d const *w, boolean rest)
{
	memset(state, 0, sizeof(*state));
	state->object_index = VEHICLE;
	state->position = *position;
	if (rest) SET_FLAG(state->flags, _distributed_object_at_rest_bit, TRUE);
	pack_vector(1.0f, 0.0f, 0.0f, DISTRIBUTED_UNIT_SCALE, &state->forward);
	pack_vector(0.0f, 0.0f, 1.0f, DISTRIBUTED_UNIT_SCALE, &state->up);
	pack_vector(v->i, v->j, v->k, DISTRIBUTED_VELOCITY_SCALE, &state->translational_velocity);
	pack_vector(w->i, w->j, w->k, DISTRIBUTED_ANGULAR_VELOCITY_SCALE, &state->angular_velocity);
}
static real_vector3d wire_velocity(struct distributed_object_state const *state)
{
	real_vector3d v;
	distributed_vector_unpack(&state->translational_velocity, DISTRIBUTED_VELOCITY_SCALE, &v);
	return v;
}
static real_vector3d wire_angular(struct distributed_object_state const *state)
{
	real_vector3d w;
	distributed_vector_unpack(&state->angular_velocity, DISTRIBUTED_ANGULAR_VELOCITY_SCALE, &w);
	return w;
}
static void setup(real_point3d const *position, real_vector3d const *v, real_vector3d const *w)
{
	memset(&vehicle, 0, sizeof(vehicle)); memset(&driver, 0, sizeof(driver));
	enabled = TRUE; unsteered = FALSE; tick = 100;
	sets = moves = writes = carries = corrections = own_corrections = notes = 0;
	vehicle.object.type = _object_type_vehicle;
	vehicle.object.flags = FLAG(_object_connected_to_map_bit);
	vehicle.object.parent_object_index = NONE;
	vehicle.unit.driver_object_index = NONE;
	vehicle.object.position = *position;
	vehicle.object.forward.i = 1.0f; vehicle.object.up.k = 1.0f;
	vehicle.object.translational_velocity = *v;
	vehicle.object.angular_velocity = *w;
}
static void receive(struct distributed_object_state const *state)
{
	network_objects_handle_states(state, 1, tick);
}
static void unchanged(struct object_datum const *before, char const *what)
{
	CHECK(!memcmp(&before->object.position, &vehicle.object.position, sizeof(real_point3d)) &&
		!memcmp(&before->object.forward, &vehicle.object.forward, sizeof(real_vector3d)) &&
		!memcmp(&before->object.up, &vehicle.object.up, sizeof(real_vector3d)), "%s: the pose changed", what);
	CHECK(sets == 0 && moves == 0 && writes == 0, "%s: reconcile wrote (%d, %d, %d)", what, sets, moves, writes);
}
static void velocity(real i, real j, real k)
{
	vector_is(&vehicle.object.translational_velocity, i, j, k, 0.00002f, "velocity");
}
int main(int argc, char **argv)
{
	char const *case_name = argv[1];
	real_point3d client_origin = {35.2589f, -90.8963f, 0.5708f};
	real_vector3d client_v = {0.0266f, 0.0221f, -0.0049f}, client_w = {0.0007f, 0.0014f, 0.0081f};
	real_vector3d host_v = {0.0286f, 0.0250f, -0.0029f}, host_w = {0.0005f, 0.0003f, 0.0093f};
	struct distributed_object_state state;
	struct object_datum before;
	real_point3d near_state = {35.2689f, -90.8963f, 0.5708f};
	real_vector3d wire;

	CASE("vehicle-state-close-refreshes-velocity")
	{
		setup(&client_origin, &client_v, &client_w);
		state_make(&state, &near_state, &host_v, &host_w, FALSE);
		before = vehicle;
		receive(&state);
		wire = wire_velocity(&state);
		vector_is(&vehicle.object.translational_velocity, wire.i, wire.j, wire.k, 0.0f, "refreshed velocity");
		wire = wire_angular(&state);
		vector_is(&vehicle.object.angular_velocity, wire.i, wire.j, wire.k, 0.0f, "refreshed angular velocity");
		unchanged(&before, "close vehicle state");
		return 0;
	}
	CASE("vehicle-state-close-riding-off-original")
	{
		setup(&client_origin, &client_v, &client_w);
		enabled = FALSE;
		state_make(&state, &near_state, &host_v, &host_w, FALSE);
		before = vehicle;
		receive(&state);
		CHECK(!memcmp(&before.object.translational_velocity, &vehicle.object.translational_velocity, sizeof(real_vector3d)) &&
			!memcmp(&before.object.angular_velocity, &vehicle.object.angular_velocity, sizeof(real_vector3d)),
			"switch off: the velocities changed");
		unchanged(&before, "switch off");
		return 0;
	}
	CASE("vehicle-state-close-own-vehicle-unchanged")
	{
		setup(&client_origin, &client_v, &client_w);
		vehicle.unit.driver_object_index = DRIVER;
		driver.unit.player_index = 0;
		state_make(&state, &near_state, &host_v, &host_w, FALSE);
		before = vehicle;
		receive(&state);
		CHECK(own_corrections == 1, "the own vehicle did not take its own path");
		CHECK(!memcmp(&before.object.translational_velocity, &vehicle.object.translational_velocity, sizeof(real_vector3d)) &&
			!memcmp(&before.object.angular_velocity, &vehicle.object.angular_velocity, sizeof(real_vector3d)),
			"own vehicle: the velocities changed");
		unchanged(&before, "own vehicle");
		return 0;
	}
	CASE("vehicle-state-far-unchanged")
	{
		long far;
		for (far = 0; far < 2; far++)
		{
			real_point3d where = client_origin;

			setup(&client_origin, &client_v, &client_w);
			where.x += far ? 5.0f : 0.5f;
			state_make(&state, &where, &host_v, &host_w, FALSE);
			receive(&state);
			CHECK(sets + moves == 1, "far %ld: expected one write, got sets %d moves %d", far, sets, moves);
			CHECK((far == 0 && sets == 1 && moves == 0) || (far == 1 && moves == 1 && sets == 0),
				"far %ld: the wrong path (sets %d moves %d)", far, sets, moves);
			wire = wire_velocity(&state);
			vector_is(&vehicle.object.translational_velocity, wire.i, wire.j, wire.k, 0.0f, "far velocity");
			wire = wire_angular(&state);
			vector_is(&vehicle.object.angular_velocity, wire.i, wire.j, wire.k, 0.0f, "far angular velocity");
			near(vehicle.object.position.x, where.x, 1e-4f, "far position");
		}
		return 0;
	}
	CASE("vehicle-state-unsteered-path-unchanged")
	{
		setup(&client_origin, &client_v, &client_w);
		unsteered = TRUE;
		state_make(&state, &near_state, &host_v, &host_w, FALSE);
		before = vehicle;
		receive(&state);
		CHECK(notes == 1, "the unsteered vehicle's word did not go to its path: %d", notes);
		CHECK(!memcmp(&before.object.translational_velocity, &vehicle.object.translational_velocity, sizeof(real_vector3d)) &&
			!memcmp(&before.object.angular_velocity, &vehicle.object.angular_velocity, sizeof(real_vector3d)),
			"unsteered: the velocities were refreshed from the wire");
		unchanged(&before, "unsteered");
		return 0;
	}
	CASE("gun-takeoff-frame-refreshed")
	{
		struct vehicle_riding_vehicle client_vehicle, host_vehicle;
		real_point3d client_base = {35.87329865f, -90.43139648f, 0.91992658f};
		real_point3d host_base = {35.91934967f, -90.37454224f, 0.92383105f};
		real_point3d host_origin = {35.2751f, -90.8755f, 0.5821f};
		real_vector3d client_frame, host_frame;
		real_vector3d stale_v = {0.0279f, 0.0232f, -0.0041f}, stale_w = {0.0006f, 0.0009f, 0.0086f};

		setup(&client_origin, &stale_v, &stale_w);
		state_make(&state, &host_origin, &host_v, &host_w, FALSE);
		receive(&state);
		memset(&client_vehicle, 0, sizeof(client_vehicle)); memset(&host_vehicle, 0, sizeof(host_vehicle));
		client_vehicle.origin = vehicle.object.position;
		client_vehicle.translational_velocity = vehicle.object.translational_velocity;
		client_vehicle.angular_velocity = vehicle.object.angular_velocity;
		host_vehicle.origin = host_origin;
		host_vehicle.translational_velocity = host_v;
		host_vehicle.angular_velocity = host_w;
		vehicle_riding_deck_point_velocity(&client_vehicle, &client_base, FALSE, &client_frame);
		vehicle_riding_deck_point_velocity(&host_vehicle, &host_base, FALSE, &host_frame);
		vector_is(&client_frame, host_frame.i, host_frame.j, host_frame.k, 1.0f / 1024.0f, "takeoff frame");
		return 0;
	}
	fprintf(stderr, "no case %s\n", case_name); return 2;
}
