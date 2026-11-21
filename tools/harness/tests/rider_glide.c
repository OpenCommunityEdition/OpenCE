/* A vehicle a netcode write moves, drawn gliding from where it was
(render_interpolation.c), and a player riding it, whom the write carries
and who is no child of it (vehicle_riding_bipeds.c): the vehicle's
correction goes to its riders too. One node each, no turn. */
#define HALO_ANDROID
#include "harness.h"
#include <math.h>

#include "config.inc"

#define MAXIMUM_LOCAL_PLAYERS 4
#define MAXIMUM_OBJECTS_PER_MAP 8
#define HALO_PORT_MAXIMUM_OBJECTS_PER_MAP 8
#define HALO_PORT_MAXIMUM_NETWORK_PLAYERS 2
#define _object_mask_all 0
#define _object_mask_vehicle 0
enum { _object_type_biped, _object_type_vehicle, _object_type_weapon };
#define _object_dead_bit 3

#define VEHICLE 0x10000L
#define RIDER 0x10001L
#define OTHER_VEHICLE 0x10002L
#define OTHER_RIDER 0x10003L
#define WEAPON 0x10004L
#define OBJECTS 5

struct object_data
{
	short type;
	long first_child_object_index;
	long next_object_index;
	long parent_object_index;
	long damage_flags;
	struct { short size; } node_matrices;
};
struct object_datum { struct object_data object; };
struct biped_datum
{
	struct object_data object;
	struct { long player_index; } unit;
	struct { long riding_object_index; char riding_ticks; } biped;
};
struct object_iterator { long index; long next; };
struct data_iterator { long datum_index; long next; };
struct player_datum { long unit_index; };
struct observer_result
{
	real_point3d position;
	real_vector3d forward, up;
	real field_of_view;
};

static struct biped_datum bipeds[OBJECTS];
static struct player_datum players[2];
static void *player_data;
static real_matrix4x3 nodes[OBJECTS];
static real_vector3d zero_vector;
#define global_zero_vector3d (&zero_vector)

static void object_iterator_new(struct object_iterator *iterator, long mask, long flags) { iterator->next = 0; }
static void *object_iterator_next(struct object_iterator *iterator)
{
	if (iterator->next >= OBJECTS)
		return NULL;
	iterator->index = 0x10000L + iterator->next;
	return &bipeds[iterator->next++];
}
static struct object_datum *object_get(long index) { return (struct object_datum *)&bipeds[index & 0xFFFF]; }
static real_matrix4x3 *object_get_node_matrices(long index) { return &nodes[index & 0xFFFF]; }
static boolean interpolation_enabled = TRUE;
static int halo_interpolation_enabled(void) { return interpolation_enabled; }
static long player_control_get_unit_index(short local_player_index) { return local_player_index == 0 ? RIDER : NONE; }
static short unit_get_local_player_index(long index) { return index == RIDER ? 0 : NONE; }

#define MAXIMUM_TRACKED_PLAYERS HALO_PORT_MAXIMUM_NETWORK_PLAYERS
enum { _game_connection_network_server, _game_connection_network_client };
static short connection = _game_connection_network_server;
static short game_connection(void) { return connection; }
static struct { boolean in_tick; } distributed_machines;
static boolean distributed_rider_pass[MAXIMUM_TRACKED_PLAYERS];
static struct { boolean valid; struct { long unit_index; } state; } distributed_predictions[MAXIMUM_TRACKED_PLAYERS];
static struct player_datum *distributed_player(short index) { return &players[index]; }
static long distributed_living_unit(struct player_datum const *player) { return player->unit_index; }
static struct biped_datum *unit_get(long index) { return &bipeds[index & 0xFFFF]; }
static boolean network_coop_player_has_structure_bsp(long index) { return TRUE; }
static boolean mcc_script_effects_host_movement(void) { return FALSE; }
boolean distributed_rider_prediction_pending(short player_index, long unit_index);
static long game_time_get(void) { return 100; }
static boolean server_mode;
static int halo_server_mode(void) { return server_mode; }
static struct object_datum *object_try_and_get(long index) { return object_get(index); }
static long object_get_ultimate_parent(long index)
{
	while (object_get(index)->object.parent_object_index != NONE)
		index = object_get(index)->object.parent_object_index;
	return index;
}
enum { _director_perspective_first_person, _director_perspective_scripted };
static short director_get_perspective(short local_player_index) { return _director_perspective_first_person; }
static long scripted_camera_object_relative_to(void) { return NONE; }
static boolean scripted_camera_object_is_first_person_camera(long index) { return FALSE; }
static void *object_try_and_get_and_verify_type(long index, long mask) { return object_get(index); }
static void *object_header_block_get(long index, void const *block) { return object_get_node_matrices(index); }

