#include "harness.h"
#include <math.h>
#include <string.h>

/* Debris as rigid bodies (physics_debris.c), in a fake world of eight
particles and eight objects, each known by a handle whose salt is above its
absolute index, as the game's data arrays have them. The particles' point
physics are the maps' own: bloodgulch's casings (effects\point physics\stones),
the Warthog's (old tags\stones) and the frag grenade's smoke, with one made
up past the limits to be clamped. A particle that strikes the level becomes a
capsule; a piece of garbage a box; past the cap the oldest is handed back. */
typedef struct { real scale; union { real n[4][3]; struct { real_vector3d forward, left, up; real_point3d position; }; }; }
	real_matrix4x3;
typedef struct { real_vector3d v; real w; } real_quaternion;
#define _real_epsilon 0.0001f
#define _pi 3.14159265359f
#define CEILING(n, ceiling) ((n) > (ceiling) ? (ceiling) : (n))
#define PIN(n, floor, ceiling) ((n) < (floor) ? (floor) : CEILING((n), (ceiling)))
#define TICKS_PER_SECOND 30
#define HALO_PORT_MAXIMUM_PARTICLES 8
#define HALO_PORT_MAXIMUM_OBJECTS_PER_MAP 8
#define HOST_PHYSICS_SHAPE_CAPSULE 1
#define HOST_PHYSICS_SHAPE_BOX 2
enum { _particle_datum_at_rest_bit = 2 };
enum { _object_type_garbage = 12 };
enum { _object_mask_garbage = 1 << _object_type_garbage };
struct location { long leaf_index; short cluster_index; };
struct point_physics_definition
{
	unsigned long flags;
	real density, air_friction, water_friction, contact_friction, elasticity;
};
struct particle_definition { struct { long index; } physics; };
struct particle_datum
{
	long definition_index; unsigned long flags; struct location location;
	real_point3d position; real_vector3d translational_velocity; real_vector3d direction; real radius;
};
struct object_header_block_reference { unsigned short size, offset; };
struct _object_datum
{
	long parent_object_index; struct object_header_block_reference node_matrices;
	real_point3d bounding_sphere_center; real bounding_sphere_radius;
	real_vector3d translational_velocity, angular_velocity;
};
struct object_datum { long definition_index; struct _object_datum object; };
struct object_iterator { long index; long next; };

static real square_root(real x) { return sqrtf(x); }
static real normalize3d(real_vector3d *v)
{
	real length = sqrtf(v->i * v->i + v->j * v->j + v->k * v->k);

	if (length > 0.0f) { v->i /= length; v->j /= length; v->k /= length; }
	return length;
}
static real_vector3d *cross_product3d(real_vector3d const *a, real_vector3d const *b, real_vector3d *out)
{
	out->i = a->j * b->k - a->k * b->j; out->j = a->k * b->i - a->i * b->k; out->k = a->i * b->j - a->j * b->i;
	return out;
}
static short const matrix4x3_next[3] = {1, 2, 0};

#include "config.inc"

/* the settings */
static boolean enabled = TRUE;
static long cap = PHYSICS_DEBRIS_MAX;
static unsigned long changes = 1;
static boolean physics_jolt_debris_enabled(void) { return enabled; }
static long config_integer(char const *name) { return strcmp(name, "game.physics_debris_max") == 0 ? cap : 0; }
static unsigned long config_changes(void) { return changes; }

/* the point physics: 0 bloodgulch's casings, 1 the Warthog's, 2 to 4 the
smoke's lightest, the grenade's and heaviest, 5 made up past the limits */
static struct point_physics_definition point_physics[6] = {
	{3, 2.4f, 900.0f, 14000.0f, 0.1f, 0.35f},
	{3, 2.4f, 900.0f, 14000.0f, 0.2f, 0.5f},
	{0, 0.0004f, 200.0f, 0.0f, 0.01f, 0.0f},
	{0, 0.0008f, 200.0f, 0.0f, 0.01f, 0.0f},
	{0, 0.00115f, 200.0f, 0.0f, 0.01f, 0.0f},
	{3, 2.4f, 900.0f, 14000.0f, 1.5f, -0.2f},
};
/* the particles' definitions: by point physics, and 6 one without any */
static struct particle_definition particle_definitions[7] = {{{0}}, {{1}}, {{2}}, {{3}}, {{4}}, {{5}}, {{NONE}}};
static struct point_physics_definition *point_physics_definition_get(long index) { return &point_physics[index]; }
static struct particle_definition *particle_definition_get(long index) { return &particle_definitions[index]; }

