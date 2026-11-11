/*
VEHICLE_RIDING.C (test)

vehicle_riding.c compiled whole: the carry, the frames, the guards and the
collision decision of a player standing on a moving vehicle, against fake
settings, connection and look impulses. test_vehicle_riding.py takes the
code (under_test.inc) and the enums (enums.inc) from the sources.
*/

#include "harness.h"
typedef unsigned short word;
#include <math.h>
/* (the airborne branch's 2d maths, engine.inc) */
typedef struct { real i, j; } real_vector2d;
#include "enums.inc"
#include "engine.inc"

typedef struct { real yaw, pitch; } real_euler_angles2d;
#define TICKS_PER_SECOND 30
#define PI 3.14159265f

/* the settings (port_config.c) */
static int setting_riding = 1, setting_override = 0, setting_look_carry = 1;
static double setting_damage_speed, setting_teleport_step = 4.0, setting_moved_alone = 0.25, setting_air_grip = 0.045, setting_grace = 4.0, setting_leash = 0.25;

int config_boolean(const char *name)
{
	if (!strcmp(name, "network.vehicle_riding"))
		return setting_riding;
	if (!strcmp(name, "server.vehicle_riding_override_gametype"))
		return setting_override;
	if (!strcmp(name, "debug.riding_look_carry"))
		return setting_look_carry;
	return 0;
}

double config_real(const char *name)
{
	if (!strcmp(name, "debug.riding_transition_leash"))
		return setting_leash;
	if (!strcmp(name, "debug.riding_damage_speed"))
		return setting_damage_speed;
	if (!strcmp(name, "debug.riding_teleport_step"))
		return setting_teleport_step;
	if (!strcmp(name, "debug.riding_moved_alone"))
		return setting_moved_alone;
	if (!strcmp(name, "debug.riding_air_grip"))
		return setting_air_grip;
	if (!strcmp(name, "debug.riding_enclosure_grace_ticks"))
		return setting_grace;
	return 0.0;
}

static short connection = _game_connection_local;
static boolean in_editor;
static boolean double_speed = FALSE;

boolean game_players_are_double_speed(void)
{
	return double_speed;
}

short game_connection(void)
{
	return connection;
}

boolean game_in_editor(void)
{
	return in_editor;
}

/* the look impulses each local player was given */
static real impulses[MAXIMUM_LOCAL_PLAYERS];

void player_control_permanent_impulse(short local_player_index, real_euler_angles2d const *delta)
{
	impulses[local_player_index] += delta->yaw;
}

/* the network game's switches (network_game_manager.h's cooperative_flags):
the host's, on every machine */
struct game_variant { short game_engine_index; struct { unsigned long flags; } universal_variant; };
struct network_game { struct game_variant variant; byte cooperative_flags; };
static struct network_game test_game;
static struct network_game *test_game_pointer = &test_game;

struct network_game *network_game_get_game(void)
{
	return test_game_pointer;
}

/* the running game's engine and variant (a local game), and whether this is a dedicated server */
static boolean engine_running;
static struct game_variant engine_variant;
static int server_mode;

boolean game_engine_running(void)
{
	return engine_running;
}

struct game_variant *game_engine_get_variant(void)
{
	return &engine_variant;
}

int halo_server_mode(void)
{
	return server_mode;
}

static long slot_storage = NONE, slot_size, slot_calls, game_time;
static short script_type = NONE, scenario_global = NONE;
static int logged, cleared;
static char const *global_name;
static void *game_state_malloc(char const *name, void *type, long size)
{
	CHECK(!strcmp(name, "vehicle riding map") && !type, "allocation arguments");
	slot_size = size;
	slot_calls++;
	return &slot_storage;
}
static short hs_runtime_port_executing_script_type(void) { return script_type; }
static short hs_runtime_port_scenario_boolean(char const *name) { global_name = name; return scenario_global; }
static long game_time_get(void) { return game_time; }
static void error(long priority, char const *format, ...) { (void)priority; (void)format; logged++; }
struct object_iterator { int next; };
struct biped_datum { int unused; };
static struct biped_datum fake_biped;
enum { _object_mask_biped = 1 };
static void object_iterator_new(struct object_iterator *i, long mask, long flags) { (void)mask; (void)flags; i->next = 0; }
static void *object_iterator_next(struct object_iterator *i) { return i->next++ ? NULL : &fake_biped; }
static void biped_riding_clear(struct biped_datum *biped) { (void)biped; cleared++; }
#include "under_test.inc"

/* the host's start of a game (the settings record goes out with its riding bit), and every machine's map */
static void riding_host_start(void)
{
	connection = _game_connection_network_server;
	network_game_port_vehicle_riding_start(&test_game);
}

static void riding_map_now(void)
{
	vehicle_riding_map_reset();
	vehicle_riding_map_decide();
}

static void riding_game(int engine, int riding)
{
	memset(&test_game.variant, 0, sizeof(test_game.variant));
	test_game.variant.game_engine_index = (short)engine;
	if (riding)
		test_game.variant.universal_variant.flags = FLAG(_game_variant_vehicle_riding_bit);
}

/* the airborne and ground branches of biped_update_physics (bipeds.c),
taken whole into airborne_branch (airborne.inc) and ground_branch
(ground.inc): the fields of struct biped_physics they read and write */
struct biped_physics
{
	real_vector3d forward;
	real_vector3d aiming;
	real_vector3d velocity;
	real_vector3d movement_desired;
	real movement_penalty;
	real acceleration_maximum;
	real airborne_acceleration_maximum;
	real downhill_k0;
	real downhill_k1;
	real downhill_velocity_scale;
	real uphill_k0;
	real uphill_k1;
	real uphill_velocity_scale;
	real_plane3d ground_plane;
	real_vector3d new_velocity;
	unsigned short out_flags;
	real_vector3d air_frame_velocity;
};

static real global_gravity = .004f;
static real_vector3d const global_up = { 0.f, 0.f, 1.f };
static real_vector3d const global_forward = { 1.f, 0.f, 0.f };
static real_vector3d const *global_up3d = &global_up;
static real_vector3d const *global_forward3d = &global_forward;

#include "airborne.inc"
#include "ground.inc"

#define NEAR(a, b) CHECK(fabs((double)(a) - (double)(b)) < .0001, "%s: %g, not %g", #a, (double)(a), (double)(b))
#define VEHICLE 0x10000001
#define OTHER_VEHICLE 0x10010002
#define THIRD_VEHICLE 0x10020003
#define BIPED 0x20000005
#define OTHER_BIPED 0x20010006
/* the width less 1/64 at which a vehicle's touch is found
(physics_compute_biped_collision), for a pill 0.2 wide */
#define CONTACT_WIDTH (.2f - .015625f)
/* a node limit that never rejects (the cases of the carry itself) */
#define ANY_LIMIT 100.f
/* the rider's damage speed (0.257 a tick for a player) */
#define NODE_LIMIT .257f

static real_vector3d const zero = { 0.f, 0.f, 0.f };
static real_vector3d const up = { 0.f, 0.f, 1.f };

static real_point3d point(real x, real y, real z)
{
	real_point3d result;

	result.x = x;
	result.y = y;
	result.z = z;
	return result;
}

static real_vector3d vector(real i, real j, real k)
{
	real_vector3d result;

	result.i = i;
	result.j = j;
	result.k = k;
	return result;
}

/* a vehicle at (x, y, z), turned by yaw about up, after this many updates */
static struct vehicle_riding_vehicle vehicle_at(long object_index, real x, real y, real z, real yaw, long updates)
{
	struct vehicle_riding_vehicle vehicle;

	memset(&vehicle, 0, sizeof(vehicle));
	vehicle.object_index = object_index;
	vehicle.node_index = NONE;
	vehicle.position = point(x, y, z);
	vehicle.origin = vehicle.position;
	vehicle.forward = vector((real)cos(yaw), (real)sin(yaw), 0.f);
	vehicle.up = up;
	vehicle.body_forward = vehicle.forward;
	vehicle.body_up = vehicle.up;
	vehicle.updates = updates;
	vehicle.bounding_sphere_center = vehicle.position;
	vehicle.bounding_sphere_radius = 3.f;
	vehicle.correction = *vehicle_riding_vehicle_correction(object_index);
	return vehicle;
}

/* a node of a body that stands at its origin, not turned: the node's own pose is the vehicle's */
static struct vehicle_riding_vehicle node_of_body(struct vehicle_riding_vehicle node, short node_index, real_point3d origin)
{
	node.node_index = node_index;
	node.origin = origin;
	node.body_forward = vector(1.f, 0.f, 0.f);
	node.body_up = up;
	return node;
}

/* a biped that ended its last tick standing at base on the vehicle */
static struct vehicle_riding_state riding_on(struct vehicle_riding_vehicle const *vehicle, real_point3d base,
	real_vector3d velocity)
{
	struct vehicle_riding_state state;

	vehicle_riding_clear(&state);
	state.ticks = RIDING_TICKS;
	state.object_index = vehicle->object_index;
	state.vehicle_position = vehicle->position;
	state.vehicle_forward = vehicle->forward;
	state.vehicle_up = vehicle->up;
	state.vehicle_updates = vehicle->updates;
	state.velocity = velocity;
	state.node_index = vehicle->node_index;
	state.body_position = vehicle->origin;
	state.body_forward = vehicle->body_forward;
	state.body_up = vehicle->body_up;
	state.mark.role = vehicle->role;
	state.mark.time = vehicle->time;
	state.mark.correction = vehicle->correction;
	vehicle_riding_point_to_local(&vehicle->position, &vehicle->forward, &vehicle->up, &base, &state.local_point);
	return state;
}

/* a vector turned about the vehicle's left axis (j) by angle: a pitch */
static real_vector3d pitched(real_vector3d v, real angle)
{
	return vector(v.i*(real)cos(angle) + v.k*(real)sin(angle), v.j, -v.i*(real)sin(angle) + v.k*(real)cos(angle));
}

/* the world velocity after a tick in which a biped riding at velocity relative
to its deck (no friction, no gravity) was carried and recorded on support */
static real_vector3d riding_tick(struct vehicle_riding_state *state, struct vehicle_riding_vehicle const *vehicle,
	real_point3d *base, real_vector3d world_velocity, struct vehicle_riding_vehicle const *support, boolean supported,
	real_vector3d support_velocity)
{
	struct vehicle_riding_step tick_step;
	real_vector3d velocity, frame;
	boolean valid = state->ticks > 0 &&
		vehicle_riding_snapshot_step(state, vehicle, base, 1.4f, .2f, FALSE, ANY_LIMIT, &tick_step);

	if (valid && vehicle_riding_is_riding(state))
	{
		base->x += tick_step.displacement.i;
		base->y += tick_step.displacement.j;
		base->z += tick_step.displacement.k;
	}
	vehicle_riding_seed(state, &world_velocity, &velocity, &frame);
	base->x += velocity.i;
	base->y += velocity.j;
	base->z += velocity.k;
	vehicle_riding_record(state, valid ? &tick_step : NULL, support, supported, &support_velocity, &up, base, FALSE,
		&velocity);
	return velocity;
}

/* a tick of a turret (node 2) at the body's origin that turns by yaw while the body drives 0.35 forward, over a rider 2 to its
right (it moves 4 sin(yaw/2) on the body) */
static boolean turret_tick(real yaw, real limit, struct vehicle_riding_state *state, real_point3d *base,
	struct vehicle_riding_step *out)
{
	struct vehicle_riding_vehicle before = node_of_body(vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10), 2, point(0.f, 0.f, 0.f));
	struct vehicle_riding_vehicle after = node_of_body(vehicle_at(VEHICLE, .35f, 0.f, 0.f, yaw, 11), 2, point(.35f, 0.f, 0.f));

	*base = point(0.f, -2.f, 0.f);
	*state = riding_on(&before, *base, zero);
	return vehicle_riding_snapshot_step(state, &after, base, 1.4f, .2f, FALSE, limit, out);
}

/* one airborne tick in the window over a deck now moving at (deck_i,
deck_j, 0.02), at a grip, open or enclosed, the vehicle's update spanning
one player tick: the air frame was the take-off's (0.3, 0, -0.05), the
biped moves at (0.35, 0.05, 0.1) */
static struct vehicle_riding_state gripped(double grip, real deck_i, real deck_j, short enclosure,
	real_vector3d *velocity, real_vector3d *frame)
{
	struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
	struct vehicle_riding_state state = riding_on(&vehicle, point(0.f, 0.f, 0.f), vector(.3f, 0.f, -.05f));
	struct vehicle_riding_step held;
	struct vehicle_riding_air air;

	memset(&held, 0, sizeof(held));
	held.updates = 1;
	held.player_ticks = 1.f;
	held.over_deck = TRUE;
	held.tracked_deck_velocity = vector(deck_i, deck_j, .02f);
	memset(&air, 0, sizeof(air));
	air.enclosure = enclosure;
	setting_air_grip = grip;
	vehicle_riding_latch();
	state.ticks = 40;
	*velocity = vector(.35f, .05f, .1f);
	vehicle_riding_air_frame(&state, &held, &air, TRUE, velocity, frame);
	return state;
}

/* the air frame of a jump with no peak yet and no verdict (a window's first
tick of it, each tick) */
static void open_air_frame(struct vehicle_riding_state *state, struct vehicle_riding_step const *step,
	boolean airborne, real_vector3d *velocity, real_vector3d *frame)
{
	struct vehicle_riding_air air;

	memset(&air, 0, sizeof(air));
	vehicle_riding_air_frame(state, step, &air, airborne, velocity, frame);
}

/* a biped that left a deck moving at (i, j, 0) a tick, its air frame that,
its air fresh (no peak yet) and open or enclosed */
static struct vehicle_riding_state taken_off(real i, real j, struct vehicle_riding_air *air, short enclosure)
{
	struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
	struct vehicle_riding_state state = riding_on(&vehicle, point(0.f, 0.f, 0.f), vector(i, j, 0.f));

	state.ticks = 40;
	state.falling = 1;
	memset(air, 0, sizeof(*air));
	air->enclosure = enclosure;
	return state;
}

/* one airborne tick over a deck now moving at (deck_i, deck_j, 0), the
vehicle's update spanning one player tick */
static void air_tick(struct vehicle_riding_state *state, struct vehicle_riding_air *air, real deck_i, real deck_j,
	real_vector3d *velocity, real_vector3d *frame)
{
	struct vehicle_riding_step held;

	memset(&held, 0, sizeof(held));
	held.updates = 1;
	held.player_ticks = 1.f;
	held.over_deck = TRUE;
	held.tracked_deck_velocity = vector(deck_i, deck_j, 0.f);
	vehicle_riding_air_frame(state, &held, air, TRUE, velocity, frame);
}

/* contact-sequence's world: the vehicle the biped rides or left
(vehicles[0], VEHICLE) and two others, each moving by its motion an update;
the faces that touch the biped (a plane of a vehicle through faces[n], out
of it along normals[n], moving with it, its contact plane reported as
planes[n]; has_face[n] FALSE: none); the biped; and what the last tick did */
struct contact_world
{
	struct vehicle_riding_vehicle vehicles[3];
	real_vector3d motions[3];
	boolean has_face[3];
	real_point3d faces[3];
	real_vector3d normals[3];
	real_vector3d planes[3];
	struct vehicle_riding_state state;
	struct vehicle_riding_air air;
	real_point3d base;
	real_vector3d velocity;
	boolean airborne;
	boolean over_deck;
	boolean double_speed;
	short tick_touches;
	real moved;
	short settle;
	short fallbacks;
};

/* the vehicles at rest at (0, 0, 0), (-2, 0, 0) and (0, 2, 0), with no
faces; the biped at base moving at velocity, in the window of vehicles[0]
(ticks 1-59, its air frame frame) or riding it (RIDING_TICKS); no records */
static void contact_world_new(struct contact_world *world, real_point3d base, real_vector3d velocity,
	real_vector3d frame, char ticks, boolean airborne, boolean over_deck)
{
	memset(world, 0, sizeof(*world));
	world->vehicles[0] = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
	world->vehicles[1] = vehicle_at(OTHER_VEHICLE, -2.f, 0.f, 0.f, 0.f, 10);
	world->vehicles[2] = vehicle_at(THIRD_VEHICLE, 0.f, 2.f, 0.f, 0.f, 10);
	world->state = riding_on(&world->vehicles[0], base, frame);
	world->state.ticks = ticks;
	world->state.falling = airborne && ticks < RIDING_TICKS;
	world->base = base;
	world->velocity = velocity;
	world->airborne = airborne;
	world->over_deck = over_deck;
	vehicle_riding_contacts_clear();
}

/* a face of vehicles[vehicle], depth in front of the biped's base along
normal, the vehicle moving by motion an update */
static void contact_face(struct contact_world *world, short vehicle, real_vector3d normal, real depth,
	real_vector3d motion)
{
	world->has_face[vehicle] = TRUE;
	world->normals[vehicle] = normal;
	world->planes[vehicle] = normal;
	world->faces[vehicle] = point(world->base.x - normal.i*depth, world->base.y - normal.j*depth,
		world->base.z - normal.k*depth);
	world->motions[vehicle] = motion;
}

/* how far in front of a vehicle's face the biped's base is, offset by ahead */
static real contact_depth(struct contact_world const *world, short vehicle, real_vector3d const *ahead)
{
	real_point3d at = point(world->base.x + ahead->i, world->base.y + ahead->j, world->base.z + ahead->k);
	real_vector3d from;

	vector_from_points3d(&world->faces[vehicle], &at, &from);
	return dot_product3d(&from, &world->normals[vehicle]);
}

/* one game tick in the game's order (objects_update_passes): the vehicles
update (unless double speed skips them) and each face within the contact
width of the biped, where its last tick left it, is judged and recorded
(biped_riding_collision); then the biped's tick (biped_update_moving): its
step, the carry, the air frame, its records resolved against the velocity
the physics then moves it by (biped_riding_contacts), the real physics
branch, its move (no sweep: nothing in this world but the faces) and its
record */
static void contact_tick(struct contact_world *world, boolean vehicles_update)
{
	struct vehicle_riding_step tick_step;
	struct vehicle_riding_contact contacts[RIDING_CONTACT_RECORDS];
	struct biped_physics physics;
	real_vector3d deck;
	real_vector3d normal;
	real_vector3d move;
	real_vector3d ahead;
	real_vector3d further;
	real_vector3d frame;
	boolean valid = FALSE;
	boolean carried;
	short count;
	short index;
	short vehicle;

	world->tick_touches = 0;
	world->moved = 0.f;
	for (vehicle = 0; vehicles_update && vehicle < 3; vehicle++)
	{
		struct vehicle_riding_vehicle *moving = &world->vehicles[vehicle];
		real_vector3d const *motion = &world->motions[vehicle];

		moving->position = point(moving->position.x + motion->i, moving->position.y + motion->j,
			moving->position.z + motion->k);
		moving->origin = moving->position;
		moving->bounding_sphere_center = moving->position;
		moving->translational_velocity = *motion;
		moving->updates++;
		world->faces[vehicle] = point(world->faces[vehicle].x + motion->i, world->faces[vehicle].y + motion->j,
			world->faces[vehicle].z + motion->k);
	}
	for (vehicle = 0; vehicles_update && vehicle < 3; vehicle++)
	{
		struct vehicle_riding_vehicle const *touching = &world->vehicles[vehicle];

		if (!world->has_face[vehicle] || contact_depth(world, vehicle, &zero) >= CONTACT_WIDTH ||
			vehicle_riding_collision_window(&world->state, TRUE, touching->object_index) !=
				_vehicle_riding_collision_judge)
		{
			continue;
		}
		vehicle_riding_deck_point_velocity(touching, &world->base, world->double_speed, &deck);
		CHECK(vehicle_riding_collision_contact(&world->state, touching->object_index, world->over_deck && vehicle == 0,
			&touching->bounding_sphere_center, &world->base, &world->planes[vehicle], &deck, NODE_LIMIT,
			&world->velocity, &normal) == _vehicle_riding_collision_resolved, "a slow touch was a hit");
		CHECK(vehicle_riding_contact_record(0, touching->object_index, &normal), "a touch not recorded");
		world->tick_touches++;
	}
	if (world->state.ticks > 0)
	{
		valid = vehicle_riding_snapshot_step(&world->state, &world->vehicles[0], &world->base, 1.4f, .2f,
			world->double_speed, ANY_LIMIT, &tick_step);
	}
	carried = valid && vehicle_riding_is_riding(&world->state);
	if (valid && !carried)
		tick_step.over_deck = world->over_deck;
	if (carried)
	{
		world->base = point(world->base.x + tick_step.displacement.i, world->base.y + tick_step.displacement.j,
			world->base.z + tick_step.displacement.k);
	}
	vehicle_riding_air_begin(&world->air, &world->state, valid ? &tick_step : NULL, world->airborne);
	memset(&physics, 0, sizeof(physics));
	physics.forward = vector(1.f, 0.f, 0.f);
	physics.aiming = physics.forward;
	physics.acceleration_maximum = .0053333333f;
	physics.airborne_acceleration_maximum = .001f;
	physics.ground_plane.n = up;
	physics.downhill_k0 = -.5f;
	physics.downhill_k1 = -.9f;
	physics.downhill_velocity_scale = 1.f;
	physics.uphill_k0 = .5f;
	physics.uphill_k1 = .9f;
	physics.uphill_velocity_scale = 1.f;
	vehicle_riding_seed(&world->state, &world->velocity, &physics.velocity, &frame);
	vehicle_riding_air_frame(&world->state, valid ? &tick_step : NULL, &world->air, world->airborne,
		&physics.velocity, &physics.air_frame_velocity);
	/* (after the air frame, from the velocity the physics moves it by: a
	change of the frame along the touch is then counted once) */
	count = vehicle_riding_contact_take(0, contacts);
	for (index = 0; index < count; index++)
	{
		vehicle = 0;
		while (world->vehicles[vehicle].object_index != contacts[index].vehicle_index)
		{
			vehicle++;
		}
		world->moved += vehicle_riding_contact_move(valid ? &tick_step : NULL, carried, &world->vehicles[vehicle],
			&world->base, &physics.velocity, &contacts[index].normal, world->double_speed, &move, &ahead);
		world->base = point(world->base.x + move.i, world->base.y + move.j, world->base.z + move.k);
		/* (the fallback, judged on this world's face: still touching once the
		biped's own motion ahead is made, then one spot twice the width out
		along the touch's normal; the fix is not in this world and frees it) */
		further = vector(contacts[index].normal.i*.4f + ahead.i, contacts[index].normal.j*.4f + ahead.j,
			contacts[index].normal.k*.4f + ahead.k);
		world->settle = vehicle_riding_contact_settle(contact_depth(world, vehicle, &ahead) < CONTACT_WIDTH,
			contact_depth(world, vehicle, &further) < CONTACT_WIDTH, TRUE);
		if (world->settle != _vehicle_riding_settle_moved)
			world->fallbacks++;
		if (world->settle == _vehicle_riding_settle_candidate)
		{
			world->base = point(world->base.x + contacts[index].normal.i*.4f,
				world->base.y + contacts[index].normal.j*.4f, world->base.z + contacts[index].normal.k*.4f);
		}
	}
	if (world->airborne)
	{
		airborne_branch(&physics, FLAG(_biped_physics_in_airborne_bit));
	}
	else
	{
		ground_branch(&physics, 0);
		/* (no floor in this world: the ground's press into it is no move) */
		physics.new_velocity.k = 0.f;
	}
	world->base = point(world->base.x + physics.new_velocity.i, world->base.y + physics.new_velocity.j,
		world->base.z + physics.new_velocity.k);
	vehicle_riding_record(&world->state, valid ? &tick_step : NULL, carried ? &world->vehicles[0] : NULL,
		!world->airborne, &zero, &up, &world->base, FALSE, &physics.new_velocity);
	world->velocity = physics.new_velocity;
}

/* one tick of a rider standing in an enclosed vehicle pitched to pitch (radians) at its updates'th update: the
carry, the cabin's protection (its verdict as the cabin has it), native physics on the vehicle's floor (ground while
the floor's normal k is at least the biped's limit 0.7, else airborne and falling in the deck's frame), an enclosed
rider's steep vehicle carrying it when the sweep touched it (contact), or holding it with no touch when the deck had
carried it into the tick (beside: another vehicle's touch), and the recording. TRUE: carried by the steep vehicle */
static boolean steep_tick(struct vehicle_riding_state *state, struct vehicle_riding_cabin *cabin, real pitch,
	long updates, boolean contact, boolean beside, boolean eligible, real_point3d *base, real_vector3d *relative,
	real_vector3d *world)
{
	struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, updates);
	struct vehicle_riding_step tick_step;
	long objects[2];
	long carrier_vehicle;
	short carrier = NONE;
	boolean valid;
	boolean native;
	boolean entered;
	boolean held;

	vehicle.forward = vector((real)cos(pitch), 0.f, (real)sin(pitch));
	vehicle.up = vector(-(real)sin(pitch), 0.f, (real)cos(pitch));
	vehicle.body_forward = vehicle.forward;
	vehicle.body_up = vehicle.up;
	valid = state->ticks > 0 && vehicle_riding_snapshot_step(state, &vehicle, base, 1.4f, .2f, FALSE, ANY_LIMIT, &tick_step);
	entered = valid && vehicle_riding_is_riding(state);
	if (entered)
	{
		base->x += tick_step.displacement.i;
		base->y += tick_step.displacement.j;
		base->z += tick_step.displacement.k;
	}
	vehicle_riding_cabin_begin(cabin, BIPED, state, valid);
	cabin->exception = valid && vehicle_riding_cabin_exception(eligible, TRUE,
		vehicle_riding_cabin_encloses(cabin, state, NULL), vehicle_riding_cabin_up(&tick_step), .7f);
	carrier_vehicle = vehicle_riding_cabin_carrier_vehicle(cabin, state, cabin->exception);
	native = contact && vehicle.up.k >= .7f;
	if (native)
		*relative = zero;
	else
		relative->k -= .004f;
	base->x += relative->i;
	base->y += relative->j;
	base->z += relative->k;
	objects[0] = beside ? OTHER_VEHICLE : NONE;
	objects[1] = contact ? VEHICLE : NONE;
	if (carrier_vehicle != NONE)
		carrier = vehicle_riding_cabin_carrier(carrier_vehicle, 2, objects);
#ifdef HOLD_ANY_VERDICT
	held = carrier == NONE && vehicle_riding_cabin_holds(state->object_index, entered);
#else
	held = carrier == NONE && vehicle_riding_cabin_holds(carrier_vehicle, entered);
#endif
	*world = *relative;
	vehicle_riding_record(state, valid ? &tick_step : NULL, carrier != NONE || held || native ? &vehicle : NULL, native,
		&zero, &up, base, FALSE, world);
	cabin->take_off = FALSE;
	return carrier != NONE || held;
}

/* a rider on an upright enclosed vehicle, its cabin enclosed; and a tick of it pitched to degrees (steep_tick) */
struct steep_rider
{
	struct vehicle_riding_state state;
	struct vehicle_riding_cabin cabin;
	real_point3d base;
	real_vector3d relative;
	real_vector3d world;
	long updates;
};

static void steep_board(struct steep_rider *rider)
{
	struct vehicle_riding_vehicle level = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);

	rider->base = point(1.f, 0.f, .2f);
	rider->relative = zero;
	rider->world = zero;
	rider->updates = 10;
	rider->state = riding_on(&level, rider->base, zero);
	vehicle_riding_cabin_clear(&rider->cabin);
	vehicle_riding_cabin_begin(&rider->cabin, BIPED, &rider->state, TRUE);
	vehicle_riding_cabin_standing(&rider->cabin, TRUE, 4);
}

static boolean steep_step(struct steep_rider *rider, int degrees, boolean contact, boolean beside, boolean eligible)
{
	return steep_tick(&rider->state, &rider->cabin, (real)degrees*PI/180.f, ++rider->updates, contact, beside, eligible,
		&rider->base, &rider->relative, &rider->world);
}

/* boarded upright, then pitched a degree a tick to degrees with the steep contact */
static void steep_pitch_to(struct steep_rider *rider, int degrees)
{
	int tick;

	steep_board(rider);
	for (tick = 1; tick <= degrees; tick++)
		steep_step(rider, tick, TRUE, FALSE, TRUE);
}

/* a correction write, separate from a vehicle's counted physics update */
static void correction_write(struct vehicle_riding_vehicle const *before, struct vehicle_riding_vehicle *after)
{
	vehicle_riding_vehicle_corrected(after->object_index, &before->origin, &before->body_forward, &before->body_up,
		&after->origin, &after->body_forward, &after->body_up);
	after->correction = *vehicle_riding_vehicle_correction(after->object_index);
}

#define PRECISE(a, b) CHECK(fabs((double)(a) - (double)(b)) < .00001, "%s: %.9g, not %.9g", #a, (double)(a), (double)(b))

static void correction_velocity(struct vehicle_riding_step const *step, real i, real j, real k)
{
	PRECISE(step->velocity_now.i, i);
	PRECISE(step->velocity_now.j, j);
	PRECISE(step->velocity_now.k, k);
	PRECISE(step->tracked_deck_velocity.i, i);
	PRECISE(step->tracked_deck_velocity.j, j);
	PRECISE(step->tracked_deck_velocity.k, k);
	PRECISE(step->body_deck_velocity.i, i);
	PRECISE(step->body_deck_velocity.j, j);
	PRECISE(step->body_deck_velocity.k, k);
}

