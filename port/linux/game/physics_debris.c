/*
PHYSICS_DEBRIS.C

Debris as rigid bodies in the host's Jolt world (physics_debris.h). Each
piece takes a slot, and slot n is the host's body PHYSICS_DEBRIS_FIRST_ID + n.
A slot is new until physics_jolt.c adds its body, then live; the host's poses
come back a tick at a time, and a particle is moved between the last two of
them as its frames are drawn. A piece is known by its full handle, so a
particle or object made again under the same absolute index is another
piece.
*/

#include "cseries/cseries.h"
#include "math/real_math.h"
#include "memory/data.h"
#include "effects/particles.h"
#include "physics/point_physics.h"
#include "objects/object_types.h"
#include "objects/objects.h"
#include "scenario/scenario.h"
#include "physics_jolt.h"
#include "physics_debris.h"
#include "ragdoll_bodies.h"
#include "ragdolls.h"

#include <string.h>

#if defined(HALO_GUEST_VM) && defined(HALO_METAL)
#include "host_physics.h"
#else
#define HOST_PHYSICS_SHAPE_CAPSULE 1
#define HOST_PHYSICS_SHAPE_BOX 2
#endif

/* port/linux/src/port_config.c */
long config_integer(const char *name);
unsigned long config_changes(void);
struct real_matrix4x3 *render_interpolation_object_node_matrices(long object_index);

/* the pieces of garbage drawn from their bodies at once, and the most nodes
such a piece has */
#define PHYSICS_DEBRIS_GARBAGE_MAX 64
#define PHYSICS_DEBRIS_NODES_MAX 16
/* the ticks a new body may go unposed before the host is taken to have
refused it */
#define PHYSICS_DEBRIS_UNPOSED_TICKS 2
/* lighter than this (grams a millilitre) is smoke, a spark or a flash */
#define PHYSICS_DEBRIS_DENSITY_MIN 0.05f
/* a garbage piece's material */
#define PHYSICS_DEBRIS_GARBAGE_FRICTION 0.6f
#define PHYSICS_DEBRIS_GARBAGE_RESTITUTION 0.2f
/* 10 feet (host_physics.h's) */
#define PHYSICS_DEBRIS_METRES_PER_UNIT 3.048f
/* the fastest a body is sent (units a second; the host takes 300) */
#define PHYSICS_DEBRIS_SPEED_MAX 250.0f
/* a body slower than the game's particles at rest (units a second,
particle_update_physics') for half a second, Jolt's time before sleep, is at
rest: a casing is smaller than Jolt's contacts are exact, and creeps */
#define PHYSICS_DEBRIS_REST_SPEED 0.25f
#define PHYSICS_DEBRIS_REST_TICKS 15

enum
{
	_physics_debris_free = 0,
	_physics_debris_new,
	_physics_debris_live,
};

enum
{
	_physics_debris_particle = 0,
	_physics_debris_garbage,
};

struct physics_debris
{
	long datum_index;
	unsigned long serial;
	byte state;
	byte kind;
	/* the host still has the body of the piece this slot had */
	boolean remove_first;
	boolean posed;
	boolean awake;
	short ticks_unposed;
	short ticks_slow;
	/* the garbage's drawn matrices */
	short drawn_index;
	real elapsed;
	struct ragdoll_pose previous;
	struct ragdoll_pose current;
	/* the garbage's body's centre in its node 0's frame */
	real_point3d center;
	struct physics_debris_body body;
};

static struct
{
	struct physics_debris debris[PHYSICS_DEBRIS_MAX];
	/* each particle's and object's slot + 1, by absolute index */
	short particle_slots[HALO_PORT_MAXIMUM_PARTICLES];
	short object_slots[HALO_PORT_MAXIMUM_OBJECTS_PER_MAP];
	/* the handle handed back last at each absolute index */
	long particles_handed_back[HALO_PORT_MAXIMUM_PARTICLES];
	long objects_handed_back[HALO_PORT_MAXIMUM_OBJECTS_PER_MAP];
	unsigned long serial;
	unsigned long changes;
	long maximum;
	boolean drawn_used[PHYSICS_DEBRIS_GARBAGE_MAX];
	real_matrix4x3 drawn[PHYSICS_DEBRIS_GARBAGE_MAX][PHYSICS_DEBRIS_NODES_MAX];
} physics_debris;

