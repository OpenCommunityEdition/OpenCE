/*
OVERSHIELD_COLORS.C (test)

The real overshield_effect_green_target, overshield_effect_green_turn and
overshield_effect_mix_color (overshield_effect.c): the overshield's flare
(display.overshield_effect) is green as it charges and while the shield is
over 2, as the shield meter shows its second overcharge in green, and the
powerup's red at 2 or less, as the meter shows its first in red, turning
from one to the other in a quarter of a second.
test_overshield_colors.py takes the code from the sources (under_test.inc).
*/

#include "harness.h"

#include <math.h>

typedef struct { real red, green, blue; } real_rgb_color;

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CEILING(n,ceiling) ((n)>(ceiling)?(ceiling):(n))
#define PIN(n,floor,ceiling) ((n)<(floor) ? (floor) : CEILING((n),(ceiling)))

#include "under_test.inc"

static boolean near(real a, real b)
{
	return fabsf(a - b) < 1e-4f;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* picked up: charging (from a full shield, or from a down one), green */
	CASE("picked up")
	{
		CHECK(overshield_effect_green_target(TRUE, 1.0f) == 1.0f, "charging from 1: %g", overshield_effect_green_target(TRUE, 1.0f));
		CHECK(overshield_effect_green_target(TRUE, 0.1f) == 1.0f, "charging from 0.1: %g", overshield_effect_green_target(TRUE, 0.1f));
		CHECK(overshield_effect_green_target(TRUE, 2.5f) == 1.0f, "charging at 2.5: %g", overshield_effect_green_target(TRUE, 2.5f));
		return 0;
	}
	/* charged: green over 2 (the meter's second overcharge), red at 2 or less
	(its first) */
	CASE("meter layers")
	{
		CHECK(overshield_effect_green_target(FALSE, 3.0f) == 1.0f, "at 3: %g", overshield_effect_green_target(FALSE, 3.0f));
		CHECK(overshield_effect_green_target(FALSE, 2.01f) == 1.0f, "at 2.01: %g", overshield_effect_green_target(FALSE, 2.01f));
		CHECK(overshield_effect_green_target(FALSE, 2.0f) == 0.0f, "at 2: %g", overshield_effect_green_target(FALSE, 2.0f));
		CHECK(overshield_effect_green_target(FALSE, 1.5f) == 0.0f, "at 1.5: %g", overshield_effect_green_target(FALSE, 1.5f));
		return 0;
	}
	/* from green to red: a quarter of a second, and none for a time gone
	backward */
	CASE("turn")
	{
		real green = 1.0f;

		green = overshield_effect_green_turn(green, 0.0f, 0.1f);
		CHECK(near(green, 0.6f), "after 0.1 s: %g", green);
		green = overshield_effect_green_turn(green, 0.0f, -0.5f);
		CHECK(near(green, 0.6f), "a time gone backward: %g", green);
		green = overshield_effect_green_turn(green, 0.0f, 0.15f);
		CHECK(near(green, 0.0f), "after 0.25 s: %g", green);
		green = overshield_effect_green_turn(green, 0.0f, 0.5f);
		CHECK(near(green, 0.0f), "past it: %g", green);
		green = overshield_effect_green_turn(green, 1.0f, 0.05f);
		CHECK(near(green, 0.2f), "back toward green after 0.05 s: %g", green);
		return 0;
	}
	/* the two colors: the powerup's (253, 78, 116) and green (136, 253, 98) */
	CASE("colors")
	{
		real_rgb_color red, green, half;

		overshield_effect_mix_color(0.0f, &red);
		overshield_effect_mix_color(1.0f, &green);
		overshield_effect_mix_color(0.5f, &half);
		CHECK(near(red.red, 0.992f) && near(red.green, 0.306f) && near(red.blue, 0.455f),
			"red (%g, %g, %g)", red.red, red.green, red.blue);
		CHECK(green.green > green.red && green.green > green.blue && green.green > 0.9f,
			"green (%g, %g, %g)", green.red, green.green, green.blue);
		CHECK(near(half.green, (red.green + green.green) * 0.5f), "half way %g", half.green);
		return 0;
	}

	fprintf(stderr, "no such case: %s\n", case_name);
	return 2;
}
