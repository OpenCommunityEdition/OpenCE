/*
WRITE_CARRY.C (test)

bipeds.c's biped_riding_vehicle_writing and biped_riding_vehicle_written,
taken from the sources by test_write_carry.py with vehicle_riding.c whole,
against fakes of the object iterator, the pill, the carry's sweep (which
can stop short) and the touch test. The touch test models the engine's
pill: upright in the world (the base and the points half a height and a
height above it, each a width wide), never turned with the vehicle,
against a hull box posed at the vehicle's current pose. These are glue
tests: the physical proof is the live run.
*/

#include "harness.h"
typedef unsigned short word;
#include <math.h>
typedef struct { real i, j; } real_vector2d;
#include "enums.inc"
#include "engine.inc"

typedef struct { real yaw, pitch; } real_euler_angles2d;
#define TICKS_PER_SECOND 30
#define PI 3.14159265f
#define csmemset memset

/* vehicle_riding.c's world: settings, connection, look, the host's switches */
static int config_riding = 1;
int config_boolean(const char *name) { return !strcmp(name, "network.vehicle_riding") ? config_riding : 0; }
double config_real(const char *name) { return !strcmp(name, "debug.riding_enclosure_grace_ticks") ? 4.0 :
	!strcmp(name, "debug.riding_teleport_step") ? 4.0 : !strcmp(name, "debug.riding_moved_alone") ? .25 : 0.0; }
short game_connection(void) { return _game_connection_local; }
boolean game_in_editor(void) { return FALSE; }
boolean game_players_are_double_speed(void) { return FALSE; }
void player_control_permanent_impulse(short local_player_index, real_euler_angles2d const *delta)
{
	(void)local_player_index;
	(void)delta;
}
struct game_variant { short game_engine_index; struct { unsigned long flags; } universal_variant; };
struct network_game { struct game_variant variant; byte cooperative_flags; };
static struct network_game test_game;
struct network_game *network_game_get_game(void) { return &test_game; }
static struct game_variant engine_variant;
boolean game_engine_running(void) { return FALSE; }
struct game_variant *game_engine_get_variant(void) { return &engine_variant; }
int halo_server_mode(void) { return 0; }

/* the map's riding slot and its script and latch reach (vehicle_riding.c's map functions); no case here sets a map */
struct object_datum;
struct biped_datum;
struct object_iterator { long index; long next; };
static struct object_datum *object_iterator_next(struct object_iterator *iterator);
static void object_iterator_new(struct object_iterator *iterator, long mask, long flags);
#define _object_mask_biped 1
static long map_slot_storage;
static void *game_state_malloc(char const *name, void *type, long size) { (void)name; (void)type; (void)size; return &map_slot_storage; }
static short hs_runtime_port_executing_script_type(void) { return NONE; }
static short hs_runtime_port_scenario_boolean(char const *name) { (void)name; return NONE; }
static long game_time_get(void) { return 0; }
static void error(long priority, char const *format, ...) { (void)priority; (void)format; }
static void biped_riding_clear(struct biped_datum *biped) { (void)biped; }
#include "under_test.inc"

/* the map's decision as vehicle_riding_objects_reset makes it: a setting changed counts from the next map */
static void decide_now(void)
{
#ifndef FIXTURE_NO_DECIDE
	vehicle_riding_map_decide();
#endif
}

#define VEHICLE 0x10000001
#define OTHER_VEHICLE 0x10010002
#define PLAYER_BIPED(n) (0x20000000 + (n))
#define HEIGHT 1.4f
#define WIDTH .2f
/* the pill's base above the object's position */
#define BASE_RAISE .07f
#define _object_mask_biped 1
#define PRECISE(a, b) CHECK(fabs((double)(a) - (double)(b)) < .00001, "%s: %.9g, not %.9g", #a, (double)(a), (double)(b))

/* ---------- the fake world */

struct object_fields
{
	long parent_object_index;
	real_point3d position;
	real_vector3d forward;
	real_vector3d up;
	real_vector3d translational_velocity;
	real_point3d bounding_sphere_center;
	real bounding_sphere_radius;
};

struct object_datum
{
	struct object_fields object;
};

/* a fake biped starts with its object, as the iterator hands it back */
struct biped_datum
{
	struct object_fields object;
	struct { long player_index; } unit;
	struct { unsigned long flags; } biped;
	long definition_index;
	boolean applies;
	struct vehicle_riding_state riding;
};

struct biped_definition { long unused; };

/* a vehicle: its object, and the hull as one box in its body frame (low above high: none) */
struct fake_vehicle
{
	long index;
	struct object_datum datum;
	real low[3];
	real high[3];
};

