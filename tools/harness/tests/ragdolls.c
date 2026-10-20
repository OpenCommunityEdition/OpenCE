#include "harness.h"
#include <math.h>
#include <string.h>

/* Deaths and hits for the ragdolls (ragdolls.c), in a fake world of three
objects: 0 a biped with a body, 1 a vehicle with a model that would make
one, 2 a biped whose model has no body, and from 3 on more of the first. A hit is remembered for its tick; a biped's death queues its node
matrices, velocity and the hit that killed it that tick. */
typedef struct { real scale; union { real n[4][3]; struct { real_vector3d forward, left, up; real_point3d position; }; }; }
	real_matrix4x3;
typedef struct { real_vector3d v; real w; } real_quaternion;
struct tag_block { long count; void *address; long definition; };
#define _real_epsilon 0.0001f
#define CEILING(n, ceiling) ((n) > (ceiling) ? (ceiling) : (n))
#define PIN(n, floor, ceiling) ((n) < (floor) ? (floor) : CEILING((n), (ceiling)))
enum { _object_type_biped = 0, _object_type_vehicle = 1 };
enum { _object_mask_biped = 1 << _object_type_biped };
struct _object_datum { short type; };
struct object_datum { long definition_index; struct _object_datum object; };
struct object_definition { struct { real acceleration_scale; } object; };
struct damage_definition { real instantaneous_acceleration, instantaneous_acceleration_zero_scale_factor; };
struct damage_effect_definition { struct damage_definition damage; };
struct damage_data { long definition_index; real_point3d origin; real_vector3d direction; real scale; };

#include "config.inc"

static boolean enabled = TRUE;
static long now = 10;
static struct object_datum objects[3] = {{100, {_object_type_biped}}, {101, {_object_type_vehicle}}, {102, {_object_type_biped}}};
static real_matrix4x3 matrices[RAGDOLL_BODY_NODES_MAX];
static struct ragdoll_body body;
static struct damage_effect_definition grenade = {{6.0f, 0.25f}};
static struct damage_effect_definition poke = {{0.0f, 0.0f}};
/* the bipeds': 100's, and 102's half */
static struct object_definition whole = {{1.0f}}, half = {{0.5f}};

static boolean deleted[3];
static short const matrix4x3_next[3] = {1, 2, 0};

static boolean physics_jolt_ragdolls_enabled(void) { return enabled; }
static real square_root(real x) { return sqrtf(x); }
static void *object_try_and_get_and_verify_type(long index, unsigned long mask)
{
	struct object_datum *object = &objects[index < 3 ? index : 0];

	return index >= 0 && !deleted[index < 3 ? index : 0] && (mask & (1u << object->object.type)) ? object : NULL;
}
static long game_time_get(void) { return now; }
static struct object_datum *object_get(long index) { return &objects[index < 3 ? index : 0]; }
static real_matrix4x3 *object_get_node_matrices(long index) { (void)index; return matrices; }
static void object_get_velocities(long index, real_vector3d *velocity, real_vector3d *angular)
{
	velocity->i = 1.0f + index; velocity->j = 2.0f; velocity->k = -3.0f;
	angular->i = angular->j = angular->k = 0.0f;
}
static struct damage_effect_definition *damage_effect_definition_get(long index) { return index == 8 ? &poke : &grenade; }
static struct object_definition *object_definition_get(long index) { return index == 102 ? &half : &whole; }
static struct ragdoll_body const *ragdoll_body_get(long definition_index) { return definition_index != 102 ? &body : NULL; }
static real magnitude3d(real_vector3d const *v) { return sqrtf(v->i * v->i + v->j * v->j + v->k * v->k); }
static real_vector3d *scale_vector3d(real_vector3d const *v, real s, real_vector3d *out)
{
	out->i = v->i * s; out->j = v->j * s; out->k = v->k * s; return out;
}

/* whether a frame between ticks is drawn now, from interpolation's snapshots */
static boolean interpolating;
static real_matrix4x3 *render_interpolation_object_node_matrices(long index) { (void)index; return interpolating ? matrices : NULL; }

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
	return damage;
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
	fprintf(stderr, "unknown case '%s'\n", case_name);
	return 2;
}