static void data_iterator_new(struct data_iterator *iterator, void *data) { iterator->next = 0; }
static void *data_iterator_next(struct data_iterator *iterator)
{
	if (iterator->next >= 2)
		return NULL;
	iterator->datum_index = 0x10000L + iterator->next;
	return &players[iterator->next++];
}
static struct biped_datum *biped_get(long index) { return &bipeds[index & 0xFFFF]; }
static struct biped_datum *biped_try_and_get(long index) { return biped_get(index); }
static short biped_riding_player(struct biped_datum const *biped) { return (short)(biped->unit.player_index & 0xFFFF); }

void biped_riding_correct_riders(long vehicle_index, real_vector3d const *offset);
boolean biped_riding_uncarried_write(long biped_index, real_vector3d *offset, real *dyaw);

static struct vehicle_riding_correction corrections[OBJECTS];
static struct vehicle_riding_mark marks[2];
static boolean look_carry = TRUE;
boolean vehicle_riding_enabled(void) { return TRUE; }
static boolean in_cinematic = FALSE;
struct vehicle_riding_correction const *vehicle_riding_vehicle_correction(long index) { return &corrections[index & 0xFFFF]; }
struct vehicle_riding_mark *vehicle_riding_player_mark(short player) { return &marks[player]; }
boolean vehicle_riding_look_carries(void) { return look_carry; }
static void biped_get_physics_pill(long index, real_point3d *base, real *height, real *width)
{
	*base = nodes[index & 0xFFFF].position;
}
static real game_time_get_tick_fraction(void) { return 1.0f; }
static boolean cinematic_in_progress(void) { return in_cinematic; }

#include "under_test.inc"

static void place(long index, short type, real x, real z)
{
	struct object_datum *object = object_get(index);

	object->object.type = type;
	object->object.first_child_object_index = NONE;
	object->object.next_object_index = NONE;
	object->object.parent_object_index = NONE;
	object->object.node_matrices.size = sizeof(real_matrix4x3);
	nodes[index & 0xFFFF].scale = 1.0f;
	nodes[index & 0xFFFF].forward.i = 1.0f;
	nodes[index & 0xFFFF].left.j = 1.0f;
	nodes[index & 0xFFFF].up.k = 1.0f;
	set_real_point3d(&nodes[index & 0xFFFF].position, x, 0.0f, z);
}

static void rider(long index, short player, long vehicle_index)
{
	struct biped_datum *biped = biped_get(index);

	biped->unit.player_index = 0x10000L + player;
	biped->biped.riding_object_index = vehicle_index;
	biped->biped.riding_ticks = 1;
	players[player].unit_index = index;
}

static void move(long index, real dx, real dz)
{
	nodes[index & 0xFFFF].position.x += dx;
	nodes[index & 0xFFFF].position.z += dz;
}

/* the netcode's write of the vehicle, as distributed_object_move makes it */
static void write(long vehicle_index, real dz)
{
	real_vector3d offset = { 0.0f, 0.0f, -dz };

	move(vehicle_index, 0.0f, dz);
	render_interpolation_correct_object(vehicle_index, &offset);
	/* the rider's carry, which the write's takes as the vehicle's does */
	if (vehicle_index == VEHICLE)
		move(RIDER, 0.0f, dz);
}

/* the rider drawn a fraction into the tick, as high over the drawn deck as it stands */
static void expect_on_deck(real fraction)
{
	real_matrix4x3 const *vehicle;
	real_matrix4x3 const *on;

	interpolation_fraction = fraction;
	interpolation_frame++;
	vehicle = render_interpolation_object_node_matrices(VEHICLE);
	on = render_interpolation_object_node_matrices(RIDER);
	CHECK(vehicle && on, "not drawn between ticks");
	CHECK(fabs(on->position.x - vehicle->position.x) < 1e-4f &&
		fabs(on->position.z - vehicle->position.z - 1.0f) < 1e-4f,
		"drawn %f, %f from the deck at %.2f into the tick, not 0, 1",
		on->position.x - vehicle->position.x, on->position.z - vehicle->position.z, fraction);
}