#define BIPEDS 256
static struct biped_datum bipeds[BIPEDS];
static short biped_count;
static struct fake_vehicle vehicles[2];
static struct biped_definition fake_definition;
static real_vector3d const zero_vector = { 0.f, 0.f, 0.f };
static real_vector3d const *const global_zero_vector3d = &zero_vector;

/* the player table the write hooks walk (the slot number is the player's absolute index, its datum index 0x3000 salted) */
struct data_array;
static struct data_array *player_data;
struct player_datum { long unit_index; };
struct data_iterator { long datum_index; short slot; };
#define PLAYERS 16
static struct player_datum players[PLAYERS];
static boolean player_used[PLAYERS];

static void data_iterator_new(struct data_iterator *iterator, struct data_array *data)
{
	(void)data;
	iterator->slot = NONE;
	iterator->datum_index = NONE;
}

static struct player_datum *data_iterator_next(struct data_iterator *iterator)
{
	while (++iterator->slot < PLAYERS)
	{
		if (player_used[iterator->slot])
		{
			iterator->datum_index = 0x30000000 + iterator->slot;
			return &players[iterator->slot];
		}
	}
	return NULL;
}

/* the spies */
static short object_walks;
static short carried_count;
static long carried_order[BIPEDS];
static short object_sets;
static short rebases;
static long rebased_index;
static short carries;
/* how much of the carry's displacement its sweep gets through (1: all of it) */
static real carry_fraction = 1.f;

static struct fake_vehicle *fake_vehicle_find(long index)
{
	short at;

	for (at = 0; at < 2; at++)
	{
		if (vehicles[at].index == index)
			return &vehicles[at];
	}
	CHECK(FALSE, "no fake vehicle %lx", (unsigned long)index);
	return NULL;
}

static struct object_datum *fake_object_get(long index)
{
	if ((index & 0xFFFF0000) == 0x20000000)
		return (struct object_datum *)&bipeds[index & 0xFFFF];
	return &fake_vehicle_find(index)->datum;
}

#define object_get(index) fake_object_get(index)
#define biped_definition_get(index) (&fake_definition)
#define biped_get(index) ((struct biped_datum *)&bipeds[(index) & 0xFFFF])
#define biped_try_and_get(index) fake_biped_try_and_get(index)

static struct biped_datum *fake_biped_try_and_get(long index)
{
	return (index & 0xFFFF0000) == 0x20000000 && (index & 0xFFFF) < biped_count ? &bipeds[index & 0xFFFF] : NULL;
}

static void object_iterator_new(struct object_iterator *iterator, long mask, long flags)
{
	(void)mask;
	(void)flags;
	iterator->index = NONE;
	iterator->next = 0;
}

static struct object_datum *object_iterator_next(struct object_iterator *iterator)
{
	object_walks++;
	if (iterator->next >= biped_count)
		return NULL;
	iterator->index = PLAYER_BIPED(iterator->next);
	return (struct object_datum *)&bipeds[iterator->next++];
}

static boolean biped_riding_applies(struct biped_datum const *biped)
{
	return biped->applies;
}

static short biped_riding_player(struct biped_datum const *biped)
{
	return biped->unit.player_index != NONE ? (short)DATUM_INDEX_TO_ABSOLUTE_INDEX(biped->unit.player_index) : NONE;
}

static void biped_riding_get(struct biped_datum const *biped, struct vehicle_riding_state *state)
{
	*state = biped->riding;
}

static void biped_get_physics_pill(long biped_index, real_point3d *base, real *height, real *width)
{
	struct object_datum const *object = fake_object_get(biped_index);

	set_real_point3d(base, object->object.position.x, object->object.position.y, object->object.position.z + BASE_RAISE);
	*height = HEIGHT;
	*width = WIDTH;
}

static void object_set_position(long index, real_point3d const *position, real_vector3d const *forward,
	real_vector3d const *up)
{
	CHECK(!forward && !up, "a unit placed with a pose");
	fake_object_get(index)->object.position = *position;
	object_sets++;
}

static boolean biped_riding_rebase(long biped_index)
{
	rebases++;
	rebased_index = biped_index;
	return TRUE;
}

/* the sweep: through everything but the vehicle, stopping short by carry_fraction */
static void biped_riding_carry(long biped_index, struct biped_datum const *biped, struct biped_definition const *definition,
	long vehicle_index, real_vector3d const *displacement, real height, real width, real_point3d *position)
{
	(void)biped;
	(void)definition;
	(void)height;
	(void)width;
	CHECK(vehicle_index == VEHICLE, "a carry past another vehicle: %lx", (unsigned long)vehicle_index);
	carries++;
	carried_order[carried_count++] = biped_index;
	position->x += displacement->i*carry_fraction;
	position->y += displacement->j*carry_fraction;
	position->z += displacement->k*carry_fraction;
}