static struct particle_datum particles[HALO_PORT_MAXIMUM_PARTICLES];
/* each particle's handle, NONE while there is none */
static long particle_handles[HALO_PORT_MAXIMUM_PARTICLES];
static char particle_data[1];
static void *datum_try_and_get(void *data, long index)
{
	long absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(index);

	(void)data;
	return index != NONE && absolute_index < HALO_PORT_MAXIMUM_PARTICLES && particle_handles[absolute_index] == index ?
		&particles[absolute_index] : NULL;
}
static struct particle_datum *particle_get(long index) { return &particles[DATUM_INDEX_TO_ABSOLUTE_INDEX(index)]; }
static real particle_get_radius(long index) { return particle_get(index)->radius; }
/* the level: everything above z = -100 */
static void scenario_location_from_point(struct location *location, real_point3d const *point)
{
	location->leaf_index = 1;
	location->cluster_index = point->z > -100.0f ? 3 : NONE;
}

static struct object_datum objects[HALO_PORT_MAXIMUM_OBJECTS_PER_MAP];
static long object_handles[HALO_PORT_MAXIMUM_OBJECTS_PER_MAP];
static boolean object_is_garbage[HALO_PORT_MAXIMUM_OBJECTS_PER_MAP];
static real_matrix4x3 object_nodes[HALO_PORT_MAXIMUM_OBJECTS_PER_MAP][20];
static void *object_try_and_get_and_verify_type(long index, unsigned long mask)
{
	long absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(index);

	return index != NONE && absolute_index < HALO_PORT_MAXIMUM_OBJECTS_PER_MAP &&
		object_handles[absolute_index] == index && object_is_garbage[absolute_index] && (mask & _object_mask_garbage) ?
		&objects[absolute_index] : NULL;
}
static void object_iterator_new(struct object_iterator *iterator, unsigned long mask, long flags)
{
	(void)mask; (void)flags;
	iterator->index = NONE;
	iterator->next = 0;
}
static void *object_iterator_next(struct object_iterator *iterator)
{
	while (iterator->next < HALO_PORT_MAXIMUM_OBJECTS_PER_MAP)
	{
		long absolute_index = iterator->next++;

		if (object_handles[absolute_index] != NONE && object_is_garbage[absolute_index])
		{
			iterator->index = object_handles[absolute_index];
			return &objects[absolute_index];
		}
	}
	return NULL;
}
static real_matrix4x3 *object_get_node_matrices(long index) { return object_nodes[DATUM_INDEX_TO_ABSOLUTE_INDEX(index)]; }
/* definition 200's collision model has a box, the others none */
static boolean ragdoll_hull_get(long definition_index, real_point3d *center, real_vector3d *half_extents)
{
	if (definition_index != 200)
		return FALSE;
	center->x = 0.1f; center->y = 0.0f; center->z = 0.05f;
	half_extents->i = 0.1f; half_extents->j = 0.05f; half_extents->k = 0.025f;
	return TRUE;
}
/* whether a frame between ticks is drawn now, from interpolation's snapshots */
static boolean interpolating;
static real_matrix4x3 *render_interpolation_object_node_matrices(long index)
{
	return interpolating ? object_get_node_matrices(index) : NULL;
}

#include "under_test.inc"

static boolean near(real a, real b) { return fabsf(a - b) <= 0.0005f * (1.0f + fabsf(b)); }

static void identity(real_matrix4x3 *matrix, real x, real y, real z, real scale)
{
	memset(matrix, 0, sizeof(*matrix));
	matrix->scale = scale;
	matrix->forward.i = matrix->left.j = matrix->up.k = 1.0f;
	matrix->position.x = x; matrix->position.y = y; matrix->position.z = z;
}

