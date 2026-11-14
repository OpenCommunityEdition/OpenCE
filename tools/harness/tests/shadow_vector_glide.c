/*
SHADOW_VECTOR_GLIDE.C (test)

The real shadow_vector_drawn (render_interpolation.c): an object's lighting
turns its shadow vector by a step (up to 0.012 a component) each time it is
refreshed, every tick or every 4 or 11, and holds in between; drawn as it
was, a dropship's shadow, cast from 15 world units up, hopped a third of a
world unit every 4 ticks while the dropship glided. The direction drawn
glides from step to step, as steadily as the steps come; a new object's, one
long unseen and a turn no step makes are drawn as the lighting has them.
test_shadow_vector_glide.py takes the code from the sources (under_test.inc).
*/

#include "harness.h"

#include <math.h>

#define FLOOR(n,floor) ((n)<(floor)?(floor):(n))
#define CEILING(n,ceiling) ((n)>(ceiling)?(ceiling):(n))
#define PIN(n,floor,ceiling) ((n)<(floor) ? (floor) : CEILING((n),(ceiling)))

#include "under_test.inc"

static unsigned long random_state = 12345;

static double random_unit(void)
{
	random_state = random_state * 1103515245UL + 12345UL;
	return (double)((random_state >> 8) & 0xffff) / 65536.0;
}

static real angle_between(real_vector3d const *a, real_vector3d const *b)
{
	real_vector3d cross;

	cross.i = a->j * b->k - a->k * b->j;
	cross.j = a->k * b->i - a->i * b->k;
	cross.k = a->i * b->j - a->j * b->i;
	return (real)atan2(vector_length(&cross), a->i * b->i + a->j * b->j + a->k * b->k) * 57.29578f;
}

/* render_objects.c's step of the lighting's shadow vector */
static void lighting_step(real_vector3d *normal, real_vector3d const *desired)
{
	real length;

	normal->i += PIN(desired->i - normal->i, -0.012f, 0.012f);
	normal->j += PIN(desired->j - normal->j, -0.012f, 0.012f);
	normal->k += PIN(desired->k - normal->k, -0.012f, 0.012f);
	length = vector_length(normal);
	normal->i /= length;
	normal->j /= length;
	normal->k /= length;
}

struct glide
{
	real largest_turn; /* degrees a frame, at most */
	real largest_speed; /* degrees a tick, at most, over a frame */
	real typical_speed; /* ... the mean while turning */
	real largest_lag; /* degrees behind the lighting, at most */
	real final_error; /* degrees from the lighting, at the end */
};

/* the lighting turning from straight down toward 20 degrees off it, a step
every refresh_interval ticks, then holding; frames of frame_length to twice
it (in ticks), the shadow vector drawn each */
static struct glide glide_through(real frame_length, long refresh_interval)
{
	struct interpolated_shadow shadow;
	struct glide result = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	real_vector3d lighting = { 0.0f, 0.0f, -1.0f };
	real_vector3d desired = { 0.34202f, 0.0f, -0.93969f };
	real_vector3d drawn, previous_drawn;
	double time = 0.0;
	long tick = 0, last_refresh = -1000;
	real speed_sum = 0.0f;
	long speed_count = 0;
	int frame;

	memset(&shadow, 0, sizeof(shadow));
	shadow.object_index = NONE;
	for (frame = 0; frame < 3000; frame++)
	{
		real step = (real)(frame_length * (1.0 + random_unit()));
		real turn;

		time += step;
		while ((long)floor(time) > tick)
		{
			tick++;
			if (tick - last_refresh >= refresh_interval)
			{
				lighting_step(&lighting, &desired);
				last_refresh = tick;
			}
		}
		shadow_vector_drawn(&shadow, 7, &lighting, (real)time, &drawn);
		/* (from the second step: the first, as the shadow is first drawn,
		comes with no time since a step before it, and glides over a tick) */
		if (frame > 0 && tick > refresh_interval + 1)
		{
			turn = angle_between(&drawn, &previous_drawn);
			if (turn > result.largest_turn)
				result.largest_turn = turn;
			if (turn / step > result.largest_speed)
				result.largest_speed = turn / step;
			if (turn > 1e-4f && angle_between(&lighting, &desired) > 2.0f)
			{
				speed_sum += turn / step;
				speed_count++;
			}
		}
		if (angle_between(&drawn, &lighting) > result.largest_lag)
			result.largest_lag = angle_between(&drawn, &lighting);
		previous_drawn = drawn;
	}
	result.typical_speed = speed_count ? speed_sum / speed_count : 0.0f;
	result.final_error = angle_between(&drawn, &lighting);
	return result;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* a step (about a degree) every 4 ticks, at about 144 frames a second:
	no hop, a steady turn, and at the lighting in the end */
	CASE("steady turn")
	{
		struct glide glide = glide_through(30.0f / 144.0f / 1.5f, 4);

		CHECK(glide.largest_turn < 0.15f, "turned %g degrees in a frame", glide.largest_turn);
		CHECK(glide.largest_speed < 1.5f * glide.typical_speed, "%g degrees a tick at most, %g typically",
			glide.largest_speed, glide.typical_speed);
		CHECK(glide.largest_lag < 2.5f, "%g degrees behind the lighting", glide.largest_lag);
		CHECK(glide.final_error < 1e-3f, "%g degrees from the lighting at the end", glide.final_error);
		return 0;
	}
	/* a step every tick (an object large on the screen) */
	CASE("every tick")
	{
		struct glide glide = glide_through(30.0f / 144.0f / 1.5f, 1);

		CHECK(glide.largest_turn < 0.6f, "turned %g degrees in a frame", glide.largest_turn);
		CHECK(glide.largest_speed < 1.5f * glide.typical_speed, "%g degrees a tick at most, %g typically",
			glide.largest_speed, glide.typical_speed);
		CHECK(glide.final_error < 1e-3f, "%g degrees from the lighting at the end", glide.final_error);
		return 0;
	}
	/* a new object's, one not drawn for longer than steps come, and a turn
	no step makes (30 degrees): as the lighting has them */
	CASE("drawn at once")
	{
		struct interpolated_shadow shadow;
		real_vector3d down = { 0.0f, 0.0f, -1.0f };
		real_vector3d slanted = { 0.5f, 0.0f, -0.866025f };
		real_vector3d nearly = { 0.0174524f, 0.0f, -0.9998477f };
		real_vector3d drawn;

		memset(&shadow, 0, sizeof(shadow));
		shadow.object_index = NONE;
		shadow_vector_drawn(&shadow, 7, &slanted, 10.0f, &drawn);
		CHECK(angle_between(&drawn, &slanted) < 1e-3f, "a new object's %g degrees off", angle_between(&drawn, &slanted));
		shadow_vector_drawn(&shadow, 7, &down, 10.2f, &drawn);
		CHECK(angle_between(&drawn, &down) < 1e-3f, "a 30 degree turn %g degrees off", angle_between(&drawn, &down));
		shadow_vector_drawn(&shadow, 7, &nearly, 30.0f, &drawn);
		CHECK(angle_between(&drawn, &nearly) < 1e-3f, "after 20 ticks unseen %g degrees off",
			angle_between(&drawn, &nearly));
		shadow_vector_drawn(&shadow, 8, &down, 30.1f, &drawn);
		CHECK(angle_between(&drawn, &down) < 1e-3f, "another object's %g degrees off", angle_between(&drawn, &down));
		return 0;
	}

	fprintf(stderr, "no such case: %s\n", case_name);
	return 2;
}
