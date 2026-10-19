#include "harness.h"
#include <string.h>

#define MAXIMUM_GAMEPADS 4
#define NUMBER_OF_GAMEPAD_BUTTONS 16
#include "config.inc"
#include "gamepad_layouts.h"

/* the profile's preferences, as player_ui.c builds them from the Xbox's layout */
struct game_input_preferences
{
	byte game_control_to_xbox_buttons[12];
};
static byte port_sprint_buttons[MAXIMUM_GAMEPADS];
static char const *layout_setting;
static char const *config_string(char const *name)
{
	CHECK(!strcmp(name, "input.gamepad_layout"), "asked for %s", name);
	return layout_setting;
}
#include "under_test.inc"

static byte const xbox_buttons[12] = { 0, 4, 2, 3, 1, 5, 6, 7, 12, 13, 14, 15 };

static void apply(char const *layout, struct game_input_preferences *preferences)
{
	layout_setting = layout;
	memcpy(preferences->game_control_to_xbox_buttons, xbox_buttons, sizeof(xbox_buttons));
	input_abstraction_port_apply_layout(0, preferences);
}

int main(int argc, char **argv)
{
	struct game_input_preferences preferences;
	int index;

	CHECK(argc == 2, "a case");
	if (!strcmp(argv[1], "profile"))
	{
		/* the profile's own and an unknown name leave the buttons as they were */
		apply("profile", &preferences);
		CHECK(!memcmp(preferences.game_control_to_xbox_buttons, xbox_buttons, 12), "profile changed the buttons");
		apply("halo9", &preferences);
		CHECK(!memcmp(preferences.game_control_to_xbox_buttons, xbox_buttons, 12), "an unknown layout changed them");
		layout_setting = NULL;
		input_abstraction_port_apply_layout(0, &preferences);
	}
	else if (!strcmp(argv[1], "infinite"))
	{
		apply("infinite", &preferences);
		/* jump A, switch grenades d-pad left, action X, switch weapons Y, melee right stick, flashlight d-pad up,
		   grenade left bumper (white), fire right trigger, crouch B, zoom left trigger */
		CHECK(preferences.game_control_to_xbox_buttons[_game_control_jump] == 0, "jump");
		CHECK(preferences.game_control_to_xbox_buttons[_game_control_switch_grenades] == 10, "switch grenades");
		CHECK(preferences.game_control_to_xbox_buttons[_game_control_melee] == 15, "melee");
		CHECK(preferences.game_control_to_xbox_buttons[_game_control_flashlight] == 8, "flashlight");
		CHECK(preferences.game_control_to_xbox_buttons[_game_control_grenade] == 5, "grenade");
		CHECK(preferences.game_control_to_xbox_buttons[_game_control_crouch] == 1, "crouch");
		CHECK(preferences.game_control_to_xbox_buttons[_game_control_zoom] == 6, "zoom");
	}
	else if (!strcmp(argv[1], "start_back"))
	{
		/* Start and Back stay where the game checks them (input_abstraction_update_local_player_preferences) */
		for (index = _gamepad_layout_halo2; index < NUMBER_OF_GAMEPAD_LAYOUTS; index++)
		{
			apply(gamepad_layouts[index].name, &preferences);
			CHECK(preferences.game_control_to_xbox_buttons[_game_control_start] == 12 &&
				preferences.game_control_to_xbox_buttons[_game_control_back] == 13, "%s moved start or back",
				gamepad_layouts[index].name);
		}
	}
	else if (!strcmp(argv[1], "sprint_button"))
	{
		/* the left stick's click sprints in Halo 4, Halo 5 and Infinite; the others and the profile have no button */
		apply("halo4", &preferences);
		CHECK(port_sprint_buttons[0] == 14, "halo4 sprint");
		apply("infinite", &preferences);
		CHECK(port_sprint_buttons[0] == 14, "infinite sprint");
		apply("halo3", &preferences);
		CHECK(port_sprint_buttons[0] == GAMEPAD_LAYOUT_NONE, "halo3 has sprint");
		apply("halo5", &preferences);
		apply("profile", &preferences);
		CHECK(port_sprint_buttons[0] == GAMEPAD_LAYOUT_NONE, "the profile's layout kept the last one's sprint");
	}
	else if (!strcmp(argv[1], "halo2"))
	{
		apply("halo2", &preferences);
		CHECK(!memcmp(preferences.game_control_to_xbox_buttons, xbox_buttons, 12), "halo2 is the Xbox's layout");
	}
	else
	{
		CHECK(0, "no case %s", argv[1]);
	}
	return 0;
}