static void reset(void)
{
	long index;

	physics_debris_dispose_from_old_map();
	memset(particles, 0, sizeof(particles));
	memset(objects, 0, sizeof(objects));
	for (index = 0; index < HALO_PORT_MAXIMUM_PARTICLES; index++)
		particle_handles[index] = NONE;
	for (index = 0; index < HALO_PORT_MAXIMUM_OBJECTS_PER_MAP; index++)
	{
		object_handles[index] = NONE;
		object_is_garbage[index] = FALSE;
	}
	enabled = TRUE;
	cap = PHYSICS_DEBRIS_MAX;
	changes++;
	interpolating = FALSE;
}

/* a particle of a definition, made at an absolute index under a salt, flying
from where it struck the level */
static long particle_new(long absolute_index, long salt, long definition_index, real radius)
{
	struct particle_datum *particle = &particles[absolute_index];

	memset(particle, 0, sizeof(*particle));
	particle->definition_index = definition_index;
	particle->radius = radius;
	particle->position.x = 10.0f + absolute_index; particle->position.y = 20.0f; particle->position.z = 1.0f;
	particle->translational_velocity.i = 3.0f; particle->translational_velocity.k = -4.0f;
	particle->location.cluster_index = 3;
	particle_handles[absolute_index] = (salt << 16) | absolute_index;
	return particle_handles[absolute_index];
}

/* a piece of garbage of a definition at an absolute index, its nodes 0 at
(5, 6, 7) and 1 a unit above it */
static long garbage_new(long absolute_index, long definition_index, long node_count, real scale)
{
	struct object_datum *object = &objects[absolute_index];
	long node;

	memset(object, 0, sizeof(*object));
	object->definition_index = definition_index;
	object->object.parent_object_index = NONE;
	object->object.node_matrices.size = (unsigned short)(node_count * sizeof(real_matrix4x3));
	object->object.bounding_sphere_center.x = 5.0f; object->object.bounding_sphere_center.y = 6.0f;
	object->object.bounding_sphere_center.z = 7.2f;
	object->object.bounding_sphere_radius = 0.2f;
	object->object.translational_velocity.i = 0.1f; object->object.translational_velocity.k = 0.2f;
	object->object.angular_velocity.j = 0.05f;
	for (node = 0; node < node_count && node < 20; node++)
		identity(&object_nodes[absolute_index][node], 5.0f, 6.0f, 7.0f + node, scale);
	object_handles[absolute_index] = (0x6a00L << 16) | absolute_index;
	object_is_garbage[absolute_index] = TRUE;
	return object_handles[absolute_index];
}

/* the way a pose's body points (its frame's forward) */
static real_vector3d pose_forward(struct ragdoll_pose const *pose)
{
	real_matrix4x3 frame;
	real_quaternion rotation;

	rotation.v.i = -pose->rotation.v.i; rotation.v.j = -pose->rotation.v.j; rotation.v.k = -pose->rotation.v.k;
	rotation.w = pose->rotation.w;
	matrix4x3_rotation_from_quaternion(&frame, &rotation);
	return frame.forward;
}

/* a capsule's mass, from its sizes in world units and its density in grams
a millilitre */
static real capsule_mass(real radius, real half_length, real density)
{
	real a = radius * 3.048f, h = half_length * 3.048f;

	return density * 1000.0f * (3.14159265f * a * a * 2.0f * h + 4.0f / 3.0f * 3.14159265f * a * a * a);
}

static struct ragdoll_pose pose_at(real x, real y, real z)
{
	struct ragdoll_pose pose;

	memset(&pose, 0, sizeof(pose));
	pose.position.x = x; pose.position.y = y; pose.position.z = z;
	pose.rotation.w = 1.0f;
	return pose;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	long const first = PHYSICS_DEBRIS_FIRST_ID;

	reset();

