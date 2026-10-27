/*
CONTROLLER_REMAP.H

A controller's own button layout (input.controller_p1 to _p4, set on
Settings > Gamepad Setup > Buttons > Button Remapping): which of its buttons
does each of ten of the game's controls, kept apart from the profile's
button layout, which still drives the menus and every control left out
(start, back, the sticks' moving and looking). The layout moves the
controller's buttons before the keyboard's and the touch controls' are
added (port/linux/src/controller_remap_input.c, called by xinput_sdl.c), so
those keep their own bindings; the HUD's prompts name the moved buttons
(port/linux/game/controller_remap_game.c). An empty or unreadable setting
is the profile's layout.
*/

#ifndef CONTROLLER_REMAP_H
#define CONTROLLER_REMAP_H

/* the game's controls a layout moves, in the setting's order: jump, switch
grenades, action (use and reload), switch weapons, melee, flashlight,
grenade, fire, crouch and zoom (controller_remap_game.c's game controls) */
#define CONTROLLER_REMAP_ACTIONS 10
/* the buttons they can be on (controller_remap.c's names) */
#define CONTROLLER_REMAP_BUTTONS 11
/* a layout as its setting's text: ten names, the longest "dpad_left" */
#define CONTROLLER_REMAP_TEXT 128

/* the controller's buttons, in source/input/input.h's order (its gamepad
buttons'), which a profile's layout (game_input_preferences) names too */
enum controller_remap_button
{
	_remap_button_a,
	_remap_button_b,
	_remap_button_x,
	_remap_button_y,
	_remap_button_black,
	_remap_button_white,
	_remap_button_left_trigger,
	_remap_button_right_trigger,
	_remap_button_dpad_up,
	_remap_button_dpad_down,
	_remap_button_dpad_left,
	_remap_button_dpad_right,
	_remap_button_start,
	_remap_button_back,
	_remap_button_left_thumb,
	_remap_button_right_thumb,
	NUMBER_OF_REMAP_BUTTONS
};

/* what a capture ended with (controller_remap_capture_poll) */
enum controller_remap_capture_result
{
	_remap_capture_waiting,
	_remap_capture_bound,
	_remap_capture_cancelled
};

/* ---------- layouts (controller_remap.c) */

/* a port's setting, input.controller_p<port + 1> */
char const *controller_remap_setting(short port);
/* a layout from its text, or FALSE (and the layout left) where it does not
name ten different buttons */
int controller_remap_parse(char const *text, unsigned char *buttons);
/* a layout as its text, empty where it has a button no name has */
void controller_remap_format(unsigned char const *buttons, char *text, unsigned int size);
/* a port's layout, or FALSE where it has none (the profile's) */
int controller_remap_read(short port, unsigned char *buttons);
/* whether a layout can use a button (not start, back or the d-pad's up,
down and right, which the menus and the game's controls keep) */
int controller_remap_bindable(int button);
/* an action moved to a button, the action that had it taking the action's */
void controller_remap_swap(unsigned char *buttons, int action, int button);
/* the button before or after one, of those a layout can use */
int controller_remap_cycle(int button, int direction);
/* a button's name on a family's controller (controller_buttons.h) */
char const *controller_remap_button_name(int button, int family);

/* ---------- the game's side (controller_remap_game.c) */

/* the profile's layout for a port's controller, as a layout */
void controller_remap_profile(short port, unsigned char *buttons);
/* whether a port's player is playing (not in a menu, not typing a chat
line): only then is a layout used */
int controller_remap_playing(short port);
/* the HUD's prompts' buttons for a local player: their controller's
layout's, where it has one */
struct game_input_preferences;
void controller_remap_prompt(short local_player_index, struct game_input_preferences *preferences);

/* ---------- the controller (controller_remap_input.c) */

/* a port's pad (as the controller has it; connected: whether it has one)
moved to its layout while its player plays; TRUE while a capture takes the
pad, when the port reads nothing at all */
struct _XINPUT_GAMEPAD;
int controller_remap_input(short port, struct _XINPUT_GAMEPAD *pad, int connected);
/* the next button pressed on a port's controller, for the menu */
void controller_remap_capture_begin(short port);
/* a controller_remap_capture_result, with the button where it is bound */
int controller_remap_capture_poll(int *button);
void controller_remap_capture_end(void);

#endif
