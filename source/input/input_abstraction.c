/*
INPUT_ABSTRACTION.C

symbols in this file:
000BDA30 0020:
	_input_abstraction_dispose (0000)
000BDA50 0010:
	_input_abstraction_reset_controller_detection_timer (0000)
000BDA60 0080:
	_input_abstraction_get_local_player_preferences (0000)
000BDAE0 00b0:
	_input_abstraction_update_local_player_preferences (0000)
000BDB90 0050:
	_input_abstraction_get_input_state (0000)
000BDBE0 0080:
	_input_abstraction_update_device_changes (0000)
000BDC60 00e0:
	_code_000bdc60 (0000)
000BDD40 0050:
	_code_000bdd40 (0000)
000BDD90 00a0:
	_input_abstraction_initialize (0000)
000BDE30 0950:
	_input_abstraction_update (0000)
0026F3A0 0014:
	_rdata_0026f3a0 (0000)
0026F3B4 000c:
	??_C@_0M@OAMDCHEH@preferences?$AA@ (0000)
0026F3C0 0041:
	??_C@_0EB@NBKADHOE@?$CIlocal_player_index?$DO?$DN0?$CJ?5?$CG?$CG?5?$CIloca@ (0000)
0026F404 0029:
	??_C@_0CJ@KPJFDJEG@c?3?2halo?2SOURCE?2input?2input_abstr@ (0000)
0026F430 0041:
	??_C@_0EB@LOCNLGGH@invalid?5controller?5preferences?$DL?5@ (0000)
0026F474 003d:
	??_C@_0DN@PDKODJDH@?$CIcontroller_index?$DO?$DN0?$CJ?5?$CG?$CG?5?$CIcontro@ (0000)
0026F4B4 0039:
	??_C@_0DJ@DKGJGJAM@stopping?5bink?5playback?5to?5due?5to@ (0000)
0026F4F0 004c:
	??_C@_0EM@DKLJMFDI@?$CIcontroller_index?$DO?$DN0?$CJ?5?$CG?$CG?5?$CIcontro@ (0000)
0026F53C 0018:
	??_C@_0BI@DEJBIOHJ@unknown?5joystick?5preset?$AA@ (0000)
0026F558 0008:
	__real@3fc6571840000000 (0000)
0026F560 0008:
	__real@3ffa313e30a3879f (0000)
0026F568 0008:
	__real@3fe38c3550000000 (0000)
004535C0 00e0:
	_input_abstraction_globals (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "bink/bink_playback.h"
#include "cseries/errors.h"
#include "input/input.h"
#include "math/real_math.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct game_input_preferences
{
	real yaw_rate;
	real pitch_rate;
	byte game_control_to_xbox_buttons[12];
	short joystick_controls;
	boolean invert_look;
	boolean invert_look_aircraft_control;
};

struct game_input_state
{
	byte buttons[12];
	real forward_movement;
	real strafe;
	real yaw;
	real pitch;
};

struct input_abstraction_runtime_globals
{
	struct game_input_preferences player_control_preferences[MAXIMUM_GAMEPADS];
	struct game_input_state input_states[MAXIMUM_GAMEPADS];
	unsigned long device_enumeration_startup_timer;
	boolean controller_available[MAXIMUM_GAMEPADS];
	boolean initialized;
	unsigned long time_of_first_device_insertion;
};

typedef char verify_game_input_preferences_size[
	sizeof(struct game_input_preferences) == 0x18 ? 1 : -1];
typedef char verify_game_input_state_size[
	sizeof(struct game_input_state) == 0x1C ? 1 : -1];
typedef char verify_input_abstraction_input_states_offset[
	offsetof(struct input_abstraction_runtime_globals, input_states) == 0x60 ? 1 : -1];
typedef char verify_input_abstraction_device_timer_offset[
	offsetof(struct input_abstraction_runtime_globals, device_enumeration_startup_timer) == 0xD0 ? 1 : -1];
typedef char verify_input_abstraction_controller_available_offset[
	offsetof(struct input_abstraction_runtime_globals, controller_available) == 0xD4 ? 1 : -1];
typedef char verify_input_abstraction_initialized_offset[
	offsetof(struct input_abstraction_runtime_globals, initialized) == 0xD8 ? 1 : -1];
typedef char verify_input_abstraction_first_insertion_offset[
	offsetof(struct input_abstraction_runtime_globals, time_of_first_device_insertion) == 0xDC ? 1 : -1];
typedef char verify_input_abstraction_runtime_globals_size[
	sizeof(struct input_abstraction_runtime_globals) == 0xE0 ? 1 : -1];

/* ---------- prototypes */

