#include "harness.h"
#include <math.h>
#include <string.h>

/* Deaths and hits for the ragdolls (ragdolls.c), in a fake world of three
objects: 0 a biped with a body, 1 a vehicle with a model that would make
one, 2 a biped whose model has no body, and from 3 on more of the first. A
hit is remembered for its tick; a biped's death queues its node matrices,
velocity and the hit that killed it that tick; an explosion that kills a
biped, or reaches its ragdoll, tears the joints it hurts past their
thresholds, and bleeds there; a torn body is drawn a piece at a time. */
typedef struct { real scale; union { real n[4][3]; struct { real_vector3d forward, left, up; real_point3d position; }; }; }
	real_matrix4x3;
typedef struct { real_vector3d v; real w; } real_quaternion;
struct tag_block { long count; void *address; long definition; };
#define _real_epsilon 0.0001f
#define CEILING(n, ceiling) ((n) > (ceiling) ? (ceiling) : (n))
#define PIN(n, floor, ceiling) ((n) < (floor) ? (floor) : CEILING((n), (ceiling)))
enum { _object_type_biped = 0, _object_type_vehicle = 1 };
enum { _object_mask_biped = 1 << _object_type_biped };
struct object_header_block_reference { unsigned short size, offset; };
struct _object_datum { short type; struct object_header_block_reference node_matrices; };
struct object_datum { long definition_index; struct _object_datum object; };
struct object_definition { struct { real acceleration_scale; struct { long index; } collision_model; } object; };
struct collision_model { struct { struct { long index; } localized_damage_effect; } resistance; };
enum { _damage_area_of_effect_bit = 0 };
struct damage_definition
{
	real instantaneous_acceleration, instantaneous_acceleration_zero_scale_factor;
	real damage_minimum, damage_lower_bound, damage_upper_bound;
	short category;
};
struct damage_effect_definition { struct damage_definition damage; unsigned long flags; real falloff_radius, cutoff_radius; };
struct damage_data
{
	long definition_index; real_point3d origin; real_vector3d direction; real scale;
	unsigned long flags; real_point3d epicenter; real multiplier;
};

#include "config.inc"

static boolean enabled = TRUE;
static long now = 10;
static struct object_datum objects[3] = {{100, {_object_type_biped}}, {101, {_object_type_vehicle}}, {102, {_object_type_biped}}};
static real_matrix4x3 matrices[RAGDOLL_BODY_NODES_MAX];
static struct ragdoll_body body;
/* a grenade's: 10 at its edge, 100 to 140 at its heart, falling off from one
unit to three */
static struct damage_effect_definition grenade = {{6.0f, 0.25f, 10.0f, 100.0f, 140.0f, 3}, 0, 1.0f, 3.0f};
static struct damage_effect_definition poke = {{0.0f, 0.0f}};
/* the bipeds': 100's, and 102's half */
static struct object_definition whole = {{1.0f, {5}}}, half = {{0.5f, {-1}}};
/* the bipeds' collision model, with its blood */
static struct collision_model flesh = {{{77}}};
static real_vector3d const zero_vector = {0.0f, 0.0f, 0.0f};
static real_vector3d const *global_zero_vector3d = &zero_vector;

/* the effects made: how many, and the last one's */
static long bled_count;
static long bled_effect, bled_owner;
static char const *bled_names[8];
static real_point3d bled_at;
static real_vector3d bled_normal, bled_gravity;

static boolean deleted[3];
static short const matrix4x3_next[3] = {1, 2, 0};

static boolean dismember = TRUE;

static boolean physics_jolt_ragdolls_enabled(void) { return enabled; }
static boolean physics_jolt_dismemberment_enabled(void) { return dismember; }
static real square_root(real x) { return sqrtf(x); }
static void *object_try_and_get_and_verify_type(long index, unsigned long mask)
{
	struct object_datum *object = &objects[index < 3 ? index : 0];

	return index >= 0 && !deleted[index < 3 ? index : 0] && (mask & (1u << object->object.type)) ? object : NULL;
}
static long game_time_get(void) { return now; }
static struct object_datum *object_get(long index) { return &objects[index < 3 ? index : 0]; }
/* while a frame is drawn, interpolation's pose, else the game's */
static real_matrix4x3 *interpolated;
static real_matrix4x3 *object_get_node_matrices(long index) { (void)index; return interpolated ? interpolated : matrices; }
static void *object_header_block_get(long index, struct object_header_block_reference *reference)
{
	(void)index; (void)reference;
	return matrices;
}
static void object_get_velocities(long index, real_vector3d *velocity, real_vector3d *angular)
{
	velocity->i = 1.0f + index; velocity->j = 2.0f; velocity->k = -3.0f;
	angular->i = angular->j = angular->k = 0.0f;
}
static struct damage_effect_definition *damage_effect_definition_get(long index) { return index == 8 ? &poke : &grenade; }
static struct object_definition *object_definition_get(long index) { return index == 102 ? &half : &whole; }
static struct ragdoll_body const *ragdoll_body_get(long definition_index) { return definition_index != 102 ? &body : NULL; }
static struct collision_model *collision_model_definition_get(long index) { return index == 5 ? &flesh : NULL; }
static long effect_new_unattached_from_markers(long definition_index, long owner_object_index,
	real_vector3d const *velocity, short marker_count, char const **marker_names, real_point3d const *marker_points,
	real_vector3d const *marker_forwards, real scale_a, real scale_b, void const *color, void const *impulse_field,
	boolean can_be_deterministic)
{
	short marker;

	(void)velocity; (void)scale_a; (void)scale_b; (void)color; (void)impulse_field; (void)can_be_deterministic;
	bled_count++;
	bled_effect = definition_index;
	bled_owner = owner_object_index;
	for (marker = 0; marker < marker_count && marker < 8; marker++)
		bled_names[marker] = marker_names[marker];
	bled_at = marker_points[0];
	bled_normal = marker_forwards[0];
	bled_gravity = marker_forwards[marker_count - 1];
	return 1;
}
static real magnitude3d(real_vector3d const *v) { return sqrtf(v->i * v->i + v->j * v->j + v->k * v->k); }
static real_vector3d *scale_vector3d(real_vector3d const *v, real s, real_vector3d *out)
{
	out->i = v->i * s; out->j = v->j * s; out->k = v->k * s; return out;
}

