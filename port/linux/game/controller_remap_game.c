/*
CONTROLLER_REMAP_GAME.C

The game's side of the controllers' own button layouts (controller_remap.h):
the profile's layout a controller's is moved onto, whether a controller's
player is playing, and the HUD's prompts' buttons. A layout never changes
the profile.

A controller is a port (xinput_sdl.c's); its player is the local player
the single player game gave it, else the local player of its number, as
player_ui.c's set_local_player_controls_from_player_profile has it.
*/

#include "cseries.h"
#include "controller_buttons.h"
#include "controller_remap.h"
#include "input/input_abstraction.h"
#include "interface/player_ui.h"
#include "interface/ui_widget.h"
#include "game/game.h"

/* port/linux/game/network_chat.c */
unsigned char network_chat_keyboard_open(void);

/* the game's controls (input_abstraction.c's) */
enum
{
	_remap_control_jump,
	_remap_control_switch_grenades,
	_remap_control_action,
	_remap_control_switch_weapons,
	_remap_control_melee,
	_remap_control_flashlight,
	_remap_control_grenade,
	_remap_control_primary_trigger,
	_remap_control_start,
	_remap_control_back,
	_remap_control_crouch,
	_remap_control_zoom
};

/* the controls a layout moves, in its order (controller_remap.h) */
static unsigned char const remap_controls[CONTROLLER_REMAP_ACTIONS] =
{
	_remap_control_jump, _remap_control_switch_grenades, _remap_control_action, _remap_control_switch_weapons,
	_remap_control_melee, _remap_control_flashlight, _remap_control_grenade, _remap_control_primary_trigger,
	_remap_control_crouch, _remap_control_zoom
};

/* the standard button layout's buttons for them (player_ui.c's) */
static unsigned char const standard_layout[CONTROLLER_REMAP_ACTIONS] =
{
	_remap_button_a, _remap_button_black, _remap_button_x, _remap_button_y, _remap_button_b, _remap_button_white,
	_remap_button_left_trigger, _remap_button_right_trigger, _remap_button_left_thumb, _remap_button_right_thumb
};

static short controller_local_player(short port)
{
	short local_player_index = player_ui_get_single_player_local_player_from_controller(port);

	return local_player_index != NONE ? local_player_index : port;
}

static short local_player_controller(short local_player_index)
{
	short port = player_ui_get_single_player_local_player_controller(local_player_index);

	return port != NONE ? port : local_player_index;
}

void controller_remap_profile(short port, unsigned char *buttons)
{
	struct game_input_preferences preferences;
	unsigned int used = 0;
	short action;

	input_abstraction_get_local_player_preferences(port, &preferences);
	for (action = 0; action < CONTROLLER_REMAP_ACTIONS; action++)
	{
		byte button = preferences.game_control_to_xbox_buttons[remap_controls[action]];

		if (!controller_remap_bindable(button) || (used & (1u << button)))
			break;
		used |= 1u << button;
		buttons[action] = button;
	}
	/* (a layout a layout cannot be moved onto: the menus' profile, before
	the game has given the controller one, has none) */
	if (action < CONTROLLER_REMAP_ACTIONS)
		csmemcpy(buttons, standard_layout, sizeof(standard_layout));
}

int controller_remap_playing(short port)
{
	return game_in_progress() && !ui_widgets_active_for_local_player(controller_local_player(port)) &&
		!(port == 0 && network_chat_keyboard_open());
}

void controller_remap_prompt(short local_player_index, struct game_input_preferences *preferences)
{
	unsigned char buttons[CONTROLLER_REMAP_ACTIONS];
	short action;

	/* (as many local players as ports) */
	if (local_player_index < 0 || local_player_index >= CONTROLLER_PORTS)
		return;
	if (!controller_remap_read(local_player_controller(local_player_index), buttons))
		return;
	for (action = 0; action < CONTROLLER_REMAP_ACTIONS; action++)
		preferences->game_control_to_xbox_buttons[remap_controls[action]] = buttons[action];
}
