/*
CAMERA_SNAPSHOT_BLEND.C (test)

The real camera_snapshot_blend (render_interpolation.c): a cinematic's camera
set to a point, which the observer moves every frame, is drawn from its
snapshots (taken at the first frame after each tick, some part of a tick
after it) at the moment the objects are drawn at, a tick behind: a steady
move is drawn steadily, at whatever times the frames come. Blended by the
frame's fraction itself, it sped up and slowed down every tick: a cutscene's
turning camera stuttered from beginning to end (d40's "... And the Horse You
Rode In On").
test_camera_snapshot_blend.py takes the code from the sources (under_test.inc).
*/

#include "harness.h"

#include <math.h>

#include "under_test.inc"

/* a camera moving steadily: where it is at a time (in ticks) */
static real position_at(double time)
{
	return (real)(3.0 * time + 5.0);
}

static unsigned long random_state = 12345;

static double random_unit(void)
{
	random_state = random_state * 1103515245UL + 12345UL;
	return (double)((random_state >> 8) & 0xffff) / 65536.0;
}

/* frames from first to last length (in ticks; a frame's length is
this plus up to as much again), drawn as the port draws them: the snapshot
at the first frame after each tick, the camera blended from the last two;
the largest distance from where the camera was a tick before */
static real worst_error(double frame_length)
{
	double time = 0.0;
	long tick = 0;
	real previous = 0.0f, latest = 0.0f, previous_taken = 0.0f, latest_taken = 0.0f;
	int snapshots = 0;
	real worst = 0.0f;
	int frame;

	for (frame = 0; frame < 4000; frame++)
	{
		real fraction;

		time += frame_length * (1.0 + random_unit());
		fraction = (real)(time - floor(time));
		if ((long)floor(time) != tick)
		{
			/* the first frame after a tick (one at most here) */
			tick = (long)floor(time);
			previous = latest;
			previous_taken = latest_taken;
			latest = position_at(time);
			latest_taken = fraction;
			snapshots++;
		}
		if (snapshots >= 2)
		{
			real t = camera_snapshot_blend(fraction, previous_taken, latest_taken);
			real drawn = previous + t * (latest - previous);
			real error = fabsf(drawn - position_at(time - 1.0));

			if (error > worst)
				worst = error;
		}
	}
	return worst;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* at 144 frames a second, unevenly */
	CASE("fast frames")
	{
		real error = worst_error(30.0 / 144.0 / 1.5);

		CHECK(error < 1e-3f, "drawn up to %g from a tick before (of 3 a tick)", error);
		return 0;
	}
	/* at 60, where a frame is half a tick */
	CASE("slow frames")
	{
		real error = worst_error(30.0 / 60.0 / 1.5);

		CHECK(error < 1e-3f, "drawn up to %g from a tick before (of 3 a tick)", error);
		return 0;
	}
	/* taken at the tick itself both times: the fraction */
	CASE("on the tick")
	{
		CHECK(camera_snapshot_blend(0.25f, 0.0f, 0.0f) == 0.25f, "taken on the ticks, 0.25 drawn at %g",
			camera_snapshot_blend(0.25f, 0.0f, 0.0f));
		return 0;
	}

	fprintf(stderr, "no such case: %s\n", case_name);
	return 2;
}