/* a vehicle the client follows (the host's word, no physics of its own), written at message time
time and moving at vx */
static struct vehicle_riding_vehicle followed_at(real x, long time, real vx)
{
	struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, x, 0.f, 0.f, 0.f, 10);

	vehicle.role = _vehicle_riding_role_followed;
	vehicle.time = time;
	vehicle.translational_velocity = vector(vx, 0.f, 0.f);
	return vehicle;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	struct vehicle_riding_step step;

	vehicle_riding_map_decide();
	logged = 0;
	vehicle_riding_latch();

	if (!strncmp(case_name, "map-", 4))
	{
		if (strcmp(case_name, "map-unallocated"))
		{
			vehicle_riding_map_initialize();
			vehicle_riding_map_reset();
		}
		script_type = _hs_script_startup;
		CASE("map-wins-local") {
			vehicle_riding_map_script(FALSE); CHECK(!vehicle_riding_enabled(), "map off lost");
			setting_riding = 0; vehicle_riding_map_decide(); vehicle_riding_latch();
			vehicle_riding_map_script(TRUE); CHECK(vehicle_riding_enabled(), "map on lost");
			vehicle_riding_map_script(FALSE); vehicle_riding_map_script(TRUE);
			CHECK(vehicle_riding_enabled(), "last call lost");
		}
		else CASE("map-wins-network") {
			connection = _game_connection_network_server;
			vehicle_riding_map_script(TRUE); CHECK(vehicle_riding_enabled(), "host map on lost");
			test_game.cooperative_flags = FLAG(_network_game_cooperative_vehicle_riding_bit); vehicle_riding_map_decide();
			vehicle_riding_map_script(FALSE); CHECK(!vehicle_riding_enabled(), "host map off lost");
			connection = _game_connection_network_client; test_game.cooperative_flags = 0; vehicle_riding_map_decide();
			vehicle_riding_map_received(TRUE); CHECK(vehicle_riding_enabled(), "received on lost");
			test_game.cooperative_flags = FLAG(_network_game_cooperative_vehicle_riding_bit); vehicle_riding_map_decide();
			vehicle_riding_map_received(FALSE); CHECK(!vehicle_riding_enabled(), "received off lost");
			vehicle_riding_map_received(NONE); CHECK(vehicle_riding_enabled(), "bit fallback lost");
			test_game.cooperative_flags = 0; vehicle_riding_map_decide(); CHECK(!vehicle_riding_enabled(), "bit off fallback lost");
		}
		else CASE("map-client-own-call") {
			connection = _game_connection_network_client; vehicle_riding_map_script(TRUE);
			CHECK(slot_storage == NONE && !logged, "client wrote or logged");
			connection = _game_connection_local; CHECK(vehicle_riding_map() == NONE, "client slot counted");
		}
		else CASE("map-script-startup-only") {
			short types[] = { NONE, _hs_script_dormant, _hs_script_continuous, _hs_script_startup };
			int i;
			for (i = 0; i < 4; i++) {
				script_type = types[i]; game_time = i == 3 ? 1 : 0; vehicle_riding_map_script(TRUE);
				CHECK(vehicle_riding_map() == NONE, "invalid script counted");
			}
			CHECK(logged == 1, "refusal not once"); vehicle_riding_map_reset();
			vehicle_riding_map_script(TRUE); CHECK(logged == 2, "refusal not rearmed");
		}
		else CASE("map-per-map") {
			setting_riding = 0; vehicle_riding_map_decide(); vehicle_riding_latch(); vehicle_riding_map_script(TRUE);
			vehicle_riding_map_reset(); CHECK(vehicle_riding_map() == NONE && !vehicle_riding_enabled(), "map leaked");
		}
		else CASE("map-latch") {
			setting_riding = 0; vehicle_riding_map_decide(); vehicle_riding_map_script(TRUE); CHECK(vehicle_riding_latch(), "latch ignored on");
			setting_riding = 1; vehicle_riding_map_decide(); vehicle_riding_map_script(FALSE); CHECK(!vehicle_riding_latch(), "latch ignored off");
			vehicle_riding_map_reset(); CHECK(vehicle_riding_latch(), "latch fallback lost");
		}
		else CASE("map-revert-kept") { vehicle_riding_map_script(TRUE); vehicle_riding_latch(); CHECK(vehicle_riding_map() == TRUE, "latch erased map"); }
		else CASE("map-persist") {
			long saved;
			CHECK(slot_calls == 1 && slot_size == sizeof(long) && slot_size == 4, "not game state allocation");
			vehicle_riding_map_script(TRUE); saved = slot_storage;
			vehicle_riding_map_script(FALSE); CHECK(vehicle_riding_map() == FALSE, "change lost");
			slot_storage = saved; CHECK(vehicle_riding_map() == TRUE, "saved bytes did not restore choice");
		}
		else CASE("map-restore-server") {
			int i;
			for (i = 0; i < 2; i++) {
				connection = i ? _game_connection_local : _game_connection_network_server;
				setting_riding = 0; vehicle_riding_map_decide(); vehicle_riding_map_script(TRUE); cleared = 0;
				bipeds_riding_latch(); CHECK(cleared == 0, "restored rider cleared");
				vehicle_riding_map_reset(); bipeds_riding_latch(); CHECK(cleared == 1, "off rider kept");
				setting_riding = 1; vehicle_riding_map_decide(); vehicle_riding_map_script(FALSE); cleared = 0;
				bipeds_riding_latch(); CHECK(cleared == 1, "map off rider kept");
			}
		}
		else CASE("map-editor") {
			vehicle_riding_map_script(TRUE); in_editor = TRUE; CHECK(!vehicle_riding_enabled(), "editor rides");
			vehicle_riding_map_reset(); vehicle_riding_map_script(TRUE);
			CHECK(vehicle_riding_map() == NONE && !logged, "editor call counted");
		}
		else CASE("map-unallocated") { vehicle_riding_map_script(TRUE); CHECK(vehicle_riding_map() == NONE && !logged, "unallocated counted"); }
		else CASE("map-global") {
			setting_riding = 0; vehicle_riding_map_decide(); vehicle_riding_latch(); scenario_global = TRUE; vehicle_riding_map_start();
			CHECK(vehicle_riding_enabled() && !strcmp(global_name, "vehicle_riding"), "global on or name lost");
			setting_riding = 1; vehicle_riding_map_decide(); vehicle_riding_latch(); scenario_global = FALSE; vehicle_riding_map_start();
			CHECK(!vehicle_riding_enabled(), "global off lost");
			vehicle_riding_map_reset(); scenario_global = NONE; vehicle_riding_map_start();
			CHECK(vehicle_riding_map() == NONE && vehicle_riding_enabled(), "absent global counted");
		}
		else CASE("map-global-then-call") {
			scenario_global = TRUE; vehicle_riding_map_start(); vehicle_riding_map_script(FALSE);
			CHECK(vehicle_riding_map() == FALSE, "global beat call");
		}
		else CASE("map-global-set-later") {
			scenario_global = TRUE; vehicle_riding_map_start(); scenario_global = FALSE;
			CHECK(vehicle_riding_latch() && vehicle_riding_enabled() && vehicle_riding_map() == TRUE, "set leaked through latch");
			vehicle_riding_map_reset(); vehicle_riding_map_start(); CHECK(vehicle_riding_map() == FALSE, "new map not read");
		}
		else CASE("map-global-client") {
			connection = _game_connection_network_client; scenario_global = TRUE; vehicle_riding_map_start();
			CHECK(slot_storage == NONE, "client global counted");
		}
		else CASE("map-global-editor") {
			in_editor = TRUE; scenario_global = TRUE; vehicle_riding_map_start(); CHECK(slot_storage == NONE, "editor global counted");
		}
		else CASE("map-decision-held") {
			short connections[3] = { _game_connection_local, _game_connection_network_server, _game_connection_network_client };
			int index;
			for (index = 0; index < 3; index++) {
				connection = connections[index];
				setting_riding = 1; engine_running = FALSE; engine_variant.universal_variant.flags = 0;
				test_game.cooperative_flags = FLAG(_network_game_cooperative_vehicle_riding_bit);
				riding_map_now();
				CHECK(vehicle_riding_enabled(), "the map's decision is off (connection %d)", connection);
				/* (the option, the gametype's bit and the host's bit all change mid-map, then a revert's latch) */
				setting_riding = 0; engine_variant.universal_variant.flags = FLAG(_game_variant_vehicle_riding_bit);
				test_game.cooperative_flags = 0;
				vehicle_riding_latch();
				CHECK(vehicle_riding_enabled(), "a change mid-map or a revert took the decision (connection %d)", connection);
				riding_map_now();
				CHECK(!vehicle_riding_enabled(), "the next map did not take the change (connection %d)", connection);
			}
		}
		else CASE("map-on-film") {
			vehicle_riding_map_script(TRUE);
			CHECK(vehicle_riding_map() == TRUE, "the map's choice is not set");
			setting_riding = 1; test_game.cooperative_flags = FLAG(_network_game_cooperative_vehicle_riding_bit);
			vehicle_riding_map_decide();
			CHECK(vehicle_riding_enabled(), "riding is not on under the local connection");
			connection = _game_connection_film_playback;
			CHECK(!vehicle_riding_enabled(), "a map's choice rides in film playback");
		}
		else return 2;
		return 0;
	}

	if (!strncmp(case_name, "seq-", 4))
	{
		/* one host over several games, with no restart: a start (the host's) then a map (every machine's) each */
		vehicle_riding_map_initialize();
		vehicle_riding_map_reset();
		script_type = _hs_script_startup;
		game_time = 0;
		network_game_port_vehicle_riding_entry(NONE);
		CASE("seq-coop-then-silent-mp") {
			setting_riding = 1;
			riding_game(game_engine_none, FALSE);
			riding_host_start(); riding_map_now();
			CHECK(vehicle_riding_enabled(), "co-op with the option on does not ride");
			riding_game(game_engine_slayer, FALSE);
			riding_host_start(); riding_map_now();
			CHECK(!vehicle_riding_enabled(), "a silent slayer after co-op carried its riding");
			connection = _game_connection_network_client;
			riding_map_now();
			CHECK(!vehicle_riding_enabled(), "a client's own option rides a silent slayer");
		}
		else CASE("seq-riding-mp-then-coop-off") {
			setting_riding = 1;
			riding_game(game_engine_slayer, TRUE);
			riding_host_start(); riding_map_now();
			CHECK(vehicle_riding_enabled(), "a riding slayer does not ride");
			setting_riding = 0;
			riding_game(game_engine_none, FALSE);
			riding_host_start(); riding_map_now();
			CHECK(!vehicle_riding_enabled(), "co-op with the option off kept a riding slayer's riding");
		}
		else CASE("seq-riding-mp-then-silent-mp") {
			riding_game(game_engine_slayer, TRUE);
			riding_host_start(); riding_map_now();
			CHECK(vehicle_riding_enabled(), "a riding slayer does not ride");
			riding_game(game_engine_slayer, FALSE);
			riding_host_start(); riding_map_now();
			CHECK(!vehicle_riding_enabled(), "a silent slayer kept a riding slayer's riding");
		}
		else CASE("seq-dedicated-rotation") {
			static const struct { int engine, riding, expected; } rotation[4] = {
				{ game_engine_none, FALSE, TRUE }, { game_engine_slayer, FALSE, FALSE },
				{ game_engine_none, FALSE, TRUE }, { game_engine_slayer, TRUE, TRUE } };
			int entry;
			server_mode = 1; setting_riding = 1; setting_override = 0;
			for (entry = 0; entry < 4; entry++) {
				riding_game(rotation[entry].engine, rotation[entry].riding);
				riding_host_start(); riding_map_now();
				CHECK(vehicle_riding_enabled() == rotation[entry].expected, "entry %d: riding %d, expected %d", entry, vehicle_riding_enabled(), rotation[entry].expected);
			}
		}
		else CASE("seq-coop-entry-rule") {
			static const struct { short rule; int base, expected; } entries[4] = {
				{ TRUE, 0, TRUE }, { NONE, 0, FALSE }, { FALSE, 1, FALSE }, { NONE, 1, TRUE } };
			int entry;
			server_mode = 1; setting_override = 0;
			riding_game(game_engine_none, FALSE);
			for (entry = 0; entry < 4; entry++) {
				setting_riding = entries[entry].base;
				network_game_port_vehicle_riding_entry(entries[entry].rule);
				riding_host_start(); riding_map_now();
				CHECK(vehicle_riding_enabled() == entries[entry].expected, "entry %d (rule %d, base %d): riding %d, expected %d", entry, entries[entry].rule, entries[entry].base, vehicle_riding_enabled(), entries[entry].expected);
				connection = _game_connection_network_client;
				CHECK(vehicle_riding_enabled() == entries[entry].expected, "entry %d: a client rides %d, expected %d", entry, vehicle_riding_enabled(), entries[entry].expected);
			}
		}
		else CASE("seq-override-base-off") {
			server_mode = 1; setting_riding = 0; setting_override = 1;
			riding_game(game_engine_slayer, TRUE);
			riding_host_start(); riding_map_now();
			CHECK(!vehicle_riding_enabled(), "the override with the base off let a riding slayer ride");
			vehicle_riding_map_script(TRUE);
			CHECK(vehicle_riding_enabled(), "the map's choice does not win over the override");
		}
		else CASE("seq-late-joiner") {
			riding_game(game_engine_none, FALSE);
			setting_riding = 0;
			riding_host_start(); riding_map_now();
			CHECK(!vehicle_riding_enabled(), "the host rides with the option off");
			setting_riding = 1;
			connection = _game_connection_network_client;
			riding_map_now();
			CHECK(riding_local_decision && !riding_network_decision, "the stored decisions do not differ (local %d, network %d)", riding_local_decision, riding_network_decision);
			CHECK(!vehicle_riding_enabled(), "a late joiner rides on its own option");
			connection = _game_connection_network_server;
			CHECK(!vehicle_riding_enabled(), "the host rides after a joiner's map");
		}
		else return 2;
		return 0;
	}

	CASE("local-gametype")
	{
		connection = _game_connection_local;
		engine_running = TRUE;
		engine_variant.universal_variant.flags = FLAG(_game_variant_vehicle_riding_bit);
		setting_riding = 0;
		vehicle_riding_map_decide();
		CHECK(vehicle_riding_enabled(), "a local multiplayer game with the gametype's bit does not ride");
		engine_variant.universal_variant.flags = 0;
		setting_riding = 1;
		vehicle_riding_map_decide();
		CHECK(!vehicle_riding_enabled(), "a local multiplayer game without the bit rides on the option");
		engine_running = FALSE;
		vehicle_riding_map_decide();
		CHECK(vehicle_riding_enabled(), "a local game with no engine does not ride on the option");
		return 0;
	}

	if (!strncmp(case_name, "host-", 5))
	{
		struct distributed_riding_latched own, got;
		struct distributed_riding_latched host = { .5f, 7.f, .6f, .2f, 0, 2 };

		setting_damage_speed = .1; setting_teleport_step = 3.0; setting_moved_alone = .4; setting_air_grip = .3;
		setting_look_carry = 1; setting_grace = 9.0;
		connection = _game_connection_network_client;
		vehicle_riding_latch();
		CASE("host-values-win") {
			vehicle_riding_latched_get(&own);
			CHECK(own.air_grip == .3f && own.cabin_grace == 9 && own.look_carry == 1, "own values lost before the message");
			logged = 0;
			vehicle_riding_latched_received(&host);
			CHECK(logged == 1, "the values from the host not logged once");
			vehicle_riding_latched_get(&got);
			CHECK(got.damage_speed == .5f && got.teleport_step == 7.f && got.moved_alone == .6f && got.air_grip == .2f &&
				got.look_carry == 0 && got.cabin_grace == 2, "client kept its own config over the host's");
			CHECK(vehicle_riding_cabin_grace() == 2, "grace not the host's");
			vehicle_riding_latch();
			vehicle_riding_latched_get(&got);
			CHECK(got.air_grip == .2f && got.cabin_grace == 2, "a later latch took the client's config back");
			vehicle_riding_latched_forget(); vehicle_riding_latch();
			vehicle_riding_latched_get(&got);
			CHECK(got.air_grip == .3f && got.cabin_grace == 9, "host values kept past the game");
		}
		else CASE("host-no-message-keeps-own") {
			vehicle_riding_latched_get(&got);
			CHECK(got.damage_speed == .1f && got.teleport_step == 3.f && got.moved_alone == .4f && got.air_grip == .3f &&
				got.look_carry == 1 && got.cabin_grace == 9, "client lost its own values with no message");
		}
		else CASE("host-bad-value-clamped") {
			host.damage_speed = (real)NAN; host.teleport_step = -1.f; host.moved_alone = (real)INFINITY; host.air_grip = (real)NAN;
			host.look_carry = 7; host.cabin_grace = 60000;
			vehicle_riding_latched_received(&host);
			vehicle_riding_latched_get(&got);
			CHECK(got.damage_speed == 0.f, "NaN damage speed taken");
			CHECK(got.teleport_step == 4.f, "negative teleport step taken");
			CHECK(got.moved_alone == .25f, "infinite moved alone taken");
			CHECK(got.air_grip == 0.f, "NaN air grip taken");
			CHECK(got.look_carry == 1, "look carry not a boolean");
			CHECK(got.cabin_grace == 30 && vehicle_riding_cabin_grace() == 30, "grace past the maximum taken");
			host.air_grip = -.5f; host.damage_speed = -2.f; host.teleport_step = (real)NAN; host.moved_alone = -.1f;
			vehicle_riding_latched_received(&host);
			vehicle_riding_latched_get(&got);
			CHECK(got.air_grip == 0.f && got.damage_speed == 0.f && got.teleport_step == 4.f && got.moved_alone == .25f,
				"negative values taken");
		}
		else CASE("host-latch-clamped") {
			connection = _game_connection_network_server;
			setting_damage_speed = -2.0; setting_teleport_step = NAN; setting_moved_alone = -1.0;
			vehicle_riding_latch();
			vehicle_riding_latched_get(&got);
			CHECK(got.damage_speed == 0.f, "negative damage speed latched");
			CHECK(got.teleport_step == 4.f, "NaN teleport step latched");
			CHECK(got.moved_alone == .25f, "negative moved alone latched");
			setting_damage_speed = INFINITY; setting_teleport_step = -1.0; setting_moved_alone = NAN;
			vehicle_riding_latch();
			vehicle_riding_latched_get(&got);
			CHECK(got.damage_speed == 0.f && got.teleport_step == 4.f && got.moved_alone == .25f, "non-finite or negative latched");
		}
		else CASE("host-joiner-latched") {
			connection = _game_connection_network_server;
			setting_damage_speed = .9; setting_teleport_step = 8.0; setting_moved_alone = .8; setting_air_grip = .7;
			setting_look_carry = 0; setting_grace = 3.0;
			vehicle_riding_latched_get(&got);
			CHECK(got.damage_speed == .1f && got.teleport_step == 3.f && got.moved_alone == .4f && got.air_grip == .3f &&
				got.look_carry == 1 && got.cabin_grace == 9, "a mid-match config change reached the latched values");
			vehicle_riding_latched_received(&host);
			vehicle_riding_latched_get(&got);
			CHECK(got.air_grip == .3f, "host took values from the wire");
		}
		else CASE("host-local-unchanged") {
			connection = _game_connection_local;
			vehicle_riding_latched_received(&host);
			vehicle_riding_latched_get(&got);
			CHECK(got.air_grip == .3f && got.cabin_grace == 9, "a local game took values from the wire");
		}
		else return 2;
		return 0;
	}

	CASE("claim-grace")
	{
		struct vehicle_riding_claim *claim = vehicle_riding_player_claim(127);
		long tick;

		CHECK(claim && !vehicle_riding_player_claim(-1) && !vehicle_riding_player_claim(128), "claim table bounds");
		for (tick = 1; tick <= 15; tick++)
			CHECK(!vehicle_riding_claim_grace(claim, VEHICLE, NONE, VEHICLE, 3, 15), "grace expired early");
		CHECK(vehicle_riding_claim_grace(claim, VEHICLE, NONE, VEHICLE, 3, 15), "sixteenth disagreement not taken");
		CHECK(claim->taken && claim->vehicle_index == VEHICLE && claim->node_index == 3, "taken claim not recorded");
		CHECK(!vehicle_riding_claim_grace(claim, VEHICLE, 3, VEHICLE, 3, 15) && claim->disagreement == 0, "agreement not reset");
		CHECK(!vehicle_riding_claim_grace(claim, VEHICLE, 3, VEHICLE + 1, NONE, 15) && !claim->taken,
			"host changing claim did not clear take");
		vehicle_riding_latch();
		CHECK(!claim->taken && claim->disagreement == 0 && claim->vehicle_index == NONE, "latch kept claim");
		return 0;
	}

	CASE("correction-record")
	{
		struct vehicle_riding_vehicle before = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_vehicle snap = vehicle_at(VEHICLE, -.5f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_vehicle after;
		struct vehicle_riding_state state;
		real_point3d base = point(-.5f, .2f, 0.f);
		real_vector3d support, air, move, ahead, contact = { 1.f, 0.f, 0.f };

		state = riding_on(&before, base, vector(.1f, 0.f, 0.f));
		correction_write(&before, &snap);
		after = vehicle_at(VEHICLE, -.4f, 0.f, 0.f, 0.f, 11);
		CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "correction refused");
		PRECISE(step.displacement.i, -.4f);
		PRECISE(step.displacement.j, 0.f);
		PRECISE(step.displacement.k, 0.f);
		PRECISE(step.player_ticks, 1.f);
		correction_velocity(&step, .1f, 0.f, 0.f);
		vehicle_riding_support_velocity(VEHICLE, NONE, &state.velocity, VEHICLE,
			&step.tracked_deck_velocity, &zero, &support);
		PRECISE(support.i, .1f);
		PRECISE(support.j, 0.f);
		PRECISE(support.k, 0.f);
		vehicle_riding_contact_move(&step, FALSE, &after, &base, &zero, &contact, FALSE, &move, &ahead);
		PRECISE(move.i, .1f);
		PRECISE(move.j, 0.f);
		PRECISE(move.k, 0.f);
		base.x += step.displacement.i;
		air = zero;
		vehicle_riding_record(&state, &step, NULL, FALSE, &zero, &up, &base, FALSE, &air);
		PRECISE(state.velocity.i, .1f);
		PRECISE(state.velocity.j, 0.f);
		PRECISE(state.velocity.k, 0.f);
		return 0;
	}
	CASE("correction-guards")
	{
		int index;
		for (index = 0; index < 2; index++)
		{
			real jump = index ? 5.f : 3.f;
			struct vehicle_riding_vehicle before, snap, after;
			struct vehicle_riding_state state;
			real_point3d base = point(-.5f, .2f, 0.f);
			real_vector3d velocity = { 0.f, 0.f, .2f };

			vehicle_riding_latch();
			before = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
			state = riding_on(&before, base, vector(.1f, 0.f, 0.f));
			snap = vehicle_at(VEHICLE, jump, 0.f, 0.f, 0.f, 10);
			correction_write(&before, &snap);
			after = vehicle_at(VEHICLE, jump + .1f, 0.f, 0.f, 0.f, 11);
			CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "snap refused");
			CHECK(state.ticks == 60, "snap dropped rider");
			PRECISE(step.displacement.i, jump + .1f);
			correction_velocity(&step, .1f, 0.f, 0.f);
			base.x += step.displacement.i;
			vehicle_riding_record(&state, &step, NULL, FALSE, &zero, &up, &base, FALSE, &velocity);
			PRECISE(state.velocity.i, .1f);
			PRECISE(state.velocity.j, 0.f);
			PRECISE(state.velocity.k, 0.f);
			PRECISE(velocity.i, .1f);
		}
		{
			struct vehicle_riding_vehicle before, after;
			struct vehicle_riding_state state;
			real_point3d base = point(-.5f, .2f, 0.f);
			vehicle_riding_latch();
			before = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
			after = vehicle_at(VEHICLE, 5.f, 0.f, 0.f, 0.f, 11);
			state = riding_on(&before, base, zero);
			CHECK(!vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "unrecorded teleport accepted");
		}
		return 0;
	}
	CASE("correction-two-riders")
	{
		struct vehicle_riding_vehicle before = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_vehicle snap = vehicle_at(VEHICLE, -.5f, 0.f, 0.f, 0.f, 10), after;
		real_point3d a = point(-.5f, .2f, 0.f), b = a;
		struct vehicle_riding_state sa = riding_on(&before, a, zero), sb = sa;

		correction_write(&before, &snap);
		a.x -= .5f;
		CHECK(vehicle_riding_rebase(&sa, &snap, &a), "host commit not rebased");
		after = vehicle_at(VEHICLE, -.4f, 0.f, 0.f, 0.f, 11);
		CHECK(vehicle_riding_snapshot_step(&sa, &after, &a, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "rebased rider refused");
		PRECISE(step.displacement.i, .1f);
		correction_velocity(&step, .1f, 0.f, 0.f);
		CHECK(vehicle_riding_snapshot_step(&sb, &after, &b, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "other rider refused");
		PRECISE(step.displacement.i, -.4f);
		correction_velocity(&step, .1f, 0.f, 0.f);
		return 0;
	}
	CASE("correction-skipped-tick")
	{
		struct vehicle_riding_vehicle before = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_vehicle snap = vehicle_at(VEHICLE, .5f, 0.f, 0.f, .1f, 10);
		real_point3d base = point(0.f, 0.f, 0.f);
		struct vehicle_riding_state state = riding_on(&before, base, vector(.1f, .02f, .03f));

		correction_write(&before, &snap);
		CHECK(vehicle_riding_snapshot_step(&state, &snap, &base, 1.4f, .2f, TRUE, ANY_LIMIT, &step), "skipped snap refused");
		PRECISE(step.displacement.i, .5f);
		PRECISE(step.displacement.j, 0.f);
		PRECISE(step.displacement.k, 0.f);
		PRECISE(step.dyaw, .1f);
		PRECISE(step.player_ticks, 0.f);
		correction_velocity(&step, .1f, .02f, .03f);
		return 0;
	}
	CASE("correction-message-time")
	{
		struct vehicle_riding_vehicle before = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_vehicle first = vehicle_at(VEHICLE, -.2f, 0.f, 0.f, 0.f, 10), second, after;
		real_point3d base = point(-.5f, .2f, 0.f);
		struct vehicle_riding_state state = riding_on(&before, base, zero);

		correction_write(&before, &first);
		second = vehicle_at(VEHICLE, -.5f, 0.f, 0.f, 0.f, 10);
		correction_write(&first, &second);
		after = vehicle_at(VEHICLE, -.4f, 0.f, 0.f, 0.f, 11);
		CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "first tick refused");
		PRECISE(step.displacement.i, -.4f);
		correction_velocity(&step, .1f, 0.f, 0.f);
		base.x += step.displacement.i;
		{
			real_vector3d velocity = zero;
			vehicle_riding_record(&state, &step, &after, TRUE, &step.tracked_deck_velocity, &up, &base, FALSE, &velocity);
		}
		after = vehicle_at(VEHICLE, -.3f, 0.f, 0.f, 0.f, 12);
		CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "second tick refused");
		PRECISE(step.displacement.i, .1f);
		correction_velocity(&step, .1f, 0.f, 0.f);
		return 0;
	}
	CASE("correction-rotation")
	{
		int pitch, radius;
		for (pitch = 0; pitch < 2; pitch++)
		for (radius = 0; radius <= 2; radius += 2)
		{
			struct vehicle_riding_vehicle before, snap, after;
			struct vehicle_riding_state state;
			real_point3d base = point((real)radius, 0.f, 0.f);
			real dx = radius*((real)cos(.05f) - 1.f), dy = radius*(real)sin(.05f);

			vehicle_riding_latch();
			before = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
			state = riding_on(&before, base, zero);
			snap = vehicle_at(VEHICLE, 5.f, 0.f, 0.f, 0.f, 10);
			correction_write(&before, &snap);
			after = vehicle_at(VEHICLE, 5.f, 0.f, 0.f, pitch ? 0.f : .05f, 11);
			if (pitch)
			{
				after.forward = pitched(before.forward, .05f);
				after.up = pitched(before.up, .05f);
				after.body_forward = after.forward;
				after.body_up = after.up;
			}
			CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "rotated snap refused");
			PRECISE(step.displacement.i, 5.f + dx);
			correction_velocity(&step, dx, pitch ? 0.f : dy, pitch ? -dy : 0.f);
		}
		return 0;
	}
	CASE("correction-noncommuting")
	{
		struct vehicle_riding_vehicle before = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_vehicle first = vehicle_at(VEHICLE, 1.f, 0.f, 0.f, 0.f, 10), second, after;
		real_point3d base = point(2.f, 0.f, 0.f);
		struct vehicle_riding_state state = riding_on(&before, base, zero);

		correction_write(&before, &first);
		/* (the second write turns about the translated vehicle's origin, so order matters) */
		second = vehicle_at(VEHICLE, 1.f, 0.f, 0.f, .1f, 10);
		correction_write(&first, &second);
		after = second;
		after.position.x += .1f;
		after.origin = after.position;
		after.updates++;
		CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "composition refused");
		PRECISE(base.x + step.displacement.i, 1.f + 2.f*(real)cos(.1f) + .1f);
		PRECISE(base.y + step.displacement.j, 2.f*(real)sin(.1f));
		correction_velocity(&step, .1f, 0.f, 0.f);
		return 0;
	}
	CASE("correction-renormalised")
	{
		struct vehicle_riding_correction correction;
		struct vehicle_riding_vehicle before = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 0);
		struct vehicle_riding_vehicle after = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, .001f, 0);
		int index;

		vehicle_riding_correction_identity(&correction, VEHICLE);
		for (index = 0; index < 1000; index++)
			vehicle_riding_correction_compose(&correction, &before.origin, &before.forward, &before.up,
				&after.origin, &after.forward, &after.up);
		PRECISE(sqrt(dot_product3d(&correction.rotation.v, &correction.rotation.v) +
			correction.rotation.w*correction.rotation.w), 1.f);
		return 0;
	}
	CASE("correction-since-same")
	{
		struct vehicle_riding_correction correction;
		real_matrix4x3 since;

		/* A rotation on all three axes exposes the inverse multiplication's round-off;
		a yaw alone can happen to multiply back to an exact identity. */
		vehicle_riding_correction_identity(&correction, VEHICLE);
		set_real_quaternion(&correction.rotation, .1f, .2f, .3f, .9f);
		quaternion_normalize(&correction.rotation);
		correction.translation.i = 71.3f;
		correction.translation.j = -42.9f;
		correction.translation.k = 18.6f;
		vehicle_riding_correction_since(&correction, &correction, &since);
		CHECK(since.position.x == 0.f && since.position.y == 0.f && since.position.z == 0.f &&
			since.forward.i == 1.f && since.forward.j == 0.f && since.forward.k == 0.f &&
			since.left.i == 0.f && since.left.j == 1.f && since.left.k == 0.f &&
			since.up.i == 0.f && since.up.j == 0.f && since.up.k == 1.f &&
			since.scale == 1.f, "a correction since itself moved: position %.9g,%.9g,%.9g forward %.9g,%.9g,%.9g",
			since.position.x, since.position.y, since.position.z, since.forward.i, since.forward.j, since.forward.k);
		return 0;
	}
	CASE("correction-keys")
	{
		long index;
		struct vehicle_riding_vehicle before = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 0);
		struct vehicle_riding_vehicle after = vehicle_at(VEHICLE, .5f, 0.f, 0.f, 0.f, 0);
		struct vehicle_riding_mark *mark;
		real_matrix4x3 since;

		/* (every object slot and player slot, not just a small local sample) */
		for (index = 0; index < HALO_PORT_MAXIMUM_OBJECTS_PER_MAP; index++)
		{
			long salted = 0x10000000 | index;
			before.object_index = after.object_index = salted;
			correction_write(&before, &after);
			PRECISE(vehicle_riding_vehicle_correction(salted)->translation.i, .5f);
			PRECISE(vehicle_riding_vehicle_correction(salted + 0x10000)->translation.i, 0.f);
			PRECISE(vehicle_riding_vehicle_correction(salted + 0x10000)->rotation.w, 1.f);
		}
		vehicle_riding_correction_since(vehicle_riding_vehicle_correction(0x10000000),
			vehicle_riding_vehicle_correction(0x10000001), &since);
		PRECISE(since.position.x, 0.f);
		PRECISE(since.forward.i, 1.f);
		CHECK(vehicle_riding_player_mark(NONE) == NULL && vehicle_riding_player_mark(HALO_PORT_MAXIMUM_NETWORK_PLAYERS) == NULL,
			"invalid mark index accepted");
		for (index = 0; index < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; index++)
		{
			mark = vehicle_riding_player_mark((short)index);
			CHECK(mark != NULL, "missing remote player mark");
			mark->time = index + 1;
		}
		vehicle_riding_latch();
		for (index = 0; index < HALO_PORT_MAXIMUM_OBJECTS_PER_MAP; index++)
			PRECISE(vehicle_riding_vehicle_correction(0x10000000 | index)->translation.i, 0.f);
		for (index = 0; index < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; index++)
		{
			mark = vehicle_riding_player_mark((short)index);
			CHECK(mark->time == 0 && mark->correction.vehicle_index == NONE, "latch kept mark");
			PRECISE(mark->correction.rotation.w, 1.f);
		}
		return 0;
	}
	CASE("correction-identity")
	{
		struct vehicle_riding_vehicle before = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_vehicle after = vehicle_at(VEHICLE, 5.f, 0.f, 0.f, PI/2.f, 11);
		real_point3d base = point(2.f, 0.f, 0.f);
		struct vehicle_riding_state state = riding_on(&before, base, zero);

		CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "identity refused");
		PRECISE(step.displacement.i, 3.f);
		PRECISE(step.displacement.j, 2.f);
		PRECISE(step.displacement.k, 0.f);
		PRECISE(step.dyaw, PI/2.f);
		PRECISE(step.player_ticks, 1.f);
		CHECK(step.updates == 1 && !step.node_rejected && !step.over_deck, "identity step flags changed");
		PRECISE(step.node_shift, 0.f);
		PRECISE(step.node_tilt, 0.f);
		correction_velocity(&step, 3.f, 2.f, 0.f);
		return 0;
	}
	CASE("node-judged-moved")
	{
		char const *cases[] = { "node-carry", "turret-with-travel", "turret-past-limit", "limit-edge", "leg-stomp", "node-turret-test" };
		int index;
		for (index = 0; index < 6; index++)
		{
			char *args[] = { argv[0], (char *)cases[index], NULL };
			CHECK(main(2, args) == 0, "existing node case changed");
		}
		return 0;
	}

	CASE("followed-elapsed")
	{
		/* three writes at message time over two ticks, the vehicle's update count unchanged */
		struct vehicle_riding_vehicle before = followed_at(0.f, 100, .1f);
		struct vehicle_riding_vehicle after = followed_at(.2f, 102, .1f);
		real_point3d base = point(-.5f, .2f, 0.f);
		struct vehicle_riding_state state = riding_on(&before, base, zero);

		CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "followed step refused");
		PRECISE(step.player_ticks, 2.f);
		PRECISE(step.displacement.i, .2f);
		correction_velocity(&step, .1f, 0.f, 0.f);
		return 0;
	}
	CASE("followed-elapsed-zero")
	{
		struct vehicle_riding_vehicle before = followed_at(0.f, 100, .1f);
		struct vehicle_riding_vehicle after = followed_at(.2f, 100, .1f);
		real_point3d base = point(-.5f, .2f, 0.f);
		struct vehicle_riding_state state = riding_on(&before, base, vector(.05f, 0.f, 0.f));

		CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "elapsed zero refused");
		PRECISE(step.displacement.i, .2f);
		correction_velocity(&step, .05f, 0.f, 0.f);
		CHECK(isfinite(step.velocity_now.i) && isfinite(step.tracked_deck_velocity.i) && isfinite(step.body_deck_velocity.i),
			"elapsed zero divided");
		return 0;
	}
	CASE("followed-stopped")
	{
		struct vehicle_riding_vehicle before = followed_at(0.f, 100, 0.f);
		struct vehicle_riding_vehicle after = followed_at(0.f, 105, 0.f);
		real_point3d base = point(-.5f, .2f, 0.f);
		struct vehicle_riding_state state = riding_on(&before, base, vector(.1f, 0.f, 0.f));

		CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "stopped refused");
		PRECISE(step.player_ticks, 5.f);
		correction_velocity(&step, 0.f, 0.f, 0.f);
		return 0;
	}
	CASE("followed-jumper")
	{
		struct vehicle_riding_vehicle before = followed_at(0.f, 100, .1f);
		struct vehicle_riding_vehicle after = followed_at(.2f, 102, .1f);
		real_point3d base = point(-.5f, .2f, 0.f);
		struct vehicle_riding_state state = riding_on(&before, base, zero);
		real_vector3d velocity = { 0.f, 0.f, .2f };

		CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "followed step refused");
		base.x += step.displacement.i;
		vehicle_riding_record(&state, &step, NULL, FALSE, &zero, &up, &base, FALSE, &velocity);
		PRECISE(state.velocity.i, .1f);
		PRECISE(velocity.i, .1f);
		PRECISE(velocity.k, .2f);
		return 0;
	}
	CASE("followed-role-change")
	{
		struct vehicle_riding_vehicle before = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_vehicle after = followed_at(.5f, 103, .1f);
		struct vehicle_riding_vehicle later = followed_at(.7f, 105, .1f);
		real_point3d base = point(-.5f, .2f, 0.f);
		struct vehicle_riding_state state;
		real_vector3d velocity = zero;

		before.time = 100;
		state = riding_on(&before, base, vector(.05f, 0.f, 0.f));
		CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "role change refused");
		PRECISE(step.displacement.i, .5f);
		correction_velocity(&step, .05f, 0.f, 0.f);
		base.x += step.displacement.i;
		vehicle_riding_record(&state, &step, &after, TRUE, &zero, &up, &base, FALSE, &velocity);
		CHECK(state.mark.role == _vehicle_riding_role_followed && state.mark.time == 103, "record kept the old role");
		CHECK(vehicle_riding_snapshot_step(&state, &later, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "next step refused");
		PRECISE(step.player_ticks, 2.f);
		PRECISE(step.displacement.i, .2f);
		correction_velocity(&step, .1f, 0.f, 0.f);
		return 0;
	}
	CASE("followed-snap")
	{
		static real const snaps[] = { 3.f, 5.f };
		int index;

		for (index = 0; index < 2; index++)
		{
			struct vehicle_riding_vehicle before = followed_at(0.f, 100, .1f);
			struct vehicle_riding_vehicle after = followed_at(snaps[index], 101, .1f);
			real_point3d base = point(-.5f, .2f, 0.f);
			struct vehicle_riding_state state = riding_on(&before, base, vector(.05f, 0.f, 0.f));
			real_vector3d velocity = { 0.f, 0.f, .2f };

			CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "snap refused");
			CHECK(state.ticks == 60, "snap dropped rider");
			PRECISE(step.displacement.i, snaps[index]);
			correction_velocity(&step, .05f, 0.f, 0.f);
			base.x += step.displacement.i;
			vehicle_riding_record(&state, &step, NULL, FALSE, &zero, &up, &base, FALSE, &velocity);
			PRECISE(velocity.i, .05f);
		}
		{
			struct vehicle_riding_vehicle before = followed_at(0.f, 100, .1f);
			struct vehicle_riding_vehicle after = followed_at(.1f, 101, .1f);
			real_point3d base = point(-.5f, .2f, 0.f);
			struct vehicle_riding_state state = riding_on(&before, base, vector(.05f, 0.f, 0.f));

			CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "motion refused");
			correction_velocity(&step, .1f, 0.f, 0.f);
		}
		{
			struct vehicle_riding_vehicle before = followed_at(0.f, 100, 2.5f);
			struct vehicle_riding_vehicle after = followed_at(2.5f, 101, 2.5f);
			real_point3d base = point(-.5f, .2f, 0.f);
			struct vehicle_riding_state state = riding_on(&before, base, vector(.05f, 0.f, 0.f));

			CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "fast motion refused");
			correction_velocity(&step, 2.5f, 0.f, 0.f);
		}
		return 0;
	}
	CASE("followed-role-change-teleport")
	{
		int index;

		for (index = 0; index < 2; index++)
		{
			struct vehicle_riding_vehicle before = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
			struct vehicle_riding_vehicle after = followed_at(index ? 3.f : 5.f, 102, 0.f);
			real_point3d base = point(-.5f, .2f, 0.f);
			struct vehicle_riding_state state;

			before.time = 100;
			state = riding_on(&before, base, vector(.05f, 0.f, 0.f));
			if (index)
			{
				CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "3 wu refused");
				PRECISE(step.displacement.i, 3.f);
				correction_velocity(&step, .05f, 0.f, 0.f);
			}
			else
			{
				CHECK(!vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "5 wu carried");
				CHECK(state.ticks == 59, "5 wu closed the window");
			}
		}
		return 0;
	}
	CASE("followed-node")
	{
		/* a turret turning .15 as the body drives .35: .3 a tick on the body, over the limit if judged on one
		tick; the last is a role change, never judged */
		static long const elapsed[] = { 1, 2, 0, 3 };
		int index;

		for (index = 0; index < 4; index++)
		{
			struct vehicle_riding_vehicle before = node_of_body(followed_at(0.f, 100, 0.f), 2, point(0.f, 0.f, 0.f));
			struct vehicle_riding_vehicle after = node_of_body(followed_at(.35f, 100 + elapsed[index], 0.f), 2, point(.35f, 0.f, 0.f));
			real_point3d base = point(0.f, -2.f, 0.f);
			struct vehicle_riding_state state;

			if (index == 3)
				before.role = _vehicle_riding_role_simulated;
			after.forward = vector((real)cos(.15f), (real)sin(.15f), 0.f);
			state = riding_on(&before, base, zero);
			CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, NODE_LIMIT, &step), "followed turret refused");
			CHECK(step.node_rejected == (index == 0), "node judged on the wrong span (%d)", index);
			if (index == 0)
				NEAR(step.displacement.i, .35f);
			else
				NEAR(step.displacement.j, 2.f*(1.f - (real)cos(.15f)));
		}
		return 0;
	}
	CASE("carry")
	{
		struct vehicle_riding_vehicle before = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_vehicle after = vehicle_at(VEHICLE, 5.f, 0.f, 0.f, PI/2.f, 11);
		struct vehicle_riding_state state = riding_on(&before, point(2.f, 0.f, 0.f), zero);
		real_point3d base = point(2.f, 0.f, 0.f);

		CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "dropped");
		NEAR(base.x + step.displacement.i, 5.f);
		NEAR(base.y + step.displacement.j, 2.f);
		NEAR(base.z + step.displacement.k, 0.f);
		NEAR(step.dyaw, PI/2.f);
		return 0;
	}
	CASE("dyaw-vertical")
	{
		/* forward nearly straight up: a yaw of 0.1 is still 0.1, a pitch alone none */
		real_vector3d forward = vector(.0995037f, 0.f, .9950372f);
		real_vector3d upward = vector(-.9950372f, 0.f, .0995037f);
		real_vector3d yawed_forward = forward, yawed_up = upward;
		real_vector3d tilted_forward = pitched(forward, .1f), tilted_up = pitched(upward, .1f);

		vehicle_riding_turn_vector(&yawed_forward, .1f);
		vehicle_riding_turn_vector(&yawed_up, .1f);
		NEAR(vehicle_riding_yaw(&forward, &upward, &yawed_forward, &yawed_up), .1f);
		NEAR(vehicle_riding_yaw(&forward, &upward, &tilted_forward, &tilted_up), 0.f);
		return 0;
	}
	CASE("node-carry")
	{
		/* the vehicle still, its node (a turret) at (3, 0, 1) turning 0.2 about up */
		struct vehicle_riding_vehicle before = vehicle_at(VEHICLE, 3.f, 0.f, 1.f, 0.f, 10);
		struct vehicle_riding_vehicle after = vehicle_at(VEHICLE, 3.f, 0.f, 1.f, .2f, 11);
		struct vehicle_riding_state state;
		real_point3d base = point(5.f, 0.f, 1.f);

		before.node_index = after.node_index = 2;
		before.origin = after.origin = point(0.f, 0.f, 0.f);
		/* (the body still: only the turret turns, on it) */
		after.body_forward = before.body_forward;
		state = riding_on(&before, base, zero);
		CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "dropped");
		NEAR(base.x + step.displacement.i, 3.f + 2.f*(real)cos(.2f));
		NEAR(base.y + step.displacement.j, 2.f*(real)sin(.2f));
		NEAR(step.dyaw, .2f);
		return 0;
	}
	CASE("stuck")
	{
		/* a vehicle that reports a velocity but did not move, update after update */
		struct vehicle_riding_vehicle before = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_vehicle after = before;
		struct vehicle_riding_state state = riding_on(&before, point(1.f, 0.f, 0.f), zero);
		real_point3d base = point(1.f, 0.f, 0.f);
		short tick;

		after.translational_velocity = vector(.3f, 0.f, 0.f);
		for (tick = 0; tick < 30; tick++)
		{
			after.updates++;
			CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "dropped");
			NEAR(step.displacement.i, 0.f);
			NEAR(step.velocity_now.i, 0.f);
			state.vehicle_updates = after.updates;
		}
		return 0;
	}
	CASE("contact-point")
	{
		/* the base 0.2 from where the biped ended its tick: the carry is the contact point's */
		struct vehicle_riding_vehicle before = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_vehicle after = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, .1f, 11);
		struct vehicle_riding_state state = riding_on(&before, point(2.f, 0.f, 0.f), zero);
		real_point3d base = point(2.2f, 0.f, 0.f);

		CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "dropped");
		NEAR(step.displacement.i, 2.f*(real)cos(.1f) - 2.f);
		NEAR(step.displacement.j, 2.f*(real)sin(.1f));
		return 0;
	}
	CASE("guards")
	{
		struct vehicle_riding_vehicle before = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_vehicle after;
		struct vehicle_riding_state state;
		real_point3d base;

		/* moved alone 3 from where it ended its tick: no longer carried, put in the window; 0.1: kept */
		state = riding_on(&before, point(1.f, 0.f, 0.f), zero);
		after = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 11);
		base = point(4.f, 0.f, 0.f);
		CHECK(!vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "moved alone kept");
		CHECK(state.object_index == VEHICLE && state.ticks == RIDING_TICKS - 1 && state.falling == 1,
			"not put in the window");
		state = riding_on(&before, point(1.f, 0.f, 0.f), zero);
		base = point(1.1f, 0.f, 0.f);
		CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "0.1 dropped");
		/* a deck step of 5 in one update: dropped; 1: kept; 6 over two updates: kept */
		base = point(1.f, 0.f, 0.f);
		state = riding_on(&before, base, zero);
		after = vehicle_at(VEHICLE, 5.f, 0.f, 0.f, 0.f, 11);
		CHECK(!vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "5 in one kept");
		state = riding_on(&before, base, zero);
		after = vehicle_at(VEHICLE, 1.f, 0.f, 0.f, 0.f, 11);
		CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "1 in one dropped");
		state = riding_on(&before, base, zero);
		after = vehicle_at(VEHICLE, 6.f, 0.f, 0.f, 0.f, 12);
		CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "6 in two dropped");
		/* moved without an update: dropped */
		state = riding_on(&before, base, zero);
		after = vehicle_at(VEHICLE, .01f, 0.f, 0.f, 0.f, 10);
		CHECK(!vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "unupdated move kept");
		/* gone while the biped falls from it: dropped, the falling frame kept */
		state = riding_on(&before, base, vector(0.f, 0.f, -.3f));
		state.ticks = 30;
		state.falling = 1;
		CHECK(!vehicle_riding_snapshot_step(&state, NULL, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "gone kept");
		CHECK(state.object_index == NONE && state.falling == 1, "falling frame lost");
		NEAR(state.velocity.k, -.3f);
		/* in the window, not riding, outside the widened sphere: dropped */
		state = riding_on(&before, base, zero);
		state.ticks = 30;
		after = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 11);
		base = point(5.f, 0.f, 0.f);
		CHECK(!vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "far kept");
		return 0;
	}
	CASE("gate")
	{
		CHECK(vehicle_riding_enabled(), "off");
		CHECK(vehicle_riding_applies(TRUE, FALSE), "a living player does not ride");
		CHECK(!vehicle_riding_applies(FALSE, FALSE), "an AI rides");
		CHECK(!vehicle_riding_applies(TRUE, TRUE), "a dead player rides");
		/* the connection read on every call, not at the latch */
		connection = _game_connection_network_client;
		CHECK(!vehicle_riding_enabled(), "on in a network game");
		connection = _game_connection_local;
		in_editor = TRUE;
		CHECK(!vehicle_riding_enabled(), "on in the editor");
		in_editor = FALSE;
		setting_riding = 0;
		vehicle_riding_map_decide();
		vehicle_riding_latch();
		CHECK(!vehicle_riding_enabled(), "on with the setting off");
		return 0;
	}
	CASE("deferred")
	{
		struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_state state = riding_on(&vehicle, point(0.f, 0.f, 0.f), zero);

		CHECK(vehicle_riding_deferred(&state, TRUE), "a rider not deferred");
		state.ticks = 1;
		CHECK(vehicle_riding_deferred(&state, TRUE), "a biped in the window not deferred");
		CHECK(!vehicle_riding_deferred(&state, FALSE), "deferred where riding does not apply");
		state.ticks = 0;
		CHECK(!vehicle_riding_deferred(&state, TRUE), "deferred outside any window");
		return 0;
	}
	CASE("look-carry")
	{
		NEAR(vehicle_riding_look(0, TRUE, .2f), .2f);
		NEAR(impulses[0], .2f);
		NEAR(vehicle_riding_kept_dyaw(0), .2f);
		/* the next tick off the deck */
		NEAR(vehicle_riding_look(0, FALSE, .2f), 0.f);
		NEAR(vehicle_riding_kept_dyaw(0), 0.f);
		NEAR(impulses[0], .2f);
		/* the setting off: no turn (the position is carried all the same, by the step) */
		setting_look_carry = 0;
		vehicle_riding_latch();
		NEAR(vehicle_riding_look(0, TRUE, .2f), 0.f);
		NEAR(impulses[0], .2f);
		return 0;
	}
	CASE("two-local-players")
	{
		vehicle_riding_look(0, TRUE, .2f);
		vehicle_riding_look(1, TRUE, -.1f);
		NEAR(impulses[0], .2f);
		NEAR(impulses[1], -.1f);
		NEAR(vehicle_riding_kept_dyaw(0), .2f);
		NEAR(vehicle_riding_kept_dyaw(1), -.1f);
		vehicle_riding_look(1, FALSE, 0.f);
		NEAR(vehicle_riding_kept_dyaw(0), .2f);
		NEAR(vehicle_riding_kept_dyaw(1), 0.f);
		return 0;
	}
	CASE("latch")
	{
		vehicle_riding_look(1, TRUE, .2f);
		setting_riding = 0;
		vehicle_riding_map_decide();
		CHECK(!vehicle_riding_latch(), "on");
		NEAR(vehicle_riding_kept_dyaw(1), 0.f);
		setting_riding = 1;
		vehicle_riding_map_decide();
		CHECK(vehicle_riding_latch(), "off");
		return 0;
	}
	CASE("momentum")
	{
		struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_state state = riding_on(&vehicle, point(0.f, 0.f, 0.f), vector(.1f, 0.f, 0.f));
		real_point3d base = point(0.f, 0.f, 0.f);
		real_vector3d world;

		/* the tick it steps off: its own velocity plus the deck's */
		vehicle = vehicle_at(VEHICLE, .1f, 0.f, 0.f, 0.f, 11);
		world = riding_tick(&state, &vehicle, &base, vector(.15f, 0.f, 0.f), NULL, FALSE, zero);
		NEAR(world.i, .15f);
		CHECK(state.falling == 1 && state.ticks == RIDING_TICKS - 1, "not left");
		/* the next tick: no carry, the world velocity alone */
		vehicle = vehicle_at(VEHICLE, .2f, 0.f, 0.f, 0.f, 12);
		{
			real start = base.x;

			riding_tick(&state, &vehicle, &base, world, NULL, FALSE, zero);
			NEAR(base.x - start, .15f);
		}
		return 0;
	}
	CASE("no-double-count")
	{
		struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_state state = riding_on(&vehicle, point(0.f, 0.f, 0.f), vector(.1f, 0.f, 0.f));
		real_point3d base = point(0.f, 0.f, 0.f);
		real_vector3d world = vector(.1f, 0.f, 0.f);
		short tick;

		for (tick = 1; tick <= 10; tick++)
		{
			vehicle = vehicle_at(VEHICLE, .1f*tick, 0.f, 0.f, 0.f, 10 + tick);
			world = riding_tick(&state, &vehicle, &base, world, &vehicle, TRUE, zero);
		}
		NEAR(base.x, 1.f);
		NEAR(world.i, .1f);
		return 0;
	}
	CASE("spans")
	{
		/* landed before the vehicle's update in datum order: the next carry spans two */
		struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_state state;
		real_point3d base = point(0.f, 0.f, 0.f);
		real_vector3d velocity = vector(.1f, 0.f, 0.f);
		real_vector3d deck = vector(.1f, 0.f, 0.f);

		vehicle_riding_clear(&state);
		vehicle.translational_velocity = deck;
		CHECK(vehicle_riding_record(&state, NULL, &vehicle, TRUE, &deck, &up, &base, FALSE, &velocity), "no elevator clear");
		vehicle = vehicle_at(VEHICLE, .2f, 0.f, 0.f, 0.f, 12);
		CHECK(vehicle_riding_snapshot_step(&state, &vehicle, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "dropped");
		NEAR(step.velocity_now.i, .1f);
		/* leaving then: the vehicle's speed, not twice it */
		velocity = zero;
		vehicle_riding_record(&state, &step, NULL, FALSE, &zero, &up, &base, FALSE, &velocity);
		NEAR(velocity.i, .1f);
		return 0;
	}
	CASE("double-speed")
	{
		/* a vehicle without a player, 0.2 an update, an update every other tick */
		struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_state state;
		real_point3d base = point(0.f, 0.f, 0.f);
		real_vector3d velocity;
		real_vector3d deck;

		/* first landing: the deck's motion per player tick */
		vehicle.translational_velocity = vector(.2f, 0.f, 0.f);
		vehicle_riding_deck_point_velocity(&vehicle, &base, TRUE, &deck);
		NEAR(deck.i, .1f);
		vehicle_riding_clear(&state);
		velocity = deck;
		vehicle_riding_record(&state, NULL, &vehicle, TRUE, &deck, &up, &base, FALSE, &velocity);
		NEAR(state.velocity.i, .1f);
		/* an even tick (the vehicle updated): 0.1 a tick */
		vehicle = vehicle_at(VEHICLE, .2f, 0.f, 0.f, 0.f, 11);
		base.x = .2f;
		CHECK(vehicle_riding_snapshot_step(&state, &vehicle, &base, 1.4f, .2f, TRUE, ANY_LIMIT, &step), "dropped");
		NEAR(step.velocity_now.i, .1f);
		/* leaving on it */
		velocity = zero;
		vehicle_riding_record(&state, &step, NULL, FALSE, &zero, &up, &base, FALSE, &velocity);
		NEAR(velocity.i, .1f);
		/* an odd tick (no update): the frames hold, landing back gives 0.1 too */
		CHECK(vehicle_riding_snapshot_step(&state, &vehicle, &base, 1.4f, .2f, TRUE, ANY_LIMIT, &step), "dropped");
		NEAR(step.tracked_deck_velocity.i, .1f);
		velocity = zero;
		vehicle_riding_record(&state, &step, &vehicle, TRUE, &step.tracked_deck_velocity, &up, &base, FALSE, &velocity);
		NEAR(state.velocity.i, .1f);
		/* an odd tick, riding: no carry, 0.1 held */
		CHECK(vehicle_riding_snapshot_step(&state, &vehicle, &base, 1.4f, .2f, TRUE, ANY_LIMIT, &step), "dropped");
		NEAR(step.displacement.i, 0.f);
		NEAR(step.velocity_now.i, .1f);
		return 0;
	}
	CASE("landing-frame")
	{
		/* riding a deck descending 0.3 a tick, moving 0.05 across it */
		real_vector3d frame = vector(0.f, 0.f, -.3f);
		real_vector3d relative = vector(.05f, 0.f, 0.f);
		real_vector3d other = vector(0.f, 0.f, -.1f);
		real_vector3d support;
		struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_vehicle second = vehicle_at(OTHER_VEHICLE, 2.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_state state;
		real_point3d base = point(0.f, 0.f, 0.f);
		real_vector3d velocity;

		/* onto the ground: the world landing velocity */
		vehicle_riding_support_velocity(NONE, VEHICLE, &frame, NONE, &zero, &zero, &support);
		NEAR(vehicle_riding_landing_velocity(&relative, &frame, &support, &up), .3f);
		/* onto a second vehicle descending 0.1: relative to it */
		vehicle_riding_support_velocity(OTHER_VEHICLE, VEHICLE, &frame, NONE, &zero, &other, &support);
		NEAR(vehicle_riding_landing_velocity(&relative, &frame, &support, &up), .2f);
		/* back on the ridden deck: as before */
		vehicle_riding_support_velocity(VEHICLE, VEHICLE, &frame, NONE, &zero, &other, &support);
		NEAR(vehicle_riding_landing_velocity(&relative, &frame, &support, &up), 0.f);
		/* recorded on the ground: no part into it left, so no second landing */
		state = riding_on(&vehicle, base, frame);
		vehicle = vehicle_at(VEHICLE, 0.f, 0.f, -.3f, 0.f, 11);
		CHECK(vehicle_riding_snapshot_step(&state, &vehicle, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "dropped");
		velocity = relative;
		vehicle_riding_record(&state, &step, NULL, TRUE, &zero, &up, &base, FALSE, &velocity);
		NEAR(velocity.k, 0.f);
		NEAR(velocity.i, .05f);
		/* recorded on the second vehicle: only its own descent left */
		state = riding_on(&vehicle, base, frame);
		vehicle = vehicle_at(VEHICLE, 0.f, 0.f, -.6f, 0.f, 12);
		CHECK(vehicle_riding_snapshot_step(&state, &vehicle, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "dropped");
		velocity = relative;
		vehicle_riding_record(&state, &step, &second, TRUE, &other, &up, &base, FALSE, &velocity);
		NEAR(velocity.k, -.1f);
		CHECK(state.object_index == OTHER_VEHICLE, "not riding the second vehicle");
		NEAR(state.velocity.k, -.1f);
		/* landing back on the tracked deck (in its window, not riding): its tracked velocity */
		{
			real_vector3d tracked = vector(0.f, 0.f, -.2f);

			vehicle_riding_support_velocity(VEHICLE, NONE, &zero, VEHICLE, &tracked, &other, &support);
			NEAR(support.k, -.2f);
		}
		return 0;
	}
	CASE("seat-exit-onto-deck")
	{
		/* a seat exit leaves the biped with the vehicle's velocity: landing on the
		same deck, it stands still on it */
		struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_state state;
		real_point3d base = point(0.f, 0.f, 1.f);
		real_vector3d deck = vector(.3f, 0.f, 0.f);
		real_vector3d velocity = deck;
		real_vector3d relative;
		real_vector3d frame;

		vehicle.translational_velocity = deck;
		vehicle_riding_clear(&state);
		vehicle_riding_record(&state, NULL, &vehicle, TRUE, &deck, &up, &base, FALSE, &velocity);
		vehicle_riding_seed(&state, &velocity, &relative, &frame);
		NEAR(relative.i, 0.f);
		return 0;
	}
	CASE("refresh")
	{
		/* a deck accelerating upward: stepping off keeps this tick's deck velocity */
		struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_state state = riding_on(&vehicle, point(0.f, 0.f, 0.f), vector(0.f, 0.f, .1f));
		real_point3d base = point(0.f, 0.f, 0.f);
		real_vector3d velocity = zero;

		vehicle = vehicle_at(VEHICLE, 0.f, 0.f, .2f, 0.f, 11);
		CHECK(vehicle_riding_snapshot_step(&state, &vehicle, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "dropped");
		vehicle_riding_record(&state, &step, NULL, FALSE, &zero, &up, &base, FALSE, &velocity);
		NEAR(state.velocity.k, .2f);
		return 0;
	}
	CASE("elevator")
	{
		struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_state state;
		real_point3d base = point(0.f, 0.f, 0.f);
		real_vector3d velocity = zero;

		vehicle_riding_clear(&state);
		CHECK(vehicle_riding_record(&state, NULL, &vehicle, TRUE, &zero, &up, &base, FALSE, &velocity),
			"riding does not clear the elevator");
		vehicle_riding_clear(&state);
		CHECK(!vehicle_riding_record(&state, NULL, NULL, TRUE, &zero, &up, &base, FALSE, &velocity),
			"the ground clears the elevator");
		return 0;
	}
	CASE("local-frame")
	{
		/* a point into a yawed and pitched vehicle's frame and back */
		real_vector3d forward = pitched(vector((real)cos(.3f), (real)sin(.3f), 0.f), .2f);
		real_vector3d upward = pitched(up, .2f);
		real_point3d origin = point(1.f, 2.f, 3.f);
		real_point3d world = point(2.5f, -1.f, 4.f);
		real_point3d local, back;

		vehicle_riding_point_to_local(&origin, &forward, &upward, &world, &local);
		vehicle_riding_point_to_world(&origin, &forward, &upward, &local, &back);
		CHECK(fabs(local.y) > .1, "the case needs a point to the side");
		NEAR(back.x, 2.5f);
		NEAR(back.y, -1.f);
		NEAR(back.z, 4.f);
		/* 30 ticks in the air off a moving deck: its place on the body, taken
		the tick it leaves the floor, stays */
		{
			struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
			struct vehicle_riding_state state = riding_on(&vehicle, point(1.f, 1.f, 0.f), vector(.1f, 0.f, 0.f));
			real_point3d base = point(1.f, 1.f, 0.f);
			real_point3d kept = point(0.f, 0.f, 0.f);
			real_vector3d velocity = zero;
			short tick;

			for (tick = 1; tick <= 30; tick++)
			{
				vehicle = vehicle_at(VEHICLE, .1f*tick, 0.f, 0.f, 0.f, 10 + tick);
				base.z += .05f;
				CHECK(vehicle_riding_snapshot_step(&state, &vehicle, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "dropped");
				vehicle_riding_record(&state, &step, NULL, FALSE, &zero, &up, &base, FALSE, &velocity);
				if (tick == 1)
					kept = state.local_point;
			}
			NEAR(kept.x, .9f);
			NEAR(state.local_point.x, kept.x);
			NEAR(state.local_point.y, kept.y);
			NEAR(state.local_point.z, kept.z);
		}
		return 0;
	}
	CASE("node-change")
	{
		/* from the bed (node 1) onto the turret (node 2) of the same vehicle */
		struct vehicle_riding_vehicle bed = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_vehicle turret = vehicle_at(VEHICLE, 1.f, 0.f, 1.f, .5f, 10);
		struct vehicle_riding_state state;
		real_point3d base = point(1.5f, .5f, 1.f);
		real_vector3d velocity = zero;

		bed.node_index = 1;
		turret.node_index = 2;
		turret.origin = bed.origin;
		vehicle_riding_clear(&state);
		vehicle_riding_record(&state, NULL, &bed, TRUE, &zero, &up, &base, FALSE, &velocity);
		CHECK(state.node_index == 1, "node %d, not the bed's", state.node_index);
		bed.updates = turret.updates = 11;
		CHECK(vehicle_riding_snapshot_step(&state, &bed, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "dropped");
		vehicle_riding_record(&state, &step, &turret, TRUE, &zero, &up, &base, FALSE, &velocity);
		CHECK(state.node_index == 2, "node %d, not the turret's", state.node_index);
		/* both still: no carry, no jump from the change of frame */
		turret.updates = 12;
		CHECK(vehicle_riding_snapshot_step(&state, &turret, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "dropped");
		NEAR(step.displacement.i, 0.f);
		NEAR(step.displacement.j, 0.f);
		NEAR(step.displacement.k, 0.f);
		return 0;
	}
	CASE("suspended")
	{
		/* a suspended rider (a cutscene holds it): the carried place stands and its velocity is the deck's */
		struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_state state = riding_on(&vehicle, point(0.f, 0.f, 0.f), vector(.1f, 0.f, 0.f));
		real_point3d base = point(0.f, 0.f, 0.f);
		real_vector3d velocity = vector(.5f, 0.f, 0.f);

		vehicle = vehicle_at(VEHICLE, .1f, 0.f, 0.f, 0.f, 11);
		CHECK(vehicle_riding_snapshot_step(&state, &vehicle, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "dropped");
		vehicle_riding_record(&state, &step, &vehicle, TRUE, &zero, &up, &base, TRUE, &velocity);
		NEAR(velocity.i, .1f);
		return 0;
	}
	CASE("falling")
	{
		/* a deck descending 2 a tick, faster than any falling limit */
		struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_state state = riding_on(&vehicle, point(0.f, 0.f, 0.f), vector(0.f, 0.f, -2.f));
		real_point3d base = point(0.f, 0.f, 0.f);
		real_vector3d velocity;
		short tick;

		NEAR(vehicle_riding_falling_velocity(&state, -2.05f), -.05f);
		/* the tick it steps off: relative */
		vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 11);
		CHECK(vehicle_riding_snapshot_step(&state, &vehicle, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "dropped");
		step.velocity_now = step.body_deck_velocity = vector(0.f, 0.f, -2.f);
		velocity = zero;
		vehicle_riding_record(&state, &step, NULL, FALSE, &zero, &up, &base, FALSE, &velocity);
		NEAR(vehicle_riding_falling_velocity(&state, -2.05f), -.05f);
		/* 90 ticks later, fallen far below the deck and out of its sphere (in
		the air the window holds; the sphere ends it), still in the air: relative */
		for (tick = 0; tick < 90; tick++)
		{
			boolean valid;

			vehicle.updates++;
			base.z -= .1f;
			valid = state.ticks > 0 && vehicle_riding_snapshot_step(&state, &vehicle, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step);
			vehicle_riding_record(&state, valid ? &step : NULL, NULL, FALSE, &zero, &up, &base, FALSE, &velocity);
		}
		CHECK(state.ticks == 0 && state.falling == 1, "the falling frame did not outlast the window");
		NEAR(vehicle_riding_falling_velocity(&state, -2.05f), -.05f);
		base = point(0.f, 0.f, 0.f);
		/* landed on the ground beside a rising deck, in its window: the world's */
		state = riding_on(&vehicle, base, vector(0.f, 0.f, 2.f));
		state.ticks = 30;
		state.falling = 1;
		vehicle.updates++;
		CHECK(vehicle_riding_snapshot_step(&state, &vehicle, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "dropped");
		vehicle_riding_record(&state, &step, NULL, TRUE, &zero, &up, &base, FALSE, &velocity);
		NEAR(vehicle_riding_falling_velocity(&state, -1.f), -1.f);
		/* landed beside a descending deck, in its window: the world's too */
		state = riding_on(&vehicle, base, vector(0.f, 0.f, -2.f));
		state.ticks = 30;
		state.falling = 1;
		vehicle.updates++;
		CHECK(vehicle_riding_snapshot_step(&state, &vehicle, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "dropped");
		vehicle_riding_record(&state, &step, NULL, TRUE, &zero, &up, &base, FALSE, &velocity);
		NEAR(vehicle_riding_falling_velocity(&state, -1.f), -1.f);
		/* off a deck rising 0.1 a tick: never stricter than the world */
		state = riding_on(&vehicle, base, vector(0.f, 0.f, .1f));
		state.ticks = 30;
		state.falling = 1;
		NEAR(vehicle_riding_falling_velocity(&state, -1.f), -1.f);
		return 0;
	}
	CASE("jump-deck-frame")
	{
		/* the jump as biped_jump takes it, above the frame (jump velocity 0.2) */
		real decks[2] = { .3f, -.3f };
		short index;

		for (index = 0; index < 2; index++)
		{
			struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
			struct vehicle_riding_state state = riding_on(&vehicle, point(0.f, 0.f, 0.f), vector(0.f, 0.f, decks[index]));
			real_vector3d velocity = vector(0.f, 0.f, decks[index]);
			real_vector3d jump_frame;
			real upward;

			vehicle_riding_jump_frame(&state, &jump_frame);
			velocity.k -= jump_frame.k;
			upward = velocity.k;
			if (upward < .2f)
				velocity.k += .2f - upward;
			velocity.k += jump_frame.k;
			NEAR(velocity.k - decks[index], .2f);
		}
		/* stepped onto a rising deck this tick (recorded), jumping in the same tick */
		{
			struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
			struct vehicle_riding_state state;
			real_point3d base = point(0.f, 0.f, 0.f);
			real_vector3d deck = vector(0.f, 0.f, .3f);
			real_vector3d velocity = zero;
			real_vector3d jump_frame;

			vehicle_riding_clear(&state);
			vehicle_riding_record(&state, NULL, &vehicle, TRUE, &deck, &up, &base, FALSE, &velocity);
			vehicle_riding_jump_frame(&state, &jump_frame);
			NEAR(jump_frame.k, .3f);
		}
		return 0;
	}
	CASE("contact-record")
	{
		struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_state state = riding_on(&vehicle, point(0.f, 0.f, 0.f), zero);
		real_point3d centre = point(0.f, 0.f, 0.f);
		real_point3d beside = point(2.f, 0.f, 0.f);
		real_point3d across = point(0.f, 2.f, 0.f);
		real_vector3d side = vector(0.f, 1.f, 0.f);
		real_vector3d deck;
		real_vector3d velocity;
		real_vector3d normal;
		real_point3d base;

		CHECK(vehicle_riding_collision_window(&state, TRUE, VEHICLE) == _vehicle_riding_collision_ignore, "rider hit");
		state.ticks = 30;
		CHECK(vehicle_riding_collision_window(&state, TRUE, VEHICLE) == _vehicle_riding_collision_judge,
			"window not judged");
		/* the vehicle under it, however fast: resolved, never the original;
		the normal the contact plane's */
		velocity = vector(0.f, 0.f, -.3f);
		deck = vector(.6f, .1f, .2f);
		CHECK(vehicle_riding_collision_contact(&state, VEHICLE, TRUE, &centre, &beside, &up, &deck, .25f, &velocity,
			&normal) == _vehicle_riding_collision_resolved, "the deck under it hit it");
		NEAR(normal.i, 0.f);
		NEAR(normal.k, 1.f);
		/* only the vehicle it left is the one under it: another as fast, the original */
		CHECK(vehicle_riding_collision_contact(&state, OTHER_VEHICLE, TRUE, &centre, &beside, &up, &deck, .25f,
			&velocity, &normal) == _vehicle_riding_collision_original, "another vehicle spared as the deck under it");
		/* the vehicle it left, not under it: closing faster than the damage
		speed along the centres, the original, however its plane faces */
		velocity = zero;
		deck = vector(.5f, 0.f, 0.f);
		CHECK(vehicle_riding_collision_contact(&state, VEHICLE, FALSE, &centre, &beside, &side, &deck, .25f, &velocity,
			&normal) == _vehicle_riding_collision_original, "a fast hit spared");
		/* at the damage speed: resolved; no plane, the normal the centres' way */
		deck = vector(.25f, 0.f, 0.f);
		CHECK(vehicle_riding_collision_contact(&state, VEHICLE, FALSE, &centre, &beside, NULL, &deck, .25f, &velocity,
			&normal) == _vehicle_riding_collision_resolved, "hit at the damage speed");
		NEAR(normal.i, 1.f);
		NEAR(normal.k, 0.f);
		/* faster than the vehicle along the push: resolved */
		velocity = vector(.4f, 0.f, 0.f);
		deck = vector(.1f, 0.f, 0.f);
		CHECK(vehicle_riding_collision_contact(&state, VEHICLE, FALSE, &centre, &beside, NULL, &deck, .25f, &velocity,
			&normal) == _vehicle_riding_collision_resolved, "hit while outrunning it");
		/* falling past its side: no closing along the push */
		velocity = vector(0.f, 0.f, -2.f);
		deck = zero;
		CHECK(vehicle_riding_collision_contact(&state, VEHICLE, FALSE, &centre, &beside, NULL, &deck, .25f, &velocity,
			&normal) == _vehicle_riding_collision_resolved, "a fall past its side was a hit");
		/* a vehicle dropping onto a biped below it: closing */
		{
			real_point3d below = point(0.f, 0.f, -2.f);

			velocity = zero;
			deck = vector(0.f, 0.f, -.5f);
			CHECK(vehicle_riding_collision_contact(&state, VEHICLE, FALSE, &centre, &below, NULL, &deck, .25f,
				&velocity, &normal) == _vehicle_riding_collision_original, "dropped onto, spared");
		}
		/* a biped above the vehicle's centre falling at 0.3 a tick, the
		vehicle 0.1 sideways: its own fall does not close on the vehicle */
		{
			real_point3d over = point(.5f, 0.f, 2.f);

			velocity = vector(0.f, 0.f, -.3f);
			deck = vector(.1f, 0.f, 0.f);
			CHECK(vehicle_riding_collision_contact(&state, VEHICLE, FALSE, &centre, &over, NULL, &deck, .25f,
				&velocity, &normal) == _vehicle_riding_collision_resolved, "its own fall was a hit");
		}
		/* a vehicle at 0.8 a tick touching it with its front face while the
		centres' way is 75 degrees off its travel (0.21 along it, under the
		damage speed): the original, along the face's normal */
		{
			real_point3d oblique = point(2.f*(real)cos(75.f*PI/180.f), 2.f*(real)sin(75.f*PI/180.f), 0.f);
			real_vector3d front = vector(1.f, 0.f, 0.f);

			velocity = zero;
			deck = vector(.8f, 0.f, 0.f);
			CHECK(vehicle_riding_collision_contact(&state, VEHICLE, FALSE, &centre, &oblique, &front, &deck, .25f,
				&velocity, &normal) == _vehicle_riding_collision_original, "a fast touch on an oblique face resolved");
		}
		/* a side across the vehicle's travel at 0.5 a tick, brushing past a
		biped beside it: resolved, the normal the plane's */
		velocity = zero;
		deck = vector(.5f, 0.f, 0.f);
		CHECK(vehicle_riding_collision_contact(&state, VEHICLE, FALSE, &centre, &across, &side, &deck, .25f, &velocity,
			&normal) == _vehicle_riding_collision_resolved, "a side brushing past was a hit");
		NEAR(normal.j, 1.f);
		/* the centres at one point and no plane: resolved, no NaN */
		velocity = vector(.1f, .2f, .3f);
		deck = vector(.2f, 0.f, 0.f);
		CHECK(vehicle_riding_collision_contact(&state, VEHICLE, FALSE, &centre, &centre, NULL, &deck, .25f, &velocity,
			&normal) == _vehicle_riding_collision_resolved, "no direction was a hit");
		NEAR(normal.i, 0.f);
		NEAR(normal.j, 0.f);
		NEAR(normal.k, 0.f);
		/* the convoy: riding the vehicle, another beside it judged the same way */
		state = riding_on(&vehicle, point(0.f, 0.f, 0.f), zero);
		velocity = zero;
		deck = vector(.5f, 0.f, 0.f);
		CHECK(vehicle_riding_collision_contact(&state, OTHER_VEHICLE, FALSE, &centre, &beside, NULL, &deck, .25f,
			&velocity, &normal) == _vehicle_riding_collision_original, "a convoy's fast hit spared");
		deck = vector(.1f, 0.f, 0.f);
		CHECK(vehicle_riding_collision_contact(&state, OTHER_VEHICLE, FALSE, &centre, &beside, NULL, &deck, .25f,
			&velocity, &normal) == _vehicle_riding_collision_resolved, "a convoy's slow touch was a hit");
		/* the deck's velocity at the biped: a large deck turning 0.05 a tick,
		4 from its origin, moves 0.2 there */
		vehicle.angular_velocity = vector(0.f, 0.f, .05f);
		base = point(4.f, 0.f, 0.f);
		vehicle_riding_deck_point_velocity(&vehicle, &base, FALSE, &deck);
		NEAR(deck.j, .2f);
		return 0;
	}
	CASE("claim")
	{
		/* the best hit is the other Warthog's deck (0), the ridden one's deck is hit too (1) */
		long objects[3] = { OTHER_VEHICLE, VEHICLE, NONE };
		real normal_ks[3] = { 1.f, .95f, 1.f };

		CHECK(vehicle_riding_claim(VEHICLE, 3, objects, normal_ks, .7f, 0) == 1, "the ridden deck lost its rider");
		/* the ridden vehicle hit only on a wall: the best takes over */
		normal_ks[1] = .2f;
		CHECK(vehicle_riding_claim(VEHICLE, 3, objects, normal_ks, .7f, 0) == 0, "a wall kept the rider");
		/* not riding: the best */
		normal_ks[1] = .95f;
		CHECK(vehicle_riding_claim(NONE, 3, objects, normal_ks, .7f, 0) == 0, "claimed while not riding");
		/* the best already the ridden one */
		CHECK(vehicle_riding_claim(VEHICLE, 3, objects, normal_ks, .7f, 1) == 1, "the best ridden deck lost");
		/* no best hit at all */
		CHECK(vehicle_riding_claim(VEHICLE, 3, objects, normal_ks, .7f, NONE) == NONE, "claimed with no best");
		/* the ridden deck hit exactly at the limit still holds the rider */
		normal_ks[1] = .7f;
		CHECK(vehicle_riding_claim(VEHICLE, 3, objects, normal_ks, .7f, 0) == 1, "the limit lost the rider");
		return 0;
	}
	CASE("collision-others")
	{
		struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_state state;

		/* never rode */
		vehicle_riding_clear(&state);
		CHECK(vehicle_riding_collision_window(&state, TRUE, VEHICLE) == _vehicle_riding_collision_original, "pedestrian");
		/* in another vehicle's window */
		state = riding_on(&vehicle, point(0.f, 0.f, 0.f), zero);
		state.ticks = 30;
		CHECK(vehicle_riding_collision_window(&state, TRUE, OTHER_VEHICLE) == _vehicle_riding_collision_original,
			"another vehicle's window");
		/* riding where riding does not apply (an AI, a network game) */
		CHECK(vehicle_riding_collision_window(&state, FALSE, VEHICLE) == _vehicle_riding_collision_original, "AI");
		/* riding one Warthog, a second one alongside: never as a pedestrian */
		state = riding_on(&vehicle, point(0.f, 0.f, 0.f), zero);
		CHECK(vehicle_riding_collision_window(&state, TRUE, OTHER_VEHICLE) == _vehicle_riding_collision_judge,
			"a second vehicle treats its rider as a pedestrian");
		return 0;
	}
	CASE("run-jump-shared")
	{
		real run = (real)sqrt(2.25f*2.25f + 1.75f*1.75f)/30.f;
		real expected = (real)sqrt(run*run + .11f*.11f);

		setting_damage_speed = 0.0;
		vehicle_riding_latch();
		CHECK(fabs(vehicle_riding_run_jump_speed(TRUE, 2.25f, 2.f, 1.75f, FALSE, 1.f, 1.f, .11f) - expected) < 1e-6f,
			"run and jump speed differs");
		CHECK(vehicle_riding_damage_speed(TRUE, 2.25f, 2.f, 1.75f, FALSE, 1.f, 1.f, .11f) == 2.f*expected,
			"damage speed differs from twice run and jump speed");
		return 0;
	}
	CASE("rebase")
	{
		struct vehicle_riding_vehicle before = node_of_body(vehicle_at(VEHICLE, 1.f, 2.f, 3.f, 0.f, 10), 2,
			point(0.f, 0.f, 0.f));
		struct vehicle_riding_vehicle after = node_of_body(vehicle_at(VEHICLE, 1.4f, 2.f, 3.f, 0.f, 11), 2,
			point(.4f, 0.f, 0.f));
		struct vehicle_riding_vehicle other = after;
		struct vehicle_riding_state state = riding_on(&before, point(1.2f, 2.3f, 3.5f), vector(.02f, .03f, .04f));
		struct vehicle_riding_state saved;
		real_point3d base = point(1.7f, 2.3f, 3.5f);
		real_point3d local;

		state.falling = 1;
		saved = state;
		CHECK(vehicle_riding_rebase(&state, &after, &base), "rebase refused its own node");
		CHECK(!memcmp(&state.vehicle_position, &after.position, sizeof(real_point3d)), "snapshot position old");
		CHECK(!memcmp(&state.vehicle_forward, &after.forward, sizeof(real_vector3d)), "snapshot forward old");
		CHECK(!memcmp(&state.vehicle_up, &after.up, sizeof(real_vector3d)), "snapshot up old");
		CHECK(!memcmp(&state.body_position, &after.origin, sizeof(real_point3d)), "body position old");
		CHECK(!memcmp(&state.body_forward, &after.body_forward, sizeof(real_vector3d)), "body forward old");
		CHECK(!memcmp(&state.body_up, &after.body_up, sizeof(real_vector3d)), "body up old");
		CHECK(state.vehicle_updates == after.updates, "snapshot updates old");
		vehicle_riding_point_to_local(&after.position, &after.forward, &after.up, &base, &local);
		CHECK(distance3d(&state.local_point, &local) < 1e-5f, "local place old");
		CHECK(state.ticks == saved.ticks && state.falling == saved.falling &&
			!memcmp(&state.velocity, &saved.velocity, sizeof(real_vector3d)), "rebase changed frame or window");
		CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "next step refused");
		CHECK(magnitude3d(&step.displacement) < 1e-5f, "next step carried twice");
		saved = state;
		other.object_index = OTHER_VEHICLE;
		CHECK(!vehicle_riding_rebase(&state, &other, &base), "other object accepted");
		CHECK(!memcmp(&state, &saved, sizeof(state)), "other object changed state");
		other = after;
		other.node_index = 3;
		CHECK(!vehicle_riding_rebase(&state, &other, &base), "other node accepted");
		CHECK(!memcmp(&state, &saved, sizeof(state)), "other node changed state");
		CHECK(!vehicle_riding_rebase(&state, NULL, &base), "missing vehicle accepted");
		CHECK(!memcmp(&state, &saved, sizeof(state)), "missing vehicle changed state");
		return 0;
	}
	CASE("write-carry-decision")
	{
		struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_state state = riding_on(&vehicle, point(0.f, 0.f, 1.f), zero);

		state.ticks = 0;
		state.object_index = OTHER_VEHICLE;
		CHECK(vehicle_riding_write_carries(&state, VEHICLE, FALSE, TRUE, .066f), "a rider in no window not carried");
		state.ticks = RIDING_TICKS - 1;
		state.object_index = VEHICLE;
		CHECK(vehicle_riding_write_carries(&state, VEHICLE, FALSE, TRUE, .066f), "a rider in the vehicle's window not carried");
		CHECK(!vehicle_riding_write_carries(&state, VEHICLE, FALSE, FALSE, .066f), "a rider not touching carried");
		CHECK(!vehicle_riding_write_carries(&state, VEHICLE, TRUE, TRUE, .066f), "a touch from before the write carried");
		CHECK(!vehicle_riding_write_carries(&state, VEHICLE, FALSE, TRUE, .2501f), "a write of .2501 carried");
		CHECK(vehicle_riding_write_carries(&state, VEHICLE, FALSE, TRUE, .25f), "a write of .25 not carried");
		state.object_index = OTHER_VEHICLE;
		CHECK(!vehicle_riding_write_carries(&state, VEHICLE, FALSE, TRUE, .066f), "another vehicle's window rider carried");
		state.ticks = RIDING_TICKS;
		state.object_index = VEHICLE;
		CHECK(!vehicle_riding_write_carries(&state, VEHICLE, FALSE, TRUE, .066f), "a riding rider carried (its step does)");
		return 0;
	}
	CASE("write-carry-window-next-step")
	{
		struct vehicle_riding_vehicle p0 = vehicle_at(VEHICLE, 1.2f, 2.1f, .8f, .3f, 10);
		struct vehicle_riding_vehicle p1 = p0;
		struct vehicle_riding_vehicle p2;
		struct vehicle_riding_state state;
		real_point3d b = point(1.5f, 2.7f, 1.3f);
		real_point3d local, carried, expected;
		real_vector3d saved_velocity, motion, move, ahead, normal = { .6f, .0f, .8f };
		real into, wanted;
		real yaw = .02f;

		/* the write W: a shift and a 0.05 rad pitch of the pose, recorded as the netcode records it */
		p1.position = p1.origin = p1.bounding_sphere_center = point(1.23f, 2.11f, .83f);
		p1.forward = p1.body_forward = pitched(p0.forward, .05f);
		p1.up = p1.body_up = pitched(p0.up, .05f);
		correction_write(&p0, &p1);
		state = riding_on(&p0, b, vector(.05f, .02f, .01f));
		state.ticks = RIDING_TICKS - 1;
		saved_velocity = state.velocity;
		vehicle_riding_point_to_local(&p0.position, &p0.forward, &p0.up, &b, &local);
		vehicle_riding_point_to_world(&p1.position, &p1.forward, &p1.up, &local, &carried);
#ifndef WRITE_CARRY_NO_REBASE
		CHECK(vehicle_riding_rebase(&state, &p1, &carried), "rebase refused");
#endif
		/* one update of the vehicle's own motion D: a turn about its up and a drive */
		p2 = p1;
		p2.updates = 11;
		p2.position = p2.origin = p2.bounding_sphere_center = point(p1.position.x + .03f, p1.position.y - .01f, p1.position.z + .002f);
		p2.forward = p2.body_forward = vector(p1.forward.i*(real)cos(yaw) - p1.forward.j*(real)sin(yaw),
			p1.forward.i*(real)sin(yaw) + p1.forward.j*(real)cos(yaw), p1.forward.k);
		p2.up = p2.body_up = vector(p1.up.i*(real)cos(yaw) - p1.up.j*(real)sin(yaw),
			p1.up.i*(real)sin(yaw) + p1.up.j*(real)cos(yaw), p1.up.k);
		/* D's motion at W(b): the carried place, on the deck as it was after the write, on the deck as it is now */
		vehicle_riding_point_to_local(&p1.position, &p1.forward, &p1.up, &carried, &local);
		vehicle_riding_point_to_world(&p2.position, &p2.forward, &p2.up, &local, &expected);
		vector_from_points3d(&carried, &expected, &motion);
		CHECK(vehicle_riding_snapshot_step(&state, &p2, &carried, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "step refused");
		PRECISE(step.player_ticks, 1.f);
		CHECK(fabs(step.displacement.i - motion.i) < 1e-6 && fabs(step.displacement.j - motion.j) < 1e-6 &&
			fabs(step.displacement.k - motion.k) < 1e-6, "the step's displacement has a write term: (%.9g %.9g %.9g), not (%.9g %.9g %.9g)",
			step.displacement.i, step.displacement.j, step.displacement.k, motion.i, motion.j, motion.k);
		CHECK(fabs(step.body_deck_velocity.i*step.player_ticks - motion.i) < 1e-6 &&
			fabs(step.body_deck_velocity.j*step.player_ticks - motion.j) < 1e-6 &&
			fabs(step.body_deck_velocity.k*step.player_ticks - motion.k) < 1e-6,
			"the deck velocity has a write term: (%.9g %.9g %.9g), not (%.9g %.9g %.9g)",
			step.body_deck_velocity.i, step.body_deck_velocity.j, step.body_deck_velocity.k, motion.i, motion.j, motion.k);
		CHECK(!memcmp(&state.velocity, &saved_velocity, sizeof(saved_velocity)), "the carry and rebase changed the air frame");
		into = vehicle_riding_contact_move(&step, FALSE, &p2, &carried, &zero, &normal, FALSE, &move, &ahead);
		wanted = (expected.x - carried.x)*normal.i + (expected.y - carried.y)*normal.j + (expected.z - carried.z)*normal.k;
		if (wanted < 0.f)
			wanted = 0.f;
		CHECK(fabs(into - wanted) < 1e-6, "a touch's into %.9g, not %.9g from the vehicle's own motion", into, wanted);
		return 0;
	}
	CASE("window-base-matches-step")
	{
		/* a window anchored to the deck as it was; the deck since driven .217 and turned .05 */
		struct vehicle_riding_vehicle before = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_vehicle after = vehicle_at(VEHICLE, .217f, 0.f, 0.f, .05f, 11);
		struct vehicle_riding_vehicle other = vehicle_at(OTHER_VEHICLE, .217f, 0.f, 0.f, .05f, 11);
		real_point3d base = point(-.85f, .1f, .5f);
		struct vehicle_riding_state state = riding_on(&before, base, vector(.217f, 0.f, 0.f));
		real_point3d origin;
		real_point3d local;
		real_point3d carried;

		state.ticks = RIDING_TICKS - 1;
		CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "window refused");
		vehicle_riding_window_base(&state, &after, &base, &origin);
		PRECISE(origin.x - base.x, step.tracked_deck_velocity.i*step.player_ticks);
		PRECISE(origin.y - base.y, step.tracked_deck_velocity.j*step.player_ticks);
		PRECISE(origin.z - base.z, step.tracked_deck_velocity.k*step.player_ticks);
		vehicle_riding_point_to_local(&before.position, &before.forward, &before.up, &base, &local);
		vehicle_riding_point_to_world(&after.position, &after.forward, &after.up, &local, &carried);
		PRECISE(origin.x, carried.x);
		PRECISE(origin.y, carried.y);
		PRECISE(origin.z, carried.z);
		vehicle_riding_window_base(&state, &other, &base, &origin);
		CHECK(origin.x == base.x && origin.y == base.y && origin.z == base.z, "another vehicle's motion carried the base");
		return 0;
	}
	CASE("window-base-write-left-out")
	{
		/* a deck written .5 back under a biped in the air (no touch, so no carry), then driven .1 itself */
		struct vehicle_riding_vehicle before = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_vehicle snap = vehicle_at(VEHICLE, -.5f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_vehicle after;
		real_point3d base = point(.3f, .2f, .5f);
		struct vehicle_riding_state state = riding_on(&before, base, vector(.1f, 0.f, 0.f));
		real_point3d origin;

		state.ticks = RIDING_TICKS - 1;
		correction_write(&before, &snap);
		after = vehicle_at(VEHICLE, -.4f, 0.f, 0.f, 0.f, 11);
		vehicle_riding_window_base(&state, &after, &base, &origin);
		PRECISE(origin.x, base.x + .1f);
		PRECISE(origin.y, base.y);
		PRECISE(origin.z, base.z);
		return 0;
	}
	CASE("window-base-followed")
	{
		/* a window over a vehicle the client follows, the host's word at time 100 */
		struct vehicle_riding_vehicle before = followed_at(0.f, 100, .1f);
		struct vehicle_riding_vehicle along = followed_at(.1f, 101, .1f);
		struct vehicle_riding_vehicle snap = followed_at(3.f, 101, .1f);
		struct vehicle_riding_vehicle no_time = followed_at(.3f, 100, .1f);
		real_point3d base = point(-.5f, .2f, .5f);
		struct vehicle_riding_state state = riding_on(&before, base, vector(.1f, 0.f, 0.f));
		real_point3d origin;

		state.ticks = RIDING_TICKS - 1;
		/* along its own path: carried as the step counts the deck's motion */
		CHECK(vehicle_riding_snapshot_step(&state, &along, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "the word refused");
		vehicle_riding_window_base(&state, &along, &base, &origin);
		PRECISE(origin.x - base.x, step.tracked_deck_velocity.i*step.player_ticks);
		PRECISE(origin.x, base.x + .1f);
		/* the host's snap, 2.9 off its path: the step holds the frame, and the base stays */
		vehicle_riding_window_base(&state, &snap, &base, &origin);
		PRECISE(origin.x, base.x);
		PRECISE(origin.y, base.y);
		PRECISE(origin.z, base.z);
		/* no time since the snapshot: the base stays */
		vehicle_riding_window_base(&state, &no_time, &base, &origin);
		PRECISE(origin.x, base.x);
		PRECISE(origin.y, base.y);
		PRECISE(origin.z, base.z);
		return 0;
	}
	CASE("window-base-role-change")
	{
		/* a window over a vehicle this machine simulated, followed since: the move across the change is no deck motion */
		struct vehicle_riding_vehicle before = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_vehicle after = followed_at(.5f, 103, .1f);
		real_point3d base = point(-.5f, .2f, .5f);
		struct vehicle_riding_state state;
		real_point3d origin;

		before.time = 100;
		state = riding_on(&before, base, vector(.05f, 0.f, 0.f));
		state.ticks = RIDING_TICKS - 1;
		vehicle_riding_window_base(&state, &after, &base, &origin);
		PRECISE(origin.x, base.x);
		PRECISE(origin.y, base.y);
		PRECISE(origin.z, base.z);
		return 0;
	}
	CASE("damage-speed-default")
	{
		real run = (real)sqrt(3.f*3.f + 2.75f*2.75f)/30.f;

		setting_damage_speed = 0.0;
		vehicle_riding_latch();
		NEAR(vehicle_riding_damage_speed(TRUE, 3.f, 2.5f, 2.75f, FALSE, 1.5f, 1.f, .1f),
			2.f*(real)sqrt(run*run + .01f));
		NEAR(vehicle_riding_damage_speed(TRUE, 3.f, 2.5f, 2.75f, TRUE, 1.5f, 1.f, .1f),
			2.f*(real)sqrt(1.5f*run*1.5f*run + .01f));
		NEAR(vehicle_riding_damage_speed(TRUE, 3.f, 2.5f, 2.75f, TRUE, 1.5f, 1.5f, .1f),
			2.f*(real)sqrt(2.25f*run*2.25f*run + .01f));
		NEAR(vehicle_riding_damage_speed(FALSE, 3.f, 2.5f, 2.75f, FALSE, 1.f, 1.f, .1f), .2f);
		NEAR(vehicle_riding_damage_speed(TRUE, 3.f, 2.5f, (real)NAN, FALSE, 1.f, 1.f, .1f), 0.f);
		setting_damage_speed = 0.4;
		vehicle_riding_latch();
		NEAR(vehicle_riding_damage_speed(TRUE, 3.f, 2.5f, 2.75f, FALSE, 1.f, 1.f, .1f), .4f);
		return 0;
	}
	CASE("leg-stomp")
	{
		/* the body drives 0.35, the ridden leg (node 28) drops 1 on its own */
		struct vehicle_riding_vehicle before = node_of_body(vehicle_at(VEHICLE, 2.f, 0.f, 0.f, 0.f, 10), 28, point(0.f, 0.f, 0.f));
		struct vehicle_riding_vehicle after = node_of_body(vehicle_at(VEHICLE, 2.35f, 0.f, -1.f, 0.f, 11), 28, point(.35f, 0.f, 0.f));
		struct vehicle_riding_state state = riding_on(&before, point(2.f, 0.f, .5f), zero);
		real_point3d base = point(2.f, 0.f, .5f);
		real_point3d moved;
		real_vector3d velocity = zero;

		CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, NODE_LIMIT, &step), "dropped");
		CHECK(step.node_rejected, "the leg's fall was taken for a deck");
		NEAR(step.displacement.i, .35f);
		NEAR(step.displacement.j, 0.f);
		NEAR(step.displacement.k, 0.f);
		NEAR(step.velocity_now.i, .35f);
		NEAR(step.velocity_now.k, 0.f);
		NEAR(step.tracked_deck_velocity.i, .35f);
		NEAR(step.tracked_deck_velocity.k, 0.f);
		/* standing on that same leg: anchored to the body instead */
		moved = point(base.x + step.displacement.i, base.y + step.displacement.j, base.z + step.displacement.k);
		vehicle_riding_record(&state, &step, &after, TRUE, &zero, &up, &moved, FALSE, &velocity);
		CHECK(state.node_index == NONE, "node %d, not the body", state.node_index);
		CHECK(vehicle_riding_is_riding(&state) && state.object_index == VEHICLE, "not riding the vehicle");
		NEAR(state.vehicle_position.x, .35f);
		NEAR(state.vehicle_position.y, 0.f);
		NEAR(state.vehicle_position.z, 0.f);
		NEAR(state.body_position.x, .35f);
		NEAR(state.local_point.x, 2.f);
		NEAR(state.local_point.y, 0.f);
		NEAR(state.local_point.z, .5f);
		return 0;
	}
	CASE("turret-with-travel")
	{
		/* the body drives 0.35 and the turret turns .05: the rider, 2 from its axis, moves 0.1 on it, with the travel (0.45) */
		struct vehicle_riding_state state;
		real_point3d base;

		CHECK(turret_tick(.05f, NODE_LIMIT, &state, &base, &step), "dropped");
		CHECK(!step.node_rejected, "the turret's turn was taken for no deck, with the travel");
		NEAR(step.displacement.i, .35f + 2.f*(real)sin(.05f));
		NEAR(step.displacement.j, 2.f*(1.f - (real)cos(.05f)));
		NEAR(step.dyaw, .05f);
		return 0;
	}
	CASE("turret-past-limit")
	{
		/* the same, turned .15: 0.3 a tick on the body, over the limit */
		struct vehicle_riding_state state;
		real_point3d base;

		CHECK(turret_tick(.15f, NODE_LIMIT, &state, &base, &step), "dropped");
		CHECK(step.node_rejected, "a turret turning 0.3 a tick was a deck");
		NEAR(step.displacement.i, .35f);
		NEAR(step.displacement.j, 0.f);
		NEAR(step.dyaw, 0.f);
		return 0;
	}
	CASE("limit-edge")
	{
		/* a turret (node 2) on a still body turning a quarter turn in one
		update, its rider at (0.125, 0.125) on it: the rider's spot moves 0.25
		on the body (exact axes, so exactly 0.25). The speed limit exactly
		that: kept; a hair below: rejected */
		struct vehicle_riding_vehicle before = node_of_body(vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10), 2, point(0.f, 0.f, 0.f));
		struct vehicle_riding_vehicle after = node_of_body(vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 11), 2, point(0.f, 0.f, 0.f));
		struct vehicle_riding_state state = riding_on(&before, point(.125f, .125f, 0.f), zero);
		real_point3d base = point(.125f, .125f, 0.f);

		after.forward = vector(0.f, 1.f, 0.f);
		CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, .25f, &step), "dropped");
		CHECK(!step.node_rejected, "a turret exactly at the limit was rejected");
		NEAR(step.displacement.i, -.25f);
		NEAR(step.displacement.j, 0.f);
		CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, .2499f, &step), "dropped");
		CHECK(step.node_rejected, "a turret over the limit was kept");
		return 0;
	}
	CASE("body-never-judged")
	{
		/* the body itself, 2 in one update: no node to reject */
		struct vehicle_riding_vehicle before = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_vehicle after = vehicle_at(VEHICLE, 2.f, 0.f, 0.f, 0.f, 11);
		struct vehicle_riding_state state = riding_on(&before, point(1.f, 0.f, 0.f), zero);
		real_point3d base = point(1.f, 0.f, 0.f);

		CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, NODE_LIMIT, &step), "dropped");
		CHECK(!step.node_rejected, "the body was judged");
		NEAR(step.displacement.i, 2.f);
		NEAR(step.velocity_now.i, 2.f);
		return 0;
	}
	CASE("take-off-body")
	{
		/* standing on a turret (node 2) that turns .05 while the body drives
		0.35, 2 to its right: leaving the floor anchors the window to the body,
		at the body's motion there (0.35), never the turret's (about 0.45) */
		struct vehicle_riding_state state;
		real_point3d base;
		real_vector3d velocity = vector(0.f, 0.f, .2f);

		CHECK(turret_tick(.05f, ANY_LIMIT, &state, &base, &step), "dropped");
		CHECK(!step.node_rejected, "the turret was rejected");
		CHECK(step.velocity_now.i > .4f, "the case needs a turret faster than the body: %g", (double)step.velocity_now.i);
		NEAR(step.body_deck_velocity.i, .35f);
		NEAR(step.body_deck_velocity.j, 0.f);
		vehicle_riding_record(&state, &step, NULL, FALSE, &zero, &up, &base, FALSE, &velocity);
		NEAR(velocity.i, .35f);
		NEAR(velocity.j, 0.f);
		NEAR(velocity.k, .2f);
		NEAR(state.velocity.i, .35f);
		NEAR(state.velocity.j, 0.f);
		CHECK(state.node_index == NONE, "node %d, not the body", state.node_index);
		CHECK(state.ticks == RIDING_TICKS - 1 && state.falling == 1, "the window not opened");
		NEAR(state.vehicle_position.x, .35f);
		NEAR(state.vehicle_position.y, 0.f);
		NEAR(state.vehicle_forward.i, 1.f);
		NEAR(state.vehicle_forward.j, 0.f);
		NEAR(state.body_position.x, .35f);
		/* its place in the body's frame: 0.35 behind the origin and 2 to its right */
		NEAR(state.local_point.x, -.35f);
		NEAR(state.local_point.y, -2.f);
		NEAR(state.local_point.z, 0.f);
		return 0;
	}
	CASE("drop-opens-window")
	{
		/* riding a node (2) at (1, 0, 1) of a body whose origin is (0, 0, 0) */
		struct vehicle_riding_vehicle before = node_of_body(vehicle_at(VEHICLE, 1.f, 0.f, 1.f, 0.f, 10), 2, point(0.f, 0.f, 0.f));
		struct vehicle_riding_vehicle after = node_of_body(vehicle_at(VEHICLE, 1.f, 0.f, 1.f, 0.f, 11), 2, point(0.f, 0.f, 0.f));
		struct vehicle_riding_state state = riding_on(&before, point(1.f, 0.f, 1.f), vector(.1f, 0.f, 0.f));
		real_point3d base = point(3.f, 0.f, 1.f);

		/* moved alone 2 (a script, a fix): in the window from the body where it is now, its frame kept */
		CHECK(!vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "kept riding");
		CHECK(state.object_index == VEHICLE, "dropped whole");
		CHECK(state.ticks == RIDING_TICKS - 1 && state.falling == 1, "ticks %d falling %d", state.ticks, state.falling);
		CHECK(state.node_index == NONE, "node %d, not the body", state.node_index);
		NEAR(state.velocity.i, .1f);
		NEAR(state.vehicle_position.x, 0.f);
		NEAR(state.vehicle_position.z, 0.f);
		NEAR(state.body_position.x, 0.f);
		NEAR(state.local_point.x, 3.f);
		NEAR(state.local_point.z, 1.f);
		CHECK(state.vehicle_updates == 11, "snapshot of update %ld", state.vehicle_updates);
		/* a teleport of the vehicle (5 in one update): in the window at the body's new place */
		state = riding_on(&before, point(1.f, 0.f, 1.f), vector(.1f, 0.f, 0.f));
		base = point(1.f, 0.f, 1.f);
		after = node_of_body(vehicle_at(VEHICLE, 6.f, 0.f, 1.f, 0.f, 11), 2, point(5.f, 0.f, 0.f));
		CHECK(!vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "teleport carried");
		CHECK(state.object_index == VEHICLE && state.ticks == RIDING_TICKS - 1 && state.falling == 1, "not in the window");
		NEAR(state.velocity.i, .1f);
		NEAR(state.vehicle_position.x, 5.f);
		NEAR(state.local_point.x, -4.f);
		NEAR(state.local_point.z, 1.f);
		/* its vehicle gone while it rides: riding ends, falling from the frame it rode */
		state = riding_on(&before, point(1.f, 0.f, 1.f), vector(.1f, 0.f, -.2f));
		CHECK(!vehicle_riding_snapshot_step(&state, NULL, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "gone kept");
		CHECK(state.object_index == NONE && state.ticks == 0 && state.falling == 1, "falling %d", state.falling);
		NEAR(state.velocity.i, .1f);
		NEAR(state.velocity.k, -.2f);
		/* gone while it is in the air in a held window over it: the same */
		state = riding_on(&before, point(1.f, 0.f, 1.f), vector(.1f, 0.f, -.2f));
		state.ticks = 40;
		state.falling = 1;
		CHECK(!vehicle_riding_snapshot_step(&state, NULL, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "gone kept");
		CHECK(state.object_index == NONE && state.falling == 1, "falling %d", state.falling);
		NEAR(state.velocity.k, -.2f);
		return 0;
	}
	CASE("grip-limit")
	{
		struct vehicle_riding_vehicle vehicle;
		struct vehicle_riding_state state;
		struct vehicle_riding_air air;
		real_point3d base;
		real_vector3d velocity;
		real_vector3d frame;
		real side = .045f/(real)sqrt(2.);

		/* a turn of 0.02 in a tick, under the limit of 0.045: followed whole,
		the biped given the same change, the frame's k the deck's */
		state = gripped(.045, .3f, .02f, _vehicle_riding_enclosure_open, &velocity, &frame);
		NEAR(state.velocity.i, .3f);
		NEAR(state.velocity.j, .02f);
		NEAR(state.velocity.k, .02f);
		NEAR(velocity.i, .35f);
		NEAR(velocity.j, .07f);
		NEAR(velocity.k, .1f);
		NEAR(frame.i, .3f);
		NEAR(frame.j, .02f);
		NEAR(frame.k, 0.f);
		/* enclosed, a speed-up and a turn of (0.06, 0.06), over it: 0.045
		along the change (0.0318 each way) in one clamp, never 0.045 on each
		axis */
		state = gripped(.045, .36f, .06f, _vehicle_riding_enclosure_enclosed, &velocity, &frame);
		NEAR(state.velocity.i, .3f + side);
		NEAR(state.velocity.j, side);
		NEAR(velocity.i, .35f + side);
		NEAR(velocity.j, .05f + side);
		NEAR(frame.i, .3f + side);
		NEAR(frame.j, side);
		/* a grip of 0: no turn and no speed-up followed (the frame's k still the deck's) */
		state = gripped(0., .36f, .06f, _vehicle_riding_enclosure_enclosed, &velocity, &frame);
		NEAR(state.velocity.i, .3f);
		NEAR(state.velocity.j, 0.f);
		NEAR(state.velocity.k, .02f);
		NEAR(velocity.i, .35f);
		NEAR(velocity.j, .05f);
		/* a large grip: any such change followed at once (no upper clamp
		needed) */
		state = gripped(2., .36f, .06f, _vehicle_riding_enclosure_enclosed, &velocity, &frame);
		NEAR(state.velocity.i, .36f);
		NEAR(state.velocity.j, .06f);
		NEAR(velocity.i, .41f);
		NEAR(velocity.j, .11f);
		/* below 0, and not a number: none */
		state = gripped(-1., .36f, .06f, _vehicle_riding_enclosure_enclosed, &velocity, &frame);
		NEAR(state.velocity.i, .3f);
		NEAR(state.velocity.j, 0.f);
		NEAR(velocity.i, .35f);
		state = gripped(NAN, .36f, .06f, _vehicle_riding_enclosure_enclosed, &velocity, &frame);
		NEAR(state.velocity.i, .3f);
		NEAR(velocity.i, .35f);
		/* double speed: a vehicle without a player updates every other tick and
		its update spans two, so the limit over it is 0.09 and the same
		enclosed change (0.085) is followed whole */
		setting_air_grip = .045;
		vehicle_riding_latch();
		vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		state = riding_on(&vehicle, point(0.f, 0.f, 0.f), vector(.3f, 0.f, 0.f));
		state.ticks = 40;
		state.falling = 1;
		base = point(0.f, 0.f, 0.f);
		vehicle = vehicle_at(VEHICLE, .72f, .12f, 0.f, 0.f, 11);
		CHECK(vehicle_riding_snapshot_step(&state, &vehicle, &base, 1.4f, .2f, TRUE, ANY_LIMIT, &step), "dropped");
		NEAR(step.player_ticks, 2.f);
		NEAR(step.tracked_deck_velocity.i, .36f);
		NEAR(step.tracked_deck_velocity.j, .06f);
		step.over_deck = TRUE;
		memset(&air, 0, sizeof(air));
		air.enclosure = _vehicle_riding_enclosure_enclosed;
		velocity = vector(.35f, .05f, .1f);
		vehicle_riding_air_frame(&state, &step, &air, TRUE, &velocity, &frame);
		NEAR(state.velocity.i, .36f);
		NEAR(state.velocity.j, .06f);
		NEAR(velocity.i, .41f);
		return 0;
	}
	CASE("air-frame-constant")
	{
		/* a deck at 0.3 a tick; a running jump off it, 0.15 a tick forward on it */
		struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_state state = riding_on(&vehicle, point(-1.f, 0.f, 0.f), vector(.3f, 0.f, 0.f));
		real_point3d base = point(-1.f, 0.f, 0.f);
		real_vector3d velocity = vector(.45f, 0.f, .2f);
		real_vector3d frame = zero;
		short tick;

		state.ticks = RIDING_TICKS - 1;
		state.falling = 1;
		for (tick = 1; tick <= 20; tick++)
		{
			vehicle = vehicle_at(VEHICLE, .3f*tick, 0.f, 0.f, 0.f, 10 + tick);
			CHECK(vehicle_riding_snapshot_step(&state, &vehicle, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "dropped");
			step.over_deck = TRUE;
			open_air_frame(&state, &step, TRUE, &velocity, &frame);
			base.x += velocity.i;
			vehicle_riding_record(&state, &step, NULL, FALSE, &zero, &up, &base, FALSE, &velocity);
		}
		/* the frame unchanged, the jump's own motion kept */
		NEAR(state.velocity.i, .3f);
		NEAR(frame.i, .3f);
		NEAR(velocity.i, .45f);
		CHECK(state.ticks == RIDING_TICKS - 1, "the window ran down over the deck: %d", state.ticks);
		/* where it comes down: 0.15 a tick ahead on the deck, 3 in 20 ticks, from 1 behind */
		NEAR(base.x - vehicle.position.x, 2.f);
		return 0;
	}
	CASE("parked-jump")
	{
		/* a parked deck: the take-off's frame 0, no air frame, the biped's velocity the original game's */
		struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_state state = riding_on(&vehicle, point(0.f, 0.f, 0.f), zero);
		real_point3d base = point(0.f, 0.f, 0.f);
		real_vector3d velocity = vector(.1f, 0.f, .2f);
		real_vector3d frame;
		short tick;

		vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 11);
		CHECK(vehicle_riding_snapshot_step(&state, &vehicle, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "dropped");
		vehicle_riding_record(&state, &step, NULL, FALSE, &zero, &up, &base, FALSE, &velocity);
		NEAR(velocity.i, .1f);
		NEAR(velocity.k, .2f);
		for (tick = 1; tick <= 10; tick++)
		{
			vehicle.updates++;
			CHECK(vehicle_riding_snapshot_step(&state, &vehicle, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "dropped");
			step.over_deck = TRUE;
			open_air_frame(&state, &step, TRUE, &velocity, &frame);
			NEAR(frame.i, 0.f);
			NEAR(frame.j, 0.f);
			NEAR(velocity.i, .1f);
			NEAR(velocity.j, 0.f);
			NEAR(velocity.k, .2f);
			vehicle_riding_record(&state, &step, NULL, FALSE, &zero, &up, &base, FALSE, &velocity);
		}
		NEAR(state.velocity.i, 0.f);
		NEAR(state.velocity.j, 0.f);
		NEAR(state.velocity.k, 0.f);
		return 0;
	}
	CASE("grip-standing")
	{
		/* in the window over a still deck, standing on a crate inside the
		vehicle: no grip (the ground's physics is the world's), the window held */
		struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_state state = riding_on(&vehicle, point(0.f, 0.f, 1.f), vector(.3f, 0.f, 0.f));
		real_point3d base = point(0.f, 0.f, 1.f);
		real_vector3d velocity = zero;
		real_vector3d frame = zero;
		short tick;

		state.ticks = 30;
		state.falling = 1;
		for (tick = 1; tick <= 200; tick++)
		{
			vehicle.updates++;
			CHECK(vehicle_riding_snapshot_step(&state, &vehicle, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "dropped");
			step.over_deck = TRUE;
			velocity = zero;
			open_air_frame(&state, &step, FALSE, &velocity, &frame);
			vehicle_riding_record(&state, &step, NULL, TRUE, &zero, &up, &base, FALSE, &velocity);
		}
		NEAR(state.velocity.i, .3f);
		NEAR(velocity.i, 0.f);
		NEAR(frame.i, 0.f);
		CHECK(state.ticks == 30 && state.falling == 0, "ticks %d falling %d: not held on the crate", state.ticks,
			state.falling);
		/* stepping off the crate, still over the deck: falling again, still held */
		vehicle.updates++;
		CHECK(vehicle_riding_snapshot_step(&state, &vehicle, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "dropped");
		step.over_deck = TRUE;
		vehicle_riding_record(&state, &step, NULL, FALSE, &zero, &up, &base, FALSE, &velocity);
		CHECK(state.ticks == 30 && state.falling == 1, "ticks %d falling %d: not falling off the crate", state.ticks,
			state.falling);
		return 0;
	}
	CASE("off-edge")
	{
		struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_state state = riding_on(&vehicle, point(0.f, 0.f, 1.f), vector(.3f, 0.f, 0.f));
		real_point3d base = point(0.f, 0.f, 1.f);
		real_vector3d velocity = vector(.3f, 0.f, 0.f);
		real_vector3d frame;

		/* the reach: a jump of 0.1 a tick under a gravity of 0.005 a tick rises 1, plus a pill height of 1.4 */
		NEAR(vehicle_riding_reach(.1f, .005f, 1.4f), 2.4f);
		/* no gravity: the pill height alone */
		NEAR(vehicle_riding_reach(.1f, 0.f, 1.4f), 1.4f);
		CHECK(vehicle_riding_over_deck(TRUE, 2.f, 2.4f, &up), "a deck within reach missed");
		CHECK(!vehicle_riding_over_deck(FALSE, 0.f, 2.4f, &up), "over a deck the ray missed");
		CHECK(!vehicle_riding_over_deck(TRUE, 2.5f, 2.4f, &up), "over a deck beyond reach");
		/* off the edge (the verdict as the step leaves it: no hit): no grip, no
		air frame; in the air the window holds */
		state.ticks = 30;
		state.falling = 1;
		vehicle.updates++;
		CHECK(vehicle_riding_snapshot_step(&state, &vehicle, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "dropped");
		open_air_frame(&state, &step, TRUE, &velocity, &frame);
		NEAR(state.velocity.i, .3f);
		NEAR(velocity.i, .3f);
		NEAR(frame.i, 0.f);
		vehicle_riding_record(&state, &step, NULL, FALSE, &zero, &up, &base, FALSE, &velocity);
		CHECK(state.ticks == 30 && state.falling == 1, "ticks %d falling %d: not held in the air", state.ticks,
			state.falling);
		/* down on the ground off the deck: the window runs down */
		vehicle.updates++;
		CHECK(vehicle_riding_snapshot_step(&state, &vehicle, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "dropped");
		vehicle_riding_record(&state, &step, NULL, TRUE, &zero, &up, &base, FALSE, &velocity);
		CHECK(state.ticks == 29 && state.falling == 0, "ticks %d falling %d: not run down on the ground", state.ticks,
			state.falling);
		return 0;
	}
	CASE("tilted")
	{
		real_vector3d fifty = vector((real)sin(50.f*PI/180.f), 0.f, (real)cos(50.f*PI/180.f));
		real_vector3d forty = vector((real)sin(40.f*PI/180.f), 0.f, (real)cos(40.f*PI/180.f));
		struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_state state = riding_on(&vehicle, point(0.f, 0.f, 1.f), vector(.3f, 0.f, 0.f));
		real_point3d base = point(0.f, 0.f, 1.f);
		real_vector3d velocity = vector(.3f, 0.f, 0.f);
		real_vector3d frame;

		CHECK(!vehicle_riding_over_deck(TRUE, 1.f, 2.4f, &fifty), "a body rolled 50 degrees was a deck");
		CHECK(vehicle_riding_over_deck(TRUE, 1.f, 2.4f, &forty), "a body rolled 40 degrees was no deck");
		/* rolled 50 degrees: no grip (in the air the window holds all the same) */
		state.ticks = 30;
		state.falling = 1;
		vehicle.updates++;
		CHECK(vehicle_riding_snapshot_step(&state, &vehicle, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "dropped");
		step.over_deck = vehicle_riding_over_deck(TRUE, 1.f, 2.4f, &fifty);
		open_air_frame(&state, &step, TRUE, &velocity, &frame);
		NEAR(state.velocity.i, .3f);
		vehicle_riding_record(&state, &step, NULL, FALSE, &zero, &up, &base, FALSE, &velocity);
		CHECK(state.ticks == 30, "ticks %d: the window ran down in the air", state.ticks);
		/* 40 degrees: the frame follows the deck (still) at once, and the hold */
		vehicle.updates++;
		CHECK(vehicle_riding_snapshot_step(&state, &vehicle, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "dropped");
		step.over_deck = vehicle_riding_over_deck(TRUE, 1.f, 2.4f, &forty);
		open_air_frame(&state, &step, TRUE, &velocity, &frame);
		NEAR(state.velocity.i, 0.f);
		NEAR(velocity.i, 0.f);
		vehicle_riding_record(&state, &step, NULL, FALSE, &zero, &up, &base, FALSE, &velocity);
		CHECK(state.ticks == 30, "ticks %d: not held at 40 degrees", state.ticks);
		return 0;
	}
	CASE("turn-relative-velocity")
	{
		/* a deck turning 0.1 this tick with the look carry off: the rider's
		relative velocity turns with it all the same (the function takes no
		local player: remote players and the host's copies turn too) */
		struct vehicle_riding_vehicle before = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_vehicle after = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, .1f, 11);
		struct vehicle_riding_state state = riding_on(&before, point(1.f, 0.f, 0.f), zero);
		real_point3d base = point(1.f, 0.f, 0.f);
		real_vector3d velocity = vector(.1f, 0.f, .05f);

		setting_look_carry = 0;
		vehicle_riding_latch();
		CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "dropped");
		NEAR(vehicle_riding_look(0, TRUE, step.dyaw), 0.f);
		vehicle_riding_turn_relative(&state, &step, &velocity);
		NEAR(velocity.i, .1f*(real)cos(.1f));
		NEAR(velocity.j, .1f*(real)sin(.1f));
		NEAR(velocity.k, .05f);
		/* in the window: not turned */
		state = riding_on(&before, point(1.f, 0.f, 0.f), zero);
		state.ticks = 30;
		velocity = vector(.1f, 0.f, .05f);
		CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "dropped");
		vehicle_riding_turn_relative(&state, &step, &velocity);
		NEAR(velocity.i, .1f);
		NEAR(velocity.j, 0.f);
		/* no step (riding off, or dropped): not turned */
		vehicle_riding_turn_relative(&state, NULL, &velocity);
		NEAR(velocity.i, .1f);
		return 0;
	}
	CASE("skipped-tick-held")
	{
		/* double speed, a vehicle without a player that did not update this
		tick: a held window keeps its frame, the deck's as of the last update */
		struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_state state = riding_on(&vehicle, point(0.f, 0.f, 0.f), vector(.2f, 0.f, -.1f));
		real_point3d base = point(0.f, 0.f, 0.f);
		real_vector3d velocity = vector(.3f, 0.f, .1f);
		real_vector3d frame;

		state.ticks = 40;
		state.falling = 1;
		CHECK(vehicle_riding_snapshot_step(&state, &vehicle, &base, 1.4f, .2f, TRUE, ANY_LIMIT, &step), "dropped");
		CHECK(step.updates == 0, "the vehicle updated");
		NEAR(step.tracked_deck_velocity.i, .2f);
		step.over_deck = TRUE;
		open_air_frame(&state, &step, TRUE, &velocity, &frame);
		NEAR(state.velocity.i, .2f);
		NEAR(state.velocity.k, -.1f);
		NEAR(velocity.i, .3f);
		NEAR(frame.i, .2f);
		vehicle_riding_record(&state, &step, NULL, FALSE, &zero, &up, &base, FALSE, &velocity);
		CHECK(state.ticks == 40, "ticks %d: not held", state.ticks);
		/* a rider leaving on such a tick keeps the frame it rode */
		state = riding_on(&vehicle, base, vector(.2f, 0.f, -.1f));
		velocity = zero;
		CHECK(vehicle_riding_snapshot_step(&state, &vehicle, &base, 1.4f, .2f, TRUE, ANY_LIMIT, &step), "dropped");
		vehicle_riding_record(&state, &step, NULL, FALSE, &zero, &up, &base, FALSE, &velocity);
		NEAR(velocity.i, .2f);
		NEAR(state.velocity.i, .2f);
		NEAR(state.velocity.k, -.1f);
		return 0;
	}
	CASE("riding-off")
	{
		/* the setting off: nothing applies, every vehicle hits as in the
		original game, no air frame, no turn */
		struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_state state;
		real_vector3d velocity = vector(.3f, 0.f, .1f);
		real_vector3d frame;

		setting_riding = 0;
		vehicle_riding_map_decide();
		vehicle_riding_latch();
		CHECK(!vehicle_riding_applies(TRUE, FALSE), "a player rides");
		vehicle_riding_clear(&state);
		CHECK(vehicle_riding_collision_window(&state, FALSE, VEHICLE) == _vehicle_riding_collision_original, "pedestrian");
		state = riding_on(&vehicle, point(0.f, 0.f, 0.f), zero);
		CHECK(vehicle_riding_collision_window(&state, FALSE, VEHICLE) == _vehicle_riding_collision_original,
			"a stale ride kept from the vehicle");
		/* (bipeds.c clears the state and takes no step and casts no ray with riding off) */
		vehicle_riding_clear(&state);
		open_air_frame(&state, NULL, TRUE, &velocity, &frame);
		NEAR(frame.i, 0.f);
		NEAR(frame.j, 0.f);
		NEAR(velocity.i, .3f);
		vehicle_riding_turn_relative(&state, NULL, &velocity);
		NEAR(velocity.i, .3f);
		NEAR(velocity.j, 0.f);
		return 0;
	}

	CASE("air-control-relative")
	{
		/* the game's own airborne branch, air control 0.01 a tick, facing along x */
		struct biped_physics physics;

		memset(&physics, 0, sizeof(physics));
		physics.forward = vector(1.f, 0.f, 0.f);
		physics.airborne_acceleration_maximum = .01f;
		/* moving with the air frame, no input: it keeps the frame's motion,
		not braked toward the world's rest */
		physics.velocity = vector(.3f, .1f, .2f);
		physics.air_frame_velocity = vector(.3f, .1f, 0.f);
		airborne_branch(&physics, FLAG(_biped_physics_in_airborne_bit));
		NEAR(physics.new_velocity.i, .3f);
		NEAR(physics.new_velocity.j, .1f);
		NEAR(physics.new_velocity.k, .2f - global_gravity);
		/* with input: toward the frame's motion plus the stick's, as far as air
		control goes in a tick */
		physics.movement_desired = vector(.05f, 0.f, 0.f);
		airborne_branch(&physics, FLAG(_biped_physics_in_airborne_bit));
		NEAR(physics.new_velocity.i, .31f);
		NEAR(physics.new_velocity.j, .1f);
		/* faster than the frame, no input: the relative part braked as the
		original brakes a world velocity on still ground */
		physics.movement_desired = zero;
		physics.velocity = vector(.4f, .1f, .2f);
		airborne_branch(&physics, FLAG(_biped_physics_in_airborne_bit));
		NEAR(physics.new_velocity.i, .39f);
		NEAR(physics.new_velocity.j, .1f);
		/* no air frame (riding off, off the deck, a parked deck): the original
		game's, braked toward the world's rest */
		physics.air_frame_velocity = zero;
		physics.velocity = vector(.3f, .1f, .2f);
		airborne_branch(&physics, FLAG(_biped_physics_in_airborne_bit));
		NEAR(physics.new_velocity.i, .3f - .01f*.3f/(real)sqrt(.1));
		NEAR(physics.new_velocity.j, .1f - .01f*.1f/(real)sqrt(.1));
		return 0;
	}
	CASE("window-holds-in-air")
	{
		/* a deck driving 0.2 a tick; the biped leaves it 1 to its left and
		stays beside it, off its deck but inside its sphere */
		struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_state state = riding_on(&vehicle, point(0.f, 1.f, 0.f), vector(.2f, 0.f, 0.f));
		real_point3d base = point(0.f, 1.f, 0.f);
		real_point3d below = point(0.f, 1.f, -10.f);
		real_vector3d velocity = zero;
		short tick;

		/* the take-off tick: counted down, so it rides no more */
		vehicle = vehicle_at(VEHICLE, .2f, 0.f, 0.f, 0.f, 11);
		CHECK(vehicle_riding_snapshot_step(&state, &vehicle, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "dropped");
		vehicle_riding_record(&state, &step, NULL, FALSE, &zero, &up, &base, FALSE, &velocity);
		CHECK(state.ticks == RIDING_TICKS - 1 && !vehicle_riding_is_riding(&state) && state.falling == 1,
			"ticks %d falling %d: the take-off did not open the window", state.ticks, state.falling);
		/* 100 ticks in the air off the deck (the ray misses it): held, still the vehicle left */
		for (tick = 1; tick <= 100; tick++)
		{
			vehicle.updates++;
			CHECK(vehicle_riding_snapshot_step(&state, &vehicle, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step),
				"dropped in the air at tick %d", tick);
			vehicle_riding_record(&state, &step, NULL, FALSE, &zero, &up, &base, FALSE, &velocity);
		}
		CHECK(state.ticks == RIDING_TICKS - 1 && state.object_index == VEHICLE, "ticks %d: not held in the air",
			state.ticks);
		/* on the ground off the deck: counted down, a tick at a time */
		vehicle.updates++;
		CHECK(vehicle_riding_snapshot_step(&state, &vehicle, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "dropped");
		vehicle_riding_record(&state, &step, NULL, TRUE, &zero, &up, &base, FALSE, &velocity);
		CHECK(state.ticks == RIDING_TICKS - 2 && state.falling == 0, "ticks %d falling %d: not counted on the ground",
			state.ticks, state.falling);
		vehicle.updates++;
		CHECK(vehicle_riding_snapshot_step(&state, &vehicle, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "dropped");
		vehicle_riding_record(&state, &step, NULL, TRUE, &zero, &up, &base, FALSE, &velocity);
		CHECK(state.ticks == RIDING_TICKS - 3, "ticks %d: not counted on the ground", state.ticks);
		/* far below it, out of its sphere: the window ends */
		vehicle.updates++;
		CHECK(!vehicle_riding_snapshot_step(&state, &vehicle, &below, 1.4f, .2f, FALSE, ANY_LIMIT, &step),
			"a window far from its vehicle kept");
		CHECK(state.object_index == NONE && state.ticks == 0, "ticks %d: the far window not ended", state.ticks);
		return 0;
	}
	CASE("node-turret-test")
	{
		struct vehicle_riding_vehicle before;
		struct vehicle_riding_vehicle after;
		struct vehicle_riding_state state;
		real_point3d base;

		/* a turret (node 2) on top of a body that drives 0.35 and turns 0.1,
		the turret turning 0.05 more on it: it stays put on the body and turns
		about its own up, a deck */
		before = vehicle_at(VEHICLE, 1.f, 0.f, 1.f, 0.f, 10);
		after = vehicle_at(VEHICLE, .35f + (real)cos(.1f), (real)sin(.1f), 1.f, .15f, 11);
		before.node_index = after.node_index = 2;
		before.origin = point(0.f, 0.f, 0.f);
		after.origin = point(.35f, 0.f, 0.f);
		after.body_forward = vector((real)cos(.1f), (real)sin(.1f), 0.f);
		base = point(2.f, 0.f, 1.f);
		state = riding_on(&before, base, zero);
		CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, NODE_LIMIT, &step), "dropped");
		CHECK(!step.node_rejected, "a turret turning on a driving body was no deck");
		NEAR(step.node_shift, 0.f);
		NEAR(step.node_tilt, 0.f);
		/* a leg (node 28) of a still body sweeping back 0.07 a tick, level: its
		origin moves on the body, so no deck, though it is slower than the speed
		limit */
		before = node_of_body(vehicle_at(VEHICLE, 2.f, 0.f, 0.f, 0.f, 10), 28, point(0.f, 0.f, 0.f));
		after = node_of_body(vehicle_at(VEHICLE, 1.93f, 0.f, 0.f, 0.f, 11), 28, point(0.f, 0.f, 0.f));
		base = point(2.f, 0.f, .5f);
		state = riding_on(&before, base, zero);
		CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, NODE_LIMIT, &step), "dropped");
		CHECK(step.node_rejected, "a leg sweeping back was a deck");
		NEAR(step.node_shift, .07f);
		NEAR(step.displacement.i, 0.f);
		/* rising 0.02 a tick, and falling as much: no deck either way */
		after = node_of_body(vehicle_at(VEHICLE, 2.f, 0.f, .02f, 0.f, 11), 28, point(0.f, 0.f, 0.f));
		state = riding_on(&before, base, zero);
		CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, NODE_LIMIT, &step), "dropped");
		CHECK(step.node_rejected, "a rising leg was a deck");
		after = node_of_body(vehicle_at(VEHICLE, 2.f, 0.f, -.02f, 0.f, 11), 28, point(0.f, 0.f, 0.f));
		state = riding_on(&before, base, zero);
		CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, NODE_LIMIT, &step), "dropped");
		CHECK(step.node_rejected, "a falling leg was a deck");
		/* a node swinging 0.05 about its own forward, its origin put: its up
		tilts on the body, no deck */
		after = node_of_body(vehicle_at(VEHICLE, 2.f, 0.f, 0.f, 0.f, 11), 28, point(0.f, 0.f, 0.f));
		after.up = vector(0.f, -(real)sin(.05f), (real)cos(.05f));
		state = riding_on(&before, base, zero);
		CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, NODE_LIMIT, &step), "dropped");
		CHECK(step.node_rejected, "a swinging node was a deck");
		NEAR(step.node_tilt, .05f);
		NEAR(step.node_shift, 0.f);
		/* the shift exactly at the limit: a deck (the limit is "more than") */
		before = node_of_body(vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10), 2, point(0.f, 0.f, 0.f));
		after = node_of_body(vehicle_at(VEHICLE, RIDING_NODE_SHIFT, 0.f, 0.f, 0.f, 11), 2, point(0.f, 0.f, 0.f));
		base = point(1.f, 0.f, 0.f);
		state = riding_on(&before, base, zero);
		CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, FALSE, NODE_LIMIT, &step), "dropped");
		CHECK(!step.node_rejected, "a node exactly at the shift limit was rejected");
		NEAR(step.node_shift, RIDING_NODE_SHIFT);
		/* double speed: a vehicle without a player moving its node 0.015 on the
		body in one update spans two ticks, 0.0075 a tick: a deck */
		after = node_of_body(vehicle_at(VEHICLE, .015f, 0.f, 0.f, 0.f, 11), 2, point(0.f, 0.f, 0.f));
		state = riding_on(&before, base, zero);
		CHECK(vehicle_riding_snapshot_step(&state, &after, &base, 1.4f, .2f, TRUE, NODE_LIMIT, &step), "dropped");
		CHECK(!step.node_rejected, "a node's motion over two ticks judged as one");
		NEAR(step.node_shift, .0075f);
		return 0;
	}
	CASE("record-lifetime")
	{
		struct vehicle_riding_contact contacts[RIDING_CONTACT_RECORDS];
		real_vector3d side = vector(0.f, 1.f, 0.f);
		real_vector3d front = vector(1.f, 0.f, 0.f);
		short index;

		/* written by the vehicle, read by the biped's tick, cleared after */
		CHECK(vehicle_riding_contact_record(1, VEHICLE, &side), "a touch not recorded");
		CHECK(vehicle_riding_contact_take(1, contacts) == 1 && contacts[0].vehicle_index == VEHICLE, "a touch not read");
		NEAR(contacts[0].normal.j, 1.f);
		CHECK(vehicle_riding_contact_take(1, contacts) == 0, "a touch read twice");
		/* a deferral rides with its touch, until the touch is taken; a fresh record starts undeferred */
		CHECK(!vehicle_riding_contact_defer(1, VEHICLE), "a deferral of no touch");
		CHECK(vehicle_riding_contact_record(1, VEHICLE, &side), "a touch not recorded");
		CHECK(vehicle_riding_contact_take(1, contacts) == 1 && !contacts[0].deferred, "a fresh touch deferred");
		CHECK(vehicle_riding_contact_record(1, VEHICLE, &side), "a touch not recorded");
		CHECK(vehicle_riding_contact_defer(1, VEHICLE) && !vehicle_riding_contact_defer(1, OTHER_VEHICLE), "deferral");
		CHECK(vehicle_riding_contact_record(1, VEHICLE, &front), "a second touch not recorded");
		CHECK(vehicle_riding_contact_take(1, contacts) == 1 && contacts[0].deferred, "a deferral lost with its touch");
		CHECK(vehicle_riding_contact_record(1, VEHICLE, &side), "a touch not recorded");
		CHECK(vehicle_riding_contact_take(1, contacts) == 1 && !contacts[0].deferred, "a deferral kept past its touch");
		/* one a vehicle: its second touch in the same update replaces its first */
		CHECK(vehicle_riding_contact_record(1, VEHICLE, &side), "a touch not recorded");
		CHECK(vehicle_riding_contact_record(1, VEHICLE, &front), "a second touch not recorded");
		CHECK(vehicle_riding_contact_take(1, contacts) == 1, "one vehicle recorded twice");
		NEAR(contacts[0].normal.i, 1.f);
		/* each local player its own */
		CHECK(vehicle_riding_contact_record(0, VEHICLE, &side), "a touch not recorded");
		CHECK(vehicle_riding_contact_take(1, contacts) == 0, "another player's touch read");
		CHECK(vehicle_riding_contact_take(0, contacts) == 1, "a player's own touch lost");
		/* up to four vehicles; a fifth replaces nothing */
		for (index = 0; index < RIDING_CONTACT_RECORDS; index++)
		{
			CHECK(vehicle_riding_contact_record(2, VEHICLE + index, &side), "vehicle %d not recorded", index);
		}
		CHECK(!vehicle_riding_contact_record(2, OTHER_VEHICLE, &front), "a fifth vehicle recorded");
		CHECK(vehicle_riding_contact_take(2, contacts) == RIDING_CONTACT_RECORDS, "not the four vehicles");
		for (index = 0; index < RIDING_CONTACT_RECORDS; index++)
		{
			CHECK(contacts[index].vehicle_index == VEHICLE + index, "record %d is %lx", index,
				(unsigned long)contacts[index].vehicle_index);
		}
		/* a biped whose tick never came (an early return, a seat, death):
		cleared at the next game tick's start */
		CHECK(vehicle_riding_contact_record(3, VEHICLE, &side), "a touch not recorded");
		vehicle_riding_contacts_clear();
		CHECK(vehicle_riding_contact_take(3, contacts) == 0, "a skipped tick's touch kept");
		/* and at the latch (a map's start, a game's start, a load) */
		CHECK(vehicle_riding_contact_record(3, VEHICLE, &side), "a touch not recorded");
		vehicle_riding_latch();
		CHECK(vehicle_riding_contact_take(3, contacts) == 0, "a touch kept over the latch");
		/* no local player: nothing kept */
		CHECK(!vehicle_riding_contact_record(NONE, VEHICLE, &side), "a touch kept with no player");
		CHECK(!vehicle_riding_contact_record(HALO_PORT_MAXIMUM_NETWORK_PLAYERS, VEHICLE, &side),
			"a touch kept past the players");
		CHECK(vehicle_riding_contact_take(NONE, contacts) == 0, "touches read with no player");
		return 0;
	}
	CASE("contact-sequence")
	{
		struct contact_world world;
		struct vehicle_riding_step held;
		struct vehicle_riding_vehicle other;
		real_point3d base = point(0.f, 0.f, 0.f);
		real_vector3d front = vector(1.f, 0.f, 0.f);
		real_vector3d velocity;
		real_vector3d move;
		real_vector3d ahead;
		real fall;
		short tick;

		/* the resolution's arithmetic: the vehicle the window tracks moved 0.1
		a tick at the base by its pose (its velocities 0), its update spanning
		two ticks; the biped moves 0.05 a tick level and falls 0.3: moved out
		0.1, its own 0.1 still ahead of it, its fall not */
		memset(&held, 0, sizeof(held));
		held.vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 11);
		held.updates = 1;
		held.player_ticks = 2.f;
		held.body_deck_velocity = vector(.1f, 0.f, 0.f);
		velocity = vector(.05f, 0.f, -.3f);
		NEAR(vehicle_riding_contact_move(&held, FALSE, &held.vehicle, &base, &velocity, &front, TRUE, &move, &ahead),
			.1f);
		NEAR(move.i, .1f);
		NEAR(ahead.i, .1f);
		NEAR(ahead.k, 0.f);
		/* a deck rising 0.05 under a biped falling 0.3: lifted by the rise alone */
		held.player_ticks = 1.f;
		held.body_deck_velocity = vector(0.f, 0.f, .05f);
		velocity = vector(0.f, 0.f, -.3f);
		NEAR(vehicle_riding_contact_move(&held, FALSE, &held.vehicle, &base, &velocity, &up, FALSE, &move, &ahead), .05f);
		NEAR(move.k, .05f);
		/* a vehicle moving away: never pulled */
		held.body_deck_velocity = vector(-.1f, 0.f, 0.f);
		velocity = zero;
		NEAR(vehicle_riding_contact_move(&held, FALSE, &held.vehicle, &base, &velocity, &front, FALSE, &move, &ahead),
			0.f);
		NEAR(move.i, 0.f);
		/* another vehicle (a convoy's) without a player in double speed: its
		update's motion, 0.2 */
		other = vehicle_at(OTHER_VEHICLE, -2.f, 0.f, 0.f, 0.f, 10);
		other.translational_velocity = vector(.2f, 0.f, 0.f);
		NEAR(vehicle_riding_contact_move(&held, FALSE, &other, &base, &velocity, &front, TRUE, &move, &ahead), .2f);
		/* a rider its own vehicle carried 0.2 this tick, touched by one moving
		with it: not moved, its own motion made, none ahead */
		held.displacement = vector(.2f, 0.f, 0.f);
		NEAR(vehicle_riding_contact_move(&held, TRUE, &other, &base, &velocity, &front, TRUE, &move, &ahead), 0.f);
		NEAR(ahead.i, 0.f);

		/* a standing biped in the window of the vehicle it left, struck by its
		front driving 0.1 a tick: from the first touch moved exactly 0.1 a
		tick, never twice, its velocity never given (its friction has nothing
		to undo) */
		contact_world_new(&world, point(3.f, 0.f, 0.f), zero, zero, 40, FALSE, FALSE);
		contact_face(&world, 0, front, .3f, vector(.1f, 0.f, 0.f));
		contact_tick(&world, TRUE);
		CHECK(world.tick_touches == 0, "touched 0.2 off");
		NEAR(world.base.x, 3.f);
		for (tick = 2; tick <= 6; tick++)
		{
			contact_tick(&world, TRUE);
			CHECK(world.tick_touches == 1, "struck: tick %d, %d touches", tick, world.tick_touches);
			NEAR(world.moved, .1f);
			NEAR(world.base.x, 3.f + .1f*(tick - 1));
			NEAR(world.velocity.i, 0.f);
		}

		CHECK(world.fallbacks == 0, "%d fallbacks where the move left no overlap", world.fallbacks);
		/* a jumper over a sloped hood (its normal 0.6 forward, 0.8 up) of the
		vehicle it left, driving 0.2 a tick, its air frame matched: touched,
		never moved (no slide forward, no hover), its own fall alone taking it
		down */
		contact_world_new(&world, point(1.f, 0.f, 1.f), vector(.2f, 0.f, 0.f), vector(.2f, 0.f, 0.f), 40, TRUE, TRUE);
		contact_face(&world, 0, vector(.6f, 0.f, .8f), .3f, vector(.2f, 0.f, 0.f));
		fall = 0.f;
		for (tick = 1; tick <= 3; tick++)
		{
			contact_tick(&world, TRUE);
			fall += global_gravity*tick;
			CHECK(world.tick_touches == 1, "hood: tick %d, %d touches", tick, world.tick_touches);
			NEAR(world.moved, 0.f);
			NEAR(world.base.x, 1.f + .2f*tick);
			NEAR(world.base.z, 1.f - fall);
		}

		CHECK(world.fallbacks == 0, "%d fallbacks where the move left no overlap", world.fallbacks);
		/* a convoy: riding the vehicle (driving 0.2 a tick), touched from
		behind by another driving with it: never moved, still riding, carried
		0.2 a tick */
		contact_world_new(&world, point(0.f, 0.f, 1.f), vector(.2f, 0.f, 0.f), vector(.2f, 0.f, 0.f), RIDING_TICKS,
			FALSE, FALSE);
		world.motions[0] = vector(.2f, 0.f, 0.f);
		contact_face(&world, 1, front, .19f, vector(.2f, 0.f, 0.f));
		for (tick = 1; tick <= 3; tick++)
		{
			contact_tick(&world, TRUE);
			CHECK(world.tick_touches == 1, "convoy: tick %d, %d touches", tick, world.tick_touches);
			NEAR(world.moved, 0.f);
			CHECK(vehicle_riding_is_riding(&world.state), "convoy: knocked off the vehicle it rides at tick %d", tick);
			NEAR(world.base.x, .2f*tick);
		}

		CHECK(world.fallbacks == 0, "%d fallbacks where the move left no overlap", world.fallbacks);
		/* just off the front of the vehicle it left, its momentum matched (0.2
		a tick): not moved; then its air control brakes it 0.001 a tick toward
		the world's rest, and the front pushes it by exactly that lag */
		contact_world_new(&world, point(3.f, 0.f, 1.f), vector(.2f, 0.f, 0.f), vector(.2f, 0.f, 0.f), 40, TRUE, FALSE);
		contact_face(&world, 0, front, .19f, vector(.2f, 0.f, 0.f));
		for (tick = 1; tick <= 3; tick++)
		{
			contact_tick(&world, TRUE);
			CHECK(world.tick_touches == 1, "off the front: tick %d, %d touches", tick, world.tick_touches);
			NEAR(world.moved, .001f*(tick - 1));
		}

		CHECK(world.fallbacks == 0, "%d fallbacks where the move left no overlap", world.fallbacks);
		/* a tailgate (the vehicle driving 0.2 a tick) closing on a jumper over
		its bed lagging at 0.15: pushed by exactly the lag, 0.05 a tick; the
		open deck's speed-up beyond the take-off's not followed */
		contact_world_new(&world, point(1.f, 0.f, 1.f), vector(.15f, 0.f, 0.f), vector(.15f, 0.f, 0.f), 40, TRUE,
			TRUE);
		contact_face(&world, 0, front, .19f, vector(.2f, 0.f, 0.f));
		for (tick = 1; tick <= 3; tick++)
		{
			contact_tick(&world, TRUE);
			CHECK(world.tick_touches == 1, "tailgate: tick %d, %d touches", tick, world.tick_touches);
			NEAR(world.moved, .05f);
			NEAR(world.state.velocity.i, .15f);
		}

		CHECK(world.fallbacks == 0, "%d fallbacks where the move left no overlap", world.fallbacks);
		/* the deck of the vehicle it left rising 0.05 a tick under a biped
		falling 0.3: lifted by the rise alone; its velocity, so its landing
		and the falling kill, as without the touch */
		contact_world_new(&world, point(1.f, 0.f, 1.f), vector(0.f, 0.f, -.3f), zero, 40, TRUE, TRUE);
		contact_face(&world, 0, up, .19f, vector(0.f, 0.f, .05f));
		contact_tick(&world, TRUE);
		CHECK(world.tick_touches == 1, "rising deck: %d touches", world.tick_touches);
		NEAR(world.moved, .05f);
		NEAR(world.base.z, 1.f + .05f - .3f - global_gravity);
		NEAR(world.velocity.i, 0.f);
		NEAR(world.velocity.k, -.3f - global_gravity);
		NEAR(vehicle_riding_falling_velocity(&world.state, world.velocity.k), -.3f - global_gravity);

		CHECK(world.fallbacks == 0, "%d fallbacks where the move left no overlap", world.fallbacks);
		/* riding a still vehicle, touched in one tick by two others closing at
		0.05, from behind and from the side: both resolved, each by its own
		motion */
		contact_world_new(&world, point(0.f, 0.f, 1.f), zero, zero, RIDING_TICKS, FALSE, FALSE);
		contact_face(&world, 1, front, .19f, vector(.05f, 0.f, 0.f));
		contact_face(&world, 2, vector(0.f, -1.f, 0.f), .19f, vector(0.f, -.05f, 0.f));
		contact_tick(&world, TRUE);
		CHECK(world.tick_touches == 2, "two vehicles: %d touches", world.tick_touches);
		NEAR(world.moved, .1f);
		NEAR(world.base.x, .05f);
		NEAR(world.base.y, -.05f);
		CHECK(vehicle_riding_is_riding(&world.state), "two vehicles: knocked off the vehicle it rides");

		CHECK(world.fallbacks == 0, "%d fallbacks where the move left no overlap", world.fallbacks);
		/* a touch in a tick the air frame changes: a jumper over the bed whose
		frame (0.155) is under its peak (0.2), the tailgate driving 0.2: the
		frame climbs back to 0.2 this tick, so the tailgate moves it by nothing
		(resolved before the frame: 0.045 more on top of the frame's own, a
		slide forward) */
		contact_world_new(&world, point(1.f, 0.f, 1.f), vector(.155f, 0.f, 0.f), vector(.155f, 0.f, 0.f), 40, TRUE,
			TRUE);
		world.air.have_peak = TRUE;
		world.air.peak = .2f;
		contact_face(&world, 0, front, .19f, vector(.2f, 0.f, 0.f));
		contact_tick(&world, TRUE);
		CHECK(world.tick_touches == 1, "frame climbing back: %d touches", world.tick_touches);
		NEAR(world.state.velocity.i, .2f);
		NEAR(world.moved, 0.f);
		NEAR(world.base.x, 1.2f);

		CHECK(world.fallbacks == 0, "%d fallbacks where the move left no overlap", world.fallbacks);
		/* a one-tick reversal (0.1 to -0.1) against the cab's back wall ahead
		of a jumper over the bed: the frame follows it at once, so the wall
		moves it by nothing (resolved before the frame: 0.2 back on top of the
		frame's own -0.1, a shove) */
		contact_world_new(&world, point(1.f, 0.f, 1.f), vector(.1f, 0.f, 0.f), vector(.1f, 0.f, 0.f), 40, TRUE, TRUE);
		contact_face(&world, 0, vector(-1.f, 0.f, 0.f), .19f, vector(-.1f, 0.f, 0.f));
		contact_tick(&world, TRUE);
		CHECK(world.tick_touches == 1, "reversal: %d touches", world.tick_touches);
		NEAR(world.state.velocity.i, -.1f);
		NEAR(world.moved, 0.f);
		NEAR(world.base.x, .9f);

		CHECK(world.fallbacks == 0, "%d fallbacks where the move left no overlap", world.fallbacks);
		/* double speed: the vehicle it left, without a player, driving 0.2 an
		update every other tick into a standing biped: moved 0.2 on each tick
		it updates, nothing on the ticks it skips */
		contact_world_new(&world, point(3.f, 0.f, 0.f), zero, zero, 40, FALSE, FALSE);
		world.double_speed = TRUE;
		contact_face(&world, 0, front, .19f, vector(.2f, 0.f, 0.f));
		for (tick = 1; tick <= 4; tick++)
		{
			contact_tick(&world, (tick & 1) != 0);
			CHECK(world.tick_touches == (tick & 1), "double speed: tick %d, %d touches", tick, world.tick_touches);
			NEAR(world.moved, tick & 1 ? .2f : 0.f);
		}
		NEAR(world.base.x, 3.4f);
		CHECK(world.fallbacks == 0, "%d fallbacks where the move left no overlap", world.fallbacks);

		/* a corner: the touch's normal across the vehicle's travel, so the move
		(0) leaves the biped in it, and so does the spot further out along that
		normal: the fix */
		contact_world_new(&world, point(3.f, 0.f, 0.f), zero, zero, 40, FALSE, FALSE);
		contact_face(&world, 0, front, .19f, vector(.1f, 0.f, 0.f));
		world.planes[0] = vector(0.f, 1.f, 0.f);
		contact_tick(&world, TRUE);
		NEAR(world.moved, 0.f);
		CHECK(world.fallbacks == 1 && world.settle == _vehicle_riding_settle_fixed, "corner: %d fallbacks, settle %d",
			world.fallbacks, world.settle);
		return 0;
	}
	CASE("air-frame-asymmetric")
	{
		struct vehicle_riding_state state;
		struct vehicle_riding_air air;
		real_vector3d velocity;
		real_vector3d frame;
		short tick;

		/* a brake from 0.3 to 0.2: followed at once along the way it goes, the
		biped given the same change */
		state = taken_off(.3f, 0.f, &air, _vehicle_riding_enclosure_open);
		velocity = vector(.35f, .05f, .1f);
		air_tick(&state, &air, .2f, 0.f, &velocity, &frame);
		NEAR(state.velocity.i, .2f);
		NEAR(velocity.i, .25f);
		NEAR(velocity.j, .05f);
		NEAR(frame.i, .2f);
		/* a stop: at rest in one tick */
		air_tick(&state, &air, 0.f, 0.f, &velocity, &frame);
		NEAR(state.velocity.i, 0.f);
		NEAR(velocity.i, .05f);
		NEAR(velocity.j, .05f);
		/* a one-tick reversal, 0.3 to -0.3: followed at once */
		state = taken_off(.3f, 0.f, &air, _vehicle_riding_enclosure_open);
		velocity = vector(.35f, .05f, .1f);
		air_tick(&state, &air, -.3f, 0.f, &velocity, &frame);
		NEAR(state.velocity.i, -.3f);
		NEAR(velocity.i, -.25f);
		/* a brake and a turn at once: the brake whole, the turn up to the limit */
		state = taken_off(.3f, 0.f, &air, _vehicle_riding_enclosure_open);
		velocity = zero;
		air_tick(&state, &air, .2f, .1f, &velocity, &frame);
		NEAR(state.velocity.i, .2f);
		NEAR(state.velocity.j, .045f);
		/* back up to the take-off's 0.3 after a brake to 0.2: followed up to the
		limit a tick, 0.245, 0.29, then 0.3 */
		state = taken_off(.3f, 0.f, &air, _vehicle_riding_enclosure_open);
		velocity = zero;
		air_tick(&state, &air, .2f, 0.f, &velocity, &frame);
		air_tick(&state, &air, .3f, 0.f, &velocity, &frame);
		NEAR(state.velocity.i, .245f);
		air_tick(&state, &air, .3f, 0.f, &velocity, &frame);
		NEAR(state.velocity.i, .29f);
		air_tick(&state, &air, .3f, 0.f, &velocity, &frame);
		NEAR(state.velocity.i, .3f);
		/* beyond the take-off's speed on an open deck: not followed, the
		vehicle pulls away and leaves it behind */
		air_tick(&state, &air, .4f, 0.f, &velocity, &frame);
		NEAR(state.velocity.i, .3f);
		/* enclosed: followed up to the limit, the peak raised with it */
		air.enclosure = _vehicle_riding_enclosure_enclosed;
		air_tick(&state, &air, .4f, 0.f, &velocity, &frame);
		NEAR(state.velocity.i, .345f);
		NEAR(air.peak, .345f);
		/* a steady drive at 0.3 with 0.02 of jitter each way, 30 ticks: never
		ahead of the deck, never below the slower deck, and back at 0.3 after
		every faster tick (no ratchet down) */
		state = taken_off(.3f, 0.f, &air, _vehicle_riding_enclosure_open);
		velocity = zero;
		for (tick = 1; tick <= 30; tick++)
		{
			real deck = tick & 1 ? .32f : .28f;

			air_tick(&state, &air, deck, 0.f, &velocity, &frame);
			CHECK(state.velocity.i <= deck + .0001f, "ahead of the deck at tick %d: %g", tick,
				(double)state.velocity.i);
			CHECK(state.velocity.i >= .28f - .0001f, "drifted below the band at tick %d: %g", tick,
				(double)state.velocity.i);
			if (tick > 1 && (tick & 1))
			{
				NEAR(state.velocity.i, .3f);
			}
		}
		/* zero lengths: a frame and a deck at rest, unchanged; both under
		0.001, the change followed whole */
		state = taken_off(0.f, 0.f, &air, _vehicle_riding_enclosure_open);
		velocity = zero;
		air_tick(&state, &air, 0.f, 0.f, &velocity, &frame);
		NEAR(state.velocity.i, 0.f);
		NEAR(state.velocity.j, 0.f);
		air_tick(&state, &air, .0005f, 0.f, &velocity, &frame);
		NEAR(state.velocity.i, .0005f);
		/* a take-off from a parked deck that then drives off: the way is the
		deck's, and the speed-up beyond the take-off's is not followed */
		state = taken_off(0.f, 0.f, &air, _vehicle_riding_enclosure_open);
		air_tick(&state, &air, .1f, 0.f, &velocity, &frame);
		NEAR(state.velocity.i, 0.f);
		NEAR(state.velocity.j, 0.f);
		/* a frame under 0.001 has no way of its own: the deck's; it is brought
		back to its 0.0005 along it and its sideways 0.0005 taken off */
		state = taken_off(0.f, .0005f, &air, _vehicle_riding_enclosure_open);
		air_tick(&state, &air, .1f, 0.f, &velocity, &frame);
		NEAR(state.velocity.i, .0005f);
		NEAR(state.velocity.j, 0.f);
		return 0;
	}
	CASE("enclosure-ray")
	{
		struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_state state = riding_on(&vehicle, point(0.f, 0.f, 1.f), vector(.3f, 0.f, 0.f));
		struct vehicle_riding_air air;
		real_point3d base = point(0.f, 0.f, 1.f);

		/* the jump height: a jump of 0.1 a tick under a gravity of 0.005 rises
		1 (the reach without a pill height) */
		NEAR(vehicle_riding_reach(.1f, .005f, 0.f), 1.f);
		/* riding: the air of an earlier jump cleared, no ray */
		air.have_peak = TRUE;
		air.peak = .5f;
		air.enclosure = _vehicle_riding_enclosure_enclosed;
		vehicle.updates++;
		CHECK(vehicle_riding_snapshot_step(&state, &vehicle, &base, 1.4f, .2f, FALSE, ANY_LIMIT, &step), "dropped");
		CHECK(!vehicle_riding_air_begin(&air, &state, &step, TRUE), "a ray while riding");
		CHECK(!air.have_peak && air.enclosure == _vehicle_riding_enclosure_unknown,
			"the air not cleared while riding: peak %d enclosure %d", air.have_peak, air.enclosure);
		/* in the window, airborne over the deck: the ray, on the first tick */
		state.ticks = 40;
		state.falling = 1;
		step.over_deck = TRUE;
		CHECK(vehicle_riding_air_begin(&air, &state, &step, TRUE), "no ray on the first airborne tick over the deck");
		/* a roof 0.8 over the pill's top, within the jump: enclosed */
		vehicle_riding_air_enclosure(&air, TRUE, .8f, 1.f, FALSE);
		CHECK(air.enclosure == _vehicle_riding_enclosure_enclosed, "a roof within a jump was open");
		/* kept for the jump: no ray again */
		CHECK(!vehicle_riding_air_begin(&air, &state, &step, TRUE), "the verdict cast again mid-jump");
		CHECK(air.enclosure == _vehicle_riding_enclosure_enclosed, "the verdict lost mid-jump");
		/* airborne off the deck: the peak kept */
		air.have_peak = TRUE;
		step.over_deck = FALSE;
		CHECK(!vehicle_riding_air_begin(&air, &state, &step, TRUE), "a ray off the deck");
		CHECK(air.have_peak, "the peak lost in the air off the deck");
		step.over_deck = TRUE;
		air.enclosure = _vehicle_riding_enclosure_enclosed;
		/* landed (on a crate inside): the verdict ends and the peak with it;
		the next jump decides again */
		CHECK(!vehicle_riding_air_begin(&air, &state, &step, FALSE), "a ray on the ground");
		CHECK(air.enclosure == _vehicle_riding_enclosure_unknown, "the verdict outlived the landing");
		CHECK(!air.have_peak, "a landed biped's next jump inherits the last one's peak");
		CHECK(vehicle_riding_air_begin(&air, &state, &step, TRUE), "the next jump not decided");
		/* a gun 1.5 over an open deck, higher than a jump: open */
		vehicle_riding_air_enclosure(&air, TRUE, 1.5f, 1.f, FALSE);
		CHECK(air.enclosure == _vehicle_riding_enclosure_open, "a gun over an open deck enclosed it");
		/* off the deck: the verdict ends too */
		step.over_deck = FALSE;
		CHECK(!vehicle_riding_air_begin(&air, &state, &step, TRUE), "a ray off the deck");
		CHECK(air.enclosure == _vehicle_riding_enclosure_unknown, "the verdict outlived leaving the deck");
		/* the pill's top inside the vehicle, the ray missing: enclosed; neither: open */
		vehicle_riding_air_enclosure(&air, FALSE, 0.f, 1.f, TRUE);
		CHECK(air.enclosure == _vehicle_riding_enclosure_enclosed, "a top inside the vehicle was open");
		vehicle_riding_air_enclosure(&air, FALSE, 0.f, 1.f, FALSE);
		CHECK(air.enclosure == _vehicle_riding_enclosure_open, "no hit was enclosed");
		/* a roof exactly a jump up: within it */
		vehicle_riding_air_enclosure(&air, TRUE, 1.f, 1.f, FALSE);
		CHECK(air.enclosure == _vehicle_riding_enclosure_enclosed, "a roof exactly a jump up was open");
		/* no window (dropped, riding off): cleared */
		air.have_peak = TRUE;
		CHECK(!vehicle_riding_air_begin(&air, &state, NULL, TRUE), "a ray with no window");
		CHECK(!air.have_peak, "the air kept with no window");
		/* no air (no local player): nothing */
		step.over_deck = TRUE;
		CHECK(!vehicle_riding_air_begin(NULL, &state, &step, TRUE), "a ray without air");
		/* each local player its own air, none outside them, the latch clears it */
		CHECK(vehicle_riding_player_air(0) != vehicle_riding_player_air(1), "two local players share an air");
		CHECK(vehicle_riding_player_air(-1) == NULL && vehicle_riding_player_air(HALO_PORT_MAXIMUM_NETWORK_PLAYERS) == NULL,
			"an air outside the local players");
		vehicle_riding_player_air(1)->have_peak = TRUE;
		vehicle_riding_latch();
		CHECK(!vehicle_riding_player_air(1)->have_peak, "the latch kept an air");
		return 0;
	}
	CASE("contact-crush")
	{
		/* no overlap of that vehicle left once its own motion is made: where
		the move put it, no fix and no damage */
		CHECK(vehicle_riding_contact_settle(FALSE, TRUE, FALSE) == _vehicle_riding_settle_moved,
			"a free biped fixed or crushed");
		/* left touching: one spot further out first, never the fix's blind offsets */
		CHECK(vehicle_riding_contact_settle(TRUE, FALSE, FALSE) == _vehicle_riding_settle_candidate,
			"the further spot skipped");
		/* that touching too: the fix from the moved base */
		CHECK(vehicle_riding_contact_settle(TRUE, TRUE, TRUE) == _vehicle_riding_settle_fixed,
			"a biped the fix freed was crushed");
		/* and nothing frees it: the original crush */
		CHECK(vehicle_riding_contact_settle(TRUE, TRUE, FALSE) == _vehicle_riding_settle_crushed,
			"a pinned biped spared");
		return 0;
	}
	CASE("player-keyed")
	{
		struct vehicle_riding_contact contacts[RIDING_CONTACT_RECORDS];
		real_vector3d side = vector(0.f, 1.f, 0.f);
		short last = HALO_PORT_MAXIMUM_NETWORK_PLAYERS - 1;

		/* a host's remote players and a client's remote units ride too, their
		slots past the local players': each its own */
		CHECK(vehicle_riding_player_air(4) != NULL && vehicle_riding_player_air(last) != NULL,
			"no air for a player past the local ones");
		CHECK(vehicle_riding_player_air(4) != vehicle_riding_player_air(last) &&
			vehicle_riding_player_air(0) != vehicle_riding_player_air(last), "two players share an air");
		CHECK(vehicle_riding_player_air(HALO_PORT_MAXIMUM_NETWORK_PLAYERS) == NULL &&
			vehicle_riding_player_air(NONE) == NULL, "an air past the player table");
		vehicle_riding_player_air(last)->have_peak = TRUE;
		vehicle_riding_player_air(last)->peak = .3f;
		vehicle_riding_player_air(4)->enclosure = _vehicle_riding_enclosure_enclosed;
		CHECK(!vehicle_riding_player_air(0)->have_peak &&
			vehicle_riding_player_air(0)->enclosure == _vehicle_riding_enclosure_unknown, "another player's air changed");
		CHECK(vehicle_riding_contact_record(last, VEHICLE, &side), "a remote player's touch not recorded");
		CHECK(vehicle_riding_contact_record(4, OTHER_VEHICLE, &side), "a fifth player's touch not recorded");
		CHECK(vehicle_riding_contact_take(0, contacts) == 0, "another player's touch read");
		CHECK(vehicle_riding_contact_take(last, contacts) == 1 && contacts[0].vehicle_index == VEHICLE,
			"a remote player's touch lost");
		CHECK(!vehicle_riding_contact_record(HALO_PORT_MAXIMUM_NETWORK_PLAYERS, VEHICLE, &side),
			"a touch past the player table kept");
		CHECK(vehicle_riding_contact_take(HALO_PORT_MAXIMUM_NETWORK_PLAYERS, contacts) == 0,
			"touches read past the player table");
		/* every player's records cleared at a game tick's start, and every
		player's air by the latch */
		CHECK(vehicle_riding_contact_record(last, VEHICLE, &side), "a remote player's touch not recorded");
		vehicle_riding_contacts_clear();
		CHECK(vehicle_riding_contact_take(last, contacts) == 0, "a remote player's touch kept over the tick's start");
		CHECK(vehicle_riding_contact_take(4, contacts) == 0, "a fifth player's touch kept over the tick's start");
		vehicle_riding_latch();
		CHECK(!vehicle_riding_player_air(last)->have_peak &&
			vehicle_riding_player_air(4)->enclosure == _vehicle_riding_enclosure_unknown,
			"the latch kept a remote player's air");
		return 0;
	}
	CASE("deck-make")
	{
		/* a vehicle at (1, 2, 0) facing +y (its left is -x): a rider that last
		stood at (1, 3, 0) on it, its pill's base now at (0, 2.5, 0.5) */
		struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 1.f, 2.f, 0.f, PI/2.f, 10);
		struct vehicle_riding_vehicle other = vehicle_at(OTHER_VEHICLE, 1.f, 2.f, 0.f, PI/2.f, 10);
		struct vehicle_riding_state state = riding_on(&vehicle, point(1.f, 3.f, 0.f), vector(.2f, .1f, 0.f));
		struct vehicle_riding_air air;
		struct vehicle_riding_deck deck;
		struct vehicle_riding_deck before;
		real_point3d base = point(0.f, 2.5f, .5f);
		real_vector3d world = vector(.5f, 0.f, -.1f);
		real_vector3d not_a_number = vector(NAN, 0.f, 0.f);

		/* riding: where the pill is now (not the place it last stood), in the
		node's frame, and its velocity relative to the deck */
		CHECK(vehicle_riding_deck_make(&state, &vehicle, &base, &world, NULL, NULL, &deck), "a rider's part refused");
		CHECK(deck.vehicle_index == VEHICLE && deck.node_index == NONE && deck.ticks == RIDING_TICKS && deck.flags == 0,
			"a rider's vehicle, node, ticks or flags");
		NEAR(deck.place.x, .5f);
		NEAR(deck.place.y, 1.f);
		NEAR(deck.place.z, .5f);
		NEAR(deck.velocity.i, .3f);
		NEAR(deck.velocity.j, -.1f);
		NEAR(deck.velocity.k, -.1f);
		NEAR(deck.peak, 0.f);
		/* in the air: the air frame (world) for the velocity, falling in the
		flags, and no peak yet: the frame's own speed across */
		state.ticks = 40;
		state.falling = 1;
		state.velocity = vector(.3f, .4f, -.05f);
		CHECK(vehicle_riding_deck_make(&state, &vehicle, &base, &world, NULL, NULL, &deck), "a jumper's part refused");
		CHECK(deck.ticks == 40 && deck.flags == FLAG(_vehicle_riding_deck_falling_bit), "a jumper's ticks or flags");
		NEAR(deck.velocity.i, .3f);
		NEAR(deck.velocity.j, .4f);
		NEAR(deck.velocity.k, -.05f);
		NEAR(deck.peak, .5f);
		/* its player's verdict in the flags beside falling, and the peak kept */
		memset(&air, 0, sizeof(air));
		air.enclosure = _vehicle_riding_enclosure_open;
		CHECK(vehicle_riding_deck_make(&state, &vehicle, &base, &world, &air, NULL, &deck), "an open jump's part refused");
		CHECK(deck.flags == (FLAG(_vehicle_riding_deck_falling_bit) | (1 << VEHICLE_RIDING_DECK_VERDICT_SHIFT)),
			"open: flags %u", (unsigned)deck.flags);
		NEAR(deck.peak, .5f);
		air.enclosure = _vehicle_riding_enclosure_enclosed;
		air.have_peak = TRUE;
		air.peak = .7f;
		CHECK(vehicle_riding_deck_make(&state, &vehicle, &base, &world, &air, NULL, &deck), "an enclosed jump's part refused");
		CHECK(deck.flags == (FLAG(_vehicle_riding_deck_falling_bit) | (2 << VEHICLE_RIDING_DECK_VERDICT_SHIFT)),
			"enclosed: flags %u", (unsigned)deck.flags);
		NEAR(deck.peak, .7f);
		/* nothing for the wire, and the part left as it was: another vehicle,
		another node, no window, a velocity or an air frame not finite */
		memset(&deck, 0xA5, sizeof(deck));
		before = deck;
		CHECK(!vehicle_riding_deck_make(&state, &other, &base, &world, &air, NULL, &deck), "another vehicle's part made");
		vehicle.node_index = 2;
		CHECK(!vehicle_riding_deck_make(&state, &vehicle, &base, &world, &air, NULL, &deck), "another node's part made");
		vehicle.node_index = NONE;
		state.ticks = 0;
		CHECK(!vehicle_riding_deck_make(&state, &vehicle, &base, &world, &air, NULL, &deck), "a part made with no window");
		state.ticks = RIDING_TICKS;
		state.falling = 0;
		CHECK(!vehicle_riding_deck_make(&state, &vehicle, &base, &not_a_number, NULL, NULL, &deck),
			"a rider's velocity not finite made");
		state.ticks = 40;
		state.velocity = not_a_number;
		CHECK(!vehicle_riding_deck_make(&state, &vehicle, &base, &world, NULL, NULL, &deck), "an air frame not finite made");
		CHECK(!memcmp(&before, &deck, sizeof(deck)), "a part refused but written");
		return 0;
	}
	CASE("deck-frame-carried")
	{
		/* a rider that ended its tick on a vehicle that has since been written to: the node it is carried in is the one of
		its snapshot, and nothing else of the vehicle changes */
		struct vehicle_riding_vehicle snapshot = vehicle_at(VEHICLE, 1.f, 2.f, 0.f, PI/2.f, 10);
		struct vehicle_riding_vehicle live = vehicle_at(VEHICLE, 1.4f, 2.1f, .1f, PI/2.f + .2f, 11);
		struct vehicle_riding_state state = riding_on(&snapshot, point(1.f, 3.f, 0.f), vector(.2f, .1f, 0.f));
		struct vehicle_riding_vehicle frame;

		live.translational_velocity = vector(.3f, .2f, .1f);
		vehicle_riding_deck_frame(&state, &live, &frame);
		CHECK(frame.position.x == snapshot.position.x && frame.position.y == snapshot.position.y &&
			frame.position.z == snapshot.position.z, "the frame's position is not the snapshot's");
		CHECK(frame.forward.i == snapshot.forward.i && frame.forward.j == snapshot.forward.j &&
			frame.forward.k == snapshot.forward.k, "the frame's forward is not the snapshot's");
		CHECK(frame.up.i == snapshot.up.i && frame.up.j == snapshot.up.j && frame.up.k == snapshot.up.k,
			"the frame's up is not the snapshot's");
		CHECK(frame.object_index == VEHICLE && frame.node_index == NONE && frame.updates == 11 &&
			frame.origin.x == live.origin.x && frame.origin.y == live.origin.y &&
			frame.translational_velocity.i == .3f && frame.bounding_sphere_radius == live.bounding_sphere_radius,
			"the frame is not the live vehicle's otherwise");
		return 0;
	}
	CASE("deck-frame-air")
	{
		/* in the air the place it is kept at is the take-off's, in the live pose as today */
		struct vehicle_riding_vehicle snapshot = vehicle_at(VEHICLE, 1.f, 2.f, 0.f, PI/2.f, 10);
		struct vehicle_riding_vehicle live = vehicle_at(VEHICLE, 1.4f, 2.1f, .1f, PI/2.f + .2f, 11);
		struct vehicle_riding_state state = riding_on(&snapshot, point(1.f, 3.f, 0.f), vector(.2f, .1f, 0.f));
		struct vehicle_riding_vehicle frame;

		state.ticks = RIDING_TICKS - 1;
		state.falling = 1;
		vehicle_riding_deck_frame(&state, &live, &frame);
		CHECK(frame.position.x == live.position.x && frame.position.y == live.position.y &&
			frame.forward.i == live.forward.i && frame.forward.j == live.forward.j &&
			frame.up.k == live.up.k, "a jumper's frame is not the live pose");
		return 0;
	}
	CASE("deck-frame-other-node")
	{
		struct vehicle_riding_vehicle snapshot = vehicle_at(VEHICLE, 1.f, 2.f, 0.f, PI/2.f, 10);
		struct vehicle_riding_vehicle live = vehicle_at(VEHICLE, 1.4f, 2.1f, .1f, PI/2.f + .2f, 11);
		struct vehicle_riding_state state = riding_on(&snapshot, point(1.f, 3.f, 0.f), vector(.2f, .1f, 0.f));
		struct vehicle_riding_vehicle frame;

		state.node_index = 2;
		vehicle_riding_deck_frame(&state, &live, &frame);
		CHECK(frame.position.x == live.position.x && frame.position.y == live.position.y &&
			frame.forward.i == live.forward.i && frame.up.k == live.up.k, "another node's frame is not the live pose");
		state.node_index = NONE;
		state.object_index = OTHER_VEHICLE;
		vehicle_riding_deck_frame(&state, &live, &frame);
		CHECK(frame.position.x == live.position.x && frame.position.y == live.position.y &&
			frame.forward.i == live.forward.i && frame.up.k == live.up.k, "another vehicle's frame is not the live pose");
		return 0;
	}
	CASE("deck-make-cabin")
	{
		struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 1.f, 2.f, 0.f, PI/2.f, 10);
		struct vehicle_riding_state state = riding_on(&vehicle, point(1.f, 3.f, 0.f), vector(.2f, .1f, 0.f));
		struct vehicle_riding_cabin cabin;
		struct vehicle_riding_air air;
		struct vehicle_riding_deck deck;
		real_point3d base = point(0.f, 2.5f, .5f);
		real_vector3d world = vector(.5f, 0.f, -.1f);

		memset(&air, 0, sizeof(air));
		vehicle_riding_cabin_clear(&cabin);
		vehicle_riding_cabin_begin(&cabin, BIPED, &state, TRUE);
		vehicle_riding_cabin_standing(&cabin, TRUE, 4);
		/* riding: the standing verdict too, falling clear */
		CHECK(vehicle_riding_deck_make(&state, &vehicle, &base, &world, &air, &cabin, &deck), "a rider's part refused");
		CHECK(deck.ticks == RIDING_TICKS && deck.flags == (2 << VEHICLE_RIDING_DECK_VERDICT_SHIFT), "riding: flags %u",
			(unsigned)deck.flags);
		/* its grace keeps it through misses; open once spent */
		vehicle_riding_cabin_standing(&cabin, FALSE, 1);
		vehicle_riding_deck_make(&state, &vehicle, &base, &world, &air, &cabin, &deck);
		CHECK(deck.flags == (2 << VEHICLE_RIDING_DECK_VERDICT_SHIFT), "a miss in grace sent open");
		vehicle_riding_cabin_standing(&cabin, FALSE, 1);
		vehicle_riding_deck_make(&state, &vehicle, &base, &world, &air, &cabin, &deck);
		CHECK(deck.flags == (1 << VEHICLE_RIDING_DECK_VERDICT_SHIFT), "an open cabin sent enclosed");
		/* a cabin of another vehicle sends nothing of it */
		cabin.vehicle_index = OTHER_VEHICLE;
		vehicle_riding_deck_make(&state, &vehicle, &base, &world, &air, &cabin, &deck);
		CHECK(deck.flags == 0, "another vehicle's verdict sent");
		cabin.vehicle_index = VEHICLE;
		/* a protected jump: its raw sample replaces the legacy air verdict */
		state.ticks = 40;
		state.falling = 1;
		air.enclosure = _vehicle_riding_enclosure_enclosed;
		cabin.jump = _vehicle_riding_enclosure_open;
		vehicle_riding_deck_make(&state, &vehicle, &base, &world, &air, &cabin, &deck);
		CHECK(deck.flags == (FLAG(_vehicle_riding_deck_falling_bit) | (1 << VEHICLE_RIDING_DECK_VERDICT_SHIFT) |
			FLAG(_vehicle_riding_deck_cabin_bit)), "the legacy verdict over the protected jump's: flags %u", (unsigned)deck.flags);
		/* a shallow jump: the legacy air verdict, as before */
		cabin.jump = _vehicle_riding_enclosure_unknown;
		vehicle_riding_deck_make(&state, &vehicle, &base, &world, &air, &cabin, &deck);
		CHECK(deck.flags == (FLAG(_vehicle_riding_deck_falling_bit) | (2 << VEHICLE_RIDING_DECK_VERDICT_SHIFT)),
			"a shallow jump's verdict lost: flags %u", (unsigned)deck.flags);
		return 0;
	}
	CASE("deck-make-cabin-bit")
	{
		struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 1.f, 2.f, 0.f, PI/2.f, 10);
		struct vehicle_riding_state state = riding_on(&vehicle, point(1.f, 3.f, 0.f), vector(.2f, .1f, 0.f));
		struct vehicle_riding_cabin cabin;
		struct vehicle_riding_air air;
		struct vehicle_riding_deck deck;
		real_point3d base = point(0.f, 2.5f, .5f);
		real_vector3d world = vector(.5f, 0.f, -.1f);
		byte jump_open = (byte)(FLAG(_vehicle_riding_deck_falling_bit) | (1 << VEHICLE_RIDING_DECK_VERDICT_SHIFT));

		memset(&air, 0, sizeof(air));
		air.enclosure = _vehicle_riding_enclosure_enclosed;
		vehicle_riding_cabin_clear(&cabin);
		vehicle_riding_cabin_begin(&cabin, BIPED, &state, TRUE);
		/* a protected jump's verdict is the cabin's: the bit rides beside it */
		state.ticks = 40;
		state.falling = 1;
		cabin.jump = _vehicle_riding_enclosure_open;
		CHECK(vehicle_riding_deck_make(&state, &vehicle, &base, &world, &air, &cabin, &deck), "a protected jump's part refused");
		CHECK(deck.flags == (jump_open | FLAG(_vehicle_riding_deck_cabin_bit)), "protected jump: flags %u", (unsigned)deck.flags);
		/* a shallow jump's legacy verdict is none */
		cabin.jump = _vehicle_riding_enclosure_unknown;
		CHECK(vehicle_riding_deck_make(&state, &vehicle, &base, &world, &air, &cabin, &deck), "a shallow jump's part refused");
		CHECK(deck.flags == (FLAG(_vehicle_riding_deck_falling_bit) | (2 << VEHICLE_RIDING_DECK_VERDICT_SHIFT)),
			"shallow jump: flags %u", (unsigned)deck.flags);
		/* the cabin of another vehicle is no evidence either */
		cabin.jump = _vehicle_riding_enclosure_open;
		cabin.vehicle_index = OTHER_VEHICLE;
		CHECK(vehicle_riding_deck_make(&state, &vehicle, &base, &world, &air, &cabin, &deck), "another cabin's part refused");
		CHECK(!TEST_FLAG(deck.flags, _vehicle_riding_deck_cabin_bit), "another vehicle's cabin sent as evidence");
		cabin.vehicle_index = VEHICLE;
		/* the host's word for the jump (its cabin evidence) is the verdict sent, and the cabin's */
		cabin.jump = _vehicle_riding_enclosure_unknown;
		CHECK(vehicle_riding_cabin_take_host(&cabin, BIPED, &state, VEHICLE, _vehicle_riding_enclosure_open,
			_vehicle_riding_cabin_sample_jump, TRUE, TRUE), "the host's jump verdict refused");
		CHECK(vehicle_riding_deck_make(&state, &vehicle, &base, &world, &air, &cabin, &deck), "a host-judged jump's part refused");
		CHECK(deck.flags == (jump_open | FLAG(_vehicle_riding_deck_cabin_bit)), "host-judged jump: flags %u", (unsigned)deck.flags);
		/* riding: never, a take-off's tick (still ticks 60) included */
		vehicle_riding_cabin_clear(&cabin);
		state.ticks = RIDING_TICKS;
		state.falling = 0;
		vehicle_riding_cabin_begin(&cabin, BIPED, &state, TRUE);
		vehicle_riding_cabin_standing(&cabin, TRUE, 4);
		CHECK(vehicle_riding_deck_make(&state, &vehicle, &base, &world, &air, &cabin, &deck), "a rider's part refused");
		CHECK(!TEST_FLAG(deck.flags, _vehicle_riding_deck_cabin_bit), "a rider's part with the bit");
		vehicle_riding_cabin_take_off(&cabin, &state, BIPED);
		cabin.jump = _vehicle_riding_enclosure_open;
		CHECK(vehicle_riding_deck_make(&state, &vehicle, &base, &world, &air, &cabin, &deck), "a take-off tick's part refused");
		CHECK(deck.ticks == RIDING_TICKS && !TEST_FLAG(deck.flags, _vehicle_riding_deck_cabin_bit),
			"the bit at ticks 60: flags %u", (unsigned)deck.flags);
		return 0;
	}
	CASE("deck-values")
	{
		struct vehicle_riding_deck deck;
		struct vehicle_riding_deck valid;
		short ticks[6] = { 1, 59, 60, 0, 61, -1 };
		short index;

		memset(&valid, 0, sizeof(valid));
		valid.vehicle_index = VEHICLE;
		valid.node_index = NONE;
		valid.ticks = RIDING_TICKS;
		CHECK(vehicle_riding_deck_values_valid(&valid), "a whole part refused");
		/* the window's ticks: 1 to RIDING_TICKS only */
		for (index = 0; index < 6; index++)
		{
			deck = valid;
			deck.ticks = (char)ticks[index];
			CHECK(vehicle_riding_deck_values_valid(&deck) == (index < 3), "ticks %d", ticks[index]);
		}
		/* falling and each verdict up to enclosed; verdict 3 or another bit: no */
		deck = valid;
		deck.flags = FLAG(_vehicle_riding_deck_falling_bit) | (2 << VEHICLE_RIDING_DECK_VERDICT_SHIFT);
		CHECK(vehicle_riding_deck_values_valid(&deck), "falling and enclosed refused");
		deck.flags = 3 << VEHICLE_RIDING_DECK_VERDICT_SHIFT;
		CHECK(!vehicle_riding_deck_values_valid(&deck), "a verdict of 3 kept");
		deck.flags = 8;
		CHECK(!vehicle_riding_deck_values_valid(&deck), "another flag kept");
		/* finite place, velocity and peak, the peak not below 0 */
		deck = valid;
		deck.place.y = NAN;
		CHECK(!vehicle_riding_deck_values_valid(&deck), "a place not a number kept");
		deck = valid;
		deck.velocity.k = INFINITY;
		CHECK(!vehicle_riding_deck_values_valid(&deck), "an infinite velocity kept");
		deck = valid;
		deck.peak = INFINITY;
		CHECK(!vehicle_riding_deck_values_valid(&deck), "an infinite peak kept");
		deck.peak = -.001f;
		CHECK(!vehicle_riding_deck_values_valid(&deck), "a peak below 0 kept");
		/* a vehicle named: not NONE, its identifier not 0 (a lookup by it would
		take any datum in its slot, data.c's datum_try_and_get) */
		deck = valid;
		deck.vehicle_index = NONE;
		CHECK(!vehicle_riding_deck_values_valid(&deck), "no vehicle kept");
		deck.vehicle_index = 0x00000005;
		CHECK(!vehicle_riding_deck_values_valid(&deck), "an identifier of 0 kept");
		CHECK(vehicle_riding_deck_vehicle_named(VEHICLE) && !vehicle_riding_deck_vehicle_named(0x00000005) &&
			!vehicle_riding_deck_vehicle_named(NONE), "named");
		return 0;
	}
	CASE("deck-node")
	{
		struct vehicle_riding_deck deck;

		memset(&deck, 0, sizeof(deck));
		deck.vehicle_index = VEHICLE;
		deck.ticks = RIDING_TICKS;
		/* NONE (the vehicle's origin) with any nodes, and the last node; never
		past them, below 0, or with the vehicle not there */
		deck.node_index = NONE;
		CHECK(vehicle_riding_deck_valid(&deck, 0), "the origin refused");
		CHECK(!vehicle_riding_deck_valid(&deck, NONE), "a vehicle not there kept");
		deck.node_index = 2;
		CHECK(vehicle_riding_deck_valid(&deck, 3), "the last node refused");
		CHECK(!vehicle_riding_deck_valid(&deck, 2), "a node past the vehicle's kept");
		deck.node_index = -2;
		CHECK(!vehicle_riding_deck_valid(&deck, 3), "a node below 0 kept");
		/* the values checked too */
		deck.node_index = 0;
		deck.ticks = 0;
		CHECK(!vehicle_riding_deck_valid(&deck, 3), "a part with no window kept");
		return 0;
	}
	CASE("network-gate")
	{
		short connections[2] = { _game_connection_network_server, _game_connection_network_client };
		short index;

		for (index = 0; index < 2; index++)
		{
			connection = connections[index];
			/* the host's switch decides, whatever this machine's own setting */
			setting_riding = 0;
			test_game.cooperative_flags = FLAG(_network_game_cooperative_vehicle_riding_bit);
			vehicle_riding_map_decide();
			vehicle_riding_latch();
			CHECK(vehicle_riding_enabled(), "the host's switch on, off here: off (connection %d)", connection);
			CHECK(vehicle_riding_applies(TRUE, FALSE) && !vehicle_riding_applies(FALSE, FALSE) &&
				!vehicle_riding_applies(TRUE, TRUE), "a living player only (connection %d)", connection);
			setting_riding = 1;
			test_game.cooperative_flags = FLAG(_network_game_cooperative_no_player_collisions_bit);
			vehicle_riding_map_decide();
			vehicle_riding_latch();
			CHECK(!vehicle_riding_enabled(), "the host's switch off, on here: on (connection %d)", connection);
			/* no game (between sessions), and the editor: off */
			test_game.cooperative_flags = FLAG(_network_game_cooperative_vehicle_riding_bit);
			test_game_pointer = NULL;
			vehicle_riding_map_decide();
			CHECK(!vehicle_riding_enabled(), "on with no game (connection %d)", connection);
			test_game_pointer = &test_game;
			vehicle_riding_map_decide();
			in_editor = TRUE;
			CHECK(!vehicle_riding_enabled(), "on in the editor (connection %d)", connection);
			in_editor = FALSE;
		}
		/* film playback: off, whatever the switch (and the decision made before) */
		test_game.cooperative_flags = FLAG(_network_game_cooperative_vehicle_riding_bit);
		connection = _game_connection_network_server;
		vehicle_riding_map_decide();
		connection = _game_connection_film_playback;
		CHECK(!vehicle_riding_enabled(), "on in film playback");
		connection = _game_connection_local;
		test_game.cooperative_flags = 0;
		return 0;
	}

	CASE("cabin-tilt")
	{
		/* a biped standing on slopes to 60 degrees (m = cos 60): protection starts past 45 */
		real m = .5f;
		real s40 = (real)sin(40.*PI/180.);
		real c40 = (real)cos(40.*PI/180.);
		real_vector3d up50 = vector((real)sin(50.*PI/180.), 0.f, (real)cos(50.*PI/180.));
		real_vector3d at45 = vector(.70710678f, 0.f, .70710678f);
		real_vector3d pitch40 = vector(s40, 0.f, c40);
		real_vector3d roll40 = vector(0.f, s40, c40);
		real_vector3d both40 = vector(s40*c40, s40, c40*c40);
		real_vector3d up35 = vector((real)sin(35.*PI/180.), 0.f, (real)cos(35.*PI/180.));
		real_vector3d long50 = vector(3.f*up50.i, 0.f, 3.f*up50.k);
		real_vector3d none = vector(0.f, 0.f, 0.f);
		real_vector3d broken = vector(NAN, NAN, NAN);

		CHECK(vehicle_riding_cabin_tilted(&up50, m), "50 degrees is past 45 within a 60 degree limit");
		CHECK(!vehicle_riding_cabin_tilted(&up, m), "level tilted");
		/* equality is within: a limit equal to this up's own cosine (exact, whatever the rounding of its length) */
		CHECK(!vehicle_riding_cabin_tilted(&at45, vehicle_riding_cabin_cosine(&at45)) &&
			!vehicle_riding_cabin_tilted(&pitch40, vehicle_riding_cabin_cosine(&pitch40)), "equality is within");
		/* pitch 40 or roll 40 alone within; both at once past (cos 40 cos 40 under cos 45) */
		CHECK(!vehicle_riding_cabin_tilted(&pitch40, m) && !vehicle_riding_cabin_tilted(&roll40, m), "40 alone tilted");
		CHECK(vehicle_riding_cabin_tilted(&both40, m), "pitch 40 and roll 40 together within");
		/* a limit steeper than 45 degrees (m = cos 30) is the start itself */
		CHECK(vehicle_riding_cabin_tilted(&up35, .8660254f) && !vehicle_riding_cabin_tilted(&up35, m), "the limit cos 30");
		CHECK(vehicle_riding_cabin_tilted(&long50, m), "an up not of length 1");
		/* no up, or one not a number (a physics fault): never tilted */
		CHECK(!vehicle_riding_cabin_tilted(&none, m) && !vehicle_riding_cabin_tilted(&broken, m), "a broken up tilted");
		/* the exception needs every term */
		CHECK(vehicle_riding_cabin_exception(TRUE, TRUE, TRUE, &up50, m), "an enclosed tilted rider unprotected");
		CHECK(!vehicle_riding_cabin_exception(FALSE, TRUE, TRUE, &up50, m) &&
			!vehicle_riding_cabin_exception(TRUE, FALSE, TRUE, &up50, m) &&
			!vehicle_riding_cabin_exception(TRUE, TRUE, FALSE, &up50, m) &&
			!vehicle_riding_cabin_exception(TRUE, TRUE, TRUE, &pitch40, m), "protected without every term");
		return 0;
	}
	CASE("axis-hysteresis")
	{
		struct vehicle_riding_cabin cabin;
		struct vehicle_riding_step tick_step;
		real m = .5f;
		real_vector3d axis;
		real_vector3d u;
		real_point3d base = point(1.f, 2.f, 3.f);
		real_point3d ceiling;
		real_point3d floor_start;
		short flips = 0;
		short tick;
		boolean last;

		vehicle_riding_cabin_clear(&cabin);
		/* a new record at the limit (this up's own cosine, exact): world up */
		u = vector((real)sqrt(1. - .25), 0.f, .5f);
		vehicle_riding_cabin_axis(&cabin, &u, vehicle_riding_cabin_cosine(&u), &axis);
		CHECK(!cabin.body_axis && axis.k == 1.f && axis.i == 0.f, "a new record at the limit off world up");
		/* past it: the vehicle's up at once */
		u = vector((real)sqrt(1. - .499*.499), 0.f, .499f);
		vehicle_riding_cabin_axis(&cabin, &u, m, &axis);
		CHECK(cabin.body_axis, "below the limit on world up");
		NEAR(axis.k, .499f);
		/* rocking about the limit for 20 ticks: one axis */
		last = cabin.body_axis;
		for (tick = 0; tick < 20; tick++)
		{
			real k = tick & 1 ? m + .001f : m - .001f;

			u = vector((real)sqrt(1. - (double)(k*k)), 0.f, k);
			vehicle_riding_cabin_axis(&cabin, &u, m, &axis);
			flips += cabin.body_axis != last;
			last = cabin.body_axis;
		}
		CHECK(flips == 0, "%d flips about the limit", flips);
		/* inside the band: still the vehicle's; at its edge: world up again */
		u = vector((real)sqrt(1. - .509*.509), 0.f, .509f);
		vehicle_riding_cabin_axis(&cabin, &u, m, &axis);
		CHECK(cabin.body_axis, "back on world up inside the band");
		/* (a hair past the edge: an up's length rounds) */
		u = vector(0.f, 0.f, m + RIDING_CABIN_AXIS_BAND + .00001f);
		u.i = (real)sqrt(1. - (double)(u.k*u.k));
		vehicle_riding_cabin_axis(&cabin, &u, m, &axis);
		CHECK(!cabin.body_axis && axis.k == 1.f, "the band's edge kept the vehicle's up");
		/* a limit near level: the band stops at 1 */
		u = vector((real)sqrt(1. - .99*.99), 0.f, .99f);
		vehicle_riding_cabin_axis(&cabin, &u, .995f, &axis);
		CHECK(cabin.body_axis, "below .995");
		vehicle_riding_cabin_axis(&cabin, &up, .995f, &axis);
		CHECK(!cabin.body_axis, "level kept the vehicle's up");
		/* upside down: the vehicle's up, pointing down; the ceiling's ray from the base, the floor's from the top */
		u = vector(0.f, 0.f, -1.f);
		vehicle_riding_cabin_axis(&cabin, &u, m, &axis);
		CHECK(cabin.body_axis && axis.k == -1.f, "inverted");
		vehicle_riding_cabin_ends(&base, .6f, &axis, &ceiling, &floor_start);
		CHECK(ceiling.z == base.z && floor_start.z == base.z + .6f, "inverted ends: ceiling %g floor %g", (double)ceiling.z,
			(double)floor_start.z);
		vehicle_riding_cabin_ends(&base, .6f, &up, &ceiling, &floor_start);
		CHECK(ceiling.z == base.z + .6f && floor_start.z == base.z && ceiling.x == base.x, "upright ends");
		/* the current axis is the latch's, unchanged */
		vehicle_riding_cabin_current_axis(&cabin, &u, &axis);
		CHECK(cabin.body_axis && axis.k == -1.f, "the current axis");
		/* the body's up, never a node's (a turret on its side) */
		memset(&tick_step, 0, sizeof(tick_step));
		tick_step.vehicle.body_up = up;
		tick_step.vehicle.up = vector(1.f, 0.f, 0.f);
		CHECK(vehicle_riding_cabin_up(&tick_step)->k == 1.f, "a node's up taken for the body's");
		return 0;
	}
	CASE("cabin-verdict")
	{
		struct vehicle_riding_cabin_rays rays;

		memset(&rays, 0, sizeof(rays));
		rays.valid = TRUE;
		rays.ceiling = TRUE;
		rays.ceiling_distance = .8f;
		rays.floor = TRUE;
		rays.floor_distance = .3f;
		CHECK(vehicle_riding_cabin_hit(&rays, 1.f, .6f), "a roof within a jump over a floor: open");
		/* a roof exactly a jump up, a floor exactly a jump and a pill down: within */
		rays.ceiling_distance = 1.f;
		rays.floor_distance = 1.6f;
		CHECK(vehicle_riding_cabin_hit(&rays, 1.f, .6f), "the exact reaches");
		rays.ceiling_distance = 1.01f;
		CHECK(!vehicle_riding_cabin_hit(&rays, 1.f, .6f), "a roof past a jump");
		rays.ceiling_distance = .8f;
		rays.floor_distance = 1.61f;
		CHECK(!vehicle_riding_cabin_hit(&rays, 1.f, .6f), "a floor past the reach");
		/* a roof alone, a belly overhead: open */
		rays.floor_distance = .3f;
		rays.floor = FALSE;
		CHECK(!vehicle_riding_cabin_hit(&rays, 1.f, .6f), "a roof without a floor");
		/* the top inside: still needs the floor */
		rays.ceiling = FALSE;
		rays.inside = TRUE;
		CHECK(!vehicle_riding_cabin_hit(&rays, 1.f, .6f), "inside without a floor");
		rays.floor = TRUE;
		CHECK(vehicle_riding_cabin_hit(&rays, 1.f, .6f), "inside over a floor");
		/* no jump to rise by: the inside test alone, the ray never counts */
		rays.inside = FALSE;
		rays.ceiling = TRUE;
		rays.ceiling_distance = 0.f;
		CHECK(!vehicle_riding_cabin_hit(&rays, 0.f, .6f), "J 0: the ray counted");
		rays.inside = TRUE;
		CHECK(vehicle_riding_cabin_hit(&rays, 0.f, .6f), "J 0: inside over a floor");
		/* no model or pose: open */
		rays.valid = FALSE;
		CHECK(!vehicle_riding_cabin_hit(&rays, 1.f, .6f), "no model enclosed");
		return 0;
	}
	CASE("grace-jump")
	{
		struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_state state = riding_on(&vehicle, point(0.f, 0.f, 0.f), zero);
		struct vehicle_riding_cabin cabin;
		struct vehicle_riding_air air;
		struct vehicle_riding_step tick_step;
		short miss;

		memset(&tick_step, 0, sizeof(tick_step));
		tick_step.vehicle = vehicle;
		tick_step.updates = 1;
		tick_step.player_ticks = 1.f;
		memset(&air, 0, sizeof(air));
		vehicle_riding_cabin_clear(&cabin);
		vehicle_riding_cabin_begin(&cabin, BIPED, &state, TRUE);
		/* on the deck: a standing sample, sliding in the air too */
		CHECK(vehicle_riding_cabin_phase(&cabin, &state, &tick_step, TRUE, TRUE) == _vehicle_riding_cabin_sample_standing,
			"no standing sample");
		/* an initial miss is open at once */
		vehicle_riding_cabin_standing(&cabin, FALSE, 4);
		CHECK(cabin.standing == _vehicle_riding_enclosure_open, "an initial miss enclosed");
		/* a hit encloses; misses 1-4 keep it, miss 5 opens */
		vehicle_riding_cabin_standing(&cabin, TRUE, 4);
		for (miss = 1; miss <= 4; miss++)
		{
			vehicle_riding_cabin_standing(&cabin, FALSE, 4);
			CHECK(cabin.standing == _vehicle_riding_enclosure_enclosed, "miss %d opened", miss);
		}
		vehicle_riding_cabin_standing(&cabin, FALSE, 4);
		CHECK(cabin.standing == _vehicle_riding_enclosure_open, "miss 5 kept it");
		/* a hit between starts the count again */
		vehicle_riding_cabin_standing(&cabin, TRUE, 4);
		for (miss = 0; miss < 3; miss++)
			vehicle_riding_cabin_standing(&cabin, FALSE, 4);
		vehicle_riding_cabin_standing(&cabin, TRUE, 4);
		for (miss = 0; miss < 4; miss++)
			vehicle_riding_cabin_standing(&cabin, FALSE, 4);
		CHECK(cabin.standing == _vehicle_riding_enclosure_enclosed, "a hit did not reset the count");
		/* grace 0: the first miss opens */
		vehicle_riding_cabin_standing(&cabin, TRUE, 0);
		vehicle_riding_cabin_standing(&cabin, FALSE, 0);
		CHECK(cabin.standing == _vehicle_riding_enclosure_open, "grace 0 kept a miss");
		/* double speed's skipped vehicle update: the player tick samples all the same */
		tick_step.updates = 0;
		tick_step.player_ticks = 0.f;
		CHECK(vehicle_riding_cabin_phase(&cabin, &state, &tick_step, FALSE, TRUE) == _vehicle_riding_cabin_sample_standing,
			"the skipped tick not sampled");
		tick_step.updates = 1;
		tick_step.player_ticks = 1.f;
		/* enclosed with two misses, then a steep jump: one raw sample at the take-off */
		vehicle_riding_cabin_standing(&cabin, TRUE, 4);
		vehicle_riding_cabin_standing(&cabin, FALSE, 4);
		vehicle_riding_cabin_standing(&cabin, FALSE, 4);
		vehicle_riding_cabin_take_off(&cabin, &state, BIPED);
		CHECK(cabin.take_off, "no take-off marked");
		CHECK(vehicle_riding_cabin_phase(&cabin, &state, &tick_step, TRUE, TRUE) == _vehicle_riding_cabin_sample_jump,
			"no raw sample at a protected take-off");
		vehicle_riding_cabin_jump(&cabin, FALSE);
		/* standing grace never becomes the jump's verdict */
		CHECK(vehicle_riding_cabin_verdict(&cabin, &state, &air, TRUE) == _vehicle_riding_enclosure_open,
			"the standing grace froze as the jump's verdict");
		/* in the window: no sample again, the miss count frozen */
		cabin.take_off = FALSE;
		state.ticks = 59;
		CHECK(vehicle_riding_cabin_phase(&cabin, &state, &tick_step, TRUE, TRUE) == _vehicle_riding_cabin_sample_none,
			"a raw miss sampled again until a hit");
		CHECK(vehicle_riding_cabin_verdict(&cabin, &state, &air, TRUE) == _vehicle_riding_enclosure_open && cabin.misses == 2,
			"the jump changed the verdict or the count (%d)", cabin.misses);
		/* landed: the count resumes from 2, the jump's verdict over */
		state.ticks = RIDING_TICKS;
		CHECK(vehicle_riding_cabin_phase(&cabin, &state, &tick_step, FALSE, TRUE) == _vehicle_riding_cabin_sample_standing,
			"no standing sample on landing");
		vehicle_riding_cabin_standing(&cabin, FALSE, 4);
		vehicle_riding_cabin_standing(&cabin, FALSE, 4);
		CHECK(cabin.standing == _vehicle_riding_enclosure_enclosed && cabin.jump == _vehicle_riding_enclosure_unknown,
			"misses 3 and 4 over the jump");
		vehicle_riding_cabin_standing(&cabin, FALSE, 4);
		CHECK(cabin.standing == _vehicle_riding_enclosure_open, "the jump refreshed the grace");
		/* a shallow jump: no sample; on the wire its legacy air verdict, never cabin evidence nor the standing one */
		vehicle_riding_cabin_standing(&cabin, TRUE, 4);
		vehicle_riding_cabin_take_off(&cabin, &state, BIPED);
		CHECK(vehicle_riding_cabin_phase(&cabin, &state, &tick_step, TRUE, FALSE) == _vehicle_riding_cabin_sample_none,
			"a shallow take-off sampled");
		cabin.take_off = FALSE;
		state.ticks = 59;
		air.enclosure = _vehicle_riding_enclosure_enclosed;
		CHECK(vehicle_riding_cabin_verdict(&cabin, &state, &air, FALSE) == _vehicle_riding_enclosure_enclosed,
			"the shallow jump's wire verdict");
		CHECK(vehicle_riding_cabin_verdict(&cabin, &state, &air, TRUE) == _vehicle_riding_enclosure_unknown,
			"a legacy single ray taken as cabin evidence");
		air.enclosure = _vehicle_riding_enclosure_open;
		CHECK(vehicle_riding_cabin_verdict(&cabin, &state, &air, FALSE) == _vehicle_riding_enclosure_open,
			"the standing grace over a shallow jump's verdict");
		/* that jump tilting into protection: one raw sample at its first protected tick, then none */
		CHECK(vehicle_riding_cabin_phase(&cabin, &state, &tick_step, TRUE, TRUE) == _vehicle_riding_cabin_sample_jump,
			"no sample crossing into protection");
		vehicle_riding_cabin_jump(&cabin, TRUE);
		CHECK(vehicle_riding_cabin_verdict(&cabin, &state, &air, TRUE) == _vehicle_riding_enclosure_enclosed &&
			vehicle_riding_cabin_verdict(&cabin, &state, &air, FALSE) == _vehicle_riding_enclosure_enclosed,
			"the protected sample not the verdict");
		CHECK(vehicle_riding_cabin_phase(&cabin, &state, &tick_step, TRUE, TRUE) == _vehicle_riding_cabin_sample_none,
			"sampled twice in a jump");
		/* in the window standing on something else: no jump sample */
		state.ticks = RIDING_TICKS;
		vehicle_riding_cabin_standing(&cabin, TRUE, 4);
		state.ticks = 40;
		CHECK(vehicle_riding_cabin_phase(&cabin, &state, &tick_step, FALSE, TRUE) == _vehicle_riding_cabin_sample_none,
			"a jump sample on the ground");
		/* no window: nothing */
		CHECK(vehicle_riding_cabin_phase(&cabin, &state, NULL, TRUE, TRUE) == _vehicle_riding_cabin_sample_none,
			"a sample with no window");
		return 0;
	}
	CASE("cabin-keys")
	{
		struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_state state = riding_on(&vehicle, point(0.f, 0.f, 0.f), zero);
		struct vehicle_riding_cabin *cabin = vehicle_riding_player_cabin(4);
		struct vehicle_riding_cabin *last = vehicle_riding_player_cabin(127);

		/* each player its own record, past the local players; none past the table */
		CHECK(cabin && last && cabin != last, "no record for players 4 and 127");
		CHECK(!vehicle_riding_player_cabin(HALO_PORT_MAXIMUM_NETWORK_PLAYERS) && !vehicle_riding_player_cabin(-1),
			"a record past the table");
		vehicle_riding_cabin_begin(cabin, BIPED, &state, TRUE);
		CHECK(cabin->unit_index == BIPED && cabin->vehicle_index == VEHICLE, "not keyed");
		vehicle_riding_cabin_standing(cabin, TRUE, 4);
		cabin->have_anchor = TRUE;
		/* the same unit and vehicle: kept */
		vehicle_riding_cabin_begin(cabin, BIPED, &state, TRUE);
		CHECK(cabin->standing == _vehicle_riding_enclosure_enclosed && cabin->have_anchor, "lost in an unbroken window");
		CHECK(last->standing == _vehicle_riding_enclosure_unknown, "another player's changed");
		/* the window lost (a guard, a teleport, the vehicle gone): cleared */
		vehicle_riding_cabin_begin(cabin, BIPED, &state, FALSE);
		CHECK(cabin->unit_index == NONE && cabin->standing == _vehicle_riding_enclosure_unknown && !cabin->have_anchor,
			"kept past the window");
		/* another vehicle: started again for it */
		vehicle_riding_cabin_begin(cabin, BIPED, &state, TRUE);
		vehicle_riding_cabin_standing(cabin, TRUE, 4);
		state.object_index = OTHER_VEHICLE;
		vehicle_riding_cabin_begin(cabin, BIPED, &state, TRUE);
		CHECK(cabin->vehicle_index == OTHER_VEHICLE && cabin->standing == _vehicle_riding_enclosure_unknown,
			"kept across vehicles");
		/* another unit (the player's new body): started again */
		vehicle_riding_cabin_standing(cabin, TRUE, 4);
		vehicle_riding_cabin_begin(cabin, OTHER_BIPED, &state, TRUE);
		CHECK(cabin->unit_index == OTHER_BIPED && cabin->standing == _vehicle_riding_enclosure_unknown, "kept across units");
		/* no window ticks: cleared */
		state.ticks = 0;
		vehicle_riding_cabin_begin(cabin, OTHER_BIPED, &state, TRUE);
		CHECK(cabin->unit_index == NONE, "kept with no window");
		/* acquisition: keyed and sampled once; a verdict it has carries on */
		state = riding_on(&vehicle, point(0.f, 0.f, 0.f), zero);
		CHECK(vehicle_riding_cabin_acquire(cabin, BIPED, &state) && cabin->vehicle_index == VEHICLE, "no seed on acquisition");
		vehicle_riding_cabin_standing(cabin, TRUE, 4);
		CHECK(!vehicle_riding_cabin_acquire(cabin, BIPED, &state), "a known verdict sampled again");
		state.ticks = 59;
		CHECK(!vehicle_riding_cabin_acquire(cabin, BIPED, &state), "an acquisition in the window");
		/* a take-off only from the deck, only by its own unit */
		vehicle_riding_cabin_take_off(cabin, &state, BIPED);
		CHECK(!cabin->take_off, "a take-off from the window");
		state.ticks = RIDING_TICKS;
		vehicle_riding_cabin_take_off(cabin, &state, OTHER_BIPED);
		CHECK(!cabin->take_off, "another unit's take-off");
		vehicle_riding_cabin_take_off(cabin, &state, BIPED);
		CHECK(cabin->take_off, "no take-off from the deck");
		/* the latch clears every record */
		cabin->have_anchor = TRUE;
		last->unit_index = BIPED;
		vehicle_riding_latch();
		CHECK(cabin->unit_index == NONE && !cabin->have_anchor && !cabin->take_off && last->unit_index == NONE,
			"a record outlived the latch");
		return 0;
	}
	CASE("cabin-host-verdict")
	{
		struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_state state = riding_on(&vehicle, point(0.f, 0.f, 0.f), zero);
		struct vehicle_riding_state none = state;
		struct vehicle_riding_cabin cabin;
		real_vector3d tilted = vector(0.f, .8f, .6f);
		short miss;

		vehicle_riding_cabin_clear(&cabin);
		vehicle_riding_cabin_begin(&cabin, BIPED, &state, TRUE);
		none.ticks = 0;
		/* another vehicle, no window, an old window of the same vehicle: nothing seeded */
		CHECK(!vehicle_riding_cabin_take_host(&cabin, BIPED, &state, OTHER_VEHICLE, _vehicle_riding_enclosure_enclosed,
			_vehicle_riding_cabin_sample_standing, FALSE, TRUE) && !cabin.host, "another vehicle's verdict taken");
		CHECK(!vehicle_riding_cabin_take_host(&cabin, BIPED, &none, VEHICLE, _vehicle_riding_enclosure_enclosed,
			_vehicle_riding_cabin_sample_standing, FALSE, TRUE) && !cabin.host, "a verdict taken with no window");
		CHECK(!vehicle_riding_cabin_take_host(&cabin, BIPED, &state, VEHICLE, _vehicle_riding_enclosure_enclosed,
			_vehicle_riding_cabin_sample_standing, FALSE, FALSE) && !cabin.host, "an old window's verdict taken");
		/* the host's standing verdict, then ten local misses: it stands, and so does the exception */
		CHECK(vehicle_riding_cabin_take_host(&cabin, BIPED, &state, VEHICLE, _vehicle_riding_enclosure_enclosed,
			_vehicle_riding_cabin_sample_standing, FALSE, TRUE), "the host's standing verdict refused");
		CHECK(cabin.host && cabin.host_verdict == _vehicle_riding_enclosure_enclosed &&
			cabin.host_phase == _vehicle_riding_cabin_sample_standing, "the host's verdict not kept");
		for (miss = 0; miss < 10; miss++)
			vehicle_riding_cabin_standing(&cabin, FALSE, 4);
		CHECK(cabin.standing == _vehicle_riding_enclosure_open, "the local sample is not recorded");
		CHECK(vehicle_riding_cabin_verdict(&cabin, &state, NULL, TRUE) == _vehicle_riding_enclosure_enclosed,
			"local misses aged the host's verdict");
		CHECK(vehicle_riding_cabin_exception(TRUE, TRUE,
			vehicle_riding_cabin_verdict(&cabin, &state, NULL, TRUE) == _vehicle_riding_enclosure_enclosed, &tilted, .5f),
			"the host's enclosed verdict gave no exception");
		/* a host verdict of unknown says nothing: the local sample goes again */
		CHECK(!vehicle_riding_cabin_take_host(&cabin, BIPED, &state, VEHICLE, _vehicle_riding_enclosure_unknown,
			_vehicle_riding_cabin_sample_standing, FALSE, TRUE) && !cabin.host, "an unknown verdict kept as the host's");
		CHECK(vehicle_riding_cabin_verdict(&cabin, &state, NULL, TRUE) == _vehicle_riding_enclosure_open, "no local verdict");
		/* a take-off ends a standing verdict: the jump starts unknown */
		vehicle_riding_cabin_take_host(&cabin, BIPED, &state, VEHICLE, _vehicle_riding_enclosure_enclosed,
			_vehicle_riding_cabin_sample_standing, FALSE, TRUE);
		vehicle_riding_cabin_take_off(&cabin, &state, BIPED);
		CHECK(!cabin.host, "a take-off kept the standing verdict");
		/* a cabin not keyed yet is keyed by the take; another unit's is started again */
		vehicle_riding_cabin_clear(&cabin);
		CHECK(vehicle_riding_cabin_take_host(&cabin, BIPED, &state, VEHICLE, _vehicle_riding_enclosure_open,
			_vehicle_riding_cabin_sample_standing, FALSE, TRUE) && cabin.unit_index == BIPED && cabin.vehicle_index == VEHICLE,
			"an unkeyed cabin not keyed");
		CHECK(vehicle_riding_cabin_take_host(&cabin, OTHER_BIPED, &state, VEHICLE, _vehicle_riding_enclosure_enclosed,
			_vehicle_riding_cabin_sample_standing, FALSE, TRUE) && cabin.unit_index == OTHER_BIPED &&
			cabin.host_verdict == _vehicle_riding_enclosure_enclosed, "another unit's cabin kept");
		/* a re-key clears the host's verdict, the drop does too */
		vehicle_riding_cabin_begin(&cabin, BIPED, &state, TRUE);
		CHECK(!cabin.host, "a re-key kept the host's verdict");
		vehicle_riding_cabin_take_host(&cabin, BIPED, &state, VEHICLE, _vehicle_riding_enclosure_enclosed,
			_vehicle_riding_cabin_sample_standing, FALSE, TRUE);
		vehicle_riding_cabin_begin(&cabin, BIPED, &state, TRUE);
		CHECK(cabin.host, "a tick's begin lost the host's verdict");
		vehicle_riding_cabin_drop_host(&cabin);
		CHECK(!cabin.host && cabin.unit_index == BIPED, "the drop cleared more or less than the verdict");
		return 0;
	}
	CASE("cabin-take-off-host")
	{
		struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_state state = riding_on(&vehicle, point(0.f, 0.f, 0.f), zero);
		struct vehicle_riding_cabin cabin;

		/* jump 1: the host's verdict for it (its cabin evidence) */
		vehicle_riding_cabin_clear(&cabin);
		state.ticks = 40;
		vehicle_riding_cabin_begin(&cabin, BIPED, &state, TRUE);
		CHECK(vehicle_riding_cabin_take_host(&cabin, BIPED, &state, VEHICLE, _vehicle_riding_enclosure_enclosed,
			_vehicle_riding_cabin_sample_jump, TRUE, TRUE), "the host's jump verdict refused");
		/* landed, then jump 2: its take-off starts the verdict unknown, whatever phase the old one was for */
		state.ticks = RIDING_TICKS;
		vehicle_riding_cabin_standing(&cabin, TRUE, 4);
		vehicle_riding_cabin_take_off(&cabin, &state, BIPED);
		CHECK(!cabin.host, "jump 1's host verdict survived jump 2's take-off");
		state.ticks = 59;
		CHECK(vehicle_riding_cabin_verdict(&cabin, &state, NULL, TRUE) == _vehicle_riding_enclosure_unknown,
			"jump 2's verdict was not unknown until the host's next part");
		return 0;
	}
	CASE("cabin-host-jump-verdict")
	{
		struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_state state = riding_on(&vehicle, point(0.f, 0.f, 0.f), zero);
		struct vehicle_riding_cabin cabin;
		struct vehicle_riding_air air;
		real_vector3d tilted = vector(0.f, .8f, .6f);

		memset(&air, 0, sizeof(air));
		state.ticks = 40;
		/* the client's protected jump sampled enclosed, the host's part says open with the cabin bit */
		vehicle_riding_cabin_clear(&cabin);
		vehicle_riding_cabin_begin(&cabin, BIPED, &state, TRUE);
		vehicle_riding_cabin_jump(&cabin, TRUE);
		CHECK(vehicle_riding_cabin_verdict(&cabin, &state, &air, TRUE) == _vehicle_riding_enclosure_enclosed, "own sample");
		CHECK(vehicle_riding_cabin_take_host(&cabin, BIPED, &state, VEHICLE, _vehicle_riding_enclosure_open,
			_vehicle_riding_cabin_sample_jump, TRUE, TRUE), "the host's jump verdict refused");
		CHECK(vehicle_riding_cabin_verdict(&cabin, &state, &air, TRUE) == _vehicle_riding_enclosure_open,
			"the host's open verdict not the verdict");
		CHECK(!vehicle_riding_cabin_exception(TRUE, TRUE,
			vehicle_riding_cabin_verdict(&cabin, &state, &air, TRUE) == _vehicle_riding_enclosure_enclosed, &tilted, .5f),
			"an open verdict protected");
		/* a later local sample enclosed changes neither */
		vehicle_riding_cabin_jump(&cabin, TRUE);
		CHECK(vehicle_riding_cabin_verdict(&cabin, &state, &air, TRUE) == _vehicle_riding_enclosure_open,
			"a local jump sample over the host's verdict");
		/* back on the deck, the jump's host verdict is no standing one */
		state.ticks = RIDING_TICKS;
		vehicle_riding_cabin_standing(&cabin, TRUE, 4);
		CHECK(vehicle_riding_cabin_verdict(&cabin, &state, &air, TRUE) == _vehicle_riding_enclosure_enclosed,
			"a jump verdict stood in for the standing one");
		/* the host's legacy word carries no cabin protection, whatever the local sample */
		vehicle_riding_cabin_clear(&cabin);
		state.ticks = 40;
		vehicle_riding_cabin_begin(&cabin, BIPED, &state, TRUE);
		vehicle_riding_cabin_jump(&cabin, FALSE);
		air.enclosure = _vehicle_riding_enclosure_enclosed;
		CHECK(!vehicle_riding_cabin_take_host(&cabin, BIPED, &state, VEHICLE, _vehicle_riding_enclosure_enclosed,
			_vehicle_riding_cabin_sample_jump, FALSE, TRUE) && !cabin.host, "a legacy air verdict taken as cabin evidence");
		CHECK(vehicle_riding_cabin_verdict(&cabin, &state, &air, TRUE) == _vehicle_riding_enclosure_unknown,
			"a local sample granted protection over a legacy word");
		CHECK(vehicle_riding_cabin_verdict(&cabin, &state, &air, FALSE) == _vehicle_riding_enclosure_enclosed,
			"serialisation lost the host's legacy word");
		CHECK(air.enclosure == _vehicle_riding_enclosure_enclosed, "the legacy verdict changed");
		/* the host's part enclosed with the bit, the client never sampled the jump: the host's word decides */
		vehicle_riding_cabin_clear(&cabin);
		vehicle_riding_cabin_begin(&cabin, BIPED, &state, TRUE);
		air.enclosure = _vehicle_riding_enclosure_open;
		CHECK(vehicle_riding_cabin_verdict(&cabin, &state, &air, TRUE) == _vehicle_riding_enclosure_unknown, "no cabin evidence");
		CHECK(vehicle_riding_cabin_take_host(&cabin, BIPED, &state, VEHICLE, _vehicle_riding_enclosure_enclosed,
			_vehicle_riding_cabin_sample_jump, TRUE, TRUE), "the host's jump verdict refused");
		CHECK(vehicle_riding_cabin_verdict(&cabin, &state, &air, TRUE) == _vehicle_riding_enclosure_enclosed,
			"the host's word did not decide");
		/* a part for the jump with no cabin evidence takes the verdict back */
		CHECK(!vehicle_riding_cabin_take_host(&cabin, BIPED, &state, VEHICLE, _vehicle_riding_enclosure_enclosed,
			_vehicle_riding_cabin_sample_jump, FALSE, TRUE) && !cabin.host, "a verdict kept without evidence");
		return 0;
	}
	CASE("transition-agreed-table")
	{
		enum { V1 = 0x10080, V2 = 0x20081 };
		/* host before (ticks, vehicle), host now (ticks, vehicle), client (ticks, vehicle), part, word taken */
		static struct { short hb; long hvb; short hn; long hv; short ct; long cv; boolean part, taken, agreed; } const rows[] = {
			{ 0, NONE, 60, V1, 60, V1, TRUE, FALSE, TRUE },		/* rule 1 from the ground */
			{ 59, V1, 60, V1, 60, V1, TRUE, FALSE, TRUE },		/* rule 1 from the vehicle's own air */
			{ 59, V1, 60, V2, 60, V2, TRUE, FALSE, FALSE },		/* its window to another's deck is a vehicle change */
			{ 0, NONE, 60, V1, 60, V2, TRUE, FALSE, FALSE },	/* the client rides another vehicle */
			{ 0, NONE, 60, V1, 0, NONE, FALSE, TRUE, TRUE },	/* rule 2: the world-only word was taken */
			{ 0, NONE, 60, V1, 0, NONE, FALSE, FALSE, FALSE },	/* no word taken */
			{ 0, NONE, 60, V1, 59, V1, TRUE, TRUE, FALSE },		/* a window part is a jump, not a boarding */
			{ 59, V1, 60, V1, 0, NONE, FALSE, TRUE, FALSE },	/* from the air with a world-only word */
			{ 60, V1, 60, V2, 60, V2, TRUE, TRUE, FALSE },		/* one deck to another's */
			{ 60, V1, 59, V1, 59, V1, TRUE, FALSE, TRUE },		/* rule 3: stepping off together */
			{ 60, V1, 59, V1, 59, V2, TRUE, FALSE, FALSE },		/* the client's part is of another vehicle */
			{ 60, V1, 59, V1, 0, NONE, FALSE, TRUE, FALSE },	/* the client did not step off */
			{ 60, V1, 59, V1, 60, V1, TRUE, TRUE, FALSE },		/* the client still rides */
			{ 60, V1, 59, V2, 59, V2, TRUE, FALSE, FALSE },		/* another vehicle than the one it rode */
			{ 60, V1, 0, NONE, 59, V1, TRUE, TRUE, FALSE },		/* a drop, not a window */
			{ 60, V1, 60, V1, 60, V1, TRUE, TRUE, FALSE },		/* nothing changed */
		};
		long row;
		for (row = 0; row < (long)(sizeof(rows)/sizeof(rows[0])); row++)
			CHECK(vehicle_riding_transition_agreed(rows[row].hb, rows[row].hvb, rows[row].hn, rows[row].hv, rows[row].ct, rows[row].cv,
				rows[row].part, rows[row].taken) == rows[row].agreed, "transition row %ld", row);
		return 0;
	}
	CASE("leash-setting")
	{
		double inputs[] = {0.25, NAN, -1.0, 1.5, INFINITY, -INFINITY};
		real expected[] = {0.25f, 0, 0, 1.5f, 0, 0};
		long index;
		for (index = 0; index < 6; index++)
		{
			setting_leash = inputs[index];
			vehicle_riding_latch();
			CHECK(vehicle_riding_transition_leash() == expected[index], "leash %g latched incorrectly", inputs[index]);
		}
		return 0;
	}
	CASE("grace-setting")
	{
		static double const inputs[] = { -1.e100, 1.e100, 29.9999999, 4.9999999, 4., NAN, -3., 4.9, 1.e9, 30.5, 0., INFINITY, 29.99 };
		static short const expected[] = { 0, 30, 29, 4, 4, 4, 0, 4, 30, 30, 0, 4, 29 };
		short index;

		for (index = 0; index < (short)(sizeof(inputs)/sizeof(inputs[0])); index++)
		{
			setting_grace = inputs[index];
			vehicle_riding_latch();
			CHECK(vehicle_riding_cabin_grace() == expected[index], "%g latched as %d, not %d", inputs[index],
				vehicle_riding_cabin_grace(), expected[index]);
		}
		setting_grace = 4.;
		vehicle_riding_latch();
		return 0;
	}
	CASE("steep-riding-sequence")
	{
		struct vehicle_riding_vehicle level = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_state state;
		struct vehicle_riding_cabin cabin;
		real_point3d base = point(1.f, 0.f, .2f);
		real_point3d placed;
		real_vector3d relative = zero;
		real_vector3d world;
		long updates = 10;
		short tick;
		real pitch = 0.f;

		/* boarded upright, the cabin enclosed */
		state = riding_on(&level, base, zero);
		vehicle_riding_cabin_clear(&cabin);
		vehicle_riding_cabin_begin(&cabin, BIPED, &state, TRUE);
		vehicle_riding_cabin_standing(&cabin, TRUE, 4);
		/* pitched a degree a tick to 60: native ground to the limit, then the steep contact carries it, sliding */
		for (tick = 1; tick <= 60; tick++)
		{
			boolean carried;

			pitch = (real)tick*PI/180.f;
			carried = steep_tick(&state, &cabin, pitch, ++updates, TRUE, FALSE, TRUE, &base, &relative, &world);
			CHECK(state.ticks == RIDING_TICKS, "left the deck at %d degrees", tick);
			if (tick >= 46)
				CHECK(carried, "not carried by the steep contact at %d degrees", tick);
			if (tick <= 44)
				CHECK(!carried, "carried by the exception within 45 degrees at %d", tick);
		}
		/* sliding: native gravity in the deck's frame, the frame counted once */
		CHECK(relative.k < -.01f, "no slide: %g", (double)relative.k);
		NEAR(world.k, relative.k + state.velocity.k);
		/* its place on the deck is where it ended */
		vehicle_riding_point_to_world(&state.vehicle_position, &state.vehicle_forward, &state.vehicle_up, &state.local_point,
			&placed);
		NEAR(placed.x, base.x);
		NEAR(placed.z, base.z);
		/* a jump: the next tick's carry kept, then the window, whatever it touches. Its take-off's raw sample enclosed,
		so the take-off marker alone is what lets it go (jump-kept-as-deck's only guard) */
		cabin.jump = _vehicle_riding_enclosure_enclosed;
		vehicle_riding_cabin_take_off(&cabin, &state, BIPED);
		CHECK(!steep_tick(&state, &cabin, pitch, ++updates, TRUE, FALSE, TRUE, &base, &relative, &world) &&
			state.ticks == RIDING_TICKS - 1, "a jump kept as the deck (ticks %d)", state.ticks);
		/* in the window: its jump's raw sample enclosed, another vehicle's contact alone carries nothing, nor does none */
		cabin.jump = _vehicle_riding_enclosure_enclosed;
		CHECK(!steep_tick(&state, &cabin, pitch, ++updates, FALSE, TRUE, TRUE, &base, &relative, &world) &&
			state.ticks == RIDING_TICKS - 1, "another vehicle carried it (ticks %d)", state.ticks);
		CHECK(!steep_tick(&state, &cabin, pitch, ++updates, FALSE, FALSE, TRUE, &base, &relative, &world) &&
			state.ticks == RIDING_TICKS - 1, "kept with no contact (ticks %d)", state.ticks);
		/* a steep contact takes it back */
		CHECK(steep_tick(&state, &cabin, pitch, ++updates, TRUE, FALSE, TRUE, &base, &relative, &world) &&
			state.ticks == RIDING_TICKS, "not reacquired by a steep contact");
		/* back on: held with no touch of its vehicle, and with another vehicle's touch alone */
		CHECK(steep_tick(&state, &cabin, pitch, ++updates, FALSE, FALSE, TRUE, &base, &relative, &world) &&
			state.ticks == RIDING_TICKS, "a held rider left with no touch (ticks %d)", state.ticks);
		CHECK(steep_tick(&state, &cabin, pitch, ++updates, FALSE, TRUE, TRUE, &base, &relative, &world) &&
			state.ticks == RIDING_TICKS, "a held rider left on another vehicle's touch (ticks %d)", state.ticks);
		/* not eligible (a pedestrian, an AI, dead, riding off), and an open cabin: the old rule, off at once */
		cabin.jump = _vehicle_riding_enclosure_enclosed;
		steep_tick(&state, &cabin, pitch, ++updates, TRUE, FALSE, TRUE, &base, &relative, &world);
		CHECK(!steep_tick(&state, &cabin, pitch, ++updates, TRUE, FALSE, FALSE, &base, &relative, &world) &&
			state.ticks == RIDING_TICKS - 1, "an ineligible rider carried");
		cabin.jump = _vehicle_riding_enclosure_enclosed;
		steep_tick(&state, &cabin, pitch, ++updates, TRUE, FALSE, TRUE, &base, &relative, &world);
		cabin.standing = _vehicle_riding_enclosure_open;
		CHECK(!steep_tick(&state, &cabin, pitch, ++updates, TRUE, FALSE, TRUE, &base, &relative, &world) &&
			state.ticks == RIDING_TICKS - 1, "an open cabin carried");
		/* the carrier's own rules */
		{
			long objects[3] = { NONE, OTHER_VEHICLE, VEHICLE };

			CHECK(vehicle_riding_cabin_carrier(VEHICLE, 3, objects) == 2, "the vehicle's contact not found");
			CHECK(vehicle_riding_cabin_carrier(THIRD_VEHICLE, 3, objects) == NONE, "a contact invented");
			CHECK(vehicle_riding_cabin_carrier(NONE, 3, objects) == NONE, "the structure taken for a carrier");
			CHECK(vehicle_riding_cabin_carrier_vehicle(NULL, &state, TRUE) == NONE, "a carrier with no cabin");
		}
		return 0;
	}
	CASE("cabin-hold-no-touch")
	{
		struct steep_rider rider;
		real_point3d placed;
		int degrees;

		steep_pitch_to(&rider, 49);
		/* past the biped's floor limit with gravity alone: the deck keeps the rider, its place on it following it */
		for (degrees = 50; degrees <= 69; degrees++)
		{
			CHECK(steep_step(&rider, degrees, FALSE, FALSE, TRUE), "not held with no touch at %d degrees", degrees);
			CHECK(rider.state.ticks == RIDING_TICKS, "the window opened at %d degrees (ticks %d)", degrees, rider.state.ticks);
			vehicle_riding_point_to_world(&rider.state.vehicle_position, &rider.state.vehicle_forward,
				&rider.state.vehicle_up, &rider.state.local_point, &placed);
			NEAR(placed.x, rider.base.x);
			NEAR(placed.z, rider.base.z);
		}
		CHECK(rider.relative.k < -.07f, "no fall in the deck's frame: %g", (double)rider.relative.k);
		return 0;
	}
	CASE("cabin-hold-ends")
	{
		struct steep_rider rider;
		int tick;

		/* a jump ends the hold at once, whatever the sweep finds or not */
		steep_pitch_to(&rider, 50);
		rider.cabin.jump = _vehicle_riding_enclosure_enclosed;
		vehicle_riding_cabin_take_off(&rider.cabin, &rider.state, BIPED);
		CHECK(!steep_step(&rider, 50, FALSE, FALSE, TRUE) && rider.state.ticks == RIDING_TICKS - 1,
			"a jump held with no touch (ticks %d)", rider.state.ticks);
		/* the window waits for a real touch, tick after tick */
		for (tick = 0; tick < 5; tick++)
		{
			CHECK(!steep_step(&rider, 50, FALSE, FALSE, TRUE) && rider.state.ticks == RIDING_TICKS - 1,
				"a rider in the window held with no touch (ticks %d)", rider.state.ticks);
		}
		/* an open cabin, a level vehicle and an ineligible rider are held by nothing */
		steep_pitch_to(&rider, 50);
		rider.cabin.standing = _vehicle_riding_enclosure_open;
		CHECK(!steep_step(&rider, 50, FALSE, FALSE, TRUE) && rider.state.ticks == RIDING_TICKS - 1,
			"an open cabin held (ticks %d)", rider.state.ticks);
		steep_pitch_to(&rider, 10);
		CHECK(!steep_step(&rider, 10, FALSE, FALSE, TRUE) && rider.state.ticks == RIDING_TICKS - 1,
			"a level vehicle held (ticks %d)", rider.state.ticks);
		steep_pitch_to(&rider, 50);
		CHECK(!steep_step(&rider, 50, FALSE, FALSE, FALSE) && rider.state.ticks == RIDING_TICKS - 1,
			"an ineligible rider held (ticks %d)", rider.state.ticks);
		CHECK(!vehicle_riding_cabin_holds(NONE, TRUE) && !vehicle_riding_cabin_holds(VEHICLE, FALSE) &&
			vehicle_riding_cabin_holds(VEHICLE, TRUE), "the hold's rule");
		return 0;
	}
	CASE("cabin-client-crossing")
	{
		struct vehicle_riding_vehicle level = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_state riding = riding_on(&level, point(0.f, 0.f, 0.f), zero);
		struct vehicle_riding_cabin cabin;

		vehicle_riding_cabin_clear(&cabin);
		vehicle_riding_cabin_begin(&cabin, BIPED, &riding, TRUE);
		/* the host's standing word says open, this machine's sample hit: protected on the tilt reached now */
		CHECK(vehicle_riding_cabin_take_host(&cabin, BIPED, &riding, VEHICLE, _vehicle_riding_enclosure_open,
			_vehicle_riding_cabin_sample_standing, TRUE, TRUE), "the host's word not taken");
		cabin.standing = _vehicle_riding_enclosure_enclosed;
		CHECK(vehicle_riding_cabin_encloses(&cabin, &riding, NULL), "an own hit not protecting against a stale open");
		/* the host's enclosed is never taken back by a miss here */
		vehicle_riding_cabin_take_host(&cabin, BIPED, &riding, VEHICLE, _vehicle_riding_enclosure_enclosed,
			_vehicle_riding_cabin_sample_standing, TRUE, TRUE);
		cabin.standing = _vehicle_riding_enclosure_open;
		CHECK(vehicle_riding_cabin_encloses(&cabin, &riding, NULL), "an own miss took back the host's enclosed");
		/* both open */
		vehicle_riding_cabin_take_host(&cabin, BIPED, &riding, VEHICLE, _vehicle_riding_enclosure_open,
			_vehicle_riding_cabin_sample_standing, TRUE, TRUE);
		CHECK(!vehicle_riding_cabin_encloses(&cabin, &riding, NULL), "two opens protected");
		/* a jump goes by the host's word and its own raw sample, never the standing hit */
		cabin.standing = _vehicle_riding_enclosure_enclosed;
		vehicle_riding_cabin_take_off(&cabin, &riding, BIPED);
		riding.ticks = RIDING_TICKS - 1;
		vehicle_riding_cabin_take_host(&cabin, BIPED, &riding, VEHICLE, _vehicle_riding_enclosure_open,
			_vehicle_riding_cabin_sample_jump, TRUE, TRUE);
		vehicle_riding_cabin_jump(&cabin, FALSE);
		CHECK(!vehicle_riding_cabin_encloses(&cabin, &riding, NULL), "a window rider went by the standing hit");
		/* a cabin keyed to another vehicle protects nothing */
		riding.ticks = RIDING_TICKS;
		riding.object_index = OTHER_VEHICLE;
		CHECK(!vehicle_riding_cabin_encloses(&cabin, &riding, NULL), "another vehicle's cabin protected");
		return 0;
	}
	CASE("crossing-rest-place-free")
	{
		/* a host copy and a client copy of one rider through the crossing of the floor's limit, the client with the
		host's word of the tick before; whether the standing sample was enclosed before the crossing must not decide it */
		real_point3d ends[2][2];
		int variant;

		for (variant = 0; variant < 2; variant++)
		{
			struct steep_rider host;
			struct steep_rider client;
			short word = _vehicle_riding_enclosure_unknown;
			int degrees;

			steep_pitch_to(&host, 40);
			steep_pitch_to(&client, 40);
			if (variant)
			{
				host.cabin.standing = _vehicle_riding_enclosure_open;
				client.cabin.standing = _vehicle_riding_enclosure_open;
			}
			for (degrees = 41; degrees <= 47; degrees++)
			{
				/* the crossing tick: the sample turns to the body's axis and hits on both copies */
				if (degrees == 46)
				{
					host.cabin.standing = _vehicle_riding_enclosure_enclosed;
					client.cabin.standing = _vehicle_riding_enclosure_enclosed;
				}
				if (word != _vehicle_riding_enclosure_unknown)
				{
					vehicle_riding_cabin_take_host(&client.cabin, BIPED, &client.state, VEHICLE, word,
						_vehicle_riding_cabin_sample_standing, TRUE, TRUE);
				}
				if (degrees >= 46)
				{
					CHECK(steep_step(&host, degrees, FALSE, FALSE, TRUE) && host.state.ticks == RIDING_TICKS,
						"host copy lost at %d degrees, variant %d (ticks %d)", degrees, variant, host.state.ticks);
					CHECK(steep_step(&client, degrees, FALSE, FALSE, TRUE) && client.state.ticks == RIDING_TICKS,
						"client copy lost at %d degrees, variant %d (ticks %d)", degrees, variant, client.state.ticks);
				}
				else
				{
					steep_step(&host, degrees, TRUE, FALSE, TRUE);
					steep_step(&client, degrees, TRUE, FALSE, TRUE);
				}
				/* the host's verdict at the end of its tick reaches the client one tick late */
				word = vehicle_riding_cabin_verdict(&host.cabin, &host.state, NULL, TRUE);
			}
			ends[variant][0] = host.base;
			ends[variant][1] = client.base;
		}
		NEAR(ends[0][0].x, ends[1][0].x);
		NEAR(ends[0][0].z, ends[1][0].z);
		NEAR(ends[0][1].x, ends[1][1].x);
		NEAR(ends[0][1].z, ends[1][1].z);
		NEAR(ends[0][0].z, ends[0][1].z);
		return 0;
	}
	CASE("cabin-overlap-retry")
	{
		struct vehicle_riding_cabin_check check = { 0 };
		struct vehicle_riding_cabin record;
		struct vehicle_riding_state state;
		real_point3d anchor = point(.6f, .8f, .2f);
		real_point3d entry = point(.7f, 1.12f, .2f);
		short settle;

		vehicle_riding_clear(&state);
		state.object_index = VEHICLE;
		state.ticks = RIDING_TICKS;
		vehicle_riding_cabin_clear(&record);
		vehicle_riding_cabin_begin(&record, 123, &state, TRUE);
		vehicle_riding_cabin_settled(&record, _vehicle_riding_cabin_settle_none, TRUE, &anchor);
		check.have_anchor = record.have_anchor;
		check.touching = TRUE;
		check.width = .2f;
		check.anchor_distance = .34f;
		settle = vehicle_riding_cabin_recover(&check);
		CHECK(settle == _vehicle_riding_cabin_settle_failed, "raw miss touching the hatch became a door (%d)", settle);
		vehicle_riding_cabin_settled(&record, settle, TRUE, &entry);
		CHECK(record.have_anchor && record.anchor.x == anchor.x, "failure dropped or changed the anchor");
		vehicle_riding_cabin_begin(&record, 123, &state, TRUE);
		check.have_anchor = record.have_anchor;
		check.anchor_valid = TRUE;
		CHECK(vehicle_riding_cabin_recover(&check) == _vehicle_riding_cabin_settle_anchor, "next tick did not retry the anchor");
		check.anchor_valid = FALSE;
		check.around_valid = TRUE;
		CHECK(vehicle_riding_cabin_recover(&check) == _vehicle_riding_cabin_settle_around, "a free place about the anchor ignored");
		check.touching = FALSE;
		settle = vehicle_riding_cabin_recover(&check);
		CHECK(settle == _vehicle_riding_cabin_settle_door, "a clear hatch exit held back");
		vehicle_riding_cabin_settled(&record, settle, FALSE, &entry);
		CHECK(!record.have_anchor, "a real door exit kept its anchor");
		return 0;
	}
	CASE("cabin-recovery-rules")
	{
		struct vehicle_riding_cabin_check check;
		struct vehicle_riding_vehicle vehicle = vehicle_at(VEHICLE, 0.f, 0.f, 0.f, 0.f, 10);
		struct vehicle_riding_state state = riding_on(&vehicle, point(0.f, 0.f, 0.f), zero);
		struct vehicle_riding_cabin cabin;
		real_point3d local = point(.5f, 0.f, .2f);

		memset(&check, 0, sizeof(check));
		check.width = .2f;
		/* no interior history: nothing to do, or an overlap left as it is (and logged) */
		CHECK(vehicle_riding_cabin_recover(&check) == _vehicle_riding_cabin_settle_none, "no history moved");
		check.touching = TRUE;
		CHECK(vehicle_riding_cabin_recover(&check) == _vehicle_riding_cabin_settle_failed, "an overlap without history moved");
		check.touching = FALSE;
		check.have_anchor = TRUE;
		check.interior = TRUE;
		CHECK(vehicle_riding_cabin_recover(&check) == _vehicle_riding_cabin_settle_none, "inside and free moved");
		/* out through the hull, free of it: back to the anchor, however far */
		check.interior = FALSE;
		check.crosses = TRUE;
		check.anchor_valid = TRUE;
		check.anchor_distance = 3.f;
		CHECK(vehicle_riding_cabin_recover(&check) == _vehicle_riding_cabin_settle_anchor, "a free hull escape not returned");
		/* its anchor blocked: a place about it, else nothing */
		check.anchor_valid = FALSE;
		check.around_valid = TRUE;
		CHECK(vehicle_riding_cabin_recover(&check) == _vehicle_riding_cabin_settle_around, "no place about the anchor");
		check.around_valid = FALSE;
		CHECK(vehicle_riding_cabin_recover(&check) == _vehicle_riding_cabin_settle_failed, "a blocked return moved");
		/* out through an opening: let go only once clear of its frame */
		check.crosses = FALSE;
		check.anchor_valid = TRUE;
		CHECK(vehicle_riding_cabin_recover(&check) == _vehicle_riding_cabin_settle_door, "a door exit returned");
		check.touching = TRUE;
		CHECK(vehicle_riding_cabin_recover(&check) == _vehicle_riding_cabin_settle_failed, "a door frame's overlap dropped its history");
		/* an overlap inside: the anchor within two widths, the touch's way out, the fix, else nothing */
		check.interior = TRUE;
		check.anchor_distance = .4f;
		CHECK(vehicle_riding_cabin_recover(&check) == _vehicle_riding_cabin_settle_anchor, "the near anchor");
		check.anchor_distance = .41f;
		check.normal_valid = TRUE;
		CHECK(vehicle_riding_cabin_recover(&check) == _vehicle_riding_cabin_settle_candidate, "the touch's way out");
		check.normal_valid = FALSE;
		check.fixed_valid = TRUE;
		CHECK(vehicle_riding_cabin_recover(&check) == _vehicle_riding_cabin_settle_fixed, "the fix");
		check.fixed_valid = FALSE;
		CHECK(vehicle_riding_cabin_recover(&check) == _vehicle_riding_cabin_settle_failed, "a blocked interior moved");
		/* the anchor: kept at a settled interior place, untouched by a failure or a place not interior, forgotten at a door */
		vehicle_riding_cabin_clear(&cabin);
		vehicle_riding_cabin_settled(&cabin, _vehicle_riding_cabin_settle_none, TRUE, &local);
		CHECK(cabin.have_anchor && cabin.anchor.x == .5f, "no anchor at an interior place");
		local.x = 9.f;
		vehicle_riding_cabin_settled(&cabin, _vehicle_riding_cabin_settle_failed, TRUE, &local);
		CHECK(cabin.anchor.x == .5f, "a failure moved the anchor");
		vehicle_riding_cabin_settled(&cabin, _vehicle_riding_cabin_settle_none, FALSE, &local);
		CHECK(cabin.anchor.x == .5f, "a place not interior anchored");
		vehicle_riding_cabin_settled(&cabin, _vehicle_riding_cabin_settle_door, FALSE, &local);
		CHECK(!cabin.have_anchor, "the anchor outlived a door exit");
		/* deferral: the vehicle's own touches wait while protected or with interior history (a level vehicle too); never
		another vehicle's, another unit's, or past the window */
		vehicle_riding_cabin_begin(&cabin, BIPED, &state, TRUE);
		CHECK(!vehicle_riding_cabin_defers(&cabin, &state, BIPED, VEHICLE, FALSE), "deferred with no protection or history");
		CHECK(vehicle_riding_cabin_defers(&cabin, &state, BIPED, VEHICLE, TRUE), "a protected rider's vehicle not deferred");
		cabin.have_anchor = TRUE;
		CHECK(vehicle_riding_cabin_defers(&cabin, &state, BIPED, VEHICLE, FALSE), "a level vehicle with history not deferred");
		CHECK(!vehicle_riding_cabin_defers(&cabin, &state, BIPED, OTHER_VEHICLE, TRUE), "another vehicle deferred");
		CHECK(!vehicle_riding_cabin_defers(&cabin, &state, OTHER_BIPED, VEHICLE, TRUE), "another unit deferred");
		state.ticks = 0;
		CHECK(!vehicle_riding_cabin_defers(&cabin, &state, BIPED, VEHICLE, TRUE), "deferred with no window");
		state.ticks = RIDING_TICKS;
		CHECK(!vehicle_riding_cabin_defers(NULL, &state, BIPED, VEHICLE, TRUE), "deferred with no cabin");
		/* the decision: a deferred vehicle's ignore and judge resolved in the rider's tick; the original kept */
		CHECK(vehicle_riding_cabin_collision(_vehicle_riding_collision_ignore, TRUE) == _vehicle_riding_collision_resolved &&
			vehicle_riding_cabin_collision(_vehicle_riding_collision_judge, TRUE) == _vehicle_riding_collision_resolved &&
			vehicle_riding_cabin_collision(_vehicle_riding_collision_original, TRUE) == _vehicle_riding_collision_original &&
			vehicle_riding_cabin_collision(_vehicle_riding_collision_ignore, FALSE) == _vehicle_riding_collision_ignore,
			"the collision decision");
		/* watched (worth the recovery's casts): cabin evidence only, so no stock vehicle costs a ray */
		vehicle_riding_cabin_clear(&cabin);
		vehicle_riding_cabin_begin(&cabin, BIPED, &state, TRUE);
		CHECK(!vehicle_riding_cabin_watched(&cabin, &state, BIPED), "watched with no evidence");
		cabin.standing = _vehicle_riding_enclosure_open;
		CHECK(!vehicle_riding_cabin_watched(&cabin, &state, BIPED), "an open cabin watched");
		cabin.standing = _vehicle_riding_enclosure_enclosed;
		CHECK(vehicle_riding_cabin_watched(&cabin, &state, BIPED), "an enclosed one not watched");
		cabin.standing = _vehicle_riding_enclosure_open;
		cabin.jump = _vehicle_riding_enclosure_enclosed;
		CHECK(vehicle_riding_cabin_watched(&cabin, &state, BIPED), "an enclosed jump not watched");
		cabin.jump = _vehicle_riding_enclosure_unknown;
		cabin.have_anchor = TRUE;
		CHECK(vehicle_riding_cabin_watched(&cabin, &state, BIPED) && !vehicle_riding_cabin_watched(&cabin, &state, OTHER_BIPED),
			"history not watched, or another unit watched");
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