static long physics_debris_maximum(void)
{
	if (physics_debris.changes != config_changes() || physics_debris.maximum <= 0)
	{
		physics_debris.changes = config_changes();
		physics_debris.maximum = PIN(config_integer("game.physics_debris_max"), 1, PHYSICS_DEBRIS_MAX);
	}
	return physics_debris.maximum;
}

static short *physics_debris_table(
	byte kind,
	long datum_index)
{
	long absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(datum_index);

	if (kind == _physics_debris_particle)
	{
		return absolute_index >= 0 && absolute_index < HALO_PORT_MAXIMUM_PARTICLES ?
			&physics_debris.particle_slots[absolute_index] : NULL;
	}
	return absolute_index >= 0 && absolute_index < HALO_PORT_MAXIMUM_OBJECTS_PER_MAP ?
		&physics_debris.object_slots[absolute_index] : NULL;
}

static long *physics_debris_handed_back(
	byte kind,
	long datum_index)
{
	long absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(datum_index);

	if (kind == _physics_debris_particle)
	{
		return absolute_index >= 0 && absolute_index < HALO_PORT_MAXIMUM_PARTICLES ?
			&physics_debris.particles_handed_back[absolute_index] : NULL;
	}
	return absolute_index >= 0 && absolute_index < HALO_PORT_MAXIMUM_OBJECTS_PER_MAP ?
		&physics_debris.objects_handed_back[absolute_index] : NULL;
}

/* a piece's slot, or NULL */
static struct physics_debris *physics_debris_of(
	byte kind,
	long datum_index)
{
	short *table = physics_debris_table(kind, datum_index);
	struct physics_debris *debris;

	if (!table || *table <= 0 || *table > PHYSICS_DEBRIS_MAX)
		return NULL;
	debris = &physics_debris.debris[*table - 1];
	if (debris->state == _physics_debris_free || debris->kind != kind || debris->datum_index != datum_index)
		return NULL;
	return debris;
}

static boolean physics_debris_was_handed_back(
	byte kind,
	long datum_index)
{
	long *handed_back = physics_debris_handed_back(kind, datum_index);

	return !handed_back || *handed_back == datum_index;
}

/* a slot's piece forgotten; `hand_back` keeps it from being a body again */
static void physics_debris_free(
	struct physics_debris *debris,
	boolean hand_back)
{
	short *table = physics_debris_table(debris->kind, debris->datum_index);

	if (table && *table == (short)(debris - physics_debris.debris) + 1)
		*table = 0;
	if (hand_back)
	{
		long *handed_back = physics_debris_handed_back(debris->kind, debris->datum_index);

		if (handed_back)
			*handed_back = debris->datum_index;
	}
	if (debris->kind == _physics_debris_garbage && debris->drawn_index >= 0 &&
		debris->drawn_index < PHYSICS_DEBRIS_GARBAGE_MAX)
	{
		physics_debris.drawn_used[debris->drawn_index] = FALSE;
	}
	debris->drawn_index = NONE;
	debris->state = _physics_debris_free;
	debris->posed = FALSE;
}

/* a slot for a new piece: a free one, else the oldest's, its piece handed
back (and its body removed before the new one is added) */
static struct physics_debris *physics_debris_take(
	byte kind,
	long datum_index)
{
	short *table = physics_debris_table(kind, datum_index);
	struct physics_debris *debris = NULL;
	struct physics_debris *oldest = NULL;
	long maximum = physics_debris_maximum();
	long index;

	if (!table)
		return NULL;
	for (index = 0; index < maximum && !debris; index++)
	{
		struct physics_debris *candidate = &physics_debris.debris[index];

		if (candidate->state == _physics_debris_free && !candidate->remove_first)
			debris = candidate;
		else if (candidate->state != _physics_debris_free &&
			(!oldest || candidate->serial - oldest->serial > 0x80000000u))
		{
			oldest = candidate;
		}
	}
	if (!debris)
	{
		if (!oldest)
			return NULL;
		debris = oldest;
		if (debris->state == _physics_debris_live)
			debris->remove_first = TRUE;
		physics_debris_free(debris, TRUE);
	}
	debris->datum_index = datum_index;
	debris->serial = ++physics_debris.serial;
	debris->kind = kind;
	debris->state = _physics_debris_new;
	debris->posed = FALSE;
	debris->awake = TRUE;
	debris->ticks_unposed = 0;
	debris->ticks_slow = 0;
	debris->drawn_index = NONE;
	debris->elapsed = 0.0f;
	*table = (short)(debris - physics_debris.debris) + 1;
	return debris;
}

