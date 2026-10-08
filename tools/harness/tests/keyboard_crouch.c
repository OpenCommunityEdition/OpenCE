/* Actual keyboard input over fake held actions, then the unchanged player-control crouch gate. */
#include "harness.h"
#include <stdint.h>
#include "config.inc"

typedef struct { real i, j; } real_vector2d;
struct biped_datum { struct { unsigned long flags; } biped; };
struct test_input { real_vector2d throttle; unsigned long unit_control_flags; };

#define MIN(a,b) ((a)>(b)?(b):(a))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define PIN(n,floor,ceiling) MAX((floor), MIN((n), (ceiling)))
#define SET_FLAG(f,b,v) ((v) ? ((f)|=(unsigned)FLAG(b)) : ((f)&=(unsigned)~FLAG(b)))

static unsigned long held_actions[MAXIMUM_GAMEPADS];
static long now;
static boolean controls_enable_crouch;
static unsigned long halo_keyboard_actions(short controller_index) { return held_actions[controller_index]; }
static long system_milliseconds(void) { return now; }
#include "under_test.inc"

static struct game_input_state keyboard(unsigned long held)
{
	struct game_input_state state = {0};
	held_actions[0] = held;
	keyboard_controls_update(0, &state);
	return state;
}

static boolean crouches(struct game_input_state state, unsigned long biped_flags, boolean inhibited)
{
	struct biped_datum biped = {{biped_flags}};
	struct test_input input = {{state.forward_movement, state.strafe}, 0};
	if (inhibited) state.buttons[_game_control_crouch] = 0;
	apply_crouch(&biped, &input, state.buttons);
	return TEST_FLAG(input.unit_control_flags, _unit_control_crouch_modifier_bit);
}

static real next_positive_float(real value)
{
	uint32_t bits;
	memcpy(&bits, &value, sizeof(bits));
	bits++;
	memcpy(&value, &bits, sizeof(value));
	return value;
}

