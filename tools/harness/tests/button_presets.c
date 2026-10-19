/*
BUTTON_PRESETS.C (test)

The real button settings of source/interface/player_ui.c
(set_local_player_controls_from_player_profile): the game control on each
of the controller's buttons for each of a profile's button layouts, the
port's Bumper Jumper too. test_button_presets.py takes the enums and
structures (config.inc) and the code (under_test.inc).
*/

#include "harness.h"

typedef unsigned short word;

#include "config.inc"

#define MAXIMUM_NUMBER_OF_LOCAL_PLAYERS 4

struct player_profile
{
	struct player_profile_controller_settings controller_settings;
};

static struct
{
	struct
	{
		struct player_profile profile;
	} local_players[MAXIMUM_NUMBER_OF_LOCAL_PLAYERS];
	short single_player_controller[MAXIMUM_NUMBER_OF_LOCAL_PLAYERS];
} player_ui_globals;

static struct game_input_preferences applied;
static short applied_controller;

static void input_abstraction_update_local_player_preferences(
	short controller_index,
	struct game_input_preferences const *preferences)
{
	applied_controller = controller_index;
	applied = *preferences;
}

#include "under_test.inc"

/* the Master Chief Collection's Bumper Jumper for Halo: Combat Evolved; the
left and right shoulders are the Duke's white and black (xinput_sdl.c) */
static byte const bumper_jumper[NUMBER_OF_GAME_CONTROLS] =
{
	[_game_control_jump] = _gamepad_analog_button_white,
	[_game_control_switch_grenades] = _gamepad_analog_button_a,
	[_game_control_action] = _gamepad_analog_button_b,
	[_game_control_switch_weapons] = _gamepad_analog_button_y,
	[_game_control_melee] = _gamepad_analog_button_black,
	[_game_control_flashlight] = _gamepad_analog_button_x,
	[_game_control_grenade] = _gamepad_analog_button_left_trigger,
	[_game_control_primary_trigger] = _gamepad_analog_button_right_trigger,
	[_game_control_start] = _gamepad_binary_button_start,
	[_game_control_back] = _gamepad_binary_button_back,
	[_game_control_crouch] = _gamepad_binary_button_left_thumb,
	[_game_control_zoom] = _gamepad_binary_button_right_thumb,
};

static void apply(
	byte preset)
{
	memset(&applied, 0xFF, sizeof(applied));
	applied_controller = NONE;
	player_ui_globals.local_players[0].profile.controller_settings.button_preset = preset;
	player_ui_globals.local_players[0].profile.controller_settings.look_sensitivity = 3;
	player_ui_globals.single_player_controller[0] = NONE;
	set_local_player_controls_from_player_profile(0);
	CHECK(applied_controller == 0, "preset %d was not given to the controller", preset);
}

/* start and back where input_abstraction_update_local_player_preferences
requires them (or the game stops), and no two controls on one button */
static void playable(
	byte preset)
{
	short control;
	short other;

	CHECK(applied.game_control_to_xbox_buttons[_game_control_start] == _gamepad_binary_button_start &&
		applied.game_control_to_xbox_buttons[_game_control_back] == _gamepad_binary_button_back,
		"preset %d moved start or back", preset);
	for (control = 0; control < NUMBER_OF_GAME_CONTROLS; control++)
	{
		CHECK(applied.game_control_to_xbox_buttons[control] < NUMBER_OF_GAMEPAD_BUTTONS,
			"preset %d put control %d on no button", preset, control);
		for (other = control + 1; other < NUMBER_OF_GAME_CONTROLS; other++)
		{
			CHECK(applied.game_control_to_xbox_buttons[control] != applied.game_control_to_xbox_buttons[other],
				"preset %d put controls %d and %d on one button", preset, control, other);
		}
	}
}

int main(
	int argc,
	char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	CASE("every-preset")
	{
		byte preset;

		for (preset = 0; preset < NUMBER_OF_BUTTON_PRESETS; preset++)
		{
			apply(preset);
			playable(preset);
		}
		return 0;
	}
	CASE("bumper-jumper")
	{
		short control;

		apply(_button_preset_bumper_jumper);
		for (control = 0; control < NUMBER_OF_GAME_CONTROLS; control++)
		{
			CHECK(applied.game_control_to_xbox_buttons[control] == bumper_jumper[control],
				"control %d is on button %d, not %d", control,
				applied.game_control_to_xbox_buttons[control], bumper_jumper[control]);
		}
		return 0;
	}
	CASE("unknown-preset")
	{
		struct game_input_preferences standard;

		apply(_button_preset_standard);
		standard = applied;
		apply(NUMBER_OF_BUTTON_PRESETS);
		playable(NUMBER_OF_BUTTON_PRESETS);
		CHECK(!memcmp(applied.game_control_to_xbox_buttons, standard.game_control_to_xbox_buttons,
			sizeof(standard.game_control_to_xbox_buttons)), "an unknown preset is not the standard one");
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