/* whether a frame between ticks is drawn now, from interpolation's snapshots */
static boolean interpolating;
static real_matrix4x3 *render_interpolation_object_node_matrices(long index) { (void)index; return interpolating ? matrices : NULL; }
/* (garbage drawn from its body, physics_debris.c: none here) */
static real_matrix4x3 const *physics_debris_node_matrices(long index, real_matrix4x3 const *nodes) { (void)index; return nodes; }

#include "under_test.inc"

static struct ragdoll_death death_of(long object_index)
{
	struct ragdoll_death death;

	memset(&death, 0, sizeof(death));
	death.object_index = object_index;
	death.body = &body;
	memcpy(death.nodes, matrices, sizeof(death.nodes));
	return death;
}

static real_matrix4x3 rotation(real_vector3d forward, real_vector3d left, real_vector3d up)
{
	real_matrix4x3 matrix;

	memset(&matrix, 0, sizeof(matrix));
	matrix.scale = 1.0f;
	matrix.forward = forward;
	matrix.left = left;
	matrix.up = up;
	matrix.position.x = 5.0f;
	matrix.position.z = -2.0f;
	return matrix;
}

static boolean quaternion_is(real_quaternion const *q, real i, real j, real k, real w)
{
	real same = fabsf(q->v.i - i) + fabsf(q->v.j - j) + fabsf(q->v.k - k) + fabsf(q->w - w);
	real opposite = fabsf(q->v.i + i) + fabsf(q->v.j + j) + fabsf(q->v.k + k) + fabsf(q->w + w);

	return same < 1e-4f || opposite < 1e-4f;
}

static struct damage_data blast(real i, real j, real k, real scale)
{
	struct damage_data damage;

	damage.definition_index = 7;
	damage.origin.x = 1.0f; damage.origin.y = 2.0f; damage.origin.z = 3.0f;
	damage.direction.i = i; damage.direction.j = j; damage.direction.k = k;
	damage.scale = scale;
	damage.flags = 0;
	damage.epicenter = damage.origin;
	damage.multiplier = 1.0f;
	return damage;
}

/* a grenade's blast at a point, pushing up */
static struct damage_data explosion(real x, real y, real z)
{
	struct damage_data damage = blast(0.0f, 0.0f, 1.0f, 1.0f);

	damage.flags = FLAG(_damage_area_of_effect_bit);
	damage.epicenter.x = x; damage.epicenter.y = y; damage.epicenter.z = z;
	return damage;
}

/* a fourth part, off the first at node 3; the three below it breakable at
50 */
static void breakable(void)
{
	body.part_count = 4;
	body.parts[3].node_index = 3;
	body.parts[3].parent_part_index = 0;
	body.parts[1].break_damage = body.parts[2].break_damage = body.parts[3].break_damage = 50.0f;
}

