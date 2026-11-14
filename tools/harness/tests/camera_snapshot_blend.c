/*
CAMERA_SNAPSHOT_BLEND.C (test)

The real camera_snapshot_blend, camera_snapshot_carried and observer_cut
(render_interpolation.c): a cinematic's camera set to a point, which the
observer moves every frame, is drawn from its snapshots (taken at the first
frame after each tick, some part of a tick after it) at the moment the
objects are drawn at, a tick behind: a steady move is drawn steadily, at
whatever times the frames come. Blended by the frame's fraction itself, it
sped up and slowed down every tick: a cutscene's turning camera stuttered
from beginning to end (d40's "... And the Horse You Rode In On"). And it is
cut by a move away from where its own velocity carries it, not by a long
one: d40's fly-over moves 3.1 world units a tick, past CAMERA_CUT_DISTANCE,
and was taken for a cut every few frames, drawn a tick ahead and then behind
by turns.
test_camera_snapshot_blend.py takes the code from the sources (under_test.inc).
*/

#include "harness.h"

#include <math.h>

/* (what observer_cut and camera_snapshot_carried read of the observer's result) */
struct observer_result
{
	real_point3d position;
	real_vector3d velocity;
	real_vector3d forward;
};

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

/* a cinematic's fly-over (in world units, at a time in ticks): up to 4.3
world units a tick, turning and changing speed by up to 0.6 a tick squared
(540 world units a second squared, short of the observer's 1500) */
static void flight_at(double time, struct observer_result *result)
{
	result->position.x = (real)(3.7 * time + 0.6 * sin(time));
	result->position.y = (real)(0.6 * cos(time));
	result->position.z = 20.0f;
	/* (world units a second) */
	result->velocity.i = (real)(30.0 * (3.7 + 0.6 * cos(time)));
	result->velocity.j = (real)(30.0 * -0.6 * sin(time));
	result->velocity.k = 0.0f;
	result->forward.i = 1.0f;
	result->forward.j = 0.0f;
	result->forward.k = 0.0f;
}

/* the fly-over snapshotted as the port does, at frames from frame_length to
twice it (in ticks), with a jump of jump world units (along y) at the snapshot
after jump_tick: whether each pair of snapshots a tick apart was a cut, the
pairs it counts (cuts before the jump, after: the jump's own pair) */
static void flight_cuts(double frame_length, long jump_tick, real jump, int *cuts_before, int *cut_at_jump)
{
	double time = 0.0;
	long tick = 0;
	struct observer_result previous, latest;
	real previous_taken = 0.0f, latest_taken = 0.0f;
	int snapshots = 0;
	int frame;

	*cuts_before = 0;
	*cut_at_jump = 0;
	for (frame = 0; frame < 3000; frame++)
	{
		time += frame_length * (1.0 + random_unit());
		if ((long)floor(time) != tick)
		{
			tick = (long)floor(time);
			previous = latest;
			previous_taken = latest_taken;
			flight_at(time, &latest);
			if (tick > jump_tick)
				latest.position.y += jump;
			latest_taken = (real)(time - floor(time));
			snapshots++;
			if (snapshots >= 2)
			{
				struct observer_result carried;
				boolean cut;

				camera_snapshot_carried(&previous, previous_taken, latest_taken, &carried);
				cut = observer_cut(&carried, &latest);

				if (tick - 1 == jump_tick)
					*cut_at_jump = cut;
				else if (cut)
					(*cuts_before)++;
			}
		}
	}
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
	/* a fly-over faster than CAMERA_CUT_DISTANCE a tick, at 144 and at 40
	frames a second: never a cut */
	CASE("fast flight")
	{
		int cuts, cut_at_jump;

		flight_cuts(30.0 / 144.0 / 1.5, 100000, 0.0f, &cuts, &cut_at_jump);
		CHECK(cuts == 0 && !cut_at_jump, "%d cuts at 144 frames a second", cuts + cut_at_jump);
		flight_cuts(30.0 / 40.0 / 1.5, 100000, 0.0f, &cuts, &cut_at_jump);
		CHECK(cuts == 0 && !cut_at_jump, "%d cuts at 40 frames a second", cuts + cut_at_jump);
		return 0;
	}
	/* a jump of 4 world units across the fly-over (which moves further in
	a tick), away from where it carries itself: a cut, and the only one */
	CASE("jump in flight")
	{
		int cuts, cut_at_jump;

		flight_cuts(30.0 / 144.0 / 1.5, 400, 4.0f, &cuts, &cut_at_jump);
		CHECK(cut_at_jump && cuts == 0, "the jump %s a cut, %d other cuts", cut_at_jump ? "was" : "was not", cuts);
		return 0;
	}
	/* a camera at rest, then 4 world units on (a cut to a nearby camera
	point): a cut */
	CASE("jump at rest")
	{
		struct observer_result a = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } };
		struct observer_result b = { { 0.0f, 4.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } };

		struct observer_result carried;

		camera_snapshot_carried(&a, 0.1f, 0.2f, &carried);
		CHECK(observer_cut(&carried, &b), "not a cut");
		return 0;
	}
	/* a velocity not a number: a cut (drawn from the observer) */
	CASE("not a number")
	{
		struct observer_result a = { { 0.0f, 0.0f, 0.0f }, { NAN, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } };
		struct observer_result b = { { 0.1f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } };
		struct observer_result carried;

		camera_snapshot_carried(&a, 0.1f, 0.2f, &carried);
		CHECK(observer_cut(&carried, &b), "not a cut");
		return 0;
	}

	fprintf(stderr, "no such case: %s\n", case_name);
	return 2;
}