/* ---------- particles */

boolean physics_debris_particle_body(
	real radius,
	struct point_physics_definition const *physics,
	real_point3d const *position,
	real_vector3d const *velocity,
	struct physics_debris_body *body)
{
	real_matrix4x3 frame;
	real capsule_radius;
	real half_length;
	real a;
	real h;
	real speed;
	real spin;

	if (!physics || !(physics->density >= PHYSICS_DEBRIS_DENSITY_MIN) || !(radius > 0.0f))
		return FALSE;
	/* a capsule along the way it flies, as long as the particle is wide */
	capsule_radius = PIN(0.35f * radius, 0.003f, 10.0f);
	half_length = PIN(0.65f * radius, 0.003f, 10.0f);
	a = capsule_radius * PHYSICS_DEBRIS_METRES_PER_UNIT;
	h = half_length * PHYSICS_DEBRIS_METRES_PER_UNIT;
	body->shape = HOST_PHYSICS_SHAPE_CAPSULE;
	body->size[0] = capsule_radius;
	body->size[1] = half_length;
	body->size[2] = capsule_radius;
	/* (the density is in grams a millilitre) */
	body->mass = PIN(physics->density * 1000.0f *
		(_pi * a * a * 2.0f * h + 4.0f / 3.0f * _pi * a * a * a), 0.01f, 50.0f);
	body->friction = PIN(physics->contact_friction, 0.0f, 1.0f);
	body->restitution = PIN(physics->elasticity, 0.0f, 1.0f);

	speed = square_root(velocity->i * velocity->i + velocity->j * velocity->j + velocity->k * velocity->k);
	memset(&frame, 0, sizeof(frame));
	frame.scale = 1.0f;
	if (speed > _real_epsilon)
	{
		frame.forward.i = velocity->i / speed;
		frame.forward.j = velocity->j / speed;
		frame.forward.k = velocity->k / speed;
	}
	else
	{
		frame.forward.i = 1.0f;
	}
	/* left: across the way it flies and up (or y, flying straight up or
	down) */
	if (frame.forward.k > 0.95f || frame.forward.k < -0.95f)
	{
		frame.left.i = frame.forward.k;
		frame.left.k = -frame.forward.i;
	}
	else
	{
		frame.left.i = -frame.forward.j;
		frame.left.j = frame.forward.i;
	}
	normalize3d(&frame.left);
	cross_product3d(&frame.forward, &frame.left, &frame.up);
	frame.position = *position;
	ragdoll_pose_from_matrix(&frame, &body->pose);

	body->velocity = *velocity;
	if (speed > PHYSICS_DEBRIS_SPEED_MAX)
	{
		body->velocity.i *= PHYSICS_DEBRIS_SPEED_MAX / speed;
		body->velocity.j *= PHYSICS_DEBRIS_SPEED_MAX / speed;
		body->velocity.k *= PHYSICS_DEBRIS_SPEED_MAX / speed;
	}
	/* tumbling end over end, as fast as it would roll */
	spin = PIN(speed / (4.0f * radius), 0.0f, 50.0f);
	body->angular_velocity.i = frame.left.i * spin;
	body->angular_velocity.j = frame.left.j * spin;
	body->angular_velocity.k = frame.left.k * spin;
	return TRUE;
}

boolean physics_debris_particle_is_body(
	long particle_index)
{
	return physics_debris_of(_physics_debris_particle, particle_index) != NULL;
}

void physics_debris_particle_collided(
	long particle_index)
{
	struct particle_datum *particle;
	struct particle_definition *definition;
	struct physics_debris_body body;
	struct physics_debris *debris;

	if (!physics_jolt_debris_enabled() ||
		physics_debris_of(_physics_debris_particle, particle_index) ||
		physics_debris_was_handed_back(_physics_debris_particle, particle_index))
	{
		return;
	}
	particle = (struct particle_datum *)datum_try_and_get(particle_data, particle_index);
	if (!particle)
		return;
	definition = particle_definition_get(particle->definition_index);
	if (definition->physics.index == NONE ||
		!physics_debris_particle_body(particle_get_radius(particle_index),
			point_physics_definition_get(definition->physics.index),
			&particle->position, &particle->translational_velocity, &body))
	{
		return;
	}
	debris = physics_debris_take(_physics_debris_particle, particle_index);
	if (!debris)
		return;
	debris->body = body;
	debris->previous = debris->current = body.pose;
}