	CASE("capsule")
	{
		struct physics_debris_body body;
		real_point3d position = {1.0f, 2.0f, 3.0f};
		real_vector3d velocity = {3.0f, 0.0f, -4.0f};
		real_vector3d forward, spin;

		CHECK(physics_debris_particle_body(0.02f, &point_physics[0], &position, &velocity, &body), "a casing makes a body");
		CHECK(body.shape == HOST_PHYSICS_SHAPE_CAPSULE, "a capsule: %lu", body.shape);
		CHECK(near(body.size[0], 0.007f) && near(body.size[1], 0.013f) && near(body.size[2], 0.007f),
			"as wide as the particle: %f %f %f", body.size[0], body.size[1], body.size[2]);
		CHECK(near(body.mass, capsule_mass(0.007f, 0.013f, 2.4f)), "as heavy as stone that size: %f, not %f", body.mass,
			capsule_mass(0.007f, 0.013f, 2.4f));
		CHECK(near(body.pose.position.x, 1.0f) && near(body.pose.position.y, 2.0f) && near(body.pose.position.z, 3.0f),
			"where it struck");
		forward = pose_forward(&body.pose);
		CHECK(near(forward.i, 0.6f) && near(forward.j, 0.0f) && near(forward.k, -0.8f),
			"along the way it flies: %f %f %f", forward.i, forward.j, forward.k);
		CHECK(near(body.velocity.i, 3.0f) && near(body.velocity.k, -4.0f), "flying on as it bounced");
		spin = body.angular_velocity;
		CHECK(near(spin.i * velocity.i + spin.j * velocity.j + spin.k * velocity.k, 0.0f), "tumbling end over end");
		CHECK(near(sqrtf(spin.i * spin.i + spin.j * spin.j + spin.k * spin.k), 50.0f),
			"as fast as it would roll, at most 50 a second");
	}

	CASE("real profiles")
	{
		struct physics_debris_body body;
		real_point3d position = {0.0f, 0.0f, 0.0f};
		real_vector3d velocity = {0.0f, 2.0f, -1.0f};
		long smoke;

		CHECK(physics_debris_particle_body(0.015f, &point_physics[0], &position, &velocity, &body), "bloodgulch's casing");
		CHECK(near(body.friction, 0.1f) && near(body.restitution, 0.35f), "stone's material: %f %f", body.friction,
			body.restitution);
		CHECK(body.mass >= 0.01f && body.mass <= 50.0f && near(body.mass, capsule_mass(0.00525f, 0.00975f, 2.4f)),
			"within the host's masses: %f", body.mass);
		CHECK(body.size[0] >= 0.003f && body.size[1] <= 10.0f, "within the host's sizes");
		CHECK(physics_debris_particle_body(0.015f, &point_physics[1], &position, &velocity, &body), "the Warthog's casing");
		CHECK(near(body.friction, 0.2f) && near(body.restitution, 0.5f), "its old stone's: %f %f", body.friction,
			body.restitution);
		for (smoke = 2; smoke <= 4; smoke++)
		{
			CHECK(!physics_debris_particle_body(0.3f, &point_physics[smoke], &position, &velocity, &body),
				"smoke of %f is lighter than air", point_physics[smoke].density);
		}
	}

	CASE("clamps")
	{
		struct physics_debris_body body;
		real_point3d position = {0.0f, 0.0f, 0.0f};
		real_vector3d fast = {400.0f, 0.0f, 0.0f};
		real_vector3d up = {0.0f, 0.0f, 5.0f};
		real speed;

		CHECK(physics_debris_particle_body(0.02f, &point_physics[5], &position, &fast, &body), "made");
		CHECK(near(body.friction, 1.0f) && near(body.restitution, 0.0f), "material within 0 and 1: %f %f", body.friction,
			body.restitution);
		speed = sqrtf(body.velocity.i * body.velocity.i + body.velocity.j * body.velocity.j + body.velocity.k * body.velocity.k);
		CHECK(near(speed, 250.0f), "no faster than the host takes: %f", speed);
		CHECK(physics_debris_particle_body(100.0f, &point_physics[0], &position, &up, &body), "a huge one");
		CHECK(near(body.size[0], 10.0f) && near(body.size[1], 10.0f) && near(body.mass, 50.0f), "the host's largest");
		CHECK(body.pose.rotation.w == body.pose.rotation.w && near(pose_forward(&body.pose).k, 1.0f),
			"flying straight up, a frame all the same");
		CHECK(!physics_debris_particle_body(0.0f, &point_physics[0], &position, &up, &body), "no size, no body");
		CHECK(!physics_debris_particle_body(0.02f, NULL, &position, &up, &body), "no point physics, no body");
	}

