#include "harness.h"
#include <math.h>

#define SET_FLAG(v,b,s) ((v)=(s)?((v)|FLAG(b)):((v)&~FLAG(b)))
#include "config.inc"

/* One body on a client, on flat ground at z 0. Its drawn pose is its root
node: where the object is, raised by its pose (FALL_POSE in the pose of its
fall, airborne; 0 lying), recomputed each tick unless lying limp, when it
stays where it was in the world and relaxes onto the ground there (objects.c,
bipeds.c). */
#define BODY 0x12340007L
#define FALL_POSE 0.36f
#define RELAXATION_ITERATIONS 4
struct _object_datum
{
	real_point3d position;
	real_vector3d forward, up, translational_velocity, angular_velocity;
	unsigned long flags, damage_flags;
	short type;
};
struct _unit_datum { short unused; };
struct _biped_datum { unsigned long flags; byte limp_body_current_relaxation_iterations, limp_body_max_relaxation_iterations; };
struct biped_datum { long definition_index; struct _object_datum object; struct _unit_datum unit; struct _biped_datum biped; };
#define object_datum biped_datum
struct biped_definition { struct { unsigned long flags; } biped; };
static struct biped_datum body;
static struct biped_definition definition;
static boolean debug_biped_limp_body_disable;
static real_point3d node;
static int interpolations;

static struct biped_datum *object_get(long index) { CHECK(index == BODY, "not the body"); return &body; }
static void *object_get_and_verify_type(long index, unsigned long mask)
{
	CHECK(index == BODY && TEST_FLAG(mask, body.object.type), "not a biped");
	return &body;
}
static void *object_try_and_get_and_verify_type(long index, unsigned long mask)
{
	return index == BODY && TEST_FLAG(mask, body.object.type) ? &body : NULL;
}
#define biped_get(index) ((struct biped_datum *)object_get_and_verify_type((index), _object_mask_biped))
static struct biped_definition *biped_definition_get(long index) { return &definition; }
static long biped_limp_noodle_get_max_relaxation_iterations(long index) { return RELAXATION_ITERATIONS; }
static void pose_compute(void)
{
	node = body.object.position;
	if (TEST_FLAG(body.biped.flags, _biped_airborne_bit))
		node.z += FALL_POSE;
}
/* (objects.c: placed, its nodes are computed, fixed or not) */
static void object_set_position(long index, real_point3d const *position, real_vector3d const *forward, real_vector3d const *up)
{
	CHECK(index == BODY, "not the body");
	body.object.position = *position;
	body.object.forward = *forward;
	body.object.up = *up;
	pose_compute();
}
static void render_interpolation_correct_object(long index, real_vector3d const *offset) { interpolations++; }
static boolean distributed_vehicle_unsteered(long index) { return FALSE; }
static boolean distributed_object_follows_host(long index) { return FALSE; }
static boolean distributed_transform_valid(real_point3d const *position, real_vector3d const *forward, real_vector3d const *up,
	real_vector3d const *velocity, real_vector3d const *angular_velocity, real_vector3d *valid_forward, real_vector3d *valid_up)
{
	*valid_forward = *forward;
	*valid_up = *up;
	return TRUE;
}
#include "under_test.inc"

static boolean limp(void) { return TEST_FLAG(body.biped.flags, _biped_limp_body_physics_active_bit); }
static boolean fixed(void) { return TEST_FLAG(body.object.flags, _object_do_not_recompute_node_matrices_bit); }
static real distance(real_point3d const *a, real_point3d const *b)
{
	real dx = a->x - b->x, dy = a->y - b->y, dz = a->z - b->z;

	return (real)sqrt(dx * dx + dy * dy + dz * dz);
}

/* a tick of the client's game: its physics, unless at rest (bipeds.c); the
limp body's relaxation; limp again where it can be, each tick at the end of
its dying animation (units.c); its nodes, unless fixed (objects.c) */
static void tick(void)
{
	if (!TEST_FLAG(body.object.flags, _object_at_rest_bit))
	{
		body.object.position.x += body.object.translational_velocity.i / 30.0f;
		body.object.position.y += body.object.translational_velocity.j / 30.0f;
		body.object.position.z += body.object.translational_velocity.k / 30.0f;
		if (body.object.position.z <= 0.0f)
		{
			body.object.position.z = 0.0f;
			body.object.translational_velocity.i *= 0.8f;
			body.object.translational_velocity.j *= 0.8f;
			body.object.translational_velocity.k = 0.0f;
			SET_FLAG(body.biped.flags, _biped_airborne_bit, FALSE);
		}
		else
		{
			body.object.translational_velocity.k -= 9.8f / 30.0f;
			SET_FLAG(body.biped.flags, _biped_airborne_bit, TRUE);
		}
	}
	if (limp() && body.biped.limp_body_current_relaxation_iterations < body.biped.limp_body_max_relaxation_iterations)
	{
		body.biped.limp_body_current_relaxation_iterations++;
		node.z = 0.0f;
	}
	biped_start_limp_body_physics(BODY);
	if (!fixed())
		pose_compute();
}