/* a tick of the deck moving along x with its rider (a rider's carry comes
after its vehicle's move, as the game updates them) */
static void tick(void)
{
	move(VEHICLE, 1.0f, 0.0f);
	nodes[RIDER & 0xFFFF].position = nodes[VEHICLE & 0xFFFF].position;
	nodes[RIDER & 0xFFFF].position.z += 1.0f;
	render_interpolation_tick();
}

static void expect_eye_on_rider(real fraction)
{
	struct observer_result observer = {0};
	struct observer_result const *eye;
	real_matrix4x3 const *on;

	observer.position = nodes[RIDER & 0xFFFF].position;
	observer.position.z += 0.5f;
	observer.forward.i = 1.0f;
	observer.up.k = 1.0f;
	interpolation_fraction = fraction;
	interpolation_frame++;
	eye = render_interpolation_blended_camera(0, &observer);
	on = render_interpolation_object_node_matrices(RIDER);
	CHECK(on && fabs(eye->position.x - on->position.x) < 1e-4f &&
		fabs(eye->position.z - on->position.z - 0.5f) < 1e-4f,
		"eye %f, %f from rider at %.2f into tick %ld, not 0, 0.5",
		eye->position.x - on->position.x, eye->position.z - on->position.z, fraction, interpolation_tick);
}

/* whether a drawn pose is the object's simulated one moved by the offset */
static boolean moved_by(real_matrix4x3 const *drawn, long index, real_vector3d const *offset)
{
	real_point3d const *at = &nodes[index & 0xFFFF].position;

	return drawn && fabs(drawn->position.x - at->x - offset->i) < 1e-4f &&
		fabs(drawn->position.y - at->y - offset->j) < 1e-4f && fabs(drawn->position.z - at->z - offset->k) < 1e-4f;
}

