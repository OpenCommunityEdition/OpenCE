/*
CONTROLLER_REMAP.C (test)

The real controller button layouts of port/linux/src/controller_remap.c
(their text, swapping and cycling), controller_remap_input.c (a playing
player's pad moved to their layout, and the capture of a button) and
port/linux/game/controller_remap_game.c (the profile's layout, the
controller's player, the HUD's prompts), against a fake pad, clock,
settings and game. test_controller_remap.py takes the game's enums, the
headers and the pad (config.inc) and the code (layouts.inc, input.inc,
game.inc).
*/

#include "harness.h"

#include <stdarg.h>

#define NUMBEROF(array) (sizeof(array) / sizeof((array)[0]))

typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef short SHORT;
typedef unsigned long long Uint64;

#define csmemcpy memcpy

#include "config.inc"

/* ---------- fakes: the settings, the clock, the controllers, the keyboard's capture */

static char const *settings[CONTROLLER_PORTS] = { "", "", "", "" };
static unsigned long changes;

static char const *config_string(char const *name)
{
	CHECK(!strncmp(name, "input.controller_p", 18) && name[18] >= '1' && name[18] <= '4' && !name[19],
		"an unknown setting was read: %s", name);
	return settings[name[18] - '1'];
}

static unsigned long config_changes(void)
{
	return changes;
}

static void set_layout(short port, char const *text)
{
	settings[port] = text;
	changes++;
}

static Uint64 now = 1000;

static Uint64 SDL_GetTicks(void)
{
	return now;
}

static struct controller_device devices[CONTROLLER_PORTS];

void controller_device_get(short port, struct controller_device *device)
{
	if (port >= 0 && port < CONTROLLER_PORTS)
		*device = devices[port];
	else
		memset(device, 0, sizeof(*device));
}

static int keyboard_capturing;
static int keyboard_result;

static void platform_binding_capture_begin(void)
{
	keyboard_capturing = 1;
}

static void platform_binding_capture_cancel(void)
{
	keyboard_capturing = 0;
}

static int platform_binding_capture_poll(int *input)
{
	int result = keyboard_result;

	*input = 0;
	keyboard_result = 0;
	if (result)
		keyboard_capturing = 0;
	return result;
}

/* ---------- fakes: the game */

static struct game_input_preferences profiles[CONTROLLER_PORTS];
static boolean in_game;
static boolean menus_up[CONTROLLER_PORTS];
static unsigned char chat_open;
static short single_player_controller[CONTROLLER_PORTS] = { NONE, NONE, NONE, NONE };

static void input_abstraction_get_local_player_preferences(short index, struct game_input_preferences *preferences)
{
	CHECK(index >= 0 && index < CONTROLLER_PORTS, "the preferences of controller %d", index);
	*preferences = profiles[index];
}

static boolean game_in_progress(void)
{
	return in_game;
}

static boolean ui_widgets_active_for_local_player(short local_player_index)
{
	CHECK(local_player_index >= 0 && local_player_index < CONTROLLER_PORTS, "local player %d", local_player_index);
	return menus_up[local_player_index];
}

static unsigned char network_chat_keyboard_open(void)
{
	return chat_open;
}

static short player_ui_get_single_player_local_player_controller(short local_player_index)
{
	return single_player_controller[local_player_index];
}

static short player_ui_get_single_player_local_player_from_controller(short controller_index)
{
	short local_player_index;

	for (local_player_index = 0; local_player_index < CONTROLLER_PORTS; local_player_index++)
	{
		if (single_player_controller[local_player_index] == controller_index)
			return local_player_index;
	}
	return NONE;
}

#include "layouts.inc"
#include "input.inc"
#include "game.inc"

/* ---------- helpers */

