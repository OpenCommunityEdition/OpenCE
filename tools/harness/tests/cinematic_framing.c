/*
CINEMATIC_FRAMING.C (test)

The real cinematic_framing_camera_at_shot, _xbox_width and
_vertical_field_of_view (cinematic_framing.c): while the scripted camera
looks from a10's keyboard_med point, a window wider than 4:3 has its
vertical angle narrowed to show a 4:3 window's width; another point, one a
centimetre from it, another map, a camera not at a point of its own and a
window 4:3 or narrower are drawn as they were.
test_cinematic_framing.py takes the code from the sources (under_test.inc).
*/

#include "harness.h"

#include <math.h>

#define NUMBEROF(array) (sizeof(array) / sizeof(array[0]))
#define csstrcmp strcmp
#define _pi 3.14159265359f
#define tangent(x) ((real)tan(x))
#define arctangent(y, x) ((real)atan2((y), (x)))
#define TAG_BLOCK_GET_ELEMENT(block, index, type) (&((type *)(block)->address)[index])

typedef struct { short x0, y0, x1, y1; } rectangle2d;
struct tag_block { long count; void *address; };
struct scenario_cutscene_camera_point { char name[32]; real_point3d position; };
struct scenario { struct tag_block cutscene_camera_points; };

static struct scenario_cutscene_camera_point test_points[2] =
{
	{"booth_hi_1", {-50.f, 1.f, 2.5f}},
	{"keyboard_med", {-51.377f, 0.292f, 1.974f}},
};
static struct scenario test_scenario = {{2, test_points}};
static long global_scenario_index = 4;
static char const *test_scenario_name = "levels\\a10\\a10";
static boolean test_camera_at_point = TRUE;
static real_point3d test_camera;

static char const *tag_get_name(long tag_index) { (void)tag_index; return test_scenario_name; }
static struct scenario *global_scenario_get(void) { return &test_scenario; }
static boolean scripted_camera_point_position(real_point3d *position)
{
	if (!test_camera_at_point)
		return FALSE;
	*position = test_camera;
	return TRUE;
}

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

	return cinematic_framing_vertical_field_of_view(&bounds, TEST_VERTICAL);
}

static void test_reset(void)
{
	test_camera = test_points[1].position;
	test_camera_at_point = TRUE;
	test_scenario_name = "levels\\a10\\a10";
	global_scenario_index = 4;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* a10's keyboard shot on a 16:9 window: a 4:3 window's width */
	CASE("narrowed")
	{
		real expected = tanf(0.5f * TEST_VERTICAL) * (4.f * 480.f) / (3.f * 854.f);
		real angle;

		test_reset();
		angle = test_angle(854, 480);
		CHECK(fabsf(tanf(0.5f * angle) - expected) < 1e-5f, "tangent %f, not %f", tanf(0.5f * angle), expected);
		test_camera.x += 0.0003f;
		CHECK(test_angle(854, 480) < TEST_VERTICAL - 0.1f, "not narrowed a third of a millimetre off the point");
		return 0;
	}
	/* anything else as it was */
	CASE("kept")
	{
		test_reset();
		CHECK(test_angle(640, 480) == TEST_VERTICAL, "a 4:3 window changed");
		CHECK(test_angle(480, 480) == TEST_VERTICAL, "a square window changed");
		test_camera = test_points[0].position;
		CHECK(test_angle(854, 480) == TEST_VERTICAL, "another point narrowed");
		test_reset();
		test_camera.x += 0.01f;
		CHECK(test_angle(854, 480) == TEST_VERTICAL, "a centimetre off the point narrowed");
		test_reset();
		test_scenario_name = "levels\\a30\\a30";
		CHECK(test_angle(854, 480) == TEST_VERTICAL, "another map narrowed");
		test_reset();
		test_camera_at_point = FALSE;
		CHECK(test_angle(854, 480) == TEST_VERTICAL, "a camera not at a point narrowed");
		test_reset();
		global_scenario_index = NONE;
		CHECK(test_angle(854, 480) == TEST_VERTICAL, "narrowed with no scenario");
		test_reset();
		CHECK(cinematic_framing_vertical_field_of_view(NULL, TEST_VERTICAL) == TEST_VERTICAL, "no window");
		return 0;
	}

	return 2;
}