static unsigned long direction(long x, long y)
{
	return (x < 0 ? FLAG(HALO_KEYBOARD_STRAFE_LEFT) : x > 0 ? FLAG(HALO_KEYBOARD_STRAFE_RIGHT) : 0) |
		(y < 0 ? FLAG(HALO_KEYBOARD_MOVE_BACKWARD) : y > 0 ? FLAG(HALO_KEYBOARD_MOVE_FORWARD) : 0);
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";
	unsigned long crouch = FLAG(HALO_KEYBOARD_CROUCH);
	unsigned long forward = FLAG(HALO_KEYBOARD_MOVE_FORWARD);
	long x, y;

	CASE("cardinals")
	{
		for (x = -1; x <= 1; x++) for (y = -1; y <= 1; y++) if ((x != 0) != (y != 0))
		{
			struct game_input_state state = keyboard(crouch | direction(x, y));
			CHECK(crouches(state, 0, FALSE), "grounded crouch cancelled for (%ld,%ld)", x, y);
			CHECK(state.forward_movement * y + state.strafe * -x > 0.f, "direction reversed");
		}
	}
	else CASE("diagonals")
	{
		for (x = -1; x <= 1; x += 2) for (y = -1; y <= 1; y += 2)
		{
			struct game_input_state state = keyboard(crouch | direction(x, y));
			CHECK(crouches(state, 0, FALSE), "diagonal crouch cancelled for (%ld,%ld)", x, y);
			CHECK(state.forward_movement * y == state.strafe * -x && state.forward_movement * y > 0.f,
				"diagonal direction changed");
		}
	}
	else CASE("maximum-safe-input")
	{
		struct game_input_state state = keyboard(crouch | forward);
		CHECK(crouches(state, 0, FALSE), "cardinal cap fails gate");
		state.forward_movement = next_positive_float(state.forward_movement);
		CHECK(!crouches(state, 0, FALSE), "cap is below the largest safe cardinal float");
		state = keyboard(crouch | forward | FLAG(HALO_KEYBOARD_STRAFE_LEFT));
		CHECK(crouches(state, 0, FALSE), "diagonal cap fails gate");
		state.forward_movement = next_positive_float(state.forward_movement);
		state.strafe = next_positive_float(state.strafe);
		CHECK(!crouches(state, 0, FALSE), "diagonal components are below the largest safe floats");
	}
	else CASE("run-and-release")
	{
		for (x = -1; x <= 1; x++) for (y = -1; y <= 1; y++) if (x || y)
		{
			struct game_input_state running = keyboard(direction(x, y));
			struct game_input_state walking = keyboard(crouch | direction(x, y));
			struct game_input_state released = keyboard(direction(x, y));
			real component = x && y ? 0.70710678f : 1.f;
			CHECK(running.forward_movement == y * component && running.strafe == -x * component,
				"normal running changed");
			CHECK(crouches(walking, 0, FALSE) && !crouches(released, 0, FALSE), "release keeps crouch");
			CHECK(released.forward_movement == running.forward_movement && released.strafe == running.strafe,
				"release does not restore full movement");
		}
	}
	else CASE("controller-and-mixed-input")
	{
		long controller;
		for (controller = 0; controller < MAXIMUM_GAMEPADS; controller++)
		{
			struct game_input_state state = {0};
			state.forward_movement = 1.f;
			state.buttons[_game_control_crouch] = 7;
			held_actions[controller] = 0;
			keyboard_controls_update(controller, &state);
			CHECK(state.forward_movement == 1.f && !crouches(state, 0, FALSE), "controller full stick changed");
			state.forward_movement = 0.5f;
			keyboard_controls_update(controller, &state);
			CHECK(state.forward_movement == 0.5f && crouches(state, 0, FALSE), "controller partial stick changed");
		}
		{
			struct game_input_state state = {0};
			state.forward_movement = 1.f;
			state.yaw = 0.4f;
			state.pitch = -0.2f;
			held_actions[0] = crouch;
			keyboard_controls_update(0, &state);
			CHECK(state.forward_movement == 1.f && !crouches(state, 0, FALSE), "keyboard crouch scales controller movement");
			state.buttons[_game_control_crouch] = 7;
			held_actions[0] = forward;
			keyboard_controls_update(0, &state);
			CHECK(state.forward_movement == 1.f && !crouches(state, 0, FALSE), "controller crouch scales keyboard movement");
			held_actions[0] = forward | crouch;
			keyboard_controls_update(0, &state);
			CHECK(crouches(state, 0, FALSE), "mixed input overrides keyboard cap");
			CHECK(state.yaw == 0.4f && state.pitch == -0.2f && state.buttons[_game_control_crouch] == 7,
				"look or controller button precedence changed");
		}
	}
	else CASE("cancelled-keys")
	{
		struct game_input_state state = {0};
		state.forward_movement = 0.4f;
		state.strafe = -0.3f;
		held_actions[0] = crouch | forward | FLAG(HALO_KEYBOARD_MOVE_BACKWARD) |
			FLAG(HALO_KEYBOARD_STRAFE_LEFT) | FLAG(HALO_KEYBOARD_STRAFE_RIGHT);
		keyboard_controls_update(0, &state);
		CHECK(state.forward_movement == 0.4f && state.strafe == -0.3f, "opposing keys change analog fallback");
		CHECK(crouches(state, 0, FALSE), "stationary keyboard crouch fails");
	}
	else CASE("hold-across-frame-rates")
	{
		long rates[] = {15, 30, 60, 144, 240, 1000};
		long rate, frame;
		for (rate = 0; rate < (long)(sizeof(rates) / sizeof(*rates)); rate++)
		{
			memset(keyboard_controls, 0, sizeof(keyboard_controls));
			for (frame = 0; frame < rates[rate] * 5; frame++)
			{
				struct game_input_state state;
				now = frame * 1000 / rates[rate];
				state = keyboard(crouch | forward | FLAG(HALO_KEYBOARD_RELOAD));
				CHECK(crouches(state, 0, FALSE), "held crouch fails at %ld FPS, frame %ld", rates[rate], frame);
				CHECK(keyboard_controls[0].reload_ticks != 0, "reload lost");
			}
			CHECK(keyboard(0).buttons[_game_control_crouch] == 0 && keyboard_controls[0].reload_ticks == 0,
				"release retains held buttons");
		}
	}
	else CASE("crouch-gate-exceptions")
	{
		struct game_input_state state = keyboard(forward);
		struct test_input input = {{0.f, 0.f}, 0};
		state.buttons[_game_control_crouch] = 1;
		CHECK(crouches(state, FLAG(_biped_airborne_bit), FALSE), "airborne crouch changed");
		controls_enable_crouch = TRUE;
		CHECK(crouches(state, 0, FALSE), "script-enabled full-speed crouch changed");
		state = keyboard(crouch | forward);
		CHECK(!crouches(state, 0, TRUE), "inhibited crouch bypassed");
		apply_crouch(NULL, &input, state.buttons);
		CHECK(input.unit_control_flags == 0, "non-biped gets crouch");
	}
	else { fprintf(stderr, "unknown case: %s\n", case_name); return 2; }
	return 0;
}