/* the standard button layout (player_ui.c's) */
static void standard_profile(short port)
{
	static byte const standard[12] =
	{
		_gamepad_analog_button_a, _gamepad_analog_button_black, _gamepad_analog_button_x, _gamepad_analog_button_y,
		_gamepad_analog_button_b, _gamepad_analog_button_white, _gamepad_analog_button_left_trigger,
		_gamepad_analog_button_right_trigger, _gamepad_binary_button_start, _gamepad_binary_button_back,
		_gamepad_binary_button_left_thumb, _gamepad_binary_button_right_thumb
	};

	memcpy(profiles[port].game_control_to_xbox_buttons, standard, sizeof(standard));
}

static void reset(void)
{
	short port;

	controller_remap_capture_end();
	for (port = 0; port < CONTROLLER_PORTS; port++)
	{
		settings[port] = "";
		standard_profile(port);
		devices[port].connected = 1;
		devices[port].family = _controller_xbox;
		devices[port].instance = 10 + port;
		menus_up[port] = FALSE;
		single_player_controller[port] = NONE;
	}
	changes++;
	in_game = TRUE;
	chat_open = 0;
	keyboard_capturing = 0;
	keyboard_result = 0;
}

/* a pad with buttons held (each a controller_remap_button) */
static XINPUT_GAMEPAD pad_with(int count, ...)
{
	XINPUT_GAMEPAD pad;
	va_list buttons;
	int index;

	memset(&pad, 0, sizeof(pad));
	va_start(buttons, count);
	for (index = 0; index < count; index++)
	{
		int button = va_arg(buttons, int);

		if (button < _remap_button_dpad_up)
			pad.bAnalogButtons[button] = 0xff;
		else
			pad.wButtons |= digital_masks[button - _remap_button_dpad_up];
	}
	va_end(buttons);
	return pad;
}

static int pressed(XINPUT_GAMEPAD const *pad, int button)
{
	return button < _remap_button_dpad_up ? pad->bAnalogButtons[button] > REMAP_PRESSED :
		(pad->wButtons & digital_masks[button - _remap_button_dpad_up]) != 0;
}

/* the pad a port's player gets for one held on its controller */
static XINPUT_GAMEPAD played(short port, XINPUT_GAMEPAD pad)
{
	controller_remap_input(port, &pad, 1);
	return pad;
}

/* the jump and melee swapped (east jumps, south melees), all else standard */
#define JUMP_ON_EAST "east,rb,west,north,south,lb,lt,rt,ls,rs"