	CASE("collided")
	{
		long particle = particle_new(0, 0x1234, 0, 0.02f);
		struct physics_debris_body const *body;

		CHECK(!physics_debris_remove(first), "nothing to remove");
		physics_debris_particle_collided(particle);
		body = physics_debris_add(first);
		CHECK(body != NULL, "struck the level: a body");
		CHECK(body && near(body->size[0], 0.007f) && near(body->friction, 0.1f), "its particle's");
		CHECK(body && near(body->pose.position.x, 10.0f) && near(body->pose.position.z, 1.0f), "where it struck");
		CHECK(!physics_debris_add(first), "added once");
		CHECK(!physics_debris_add(first + 1), "and only the one");
		CHECK(!physics_debris_remove(first), "kept while it flies");
	}

	CASE("off")
	{
		long particle = particle_new(0, 0x1234, 0, 0.02f);

		enabled = FALSE;
		physics_debris_particle_collided(particle);
		CHECK(!physics_debris_add(first), "game.physics_debris off: no body");
		{
			boolean settled = FALSE;

			CHECK(!physics_debris_particle_update(particle, 1.0f / 30.0f, &settled), "the game moves it");
		}
	}

	CASE("not debris")
	{
		physics_debris_particle_collided(particle_new(0, 0x1234, 3, 0.3f));
		physics_debris_particle_collided(particle_new(1, 0x1234, 6, 0.02f));
		physics_debris_particle_collided((0x1234L << 16) | 2);
		physics_debris_particle_collided((0x1234L << 16) | 40);
		CHECK(!physics_debris_add(first), "neither smoke, nor one without point physics, nor one that is not");
	}

	CASE("not twice")
	{
		long particle = particle_new(0, 0x1234, 0, 0.02f);

		physics_debris_particle_collided(particle);
		physics_debris_particle_collided(particle);
		CHECK(physics_debris_add(first) != NULL, "a body");
		physics_debris_particle_collided(particle);
		CHECK(!physics_debris_add(first + 1), "struck again as it tumbles: the same body");
	}

	CASE("held until posed")
	{
		long particle = particle_new(0, 0x1234, 0, 0.02f);
		boolean settled = FALSE;

		physics_debris_particle_collided(particle);
		CHECK(physics_debris_particle_update(particle, 1.0f / 30.0f, &settled), "a body's now");
		physics_debris_add(first);
		CHECK(physics_debris_particle_update(particle, 1.0f / 30.0f, &settled), "and still before the host poses it");
		CHECK(near(particles[0].position.x, 10.0f) && near(particles[0].translational_velocity.i, 3.0f) && !settled,
			"held where it struck");
	}

	CASE("moved by the host")
	{
		long particle = particle_new(0, 0x1234, 0, 0.02f);
		struct ragdoll_pose pose = pose_at(13.0f, 20.0f, 0.0f);
		boolean settled = TRUE;

		physics_debris_particle_collided(particle);
		physics_debris_add(first);
		physics_debris_result(first, TRUE, &pose);
		CHECK(physics_debris_particle_update(particle, 0.5f / 30.0f, &settled), "moved");
		CHECK(near(particles[0].position.x, 11.5f) && near(particles[0].position.z, 0.5f),
			"halfway through the tick: %f %f", particles[0].position.x, particles[0].position.z);
		CHECK(near(particles[0].translational_velocity.i, 90.0f) && near(particles[0].translational_velocity.k, -30.0f),
			"as fast as the body: %f", particles[0].translational_velocity.i);
		CHECK(!settled, "flying");
		physics_debris_particle_update(particle, 1.0f / 30.0f, &settled);
		CHECK(near(particles[0].position.x, 13.0f), "then where the host has it");
		pose = pose_at(13.0f, 21.0f, 0.0f);
		physics_debris_result(first, FALSE, &pose);
		physics_debris_particle_update(particle, 1.0f / 30.0f, &settled);
		CHECK(settled, "settled once the body sleeps");
		CHECK(near(particles[0].position.y, 21.0f) && particles[0].translational_velocity.j == 0.0f, "at rest");
		CHECK(near(particles[0].direction.i, 1.0f), "lying along its body");
	}

