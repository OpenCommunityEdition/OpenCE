/*
ITEM_PHYSICS.C

Weapons and grenades lying about, moved by Jolt Physics (game.item_physics,
"enhanced"; the ragdolls' world, ragdoll_physics.h): an item the game sets moving
(dropped, thrown, pushed by a shot or a blast, item_accelerate) is a rigid
body, the convex hull of its model, that falls, tumbles, slides off ledges
and comes to lean against what it meets, until it sleeps; then it lies where
Jolt left it, at rest as the game's items do, until the game moves it again.
The game's pushes are its own: what it adds to the item's velocity is added
to the body's.

The item's position is the game's (picking it up, its network state): only
how it moves between the game's pushes is Jolt's. A client in another's game
moves its copies as the game does and takes the host's positions.
*/

#include "cseries.h"
#include "math/real_math.h"
#include "objects/objects.h"
#include "items/items.h"
#include "items/item_definitions.h"
#include "physics/collision_features.h"
#include "physics/collision_usage.h"
#include "physics/collisions.h"
#include "physics/physics.h"
#include "sound/game_sound.h"
#include "networking/network_game_globals.h"

#include "ragdoll.h"
#include "object_mesh.h"

#include <math.h>
#include <string.h>

#if RAGDOLLS

#include "ragdoll_physics.h"

/* port/linux/src/port_config.c */
char const *config_string(char const *name);
unsigned long config_changes(void);

/* ragdoll.c: the world's gravity, as a share of the game's */
real ragdoll_gravity_scale(void);

enum
{
	ITEM_PHYSICS_MAXIMUM_POINTS = 96,
	ITEM_PHYSICS_MAXIMUM_TRIANGLES = MAXIMUM_COLLISION_FEATURES_PER_TEST * (MAXIMUM_POINTS_PER_COLLISION_PRISM - 2),
	/* ticks asleep before it lies still for good */
	ITEM_PHYSICS_SLEEP_TICKS = 10,
	/* ticks it may move at most (then it lies where it is) */
	ITEM_PHYSICS_MAXIMUM_TICKS = 20 * 30,
};

/* the level gathered about an item: this far about its place, again when
it leaves it */
#define ITEM_PHYSICS_REACH 0.6f
/* a change in its speed this large (world units a tick) is a knock it
sounds */
#define ITEM_PHYSICS_KNOCK 0.02f

struct item_physics
{
	long object_index; /* NONE: free */
	long start_time;
	short asleep_ticks;
	boolean environment_valid;
	real_point3d environment_center;
	/* the velocity it was last given (world units, radians a tick): what
	the game adds to it since is the game's push */
	real_vector3d velocity;
	real_vector3d spin;
	/* where it put the item last (the game moving it since, picked up and
	dropped again, is a new start) */
	real_point3d written;
};

static struct item_physics item_physics_slots[RAGDOLL_PHYSICS_MAXIMUM_ITEMS];
static boolean item_physics_initialized = FALSE;
static struct collision_feature_list item_physics_features;
static real item_physics_triangles[ITEM_PHYSICS_MAXIMUM_TRIANGLES * 9];

static boolean item_physics_enabled(
	void)
{
	static unsigned long changes = (unsigned long)-1;
	static boolean enabled = FALSE;

	if (changes != config_changes())
	{
		char const *value = config_string("game.item_physics");

		changes = config_changes();
		enabled = value && (!strcmp(value, "enhanced") || !strcmp(value, "jolt"));
	}
	return enabled;
}

/* ---------- frames and rotations (x forward, y left, z up) */