boolean physics_debris_particle_update(
	long particle_index,
	real dt,
	boolean *settled)
{
	struct physics_debris *debris = physics_debris_of(_physics_debris_particle, particle_index);
	struct particle_datum *particle;
	real_point3d position;
	struct location location;
	real t;

	if (!debris)
		return FALSE;
	/* (held where it struck until the host has it) */
	if (!debris->posed)
		return TRUE;
	particle = particle_get(particle_index);
	debris->elapsed += dt;
	t = PIN(debris->elapsed * TICKS_PER_SECOND, 0.0f, 1.0f);
	position.x = debris->previous.position.x + t * (debris->current.position.x - debris->previous.position.x);
	position.y = debris->previous.position.y + t * (debris->current.position.y - debris->previous.position.y);
	position.z = debris->previous.position.z + t * (debris->current.position.z - debris->previous.position.z);
	particle->translational_velocity.i = (debris->current.position.x - debris->previous.position.x) * TICKS_PER_SECOND;
	particle->translational_velocity.j = (debris->current.position.y - debris->previous.position.y) * TICKS_PER_SECOND;
	particle->translational_velocity.k = (debris->current.position.z - debris->previous.position.z) * TICKS_PER_SECOND;
	scenario_location_from_point(&location, &position);
	if (location.cluster_index != NONE)
	{
		particle->location = location;
		particle->position = position;
	}
	*settled = !debris->awake || debris->ticks_slow >= PHYSICS_DEBRIS_REST_TICKS;
	if (*settled)
	{
		real_matrix4x3 frame;
		real_quaternion rotation;

		/* (lying along its body) */
		rotation.v.i = -debris->current.rotation.v.i;
		rotation.v.j = -debris->current.rotation.v.j;
		rotation.v.k = -debris->current.rotation.v.k;
		rotation.w = debris->current.rotation.w;
		matrix4x3_rotation_from_quaternion(&frame, &rotation);
		particle->direction = frame.forward;
		particle->translational_velocity.i = 0.0f;
		particle->translational_velocity.j = 0.0f;
		particle->translational_velocity.k = 0.0f;
	}
	return TRUE;
}

/* ---------- garbage */

