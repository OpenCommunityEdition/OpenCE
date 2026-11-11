/*
VEHICLE_CABIN.C (test)

bipeds.c's cabin test and recovery for a rider in an enclosed vehicle,
taken from the sources by test_vehicle_cabin.py, against made-up collision
models: boxes in each vehicle's body frame, solid inside, their faces
front-facing from outside.
*/

#include "harness.h"
typedef unsigned short word;
#include <math.h>
typedef struct { real i, j; } real_vector2d;
#include "enums.inc"
#include "engine.inc"
#include "cabin_types.inc"

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
static boolean game_players_are_double_speed(void);

/* the map's riding slot and its script and latch reach (vehicle_riding.c's map functions); no case here sets a map */
struct biped_datum;
static long map_slot_storage;
static void *game_state_malloc(char const *name, void *type, long size) { (void)name; (void)type; (void)size; return &map_slot_storage; }
static short hs_runtime_port_executing_script_type(void) { return NONE; }
static short hs_runtime_port_scenario_boolean(char const *name) { (void)name; return NONE; }
static void error(long priority, char const *format, ...) { (void)priority; (void)format; }
struct object_iterator { int next; };
enum { _object_mask_biped = 1 };
static void object_iterator_new(struct object_iterator *i, long mask, long flags) { (void)mask; (void)flags; i->next = 0; }
static void *object_iterator_next(struct object_iterator *i) { (void)i; return NULL; }
static void biped_riding_clear(struct biped_datum *biped) { (void)biped; }
static long game_time_get(void);
#include "under_test.inc"

/* the map's decision as vehicle_riding_objects_reset makes it: a setting changed counts from the next map */
static void decide_now(void)
{
#ifndef FIXTURE_NO_DECIDE
	vehicle_riding_map_decide();
#endif
}

/* ---------- the fake collision world */

#define VEHICLE 0x10000001
#define OTHER_VEHICLE 0x10010002
#define GROUND 0x10020003
#define NO_MODEL 0x10030004
#define BIPED 0x20000005
#define OTHER_BIPED 0x20010006
#define NEAR(a, b) CHECK(fabs((double)(a) - (double)(b)) < .001, "%s: %g, not %g", #a, (double)(a), (double)(b))

struct box
{
	real low[3];
	real high[3];
};

struct fake_object
{
	long index;
	boolean has_model;
	real_point3d origin;
	real_vector3d forward;
	real_vector3d up;
	short box_count;
	/* (the most any case builds is 12: a cabin's 7 and a second room's 5) */
	struct box boxes[16];
};

static struct fake_object world[4];
static short world_count;
static real_plane3d hit_plane;
static real_matrix4x3 nodes[1];
/* the vector tests made since the count was cleared (Task 5: a sample at the vehicle's update is a fault) */
static long vector_casts;

struct collision_model_instance
{
	long object_index;
	short node_count;
	real_matrix4x3 const *matrices;
};

struct collision_bsp_test_vector_result
{
	real t;
	real_plane3d const *plane;
	long plane_designator;
};

struct collision_model_test_vector_result
{
	short node_index;
	struct collision_bsp_test_vector_result bsp_result;
};

static real_vector3d const world_up = { 0.f, 0.f, 1.f };

static struct fake_object *fake_find(long index)
{
	short at;

	for (at = 0; at < world_count; at++)
	{
		if (world[at].index == index)
			return &world[at];
	}
	return NULL;
}

/* an object at origin, its body turned to forward and up, no boxes yet */
static struct fake_object *fake_object(long index, real_point3d origin, real_vector3d forward, real_vector3d up)
{
	struct fake_object *object = &world[world_count++];

	memset(object, 0, sizeof(*object));
	object->index = index;
	object->has_model = TRUE;
	object->origin = origin;
	object->forward = forward;
	object->up = up;
	return object;
}

static void fake_box(struct fake_object *object, real x0, real y0, real z0, real x1, real y1, real z1)
{
	struct box *box;

	CHECK(object->box_count < (short)(sizeof(object->boxes)/sizeof(object->boxes[0])), "a fake model's boxes full");
	box = &object->boxes[object->box_count++];

	box->low[0] = x0;
	box->low[1] = y0;
	box->low[2] = z0;
	box->high[0] = x1;
	box->high[1] = y1;
	box->high[2] = z1;
}

/* the first face a segment (start, way: t 0 to 1) meets in a box, by the faces asked for: entering it from
outside (front), or leaving it from inside (back); its t and outward normal, local */
static boolean fake_box_hit(struct box const *box, real const *start, real const *way, unsigned long flags, real *t,
	real *normal)
{
	real enter = -1.e30f;
	real leave = 1.e30f;
	short enter_axis = -1;
	short leave_axis = -1;
	short axis;

	for (axis = 0; axis < 3; axis++)
	{
		real a;
		real b;

		if (way[axis] == 0.f)
		{
			if (start[axis] < box->low[axis] || start[axis] > box->high[axis])
				return FALSE;
			continue;
		}
		a = (box->low[axis] - start[axis])/way[axis];
		b = (box->high[axis] - start[axis])/way[axis];
		if ((a < b ? a : b) > enter)
		{
			enter = a < b ? a : b;
			enter_axis = axis;
		}
		if ((a < b ? b : a) < leave)
		{
			leave = a < b ? b : a;
			leave_axis = axis;
		}
	}
	if (enter > leave)
		return FALSE;
	normal[0] = normal[1] = normal[2] = 0.f;
	if (enter >= 0.f && enter <= 1.f && enter_axis >= 0 && TEST_FLAG(flags, _collision_test_front_facing_surfaces_bit))
	{
		*t = enter;
		normal[enter_axis] = way[enter_axis] > 0.f ? -1.f : 1.f;
		return TRUE;
	}
	if (enter < 0.f && leave >= 0.f && leave <= 1.f && leave_axis >= 0 &&
		TEST_FLAG(flags, _collision_test_back_facing_surfaces_bit))
	{
		*t = leave;
		normal[leave_axis] = way[leave_axis] > 0.f ? 1.f : -1.f;
		return TRUE;
	}
	return FALSE;
}

/* a segment against one object's boxes: the nearest hit, its world normal in hit_plane */
static boolean fake_object_vector(struct fake_object const *object, unsigned long flags, real_point3d const *point,
	real_vector3d const *vector, real *best_t)
{
	real_point3d end;
	real_point3d local_start;
	real_point3d local_end;
	real start[3];
	real way[3];
	real best_normal[3] = { 0.f, 0.f, 0.f };
	boolean found = FALSE;
	short at;

	end.x = point->x + vector->i;
	end.y = point->y + vector->j;
	end.z = point->z + vector->k;
	vehicle_riding_point_to_local(&object->origin, &object->forward, &object->up, point, &local_start);
	vehicle_riding_point_to_local(&object->origin, &object->forward, &object->up, &end, &local_end);
	start[0] = local_start.x;
	start[1] = local_start.y;
	start[2] = local_start.z;
	way[0] = local_end.x - local_start.x;
	way[1] = local_end.y - local_start.y;
	way[2] = local_end.z - local_start.z;
	for (at = 0; at < object->box_count; at++)
	{
		real t;
		real normal[3];

		if (fake_box_hit(&object->boxes[at], start, way, flags, &t, normal) && (!found || t < *best_t))
		{
			found = TRUE;
			*best_t = t;
			best_normal[0] = normal[0];
			best_normal[1] = normal[1];
			best_normal[2] = normal[2];
		}
	}
	if (found)
	{
		real_point3d tip;
		real_point3d world_tip;
		real_point3d base;
		real_point3d zero_point = { 0.f, 0.f, 0.f };

		tip.x = best_normal[0];
		tip.y = best_normal[1];
		tip.z = best_normal[2];
		vehicle_riding_point_to_world(&object->origin, &object->forward, &object->up, &tip, &world_tip);
		vehicle_riding_point_to_world(&object->origin, &object->forward, &object->up, &zero_point, &base);
		hit_plane.n.i = world_tip.x - base.x;
		hit_plane.n.j = world_tip.y - base.y;
		hit_plane.n.k = world_tip.z - base.z;
	}
	return found;
}

boolean collision_model_instance_new(struct collision_model_instance *instance, long object_index)
{
	struct fake_object *object = fake_find(object_index);

	if (!object || !object->has_model)
		return FALSE;
	instance->object_index = object_index;
	instance->node_count = 1;
	instance->matrices = nodes;
	return TRUE;
}

boolean collision_model_test_point(struct collision_model_instance const *instance, real_point3d const *point)
{
	struct fake_object *object = fake_find(instance->object_index);
	real_point3d local;
	short at;

	vehicle_riding_point_to_local(&object->origin, &object->forward, &object->up, point, &local);
	for (at = 0; at < object->box_count; at++)
	{
		struct box const *box = &object->boxes[at];

		if (local.x >= box->low[0] && local.x <= box->high[0] && local.y >= box->low[1] && local.y <= box->high[1] &&
			local.z >= box->low[2] && local.z <= box->high[2])
		{
			return TRUE;
		}
	}
	return FALSE;
}