static void item_physics_quaternion_from_frame(
	real_vector3d const *forward,
	real_vector3d const *up,
	float q[4])
{
	real_vector3d left;
	real m[3][3];
	real trace;

	cross_product3d(up, forward, &left);
	/* (columns forward, left, up) */
	m[0][0] = forward->i; m[0][1] = left.i; m[0][2] = up->i;
	m[1][0] = forward->j; m[1][1] = left.j; m[1][2] = up->j;
	m[2][0] = forward->k; m[2][1] = left.k; m[2][2] = up->k;
	trace = m[0][0] + m[1][1] + m[2][2];
	if (trace > 0.f)
	{
		real s = (real)sqrt(trace + 1.f) * 2.f;

		q[3] = 0.25f * s;
		q[0] = (m[2][1] - m[1][2]) / s;
		q[1] = (m[0][2] - m[2][0]) / s;
		q[2] = (m[1][0] - m[0][1]) / s;
	}
	else if (m[0][0] > m[1][1] && m[0][0] > m[2][2])
	{
		real s = (real)sqrt(1.f + m[0][0] - m[1][1] - m[2][2]) * 2.f;

		q[3] = (m[2][1] - m[1][2]) / s;
		q[0] = 0.25f * s;
		q[1] = (m[0][1] + m[1][0]) / s;
		q[2] = (m[0][2] + m[2][0]) / s;
	}
	else if (m[1][1] > m[2][2])
	{
		real s = (real)sqrt(1.f + m[1][1] - m[0][0] - m[2][2]) * 2.f;

		q[3] = (m[0][2] - m[2][0]) / s;
		q[0] = (m[0][1] + m[1][0]) / s;
		q[1] = 0.25f * s;
		q[2] = (m[1][2] + m[2][1]) / s;
	}
	else
	{
		real s = (real)sqrt(1.f + m[2][2] - m[0][0] - m[1][1]) * 2.f;

		q[3] = (m[1][0] - m[0][1]) / s;
		q[0] = (m[0][2] + m[2][0]) / s;
		q[1] = (m[1][2] + m[2][1]) / s;
		q[2] = 0.25f * s;
	}
}

static void item_physics_frame_from_quaternion(
	float const q[4],
	real_vector3d *forward,
	real_vector3d *up)
{
	real x = q[0], y = q[1], z = q[2], w = q[3];

	forward->i = 1.f - 2.f * (y * y + z * z);
	forward->j = 2.f * (x * y + w * z);
	forward->k = 2.f * (x * z - w * y);
	up->i = 2.f * (x * z + w * y);
	up->j = 2.f * (y * z - w * x);
	up->k = 1.f - 2.f * (x * x + y * y);
	normalize3d(forward);
	normalize3d(up);
}

/* ---------- the item's shape: its model's points (default pose) */

static short item_physics_points(
	long item_index,
	float *points)
{
	struct object_datum *object = object_get(item_index);
	struct object_mesh const *mesh = object_mesh_get(item_index);
	real scale = object->object.scale > 0.f ? object->object.scale : 1.f;
	short count = 0;

	if (mesh && mesh->vertex_count >= 4)
	{
		long stride = MAX(1, mesh->vertex_count / (ITEM_PHYSICS_MAXIMUM_POINTS - 6));
		long extremes[6];
		long index;
		short axis;

		/* (each axis's farthest points, so the hull reaches the item's ends) */
		for (axis = 0; axis < 6; axis++)
			extremes[axis] = 0;
		for (index = 0; index < mesh->vertex_count; index++)
		{
			real_point3d const *p = &mesh->vertices[index].position;

			if (p->x < mesh->vertices[extremes[0]].position.x) extremes[0] = index;
			if (p->x > mesh->vertices[extremes[1]].position.x) extremes[1] = index;
			if (p->y < mesh->vertices[extremes[2]].position.y) extremes[2] = index;
			if (p->y > mesh->vertices[extremes[3]].position.y) extremes[3] = index;
			if (p->z < mesh->vertices[extremes[4]].position.z) extremes[4] = index;
			if (p->z > mesh->vertices[extremes[5]].position.z) extremes[5] = index;
		}
		for (axis = 0; axis < 6; axis++)
		{
			real_point3d const *p = &mesh->vertices[extremes[axis]].position;

			points[count * 3 + 0] = p->x * scale;
			points[count * 3 + 1] = p->y * scale;
			points[count * 3 + 2] = p->z * scale;
			count++;
		}
		for (index = 0; index < mesh->vertex_count && count < ITEM_PHYSICS_MAXIMUM_POINTS; index += stride)
		{
			real_point3d const *p = &mesh->vertices[index].position;

			points[count * 3 + 0] = p->x * scale;
			points[count * 3 + 1] = p->y * scale;
			points[count * 3 + 2] = p->z * scale;
			count++;
		}
		return count;
	}
	/* (no model to read: a small box about its middle) */
	{
		real half = MAX(0.02f, 0.4f * object->object.bounding_sphere_radius);
		short corner;

		for (corner = 0; corner < 8; corner++)
		{
			points[corner * 3 + 0] = (corner & 1) ? half : -half;
			points[corner * 3 + 1] = (corner & 2) ? half * 0.4f : -half * 0.4f;
			points[corner * 3 + 2] = (corner & 4) ? half * 0.4f : -half * 0.4f;
		}
		return 8;
	}
}

