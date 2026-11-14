/*
LENS_FLARE_COVERAGE.C (test)

The real lens_flare_reflection_coverage (rasterizer_lights.c): a lens
flare's reflection smaller than a pixel of the game's 480 lines keeps as
much of its brightness as it covers of one (its diameter squared, in those
pixels); one of a pixel or more keeps all of it. The diameter is the
radius's at the depth, through the window's vertical field of view and its
height in those lines (a split screen's half is 240).
test_lens_flare_coverage.py takes the code from the sources
(under_test.inc).
*/

#include <math.h>
#include "harness.h"

static struct
{
	struct
	{
		struct { short x0, y0, x1, y1; } viewport_bounds;
		real vertical_field_of_view;
	} camera;
} global_window_parameters;

#include "under_test.inc"

#define NEAR(a, b) (fabs((a) - (b)) < 1e-4)

/* the window: 480 lines (or 240), a vertical field of view whose half's
tangent is 1, so that a radius r at depth d is r/d*240 pixels */
static void window(short lines)
{
	global_window_parameters.camera.viewport_bounds.x0 = 0;
	global_window_parameters.camera.viewport_bounds.y0 = 0;
	global_window_parameters.camera.viewport_bounds.x1 = 640;
	global_window_parameters.camera.viewport_bounds.y1 = lines;
	global_window_parameters.camera.vertical_field_of_view = 1.5707963f; /* (a right angle: tan of half is 1) */
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	window(480);
	/* d40's flames: a fifth of a pixel across keeps 4% */
	CASE("small")
	{
		real coverage = lens_flare_reflection_coverage(0.2f, 480.0f);

		CHECK(NEAR(coverage, 0.04), "a fifth of a pixel keeps %f", coverage);
		coverage = lens_flare_reflection_coverage(0.25f, 240.0f);
		CHECK(NEAR(coverage, 0.25), "half a pixel keeps %f", coverage);
		return 0;
	}
	/* a pixel or more keeps all of it */
	CASE("large")
	{
		CHECK(lens_flare_reflection_coverage(1.0f, 480.0f) == 1.0f, "a pixel dimmed");
		CHECK(lens_flare_reflection_coverage(10.0f, 100.0f) == 1.0f, "a large one dimmed");
		return 0;
	}
	/* a split screen's half: half the lines, half the diameter */
	CASE("split")
	{
		real coverage;

		window(240);
		coverage = lens_flare_reflection_coverage(0.5f, 240.0f);
		CHECK(NEAR(coverage, 0.25), "half a pixel of a half screen keeps %f", coverage);
		return 0;
	}
	/* no depth, no window, a radius not a number: left as it is */
	CASE("degenerate")
	{
		CHECK(lens_flare_reflection_coverage(1.0f, 0.0f) == 1.0f, "no depth dimmed");
		CHECK(lens_flare_reflection_coverage(1.0f, -5.0f) == 1.0f, "behind dimmed");
		CHECK(lens_flare_reflection_coverage((real)NAN, 10.0f) == 1.0f, "not a number dimmed");
		window(0);
		CHECK(lens_flare_reflection_coverage(0.1f, 10.0f) == 1.0f, "no window dimmed");
		return 0;
	}

	return 2;
}
