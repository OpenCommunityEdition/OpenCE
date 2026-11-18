/*
CINEMATIC_LETTERBOX.C (test)

The real cinematic_letterbox.c (display.cinematic_framing): while the
scripted camera shows a shot with the letterbox (or the bars still going),
the view shows the Xbox's picture between the bars, its 4:3 width and 16:9
tall, across the window, and the bars are only as high as keep it 16:9;
off, for the player's own view, or for a shot without the letterbox, the
stock view and bars.
test_cinematic_letterbox.py takes the code from the sources (under_test.inc).
*/

#include "harness.h"

#include <math.h>

#define _pi 3.14159265359f
#define tangent(x) ((real)tan(x))
#define arctangent(y, x) ((real)atan2((y), (x)))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define PIN(x, low, high) ((x) < (low) ? (low) : ((x) > (high) ? (high) : (x)))

typedef struct { short x0, y0, x1, y1; } rectangle2d;
struct cinematic_global_data { real letterbox_amount; boolean show_letterbox; };

static boolean test_framing;
static unsigned long test_changes;
static boolean test_scripted;
static boolean *director_camera_scripted = &test_scripted;
static struct cinematic_global_data test_cinematic;
static struct cinematic_global_data *cinematic_globals = &test_cinematic;

static int config_boolean(const char *name) { return !strcmp(name, "display.cinematic_framing") && test_framing; }
static unsigned long config_changes(void) { return test_changes; }

#include "config.inc"
#include "under_test.inc"

/* the stock view's vertical angle (70 degrees across at 4:3) */
#define TEST_VERTICAL 0.96982f

static rectangle2d test_window(short width, short height)
{
	rectangle2d bounds;

	bounds.x0 = 0;
	bounds.y0 = 0;
	bounds.x1 = width;
	bounds.y1 = height;
	return bounds;
}

static real test_angle(short width, short height)
{
	rectangle2d bounds = test_window(width, height);

	return cinematic_letterbox_vertical_field_of_view(&bounds, TEST_VERTICAL);
}

/* the angle's tangent over the stock one's */
static real test_scale(short width, short height)
{
	return tanf(0.5f * test_angle(width, height)) / tanf(0.5f * TEST_VERTICAL);
}

static real test_bars(short width, short height)
{
	rectangle2d bounds = test_window(width, height);

	return cinematic_letterbox_bar_height(&bounds);
}

static void test_set_framing(boolean framing)
{
	test_framing = framing;
	test_changes++;
}

/* display.cinematic_framing on, a shot of the scripted camera's with the
letterbox up */
static void test_cutscene(void)
{
	test_set_framing(TRUE);
	test_scripted = TRUE;
	test_cinematic.show_letterbox = TRUE;
	test_cinematic.letterbox_amount = 1.f;
}

#define CHECK_SCALE(width, height, expected) \
	CHECK(fabsf(test_scale(width, height) - (expected)) < 1e-4f, "%dx%d: tangent scaled by %f, not %f", \
		width, height, test_scale(width, height), (real)(expected))
#define CHECK_BARS(width, height, expected) \
	CHECK(fabsf(test_bars(width, height) - (expected)) < 1e-4f, "%dx%d: bars %f high, not %f", \
		width, height, test_bars(width, height), (real)(expected))

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* the Xbox's picture between the bars across the window: its 4:3 width
	on one 16:9 or narrower, its height (three quarters of the stock one's)
	on a wider one */
	CASE("xbox picture")
	{
		test_cutscene();
		CHECK_SCALE(1920, 1080, 0.75f);
		CHECK_SCALE(854, 480, 0.75f);
		CHECK_SCALE(640, 480, 1.f);
		CHECK_SCALE(768, 480, (4.f / 3.f) / 1.6f);
		CHECK_SCALE(1120, 480, 0.75f);
		CHECK_SCALE(480, 480, 4.f / 3.f);
		/* (the letterbox coming up: the whole way at once, with the cut) */
		test_cinematic.letterbox_amount = 0.f;
		CHECK_SCALE(854, 480, 0.75f);
		return 0;
	}
	/* the bars going while the camera stays on the shot: the view widens
	back as they go */
	CASE("bars going")
	{
		test_cutscene();
		test_cinematic.show_letterbox = FALSE;
		test_cinematic.letterbox_amount = 0.5f;
		CHECK_SCALE(854, 480, 0.875f);
		test_cinematic.letterbox_amount = 0.f;
		CHECK(test_angle(854, 480) == TEST_VERTICAL, "the shot narrowed without bars");
		return 0;
	}
	/* the stock view: the setting off, the player's own view under the bars
	(a chapter's title), a shot without the letterbox */
	CASE("stock view")
	{
		test_cutscene();
		test_set_framing(FALSE);
		CHECK(test_angle(854, 480) == TEST_VERTICAL, "narrowed with the setting off");
		test_cutscene();
		test_scripted = FALSE;
		CHECK(test_angle(854, 480) == TEST_VERTICAL, "the player's own view narrowed");
		test_cutscene();
		test_cinematic.show_letterbox = FALSE;
		test_cinematic.letterbox_amount = 0.f;
		CHECK(test_angle(854, 480) == TEST_VERTICAL, "a shot without the letterbox narrowed");
		test_cutscene();
		CHECK(cinematic_letterbox_vertical_field_of_view(NULL, TEST_VERTICAL) == TEST_VERTICAL, "no window");
		CHECK(test_angle(0, 0) == TEST_VERTICAL, "an empty window");
		return 0;
	}
	/* the bars: an eighth of the height each, or with the setting as high as
	keep the picture between them 16:9 */
	CASE("bars")
	{
		test_set_framing(FALSE);
		CHECK_BARS(1920, 1080, 0.125f);
		CHECK_BARS(640, 480, 0.125f);
		CHECK_BARS(1120, 480, 0.125f);
		CHECK(cinematic_letterbox_bar_height(NULL) == 0.125f, "no window");
		test_set_framing(TRUE);
		CHECK_BARS(1920, 1080, 0.f);
		CHECK_BARS(854, 480, 0.f);
		CHECK_BARS(1120, 480, 0.f);
		CHECK_BARS(640, 480, 0.125f);
		CHECK_BARS(768, 480, 0.05f);
		CHECK_BARS(480, 480, 0.21875f);
		CHECK_BARS(0, 0, 0.125f);
		/* (the setting read again once the settings change) */
		test_set_framing(FALSE);
		CHECK_BARS(854, 480, 0.125f);
		return 0;
	}

	return 2;
}