/* ---------- the level about it */

static void item_physics_environment(
	struct item_physics *slot,
	short slot_index,
	long item_index)
{
	struct object_datum *object = object_get(item_index);
	long triangle_count = 0;
	short prism_index;

	if (slot->environment_valid &&
		distance3d(&slot->environment_center, &object->object.position) < 0.5f * ITEM_PHYSICS_REACH)
	{
		return;
	}
	collision_get_features_in_sphere(
		FLAG(_collision_test_structure_bit) | FLAG(_collision_test_objects_bit) |
			FLAG(_collision_test_objects_scenery_bit) | FLAG(_collision_test_objects_machines_bit) |
			FLAG(_collision_test_ignore_invisible_surfaces_bit),
		&object->object.position,
		ITEM_PHYSICS_REACH + object->object.bounding_sphere_radius,
		0.f,
		0.f,
		item_index,
		&item_physics_features);
	for (prism_index = 0; prism_index < item_physics_features.count[_collision_feature_prism]; prism_index++)
	{
		struct collision_prism const *prism = &item_physics_features.prisms[prism_index];
		real_point3d points[MAXIMUM_POINTS_PER_COLLISION_PRISM];
		long point_count = MIN(prism->point_count, MAXIMUM_POINTS_PER_COLLISION_PRISM);
		long point_index;

		if (point_count < 3 || !valid_real_plane3d(&prism->plane) ||
			prism->projection_axis < _x || prism->projection_axis > _z)
		{
			continue;
		}
		for (point_index = 0; point_index < point_count; point_index++)
		{
			project_point2d(&prism->points[point_index], &prism->plane, prism->projection_axis,
				prism->projection_sign, &points[point_index]);
		}
		for (point_index = 1; point_index + 1 < point_count && triangle_count < ITEM_PHYSICS_MAXIMUM_TRIANGLES;
			point_index++)
		{
			real_point3d const *a = &points[0];
			real_point3d const *b = &points[point_index];
			real_point3d const *c = &points[point_index + 1];
			real_vector3d ab;
			real_vector3d ac;
			real_vector3d normal;
			real *out = &item_physics_triangles[triangle_count * 9];

			vector_from_points3d(a, b, &ab);
			vector_from_points3d(a, c, &ac);
			cross_product3d(&ab, &ac, &normal);
			if (dot_product3d(&normal, &prism->plane.n) < 0.f)
			{
				real_point3d const *swap = b;

				b = c;
				c = swap;
			}
			out[0] = a->x; out[1] = a->y; out[2] = a->z;
			out[3] = b->x; out[4] = b->y; out[5] = b->z;
			out[6] = c->x; out[7] = c->y; out[8] = c->z;
			triangle_count++;
		}
	}
	ragdoll_physics_item_set_environment(slot_index, item_physics_triangles, (int)triangle_count);
	slot->environment_valid = TRUE;
	slot->environment_center = object->object.position;
}

/* ---------- slots */

static void item_physics_free(
	short slot_index)
{
	ragdoll_physics_item_destroy(slot_index);
	item_physics_slots[slot_index].object_index = NONE;
}

static short item_physics_find(
	long item_index)
{
	short index;

	if (!item_physics_initialized)
	{
		for (index = 0; index < RAGDOLL_PHYSICS_MAXIMUM_ITEMS; index++)
			item_physics_slots[index].object_index = NONE;
		item_physics_initialized = TRUE;
	}
	for (index = 0; index < RAGDOLL_PHYSICS_MAXIMUM_ITEMS; index++)
	{
		struct item_physics *slot = &item_physics_slots[index];

		if (slot->object_index == NONE)
			continue;
		/* (its item gone, or a body left from another map or game) */
		if (!object_try_and_get_and_verify_type(slot->object_index, _object_mask_item) ||
			!ragdoll_physics_item_exists(index) || game_time_get() < slot->start_time)
		{
			item_physics_free(index);
			continue;
		}
		if (slot->object_index == item_index)
			return index;
	}
	return NONE;
}