	CASE("creeping")
	{
		long particle = particle_new(0, 0x1234, 0, 0.02f);
		struct ragdoll_pose pose = pose_at(13.0f, 20.0f, 0.0f);
		boolean settled = TRUE;
		int tick;

		physics_debris_particle_collided(particle);
		physics_debris_add(first);
		physics_debris_result(first, TRUE, &pose);
		/* (awake, creeping a hundredth of a unit a tick: 0.3 a second, faster than the game's at rest) */
		for (tick = 0; tick < 30; tick++)
		{
			pose.position.y += 0.01f;
			physics_debris_result(first, TRUE, &pose);
			physics_debris_particle_update(particle, 1.0f / 30.0f, &settled);
		}
		CHECK(!settled, "faster than a particle at rest");
		/* (then a two-hundredth: 0.15 a second) */
		for (tick = 0; tick < 14; tick++)
		{
			pose.position.y += 0.005f;
			physics_debris_result(first, TRUE, &pose);
			physics_debris_particle_update(particle, 1.0f / 30.0f, &settled);
		}
		CHECK(!settled, "slow for less than half a second");
		pose.position.y += 0.005f;
		physics_debris_result(first, TRUE, &pose);
		physics_debris_particle_update(particle, 1.0f / 30.0f, &settled);
		CHECK(settled && particles[0].translational_velocity.j == 0.0f, "slow for half a second: at rest, awake or not");
		pose.position.y += 0.05f;
		physics_debris_result(first, TRUE, &pose);
		physics_debris_particle_update(particle, 1.0f / 30.0f, &settled);
		CHECK(!settled, "and moving again once struck");
	}

	CASE("out of the level")
	{
		long particle = particle_new(0, 0x1234, 0, 0.02f);
		struct ragdoll_pose pose = pose_at(13.0f, 20.0f, -500.0f);
		boolean settled = FALSE;

		physics_debris_particle_collided(particle);
		physics_debris_add(first);
		physics_debris_result(first, TRUE, &pose);
		physics_debris_particle_update(particle, 1.0f / 30.0f, &settled);
		CHECK(near(particles[0].position.z, 1.0f) && particles[0].location.cluster_index == 3, "kept in the level");
	}

	CASE("lifetime")
	{
		long particle = particle_new(0, 0x1234, 0, 0.02f);
		long again;
		boolean settled = FALSE;

		physics_debris_particle_collided(particle);
		physics_debris_add(first);
		particles[0].flags |= FLAG(_particle_datum_at_rest_bit);
		CHECK(physics_debris_remove(first), "its particle at rest: the body goes");
		CHECK(!physics_debris_particle_update(particle, 1.0f / 30.0f, &settled), "forgotten");
		particles[0].flags = 0;
		particle = particle_new(1, 0x1234, 0, 0.02f);
		physics_debris_particle_collided(particle);
		physics_debris_add(first);
		particle_handles[1] = NONE;
		CHECK(physics_debris_remove(first), "its particle gone: the body goes");
		again = particle_new(1, 0x4321, 0, 0.02f);
		CHECK(!physics_debris_particle_update(again, 1.0f / 30.0f, &settled), "another made where it was is another");
		physics_debris_particle_collided(again);
		CHECK(physics_debris_add(first) != NULL, "and a body of its own");
		/* (gone, and another made where it was, between two ticks) */
		particle_handles[1] = NONE;
		again = particle_new(1, 0x5555, 0, 0.02f);
		CHECK(!physics_debris_particle_update(again, 1.0f / 30.0f, &settled), "not moved as the body of the one before");
	}

	CASE("gone before added")
	{
		long particle = particle_new(0, 0x1234, 0, 0.02f);

		physics_debris_particle_collided(particle);
		particle_handles[0] = NONE;
		CHECK(!physics_debris_remove(first), "no body to remove");
		CHECK(!physics_debris_add(first), "none added");
	}