/* the host's word on the body, as network_objects_handle_states takes it */
static void receive(real x, real y, real z, boolean at_rest)
{
	real_point3d position;
	real_vector3d forward = {1.0f, 0.0f, 0.0f}, up = {0.0f, 0.0f, 1.0f};

	position.x = x;
	position.y = y;
	position.z = z;
	SET_FLAG(body.object.flags, _object_at_rest_bit, at_rest);
	distributed_body_set_at_rest(BODY, at_rest);
	if (distance(&position, &body.object.position) > REMOTE_OBJECT_TOLERANCE)
	{
		network_objects_reconcile(BODY, &position, &forward, &up, &body.object.translational_velocity, NULL,
			at_rest ? 0.0f : REMOTE_BLEND_DISTANCE);
	}
}

/* a body come to rest on the ground at (x, y), lying limp and relaxed */
static void lying(real x, real y)
{
	int i;

	memset(&body, 0, sizeof(body));
	body.object.type = _object_type_biped;
	body.object.forward.i = 1.0f;
	body.object.up.k = 1.0f;
	body.object.position.x = x;
	body.object.position.y = y;
	SET_FLAG(body.object.damage_flags, _object_dead_bit, TRUE);
	SET_FLAG(body.object.flags, _object_at_rest_bit, TRUE);
	for (i = 0; i < 10; i++)
		tick();
	CHECK(limp() && fixed() && body.biped.limp_body_current_relaxation_iterations == RELAXATION_ITERATIONS,
		"the fake world: a body at rest lies limp");
}

static void settled(char const *what)
{
	int i;

	for (i = 0; i < 10; i++)
		tick();
	CHECK(limp(), "%s: never lies limp again", what);
	CHECK(distance(&node, &body.object.position) <= REMOTE_OBJECT_TOLERANCE, "%s: drawn %.2f from where it is (%.2f %.2f %.2f, at %.2f %.2f %.2f)",
		what, distance(&node, &body.object.position), node.x, node.y, node.z,
		body.object.position.x, body.object.position.y, body.object.position.z);
}

int main(int argc, char **argv)
{
	char const *case_name = argv[1];
	int i;

	SET_FLAG(definition.biped.flags, _biped_uses_limp_body_physics_bit, TRUE);
	/* An explosion this machine does not hear of pushes the host's body: it
	moves, each word of the host's close enough to this copy's own physics
	not to be corrected, and comes to rest a few metres away. */
	CASE("pushed")
	{
		lying(0.0f, 0.0f);
		receive(0.02f, 0.0f, 0.0f, FALSE);
		body.object.translational_velocity.i = 12.0f;
		for (i = 0; i < 30; i++)
		{
			tick();
			receive(body.object.position.x, body.object.position.y, body.object.position.z, FALSE);
		}
		CHECK(body.object.position.x > 1.0f, "the fake world: the body is pushed away");
		receive(body.object.position.x, body.object.position.y, body.object.position.z, TRUE);
		settled("pushed");
	}
	/* The host's body lands; this copy, a little behind, still in the air
	(close enough not to be corrected) when the host's word comes that it is
	at rest. */
	CASE("landed")
	{
		lying(0.0f, 0.0f);
		receive(0.0f, 0.0f, 0.0f, FALSE);
		body.object.position.z = 0.5f;
		tick();
		tick();
		body.object.position.z = 0.03f;
		CHECK(TEST_FLAG(body.biped.flags, _biped_airborne_bit) && !limp(), "the fake world: the copy is falling");
		receive(0.0f, 0.0f, 0.0f, TRUE);
		settled("landed");
		CHECK(!TEST_FLAG(body.biped.flags, _biped_airborne_bit), "landed: left airborne");
	}
	/* The host's body, at rest, put somewhere else: it lies again there, its
	relaxation from the start, not the animation's pose relaxed no further. */
	CASE("moved")
	{
		lying(0.0f, 0.0f);
		receive(3.0f, 1.0f, 0.0f, TRUE);
		CHECK(body.object.position.x == 3.0f && interpolations == 1, "moved: not put where the host has it");
		tick();
		CHECK(limp() && body.biped.limp_body_current_relaxation_iterations == 0, "moved: relaxation not started again (%d)",
			body.biped.limp_body_current_relaxation_iterations);
		settled("moved");
	}
	/* a body brought halfway to where the host has it, a little at a time */
	CASE("blended")
	{
		real_point3d position = {0.5f, 0.0f, 0.0f};
		real_vector3d forward = {1.0f, 0.0f, 0.0f}, up = {0.0f, 0.0f, 1.0f};

		lying(0.0f, 0.0f);
		network_objects_reconcile(BODY, &position, &forward, &up, NULL, NULL, REMOTE_BLEND_DISTANCE);
		CHECK(body.object.position.x == 0.25f, "the fake world: blended halfway");
		CHECK(!limp(), "blended: still lying limp where it was");
		settled("blended");
	}
	/* a living biped, as it was */
	CASE("living")
	{
		lying(0.0f, 0.0f);
		SET_FLAG(body.object.damage_flags, _object_dead_bit, FALSE);
		SET_FLAG(body.biped.flags, _biped_airborne_bit, TRUE);
		receive(0.0f, 0.0f, 0.0f, TRUE);
		CHECK(TEST_FLAG(body.biped.flags, _biped_airborne_bit), "living: its airborne changed");
		receive(2.0f, 0.0f, 0.0f, FALSE);
		CHECK(limp() && fixed(), "living: its pose changed");
	}
	return 0;
}