boolean collision_model_test_vector(struct collision_model_instance const *instance, unsigned long flags,
	real_point3d const *point, real_vector3d const *vector, struct collision_model_test_vector_result *result)
{
	real t = 0.f;

	vector_casts++;
	if (!fake_object_vector(fake_find(instance->object_index), flags, point, vector, &t))
		return FALSE;
	result->node_index = 0;
	result->bsp_result.t = t;
	result->bsp_result.plane = &hit_plane;
	result->bsp_result.plane_designator = 0;
	return TRUE;
}

/* every object's model at once, the ground and other vehicles too (a fault's: the floor must be the vehicle's own) */
static boolean fake_world_vector(unsigned long flags, real_point3d const *point, real_vector3d const *vector,
	struct collision_model_test_vector_result *result)
{
	real best = 2.f;
	real_plane3d plane;
	short at;

	for (at = 0; at < world_count; at++)
	{
		real t;

		if (world[at].has_model && fake_object_vector(&world[at], flags, point, vector, &t) && t < best)
		{
			best = t;
			plane = hit_plane;
		}
	}
	if (best > 1.f)
		return FALSE;
	hit_plane = plane;
	result->node_index = 0;
	result->bsp_result.t = best;
	result->bsp_result.plane = &hit_plane;
	result->bsp_result.plane_designator = 0;
	return TRUE;
}

/* (every fake node is the identity: hits come back in world terms) */
static void matrix4x3_transform_normal(real_matrix4x3 const *matrix, real_vector3d const *normal, real_vector3d *out)
{
	(void)matrix;
	*out = *normal;
}

/* bipeds.c's world: a biped (its player's slot and whether it is dead) and its tags */
struct biped_datum
{
	short player;
	boolean dead;
	struct vehicle_riding_state riding;
	struct { unsigned long flags; } unit;
	struct
	{
		unsigned long flags;
		long elevator_object_index;
		short elevator_ticks;
	} biped;
};

struct biped_definition
{
	struct
	{
		real jump_velocity;
		real runtime_minimum_normal_k;
	} biped;
};

static real global_gravity = .005f;

/* (bipeds.c's: a living player's slot) */
static short biped_riding_player(struct biped_datum const *biped)
{
	return biped->player;
}

static boolean biped_riding_applies(struct biped_datum const *biped)
{
	return biped->player != NONE && !biped->dead;
}

/* the tick's native physics result is supplied by the case; riding and cabin decisions are extracted */
struct biped_physics
{
	real_point3d position;
	real height, width;
	unsigned long out_flags;
	long support_object_index;
	short support_node_index;
	real_vector3d velocity;
	real_vector3d support_velocity;
	real_plane3d ground_plane;
	real_point3d new_position;
	real_vector3d new_velocity;
	long cabin_object_index;
	long carrier_object_index;
	short carrier_node_index;
};

static struct vehicle_riding_vehicle tick_vehicle;
static real_vector3d const zero_vector = { 0.f, 0.f, 0.f };
static real_vector3d const *const global_zero_vector3d = &zero_vector;
#define SET_FLAG(flags, bit, value) ((value) ? ((flags) |= (unsigned)FLAG(bit)) : ((flags) &= ~(unsigned)FLAG(bit)))

static void biped_riding_get(struct biped_datum const *biped, struct vehicle_riding_state *state)
{
	*state = biped->riding;
}

static void biped_riding_set(struct biped_datum *biped, struct vehicle_riding_state const *state)
{
	biped->riding = *state;
}

static boolean biped_riding_vehicle(long index, short node, struct vehicle_riding_vehicle *vehicle)
{
	(void)node;
	if (index != tick_vehicle.object_index)
		return FALSE;
	*vehicle = tick_vehicle;
	return TRUE;
}

static boolean game_players_are_double_speed(void) { return FALSE; }
static real biped_riding_damage_speed(struct biped_datum const *biped) { (void)biped; return 100.f; }

/* bipeds.c's world for the recovery, beside Task 5's biped, tags and gravity: the fix, the carry, the touch, the hit,
the log */
static short pushes;
static short damages;
static short logged;
static long test_tick = 1;
/* (sizeof names the line's values without running them, so a value kept
only for the log is still used) */
static int error_values(char const *format, ...);
#define error(priority, ...) ((void)(priority), (void)sizeof(error_values(__VA_ARGS__)), logged++)

static long game_time_get(void)
{
	return test_tick;
}

static void physics_biped_vehicle_collision_push(long vehicle_index, long biped_index, real_vector3d *push)
{
	(void)vehicle_index;
	(void)biped_index;
	(void)push;
	pushes++;
}

static void physics_biped_vehicle_collision_damage(long vehicle_index, long biped_index, real_vector3d const *push)
{
	(void)vehicle_index;
	(void)biped_index;
	(void)push;
	damages++;
}

/* whether a pill (its axis from base up height, its radius width) comes within width of one object's boxes */
static boolean fake_pill_touches(struct fake_object const *object, real_point3d const *base, real height, real width)
{
	short sample;
	short at;

	for (sample = 0; sample <= 4; sample++)
	{
		real_point3d world_point = *base;
		real_point3d local;

		world_point.z += height*(real)sample/4.f;
		vehicle_riding_point_to_local(&object->origin, &object->forward, &object->up, &world_point, &local);
		for (at = 0; at < object->box_count; at++)
		{
			struct box const *box = &object->boxes[at];
			real coordinates[3];
			real squared = 0.f;
			short axis;

			coordinates[0] = local.x;
			coordinates[1] = local.y;
			coordinates[2] = local.z;
			for (axis = 0; axis < 3; axis++)
			{
				real outside = coordinates[axis] < box->low[axis] ? box->low[axis] - coordinates[axis] :
					coordinates[axis] > box->high[axis] ? coordinates[axis] - box->high[axis] : 0.f;

				squared += outside*outside;
			}
			if (squared <= width*width)
				return TRUE;
		}
	}
	return FALSE;
}

/* (as bipeds.c's: that vehicle's features at the pill's width less 1/64) */
static boolean biped_riding_touches(long vehicle_index, real_point3d const *base, real_vector3d const *ahead,
	real height, real width)
{
	real_point3d moved = *base;

	moved.x += ahead->i;
	moved.y += ahead->j;
	moved.z += ahead->k;
	return fake_pill_touches(fake_find(vehicle_index), &moved, height, width - .015625f);
}

/* the carry: the pill's base and top swept along the move through every model but the vehicle's, stopped short of
the first face either meets */
static void biped_riding_carry(long biped_index, struct biped_datum const *biped, struct biped_definition const *definition,
	long vehicle_index, real_vector3d const *displacement, real height, real width, real_point3d *position)
{
	real first = 1.f;
	real length = magnitude3d(displacement);
	short end;
	short at;

	(void)biped_index;
	(void)biped;
	(void)definition;
	(void)width;
	for (end = 0; end < 2; end++)
	{
		real_point3d start = *position;

		start.z += end ? height : 0.f;
		for (at = 0; at < world_count; at++)
		{
			real t;

			if (world[at].index != vehicle_index && world[at].has_model &&
				fake_object_vector(&world[at], FLAG(_collision_test_front_facing_surfaces_bit), &start, displacement, &t) &&
				t < first)
			{
				first = t;
			}
		}
	}
	if (first < 1.f && length > 0.f)
		first = first - .01f/length > 0.f ? first - .01f/length : 0.f;
	position->x += displacement->i*first;
	position->y += displacement->j*first;
	position->z += displacement->k*first;
}

/* the fix: the first of six offsets, up first, where the pill is clear of every model */
static boolean collision_fix_pill(unsigned long flags, real_point3d const *old_position, real distance, real height,
	real width, long ignore_object_index, real_point3d *new_position)
{
	static real const offsets[6][3] = { { 0, 0, 1 }, { 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, -1 } };
	short offset;

	(void)flags;
	(void)ignore_object_index;
	for (offset = 0; offset < 6; offset++)
	{
		real_point3d candidate = *old_position;
		boolean clear = TRUE;
		short at;

		candidate.x += offsets[offset][0]*distance;
		candidate.y += offsets[offset][1]*distance;
		candidate.z += offsets[offset][2]*distance;
		for (at = 0; at < world_count && clear; at++)
		{
			/* (the touch's band, as biped_riding_touches: a pill resting on a floor is clear) */
			if (world[at].has_model && fake_pill_touches(&world[at], &candidate, height, width - .015625f))
				clear = FALSE;
		}
		if (clear)
		{
			*new_position = candidate;
			return TRUE;
		}
	}
	return FALSE;
}
#include "bipeds_cabin.inc"

/* ---------- the made-up vehicles */

#define HEIGHT .6f
#define WIDTH .2f
#define JUMP .9f

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

/* a cabin 3 long, 2 wide, its floor's top at z 0 and its roof's underside at 1.3 (a standing pill's top, 0.8, is
0.5 under it; a crouched one's, 0.55, 0.75), walls round it, a door in its left wall from x 0 to 0.8; with_roof
and with_floor FALSE leave those out (an open top, a belly with nothing under it) */
static struct fake_object *cabin(long index, real_point3d origin, real_vector3d forward, real_vector3d up,
	boolean with_roof, boolean with_floor)
{
	struct fake_object *object = fake_object(index, origin, forward, up);

	if (with_floor)
		fake_box(object, -1.5f, -1.f, -.1f, 1.5f, 1.f, 0.f);
	if (with_roof)
		fake_box(object, -1.5f, -1.f, 1.3f, 1.5f, 1.f, 1.4f);
	fake_box(object, -1.6f, -1.1f, -.1f, -1.5f, 1.1f, 1.4f);
	fake_box(object, 1.5f, -1.1f, -.1f, 1.6f, 1.1f, 1.4f);
	fake_box(object, -1.5f, -1.1f, -.1f, 1.5f, -1.f, 1.4f);
	fake_box(object, -1.5f, 1.f, -.1f, 0.f, 1.1f, 1.4f);
	fake_box(object, .8f, 1.f, -.1f, 1.5f, 1.1f, 1.4f);
	return object;
}