static boolean near(real a, real b) { return fabsf(a - b) < 1e-4f; }

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";
	struct damage_data damage;
	long index;

	/* three parts along x, at the first three nodes; the middle one's shape
	half a unit on */
	body.node_count = 5;
	body.part_count = 3;
	body.mass = 80.0f;
	for (index = 0; index < 3; index++)
	{
		body.parts[index].node_index = (short)index;
		body.parts[index].parent_part_index = (short)(index - 1);
	}
	body.parts[1].center.x = 0.5f;
	for (index = 0; index < RAGDOLL_BODY_NODES_MAX; index++)
	{
		matrices[index].scale = 1.0f;
		matrices[index].forward.i = matrices[index].left.j = matrices[index].up.k = 1.0f;
		matrices[index].position.x = (real)index;
	}

	CASE("death")
	{
		struct ragdoll_death const *death;

		ragdolls_unit_died(0);
		death = ragdolls_death_get(0);
		CHECK(ragdolls_death_count() == 1 && death, "queued: %ld", ragdolls_death_count());
		CHECK(death->object_index == 0 && death->definition_index == 100 && death->body == &body, "the biped");
		CHECK(death->nodes[4].position.x == 4.0f && death->nodes[5].position.x == 0.0f, "its body's nodes");
		CHECK(death->velocity.i == 1.0f && death->velocity.k == -3.0f, "its velocity");
		CHECK(!death->hit, "no hit");
		CHECK(!ragdolls_death_get(1) && !ragdolls_death_get(-1), "one");
		ragdolls_deaths_clear();
		CHECK(ragdolls_death_count() == 0, "cleared");
		return 0;
	}
	CASE("killing hit")
	{
		struct ragdoll_death const *death;

		damage = blast(0.0f, 3.0f, 4.0f, 0.5f);
		ragdolls_damage(0, &damage);
		ragdolls_unit_died(0);
		death = ragdolls_death_get(0);
		CHECK(death && death->hit, "the hit");
		CHECK(near(death->killing_hit.direction.j, 0.6f) && near(death->killing_hit.direction.k, 0.8f),
			"its direction: %f %f", death->killing_hit.direction.j, death->killing_hit.direction.k);
		/* all 6 of it at any scale, as damage.c accelerates the biped */
		CHECK(near(death->killing_hit.speed, 6.0f), "its speed: %f", death->killing_hit.speed);
		CHECK(death->killing_hit.point.z == 3.0f && death->killing_hit.time == now, "its point");
		return 0;
	}
	CASE("newest hit")
	{
		damage = blast(1.0f, 0.0f, 0.0f, 1.0f);
		ragdolls_damage(0, &damage);
		damage = blast(0.0f, 0.0f, 2.0f, 0.0f);
		ragdolls_damage(0, &damage);
		CHECK(ragdolls_hit_get(0) && ragdolls_hit_get(0)->direction.k == 1.0f, "the newer");
		ragdolls_damage(2, &damage);
		CHECK(near(ragdolls_hit_get(2)->speed, 3.0f), "the biped's acceleration scale: %f", ragdolls_hit_get(2)->speed);
		for (index = 0; index < RAGDOLL_HITS_MAX; index++)
			ragdolls_damage(2, &damage);
		CHECK(!ragdolls_hit_get(0), "overwritten");
		return 0;
	}
	CASE("stale hit")
	{
		damage = blast(1.0f, 0.0f, 0.0f, 1.0f);
		ragdolls_damage(0, &damage);
		now++;
		ragdolls_unit_died(0);
		CHECK(ragdolls_death_count() == 1 && !ragdolls_death_get(0)->hit, "last tick's hit");
		return 0;
	}
	CASE("bad numbers")
	{
		damage = blast(0.0f, 0.0f, 0.0f, 1.0f);
		ragdolls_damage(0, &damage);
		CHECK(ragdolls_hit_get(0)->speed == 0.0f, "no direction");
		damage = blast(NAN, 0.0f, 1.0f, 1.0f);
		ragdolls_damage(0, &damage);
		CHECK(ragdolls_hit_get(0)->speed == 0.0f && ragdolls_hit_get(0)->direction.i == 0.0f, "a nan direction");
		damage = blast(0.0f, 0.0f, 1.0f, 1.0f);
		damage.definition_index = 8;
		ragdolls_damage(0, &damage);
		CHECK(ragdolls_hit_get(0)->speed == 0.0f && ragdolls_hit_get(0)->direction.k == 0.0f, "no acceleration");
		damage.definition_index = NONE;
		ragdolls_damage(1, &damage);
		return 0;
	}
	CASE("off")
	{
		enabled = FALSE;
		damage = blast(1.0f, 0.0f, 0.0f, 1.0f);
		ragdolls_damage(0, &damage);
		ragdolls_unit_died(0);
		CHECK(ragdolls_death_count() == 0 && !ragdolls_hit_get(0), "nothing while off");
		return 0;
	}
	CASE("not biped")
	{
		damage = blast(1.0f, 0.0f, 0.0f, 1.0f);
		ragdolls_damage(1, &damage);
		ragdolls_unit_died(1);
		CHECK(ragdolls_death_count() == 0 && !ragdolls_hit_get(1), "a vehicle");
		ragdolls_unit_died(NONE);
		CHECK(ragdolls_death_count() == 0, "none");
		return 0;
	}
	CASE("no body")
	{
		ragdolls_unit_died(2);
		CHECK(ragdolls_death_count() == 0, "the stock death");
		return 0;
	}
	CASE("once")
	{
		ragdolls_unit_died(0);
		ragdolls_unit_died(0);
		CHECK(ragdolls_death_count() == 1, "twice: %ld", ragdolls_death_count());
		return 0;
	}
	CASE("capacity")
	{
		for (index = 0; index <= RAGDOLL_DEATHS_MAX; index++)
			ragdolls_unit_died(3 + index);
		CHECK(ragdolls_death_count() == RAGDOLL_DEATHS_MAX, "full: %ld", ragdolls_death_count());
		ragdolls_dispose_from_old_map();
		CHECK(ragdolls_death_count() == 0, "gone with the map");
		return 0;
	}
	CASE("pose")
	{
		static real_vector3d const x = {1, 0, 0}, y = {0, 1, 0}, z = {0, 0, 1}, minus_x = {-1, 0, 0},
			minus_y = {0, -1, 0}, minus_z = {0, 0, -1}, nothing = {0, 0, 0};
		struct ragdoll_pose pose;
		real_matrix4x3 matrix;

		/* a quarter turn about z: forward to y */
		matrix = rotation(y, minus_x, z);
		ragdoll_pose_from_matrix(&matrix, &pose);
		CHECK(quaternion_is(&pose.rotation, 0.0f, 0.0f, 0.70710678f, 0.70710678f), "about z: %f %f %f %f",
			pose.rotation.v.i, pose.rotation.v.j, pose.rotation.v.k, pose.rotation.w);
		CHECK(pose.position.x == 5.0f && pose.position.z == -2.0f, "its point");
		/* a half turn about x (no trace) */
		matrix = rotation(x, minus_y, minus_z);
		ragdoll_pose_from_matrix(&matrix, &pose);
		CHECK(quaternion_is(&pose.rotation, 1.0f, 0.0f, 0.0f, 0.0f), "about x: %f %f %f %f",
			pose.rotation.v.i, pose.rotation.v.j, pose.rotation.v.k, pose.rotation.w);
		/* a third of a turn about (1, 1, 1): x to y, y to z */
		matrix = rotation(y, z, x);
		ragdoll_pose_from_matrix(&matrix, &pose);
		CHECK(quaternion_is(&pose.rotation, 0.5f, 0.5f, 0.5f, 0.5f), "about the diagonal: %f %f %f %f",
			pose.rotation.v.i, pose.rotation.v.j, pose.rotation.v.k, pose.rotation.w);
		/* a broken matrix: no rotation */
		matrix = rotation(nothing, nothing, nothing);
		matrix.forward.i = matrix.left.j = matrix.up.k = nanf("");
		ragdoll_pose_from_matrix(&matrix, &pose);
		CHECK(quaternion_is(&pose.rotation, 0.0f, 0.0f, 0.0f, 1.0f), "a broken matrix: %f %f %f %f",
			pose.rotation.v.i, pose.rotation.v.j, pose.rotation.v.k, pose.rotation.w);
		return 0;
	}
	CASE("unused at first")
	{
		CHECK(!ragdolls_of_object(0) && !ragdolls_get(1), "no ragdoll of object 0 in a zeroed table");
		CHECK(ragdolls_action(1) == _ragdoll_action_none, "nothing to do");
		return 0;
	}
	CASE("start")
	{
		struct ragdoll_death death = death_of(0);
		long replaced = 0;
		long id = ragdolls_start(&death, &replaced);

		CHECK(id == 1 && replaced == NONE, "the first: %ld %ld", id, replaced);
		CHECK(ragdolls_get(1) && ragdolls_get(1)->object_index == 0 && ragdolls_get(1)->body == &body, "its biped");
		CHECK(ragdolls_get(1)->start_time == now && !ragdolls_get(1)->limp && !ragdolls_get(1)->posed, "new");
		CHECK(ragdolls_of_object(0) == ragdolls_get(1), "found by its object");
		CHECK(ragdolls_start(&death, &replaced) == NONE, "one a biped");
		CHECK(!ragdolls_get(0) && !ragdolls_get(RAGDOLLS_MAX + 1) && !ragdolls_get(2), "ids out of range or free");
		for (index = 1; index < RAGDOLLS_MAX; index++)
		{
			now++;
			death = death_of(3 + index);
			id = ragdolls_start(&death, &replaced);
			CHECK(id == index + 1 && replaced == NONE, "ragdoll %ld: %ld", index, id);
		}
		/* full: the oldest, the first, goes */
		now++;
		death = death_of(100);
		id = ragdolls_start(&death, &replaced);
		CHECK(id == 1 && replaced == 1, "the oldest replaced: %ld %ld", id, replaced);
		CHECK(ragdolls_get(1)->object_index == 100 && !ragdolls_of_object(0), "by the newest");
		now++;
		death = death_of(101);
		id = ragdolls_start(&death, &replaced);
		CHECK(id == 2 && replaced == 2, "then the next oldest: %ld %ld", id, replaced);
		ragdolls_dispose_from_old_map();
		CHECK(!ragdolls_get(1) && !ragdolls_of_object(100), "gone with the map");
		return 0;
	}
	CASE("drive then limp")
	{
		struct ragdoll_death death = death_of(0);
		long replaced;
		long id = ragdolls_start(&death, &replaced);
		long tick;

		for (tick = 0; tick < RAGDOLL_DRIVE_TICKS; tick++, now++)
			CHECK(ragdolls_action(id) == _ragdoll_action_drive, "driven at tick %ld", tick);
		CHECK(ragdolls_action(id) == _ragdoll_action_limp, "then let go");
		CHECK(ragdolls_get(id)->limp, "limp");
		for (tick = 0; tick < 3; tick++, now++)
			CHECK(ragdolls_action(id) == _ragdoll_action_none, "once");
		return 0;
	}
	CASE("removed")
	{
		struct ragdoll_death death = death_of(1);
		long replaced;
		long id;

		/* (a vehicle's index: no longer a biped) */
		id = ragdolls_start(&death, &replaced);
		CHECK(ragdolls_action(id) == _ragdoll_action_remove, "not a biped");
		death = death_of(2);
		id = ragdolls_start(&death, &replaced);
		CHECK(ragdolls_action(id) == _ragdoll_action_drive, "a biped");
		deleted[2] = TRUE;
		CHECK(ragdolls_action(id) == _ragdoll_action_remove, "deleted");
		ragdolls_stop(id);
		CHECK(!ragdolls_get(id) && !ragdolls_of_object(2), "stopped");
		CHECK(ragdolls_action(id) == _ragdoll_action_none, "and done");
		return 0;
	}
	CASE("nearest part")
	{
		struct ragdoll_pose poses[3];
		real_point3d point = {1.0f, 0.6f, 0.0f};

		memset(poses, 0, sizeof(poses));
		poses[0].rotation.w = poses[2].rotation.w = 1.0f;
		/* the middle part turned a quarter about z: its shape at (1, 0.5) */
		poses[1].position.x = 1.0f;
		poses[1].rotation.v.k = poses[1].rotation.w = 0.70710678f;
		poses[2].position.x = poses[2].position.y = 1.0f;
		CHECK(ragdoll_part_nearest(&body, poses, &point) == 1, "the turned shape: %d",
			ragdoll_part_nearest(&body, poses, &point));
		point.y = 1.0f;
		CHECK(ragdoll_part_nearest(&body, poses, &point) == 2, "the last");
		point.x = point.y = -1.0f;
		CHECK(ragdoll_part_nearest(&body, poses, &point) == 0, "the first");
		return 0;
	}
	CASE("corpse push")
	{
		struct ragdoll_death death = death_of(0);
		struct ragdoll_impulse const *impulse;
		long replaced;
		real length = sqrtf(1.0f + RAGDOLL_PUSH_LIFT * RAGDOLL_PUSH_LIFT);

		ragdolls_start(&death, &replaced);
		CHECK(ragdolls_impulse_count() == 0, "no hit, no push");
		/* at (1, 2, 3): nearest the middle part's shape, at (1.5, 0, 0) */
		damage = blast(0.0f, 1.0f, 0.0f, 0.1f);
		ragdolls_damage(0, &damage);
		impulse = ragdolls_impulse_get(0);
		CHECK(ragdolls_impulse_count() == 1 && impulse, "pushed: %ld", ragdolls_impulse_count());
		CHECK(impulse->id == 1 && impulse->part == 1, "its part: %ld %d", impulse->id, impulse->part);
		/* lifted, at the hit's speed times the body's mass, all of it */
		CHECK(near(impulse->impulse.i, 0.0f) && near(impulse->impulse.j, 480.0f / length) &&
			near(impulse->impulse.k, 480.0f * RAGDOLL_PUSH_LIFT / length), "its impulse: %f %f %f",
			impulse->impulse.i, impulse->impulse.j, impulse->impulse.k);
		CHECK(impulse->point.x == 1.0f && impulse->point.z == 3.0f, "where it struck");
		ragdolls_damage(2, &damage);
		CHECK(ragdolls_impulse_count() == 1, "not another biped's");
		ragdolls_impulses_clear();
		CHECK(ragdolls_impulse_count() == 0 && !ragdolls_impulse_get(0), "cleared");
		return 0;
	}
	CASE("killing push")
	{
		struct ragdoll_death const *death;
		struct ragdoll_impulse const *impulse;
		long replaced;

		damage = blast(0.0f, 0.0f, -1.0f, 1.0f);
		ragdolls_damage(0, &damage);
		ragdolls_unit_died(0);
		death = ragdolls_death_get(0);
		ragdolls_start(death, &replaced);
		CHECK(ragdolls_get(1)->poses[2].position.x == 2.0f && !ragdolls_get(1)->posed, "posed as it died");
		impulse = ragdolls_impulse_get(0);
		CHECK(ragdolls_impulse_count() == 1 && impulse->id == 1, "pushed as it starts");
		/* (down, lifted: (0, 0, -0.55)) half the push: the living biped took
		the rest */
		CHECK(near(impulse->impulse.k, -240.0f) && near(impulse->impulse.i, 0.0f), "half: %f", impulse->impulse.k);
		return 0;
	}
	CASE("pushes dropped")
	{
		struct ragdoll_death death = death_of(0);
		long replaced;
		long id = ragdolls_start(&death, &replaced);

		damage = blast(1.0f, 0.0f, 0.0f, 1.0f);
		ragdolls_damage(0, &damage);
		ragdolls_stop(id);
		CHECK(ragdolls_impulse_count() == 0, "with its ragdoll: %ld", ragdolls_impulse_count());
		id = ragdolls_start(&death, &replaced);
		for (index = 0; index < RAGDOLL_IMPULSES_MAX + 4; index++)
			ragdolls_damage(0, &damage);
		CHECK(ragdolls_impulse_count() == RAGDOLL_IMPULSES_MAX, "full: %ld", ragdolls_impulse_count());
		ragdolls_impulses_clear();
		damage = blast(0.0f, 0.0f, 0.0f, 1.0f);
		ragdolls_damage(0, &damage);
		CHECK(ragdolls_impulse_count() == 0, "no push without a speed");
		return 0;
	}
	CASE("replaced drops")
	{
		struct ragdoll_death death;
		long replaced;

		for (index = 0; index < RAGDOLLS_MAX; index++, now++)
		{
			death = death_of(3 + index);
			ragdolls_start(&death, &replaced);
		}
		damage = blast(1.0f, 0.0f, 0.0f, 1.0f);
		ragdolls_damage(3, &damage);
		CHECK(ragdolls_impulse_count() == 1 && ragdolls_impulse_get(0)->id == 1, "the oldest pushed");
		death = death_of(100);
		CHECK(ragdolls_start(&death, &replaced) == 1 && replaced == 1, "then replaced");
		CHECK(ragdolls_impulse_count() == 0, "its push dropped: %ld", ragdolls_impulse_count());
		return 0;
	}
	CASE("near")
	{
		struct ragdoll_death death = death_of(0);
		real_point3d point = {3.5f, 0.0f, 0.0f};
		long replaced;

		CHECK(!ragdolls_near(&point, 1.0f), "no ragdolls");
		ragdolls_start(&death, &replaced);
		CHECK(ragdolls_near(&point, 1.0f), "within reach of its first part");
		point.x = 4.5f;
		CHECK(!ragdolls_near(&point, 1.0f), "past it");
		CHECK(ragdolls_near(&point, 2.0f), "a bigger object");
		return 0;
	}
	CASE("stand-ins")
	{
		boolean added;

		ragdolls_stand_ins_begin();
		CHECK(ragdolls_stand_in(50, &added) == 1 && added, "the first, new");
		CHECK(ragdolls_stand_in(51, &added) == 2 && added, "the second");
		CHECK(ragdolls_stand_in(50, &added) == 1 && !added, "the first again, this tick");
		CHECK(!ragdolls_stand_in_unseen(1) && !ragdolls_stand_in_unseen(2), "both seen");
		ragdolls_stand_ins_begin();
		CHECK(ragdolls_stand_in(50, &added) == 1 && !added, "next tick, moved");
		CHECK(ragdolls_stand_in_unseen(2), "the second gone");
		CHECK(!ragdolls_stand_in_unseen(2) && !ragdolls_stand_in_unseen(0) &&
			!ragdolls_stand_in_unseen(RAGDOLL_STAND_INS_MAX + 1), "once, and in range");
		CHECK(ragdolls_stand_in(52, &added) == 2 && added, "its id taken again");
		for (index = 2; index < RAGDOLL_STAND_INS_MAX; index++)
			ragdolls_stand_in(60 + index, &added);
		CHECK(ragdolls_stand_in(99, &added) == NONE && !added, "full");
		ragdolls_dispose_from_old_map();
		CHECK(ragdolls_stand_in(50, &added) == 1 && added, "gone with the map");
		return 0;
	}
	CASE("drawn")
	{
		struct ragdoll_death death = death_of(0);
		struct ragdoll *ragdoll;
		real_matrix4x3 const *drawn;
		real_matrix4x3 game[RAGDOLL_BODY_NODES_MAX];
		long replaced;

		/* nodes 3 and 4 carried with parts 1 and 2; the game's at a scale */
		body.node_parts[1] = 1; body.node_parts[2] = 2; body.node_parts[3] = 1; body.node_parts[4] = 2;
		for (index = 0; index < RAGDOLL_BODY_NODES_MAX; index++)
			matrices[index].scale = 1.5f;
		memcpy(game, matrices, sizeof(game));
		ragdolls_start(&death, &replaced);
		ragdoll = ragdolls_get(1);
		/* each part a quarter turn about up (the usual sense) and moved */
		for (index = 0; index < 3; index++)
		{
			ragdoll->poses[index].position.x = 10.0f + index;
			ragdoll->poses[index].position.y = 20.0f;
			ragdoll->poses[index].position.z = 30.0f;
			ragdoll->poses[index].rotation.v.i = ragdoll->poses[index].rotation.v.j = 0.0f;
			ragdoll->poses[index].rotation.v.k = ragdoll->poses[index].rotation.w = sqrtf(0.5f);
		}
		ragdoll->posed = TRUE;
		drawn = ragdolls_node_matrices(0, matrices);
		CHECK(drawn != matrices, "drawn from the ragdoll");
		CHECK(near(drawn[1].forward.i, 0.0f) && near(drawn[1].forward.j, 1.0f) && near(drawn[1].left.i, -1.0f) &&
			near(drawn[1].up.k, 1.0f), "a part's node turned as its pose: forward %f %f", drawn[1].forward.i,
			drawn[1].forward.j);
		CHECK(near(drawn[1].position.x, 11.0f) && near(drawn[1].position.y, 20.0f) && near(drawn[1].scale, 1.5f),
			"where its pose is, at the game's scale: %f %f %f", drawn[1].position.x, drawn[1].position.y,
			drawn[1].scale);
		CHECK(near(drawn[3].position.x, 11.0f) && near(drawn[3].position.y, 22.0f) && near(drawn[3].position.z, 30.0f),
			"a node carried with its part, two units on: %f %f %f", drawn[3].position.x, drawn[3].position.y,
			drawn[3].position.z);
		CHECK(near(drawn[3].forward.j, 1.0f) && near(drawn[3].scale, 1.5f), "turned with it: %f %f",
			drawn[3].forward.j, drawn[3].scale);
		CHECK(near(drawn[4].position.x, 12.0f) && near(drawn[4].position.y, 22.0f), "and the other's: %f %f",
			drawn[4].position.x, drawn[4].position.y);
		CHECK(memcmp(game, matrices, sizeof(game)) == 0, "the game's own untouched");
		CHECK(ragdolls_node_matrices(1, matrices) == matrices, "another object as the game has it");
		interpolating = TRUE;
		CHECK(ragdolls_node_matrices(0, matrices) == matrices, "a frame between ticks as interpolation has it");
		return 0;
	}
	CASE("drawn unposed")
	{
		struct ragdoll_death death = death_of(0);
		long replaced;

		CHECK(ragdolls_node_matrices(0, matrices) == matrices, "no ragdoll");
		ragdolls_start(&death, &replaced);
		CHECK(ragdolls_node_matrices(0, matrices) == matrices, "not yet posed by the host");
		ragdolls_get(1)->posed = TRUE;
		CHECK(ragdolls_node_matrices(0, matrices) != matrices, "posed");
		ragdolls_stop(1);
		CHECK(ragdolls_node_matrices(0, matrices) == matrices, "stopped");
		ragdolls_start(&death, &replaced);
		ragdolls_get(1)->posed = TRUE;
		ragdolls_dispose_from_old_map();
		CHECK(ragdolls_node_matrices(0, matrices) == matrices, "gone with the map, or a saved game");
		return 0;
	}
	CASE("blast's damage")
	{
		struct ragdoll_hit const *hit;

		damage = explosion(2.0f, 0.0f, 0.0f);
		damage.multiplier = 2.0f;
		ragdolls_damage(0, &damage);
		hit = ragdolls_hit_get(0);
		CHECK(hit->explosion && hit->scaled && hit->epicenter.x == 2.0f, "an explosion's, at its epicenter");
		CHECK(near(hit->damage_minimum, 20.0f) && near(hit->damage_maximum, 240.0f), "its damage, multiplied: %f %f",
			hit->damage_minimum, hit->damage_maximum);
		CHECK(hit->falloff_radius == 1.0f && hit->cutoff_radius == 3.0f, "its reach");
		grenade.flags = 1;
		ragdolls_damage(0, &damage);
		CHECK(!ragdolls_hit_get(0)->scaled, "not scaled by distance");
		damage = blast(1.0f, 0.0f, 0.0f, 1.0f);
		ragdolls_damage(0, &damage);
		CHECK(!ragdolls_hit_get(0)->explosion, "a bullet's");
		damage = explosion(2.0f, 0.0f, 0.0f);
		grenade.damage.category = 6;
		ragdolls_damage(0, &damage);
		CHECK(!ragdolls_hit_get(0)->explosion, "a melee's, though over an area");
		grenade.damage.category = 4;
		ragdolls_damage(0, &damage);
		CHECK(ragdolls_hit_get(0)->explosion, "a high explosive's");
		grenade.damage.category = 8;
		ragdolls_damage(0, &damage);
		CHECK(ragdolls_hit_get(0)->explosion, "a mounted weapon's");
		return 0;
	}
	CASE("falloff")
	{
		struct ragdoll_hit hit;
		real_point3d point = {0.0f, 0.0f, 0.0f};

		memset(&hit, 0, sizeof(hit));
		hit.explosion = hit.scaled = TRUE;
		hit.damage_minimum = 10.0f;
		hit.damage_maximum = 120.0f;
		hit.falloff_radius = 1.0f;
		hit.cutoff_radius = 3.0f;
		CHECK(near(ragdoll_hit_damage_at(&hit, &point), 120.0f), "at its heart");
		point.y = 1.0f;
		CHECK(near(ragdoll_hit_damage_at(&hit, &point), 120.0f), "out to the falloff");
		CHECK(near(ragdoll_hit_tearing_at(&hit, &point), 80.0f), "tearing less than it hurts: %f",
			ragdoll_hit_tearing_at(&hit, &point));
		point.y = 2.0f;
		CHECK(near(ragdoll_hit_damage_at(&hit, &point), 65.0f), "half way to the cutoff: %f",
			ragdoll_hit_damage_at(&hit, &point));
		point.y = 3.0f;
		CHECK(ragdoll_hit_damage_at(&hit, &point) == 0.0f, "at the cutoff");
		point.y = 4.0f;
		CHECK(ragdoll_hit_damage_at(&hit, &point) == 0.0f, "past it");
		point.y = 2.0f;
		hit.scaled = FALSE;
		CHECK(near(ragdoll_hit_damage_at(&hit, &point), 120.0f), "unscaled within reach");
		hit.scaled = TRUE;
		hit.cutoff_radius = 1.0f;
		point.y = 9.0f;
		CHECK(near(ragdoll_hit_damage_at(&hit, &point), 120.0f), "no falloff");
		hit.explosion = FALSE;
		point.y = 0.0f;
		CHECK(ragdoll_hit_damage_at(&hit, &point) == 0.0f && ragdoll_hit_tearing_at(&hit, &point) == 0.0f,
			"no explosion");
		return 0;
	}
	CASE("pieces")
	{
		struct ragdoll_death death = death_of(0);
		struct ragdoll *ragdoll;
		long replaced;

		/* the first part's joint torn: it and the middle one beneath it a
		piece of their own; then the middle's too, a third piece */
		breakable();
		ragdoll = ragdolls_get(ragdolls_start(&death, &replaced));
		for (index = 0; index < 4; index++)
			CHECK(ragdoll_part_piece(ragdoll, (short)index) == 0, "whole: part %ld", index);
		ragdoll->broken_joints = 1UL << 1;
		CHECK(ragdoll_part_piece(ragdoll, 0) == 0 && ragdoll_part_piece(ragdoll, 3) == 0, "the body");
		CHECK(ragdoll_part_piece(ragdoll, 1) == 1 && ragdoll_part_piece(ragdoll, 2) == 1, "the limb");
		ragdoll->broken_joints |= 1UL << 2;
		CHECK(ragdoll_part_piece(ragdoll, 1) == 1 && ragdoll_part_piece(ragdoll, 2) == 2, "torn in two");
		CHECK(ragdoll_part_piece(ragdoll, 4) == NONE && ragdoll_part_piece(ragdoll, -1) == NONE, "no part");
		return 0;
	}
	CASE("chance")
	{
		real chance = ragdoll_break_chance(0, 1);

		CHECK(chance >= 0.75f && chance <= 1.25f, "within a quarter: %f", chance);
		CHECK(chance == ragdoll_break_chance(0, 1), "the same each time");
		CHECK(chance != ragdoll_break_chance(0, 2) && chance != ragdoll_break_chance(5, 1), "a body's and a part's own");
		for (index = 0; index < 1000; index++)
		{
			chance = ragdoll_break_chance(index, (short)(index % 7));
			CHECK(chance >= 0.75f && chance <= 1.25f, "within a quarter: %f", chance);
		}
		return 0;
	}
	CASE("torn by a blast")
	{
		struct ragdoll_death const *death;
		long replaced;

		/* at the last part: all three within the falloff, hurt 120 each,
		torn 100, 120 and 80 as they are near it; the last part's share of its
		threshold the most, then the middle's */
		breakable();
		damage = explosion(2.0f, 0.0f, 0.0f);
		ragdolls_damage(0, &damage);
		CHECK(ragdolls_break_count() == 0, "none for a living biped");
		ragdolls_unit_died(0);
		death = ragdolls_death_get(0);
		CHECK(ragdolls_start(death, &replaced) == 1, "a ragdoll");
		CHECK(ragdolls_break_count() == RAGDOLL_BREAKS_PER_HIT, "two of three: %ld", ragdolls_break_count());
		CHECK(ragdolls_break_get(0)->id == 1 && ragdolls_break_get(0)->part == 2, "the worst hurt first: %d",
			ragdolls_break_get(0)->part);
		CHECK(ragdolls_break_get(1)->id == 1 && ragdolls_break_get(1)->part == 1, "then the next: %d",
			ragdolls_break_get(1)->part);
		CHECK(ragdolls_impulse_count() == 1, "and pushed");
		ragdolls_breaks_clear();
		CHECK(ragdolls_break_count() == 0 && !ragdolls_break_get(0), "cleared");
		return 0;
	}
	CASE("bleeds")
	{
		struct ragdoll_death const *death;
		long replaced;

		/* as "torn by a blast": the last part, at the blast, bleeds up; then the
		middle, a unit from it, away from it */
		breakable();
		damage = explosion(2.0f, 0.0f, 0.0f);
		ragdolls_damage(0, &damage);
		CHECK(bled_count == 0, "none while it lives");
		ragdolls_unit_died(0);
		death = ragdolls_death_get(0);
		ragdolls_start(death, &replaced);
		CHECK(bled_count == 2, "a joint's blood for each torn: %ld", bled_count);
		CHECK(bled_effect == 77 && bled_owner == 0, "the biped's own: %ld %ld", bled_effect, bled_owner);
		CHECK(near(bled_at.x, 1.0f) && near(bled_at.y, 0.0f) && near(bled_at.z, 0.0f), "at the joint: %f %f %f",
			bled_at.x, bled_at.y, bled_at.z);
		CHECK(near(bled_normal.i, -1.0f) && near(bled_normal.j, 0.0f) && near(bled_normal.k, 0.0f),
			"away from the blast: %f %f %f", bled_normal.i, bled_normal.j, bled_normal.k);
		CHECK(near(bled_gravity.k, -1.0f) && strcmp(bled_names[4], "gravity") == 0 &&
			strcmp(bled_names[0], "normal") == 0, "its markers named as damage.c's");
		return 0;
	}
	CASE("bloodless")
	{
		struct ragdoll_death death = death_of(0);
		long replaced;

		/* torn, but its collision model has no blood */
		breakable();
		flesh.resistance.localized_damage_effect.index = -1;
		ragdolls_start(&death, &replaced);
		damage = explosion(2.0f, 0.0f, 0.0f);
		ragdolls_damage(0, &damage);
		CHECK(ragdolls_break_count() == 2 && bled_count == 0, "torn without blood: %ld %ld", ragdolls_break_count(),
			bled_count);
		return 0;
	}
	CASE("piece draws")
	{
		struct ragdoll_death death = death_of(0);
		struct ragdoll *ragdoll;
		real_matrix4x3 drawn[RAGDOLL_BODY_NODES_MAX];
		real_matrix4x3 const *piece;
		long replaced;

		/* a body of four parts (0, its child 1 and grandchild 2 along x, and
		3 off 0), node 4 moving with part 2; drawn ten units on from where the
		game has it, each node at its index */
		breakable();
		body.node_parts[0] = 0; body.node_parts[1] = 1; body.node_parts[2] = 2; body.node_parts[3] = 3;
		body.node_parts[4] = 2;
		memcpy(drawn, matrices, sizeof(drawn));
		for (index = 0; index < 5; index++)
			drawn[index].position.x += 10.0f;
		CHECK(ragdolls_piece_count(0) == 1 && ragdolls_piece_node_matrices(0, drawn, 0) == drawn &&
			!ragdolls_piece_node_matrices(0, drawn, 1), "no ragdoll: drawn whole");
		ragdoll = ragdolls_get(ragdolls_start(&death, &replaced));
		ragdoll->posed = TRUE;
		CHECK(ragdolls_piece_count(0) == 1 && ragdolls_piece_node_matrices(0, drawn, 0) == drawn &&
			!ragdolls_piece_node_matrices(0, drawn, 1) && !ragdolls_piece_node_matrices(0, drawn, -1),
			"untorn: drawn whole");
		/* the middle's joint torn */
		ragdoll->broken_joints = 1UL << 1;
		CHECK(ragdolls_piece_count(0) == 2 && !ragdolls_piece_node_matrices(0, drawn, 2), "two pieces");
		piece = ragdolls_piece_node_matrices(0, drawn, 0);
		CHECK(piece != drawn && memcmp(&piece[0], &drawn[0], sizeof(*piece)) == 0 &&
			memcmp(&piece[3], &drawn[3], sizeof(*piece)) == 0, "the body's own nodes as drawn");
		for (index = 1; index < 5; index += index == 2 ? 2 : 1)
		{
			CHECK(piece[index].scale == 0.0f && near(piece[index].position.x, 11.0f) && piece[index].forward.i == 1.0f,
				"the limb's node %ld at the body's stump: %f at %f", index, piece[index].scale,
				piece[index].position.x);
		}
		piece = ragdolls_piece_node_matrices(0, drawn, 1);
		CHECK(memcmp(&piece[1], &drawn[1], sizeof(*piece)) == 0 && memcmp(&piece[2], &drawn[2], sizeof(*piece)) == 0 &&
			memcmp(&piece[4], &drawn[4], sizeof(*piece)) == 0, "the limb's own nodes as drawn");
		CHECK(piece[0].scale == 0.0f && near(piece[0].position.x, 11.0f) && piece[3].scale == 0.0f &&
			near(piece[3].position.x, 11.0f), "the body's at the limb's joint: %f %f", piece[0].position.x,
			piece[3].position.x);
		/* and the last part's from it: three pieces, each stump its own */
		ragdoll->broken_joints |= 1UL << 2;
		CHECK(ragdolls_piece_count(0) == 3, "three pieces");
		piece = ragdolls_piece_node_matrices(0, drawn, 0);
		CHECK(piece[2].scale == 0.0f && near(piece[2].position.x, 11.0f) && near(piece[4].position.x, 11.0f),
			"the body's stump for both beneath it: %f %f", piece[2].position.x, piece[4].position.x);
		piece = ragdolls_piece_node_matrices(0, drawn, 1);
		CHECK(piece[1].scale == drawn[1].scale && piece[2].scale == 0.0f && near(piece[2].position.x, 12.0f) &&
			near(piece[4].position.x, 12.0f), "the middle's stump for the last: %f", piece[2].position.x);
		CHECK(piece[0].scale == 0.0f && near(piece[0].position.x, 11.0f), "the body at the middle's joint: %f",
			piece[0].position.x);
		piece = ragdolls_piece_node_matrices(0, drawn, 2);
		CHECK(piece[2].scale == drawn[2].scale && near(piece[1].position.x, 12.0f) && near(piece[0].position.x, 12.0f) &&
			piece[1].scale == 0.0f && piece[4].scale == drawn[4].scale, "the last's: the rest at its joint");
		CHECK(!ragdolls_piece_node_matrices(0, drawn, 3), "no fourth");
		/* while the frame is drawn, the stumps still from the game's pose, not
		from the drawn one, where the limbs have flown */
		{
			real_matrix4x3 flung[RAGDOLL_BODY_NODES_MAX];

			memcpy(flung, drawn, sizeof(flung));
			for (index = 1; index < 5; index++)
				flung[index].position.y += 100.0f;
			interpolated = flung;
			piece = ragdolls_piece_node_matrices(0, drawn, 0);
			interpolated = NULL;
			CHECK(near(piece[2].position.x, 11.0f) && near(piece[2].position.y, drawn[1].position.y),
				"the stump as the game has it while drawn: %f %f", piece[2].position.x, piece[2].position.y);
		}
		return 0;
	}
	CASE("past their thresholds")
	{
		struct ragdoll_death death = death_of(0);
		long replaced;

		/* the middle's 50 torn, the last's unbreakable joint and the
		fourth's 500 not */
		breakable();
		body.parts[2].break_damage = 0.0f;
		body.parts[3].break_damage = 500.0f;
		ragdolls_start(&death, &replaced);
		damage = explosion(2.0f, 0.0f, 0.0f);
		ragdolls_damage(0, &damage);
		CHECK(ragdolls_break_count() == 1 && ragdolls_break_get(0)->part == 1, "the middle alone: %ld",
			ragdolls_break_count());
		return 0;
	}
	CASE("by chance")
	{
		struct ragdoll_death death = death_of(0);
		long replaced;

		/* the first part torn 100, past 96 but not past its share of it,
		this body's chance for it being 1.059 */
		breakable();
		body.parts[1].break_damage = 96.0f;
		body.parts[2].break_damage = 0.0f;
		body.parts[3].break_damage = 500.0f;
		ragdolls_start(&death, &replaced);
		damage = explosion(2.0f, 0.0f, 0.0f);
		ragdolls_damage(0, &damage);
		CHECK(ragdolls_break_count() == 0, "held by its chance: %ld", ragdolls_break_count());
		body.parts[1].break_damage = 90.0f;
		ragdolls_damage(0, &damage);
		CHECK(ragdolls_break_count() == 1 && ragdolls_break_get(0)->part == 1, "torn under it: %ld",
			ragdolls_break_count());
		return 0;
	}
	CASE("bullets tear none")
	{
		struct ragdoll_death death = death_of(0);
		long replaced;

		breakable();
		ragdolls_start(&death, &replaced);
		damage = blast(0.0f, 0.0f, 1.0f, 1.0f);
		damage.origin.x = 2.0f; damage.origin.y = damage.origin.z = 0.0f;
		damage.epicenter = damage.origin;
		ragdolls_damage(0, &damage);
		CHECK(ragdolls_impulse_count() == 1 && ragdolls_break_count() == 0, "pushed, not torn: %ld",
			ragdolls_break_count());
		return 0;
	}
	CASE("tear off")
	{
		struct ragdoll_death death = death_of(0);
		long replaced;

		breakable();
		dismember = FALSE;
		ragdolls_start(&death, &replaced);
		damage = explosion(2.0f, 0.0f, 0.0f);
		ragdolls_damage(0, &damage);
		CHECK(ragdolls_impulse_count() == 1 && ragdolls_break_count() == 0, "whole with game.dismemberment off");
		return 0;
	}
	CASE("corpse torn")
	{
		struct ragdoll_death death = death_of(0);
		long replaced;

		breakable();
		ragdolls_start(&death, &replaced);
		CHECK(ragdolls_break_count() == 0, "no hit, no tear");
		/* a unit past the fourth part: it alone, the others too far */
		damage = explosion(4.0f, 0.0f, 0.0f);
		ragdolls_damage(0, &damage);
		CHECK(ragdolls_break_count() == 1 && ragdolls_break_get(0)->part == 3, "torn by a later blast: %ld %d",
			ragdolls_break_count(), ragdolls_break_get(0) ? ragdolls_break_get(0)->part : NONE);
		ragdolls_breaks_clear();
		damage = explosion(-10.0f, 0.0f, 0.0f);
		ragdolls_damage(0, &damage);
		CHECK(ragdolls_break_count() == 0, "out of its reach");
		ragdolls_damage(2, &damage);
		CHECK(ragdolls_break_count() == 0, "not another biped's");
		return 0;
	}
	CASE("torn once")
	{
		struct ragdoll_death death = death_of(0);
		long replaced;

		breakable();
		body.parts[3].break_damage = 0.0f;
		ragdolls_start(&death, &replaced);
		damage = explosion(2.0f, 0.0f, 0.0f);
		ragdolls_damage(0, &damage);
		CHECK(ragdolls_break_count() == 2, "both: %ld", ragdolls_break_count());
		ragdolls_damage(0, &damage);
		CHECK(ragdolls_break_count() == 2, "not again while queued: %ld", ragdolls_break_count());
		/* the host's: the middle's joint torn, so the last part is on a limb
		gone already */
		ragdolls_breaks_clear();
		ragdolls_get(1)->broken_joints = 1UL << 1;
		ragdolls_damage(0, &damage);
		CHECK(ragdolls_break_count() == 0, "nor on a torn limb: %ld", ragdolls_break_count());
		return 0;
	}
	CASE("breaks dropped")
	{
		struct ragdoll_death death = death_of(0);
		long replaced;
		long id;

		breakable();
		id = ragdolls_start(&death, &replaced);
		damage = explosion(2.0f, 0.0f, 0.0f);
		ragdolls_damage(0, &damage);
		ragdolls_stop(id);
		CHECK(ragdolls_break_count() == 0, "with its ragdoll: %ld", ragdolls_break_count());
		/* two a ragdoll, more than there is room for */
		for (index = 0; index < RAGDOLL_BREAKS_MAX / 2 + 4; index++)
		{
			death = death_of(3 + index);
			ragdolls_start(&death, &replaced);
			ragdolls_damage(3 + index, &damage);
		}
		CHECK(ragdolls_break_count() == RAGDOLL_BREAKS_MAX, "full: %ld", ragdolls_break_count());
		return 0;
	}
	fprintf(stderr, "unknown case '%s'\n", case_name);
	return 2;
}