	CASE("cap")
	{
		long oldest = particle_new(0, 0x1234, 0, 0.02f);
		long next = particle_new(1, 0x1234, 0, 0.02f);
		long newest = particle_new(2, 0x1234, 1, 0.02f);
		struct physics_debris_body const *body;
		boolean settled = FALSE;

		cap = 2;
		changes++;
		physics_debris_particle_collided(oldest);
		physics_debris_particle_collided(next);
		CHECK(physics_debris_add(first) && physics_debris_add(first + 1), "two bodies");
		physics_debris_particle_collided(newest);
		CHECK(!physics_debris_add(first), "nothing added under an id the host still has");
		CHECK(physics_debris_remove(first), "the oldest's body goes first");
		CHECK(!physics_debris_remove(first + 1), "the other kept");
		body = physics_debris_add(first);
		CHECK(body && near(body->restitution, 0.5f), "the newest's in its place");
		CHECK(!physics_debris_add(first + 2), "no more than the cap");
		CHECK(!physics_debris_particle_update(oldest, 1.0f / 30.0f, &settled), "the oldest handed back to the game");
		physics_debris_particle_collided(oldest);
		CHECK(!physics_debris_remove(first) && !physics_debris_remove(first + 1) && !physics_debris_add(first),
			"for good");
	}

	CASE("cap before added")
	{
		long oldest = particle_new(0, 0x1234, 0, 0.02f);
		long newest = particle_new(1, 0x1234, 1, 0.02f);
		struct physics_debris_body const *body;

		cap = 1;
		changes++;
		physics_debris_particle_collided(oldest);
		physics_debris_particle_collided(newest);
		CHECK(!physics_debris_remove(first), "no body yet to remove");
		body = physics_debris_add(first);
		CHECK(body && near(body->restitution, 0.5f), "the newest's");
	}

	CASE("refused")
	{
		long particle = particle_new(0, 0x1234, 0, 0.02f);
		boolean settled = FALSE;

		physics_debris_particle_collided(particle);
		physics_debris_add(first);
		CHECK(!physics_debris_remove(first) && !physics_debris_remove(first), "waited for");
		CHECK(physics_debris_remove(first), "unposed for three ticks: the host refused it");
		CHECK(!physics_debris_particle_update(particle, 1.0f / 30.0f, &settled), "the game has it");
		physics_debris_particle_collided(particle);
		CHECK(!physics_debris_add(first), "and keeps it");
	}

	CASE("ids")
	{
		struct ragdoll_pose pose = pose_at(0.0f, 0.0f, 0.0f);

		CHECK(!physics_debris_remove(first - 1) && !physics_debris_remove(first + PHYSICS_DEBRIS_MAX), "not debris'");
		CHECK(!physics_debris_add(first - 1) && !physics_debris_add(first + PHYSICS_DEBRIS_MAX), "nor these");
		physics_debris_result(first - 1, TRUE, &pose);
		physics_debris_result(first + PHYSICS_DEBRIS_MAX, TRUE, &pose);
		physics_debris_result(first, TRUE, &pose);
		CHECK(!physics_debris_add(first), "a result for no body is nothing");
	}

	CASE("disposed")
	{
		physics_debris_particle_collided(particle_new(0, 0x1234, 0, 0.02f));
		physics_debris_add(first);
		physics_debris_dispose_from_old_map();
		CHECK(!physics_debris_remove(first) && !physics_debris_add(first), "the map gone: every piece forgotten");
	}

	CASE("garbage")
	{
		long object = garbage_new(0, 200, 2, 1.0f);
		struct physics_debris_body const *body;

		physics_debris_garbage_update();
		body = physics_debris_add(first);
		CHECK(body != NULL, "garbage: a body");
		CHECK(body && body->shape == HOST_PHYSICS_SHAPE_BOX, "a box");
		CHECK(body && near(body->size[0], 0.1f) && near(body->size[1], 0.05f) && near(body->size[2], 0.025f),
			"its collision model's");
		CHECK(body && near(body->mass, 8.0f * 0.1f * 0.05f * 0.025f * 28.3168f * 1000.0f), "a litre of it a kilogram");
		CHECK(body && near(body->friction, 0.6f) && near(body->restitution, 0.2f), "garbage's material");
		CHECK(body && near(body->pose.position.x, 5.1f) && near(body->pose.position.z, 7.05f), "at its box's centre");
		CHECK(body && near(body->velocity.i, 3.0f) && near(body->velocity.k, 6.0f) && near(body->angular_velocity.j, 1.5f),
			"as fast as it was a tick");
		physics_debris_garbage_update();
		CHECK(!physics_debris_add(first + 1), "once");
		object_handles[0] = NONE;
		CHECK(physics_debris_remove(first), "its object gone: the body goes");
		(void)object;
	}