/* the item comes to rest where Jolt left it, as the game's items do */
static void item_physics_rest(
	short slot_index,
	long item_index)
{
	struct item_datum *item = item_get(item_index);

	item_physics_free(slot_index);
	item->object.translational_velocity = *global_zero_vector3d;
	item->object.angular_velocity = *global_zero_vector3d;
	SET_FLAG(item->item.flags, _item_has_nonzero_angular_velocity_bit, FALSE);
	SET_FLAG(item->item.flags, _item_on_structure_bit, FALSE);
	SET_FLAG(item->item.flags, _item_on_object_bit, FALSE);
	item->item.ignore_object_index = NONE;
	SET_FLAG(item->object.flags, _object_at_rest_bit, TRUE);
	/* (as the game's: a dropped item in the campaign is garbage, in time) */
	if (!game_engine_running() && item->object.owner_player_index == NONE)
		object_set_garbage(item_index, TRUE);
}

/* ---------- public code */

/* from item_update: whether Jolt moves the item this tick (and the game's
own movement is left out); the item free, connected to the map and one
Jolt may move (allowed: not one that keeps upright or floats) */
boolean item_physics_update(
	long item_index,
	boolean allowed)
{
	struct item_datum *item = item_get(item_index);
	struct item_physics *slot;
	short slot_index = item_physics_find(item_index);

	/* (moved by the game since: carried and dropped, thrown, put somewhere;
	its body begins again from where it is) */
	if (slot_index != NONE &&
		distance3d(&item_physics_slots[slot_index].written, &item->object.position) > 0.01f)
	{
		item_physics_free(slot_index);
		slot_index = NONE;
	}
	float position[3];
	float rotation[4];
	float velocity[3];
	float spin[3];
	boolean awake;

	if (!allowed || !item_physics_enabled() || network_game_distributed_client() ||
		TEST_FLAG(item->item.flags, _item_attached_to_unit_bit) ||
		!ragdoll_physics_initialize())
	{
		if (slot_index != NONE)
			item_physics_free(slot_index);
		return FALSE;
	}

	if (slot_index == NONE)
	{
		float points[ITEM_PHYSICS_MAXIMUM_POINTS * 3];
		short point_count;
		short oldest = NONE;
		short index;

		/* (lying still: the game's, until the game moves it) */
		if (TEST_FLAG(item->object.flags, _object_at_rest_bit))
			return FALSE;
		for (index = 0; index < RAGDOLL_PHYSICS_MAXIMUM_ITEMS && slot_index == NONE; index++)
		{
			if (item_physics_slots[index].object_index == NONE)
				slot_index = index;
			else if (oldest == NONE || item_physics_slots[index].start_time < item_physics_slots[oldest].start_time)
				oldest = index;
		}
		/* (none free: the one moving longest lies down) */
		if (slot_index == NONE)
		{
			if (oldest == NONE)
				return FALSE;
			item_physics_rest(oldest, item_physics_slots[oldest].object_index);
			slot_index = oldest;
		}
		slot = &item_physics_slots[slot_index];
		point_count = item_physics_points(item_index, points);
		item_physics_quaternion_from_frame(&item->object.forward, &item->object.up, rotation);
		position[0] = item->object.position.x;
		position[1] = item->object.position.y;
		position[2] = item->object.position.z;
		velocity[0] = item->object.translational_velocity.i;
		velocity[1] = item->object.translational_velocity.j;
		velocity[2] = item->object.translational_velocity.k;
		spin[0] = item->object.angular_velocity.i;
		spin[1] = item->object.angular_velocity.j;
		spin[2] = item->object.angular_velocity.k;
		slot->environment_valid = FALSE;
		item_physics_environment(slot, slot_index, item_index);
		if (!ragdoll_physics_item_create(slot_index, points, point_count, position, rotation, velocity, spin,
			1.f / MAX(ragdoll_gravity_scale(), 0.1f)))
		{
			ragdoll_physics_item_destroy(slot_index);
			return FALSE;
		}
		slot->object_index = item_index;
		slot->start_time = game_time_get();
		slot->asleep_ticks = 0;
		slot->velocity = item->object.translational_velocity;
		slot->spin = item->object.angular_velocity;
		slot->written = item->object.position;
		SET_FLAG(item->item.flags, _item_on_structure_bit, FALSE);
		SET_FLAG(item->item.flags, _item_on_object_bit, FALSE);
		object_set_garbage(item_index, FALSE);
		/* (anyone but the one who dropped it may take it at once, in the air
		too; the one who dropped it once it lands, or a moment after: else
		its swap, the button still held, swapped back) */
	}
	else
	{
		real_vector3d push;
		real_vector3d turn;

		slot = &item_physics_slots[slot_index];
		/* (the game's push since: a shot, a blast, item_accelerate) */
		vector_from_points3d((real_point3d const *)&slot->velocity,
			(real_point3d const *)&item->object.translational_velocity, &push);
		vector_from_points3d((real_point3d const *)&slot->spin,
			(real_point3d const *)&item->object.angular_velocity, &turn);
		if (magnitude_squared3d(&push) > 1e-8f || magnitude_squared3d(&turn) > 1e-8f)
		{
			velocity[0] = push.i; velocity[1] = push.j; velocity[2] = push.k;
			spin[0] = turn.i; spin[1] = turn.j; spin[2] = turn.k;
			ragdoll_physics_item_add_velocity(slot_index, velocity, spin);
			slot->asleep_ticks = 0;
		}
		item_physics_environment(slot, slot_index, item_index);
	}

	/* (the world was stepped this tick, with the bodies: ragdolls_step) */
	awake = ragdoll_physics_item_get(slot_index, position, rotation, velocity, spin);
	{
		real_point3d where;
		real_vector3d forward;
		real_vector3d up;
		real_vector3d moving;

		where.x = position[0];
		where.y = position[1];
		where.z = position[2];
		moving.i = velocity[0];
		moving.j = velocity[1];
		moving.k = velocity[2];
		item_physics_frame_from_quaternion(rotation, &forward, &up);
		if (!valid_real_point3d(&where) || !valid_real_vector3d(&forward) || !valid_real_vector3d(&up) ||
			!valid_real_vector3d(&moving))
		{
			item_physics_rest(slot_index, item_index);
			return TRUE;
		}
		/* (a knock, slowed by meeting something: its sound, as the game's
		items make) */
		{
			real_vector3d change;
			struct item_definition const *definition = item_definition_get(item->definition_index);

			boolean knocked;

			vector_from_points3d((real_point3d const *)&slot->velocity, (real_point3d const *)&moving, &change);
			knocked = magnitude3d(&change) > ITEM_PHYSICS_KNOCK &&
				magnitude3d(&moving) < magnitude3d(&slot->velocity) - 0.5f * ITEM_PHYSICS_KNOCK;
			/* (landed, or a moment after it was dropped: the one who dropped
			it may take it again, as the game's items are once they land) */
			if (knocked || game_time_get() - slot->start_time > 15)
				item->item.ignore_object_index = NONE;
			if (knocked && definition->item.collision_sound.index != NONE)
			{
				struct sound_location location;

				location.position = where;
				location.forward = *global_up3d;
				location.translational_velocity = *global_zero_vector3d;
				location.game_location = item->object.location;
				unattached_impulse_sound_new(definition->item.collision_sound.index, &location,
					PIN(magnitude3d(&change) / MAX(0.1f, 0.01f), 0.f, 1.f));
			}
		}
		object_set_position(item_index, &where, &forward, &up);
		slot->written = item->object.position;
		item->object.translational_velocity = moving;
		item->object.angular_velocity.i = spin[0];
		item->object.angular_velocity.j = spin[1];
		item->object.angular_velocity.k = spin[2];
		slot->velocity = item->object.translational_velocity;
		slot->spin = item->object.angular_velocity;
	}
	slot->asleep_ticks = awake ? 0 : (short)(slot->asleep_ticks + 1);
	if (slot->asleep_ticks >= ITEM_PHYSICS_SLEEP_TICKS ||
		game_time_get() - slot->start_time > ITEM_PHYSICS_MAXIMUM_TICKS)
	{
		item_physics_rest(slot_index, item_index);
	}
	return TRUE;
}

/* a new map: every item's body forgotten (ragdolls_initialize_for_new_map
resets the world) */
void item_physics_reset(
	void)
{
	short index;

	for (index = 0; index < RAGDOLL_PHYSICS_MAXIMUM_ITEMS; index++)
	{
		if (item_physics_initialized && item_physics_slots[index].object_index != NONE)
			ragdoll_physics_item_destroy(index);
		item_physics_slots[index].object_index = NONE;
	}
	item_physics_initialized = TRUE;
}

#else

boolean item_physics_update(
	long item_index,
	boolean allowed)
{
	(void)item_index;
	(void)allowed;
	return FALSE;
}

void item_physics_reset(
	void)
{
}

#endif