static void ground(void)
{
	fake_box(fake_object(GROUND, point(0.f, 0.f, 0.f), vector(1.f, 0.f, 0.f), vector(0.f, 0.f, 1.f)),
		-50.f, -50.f, -1.f, 50.f, 50.f, -.5f);
}

#define EXACT(a, b) CHECK(fabs((double)(a) - (double)(b)) < 1.e-6, "%s: %.9g, not %.9g", #a, (double)(a), (double)(b))

/* a bed 2 long and 1.2 wide, its floor's top at z 0, open above, at x on world x */
static void bed(real x)
{
	world_count = 0;
	fake_box(fake_object(VEHICLE, point(x, 0.f, 0.f), vector(1.f, 0.f, 0.f), vector(0.f, 0.f, 1.f)),
		-1.f, -.6f, -.1f, 1.f, .6f, 0.f);
}

/* a biped that stood on the bed at deck x, took off there (its window as vehicle_riding_record leaves it,
anchored to the bed's body) and is .5 over it; the bed then driven speed along its forward in its next update:
the stored window, the bed and the pill base as the biped's next tick finds them */
static void moving_bed(real speed, real deck_x, struct biped_datum *biped, struct biped_definition *definition,
	struct biped_physics *physics)
{
	real_vector3d const forward = { 1.f, 0.f, 0.f };
	real_vector3d const up = { 0.f, 0.f, 1.f };
	real_vector3d const zero = { 0.f, 0.f, 0.f };
	real_point3d base = point(deck_x, 0.f, .5f);
	real_vector3d velocity = zero;
	struct vehicle_riding_step step;

	memset(biped, 0, sizeof(*biped));
	memset(physics, 0, sizeof(*physics));
	memset(&tick_vehicle, 0, sizeof(tick_vehicle));
	biped->player = 3;
	SET_FLAG(biped->biped.flags, _biped_airborne_bit, TRUE);
	definition->biped.jump_velocity = (real)sqrt(2.*.005*JUMP);
	definition->biped.runtime_minimum_normal_k = .5f;
	vehicle_riding_cabin_clear(vehicle_riding_player_cabin(3));
	tick_vehicle.object_index = VEHICLE;
	tick_vehicle.node_index = NONE;
	/* (a memset correction is no identity: the window's write test would read garbage) */
	tick_vehicle.correction = *vehicle_riding_vehicle_correction(VEHICLE);
	tick_vehicle.forward = tick_vehicle.body_forward = forward;
	tick_vehicle.up = tick_vehicle.body_up = up;
	tick_vehicle.bounding_sphere_radius = 3.f;
	tick_vehicle.updates = 10;
	bed(0.f);
	vehicle_riding_clear(&biped->riding);
	CHECK(vehicle_riding_record(&biped->riding, NULL, &tick_vehicle, TRUE, &zero, &up, &base, FALSE, &velocity),
		"the bed not claimed");
	CHECK(vehicle_riding_snapshot_step(&biped->riding, &tick_vehicle, &base, HEIGHT, WIDTH, FALSE, 100.f, &step),
		"the take-off step refused");
	vehicle_riding_record(&biped->riding, &step, NULL, FALSE, &zero, &up, &base, FALSE, &velocity);
	CHECK(biped->riding.ticks == RIDING_TICKS - 1 && biped->riding.object_index == VEHICLE, "no window at the take-off");
	tick_vehicle.updates++;
	tick_vehicle.position.x = tick_vehicle.origin.x = tick_vehicle.bounding_sphere_center.x = speed;
	tick_vehicle.translational_velocity = vector(speed, 0.f, 0.f);
	bed(speed);
	physics->position = base;
	physics->height = HEIGHT;
	physics->width = WIDTH;
}

/* a rider in the window of a cabin tilted tilt degrees and driven speed along its forward in its next update, the
window taken off at deck x and the base as the biped's next tick finds it: a tick of the cabin's travel behind its
place on it */
static void tilted_window(real speed, real deck_x, real tilt, struct biped_datum *biped, struct biped_definition *definition,
	struct biped_physics *physics)
{
	real_vector3d const zero = { 0.f, 0.f, 0.f };
	real_vector3d up = vector((real)sin(tilt*PI/180.), 0.f, (real)cos(tilt*PI/180.));
	real_vector3d forward = vector(up.k, 0.f, -up.i);
	real_point3d base = point(deck_x*forward.i + WIDTH*up.i, 0.f, deck_x*forward.k + WIDTH*up.k);
	real_vector3d velocity = zero;
	struct vehicle_riding_step step;

	memset(biped, 0, sizeof(*biped));
	memset(physics, 0, sizeof(*physics));
	memset(&tick_vehicle, 0, sizeof(tick_vehicle));
	biped->player = 3;
	SET_FLAG(biped->biped.flags, _biped_airborne_bit, TRUE);
	definition->biped.jump_velocity = (real)sqrt(2.*.005*JUMP);
	definition->biped.runtime_minimum_normal_k = .5f;
	vehicle_riding_cabin_clear(vehicle_riding_player_cabin(3));
	tick_vehicle.object_index = VEHICLE;
	tick_vehicle.node_index = NONE;
	tick_vehicle.correction = *vehicle_riding_vehicle_correction(VEHICLE);
	tick_vehicle.forward = tick_vehicle.body_forward = forward;
	tick_vehicle.up = tick_vehicle.body_up = up;
	tick_vehicle.bounding_sphere_radius = 3.f;
	tick_vehicle.updates = 10;
	vehicle_riding_clear(&biped->riding);
	CHECK(vehicle_riding_record(&biped->riding, NULL, &tick_vehicle, TRUE, &zero, &up, &base, FALSE, &velocity),
		"the cabin not claimed");
	CHECK(vehicle_riding_snapshot_step(&biped->riding, &tick_vehicle, &base, HEIGHT, WIDTH, FALSE, 100.f, &step),
		"the take-off step refused");
	vehicle_riding_record(&biped->riding, &step, NULL, FALSE, &zero, &up, &base, FALSE, &velocity);
	CHECK(biped->riding.ticks == RIDING_TICKS - 1, "no window at the take-off");
	tick_vehicle.updates++;
	tick_vehicle.position.x = tick_vehicle.origin.x = tick_vehicle.bounding_sphere_center.x = speed*forward.i;
	tick_vehicle.position.z = tick_vehicle.origin.z = tick_vehicle.bounding_sphere_center.z = speed*forward.k;
	tick_vehicle.translational_velocity = vector(speed*forward.i, 0.f, speed*forward.k);
	world_count = 0;
	cabin(VEHICLE, tick_vehicle.origin, forward, up, TRUE, TRUE);
	physics->position = base;
	physics->height = HEIGHT;
	physics->width = WIDTH;
}

/* a rider of a level cabin standing (ticks RIDING_TICKS) or in its window (less), its anchor at deck x -.5, its base at
base_x (behind the rear wall at -1.7: through the hull) */
static void anchored_rider(short ticks, real base_x, struct biped_datum *biped, struct biped_definition *definition,
	struct biped_physics *physics)
{
	real_vector3d const forward = { 1.f, 0.f, 0.f };
	real_vector3d const up = { 0.f, 0.f, 1.f };
	real_vector3d const zero = { 0.f, 0.f, 0.f };
	real_point3d base = point(base_x, 0.f, WIDTH);
	real_point3d anchor = point(-.5f, 0.f, WIDTH);
	real_vector3d velocity = zero;
	struct vehicle_riding_cabin *record = vehicle_riding_player_cabin(3);

	memset(biped, 0, sizeof(*biped));
	memset(physics, 0, sizeof(*physics));
	memset(&tick_vehicle, 0, sizeof(tick_vehicle));
	biped->player = 3;
	SET_FLAG(biped->biped.flags, _biped_airborne_bit, ticks != RIDING_TICKS);
	definition->biped.jump_velocity = (real)sqrt(2.*.005*JUMP);
	definition->biped.runtime_minimum_normal_k = .5f;
	tick_vehicle.object_index = VEHICLE;
	tick_vehicle.node_index = NONE;
	tick_vehicle.correction = *vehicle_riding_vehicle_correction(VEHICLE);
	tick_vehicle.forward = tick_vehicle.body_forward = forward;
	tick_vehicle.up = tick_vehicle.body_up = up;
	tick_vehicle.bounding_sphere_radius = 3.f;
	tick_vehicle.updates = 10;
	vehicle_riding_clear(&biped->riding);
	CHECK(vehicle_riding_record(&biped->riding, NULL, &tick_vehicle, TRUE, &zero, &up, &base, FALSE, &velocity),
		"the cabin not claimed");
	biped->riding.ticks = (char)ticks;
	tick_vehicle.updates++;
	world_count = 0;
	cabin(VEHICLE, point(0.f, 0.f, 0.f), forward, up, TRUE, TRUE);
	vehicle_riding_cabin_clear(record);
	record->unit_index = BIPED;
	record->vehicle_index = VEHICLE;
	vehicle_riding_cabin_settled(record, _vehicle_riding_cabin_settle_none, TRUE, &anchor);
	record->standing = _vehicle_riding_enclosure_enclosed;
	physics->position = physics->new_position = point(base_x, 0.f, WIDTH);
	physics->height = HEIGHT;
	physics->width = WIDTH;
	physics->support_object_index = NONE;
	physics->cabin_object_index = NONE;
	physics->carrier_object_index = NONE;
	physics->carrier_node_index = NONE;
	physics->support_node_index = NONE;
	physics->ground_plane.n = up;
	SET_FLAG(physics->out_flags, _biped_physics_out_airborne_bit, TRUE);
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	real_vector3d const forward = { 1.f, 0.f, 0.f };
	real_vector3d const level = { 0.f, 0.f, 1.f };

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