/* the upright pill against the hull at the vehicle's current pose: the base, half a height and a height above it
(world z), each in the box with a width of room about it sideways (none along the hull's own up) */
static boolean biped_riding_touches(long vehicle_index, real_point3d const *base, real_vector3d const *ahead, real height,
	real width)
{
	struct fake_vehicle const *vehicle = fake_vehicle_find(vehicle_index);
	short at;

	for (at = 0; at < 3; at++)
	{
		real_point3d sample;
		real_point3d local;

		set_real_point3d(&sample, base->x + ahead->i, base->y + ahead->j, base->z + ahead->k + height*.5f*(real)at);
		vehicle_riding_point_to_local(&vehicle->datum.object.position, &vehicle->datum.object.forward, &vehicle->datum.object.up, &sample,
			&local);
		if (local.x >= vehicle->low[0] - width && local.x <= vehicle->high[0] + width &&
			local.y >= vehicle->low[1] - width && local.y <= vehicle->high[1] + width &&
			local.z >= vehicle->low[2] && local.z <= vehicle->high[2])
		{
			return TRUE;
		}
	}
	return FALSE;
}

#include "bipeds_write_carry.inc"

/* ---------- the cases' helpers */

struct pose
{
	real_point3d position;
	real_vector3d forward;
	real_vector3d up;
};

static struct pose pose_of(real x, real y, real z, real fi, real fj, real fk, real ui, real uj, real uk)
{
	struct pose pose;

	set_real_point3d(&pose.position, x, y, z);
	pose.forward.i = fi;
	pose.forward.j = fj;
	pose.forward.k = fk;
	pose.up.i = ui;
	pose.up.j = uj;
	pose.up.k = uk;
	return pose;
}

/* a pose at (x, y, z) rolled about its forward axis (the world's x) by angle */
static struct pose rolled(real x, real y, real z, real angle)
{
	return pose_of(x, y, z, 1.f, 0.f, 0.f, 0.f, -(real)sin(angle), (real)cos(angle));
}

static void vehicle_pose(long index, struct pose const *pose)
{
	struct fake_vehicle *vehicle = fake_vehicle_find(index);

	vehicle->datum.object.position = pose->position;
	vehicle->datum.object.forward = pose->forward;
	vehicle->datum.object.up = pose->up;
	vehicle->datum.object.bounding_sphere_center = pose->position;
	vehicle->datum.object.bounding_sphere_radius = 3.f;
}

/* the fake world with VEHICLE's hull, one box (z: its deck top above the hull's origin), and OTHER_VEHICLE with no hull */
static void world_new(real half_width, real bottom, real top)
{
	memset(bipeds, 0, sizeof(bipeds));
	memset(vehicles, 0, sizeof(vehicles));
	memset(biped_riding_writes, 0, sizeof(biped_riding_writes));
	memset(players, 0, sizeof(players));
	memset(player_used, 0, sizeof(player_used));
	biped_count = 0;
	object_walks = carried_count = 0;
	object_sets = rebases = carries = 0;
	carry_fraction = 1.f;
	vehicles[0].index = VEHICLE;
	vehicles[0].low[0] = vehicles[0].low[1] = -half_width;
	vehicles[0].high[0] = vehicles[0].high[1] = half_width;
	vehicles[0].low[2] = bottom;
	vehicles[0].high[2] = top;
	vehicles[1].index = OTHER_VEHICLE;
	vehicles[1].low[0] = vehicles[1].low[1] = vehicles[1].low[2] = 1.f;
	vehicles[1].high[0] = vehicles[1].high[1] = vehicles[1].high[2] = -1.f;
}

/* a biped with its pill base at base, in a window (or none) of a vehicle, for the player (a datum index, or NONE) */
static short biped_make(real_point3d base, short ticks, long vehicle_index, real_vector3d velocity, long player_index)
{
	struct biped_datum *biped = &bipeds[biped_count];

	CHECK(biped_count < BIPEDS, "more bipeds than the fake world holds");
	set_real_point3d(&biped->object.position, base.x, base.y, base.z - BASE_RAISE);
	biped->object.parent_object_index = NONE;
	biped->object.translational_velocity = velocity;
	biped->unit.player_index = player_index;
	biped->applies = TRUE;
	vehicle_riding_clear(&biped->riding);
	biped->riding.ticks = ticks;
	biped->riding.object_index = vehicle_index;
	return biped_count++;
}

