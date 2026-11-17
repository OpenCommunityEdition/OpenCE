/*
LIGHT_SCALE.C (test)

The real light_scale_radius (light_scale.c): with per-pixel lighting, a
light on an object reaches as far as its definition says times the
object's scale (a10's escape shrinks the lifepod to a fiftieth, and its red
cabin light, at a world unit, lit the miniature ring's hull); lit for each
vertex, or on no object, or an object of no scale, as it was.
test_light_scale.py takes the code from the sources (under_test.inc).
*/

#include "harness.h"

#define _object_mask_all 0xFFFF

struct object_datum
{
	struct { real scale; } object;
};

static struct object_datum test_object;
static boolean test_per_pixel;
static unsigned long test_changes;

static int config_boolean(const char *name)
{
	return !strcmp(name, "display.per_pixel_lighting") && test_per_pixel;
}

static unsigned long config_changes(void)
{
	return test_changes;
}

static void *object_try_and_get_and_verify_type(long object_index, unsigned long valid_type_flags)
{
	(void)valid_type_flags;
	return object_index == 7 ? &test_object : NULL;
}

#include "under_test.inc"

static boolean near(real a, real b)
{
	return a - b < 1e-5f && b - a < 1e-5f;
}

static void setting(boolean per_pixel)
{
	test_per_pixel = per_pixel;
	test_changes++;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* the lifepod at a fiftieth: its light a fiftieth as far */
	CASE("scaled")
	{
		setting(TRUE);
		test_object.object.scale = 0.02f;
		CHECK(near(light_scale_radius(7, 1.0f), 0.02f), "radius %f", light_scale_radius(7, 1.0f));
		test_object.object.scale = 3.0f;
		CHECK(near(light_scale_radius(7, 0.5f), 1.5f), "radius %f", light_scale_radius(7, 0.5f));
		return 0;
	}
	/* lit for each vertex: as on the Xbox, read again as the setting
	changes */
	CASE("per vertex")
	{
		test_object.object.scale = 0.02f;
		setting(TRUE);
		CHECK(near(light_scale_radius(7, 1.0f), 0.02f), "radius %f", light_scale_radius(7, 1.0f));
		setting(FALSE);
		CHECK(near(light_scale_radius(7, 1.0f), 1.0f), "radius %f lit for each vertex", light_scale_radius(7, 1.0f));
		return 0;
	}
	/* no object, one gone, one of no scale: as it was */
	CASE("as it was")
	{
		setting(TRUE);
		test_object.object.scale = 0.0f;
		CHECK(near(light_scale_radius(NONE, 1.0f), 1.0f), "no object");
		CHECK(near(light_scale_radius(8, 1.0f), 1.0f), "an object gone");
		CHECK(near(light_scale_radius(7, 1.0f), 1.0f), "an object of no scale");
		return 0;
	}

	return 2;
}