void physics_debris_garbage_update(void)
{
	struct object_iterator iterator;
	struct object_datum *object;

	if (!physics_jolt_debris_enabled())
		return;
	object_iterator_new(&iterator, _object_mask_garbage, 0);
	while ((object = (struct object_datum *)object_iterator_next(&iterator)) != NULL)
	{
		real_matrix4x3 const *nodes;
		real_matrix4x3 inverse;
		real_point3d center;
		real_vector3d half_extents;
		real_matrix4x3 frame;
		struct physics_debris *debris;
		long node_count;
		short drawn_index;
		real volume;

		if (object->object.parent_object_index != NONE ||
			physics_debris_of(_physics_debris_garbage, iterator.index) ||
			physics_debris_was_handed_back(_physics_debris_garbage, iterator.index))
		{
			continue;
		}
		node_count = object->object.node_matrices.size / (long)sizeof(real_matrix4x3);
		if (node_count <= 0 || node_count > PHYSICS_DEBRIS_NODES_MAX)
			continue;
		for (drawn_index = 0; drawn_index < PHYSICS_DEBRIS_GARBAGE_MAX && physics_debris.drawn_used[drawn_index];
			drawn_index++)
		{
		}
		if (drawn_index == PHYSICS_DEBRIS_GARBAGE_MAX)
			return;
		nodes = object_get_node_matrices(iterator.index);
		/* its collision model's box, else a box in its bounding sphere */
		if (!ragdoll_hull_get(object->definition_index, &center, &half_extents))
		{
			real half = 0.45f * object->object.bounding_sphere_radius;

			matrix4x3_inverse(&nodes[0], &inverse);
			matrix4x3_transform_point(&inverse, &object->object.bounding_sphere_center, &center);
			half_extents.i = half_extents.j = half_extents.k = half;
		}
		half_extents.i = PIN(half_extents.i * nodes[0].scale, 0.005f, 10.0f);
		half_extents.j = PIN(half_extents.j * nodes[0].scale, 0.005f, 10.0f);
		half_extents.k = PIN(half_extents.k * nodes[0].scale, 0.005f, 10.0f);
		debris = physics_debris_take(_physics_debris_garbage, iterator.index);
		if (!debris)
			return;
		physics_debris.drawn_used[drawn_index] = TRUE;
		debris->drawn_index = drawn_index;
		debris->center = center;
		debris->body.shape = HOST_PHYSICS_SHAPE_BOX;
		debris->body.size[0] = half_extents.i;
		debris->body.size[1] = half_extents.j;
		debris->body.size[2] = half_extents.k;
		volume = 8.0f * half_extents.i * half_extents.j * half_extents.k *
			PHYSICS_DEBRIS_METRES_PER_UNIT * PHYSICS_DEBRIS_METRES_PER_UNIT *
			PHYSICS_DEBRIS_METRES_PER_UNIT;
		debris->body.mass = PIN(volume * 1000.0f, 0.1f, 100.0f);
		debris->body.friction = PHYSICS_DEBRIS_GARBAGE_FRICTION;
		debris->body.restitution = PHYSICS_DEBRIS_GARBAGE_RESTITUTION;
		frame = nodes[0];
		matrix4x3_transform_point(&nodes[0], &center, &frame.position);
		ragdoll_pose_from_matrix(&frame, &debris->body.pose);
		/* (the game's velocities are a tick's) */
		debris->body.velocity.i = object->object.translational_velocity.i * TICKS_PER_SECOND;
		debris->body.velocity.j = object->object.translational_velocity.j * TICKS_PER_SECOND;
		debris->body.velocity.k = object->object.translational_velocity.k * TICKS_PER_SECOND;
		debris->body.angular_velocity.i = object->object.angular_velocity.i * TICKS_PER_SECOND;
		debris->body.angular_velocity.j = object->object.angular_velocity.j * TICKS_PER_SECOND;
		debris->body.angular_velocity.k = object->object.angular_velocity.k * TICKS_PER_SECOND;
		debris->previous = debris->current = debris->body.pose;
	}
}

real_matrix4x3 const *physics_debris_node_matrices(
	long object_index,
	real_matrix4x3 const *nodes)
{
	struct physics_debris *debris;
	struct object_datum const *object;
	real_matrix4x3 *drawn;
	real_matrix4x3 body;
	real_matrix4x3 inverse;
	real_matrix4x3 carry;
	real_quaternion rotation;
	real_vector3d offset;
	long node_count;
	long node;

	/* (a frame between ticks is drawn from snapshots this has given already,
	render_interpolation.c) */
	if (!nodes || object_index == NONE)
		return nodes;
	debris = physics_debris_of(_physics_debris_garbage, object_index);
	if (!debris || !debris->posed || debris->drawn_index < 0 || debris->drawn_index >= PHYSICS_DEBRIS_GARBAGE_MAX ||
		render_interpolation_object_node_matrices(object_index))
	{
		return nodes;
	}
	object = (struct object_datum const *)object_try_and_get_and_verify_type(object_index, _object_mask_garbage);
	if (!object)
		return nodes;
	node_count = object->object.node_matrices.size / (long)sizeof(real_matrix4x3);
	if (node_count <= 0 || node_count > PHYSICS_DEBRIS_NODES_MAX)
		return nodes;
	drawn = physics_debris.drawn[debris->drawn_index];
	/* node 0 where the body has it, the body's centre carried back to the
	node's origin, at the scale the game has it */
	rotation.v.i = -debris->current.rotation.v.i;
	rotation.v.j = -debris->current.rotation.v.j;
	rotation.v.k = -debris->current.rotation.v.k;
	rotation.w = debris->current.rotation.w;
	matrix4x3_rotation_from_quaternion(&body, &rotation);
	body.scale = nodes[0].scale;
	offset.i = body.scale * (body.forward.i * debris->center.x + body.left.i * debris->center.y + body.up.i * debris->center.z);
	offset.j = body.scale * (body.forward.j * debris->center.x + body.left.j * debris->center.y + body.up.j * debris->center.z);
	offset.k = body.scale * (body.forward.k * debris->center.x + body.left.k * debris->center.y + body.up.k * debris->center.z);
	body.position.x = debris->current.position.x - offset.i;
	body.position.y = debris->current.position.y - offset.j;
	body.position.z = debris->current.position.z - offset.k;
	/* every node carried as node 0 is */
	matrix4x3_inverse(&nodes[0], &inverse);
	matrix4x3_multiply(&body, &inverse, &carry);
	for (node = 0; node < node_count; node++)
		matrix4x3_multiply(&carry, &nodes[node], &drawn[node]);
	return drawn;
}