/* a player's biped (the player in slot 3 + its biped's number, entered in the player table) */
static short biped_new(real_point3d base, short ticks, long vehicle_index, real_vector3d velocity)
{
	short slot = 3 + biped_count;

	CHECK(slot < PLAYERS, "no room in the player table for slot %d", (int)slot);
	player_used[slot] = TRUE;
	players[slot].unit_index = PLAYER_BIPED(biped_count);
	return biped_make(base, ticks, vehicle_index, velocity, 0x30000000 + slot);
}

/* a biped with no player, so none in the player table */
static short bystander_new(real_point3d base)
{
	return biped_make(base, 0, NONE, zero_vector, NONE);
}

/* the biped is the unit of the player in a slot (the player it had gives its slot up) */
static void biped_player(short biped, short slot)
{
	short old = (short)DATUM_INDEX_TO_ABSOLUTE_INDEX(bipeds[biped].unit.player_index);

	player_used[old] = FALSE;
	bipeds[biped].unit.player_index = 0x30000000 + slot;
	player_used[slot] = TRUE;
	players[slot].unit_index = PLAYER_BIPED(biped);
}

static real_point3d base_of(short biped)
{
	real_point3d base;
	real height, width;

	biped_get_physics_pill(PLAYER_BIPED(biped), &base, &height, &width);
	return base;
}

/* what W does to a point: the before pose's local place, in the after pose */
static real_point3d written(struct pose const *before, struct pose const *after, real_point3d point)
{
	real_point3d local, carried;

	vehicle_riding_point_to_local(&before->position, &before->forward, &before->up, &point, &local);
	vehicle_riding_point_to_world(&after->position, &after->forward, &after->up, &local, &carried);
	return carried;
}

/* a state correction of VEHICLE from before to after, as the hooks are called around the write */
static void write_vehicle(long index, struct pose const *before, struct pose const *after)
{
	vehicle_pose(index, before);
	biped_riding_vehicle_writing(index);
	vehicle_pose(index, after);
	biped_riding_vehicle_written(index, &before->position, &before->forward, &before->up);
}

/* the touch of a biped (its pill as it is) with the hull at a pose */
static boolean touches_at(struct pose const *pose, short biped, real_point3d base)
{
	boolean touching;

	(void)biped;
	vehicle_pose(VEHICLE, pose);
	touching = biped_riding_touches(VEHICLE, &base, global_zero_vector3d, HEIGHT, WIDTH);
	return touching;
}

struct snapshot
{
	struct object_fields object;
};

static void nothing_moved(short biped, struct snapshot const *saved, char const *what)
{
	CHECK(!memcmp(&bipeds[biped].object, &saved->object, sizeof(saved->object)) && !object_sets, "%s: moved (sets %d)", what,
		(int)object_sets);
	CHECK(!rebases, "%s: rebased", what);
}

