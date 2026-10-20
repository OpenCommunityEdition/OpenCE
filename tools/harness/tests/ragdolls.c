#include "harness.h"
#include <math.h>

/* Deaths and hits for the ragdolls (ragdolls.c), in a fake world of three
objects: 0 a biped with a body, 1 a vehicle with a model that would make
one, 2 a biped whose model has no body, and from 3 on more of the first. A hit is remembered for its tick; a biped's death queues its node
matrices, velocity and the hit that killed it that tick. */
typedef struct { real scale; real_vector3d forward, left, up; real_point3d position; } real_matrix4x3;
struct tag_block { long count; void *address; long definition; };
#define _real_epsilon 0.0001f
#define CEILING(n, ceiling) ((n) > (ceiling) ? (ceiling) : (n))
#define PIN(n, floor, ceiling) ((n) < (floor) ? (floor) : CEILING((n), (ceiling)))
enum { _object_type_biped = 0, _object_type_vehicle = 1 };
struct _object_datum { short type; };
struct object_datum { long definition_index; struct _object_datum object; };
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

static boolean physics_jolt_ragdolls_enabled(void) { return enabled; }
static long game_time_get(void) { return now; }
static struct object_datum *object_get(long index) { return &objects[index < 3 ? index : 0]; }
static real_matrix4x3 *object_get_node_matrices(long index) { (void)index; return matrices; }
static void object_get_velocities(long index, real_vector3d *velocity, real_vector3d *angular)
{
	velocity->i = 1.0f + index; velocity->j = 2.0f; velocity->k = -3.0f;
	angular->i = angular->j = angular->k = 0.0f;
}
static struct damage_effect_definition *damage_effect_definition_get(long index) { (void)index; return &grenade; }
static struct ragdoll_body const *ragdoll_body_get(long definition_index) { return definition_index != 102 ? &body : NULL; }
static real magnitude3d(real_vector3d const *v) { return sqrtf(v->i * v->i + v->j * v->j + v->k * v->k); }
static real_vector3d *scale_vector3d(real_vector3d const *v, real s, real_vector3d *out)
{
	out->i = v->i * s; out->j = v->j * s; out->k = v->k * s; return out;
}

#include "under_test.inc"

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

	body.node_count = 5;
	for (index = 0; index < RAGDOLL_BODY_NODES_MAX; index++)
		matrices[index].position.x = (real)index;

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
		/* 6 * (0.25 + 0.75 * 0.5) */
		CHECK(near(death->killing_hit.speed, 3.75f), "its speed: %f", death->killing_hit.speed);
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
		CHECK(near(ragdolls_hit_get(0)->speed, 1.5f), "at zero scale: %f", ragdolls_hit_get(0)->speed);
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
		damage = blast(0.0f, 0.0f, 1.0f, NAN);
		ragdolls_damage(0, &damage);
		CHECK(ragdolls_hit_get(0)->speed == 0.0f, "a nan scale: %f", ragdolls_hit_get(0)->speed);
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
	fprintf(stderr, "unknown case '%s'\n", case_name);
	return 2;
}
