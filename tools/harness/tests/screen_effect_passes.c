/*
SCREEN_EFFECT_PASSES.C (test)

The real rasterizer_screen_effect_refinement_passes and
rasterizer_screen_effect_pass_radius (rasterizer_xbox_screen_effect.c): how
many passes a convolution (a scope's zoom warp) draws on a screen of several
pixels to the Xbox's one, and the radius of each. The passes must stay few
(each is a full-screen draw; ten of them halved the frame rate zoomed in at
2160 lines), the copies of the screen they blend must stand no farther apart
in the screen's pixels than the Xbox's stood in its own (or they show as
ghosts), and the blur must stay as wide as the Xbox's. A pass blends three
copies of the screen: one in place and one a radius either way (the scopes'
warp with its mask). test_screen_effect_passes.py takes the code
(under_test.inc).
*/

#include "harness.h"

#include "under_test.inc"

/* the Xbox's passes for a convolution with no extra passes */
enum { STOCK_PASS_COUNT = 2 };
/* the most copies the passes put along a radius: 3 to the power of six */
enum { MAXIMUM_POSITIONS = 4096 };

static int compare_reals(const void *a, const void *b)
{
	real x = *(const real *)a, y = *(const real *)b;

	return x < y ? -1 : x > y;
}

/* where the passes' copies of the screen land, in radii, and the variance of
the blur they make (each copy weighed alike) */
static long copy_positions(short pass_count, real *positions, real *variance)
{
	long count = 1;
	short pass;
	real sum = 0.0f;

	positions[0] = 0.0f;
	*variance = 0.0f;
	for (pass = 0; pass < pass_count; pass++)
	{
		real radius = rasterizer_screen_effect_pass_radius(1.0f, pass, STOCK_PASS_COUNT);
		long index;

		CHECK(count * 3 <= MAXIMUM_POSITIONS, "%d passes make too many copies", pass_count);
		for (index = 0; index < count; index++)
		{
			positions[count + index] = positions[index] - radius;
			positions[2 * count + index] = positions[index] + radius;
		}
		count *= 3;
		/* three taps -r, 0, r: a variance of two thirds of r squared */
		sum += 2.0f / 3.0f * radius * radius;
	}
	*variance = sum;
	qsort(positions, (size_t)count, sizeof(real), compare_reals);
	return count;
}

/* the widest gap between two neighbouring copies, in radii */
static real widest_gap(const real *positions, long count)
{
	real widest = 0.0f;
	long index;

	for (index = 1; index < count; index++)
	{
		real gap = positions[index] - positions[index - 1];

		if (gap > widest)
			widest = gap;
	}
	return widest;
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";
	static real positions[MAXIMUM_POSITIONS];
	/* the Xbox's: two passes, copies a radius apart in its own pixels */
	real stock_variance;
	long stock_count = copy_positions(STOCK_PASS_COUNT, positions, &stock_variance);
	real stock_gap = widest_gap(positions, stock_count);
	static const real scales[] = { 1.0f, 1.5f, 2.0f, 2.25f, 3.0f, 4.5f, 6.0f, 9.0f, 12.0f, 40.0f, 81.0f, 1000.0f };
	int index;

	/* at the Xbox's size it draws only the Xbox's passes */
	CASE("stock-size-unchanged")
	{
		CHECK(rasterizer_screen_effect_refinement_passes(1.0f) == 0, "%d passes added at 640x480",
			rasterizer_screen_effect_refinement_passes(1.0f));
		CHECK(rasterizer_screen_effect_refinement_passes(0.5f) == 0, "passes added below the Xbox's size");
		return 0;
	}
	/* the Xbox's passes keep the effect's radius; each pass after them is a
	third of the one before */
	CASE("pass-radii")
	{
		CHECK(rasterizer_screen_effect_pass_radius(9.0f, 0, STOCK_PASS_COUNT) == 9.0f, "the first pass's radius changed");
		CHECK(rasterizer_screen_effect_pass_radius(9.0f, 1, STOCK_PASS_COUNT) == 9.0f, "the second pass's radius changed");
		CHECK(rasterizer_screen_effect_pass_radius(9.0f, 2, STOCK_PASS_COUNT) == 3.0f, "the third pass is %g",
			rasterizer_screen_effect_pass_radius(9.0f, 2, STOCK_PASS_COUNT));
		CHECK(rasterizer_screen_effect_pass_radius(9.0f, 3, STOCK_PASS_COUNT) == 1.0f, "the fourth pass is %g",
			rasterizer_screen_effect_pass_radius(9.0f, 3, STOCK_PASS_COUNT));
		return 0;
	}
	/* few passes at any size, an even count (the last draws into the
	screen), and 2160 lines (4.5 to the Xbox's one) draw four, not ten */
	CASE("few-passes")
	{
		for (index = 0; index < (int)(sizeof(scales) / sizeof(scales[0])); index++)
		{
			short added = rasterizer_screen_effect_refinement_passes(scales[index]);

			CHECK(added >= 0 && added <= 4, "%d passes added at %gx", added, scales[index]);
			CHECK(added % 2 == 0, "an odd count (%d) added at %gx", added, scales[index]);
		}
		CHECK(STOCK_PASS_COUNT + rasterizer_screen_effect_refinement_passes(4.5f) == 4, "%d passes at 2160 lines",
			STOCK_PASS_COUNT + rasterizer_screen_effect_refinement_passes(4.5f));
		return 0;
	}
	/* the copies stand no farther apart in the screen's pixels than the
	Xbox's did in its own, at every size up to 81 to the Xbox's one */
	CASE("no-ghosts")
	{
		for (index = 0; index < (int)(sizeof(scales) / sizeof(scales[0])) && scales[index] <= 81.0f; index++)
		{
			short pass_count = STOCK_PASS_COUNT + rasterizer_screen_effect_refinement_passes(scales[index]);
			real variance;
			long count = copy_positions(pass_count, positions, &variance);
			real gap = widest_gap(positions, count) * scales[index];

			CHECK(gap <= stock_gap * 1.0001f, "copies %g radii apart in the screen's pixels at %gx (the Xbox's %g)",
				gap, scales[index], stock_gap);
		}
		return 0;
	}
	/* the blur is as wide as the Xbox's: its variance within a tenth */
	CASE("blur-as-wide")
	{
		for (index = 0; index < (int)(sizeof(scales) / sizeof(scales[0])); index++)
		{
			short pass_count = STOCK_PASS_COUNT + rasterizer_screen_effect_refinement_passes(scales[index]);
			real variance;

			copy_positions(pass_count, positions, &variance);
			CHECK(variance >= stock_variance * 0.999f && variance <= stock_variance * 1.1f,
				"variance %g of the Xbox's at %gx", variance / stock_variance, scales[index]);
		}
		return 0;
	}
	fprintf(stderr, "unknown case '%s'\n", case_name);
	return 2;
}