	CASE("tilt-both-gates")
	{
		struct biped_datum biped;
		struct biped_definition definition;
		struct vehicle_riding_vehicle vehicle;
		struct vehicle_riding_state state;
		struct vehicle_riding_cabin *record = vehicle_riding_player_cabin(3);
		real_vector3d up50 = vector((real)sin(50.*PI/180.), 0.f, (real)cos(50.*PI/180.));
		real_vector3d forward50 = vector(up50.k, 0.f, -up50.i);
		real_point3d base = point(up50.i*(WIDTH + .1f), 0.f, up50.k*(WIDTH + .1f));

		biped.player = 3;
		biped.dead = FALSE;
		definition.biped.jump_velocity = (real)sqrt(2.*.005*JUMP);
		/* a 60 degree surface limit: protection starts at 45, the window ray's own gate */
		definition.biped.runtime_minimum_normal_k = .5f;
		/* a cabin tilted 50 degrees, the rider in its window just over its floor */
		world_count = 0;
		cabin(VEHICLE, point(0.f, 0.f, 0.f), forward50, up50, TRUE, TRUE);
		memset(&vehicle, 0, sizeof(vehicle));
		vehicle.object_index = VEHICLE;
		vehicle.node_index = NONE;
		vehicle.forward = vehicle.body_forward = forward50;
		vehicle.up = vehicle.body_up = up50;
		vehicle.correction = *vehicle_riding_vehicle_correction(VEHICLE);
		vehicle_riding_clear(&state);
		state.object_index = VEHICLE;
		state.ticks = 40;
		state.falling = 1;
		CHECK(vehicle_riding_rebase(&state, &vehicle, &base), "the tilted window not anchored");
		vehicle_riding_cabin_clear(record);
		record->unit_index = BIPED;
		record->vehicle_index = VEHICLE;
		record->jump = _vehicle_riding_enclosure_enclosed;
		/* the old 45 degree gate loses a 50 degree deck */
		CHECK(!biped_riding_over_deck(&definition, &vehicle, &base, HEIGHT), "the window ray kept a 50 degree deck");
		/* an enclosed rider: protected at the biped's tick (its exception) and at the vehicle's update (its stored verdict,
		no ray of the cabin: the window ray's own one cast only) */
		CHECK(biped_riding_window_over_deck(&definition, &state, &vehicle, &base, HEIGHT, TRUE), "the biped's tick unprotected");
		vector_casts = 0;
		CHECK(biped_riding_recast_over_deck(BIPED, &biped, &definition, &state, &vehicle, &base, HEIGHT),
			"the vehicle's update unprotected");
		CHECK(vector_casts == 1, "%ld rays cast at the vehicle's update", vector_casts);
		/* an open rider keeps the old gate at both */
		record->jump = _vehicle_riding_enclosure_open;
		CHECK(!biped_riding_window_over_deck(&definition, &state, &vehicle, &base, HEIGHT, FALSE) &&
			!biped_riding_recast_over_deck(BIPED, &biped, &definition, &state, &vehicle, &base, HEIGHT),
			"an open rider protected");
		/* another unit's record, a dead rider, no window: unprotected */
		record->jump = _vehicle_riding_enclosure_enclosed;
		CHECK(!biped_riding_recast_over_deck(OTHER_BIPED, &biped, &definition, &state, &vehicle, &base, HEIGHT),
			"another unit's verdict protected it");
		biped.dead = TRUE;
		CHECK(!biped_riding_recast_over_deck(BIPED, &biped, &definition, &state, &vehicle, &base, HEIGHT),
			"a dead rider protected");
		biped.dead = FALSE;
		state.ticks = 0;
		CHECK(!biped_riding_recast_over_deck(BIPED, &biped, &definition, &state, &vehicle, &base, HEIGHT),
			"protected with no window");
		/* level, the old gate alone decides (an enclosed rider's level vehicle is a deck as before) */
		state.ticks = 40;
		world_count = 0;
		cabin(VEHICLE, point(0.f, 0.f, 0.f), forward, level, TRUE, TRUE);
		vehicle.forward = vehicle.body_forward = forward;
		vehicle.up = vehicle.body_up = level;
		base = point(0.f, 0.f, WIDTH + .1f);
		CHECK(vehicle_riding_rebase(&state, &vehicle, &base), "the level window not anchored");
		CHECK(biped_riding_recast_over_deck(BIPED, &biped, &definition, &state, &vehicle, &base, HEIGHT),
			"a level deck lost");
		return 0;
	}

	CASE("protected-own-hit")
	{
		struct biped_datum biped;
		struct biped_definition definition;
		struct vehicle_riding_vehicle vehicle;
		struct vehicle_riding_state state;
		struct vehicle_riding_cabin *record = vehicle_riding_player_cabin(3);
		real_vector3d up50 = vector((real)sin(50.*PI/180.), 0.f, (real)cos(50.*PI/180.));
		real_vector3d forward50 = vector(up50.k, 0.f, -up50.i);
		real_point3d base = point(up50.i*(WIDTH + .1f), 0.f, up50.k*(WIDTH + .1f));

		/* a rider on a cabin tilted 50 degrees at the vehicle's update: its own standing hit protects it, whatever the
		host's word of a tick before said (a miss there never revokes it, and the host's enclosed holds) */
		biped.player = 3;
		biped.dead = FALSE;
		definition.biped.jump_velocity = (real)sqrt(2.*.005*JUMP);
		definition.biped.runtime_minimum_normal_k = .5f;
		world_count = 0;
		cabin(VEHICLE, point(0.f, 0.f, 0.f), forward50, up50, TRUE, TRUE);
		memset(&vehicle, 0, sizeof(vehicle));
		vehicle.object_index = VEHICLE;
		vehicle.node_index = NONE;
		vehicle.forward = vehicle.body_forward = forward50;
		vehicle.up = vehicle.body_up = up50;
		vehicle.correction = *vehicle_riding_vehicle_correction(VEHICLE);
		vehicle_riding_clear(&state);
		state.object_index = VEHICLE;
		state.ticks = RIDING_TICKS;
		CHECK(vehicle_riding_rebase(&state, &vehicle, &base), "the tilted rider not anchored");
		vehicle_riding_cabin_clear(record);
		record->unit_index = BIPED;
		record->vehicle_index = VEHICLE;
		CHECK(vehicle_riding_cabin_take_host(record, BIPED, &state, VEHICLE, _vehicle_riding_enclosure_open,
			_vehicle_riding_cabin_sample_standing, TRUE, TRUE), "the host's word not taken");
		record->standing = _vehicle_riding_enclosure_enclosed;
		CHECK(biped_riding_recast_over_deck(BIPED, &biped, &definition, &state, &vehicle, &base, HEIGHT),
			"the vehicle's update left its own hit unprotected");
		record->standing = _vehicle_riding_enclosure_open;
		CHECK(!biped_riding_recast_over_deck(BIPED, &biped, &definition, &state, &vehicle, &base, HEIGHT),
			"two opens protected at the vehicle's update");
		return 0;
	}
	CASE("window-moving-deck-rear")
	{
		struct biped_datum biped;
		struct biped_definition definition;
		struct biped_physics physics;
		struct vehicle_riding_vehicle before;

		moving_bed(.217f, -.85f, &biped, &definition, &physics);
		CHECK(biped_window_tick(BIPED, &biped, &definition, &physics), "a rider over the bed's rear judged off it");
		CHECK(biped_riding_recast_over_deck(BIPED, &biped, &definition, &biped.riding, &tick_vehicle, &physics.position,
			HEIGHT), "the vehicle's update judged it off the bed");
		before = tick_vehicle;
		before.position.x = before.origin.x = before.bounding_sphere_center.x = 0.f;
		before.updates--;
		bed(0.f);
		CHECK(biped_riding_recast_over_deck(BIPED, &biped, &definition, &biped.riding, &before, &physics.position,
			HEIGHT), "the update before the move judged it off the bed");
		return 0;
	}
	CASE("window-moving-deck-front-left")
	{
		struct biped_datum biped;
		struct biped_definition definition;
		struct biped_physics physics;

		moving_bed(.217f, 1.05f, &biped, &definition, &physics);
		CHECK(!biped_window_tick(BIPED, &biped, &definition, &physics), "a rider past the bed's front judged over it");
		CHECK(!biped_riding_recast_over_deck(BIPED, &biped, &definition, &biped.riding, &tick_vehicle, &physics.position,
			HEIGHT), "the vehicle's update kept a rider past the front");
		return 0;
	}
	CASE("window-moving-deck-rear-left")
	{
		struct biped_datum biped;
		struct biped_definition definition;
		struct biped_physics physics;

		moving_bed(.217f, -1.05f, &biped, &definition, &physics);
		CHECK(!biped_window_tick(BIPED, &biped, &definition, &physics), "a rider behind the bed judged over it");
		CHECK(!biped_riding_recast_over_deck(BIPED, &biped, &definition, &biped.riding, &tick_vehicle, &physics.position,
			HEIGHT), "the vehicle's update kept a rider behind the bed");
		return 0;
	}
	CASE("window-stationary-deck")
	{
		struct biped_datum biped;
		struct biped_definition definition;
		struct biped_physics physics;
		real_point3d origin;

		moving_bed(0.f, -.95f, &biped, &definition, &physics);
		tick_vehicle.updates--;
		tick_vehicle.translational_velocity = vector(.217f, 0.f, 0.f);
		vehicle_riding_window_base(&biped.riding, &tick_vehicle, &physics.position, &origin);
		EXACT(origin.x, physics.position.x);
		EXACT(origin.y, physics.position.y);
		EXACT(origin.z, physics.position.z);
		CHECK(biped_window_tick(BIPED, &biped, &definition, &physics), "over a standing bed judged off it");
		moving_bed(0.f, -1.05f, &biped, &definition, &physics);
		tick_vehicle.updates--;
		tick_vehicle.translational_velocity = vector(.217f, 0.f, 0.f);
		vehicle_riding_window_base(&biped.riding, &tick_vehicle, &physics.position, &origin);
		EXACT(origin.x, physics.position.x);
		EXACT(origin.y, physics.position.y);
		EXACT(origin.z, physics.position.z);
		CHECK(!biped_window_tick(BIPED, &biped, &definition, &physics), "past a standing bed's rear judged over it");
		return 0;
	}