static void next_frame(void)
{
	render_interpolation_frame_end();
	render_interpolation_frame_begin();
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	short index;

	interpolation_rendering = TRUE;
	place(VEHICLE, _object_type_vehicle, 0.0f, 0.0f);
	place(RIDER, _object_type_biped, 0.0f, 1.0f);
	place(OTHER_VEHICLE, _object_type_vehicle, 50.0f, 0.0f);
	place(OTHER_RIDER, _object_type_biped, 50.0f, 1.0f);
	rider(RIDER, 0, VEHICLE);
	rider(OTHER_RIDER, 1, OTHER_VEHICLE);
	render_interpolation_tick();
	tick();

	/* the write drops the deck and its rider: both glide, and the rider
	stands on the deck in every frame, through the carries after it */
	CASE("deck")
	{
		write(VEHICLE, -0.4f);
		expect_on_deck(0.5f);
		expect_on_deck(1.0f);
		for (index = 0; index < 4; index++)
		{
			tick();
			expect_on_deck(0.25f);
			expect_on_deck(0.75f);
		}
	}
	/* a player riding another vehicle glides by none of this one's writes */
	CASE("other vehicle")
	{
		struct interpolated_object const *other = &interpolated_objects[OTHER_RIDER & 0xFFFF];

		write(VEHICLE, -0.4f);
		CHECK(other->correction.k == 0.0f && other->correction_pending.k == 0.0f,
			"glided %f, %f by another vehicle's write", other->correction.k, other->correction_pending.k);
	}
	CASE("unmoved camera")
	{
		real_vector3d offset = { 0.0f, 0.0f, 0.4f };

		/* (the first snapshot has no previous camera to blend from) */
		expect_eye_on_rider(1.0f);
		CHECK(interpolated_cameras[0].valid, "camera not primed");
		tick();
		move(VEHICLE, 0.0f, -0.4f);
		render_interpolation_correct_object(VEHICLE, &offset);
		expect_eye_on_rider(0.25f);
		expect_eye_on_rider(0.75f);
		tick();
		expect_eye_on_rider(0.25f);
		expect_eye_on_rider(0.75f);
		interpolation_enabled = FALSE;
		render_interpolation_camera_unmoved(RIDER, &offset);
		CHECK(interpolated_cameras[0].snapshot_shift.k == 0.0f, "disabled camera kept a write");
		interpolation_enabled = TRUE;
	}
	CASE("remote take")
	{
		real_vector3d offset = { 0.0f, 0.0f, 0.4f };
		struct interpolated_object const *remote = &interpolated_objects[OTHER_RIDER & 0xFFFF];
		struct interpolated_object const *local = &interpolated_objects[RIDER & 0xFFFF];

		rider(OTHER_RIDER, 1, VEHICLE);
		distributed_machines.in_tick = TRUE;
		for (index = 0; index < 2; index++)
		{
			biped_get(players[index].unit_index)->biped.riding_ticks = RIDING_TICKS;
			distributed_rider_pass[index] = TRUE;
			distributed_predictions[index].valid = TRUE;
			distributed_predictions[index].state.unit_index = players[index].unit_index;
		}
		render_interpolation_correct_object(VEHICLE, &offset);
		CHECK(remote->correction_pending.k == 0.0f, "remote rider glided before its take: %f", remote->correction_pending.k);
		CHECK(fabs(local->correction_pending.k - 0.4f) < 1e-4f, "local rider lost its glide");
		for (index = 0; index < 5; index++)
		{
			distributed_predictions[1].valid = index != 0;
			distributed_rider_pass[1] = index != 1;
			distributed_machines.in_tick = index != 2;
			connection = index == 3 ? _game_connection_network_client : _game_connection_network_server;
			biped_riding_writes[1].carried = index == 4;
			biped_riding_writes[1].vehicle_index = VEHICLE;
			render_interpolation_correct_object(VEHICLE, &offset);
			CHECK(fabs(remote->correction_pending.k - 0.4f) < 1e-4f, "rider lost glide in control %d", index);
			interpolated_objects[OTHER_RIDER & 0xFFFF].correction_pending = zero_vector;
		}
	}
	/* with smooth motion off a frame is drawn between the netcode's write of
	the vehicle and the carry of a rider by it: the rider is drawn carried */
	CASE("smooth off")
	{
		real_point3d const root = { 0.0f, 0.0f, 0.0f };
		real_vector3d const forward = { 1.0f, 0.0f, 0.0f };
		real_vector3d const up = { 0.0f, 0.0f, 1.0f };
		real const theta = 0.1f;
		real_vector3d const t = { 0.5f, 0.0f, -0.2f };
		real_vector3d turned;
		real_point3d moved_root;
		real_point3d base = { 0.5f, 0.3f, 1.0f };
		real_vector3d expected;
		struct observer_result observer = {0};
		struct observer_result moved_observer;
		struct observer_result const *eye;
		real_matrix4x3 const *body;

		interpolation_enabled = FALSE;
		render_interpolation_frame_begin();
		biped_get(RIDER)->biped.riding_ticks = RIDING_TICKS;
		nodes[RIDER & 0xFFFF].position = base;
		vehicle_riding_correction_identity(&corrections[VEHICLE & 0xFFFF], VEHICLE);
		marks[0].role = _vehicle_riding_role_simulated;
		marks[0].correction = corrections[VEHICLE & 0xFFFF];

		turned.i = (real)cos(theta);
		turned.j = (real)sin(theta);
		turned.k = 0.0f;
		set_real_point3d(&moved_root, t.i, t.j, t.k);
		vehicle_riding_correction_compose(&corrections[VEHICLE & 0xFFFF], &root, &forward, &up, &moved_root, &turned, &up);
		nodes[VEHICLE & 0xFFFF].position = moved_root;
		nodes[VEHICLE & 0xFFFF].forward = turned;
		nodes[VEHICLE & 0xFFFF].left.i = -turned.j;
		nodes[VEHICLE & 0xFFFF].left.j = turned.i;

		observer.position = base;
		observer.position.z += 0.6f;
		observer.forward = forward;
		observer.up = up;
		expected.i = root.x + t.i + (real)cos(theta) * (base.x - root.x) - (real)sin(theta) * (base.y - root.y) - base.x;
		expected.j = root.y + t.j + (real)sin(theta) * (base.x - root.x) + (real)cos(theta) * (base.y - root.y) - base.y;
		expected.k = root.z + t.k + (base.z - root.z) - base.z;

		eye = render_interpolation_camera(0, &observer);
		CHECK(eye != &observer, "view not carried");
		CHECK(fabs(eye->position.x - observer.position.x - expected.i) < 1e-4f &&
			fabs(eye->position.y - observer.position.y - expected.j) < 1e-4f &&
			fabs(eye->position.z - observer.position.z - expected.k) < 1e-4f,
			"view carried by %f, %f, %f, not %f, %f, %f", eye->position.x - observer.position.x,
			eye->position.y - observer.position.y, eye->position.z - observer.position.z, expected.i, expected.j, expected.k);
		CHECK(fabs(eye->forward.i - turned.i) < 1e-4f && fabs(eye->forward.j - turned.j) < 1e-4f &&
			fabs(eye->forward.k) < 1e-4f, "view turned to %f, %f, %f", eye->forward.i, eye->forward.j, eye->forward.k);

		body = render_interpolation_object_node_matrices(RIDER);
		CHECK(body, "body not drawn carried");
		CHECK(fabs(body->position.x - base.x - expected.i) < 1e-4f && fabs(body->position.y - base.y - expected.j) < 1e-4f &&
			fabs(body->position.z - base.z - expected.k) < 1e-4f, "body carried to %f, %f, %f",
			body->position.x, body->position.y, body->position.z);
		CHECK(render_interpolation_object_node_matrices(VEHICLE) == NULL, "deck drawn other than as simulated");

		/* a carry that does not turn the view */
		look_carry = FALSE;
		next_frame();
		eye = render_interpolation_camera(0, &observer);
		CHECK(eye != &observer && fabs(eye->position.x - observer.position.x - expected.i) < 1e-4f,
			"view not carried with no look carry");
		CHECK(fabs(eye->forward.i - 1.0f) < 1e-4f && fabs(eye->forward.j) < 1e-4f, "view turned with no look carry");
		look_carry = TRUE;

		/* a rider in the window before riding takes no write */
		biped_get(RIDER)->biped.riding_ticks = RIDING_TICKS - 1;
		next_frame();
		CHECK(render_interpolation_camera(0, &observer) == &observer, "window rider carried");
		biped_get(RIDER)->biped.riding_ticks = RIDING_TICKS;

		/* a client carries its riders at the write itself */
		connection = _game_connection_network_client;
		next_frame();
		CHECK(render_interpolation_camera(0, &observer) == &observer, "client's rider carried");
		connection = _game_connection_network_server;

		in_cinematic = TRUE;
		next_frame();
		CHECK(render_interpolation_camera(0, &observer) == &observer, "cinematic's view carried");
		in_cinematic = FALSE;

		/* smooth motion on draws between ticks, by its own glide */
		interpolation_enabled = TRUE;
		next_frame();
		CHECK(render_interpolation_carried_camera(0, &observer) == &observer, "carried while smooth motion is on");
		interpolation_enabled = FALSE;
		next_frame();

		/* outside a frame nothing is drawn apart from the tick */
		render_interpolation_frame_end();
		CHECK(render_interpolation_object_node_matrices(RIDER) == NULL, "body carried outside a frame");
		CHECK(render_interpolation_camera(0, &observer) == &observer, "view carried outside a frame");
		render_interpolation_frame_begin();

		/* the rider's next tick carries by the write: what it is owed is then none, so nothing is doubled */
		marks[0].correction = corrections[VEHICLE & 0xFFFF];
		/* (and the tick's own step of the rider's, which a body kept from the frame before would lack) */
		nodes[RIDER & 0xFFFF].position.x += expected.i + 0.25f;
		nodes[RIDER & 0xFFFF].position.y += expected.j;
		nodes[RIDER & 0xFFFF].position.z += expected.k;
		moved_observer = observer;
		moved_observer.position.x += expected.i + 0.25f;
		moved_observer.position.y += expected.j;
		moved_observer.position.z += expected.k;
		next_frame();
		eye = render_interpolation_camera(0, &moved_observer);
		CHECK(fabs(eye->position.x - moved_observer.position.x) < 1e-6f &&
			fabs(eye->position.y - moved_observer.position.y) < 1e-6f &&
			fabs(eye->position.z - moved_observer.position.z) < 1e-6f, "view carried twice");
		CHECK(fabs(eye->forward.i - 1.0f) < 1e-6f && fabs(eye->forward.j) < 1e-6f, "view turned twice");
		body = render_interpolation_object_node_matrices(RIDER);
		CHECK(render_interpolation_object_node_matrices(RIDER) == NULL, "body carried twice");
	}
	/* with smooth motion off, a client's player the host sees riding is drawn carried too, with what it holds */
	CASE("remote off")
	{
		real_point3d const root = { 0.0f, 0.0f, 0.0f };
		real_vector3d const forward = { 1.0f, 0.0f, 0.0f };
		real_vector3d const up = { 0.0f, 0.0f, 1.0f };
		real_vector3d const t = { 0.5f, 0.0f, -0.2f };
		real_point3d moved_root;
		real_point3d point;

		interpolation_enabled = FALSE;
		render_interpolation_frame_begin();
		rider(OTHER_RIDER, 1, VEHICLE);
		biped_get(OTHER_RIDER)->biped.riding_ticks = RIDING_TICKS;
		place(WEAPON, _object_type_weapon, 50.2f, 1.4f);
		object_get(WEAPON)->object.parent_object_index = OTHER_RIDER;
		object_get(OTHER_RIDER)->object.first_child_object_index = WEAPON;
		vehicle_riding_correction_identity(&corrections[VEHICLE & 0xFFFF], VEHICLE);
		marks[1].role = _vehicle_riding_role_simulated;
		marks[1].correction = corrections[VEHICLE & 0xFFFF];
		distributed_predictions[1].state.unit_index = OTHER_RIDER;
		set_real_point3d(&moved_root, t.i, t.j, t.k);
		vehicle_riding_correction_compose(&corrections[VEHICLE & 0xFFFF], &root, &forward, &up, &moved_root, &forward, &up);
		nodes[VEHICLE & 0xFFFF].position = moved_root;

		CHECK(moved_by(render_interpolation_object_node_matrices(OTHER_RIDER), OTHER_RIDER, &t), "remote rider not drawn carried");
		CHECK(moved_by(render_interpolation_object_node_matrices(WEAPON), WEAPON, &t), "weapon left behind");
		CHECK(render_interpolation_object_node_matrices(VEHICLE) == NULL, "deck drawn other than as simulated");
		point = nodes[OTHER_RIDER & 0xFFFF].position;
		render_interpolation_object_point(OTHER_RIDER, &point);
		CHECK(fabs(point.x - nodes[OTHER_RIDER & 0xFFFF].position.x - t.i) < 1e-4f &&
			fabs(point.z - nodes[OTHER_RIDER & 0xFFFF].position.z - t.k) < 1e-4f, "shadow left behind");

		server_mode = TRUE;
		next_frame();
		CHECK(render_interpolation_object_node_matrices(OTHER_RIDER) == NULL, "rider carried on a dedicated server");
		server_mode = FALSE;

		/* a dead rider's next tick carries nothing */
		object_get(OTHER_RIDER)->object.damage_flags = 1L << _object_dead_bit;
		next_frame();
		CHECK(render_interpolation_object_node_matrices(OTHER_RIDER) == NULL, "dead rider carried");
		object_get(OTHER_RIDER)->object.damage_flags = 0;

		/* a part with no nodes: the whole family is drawn as simulated, never the body moved and the weapon not */
		object_get(WEAPON)->object.node_matrices.size = 0;
		next_frame();
		CHECK(render_interpolation_object_node_matrices(OTHER_RIDER) == NULL, "body carried without its weapon");
		object_get(WEAPON)->object.node_matrices.size = sizeof(real_matrix4x3);

		render_interpolation_frame_end();
		point = nodes[OTHER_RIDER & 0xFFFF].position;
		CHECK(render_interpolation_object_node_matrices(WEAPON) == NULL, "weapon carried outside a frame");
		render_interpolation_object_point(OTHER_RIDER, &point);
		CHECK(point.x == nodes[OTHER_RIDER & 0xFFFF].position.x && point.z == nodes[OTHER_RIDER & 0xFFFF].position.z,
			"point moved outside a frame");
		render_interpolation_frame_begin();

		/* the next tick's carry: nothing is then owed */
		marks[1].correction = corrections[VEHICLE & 0xFFFF];
		next_frame();
		CHECK(render_interpolation_object_node_matrices(OTHER_RIDER) == NULL, "rider carried twice");
	}
	return 0;
}
