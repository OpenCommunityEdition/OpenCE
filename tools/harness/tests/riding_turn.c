/*
RIDING_TURN.C (test)

The rider's turn and the support it keeps, taken from the port files by
test_riding_turn.py: the eight vectors turned by one sine and cosine, and the
claimed support that asks nothing of a support that rides nothing.
*/

#include "harness.h"
#include <math.h>

#define VEHICLE 0x10000001
#define OTHER_VEHICLE 0x10000002

static long cos_calls, sin_calls, claim_calls;
#define cos(angle) (cos_calls++, (cos)(angle))
#define sin(angle) (sin_calls++, (sin)(angle))

struct biped_datum
{
	struct { real_vector3d forward; } object;
	struct
	{
		real_vector3d desired_facing_vector, desired_aiming_vector, aiming_vector, desired_looking_vector, looking_vector;
	} unit;
};
struct biped_physics
{
	real_vector3d forward, aiming;
	long riding_object_index;
	real minimum_normal_k;
};
struct collision_plane
{
	long object_index;
	struct { real_vector3d n; real d; } plane;
};

#include "under_test.inc"

static real_vector3d *field(struct biped_datum *biped, struct biped_physics *physics, short index)
{
	real_vector3d *all[8];

	all[0] = &biped->object.forward;
	all[1] = &biped->unit.desired_facing_vector;
	all[2] = &biped->unit.desired_aiming_vector;
	all[3] = &biped->unit.aiming_vector;
	all[4] = &biped->unit.desired_looking_vector;
	all[5] = &biped->unit.looking_vector;
	all[6] = &physics->forward;
	all[7] = &physics->aiming;
	return all[index];
}

static void collision(struct collision_plane *plane, long object_index, real k)
{
	memset(plane, 0, sizeof(*plane));
	plane->object_index = object_index;
	plane->plane.n.k = k;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	struct biped_datum biped;
	struct biped_physics physics;
	struct collision_plane planes[3];

	memset(&biped, 0, sizeof(biped));
	memset(&physics, 0, sizeof(physics));
	CASE("turn-eight-matches-one")
	{
		real const angles[6] = { 0.f, -0.f, 1e-7f, 0.3f, -2.9f, 3.14159265f };
		real_vector3d const samples[8] = {
			{ 1.f, 0.f, .5f }, { 0.f, -0.f, 0.f }, { -0.f, 0.f, -0.f }, { 3e38f, -3e38f, 1.f },
			{ .6f, .8f, 0.f }, { (real)NAN, .5f, .25f }, { 1e-30f, -1e-30f, 7.f }, { -.25f, 1.5f, -1.f } };
		short angle, index;

		for (angle = 0; angle < 6; angle++)
		{
			real_vector3d together[8], apart[8];
			real_vector3d *pointers[8];

			memcpy(together, samples, sizeof(together));
			memcpy(apart, samples, sizeof(apart));
			for (index = 0; index < 8; index++)
			{
				pointers[index] = &together[index];
				vehicle_riding_turn_vector(&apart[index], angles[angle]);
			}
			vehicle_riding_turn_vectors(pointers, 8, angles[angle]);
			CHECK(!memcmp(together, apart, sizeof(together)), "angle %d: the eight turned differ in bits", angle);
		}
		return 0;
	}
	CASE("turn-trig-once")
	{
		cos_calls = sin_calls = 0;
		biped_riding_turn(&biped, &physics, .3f);
		CHECK(cos_calls == 1 && sin_calls == 1, "%ld cos and %ld sin for one turn", cos_calls, sin_calls);
		return 0;
	}
	CASE("turn-all-eight")
	{
		short index;

		for (index = 0; index < 8; index++)
		{
			real_vector3d *vector = field(&biped, &physics, index);

			vector->i = 1.f;
			vector->j = 0.f;
			vector->k = .5f;
		}
		biped_riding_turn(&biped, &physics, .3f);
		for (index = 0; index < 8; index++)
		{
			real_vector3d *vector = field(&biped, &physics, index);

			CHECK(vector->i == (real)(cos)(.3) && vector->j == (real)(sin)(.3) && vector->k == .5f, "field %d not turned", index);
		}
		return 0;
	}
	CASE("claim-none-no-fill")
	{
		physics.riding_object_index = NONE;
		physics.minimum_normal_k = .7f;
		collision(&planes[0], OTHER_VEHICLE, 1.f);
		collision(&planes[1], VEHICLE, .95f);
		collision(&planes[2], NONE, 1.f);
		claim_calls = 0;
		CHECK(biped_riding_claimed_support(&physics, planes, 3, 0) == 0, "not riding: not the best");
		CHECK(biped_riding_claimed_support(&physics, planes, 3, NONE) == NONE, "not riding: no best kept");
		CHECK(claim_calls == 0, "%ld claims asked of a support that rides nothing", claim_calls);
		return 0;
	}
	CASE("claim-ridden-best")
	{
		long objects[3] = { OTHER_VEHICLE, VEHICLE, NONE };
		real normal_ks[3] = { 1.f, .95f, 1.f };

		physics.riding_object_index = VEHICLE;
		physics.minimum_normal_k = .7f;
		collision(&planes[0], OTHER_VEHICLE, 1.f);
		collision(&planes[1], VEHICLE, .95f);
		collision(&planes[2], NONE, 1.f);
		CHECK(biped_riding_claimed_support(&physics, planes, 3, 1) == 1, "the ridden best lost");
		CHECK(biped_riding_claimed_support(&physics, planes, 3, 1) ==
			(vehicle_riding_claim)(VEHICLE, 3, objects, normal_ks, .7f, 1), "the ridden best differs from the full claim");
		return 0;
	}
	CASE("claim-competing-decks")
	{
		long objects[3] = { OTHER_VEHICLE, VEHICLE, NONE };
		real normal_ks[3] = { 1.f, .95f, 1.f };
		short best;

		physics.riding_object_index = VEHICLE;
		physics.minimum_normal_k = .7f;
		collision(&planes[0], OTHER_VEHICLE, 1.f);
		collision(&planes[1], VEHICLE, .95f);
		collision(&planes[2], NONE, 1.f);
		for (best = NONE; best < 3; best++)
			CHECK(biped_riding_claimed_support(&physics, planes, 3, best) ==
				(vehicle_riding_claim)(VEHICLE, 3, objects, normal_ks, .7f, best), "best %d differs from the full claim", best);
		CHECK(biped_riding_claimed_support(&physics, planes, 3, 0) == 1, "the ridden deck lost its rider");
		planes[1].plane.n.k = .2f;
		CHECK(biped_riding_claimed_support(&physics, planes, 3, 0) == 0, "a wall kept the rider");
		return 0;
	}
	fprintf(stderr, "no such case: %s\n", case_name);
	return 2;
}