	CASE("garbage without a hull")
	{
		struct physics_debris_body const *body;

		garbage_new(0, 201, 1, 0.5f);
		physics_debris_garbage_update();
		body = physics_debris_add(first);
		CHECK(body && near(body->size[0], 0.045f) && near(body->size[2], 0.045f),
			"a box in its bounding sphere, at its scale: %f", body ? body->size[0] : 0.0f);
		CHECK(body && near(body->pose.position.z, 7.2f), "about its centre");
	}

	CASE("garbage not debris")
	{
		garbage_new(0, 200, 2, 1.0f);
		objects[0].object.parent_object_index = 5;
		garbage_new(1, 200, 17, 1.0f);
		garbage_new(2, 200, 0, 1.0f);
		physics_debris_garbage_update();
		CHECK(!physics_debris_add(first), "neither held nor of too many nodes nor of none");
		enabled = FALSE;
		garbage_new(3, 200, 2, 1.0f);
		physics_debris_garbage_update();
		CHECK(!physics_debris_add(first), "nor any with game.physics_debris off");
	}

	CASE("garbage drawn")
	{
		long object = garbage_new(0, 200, 2, 1.0f);
		real_matrix4x3 turned;
		struct ragdoll_pose pose;
		real_matrix4x3 const *drawn;

		physics_debris_garbage_update();
		physics_debris_add(first);
		CHECK(physics_debris_node_matrices(object, object_nodes[0]) == object_nodes[0], "drawn as the game has it until posed");
		/* the host has it at (10, 0, 3), turned a quarter about z */
		memset(&turned, 0, sizeof(turned));
		turned.scale = 1.0f;
		turned.forward.j = 1.0f; turned.left.i = -1.0f; turned.up.k = 1.0f;
		turned.position.x = 10.0f; turned.position.z = 3.0f;
		ragdoll_pose_from_matrix(&turned, &pose);
		physics_debris_result(first, TRUE, &pose);
		drawn = physics_debris_node_matrices(object, object_nodes[0]);
		CHECK(drawn != object_nodes[0], "drawn from its body");
		CHECK(near(drawn[0].forward.j, 1.0f) && near(drawn[0].left.i, -1.0f), "turned as its body is");
		CHECK(near(drawn[0].position.x, 10.0f) && near(drawn[0].position.y, -0.1f) && near(drawn[0].position.z, 2.95f),
			"its box's centre where the body's is: %f %f %f", drawn[0].position.x, drawn[0].position.y,
			drawn[0].position.z);
		CHECK(near(drawn[1].position.x, 10.0f) && near(drawn[1].position.y, -0.1f) && near(drawn[1].position.z, 3.95f),
			"its other nodes carried with it: %f %f %f", drawn[1].position.x, drawn[1].position.y, drawn[1].position.z);
		CHECK(near(drawn[0].scale, 1.0f), "at its scale");
		interpolating = TRUE;
		CHECK(physics_debris_node_matrices(object, object_nodes[0]) == object_nodes[0],
			"a frame between ticks from interpolation's snapshots");
		interpolating = FALSE;
		CHECK(physics_debris_node_matrices(garbage_new(1, 200, 2, 1.0f), object_nodes[1]) == object_nodes[1],
			"garbage that is no body as the game has it");
		CHECK(physics_debris_node_matrices(NONE, object_nodes[1]) == object_nodes[1] &&
			physics_debris_node_matrices(object, NULL) == NULL, "nor anything else");
	}

	return 0;
}
