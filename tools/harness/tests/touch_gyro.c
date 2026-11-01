#include "harness.h"
#include <math.h>

static const char *mode = "legacy";
static double horizontal = 2.0, vertical = 0.5;
static int invert_horizontal, invert_vertical;
static const char *config_string(const char *name) { return mode; }
static double config_real(const char *name)
{
	return strstr(name, "horizontal") ? horizontal : vertical;
}
static int config_boolean(const char *name)
{
	return strstr(name, "horizontal") ? invert_horizontal : invert_vertical;
}
#define HALO_ANDROID
#include "under_test.inc"

int main(int argc, char **argv)
{
	float yaw = 0.25f, pitch = -0.5f;
	const char *test = argc > 1 ? argv[1] : "";
	if (!strcmp(test, "legacy"))
	{
		invert_horizontal = invert_vertical = 1;
		touch_gyro_apply(&yaw, &pitch);
		halo_linux_gyro_filter(0, &yaw, &pitch);
		CHECK(yaw == 0.25f && pitch == -0.5f, "saved touch behavior changed");
		mode = "invalid";
		CHECK(touch_gyro_mode() == _touch_gyro_legacy, "unknown mode changed the default");
	}
	else if (!strcmp(test, "modes"))
	{
		mode = "false";
		touch_gyro_apply(&yaw, &pitch);
		CHECK(yaw == 0 && pitch == 0, "off mode passed gyro input");
		mode = "zoom";
		yaw = pitch = 1;
		touch_gyro_apply(&yaw, &pitch);
		halo_linux_gyro_filter(0, &yaw, &pitch);
		CHECK(yaw == 0 && pitch == 0, "unzoomed motion retained");
		halo_linux_gyro_filter(1, &yaw, &pitch);
		CHECK(yaw == 0 && pitch == 0, "zoom resurrected a dropped sample");
		yaw = pitch = 1;
		touch_gyro_apply(&yaw, &pitch);
		halo_linux_gyro_filter(1, &yaw, &pitch);
		CHECK(yaw == 2 && pitch == 0.5f, "zoomed sensitivity");
		mode = "true";
		halo_linux_gyro_filter(0, &yaw, &pitch);
		CHECK(yaw == 2 && pitch == 0.5f, "always mode requires zoom");
	}
	else if (!strcmp(test, "axes"))
	{
		mode = "true";
		invert_horizontal = 1;
		touch_gyro_apply(&yaw, &pitch);
		CHECK(yaw == -0.5f && pitch == -0.25f, "independent axes");
		horizontal = NAN;
		vertical = 11;
		yaw = pitch = 1;
		touch_gyro_apply(&yaw, &pitch);
		CHECK(yaw == -1 && pitch == 1, "invalid sensitivity was not bounded");
	}
	else CHECK(0, "unknown case");
	return 0;
}