	CASE("cabin-interior")
	{
		struct vehicle_riding_cabin_rays rays;
		real_point3d base = point(0.f, 0.f, WIDTH);
		real_vector3d axis;

		/* a closed cabin: a roof within a jump and its own floor under the pill */
		world_count = 0;
		ground();
		cabin(VEHICLE, point(0.f, 0.f, 0.f), forward, level, TRUE, TRUE);
		CHECK(biped_riding_cabin_rays(VEHICLE, &world_up, &base, HEIGHT, JUMP, &rays), "a closed cabin open");
		NEAR(rays.ceiling_distance, .5f);
		NEAR(rays.floor_distance, WIDTH);
		CHECK(rays.valid && rays.ceiling && rays.floor && !rays.inside, "the rays");
		/* crouched (a shorter pill): its top lower, the roof still within a jump */
		CHECK(biped_riding_cabin_rays(VEHICLE, &world_up, &base, .35f, JUMP, &rays), "a crouched rider open");
		NEAR(rays.ceiling_distance, .75f);
		/* a jump too low for the roof: open */
		CHECK(!biped_riding_cabin_rays(VEHICLE, &world_up, &base, HEIGHT, .4f, &rays), "a roof past a jump enclosed");
		/* no jump: the inside test alone */
		CHECK(!biped_riding_cabin_rays(VEHICLE, &world_up, &base, HEIGHT, 0.f, &rays), "J 0 with the top free enclosed");
		/* the top inside the roof (a pill pushed up into it): enclosed over the floor, J 0 too */
		CHECK(biped_riding_cabin_rays(VEHICLE, &world_up, &base, 1.15f, 0.f, &rays) && rays.inside, "a top inside open");
		/* an open top: no roof, open */
		world_count = 0;
		ground();
		cabin(VEHICLE, point(0.f, 0.f, 0.f), forward, level, FALSE, TRUE);
		CHECK(!biped_riding_cabin_rays(VEHICLE, &world_up, &base, HEIGHT, JUMP, &rays), "an open top enclosed");
		/* a high gun over an open deck, past a jump: open */
		fake_box(&world[1], -.2f, -.2f, 2.5f, .2f, .2f, 2.6f);
		CHECK(!biped_riding_cabin_rays(VEHICLE, &world_up, &base, HEIGHT, JUMP, &rays), "a high gun enclosed");
		/* under a vehicle's belly, on the ground: a roof, no floor of its own: open */
		world_count = 0;
		ground();
		cabin(VEHICLE, point(0.f, 0.f, 1.2f), forward, level, FALSE, TRUE);
		base = point(0.f, 0.f, -.5f + WIDTH);
		CHECK(!biped_riding_cabin_rays(VEHICLE, &world_up, &base, HEIGHT, JUMP, &rays), "under the belly enclosed");
		CHECK(rays.ceiling && !rays.floor, "under the belly: the belly and no floor");
		/* another vehicle's floor under a roof of this one: open (only the vehicle's own floor counts) */
		world_count = 0;
		cabin(VEHICLE, point(0.f, 0.f, 0.f), forward, level, TRUE, FALSE);
		fake_box(fake_object(OTHER_VEHICLE, point(0.f, 0.f, 0.f), forward, level), -1.4f, -.9f, -.3f, 1.4f, .9f, 0.f);
		base = point(0.f, 0.f, WIDTH);
		CHECK(!biped_riding_cabin_rays(VEHICLE, &world_up, &base, HEIGHT, JUMP, &rays), "another vehicle's floor counted");
		/* the base inside a slab of the vehicle with nothing of it under: the slab's underside is no floor */
		world_count = 0;
		{
			struct fake_object *slab = fake_object(VEHICLE, point(0.f, 0.f, 0.f), forward, level);

			fake_box(slab, -1.5f, -1.f, -.2f, 1.5f, 1.f, .3f);
			fake_box(slab, -1.5f, -1.f, 1.3f, 1.5f, 1.f, 1.4f);
		}
		base = point(0.f, 0.f, WIDTH);
		CHECK(!biped_riding_cabin_rays(VEHICLE, &world_up, &base, HEIGHT, JUMP, &rays), "a slab's underside taken for a floor");
		/* the ground under a roof alone (the roof's underside 0.6 over the pill's top): open */
		world_count = 0;
		cabin(VEHICLE, point(0.f, 0.f, -.4f), forward, level, TRUE, FALSE);
		ground();
		base = point(0.f, 0.f, -.5f + WIDTH);
		CHECK(!biped_riding_cabin_rays(VEHICLE, &world_up, &base, HEIGHT, JUMP, &rays), "the ground's floor counted");
		/* the top inside that roof, still nothing of the vehicle under: open */
		CHECK(!biped_riding_cabin_rays(VEHICLE, &world_up, &base, 1.25f, JUMP, &rays) && rays.inside,
			"inside a roof with no floor of its own enclosed");
		/* no model: open */
		world_count = 0;
		fake_object(NO_MODEL, point(0.f, 0.f, 0.f), forward, level)->has_model = FALSE;
		CHECK(!biped_riding_cabin_rays(NO_MODEL, &world_up, &base, HEIGHT, JUMP, &rays) && !rays.valid, "no model enclosed");
		/* upside down (pitched 180), the rider on its roof's inside: the ceiling's ray from the base down, the floor's
		from the top up, along the vehicle's up */
		world_count = 0;
		cabin(VEHICLE, point(0.f, 0.f, 1.5f), vector(-1.f, 0.f, 0.f), vector(0.f, 0.f, -1.f), TRUE, TRUE);
		base = point(0.f, 0.f, .2f + WIDTH);
		axis = vector(0.f, 0.f, -1.f);
		CHECK(biped_riding_cabin_rays(VEHICLE, &axis, &base, HEIGHT, JUMP, &rays), "an inverted cabin open");
		NEAR(rays.ceiling_distance, WIDTH);
		NEAR(rays.floor_distance, .5f);
		/* tilted 60 degrees with the vehicle's up for the axis: still a cabin */
		world_count = 0;
		axis = vector((real)sin(60.*PI/180.), 0.f, (real)cos(60.*PI/180.));
		cabin(VEHICLE, point(0.f, 0.f, 0.f), vector(axis.k, 0.f, -axis.i), axis, TRUE, TRUE);
		base = point(axis.i*WIDTH, 0.f, axis.k*WIDTH);
		CHECK(biped_riding_cabin_rays(VEHICLE, &axis, &base, HEIGHT, JUMP, &rays), "a tilted cabin along its up open");
		return 0;
	}
	CASE("cabin-crossing")
	{
		real_point3d inside = point(0.f, 0.f, .4f);
		real_point3d through_wall = point(2.5f, 0.f, .4f);
		real_point3d through_door = point(.4f, 2.f, .4f);
		real_point3d by_door = point(.4f, .5f, .4f);
		real_point3d on_wall = point(1.5f, 0.f, .4f);
		real_point3d back_in = point(1.f, 0.f, .4f);
		real_point3d in_wall = point(1.55f, 0.f, .4f);

		world_count = 0;
		cabin(VEHICLE, point(0.f, 0.f, 0.f), forward, level, TRUE, TRUE);
		CHECK(biped_riding_cabin_crosses(VEHICLE, &inside, &through_wall), "through a wall no crossing");
		CHECK(biped_riding_cabin_crosses(VEHICLE, &through_wall, &inside), "back through a wall no crossing");
		CHECK(!biped_riding_cabin_crosses(VEHICLE, &by_door, &through_door), "out of the door a crossing");
		/* from inside the hull outward: its back face crossed */
		CHECK(biped_riding_cabin_crosses(VEHICLE, &in_wall, &through_wall), "out of a wall's inside no crossing");
		/* a touch at an end is no crossing; nor is a segment of no length */
		CHECK(!biped_riding_cabin_crosses(VEHICLE, &on_wall, &back_in), "an end on the wall a crossing");
		CHECK(!biped_riding_cabin_crosses(VEHICLE, &inside, &inside), "no length a crossing");
		CHECK(vehicle_riding_cabin_crossing(TRUE, .5f) && !vehicle_riding_cabin_crossing(TRUE, 0.f) &&
			!vehicle_riding_cabin_crossing(TRUE, 1.f) && !vehicle_riding_cabin_crossing(FALSE, .5f), "the crossing's t");
		return 0;
	}

