/*
ATMOSPHERIC_FOG_SETTLE.C (test)

The real atmospheric_fog_settle_fraction and atmospheric_fog_settle_state
(atmospheric_fog_settle.c): a local player's atmospheric fog, which moves
toward the fog where the camera is only as the camera moves (scenario.c),
also settles there over time, by seconds / (0.5 + seconds) of the way each
frame: a camera standing still has it a second or two after it arrives, at
any frame rate. In b40, from the tunnels (dark blue, opaque at 120 world
units) onto the bridges (white, at 200), a player who stopped kept the
tunnels' fog until they walked on.
test_atmospheric_fog_settle.py takes the code from the sources (under_test.inc).
*/

#include "harness.h"

#include <math.h>

typedef struct { real red, green, blue; } real_rgb_color;

/* (what the settling reads and moves of scenario.h's and sky_definitions.h's) */
struct atmospheric_fog_state
{
	real atmospheric_start_distance;
	real atmospheric_opaque_distance;
	real atmospheric_maximum_density;
	real_rgb_color atmospheric_color;
	real indoor_fog_scale;
};

struct sky_atmospheric_fog
{
	real_rgb_color color;
	real maximum_density;
	real start_distance;
	real opaque_distance;
};

#include "under_test.inc"

/* b40's: in the tunnels, and on the bridges */
static struct atmospheric_fog_state const tunnels = { 0.0f, 120.0f, 0.95f, { 0.039f, 0.051f, 0.086f }, 0.0f };
static struct sky_atmospheric_fog const bridges = { { 0.922f, 0.941f, 0.945f }, 0.95f, 0.0f, 200.0f };

/* the share of the way still to go after standing still for seconds, at
frames a second */
static real remaining_after(real seconds, real frames_a_second, struct atmospheric_fog_state *state)
{
	long frame;
	long frames = (long)(seconds * frames_a_second + 0.5f);

	*state = tunnels;
	for (frame = 0; frame < frames; frame++)
		atmospheric_fog_settle_state(state, &bridges, 0.0f, atmospheric_fog_settle_fraction(1.0f / frames_a_second));
	return (bridges.opaque_distance - state->atmospheric_opaque_distance) /
		(bridges.opaque_distance - tunnels.atmospheric_opaque_distance);
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* standing still: within 15% a second on, 3% after two, at 30, 60 and 144
	frames a second alike */
	CASE("standing still")
	{
		static real const rates[3] = { 30.0f, 60.0f, 144.0f };
		struct atmospheric_fog_state state;
		real lowest = 1.0f, highest = 0.0f;
		int index;

		for (index = 0; index < 3; index++)
		{
			real after_one = remaining_after(1.0f, rates[index], &state);
			real after_two = remaining_after(2.0f, rates[index], &state);

			CHECK(after_one > 0.10f && after_one < 0.15f, "%g frames a second: %g left after a second",
				rates[index], after_one);
			CHECK(after_two < 0.03f, "%g frames a second: %g left after two seconds", rates[index], after_two);
			if (after_one < lowest)
				lowest = after_one;
			if (after_one > highest)
				highest = after_one;
		}
		CHECK(highest - lowest < 0.02f, "a second on, %g to %g left by frame rate", lowest, highest);
		return 0;
	}
	/* a second on the bridge: the cliffs in nearly the bridges' fog */
	CASE("on the bridge")
	{
		struct atmospheric_fog_state state;

		remaining_after(1.0f, 144.0f, &state);
		CHECK(state.atmospheric_opaque_distance > 185.0f, "opaque at %g", state.atmospheric_opaque_distance);
		CHECK(state.atmospheric_color.red > 0.8f && state.atmospheric_color.blue > 0.8f, "color (%g, %g, %g)",
			state.atmospheric_color.red, state.atmospheric_color.green, state.atmospheric_color.blue);
		return 0;
	}
	/* no time (a second call in a frame, a paused game) or a time gone
	backward: as it was; and never past the fog where the camera is */
	CASE("no time")
	{
		struct atmospheric_fog_state state = tunnels;

		CHECK(atmospheric_fog_settle_fraction(0.0f) == 0.0f, "%g", atmospheric_fog_settle_fraction(0.0f));
		CHECK(atmospheric_fog_settle_fraction(-1.0f) == 0.0f, "%g", atmospheric_fog_settle_fraction(-1.0f));
		CHECK(atmospheric_fog_settle_fraction(1000.0f) < 1.0f, "%g", atmospheric_fog_settle_fraction(1000.0f));
		atmospheric_fog_settle_state(&state, &bridges, 1.0f, atmospheric_fog_settle_fraction(1000.0f));
		CHECK(state.atmospheric_opaque_distance <= 200.0f && state.indoor_fog_scale <= 1.0f, "past it: %g, %g",
			state.atmospheric_opaque_distance, state.indoor_fog_scale);
		return 0;
	}

	fprintf(stderr, "no such case: %s\n", case_name);
	return 2;
}