unsigned long system_milliseconds(
	void);

/* ---------- globals */

extern struct input_abstraction_runtime_globals input_abstraction_globals;

/* ---------- public code */

void input_abstraction_dispose(
	void)
{
	csmemset(
		&input_abstraction_globals,
		0,
		offsetof(struct input_abstraction_runtime_globals, time_of_first_device_insertion));

	return;
}

void input_abstraction_reset_controller_detection_timer(
	void)
{
	input_abstraction_globals.device_enumeration_startup_timer = system_milliseconds();

	return;
}

void input_abstraction_get_local_player_preferences(
	short local_player_index,
	struct game_input_preferences *preferences)
{
	match_assert(
		"c:\\halo\\SOURCE\\input\\input_abstraction.c",
		495,
		(local_player_index>=0) && (local_player_index<MAXIMUM_GAMEPADS));
	match_assert(
		"c:\\halo\\SOURCE\\input\\input_abstraction.c",
		496,
		preferences);

	csmemcpy(
		preferences,
		&input_abstraction_globals.player_control_preferences[local_player_index],
		sizeof(*preferences));

	return;
}

void input_abstraction_update_local_player_preferences(
	short controller_index,
	struct game_input_preferences const *preferences)
{
	match_assert(
		"c:\\halo\\SOURCE\\input\\input_abstraction.c",
		507,
		(controller_index>=0) && (controller_index<MAXIMUM_GAMEPADS));
	match_assert(
		"c:\\halo\\SOURCE\\input\\input_abstraction.c",
		508,
		preferences);
	match_vassert(
		"c:\\halo\\SOURCE\\input\\input_abstraction.c",
		511,
		preferences->game_control_to_xbox_buttons[8] == _gamepad_binary_button_start &&
			preferences->game_control_to_xbox_buttons[9] == _gamepad_binary_button_back,
		"invalid controller preferences; can't remap start & back buttons");

	csmemcpy(
		&input_abstraction_globals.player_control_preferences[controller_index],
		preferences,
		sizeof(*preferences));

	return;
}

struct game_input_state *input_abstraction_get_input_state(
	short local_player_index)
{
	match_assert(
		"c:\\halo\\SOURCE\\input\\input_abstraction.c",
		521,
		(local_player_index>=0) && (local_player_index<MAXIMUM_GAMEPADS));

	return &input_abstraction_globals.input_states[local_player_index];
}

void input_abstraction_update_device_changes(
	unsigned long device_change_flags)
{
	if (input_abstraction_globals.initialized)
	{
		if (device_change_flags &&
			(system_milliseconds() - input_abstraction_globals.device_enumeration_startup_timer >= 2000 ||
			(input_abstraction_globals.time_of_first_device_insertion &&
			system_milliseconds() - input_abstraction_globals.time_of_first_device_insertion >= 2000)))
		{
			error(
				_error_silent,
				"stopping bink playback to due to change in input devices");
			bink_playback_stop();
		}

		if ((device_change_flags & 0xFFF000) &&
			!input_abstraction_globals.time_of_first_device_insertion)
		{
			input_abstraction_globals.time_of_first_device_insertion = system_milliseconds();
		}
	}

	return;
}

/* ---------- private code */