int main(
	int argc,
	char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	CASE("layout-text")
	{
		static char const *const refused[] =
		{
			"", "south", "south,rb,west,north,east,lb,lt,rt,ls", "south,rb,west,north,east,lb,lt,rt,ls,rs,dpad_left",
			"south,south,west,north,east,lb,lt,rt,ls,rs", "south,rb,west,north,east,lb,lt,rt,ls,start",
			"south,rb,west,north,east,lb,lt,rt,ls,rs,", "south,rb,west,north,east,lb,lt,rt,ls, rs",
		};
		unsigned char buttons[CONTROLLER_REMAP_ACTIONS], kept[CONTROLLER_REMAP_ACTIONS];
		char text[CONTROLLER_REMAP_TEXT];
		size_t index;

		CHECK(controller_remap_parse("south,rb,west,north,east,lb,lt,rt,ls,rs", buttons), "the standard layout is refused");
		CHECK(!memcmp(buttons, standard_layout, sizeof(buttons)), "the standard layout is not the profile's standard");
		CHECK(controller_remap_parse("dpad_left,rb,west,north,east,lb,lt,rt,ls,rs", buttons) &&
			buttons[0] == _remap_button_dpad_left, "the d-pad's left is refused");
		controller_remap_format(buttons, text, sizeof(text));
		CHECK(!strcmp(text, "dpad_left,rb,west,north,east,lb,lt,rt,ls,rs"), "the layout is written as %s", text);
		memcpy(kept, buttons, sizeof(kept));
		for (index = 0; index < NUMBEROF(refused); index++)
		{
			CHECK(!controller_remap_parse(refused[index], buttons), "%s is taken", refused[index]);
			CHECK(!memcmp(buttons, kept, sizeof(kept)), "%s changed the layout", refused[index]);
		}
		CHECK(!controller_remap_parse(NULL, buttons), "no text is taken");
		/* (a layout with a button no name has, or too little room, is written empty) */
		buttons[3] = _remap_button_start;
		controller_remap_format(buttons, text, sizeof(text));
		CHECK(!*text, "a layout with start is written as %s", text);
		controller_remap_format(kept, text, 12);
		CHECK(!*text, "a layout is cut short to %s", text);
		CHECK(strlen("dpad_left,south,east,west,north,rb,lb,lt,rt,ls") < CONTROLLER_REMAP_TEXT,
			"the longest layout does not fit");
		return 0;
	}
	CASE("swap-cycle")
	{
		unsigned char buttons[CONTROLLER_REMAP_ACTIONS];
		int button, steps;

		memcpy(buttons, standard_layout, sizeof(buttons));
		/* (melee to the jump's button: the jump takes the melee's) */
		controller_remap_swap(buttons, 4, _remap_button_a);
		CHECK(buttons[4] == _remap_button_a && buttons[0] == _remap_button_b, "the swap left %d and %d",
			buttons[4], buttons[0]);
		/* (a button no layout has is refused, as is an action out of range) */
		controller_remap_swap(buttons, 1, _remap_button_start);
		controller_remap_swap(buttons, CONTROLLER_REMAP_ACTIONS, _remap_button_x);
		CHECK(buttons[1] == _remap_button_black && buttons[2] == _remap_button_x, "a refused swap changed the layout");
		/* (each way round, through every button a layout can have, back) */
		for (steps = 0, button = _remap_button_a; steps < CONTROLLER_REMAP_BUTTONS; steps++)
		{
			button = controller_remap_cycle(button, 1);
			CHECK(controller_remap_bindable(button), "cycling reached button %d", button);
		}
		CHECK(button == _remap_button_a, "cycling forward did not come back: %d", button);
		for (steps = 0; steps < CONTROLLER_REMAP_BUTTONS; steps++)
			button = controller_remap_cycle(button, -1);
		CHECK(button == _remap_button_a, "cycling back did not come back: %d", button);
		CHECK(controller_remap_cycle(_remap_button_a, -1) == _remap_button_dpad_left, "before A is %d",
			controller_remap_cycle(_remap_button_a, -1));
		return 0;
	}
	CASE("names")
	{
		CHECK(!strcmp(controller_remap_button_name(_remap_button_a, _controller_xbox), "A") &&
			!strcmp(controller_remap_button_name(_remap_button_a, _controller_playstation), "CROSS") &&
			!strcmp(controller_remap_button_name(_remap_button_a, _controller_switch), "B") &&
			!strcmp(controller_remap_button_name(_remap_button_y, _controller_switch), "X") &&
			!strcmp(controller_remap_button_name(_remap_button_left_trigger, _controller_playstation), "L2"),
			"a button has the wrong name");
		CHECK(!strcmp(controller_remap_button_name(_remap_button_b, _controller_unknown), "B") &&
			!strcmp(controller_remap_button_name(_remap_button_b, 99), "B"), "an unknown controller is not named an Xbox's");
		CHECK(!strcmp(controller_remap_button_name(_remap_button_start, _controller_xbox), "-"), "start is named");
		CHECK(!strcmp(controller_remap_setting(0), "input.controller_p1") && !strcmp(controller_remap_setting(3),
			"input.controller_p4") && !controller_remap_setting(4), "the settings are misnamed");
		return 0;
	}
	CASE("settings-cached")
	{
		unsigned char buttons[CONTROLLER_REMAP_ACTIONS];

		reset();
		CHECK(!controller_remap_read(1, buttons), "an empty setting is a layout");
		set_layout(1, JUMP_ON_EAST);
		CHECK(controller_remap_read(1, buttons) && buttons[0] == _remap_button_b, "player 2's layout is not read");
		CHECK(!controller_remap_read(0, buttons) && !controller_remap_read(4, buttons), "another port has the layout");
		/* (read again only once a setting changes) */
		settings[1] = "nonsense";
		CHECK(controller_remap_read(1, buttons), "the layout was read again unchanged");
		changes++;
		CHECK(!controller_remap_read(1, buttons), "an unreadable layout is used");
		return 0;
	}
	CASE("game-profile")
	{
		unsigned char buttons[CONTROLLER_REMAP_ACTIONS];

		reset();
		/* (the game's controls are input_abstraction.c's, the buttons input.h's) */
		CHECK(_remap_control_jump == _game_control_jump && _remap_control_primary_trigger == _game_control_primary_trigger &&
			_remap_control_crouch == _game_control_crouch && _remap_control_zoom == _game_control_zoom &&
			_remap_control_start == _game_control_start, "a control is not the game's");
		CHECK(_remap_button_black == _gamepad_analog_button_black && _remap_button_dpad_left == _gamepad_binary_button_dpad_left &&
			_remap_button_right_thumb == _gamepad_binary_button_right_thumb && NUMBER_OF_REMAP_BUTTONS == NUMBER_OF_GAMEPAD_BUTTONS,
			"a button is not the game's");
		controller_remap_profile(2, buttons);
		CHECK(!memcmp(buttons, standard_layout, sizeof(buttons)), "the standard profile is not the standard layout");
		/* (a profile's own layout, as it is) */
		profiles[2].game_control_to_xbox_buttons[_game_control_jump] = _gamepad_analog_button_white;
		profiles[2].game_control_to_xbox_buttons[_game_control_flashlight] = _gamepad_analog_button_a;
		controller_remap_profile(2, buttons);
		CHECK(buttons[0] == _remap_button_white && buttons[5] == _remap_button_a, "the profile's layout is not used");
		/* (one with a control on a button no layout has, or on another's: the standard) */
		profiles[2].game_control_to_xbox_buttons[_game_control_zoom] = _gamepad_binary_button_start;
		controller_remap_profile(2, buttons);
		CHECK(!memcmp(buttons, standard_layout, sizeof(buttons)), "a profile with zoom on start is used");
		standard_profile(2);
		profiles[2].game_control_to_xbox_buttons[_game_control_melee] = _gamepad_analog_button_a;
		controller_remap_profile(2, buttons);
		CHECK(!memcmp(buttons, standard_layout, sizeof(buttons)), "a profile with two controls on A is used");
		return 0;
	}
	CASE("playing-player")
	{
		struct game_input_preferences preferences;

		reset();
		/* (controller 3 plays the single player game's player 1) */
		single_player_controller[0] = 2;
		CHECK(controller_remap_playing(2), "controller 3 does not play");
		menus_up[0] = TRUE;
		CHECK(!controller_remap_playing(2), "controller 3 plays with player 1's menu up");
		menus_up[0] = FALSE;
		menus_up[2] = TRUE;
		CHECK(controller_remap_playing(2), "controller 3 stops for local player 3's menu");
		menus_up[2] = FALSE;
		chat_open = 1;
		CHECK(!controller_remap_playing(0) && controller_remap_playing(2), "the chat keyboard stops the wrong port");
		chat_open = 0;
		in_game = FALSE;
		CHECK(!controller_remap_playing(2), "a controller plays with no game");
		/* (the prompts name player 1's controller's buttons: controller 3's layout) */
		set_layout(2, JUMP_ON_EAST);
		preferences = profiles[0];
		controller_remap_prompt(0, &preferences);
		CHECK(preferences.game_control_to_xbox_buttons[_game_control_jump] == _gamepad_analog_button_b &&
			preferences.game_control_to_xbox_buttons[_game_control_melee] == _gamepad_analog_button_a &&
			preferences.game_control_to_xbox_buttons[_game_control_start] == _gamepad_binary_button_start,
			"player 1's prompts are not controller 3's layout");
		/* (local player 2: controller 2's, which has no layout) */
		preferences = profiles[1];
		controller_remap_prompt(1, &preferences);
		CHECK(preferences.game_control_to_xbox_buttons[_game_control_jump] == _gamepad_analog_button_a,
			"local player 2's prompts have a layout");
		controller_remap_prompt(CONTROLLER_PORTS, &preferences);
		return 0;
	}
	CASE("translate")
	{
		XINPUT_GAMEPAD pad;

		reset();
		set_layout(0, JUMP_ON_EAST);
		played(0, pad_with(0));
		/* (east presses the profile's jump button, A; south its melee's, B) */
		pad = played(0, pad_with(1, _remap_button_b));
		CHECK(pressed(&pad, _remap_button_a) && !pressed(&pad, _remap_button_b), "east does not jump");
		pad = played(0, pad_with(1, _remap_button_a));
		CHECK(pressed(&pad, _remap_button_b) && !pressed(&pad, _remap_button_a), "south does not melee");
		/* (a trigger's pull, start, back, the d-pad and sticks as they are) */
		pad = pad_with(3, _remap_button_start, _remap_button_dpad_left, _remap_button_dpad_up);
		pad.bAnalogButtons[_remap_button_left_trigger] = 100;
		pad.sThumbLX = 1234;
		pad.sThumbRY = -4321;
		pad = played(0, pad);
		CHECK(pad.bAnalogButtons[_remap_button_left_trigger] == 100, "the trigger's pull is %d",
			pad.bAnalogButtons[_remap_button_left_trigger]);
		CHECK(pressed(&pad, _remap_button_start) && pressed(&pad, _remap_button_dpad_left) &&
			pressed(&pad, _remap_button_dpad_up) && pad.sThumbLX == 1234 && pad.sThumbRY == -4321,
			"start, the d-pad or a stick changed");
		/* (the d-pad's left as a control's: not the d-pad's too) */
		set_layout(0, "south,rb,west,north,east,dpad_left,lt,rt,ls,rs");
		played(0, pad_with(0));
		pad = played(0, pad_with(1, _remap_button_dpad_left));
		CHECK(pressed(&pad, _remap_button_white) && !pressed(&pad, _remap_button_dpad_left),
			"the d-pad's left does not turn on the flashlight alone");
		pad = played(0, pad_with(1, _remap_button_white));
		CHECK(!pressed(&pad, _remap_button_white), "the left shoulder still turns on the flashlight");
		/* (no layout, not playing, or no controller: the pad as it is) */
		set_layout(0, JUMP_ON_EAST);
		menus_up[0] = TRUE;
		pad = played(0, pad_with(1, _remap_button_b));
		CHECK(pressed(&pad, _remap_button_b) && !pressed(&pad, _remap_button_a), "the menus get the layout");
		menus_up[0] = FALSE;
		pad = played(1, pad_with(1, _remap_button_b));
		CHECK(pressed(&pad, _remap_button_b), "player 2 has player 1's layout");
		pad = pad_with(1, _remap_button_b);
		CHECK(!controller_remap_input(0, &pad, 0) && pressed(&pad, _remap_button_b), "no controller has a layout");
		return 0;
	}
	CASE("held-at-start")
	{
		XINPUT_GAMEPAD pad;

		reset();
		set_layout(0, JUMP_ON_EAST);
		/* (east held as the layout begins does nothing until let go) */
		pad = played(0, pad_with(1, _remap_button_b));
		CHECK(!pressed(&pad, _remap_button_a) && !pressed(&pad, _remap_button_b), "the held button jumped");
		pad = played(0, pad_with(1, _remap_button_b));
		CHECK(!pressed(&pad, _remap_button_a), "the held button jumped later");
		played(0, pad_with(0));
		pad = played(0, pad_with(1, _remap_button_b));
		CHECK(pressed(&pad, _remap_button_a), "the button let go does not jump");
		/* (as when the layout changes) */
		pad = played(0, pad_with(1, _remap_button_x));
		CHECK(pressed(&pad, _remap_button_x), "west does not act");
		set_layout(0, "south,rb,north,west,east,lb,lt,rt,ls,rs");
		pad = played(0, pad_with(1, _remap_button_x));
		CHECK(!pressed(&pad, _remap_button_x) && !pressed(&pad, _remap_button_y), "the held button acted in the new layout");
		return 0;
	}
	CASE("capture")
	{
		XINPUT_GAMEPAD pad;
		int button = NONE;

		reset();
		in_game = FALSE;
		/* (A opened it: held, it binds nothing, and nothing reaches the menu) */
		pad = pad_with(1, _remap_button_a);
		controller_remap_capture_begin(0);
		CHECK(keyboard_capturing, "the keyboard is not captured");
		CHECK(controller_remap_input(0, &pad, 1) && !pressed(&pad, _remap_button_a), "the capture's port reads A");
		CHECK(controller_remap_capture_poll(&button) == _remap_capture_waiting, "the A that opened it was bound");
		pad = pad_with(1, _remap_button_a);
		controller_remap_input(0, &pad, 1);
		CHECK(controller_remap_capture_poll(&button) == _remap_capture_waiting, "the A still held was bound");
		/* (another port in the menus reads nothing; let go, then Y) */
		pad = pad_with(1, _remap_button_a);
		CHECK(controller_remap_input(1, &pad, 1) && !pad.bAnalogButtons[_remap_button_a], "player 2 reads A");
		pad = pad_with(0);
		controller_remap_input(0, &pad, 1);
		CHECK(controller_remap_capture_poll(&button) == _remap_capture_waiting, "nothing was bound");
		pad = pad_with(1, _remap_button_dpad_up);
		controller_remap_input(0, &pad, 1);
		CHECK(controller_remap_capture_poll(&button) == _remap_capture_waiting, "the d-pad's up was bound");
		pad = pad_with(1, _remap_button_y);
		controller_remap_input(0, &pad, 1);
		CHECK(controller_remap_capture_poll(&button) == _remap_capture_bound && button == _remap_button_y,
			"Y was not bound: %d", button);
		CHECK(!keyboard_capturing, "the keyboard is still captured");
		/* (Y held reaches nothing, until let go) */
		pad = pad_with(1, _remap_button_y);
		CHECK(controller_remap_input(0, &pad, 1), "the Y bound reaches the menu");
		pad = pad_with(0);
		controller_remap_input(0, &pad, 1);
		pad = pad_with(1, _remap_button_y);
		CHECK(!controller_remap_input(0, &pad, 1) && pressed(&pad, _remap_button_y), "Y does not reach the menu");
		/* (start or back cancels) */
		controller_remap_capture_begin(0);
		pad = pad_with(0);
		controller_remap_input(0, &pad, 1);
		pad = pad_with(1, _remap_button_back);
		controller_remap_input(0, &pad, 1);
		CHECK(controller_remap_capture_poll(&button) == _remap_capture_cancelled, "back did not cancel");
		/* (as does the keyboard's escape) */
		pad = pad_with(0);
		controller_remap_input(0, &pad, 1);
		controller_remap_capture_begin(0);
		keyboard_result = 3;
		CHECK(controller_remap_capture_poll(&button) == _remap_capture_cancelled, "escape did not cancel");
		/* (as does another controller in the port) */
		controller_remap_capture_begin(0);
		devices[0].instance = 99;
		pad = pad_with(0);
		controller_remap_input(0, &pad, 1);
		CHECK(controller_remap_capture_poll(&button) == _remap_capture_cancelled, "a new controller did not cancel");
		/* (and a menu that stops asking) */
		devices[0].instance = 10;
		controller_remap_capture_begin(0);
		now += REMAP_CAPTURE_TIMEOUT_MS + 1;
		pad = pad_with(1, _remap_button_x);
		controller_remap_input(1, &pad, 1);
		CHECK(pressed(&pad, _remap_button_x), "an abandoned capture still takes player 2's pad");
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