	CASE("cabin-overlap-retry")
	{
		struct biped_datum biped = { 0 };
		struct biped_definition definition;
		struct vehicle_riding_vehicle vehicle = { 0 };
		struct vehicle_riding_cabin record;
		struct vehicle_riding_state state;
		real_point3d anchor = point(.6f, .8f, WIDTH);
		real_point3d entry = point(.7f, 1.12f, WIDTH);
		real_point3d base = entry;
		struct vehicle_riding_cabin_rays rays;
		short settle;

		definition.biped.jump_velocity = (real)sqrt(2.*.005*JUMP);
		definition.biped.runtime_minimum_normal_k = .7f;
		vehicle.object_index = VEHICLE;
		vehicle.node_index = NONE;
		vehicle.body_forward = vehicle.forward = forward;
		vehicle.body_up = vehicle.up = level;
		world_count = 0;
		cabin(VEHICLE, point(0.f, 0.f, 0.f), forward, level, TRUE, TRUE);
		CHECK(!biped_riding_cabin_rays(VEHICLE, &level, &base, HEIGHT, JUMP, &rays), "fixture must miss the raw rays");
		CHECK(biped_riding_touches(VEHICLE, &base, global_zero_vector3d, HEIGHT, WIDTH), "fixture must overlap the hatch edge");
		CHECK(!biped_riding_cabin_crosses(VEHICLE, &anchor, &base), "fixture must not cross the hatch");
		vehicle_riding_clear(&state);
		state.object_index = VEHICLE;
		state.ticks = RIDING_TICKS;
		vehicle_riding_cabin_clear(&record);
		vehicle_riding_cabin_begin(&record, BIPED, &state, TRUE);
		vehicle_riding_cabin_settled(&record, _vehicle_riding_cabin_settle_none, TRUE, &anchor);
		/* a crate blocks the cabin candidates, but is not a touch of the ridden vehicle */
		fake_box(fake_object(OTHER_VEHICLE, point(0.f, 0.f, 0.f), forward, level), -1.5f, -1.f, 0.f, 1.5f, 1.f, 1.3f);
		settle = biped_riding_cabin_settle(BIPED, &biped, &definition, &record, &vehicle, &entry, NULL,
			global_zero_vector3d, TRUE, HEIGHT, WIDTH, &base);
		CHECK(settle == _vehicle_riding_cabin_settle_failed, "touching raw miss became a door (%d)", settle);
		CHECK(record.have_anchor && base.x == entry.x && base.y == entry.y && pushes == 0 && damages == 0,
			"a blocked overlap moved, hurt, or lost its anchor");
		world_count = 1;
		test_tick++;
		vehicle_riding_cabin_begin(&record, BIPED, &state, TRUE);
		settle = biped_riding_cabin_settle(BIPED, &biped, &definition, &record, &vehicle, &entry, NULL,
			global_zero_vector3d, TRUE, HEIGHT, WIDTH, &base);
		CHECK(settle == _vehicle_riding_cabin_settle_anchor && record.have_anchor, "next tick did not return to the freed anchor (%d)", settle);
		NEAR(base.x, anchor.x);
		NEAR(base.y, anchor.y);
		base = entry = point(.4f, 1.6f, WIDTH);
		settle = biped_riding_cabin_settle(BIPED, &biped, &definition, &record, &vehicle, &entry, NULL,
			global_zero_vector3d, TRUE, HEIGHT, WIDTH, &base);
		CHECK(settle == _vehicle_riding_cabin_settle_door && !record.have_anchor && base.y == entry.y,
			"a clear door exit was returned");
		return 0;
	}
	CASE("cabin-failure-log")
	{
		struct biped_datum biped = { 0 };
		struct biped_definition definition;
		struct vehicle_riding_vehicle vehicle = { 0 };
		struct vehicle_riding_cabin record;
		real_point3d entry = point(1.4f, 0.f, WIDTH);
		real_point3d base;
		short tick;

		definition.biped.jump_velocity = (real)sqrt(2.*.005*JUMP);
		definition.biped.runtime_minimum_normal_k = .7f;
		vehicle.object_index = VEHICLE;
		vehicle.node_index = NONE;
		vehicle.body_forward = vehicle.forward = forward;
		vehicle.body_up = vehicle.up = level;
		world_count = 0;
		cabin(VEHICLE, point(0.f, 0.f, 0.f), forward, level, TRUE, TRUE);
		vehicle_riding_cabin_clear(&record);
		/* no anchor: the overlapping pill still logs, once initially and once per second */
		for (tick = 0; tick <= 60; tick++)
		{
			test_tick = tick;
			base = entry;
			biped_riding_cabin_settle(BIPED, &biped, &definition, &record, &vehicle, &entry, NULL,
				global_zero_vector3d, FALSE, HEIGHT, WIDTH, &base);
			biped_riding_cabin_settle(BIPED, &biped, &definition, &record, &vehicle, &entry, NULL,
				global_zero_vector3d, TRUE, HEIGHT, WIDTH, &base);
			CHECK(logged == 1 + tick/30, "failure log tick %d: %d lines, expected %d", tick, logged, 1 + tick/30);
		}
		base = entry = point(0.f, 0.f, WIDTH);
		test_tick = 61;
		biped_riding_cabin_settle(BIPED, &biped, &definition, &record, &vehicle, &entry, NULL,
			global_zero_vector3d, FALSE, HEIGHT, WIDTH, &base);
		CHECK(logged == 3, "a preliminary clear check ended the failure before movement");
		biped_riding_cabin_settle(BIPED, &biped, &definition, &record, &vehicle, &entry, NULL,
			global_zero_vector3d, TRUE, HEIGHT, WIDTH, &base);
		CHECK(logged == 4, "failure ending not logged once");
		biped_riding_cabin_settle(BIPED, &biped, &definition, &record, &vehicle, &entry, NULL,
			global_zero_vector3d, TRUE, HEIGHT, WIDTH, &base);
		CHECK(logged == 4, "failure ending logged twice");
		base = entry = point(.4f, 1.6f, WIDTH);
		biped_riding_cabin_settle(BIPED, &biped, &definition, &record, &vehicle, &entry, NULL,
			global_zero_vector3d, TRUE, HEIGHT, WIDTH, &base);
		CHECK(!record.have_anchor && logged == 4, "clear hatch exit did not end the interior history");
		base = entry = point(1.4f, 0.f, WIDTH);
		biped_riding_cabin_settle(BIPED, &biped, &definition, &record, &vehicle, &entry, NULL,
			global_zero_vector3d, TRUE, HEIGHT, WIDTH, &base);
		CHECK(logged == 5, "a new failure episode did not log immediately");
		return 0;
	}
	CASE("cabin-failure-alternate")
	{
		struct biped_datum biped = { 0 };
		struct biped_definition definition;
		struct vehicle_riding_vehicle vehicle = { 0 };
		struct vehicle_riding_cabin record;
		real_point3d entry = point(1.4f, 0.f, WIDTH);
		real_point3d hatch = point(.4f, 1.6f, WIDTH);
		real_point3d base;
		short tick;

		definition.biped.jump_velocity = (real)sqrt(2.*.005*JUMP);
		definition.biped.runtime_minimum_normal_k = .7f;
		vehicle.object_index = VEHICLE;
		vehicle.node_index = NONE;
		vehicle.body_forward = vehicle.forward = forward;
		vehicle.body_up = vehicle.up = level;
		world_count = 0;
		cabin(VEHICLE, point(0.f, 0.f, 0.f), forward, level, TRUE, TRUE);
		vehicle_riding_cabin_clear(&record);
		/* the staged check fails each tick and the final one is clear: one stuck episode, not one a tick */
		for (tick = 0; tick <= 60; tick++)
		{
			test_tick = tick;
			base = entry;
			biped_riding_cabin_settle(BIPED, &biped, &definition, &record, &vehicle, &entry, NULL,
				global_zero_vector3d, FALSE, HEIGHT, WIDTH, &base);
			base = hatch;
			biped_riding_cabin_settle(BIPED, &biped, &definition, &record, &vehicle, &hatch, NULL,
				global_zero_vector3d, TRUE, HEIGHT, WIDTH, &base);
			CHECK(logged == 1 + tick/30, "alternating failure tick %d: %d lines, expected %d", tick, logged, 1 + tick/30);
		}
		test_tick = 61;
		base = hatch;
		biped_riding_cabin_settle(BIPED, &biped, &definition, &record, &vehicle, &hatch, NULL,
			global_zero_vector3d, TRUE, HEIGHT, WIDTH, &base);
		CHECK(logged == 4, "a tick free of failure did not end the episode (%d lines)", logged);
		return 0;
	}
	CASE("cabin-recovery")
	{
		struct biped_datum biped = { 0 };
		struct biped_definition definition;
		struct vehicle_riding_vehicle vehicle;
		struct vehicle_riding_cabin record;
		real_vector3d inward = { -1.f, 0.f, 0.f };
		real_vector3d axis = { 0.f, 0.f, 1.f };
		real_point3d anchor = point(0.f, 0.f, WIDTH);
		real_point3d base;
		real_point3d entry;
		short settle;

		definition.biped.jump_velocity = (real)sqrt(2.*.005*JUMP);
		definition.biped.runtime_minimum_normal_k = .7f;
		memset(&vehicle, 0, sizeof(vehicle));
		vehicle.object_index = VEHICLE;
		vehicle.node_index = NONE;
		vehicle.body_forward = forward;
		vehicle.body_up = level;
		vehicle.forward = forward;
		vehicle.up = level;
		world_count = 0;
		ground();
		cabin(VEHICLE, point(0.f, 0.f, 0.f), forward, level, TRUE, TRUE);

		/* a candidate's provenance: an interior place free and reached; not one outside the door, not one past a wall
		in a second room, not one under the belly */
		base = point(.5f, 0.f, WIDTH);
		CHECK(biped_riding_cabin_valid(BIPED, &biped, &definition, &vehicle, &axis, &anchor, &anchor, &base, HEIGHT, WIDTH,
			JUMP), "a free interior place refused");
		base = point(.4f, 1.6f, WIDTH);
		CHECK(!biped_riding_cabin_valid(BIPED, &biped, &definition, &vehicle, &axis, &anchor, &anchor, &base, HEIGHT, WIDTH,
			JUMP), "a place outside the door taken");
		base = point(1.4f, 0.f, WIDTH);
		CHECK(!biped_riding_cabin_valid(BIPED, &biped, &definition, &vehicle, &axis, &anchor, &anchor, &base, HEIGHT, WIDTH,
			JUMP), "a place overlapping the vehicle's wall taken");
		{
			struct fake_object *room = &world[1];

			fake_box(room, 1.6f, -1.f, -.1f, 3.f, 1.f, 0.f);
			fake_box(room, 1.6f, -1.f, 1.3f, 3.f, 1.f, 1.4f);
			fake_box(room, 3.f, -1.1f, -.1f, 3.1f, 1.1f, 1.4f);
			fake_box(room, 1.6f, -1.1f, -.1f, 3.f, -1.f, 1.4f);
			fake_box(room, 1.6f, 1.f, -.1f, 3.f, 1.1f, 1.4f);
		}
		base = point(2.3f, 0.f, WIDTH);
		CHECK(!biped_riding_cabin_valid(BIPED, &biped, &definition, &vehicle, &axis, &anchor, &base, &base, HEIGHT, WIDTH,
			JUMP), "a sealed room past a wall taken");
		world_count = 0;
		ground();
		cabin(VEHICLE, point(0.f, 0.f, 1.f), forward, level, TRUE, TRUE);
		vehicle.origin = point(0.f, 0.f, 1.f);
		anchor = point(0.f, 0.f, 1.f + WIDTH);
		base = point(0.f, 0.f, 0.f);
		CHECK(!biped_riding_cabin_valid(BIPED, &biped, &definition, &vehicle, &axis, &anchor, &base, &base, HEIGHT, WIDTH,
			JUMP), "a free place under the belly taken");
		vehicle.origin = point(0.f, 0.f, 0.f);
		anchor = point(0.f, 0.f, WIDTH);

		/* an overlap with the wall: the touch's way out (the anchor past two widths), velocity left alone */
		world_count = 0;
		ground();
		cabin(VEHICLE, point(0.f, 0.f, 0.f), forward, level, TRUE, TRUE);
		vehicle_riding_cabin_clear(&record);
		record.unit_index = BIPED;
		record.vehicle_index = VEHICLE;
		record.have_anchor = TRUE;
		record.anchor = anchor;
		base = point(1.35f, 0.f, WIDTH);
		entry = base;
		settle = biped_riding_cabin_settle(BIPED, &biped, &definition, &record, &vehicle, &entry, &inward, global_zero_vector3d,
			TRUE, HEIGHT, WIDTH, &base);
		CHECK(settle == _vehicle_riding_cabin_settle_candidate, "an overlap settled %d", settle);
		NEAR(base.x, .95f);
		CHECK(record.have_anchor && fabs((double)(record.anchor.x - .95f)) < .001, "the settled place not the anchor");
		/* the anchor near: the anchor */
		record.anchor = point(1.1f, 0.f, WIDTH);
		base = point(1.35f, 0.f, WIDTH);
		settle = biped_riding_cabin_settle(BIPED, &biped, &definition, &record, &vehicle, &entry, &inward, global_zero_vector3d,
			TRUE, HEIGHT, WIDTH, &base);
		CHECK(settle == _vehicle_riding_cabin_settle_anchor, "the near anchor not taken (%d)", settle);
		/* after the carry (final off): the same settle, but the settled place is not made the anchor */
		record.anchor = anchor;
		base = point(1.35f, 0.f, WIDTH);
		entry = base;
		settle = biped_riding_cabin_settle(BIPED, &biped, &definition, &record, &vehicle, &entry, &inward, global_zero_vector3d,
			FALSE, HEIGHT, WIDTH, &base);
		CHECK(settle == _vehicle_riding_cabin_settle_candidate, "an after-carry overlap settled %d", settle);
		NEAR(base.x, .95f);
		CHECK(record.have_anchor && record.anchor.x == 0.f, "an after-carry settle moved the anchor to %g", (double)record.anchor.x);
		record.anchor = point(.2f, 0.f, WIDTH);
		base = point(.5f, 0.f, WIDTH);
		entry = base;
		settle = biped_riding_cabin_settle(BIPED, &biped, &definition, &record, &vehicle, &entry, NULL, global_zero_vector3d,
			FALSE, HEIGHT, WIDTH, &base);
		CHECK(settle == _vehicle_riding_cabin_settle_none && base.x == .5f, "a free interior place moved (%d)", settle);
		CHECK(record.anchor.x == .2f, "an after-carry check seeded the anchor from a free interior place (%g)",
			(double)record.anchor.x);
		/* out through the solid wall, free, far past two widths: back through the hull to the anchor */
		record.anchor = anchor;
		base = point(2.3f, 0.f, WIDTH);
		settle = biped_riding_cabin_settle(BIPED, &biped, &definition, &record, &vehicle, &base, NULL, global_zero_vector3d,
			TRUE, HEIGHT, WIDTH, &base);
		CHECK(settle == _vehicle_riding_cabin_settle_anchor && base.x == 0.f, "a hull escape not returned (%d)", settle);
		/* out of the door, over a sill, onto a step, down a ramp's edge: let go, the anchor forgotten */
		fake_box(&world[1], 0.f, 1.f, 0.f, .8f, 1.1f, .05f);
		fake_box(&world[1], 0.f, 1.1f, -.3f, .8f, 2.f, -.2f);
		fake_box(&world[0], -.5f, 2.2f, -.5f, 1.f, 3.f, .15f);
		{
			static real const outside[3][3] = { { .4f, 1.6f, .2f }, { .4f, 1.6f, 0.f }, { .4f, 2.6f, .35f } };
			short at;

			for (at = 0; at < 3; at++)
			{
				record.have_anchor = TRUE;
				record.anchor = anchor;
				base = point(outside[at][0], outside[at][1], outside[at][2]);
				entry = base;
				settle = biped_riding_cabin_settle(BIPED, &biped, &definition, &record, &vehicle, &entry, NULL,
					global_zero_vector3d, TRUE, HEIGHT, WIDTH, &base);
				CHECK(settle == _vehicle_riding_cabin_settle_door && !record.have_anchor && base.y == entry.y,
					"door exit %d dragged back or kept its anchor (%d)", at, settle);
			}
		}
		/* no history (a reset or a lost window): nothing, wherever it is */
		vehicle_riding_cabin_clear(&record);
		base = point(2.3f, 0.f, WIDTH);
		settle = biped_riding_cabin_settle(BIPED, &biped, &definition, &record, &vehicle, &base, NULL, global_zero_vector3d,
			TRUE, HEIGHT, WIDTH, &base);
		CHECK(settle == _vehicle_riding_cabin_settle_none && base.x == 2.3f, "returned with no history");
		/* another vehicle between the escape and the anchor: the return blocked, nothing moves, nothing hurts */
		record.unit_index = BIPED;
		record.vehicle_index = VEHICLE;
		record.have_anchor = TRUE;
		record.anchor = anchor;
		fake_box(fake_object(OTHER_VEHICLE, point(0.f, 0.f, 0.f), forward, level), 1.8f, -2.f, -.5f, 2.f, 2.f, 3.f);
		base = point(2.3f, 0.f, WIDTH);
		pushes = damages = logged = 0;
		settle = biped_riding_cabin_settle(BIPED, &biped, &definition, &record, &vehicle, &base, NULL, global_zero_vector3d,
			TRUE, HEIGHT, WIDTH, &base);
		CHECK(settle == _vehicle_riding_cabin_settle_failed && base.x == 2.3f, "returned through another vehicle (%d)", settle);
		/* every interior place blocked (a crate filling the cabin, another rider's pill alike): no move, no push, no
		damage, logged, the entry kept */
		world_count = 0;
		ground();
		cabin(VEHICLE, point(0.f, 0.f, 0.f), forward, level, TRUE, TRUE);
		fake_box(fake_object(OTHER_VEHICLE, point(0.f, 0.f, 0.f), forward, level), -1.5f, -1.f, 0.f, 1.15f, 1.f, 1.3f);
		vehicle_riding_cabin_clear(&record);
		record.have_anchor = TRUE;
		record.anchor = anchor;
		entry = point(1.4f, 0.f, WIDTH);
		base = point(1.35f, 0.f, WIDTH);
		pushes = damages = logged = 0;
		settle = biped_riding_cabin_settle(BIPED, &biped, &definition, &record, &vehicle, &entry, &inward, global_zero_vector3d,
			TRUE, HEIGHT, WIDTH, &base);
		CHECK(settle == _vehicle_riding_cabin_settle_failed, "a blocked interior settled %d", settle);
		CHECK(base.x == entry.x && pushes == 0 && damages == 0 && logged == 1, "a failure moved %g, pushed %d, hurt %d, "
			"logged %d", (double)base.x, pushes, damages, logged);
		CHECK(record.have_anchor && record.anchor.x == 0.f, "a failure changed the anchor");
		return 0;
	}