/* ---------- the host's bodies */

static struct physics_debris *physics_debris_of_id(
	long id)
{
	if (id < PHYSICS_DEBRIS_FIRST_ID || id >= PHYSICS_DEBRIS_FIRST_ID + PHYSICS_DEBRIS_MAX)
		return NULL;
	return &physics_debris.debris[id - PHYSICS_DEBRIS_FIRST_ID];
}

/* whether a slot's piece is gone, or (a particle) lies at rest */
static boolean physics_debris_piece_gone(
	struct physics_debris const *debris)
{
	if (debris->kind == _physics_debris_garbage)
		return !object_try_and_get_and_verify_type(debris->datum_index, _object_mask_garbage);
	{
		struct particle_datum const *particle =
			(struct particle_datum const *)datum_try_and_get(particle_data, debris->datum_index);

		return !particle || TEST_FLAG(particle->flags, _particle_datum_at_rest_bit);
	}
}

boolean physics_debris_remove(
	long id)
{
	struct physics_debris *debris = physics_debris_of_id(id);

	if (!debris)
		return FALSE;
	if (debris->remove_first)
	{
		debris->remove_first = FALSE;
		return TRUE;
	}
	if (debris->state != _physics_debris_live)
	{
		/* (a new piece gone before its body was added) */
		if (debris->state == _physics_debris_new && physics_debris_piece_gone(debris))
			physics_debris_free(debris, FALSE);
		return FALSE;
	}
	if (physics_debris_piece_gone(debris))
	{
		physics_debris_free(debris, FALSE);
		return TRUE;
	}
	if (!debris->posed && ++debris->ticks_unposed > PHYSICS_DEBRIS_UNPOSED_TICKS)
	{
		/* (the host refused it: the game has it again) */
		physics_debris_free(debris, TRUE);
		return TRUE;
	}
	return FALSE;
}

struct physics_debris_body const *physics_debris_add(
	long id)
{
	struct physics_debris *debris = physics_debris_of_id(id);

	if (!debris || debris->state != _physics_debris_new || debris->remove_first)
		return NULL;
	debris->state = _physics_debris_live;
	debris->ticks_unposed = 0;
	return &debris->body;
}

void physics_debris_result(
	long id,
	boolean awake,
	struct ragdoll_pose const *pose)
{
	struct physics_debris *debris = physics_debris_of_id(id);

	if (!debris || debris->state != _physics_debris_live)
		return;
	debris->previous = debris->posed ? debris->current : debris->previous;
	debris->current = *pose;
	{
		real dx = (debris->current.position.x - debris->previous.position.x) * TICKS_PER_SECOND;
		real dy = (debris->current.position.y - debris->previous.position.y) * TICKS_PER_SECOND;
		real dz = (debris->current.position.z - debris->previous.position.z) * TICKS_PER_SECOND;

		if (!debris->posed || dx * dx + dy * dy + dz * dz >= PHYSICS_DEBRIS_REST_SPEED * PHYSICS_DEBRIS_REST_SPEED)
			debris->ticks_slow = 0;
		else if (debris->ticks_slow < PHYSICS_DEBRIS_REST_TICKS)
			debris->ticks_slow++;
	}
	debris->elapsed = 0.0f;
	debris->awake = awake;
	debris->posed = TRUE;
}

boolean physics_debris_posed_body(
	long id,
	struct physics_debris_body *body,
	boolean *awake)
{
	struct physics_debris const *debris = physics_debris_of_id(id);

	if (!debris || debris->state != _physics_debris_live || !debris->posed)
		return FALSE;
	*body = debris->body;
	body->pose = debris->current;
	*awake = debris->awake;
	return TRUE;
}

void physics_debris_dispose_from_old_map(void)
{
	memset(&physics_debris, 0, sizeof(physics_debris));
}