#define NEAR_POINT(a, b, tolerance) CHECK(fabs((double)(a).x - (double)(b).x) < tolerance && \
	fabs((double)(a).y - (double)(b).y) < tolerance && fabs((double)(a).z - (double)(b).z) < tolerance, \
	"%s: (%.9g %.9g %.9g), not (%.9g %.9g %.9g)", #a, (double)(a).x, (double)(a).y, (double)(a).z, \
	(double)(b).x, (double)(b).y, (double)(b).z)

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	decide_now();
	vehicle_riding_latch();

	CASE("riding-on-reached")
	{
		CHECK(vehicle_riding_enabled(), "riding is not on under the fixture's own on setup");
		return 0;
	}
	CASE("riding-off-reached")
	{
		CHECK(vehicle_riding_enabled(), "riding is not on before the setting goes off");
		config_riding = 0;
#ifndef FIXTURE_NO_DECIDE_OFF
		vehicle_riding_map_decide();
#endif
		CHECK(!vehicle_riding_enabled(), "riding is still on after the setting went off and the map decided again");
		return 0;
	}

	CASE("write-carry-reboarded")
	{
		/* the owner rider of r3 1466 after a re-board: in no window, a deck 0.42 above the hull's origin */
		struct pose before = pose_of(49.36445999f, -119.09696198f, 0.75978494f, 0.538125f, -0.842182f, -0.033928f,
			0.177504f, 0.073885f, 0.981343f);
		struct pose after = pose_of(49.38689423f, -119.10076141f, 0.77995276f, 0.536732f, -0.840485f, -0.074191f,
			0.241488f, 0.068770f, 0.967964f);
		real_point3d base = { 49.09414673f, -118.29788971f, 1.21372807f };
		real_vector3d velocity = { 0.00896138f, -0.02138549f, -0.01492126f };
		real_vector3d saved_velocity = velocity;
		real_point3d expected, now;
		short biped;

		world_new(3.f, -1.f, .42f);
		biped = biped_new(base, 0, NONE, velocity);
		biped_count = 1;
		CHECK(!touches_at(&before, biped, base) && touches_at(&after, biped, base), "the fixture is no write-made touch");
		write_vehicle(VEHICLE, &before, &after);
		expected = written(&before, &after, base);
		now = base_of(biped);
		NEAR_POINT(now, expected, .00005);
		CHECK(!memcmp(&bipeds[biped].object.translational_velocity, &saved_velocity, sizeof(saved_velocity)),
			"the velocity changed");
		CHECK(object_sets == 1 && carries == 1, "sets %d carries %d", (int)object_sets, (int)carries);
		CHECK(!rebases, "a rider in no window rebased");
		return 0;
	}
	CASE("write-carry-window")
	{
		/* the owner rider of r2 1447 in V's window (59) */
		struct pose before = pose_of(48.99548340f, -118.46168518f, 0.78491533f, 0.543013f, -0.835887f, -0.080190f,
			0.227777f, 0.054704f, 0.972176f);
		struct pose after = pose_of(49.00709534f, -118.46977997f, 0.81000108f, 0.535200f, -0.836966f, -0.114230f,
			0.296970f, 0.059830f, 0.953011f);
		real_point3d base = { 48.35581970f, -117.69270325f, 1.34337246f };
		real_vector3d velocity = { 0.00610754f, 0.00729753f, 0.00972326f };
		real_vector3d saved_velocity = velocity;
		real_point3d expected, now;
		short biped;

		world_new(3.f, -1.f, .42f);
		biped = biped_new(base, RIDING_TICKS - 1, VEHICLE, velocity);
		biped_count = 1;
		CHECK(!touches_at(&before, biped, base) && touches_at(&after, biped, base), "the fixture is no write-made touch");
		write_vehicle(VEHICLE, &before, &after);
		expected = written(&before, &after, base);
		now = base_of(biped);
		NEAR_POINT(now, expected, .00005);
		CHECK(!memcmp(&bipeds[biped].object.translational_velocity, &saved_velocity, sizeof(saved_velocity)),
			"the velocity changed");
		CHECK(rebases == 1 && rebased_index == PLAYER_BIPED(biped), "the window's snapshot retaken %d times", (int)rebases);
		return 0;
	}
	CASE("write-carry-riding-skipped")
	{
		struct pose before = pose_of(48.99548340f, -118.46168518f, 0.78491533f, 0.543013f, -0.835887f, -0.080190f,
			0.227777f, 0.054704f, 0.972176f);
		struct pose after = pose_of(49.00709534f, -118.46977997f, 0.81000108f, 0.535200f, -0.836966f, -0.114230f,
			0.296970f, 0.059830f, 0.953011f);
		real_point3d base = { 48.35581970f, -117.69270325f, 1.34337246f };
		struct snapshot saved;
		short biped;

		world_new(3.f, -1.f, .42f);
		biped = biped_new(base, RIDING_TICKS, VEHICLE, zero_vector);
		biped_count = 1;
		saved.object = bipeds[biped].object;
		write_vehicle(VEHICLE, &before, &after);
		nothing_moved(biped, &saved, "a riding rider");
		return 0;
	}
	CASE("write-carry-preexisting-touch")
	{
		/* the pill rests against the hull's side (its middle touches, its base is below the hull); the write rolls the hull
		0.06 rad, so the pill's upper points swing outward: not touching at the carried place, still touching where it is */
		struct pose before = rolled(0.f, 0.f, 0.f, 0.f);
		struct pose after = rolled(0.f, 0.f, 0.f, .06f);
		real_point3d base = { 0.f, 1.18f, -.6f };
		struct snapshot saved;
		short biped;

		world_new(1.f, -.5f, 2.f);
		/* (a hull 2 wide: its side at y 1, a width and a bit of room) */
		vehicles[0].low[0] = -3.f;
		vehicles[0].high[0] = 3.f;
		biped = biped_new(base, 0, NONE, zero_vector);
		biped_count = 1;
		saved.object = bipeds[biped].object;
		CHECK(touches_at(&before, biped, base) && touches_at(&after, biped, base), "the fixture's touch is not from before");
		CHECK(!touches_at(&after, biped, written(&before, &after, base)), "the fixture's carried place touches");
		write_vehicle(VEHICLE, &before, &after);
		nothing_moved(biped, &saved, "a touch from before the write");
		return 0;
	}
	CASE("write-carry-preexisting-rolled-touch")
	{
		/* the same against the side, rolled 0.05 rad (M5's size): the destination test would carry it */
		struct pose before = rolled(0.f, 0.f, 0.f, 0.f);
		struct pose after = rolled(0.f, 0.f, 0.f, .05f);
		real_point3d base = { 0.f, 1.175f, -.6f };
		struct snapshot saved;
		short biped;

		world_new(1.f, -.5f, 2.f);
		vehicles[0].low[0] = -3.f;
		vehicles[0].high[0] = 3.f;
		biped = biped_new(base, 0, NONE, zero_vector);
		biped_count = 1;
		saved.object = bipeds[biped].object;
		CHECK(touches_at(&before, biped, base), "the fixture does not touch before the write");
		CHECK(touches_at(&after, biped, base), "the fixture's unmoved base does not touch the written hull");
		CHECK(!touches_at(&after, biped, written(&before, &after, base)), "W(base) touches the written hull");
		write_vehicle(VEHICLE, &before, &after);
		nothing_moved(biped, &saved, "a rolled touch from before");
		return 0;
	}
	CASE("write-carry-destination-touching")
	{
		/* not touching before, touching after, and the sweep reaches W(base), but the upright pill there still touches
		the written hull (a roll of -0.05 rad swings its upper points into the side) */
		struct pose before = rolled(0.f, 0.f, 0.f, 0.f);
		struct pose after = rolled(0.f, 0.f, 0.f, -.05f);
		real_point3d base = { 0.f, 1.23f, 0.f };
		struct snapshot saved;
		short biped;

		world_new(1.f, -1.f, 2.f);
		vehicles[0].low[0] = -3.f;
		vehicles[0].high[0] = 3.f;
		biped = biped_new(base, 0, NONE, zero_vector);
		biped_count = 1;
		saved.object = bipeds[biped].object;
		CHECK(!touches_at(&before, biped, base), "the fixture touches before the write");
		CHECK(touches_at(&after, biped, base), "the fixture's unmoved base does not touch the written hull");
		CHECK(touches_at(&after, biped, written(&before, &after, base)), "W(base) is clear of the written hull");
		write_vehicle(VEHICLE, &before, &after);
		CHECK(carries == 1, "the sweep did not run (%d)", (int)carries);
		nothing_moved(biped, &saved, "a carry that ends touching");
		return 0;
	}
	CASE("write-carry-blocked")
	{
		/* the hull moves up toward a pill over its deck (top at 0.42): a before place at 0.43, W(base) the same place on the
		deck. (a) the sweep stops halfway, still touching: nothing moves; (b) it stops halfway, clear: moved there */
		struct pose before = pose_of(0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
		struct pose after = pose_of(0.f, 0.f, .13f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
		real_point3d base = { .5f, .3f, .43f };
		real_point3d expected;
		struct snapshot saved;
		short biped;

		/* (a) */
		world_new(3.f, -1.f, .42f);
		biped = biped_new(base, 0, NONE, zero_vector);
		biped_count = 1;
		saved.object = bipeds[biped].object;
		carry_fraction = .5f;
		CHECK(!touches_at(&before, biped, base) && touches_at(&after, biped, base), "fixture (a) is no write-made touch");
		write_vehicle(VEHICLE, &before, &after);
		CHECK(carries == 1, "the sweep did not run (a)");
		nothing_moved(biped, &saved, "a carry stopped on the hull");
		/* (b) */
		after = pose_of(0.f, 0.f, .12f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
		base.z = .5f;
		world_new(3.f, -1.f, .42f);
		biped = biped_new(base, 0, NONE, zero_vector);
		biped_count = 1;
		carry_fraction = .5f;
		CHECK(!touches_at(&before, biped, base) && touches_at(&after, biped, base), "fixture (b) is no write-made touch");
		write_vehicle(VEHICLE, &before, &after);
		set_real_point3d(&expected, base.x, base.y, base.z + .06f);
		{
			real_point3d now = base_of(biped);

			NEAR_POINT(now, expected, .00001);
		}
		CHECK(object_sets == 1, "a stopped carry placed %d times", (int)object_sets);
		return 0;
	}
	CASE("write-carry-teleport-write")
	{
		/* a write that moves the hull 0.3 at the base: a teleport, not a correction to ride */
		struct pose before = pose_of(0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
		struct pose after = pose_of(0.f, 0.f, .3f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
		real_point3d base = { .5f, .3f, .6f };
		struct snapshot saved;
		short biped;

		world_new(3.f, -1.f, .42f);
		biped = biped_new(base, 0, NONE, zero_vector);
		biped_count = 1;
		saved.object = bipeds[biped].object;
		CHECK(!touches_at(&before, biped, base) && touches_at(&after, biped, base), "the fixture is no write-made touch");
		write_vehicle(VEHICLE, &before, &after);
		nothing_moved(biped, &saved, "a write of 0.3");
		return 0;
	}
	CASE("write-carry-other-window")
	{
		struct pose before = pose_of(0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
		struct pose after = pose_of(0.f, 0.f, .1f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
		real_point3d base = { .5f, .3f, .45f };
		struct snapshot saved;
		short biped;

		world_new(3.f, -1.f, .42f);
		biped = biped_new(base, RIDING_TICKS - 1, OTHER_VEHICLE, zero_vector);
		biped_count = 1;
		saved.object = bipeds[biped].object;
		CHECK(!touches_at(&before, biped, base) && touches_at(&after, biped, base), "the fixture is no write-made touch");
		write_vehicle(VEHICLE, &before, &after);
		nothing_moved(biped, &saved, "another vehicle's window rider");
		return 0;
	}
	CASE("write-carry-unrecorded")
	{
		/* a record another vehicle's write left in the slot (made at its own write) is not this write's measure */
		struct pose before = pose_of(0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
		struct pose after = pose_of(0.f, 0.f, .1f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
		real_point3d base = { .5f, .3f, .45f };
		struct snapshot saved;
		short biped;

		world_new(3.f, -1.f, .42f);
		biped = biped_new(base, 0, NONE, zero_vector);
		biped_count = 1;
		saved.object = bipeds[biped].object;
		CHECK(!touches_at(&before, biped, base) && touches_at(&after, biped, base), "the fixture is no write-made touch");
		/* (the other vehicle's sphere holds the rider: its writing hook records the slot) */
		vehicle_pose(OTHER_VEHICLE, &before);
		vehicle_pose(VEHICLE, &before);
		biped_riding_vehicle_writing(OTHER_VEHICLE);
		CHECK(biped_riding_writes[3].recorded && biped_riding_writes[3].vehicle_index == OTHER_VEHICLE,
			"the fixture's record is not the other vehicle's");
		vehicle_pose(VEHICLE, &after);
		biped_riding_vehicle_written(VEHICLE, &before.position, &before.forward, &before.up);
		nothing_moved(biped, &saved, "another vehicle's record");
		/* (consumed: a write that was never measured is not carried by a leftover) */
		write_vehicle(VEHICLE, &before, &after);
		CHECK(object_sets == 1 && !biped_riding_writes[3].recorded, "a measured write after it not carried");
		return 0;
	}
	CASE("write-carry-seated-dead-off")
	{
		struct pose before = pose_of(0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
		struct pose after = pose_of(0.f, 0.f, .1f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
		real_point3d base = { .5f, .3f, .45f };
		struct snapshot saved;
		short biped;

		/* seated */
		world_new(3.f, -1.f, .42f);
		biped = biped_new(base, 0, NONE, zero_vector);
		biped_count = 1;
		bipeds[biped].object.parent_object_index = VEHICLE;
		saved.object = bipeds[biped].object;
		CHECK(!touches_at(&before, biped, base) && touches_at(&after, biped, base), "the fixture is no write-made touch");
		write_vehicle(VEHICLE, &before, &after);
		nothing_moved(biped, &saved, "a seated biped");
		/* dead, or the riding code off (the biped's riding does not apply) */
		world_new(3.f, -1.f, .42f);
		biped = biped_new(base, 0, NONE, zero_vector);
		biped_count = 1;
		bipeds[biped].applies = FALSE;
		saved.object = bipeds[biped].object;
		write_vehicle(VEHICLE, &before, &after);
		nothing_moved(biped, &saved, "a biped the riding code does not apply to");
		/* control: the same fixture, applying, carried */
		world_new(3.f, -1.f, .42f);
		biped = biped_new(base, 0, NONE, zero_vector);
		biped_count = 1;
		write_vehicle(VEHICLE, &before, &after);
		CHECK(object_sets == 1, "the control fixture is not carried");
		return 0;
	}
	CASE("write-carry-header-walk")
	{
		/* a write walks the players: not one of 200 bipeds without a player is looked at */
		struct pose before = pose_of(0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
		struct pose after = pose_of(0.f, 0.f, .1f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
		real_point3d base = { .5f, .3f, .45f };
		real_point3d far_away = { 50.f, 50.f, 0.f };
		short at;

		world_new(3.f, -1.f, .42f);
		biped_new(base, 0, NONE, zero_vector);
		for (at = 0; at < 200; at++)
		{
			bystander_new(far_away);
		}
		write_vehicle(VEHICLE, &before, &after);
		CHECK(object_sets == 1 && carries == 1, "the rider was not carried (sets %d)", (int)object_sets);
		CHECK(!object_walks, "the write walked the objects (%d steps)", (int)object_walks);
		return 0;
	}
	CASE("write-carry-two-riders-order")
	{
		/* two riders carried by one write, the higher object index first in the player table: carried in object order */
		struct pose before = pose_of(0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
		struct pose after = pose_of(0.f, 0.f, .1f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
		real_point3d first = { .5f, .3f, .45f };
		real_point3d second = { -.5f, .3f, .45f };
		real_point3d expected, now;
		short low, high;

		world_new(3.f, -1.f, .42f);
		low = biped_new(first, 0, NONE, zero_vector);
		high = biped_new(second, 0, NONE, zero_vector);
		biped_player(low, 9);
		biped_player(high, 4);
		write_vehicle(VEHICLE, &before, &after);
		CHECK(carries == 2 && carried_order[0] == PLAYER_BIPED(low) && carried_order[1] == PLAYER_BIPED(high),
			"carried %d, first %lx then %lx", (int)carries, (unsigned long)carried_order[0], (unsigned long)carried_order[1]);
		expected = written(&before, &after, first);
		now = base_of(low);
		NEAR_POINT(now, expected, .00005);
		expected = written(&before, &after, second);
		now = base_of(high);
		NEAR_POINT(now, expected, .00005);
		return 0;
	}
	CASE("write-carry-mismatched-unit")
	{
		/* two bipeds that each name the player whose unit is the other are nobody's candidates (the walk over every object
		would carry both; no code path makes the state) */
		struct pose before = pose_of(0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
		struct pose after = pose_of(0.f, 0.f, .1f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
		real_point3d first = { .5f, .3f, .45f };
		real_point3d second = { -.5f, .3f, .45f };
		struct snapshot saved;
		short one, other;

		world_new(3.f, -1.f, .42f);
		one = biped_new(first, 0, NONE, zero_vector);
		other = biped_new(second, 0, NONE, zero_vector);
		players[3 + one].unit_index = PLAYER_BIPED(other);
		players[3 + other].unit_index = PLAYER_BIPED(one);
		saved.object = bipeds[one].object;
		CHECK(!touches_at(&before, one, first) && touches_at(&after, one, first), "the fixture is no write-made touch");
		write_vehicle(VEHICLE, &before, &after);
		nothing_moved(one, &saved, "a biped of a player whose unit is another");
		CHECK(!carries, "carried %d", (int)carries);
		return 0;
	}
	CASE("write-carry-left-player")
	{
		/* a write recorded a player's biped; the player left, another write of the vehicle came, and a player is in the
		slot again: the first write's record carries nothing */
		struct pose before = pose_of(0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
		struct pose after = pose_of(0.f, 0.f, .1f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
		real_point3d base = { .5f, .3f, .45f };
		struct snapshot saved;
		short biped;

		world_new(3.f, -1.f, .42f);
		biped = biped_new(base, 0, NONE, zero_vector);
		saved.object = bipeds[biped].object;
		vehicle_pose(VEHICLE, &before);
		biped_riding_vehicle_writing(VEHICLE);
		CHECK(biped_riding_writes[3].recorded, "the fixture's write did not record the player");
		player_used[3] = FALSE;
		bipeds[biped].unit.player_index = NONE;
		biped_riding_vehicle_writing(VEHICLE);
		bipeds[biped].unit.player_index = 0x30000003;
		player_used[3] = TRUE;
		vehicle_pose(VEHICLE, &after);
		biped_riding_vehicle_written(VEHICLE, &before.position, &before.forward, &before.up);
		nothing_moved(biped, &saved, "a record of a player who left");
		return 0;
	}
	CASE("write-carry-empty")
	{
		struct pose before = pose_of(0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
		struct pose after = pose_of(0.f, 0.f, .1f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);

		world_new(3.f, -1.f, .42f);
		write_vehicle(VEHICLE, &before, &after);
		CHECK(!carries && !object_sets && !rebases && !object_walks, "a write with no players did work (carries %d, walks %d)",
			(int)carries, (int)object_walks);
		return 0;
	}
	CHECK(FALSE, "no case named '%s'", case_name);
	return 1;
}