	CASE("cabin-deferred-touch")
	{
		struct biped_datum biped = { 0 };
		struct biped_definition definition;
		struct vehicle_riding_vehicle vehicle;
		struct vehicle_riding_cabin record;
		real_vector3d inward = { -1.f, 0.f, 0.f };
		real_vector3d velocity = { 0.f, 0.f, 0.f };
		real_point3d base;
		real_point3d entry;
		boolean settled;

		definition.biped.jump_velocity = (real)sqrt(2.*.005*JUMP);
		definition.biped.runtime_minimum_normal_k = .7f;
		memset(&vehicle, 0, sizeof(vehicle));
		vehicle.object_index = VEHICLE;
		vehicle.node_index = NONE;
		vehicle.body_forward = forward;
		vehicle.body_up = level;
		vehicle.forward = forward;
		vehicle.up = level;
		tick_vehicle = vehicle;
		world_count = 0;
		ground();
		cabin(VEHICLE, point(0.f, 0.f, 0.f), forward, level, TRUE, TRUE);
		vehicle_riding_cabin_clear(&record);
		record.unit_index = BIPED;
		record.vehicle_index = VEHICLE;

		/* a touch the vehicle's update deferred, met by a rider's tick whose own gate is closed (a refused step, a verdict
		dropped with no anchor): still the cabin's, so no move out, no push, no damage, the entry kept (and, in a rider the
		deck carried, the miss logged) */
		vehicle_riding_contacts_clear();
		CHECK(vehicle_riding_contact_record(0, VEHICLE, &inward) && vehicle_riding_contact_defer(0, VEHICLE), "not recorded");
		base = point(1.35f, 0.f, WIDTH);
		entry = base;
		pushes = damages = logged = 0;
		settled = biped_riding_contacts(BIPED, &biped, &definition, TRUE, NULL, FALSE, &velocity, 0, &record, NONE, HEIGHT,
			WIDTH, &base);
		CHECK(settled, "a deferred touch not settled in the cabin");
		CHECK(base.x == entry.x && pushes == 0 && damages == 0 && logged == 0,
			"a deferred touch moved %g, pushed %d, hurt %d, logged %d", (double)base.x, pushes, damages, logged);
		CHECK(velocity.i == 0.f && velocity.j == 0.f && velocity.k == 0.f, "a deferred touch changed the velocity");
		/* (carried, the rider's place is its own to settle here: the miss is logged) */
		vehicle_riding_cabin_clear(&record);
		record.unit_index = BIPED;
		record.vehicle_index = VEHICLE;
		vehicle_riding_contacts_clear();
		CHECK(vehicle_riding_contact_record(0, VEHICLE, &inward) && vehicle_riding_contact_defer(0, VEHICLE), "not recorded");
		base = point(1.35f, 0.f, WIDTH);
		pushes = damages = logged = 0;
		settled = biped_riding_contacts(BIPED, &biped, &definition, TRUE, NULL, TRUE, &velocity, 0, &record, NONE, HEIGHT,
			WIDTH, &base);
		CHECK(settled && base.x == entry.x && pushes == 0 && damages == 0 && logged == 1,
			"a carried deferred touch moved %g, pushed %d, hurt %d, logged %d", (double)base.x, pushes, damages, logged);

		/* the same touch not deferred, and no cabin gate: the ordinary way, out along the touch (a crush stays its own) */
		vehicle_riding_contacts_clear();
		CHECK(vehicle_riding_contact_record(0, VEHICLE, &inward), "not recorded");
		base = point(1.35f, 0.f, WIDTH);
		pushes = damages = logged = 0;
		settled = biped_riding_contacts(BIPED, &biped, &definition, TRUE, NULL, FALSE, &velocity, 0, &record, NONE, HEIGHT,
			WIDTH, &base);
		CHECK(!settled && logged == 0, "an ordinary touch settled in a cabin");
		CHECK(base.x < 1.f && pushes == 0 && damages == 0, "an ordinary touch not moved out (%g)", (double)base.x);
		return 0;
	}

	CASE("window-jump-sample-moving")
	{
		struct biped_datum biped;
		struct biped_definition definition;
		struct biped_physics physics;
		struct vehicle_riding_cabin *record = vehicle_riding_player_cabin(3);

		/* a cabin tilted 70 degrees driving .7 wu a tick, its rider in the window at the base a tick behind its place
		on it, which is behind the rear wall: the jump's one sample is of the cabin it left, not of where the base
		happens to lie */
		tilted_window(.7f, -1.f, 70.f, &biped, &definition, &physics);
		(void)biped_window_tick(BIPED, &biped, &definition, &physics);
		CHECK(record->unit_index == BIPED && record->vehicle_index == VEHICLE, "the cabin not keyed to the rider");
		CHECK(record->jump == _vehicle_riding_enclosure_enclosed, "an enclosed cabin's jump sampled %d at the lagging base",
			record->jump);
		return 0;
	}
	CASE("window-no-early-return")
	{
		struct biped_datum biped;
		struct biped_definition definition;
		struct biped_physics physics;
		struct vehicle_riding_cabin *record = vehicle_riding_player_cabin(3);
		real_vector3d inward = { 1.f, 0.f, 0.f };
		real_vector3d velocity = { 0.f, 0.f, 0.f };
		real_point3d base;
		real_point3d entry;
		boolean carried;

		/* a touch the vehicle's update deferred, a rider through the hull: in the window its place is not settled before its
		physics moves it (its base is a tick of the cabin's travel behind it, and a return there would be added to that
		motion), carried it is */
		for (carried = FALSE; carried <= TRUE; carried++)
		{
			anchored_rider(carried ? RIDING_TICKS : RIDING_TICKS - 1, -1.7f, &biped, &definition, &physics);
			vehicle_riding_contacts_clear();
			CHECK(vehicle_riding_contact_record(3, VEHICLE, &inward) && vehicle_riding_contact_defer(3, VEHICLE), "not recorded");
			base = entry = physics.position;
			CHECK(biped_riding_contacts(BIPED, &biped, &definition, TRUE, NULL, carried, &velocity, 3, record, NONE, HEIGHT,
				WIDTH, &base), "a deferred touch not settled in the cabin");
			if (carried)
				CHECK(base.x > -1.f, "a carried rider through the hull not returned by its touch (%g)", (double)base.x);
			else
				CHECK(base.x == entry.x, "a rider in the window settled before its physics (%g)", (double)base.x);

			anchored_rider(carried ? RIDING_TICKS : RIDING_TICKS - 1, -1.7f, &biped, &definition, &physics);
			vehicle_riding_contacts_clear();
			biped_window_settle_tick(BIPED, &biped, &definition, &physics);
			if (carried)
				CHECK(physics.position.x > -1.f, "a carried rider through the hull not returned (%g)", (double)physics.position.x);
			else
				CHECK(physics.position.x == -1.7f, "a rider in the window watched before its physics (%g)",
					(double)physics.position.x);
		}
		return 0;
	}
	CASE("window-final-return")
	{
		struct biped_datum biped;
		struct biped_definition definition;
		struct biped_physics physics;

		/* after its own movement a rider in the window that went through the hull is returned */
		anchored_rider(RIDING_TICKS - 1, -1.7f, &biped, &definition, &physics);
		vehicle_riding_contacts_clear();
		CHECK(biped_cabin_refusal_tick(BIPED, &biped, &definition, &physics), "the window's step refused");
		CHECK(physics.new_position.x > -1.f, "a through-hull rider in the window left at %g", (double)physics.new_position.x);
		return 0;
	}

	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
