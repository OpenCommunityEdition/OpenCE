/* Match camera rules never overwrite a player's saved camera preference. */
#include "cseries.h"
#include "third_person.h"
#include "third_person_protocol.h"
#include "halo_keyboard.h"
#include "controller_remap.h"
#include "networking/network_game_globals.h"
#include "networking/network_game_manager.h"

char const *config_string(char const *name);
int config_boolean(char const *name);
int config_write(char const *name, char const *value);

boolean third_person_match_flags_valid(unsigned long flags)
{
	/* Bits zero through two are upstream co-op flags; policy value three is reserved. */
	return !(flags & ~(7UL | (third_person_session_active() ? THIRD_PERSON_POLICY_MASK : 0))) &&
		((flags & THIRD_PERSON_POLICY_MASK) >> THIRD_PERSON_POLICY_SHIFT) <= _third_person_forced_off;
}

short third_person_host_policy(void)
{
	char const *setting = config_string("network.camera_policy");
	if (setting && !strcmp(setting, "forced_on")) return _third_person_forced_on;
	if (setting && !strcmp(setting, "forced_off")) return _third_person_forced_off;
	return _third_person_player_choice;
}

short third_person_policy(void)
{
	struct network_game const *game = network_game_get_game();
	short policy = game ? (game->cooperative_flags & THIRD_PERSON_POLICY_MASK) >> THIRD_PERSON_POLICY_SHIFT : 0;
	/* Local split screen has no remote peers and retains the offline preference. */
    if (network_game_is_splitscreen_local() && !third_person_session_active()) return _third_person_player_choice;
    if (network_game_is_active() && !third_person_session_active()) return _third_person_forced_off;
    /* The receiver rejects invalid records; fail closed if called earlier. */
	return policy <= _third_person_forced_off ? policy : _third_person_forced_off;
}

boolean third_person_enabled(void)
{
	short policy = third_person_policy();
	return policy == _third_person_forced_on ||
		(policy == _third_person_player_choice && config_boolean("display.third_person"));
}

void third_person_input_update(void)
{
	static unsigned long previous;
	unsigned long held = halo_keyboard_actions(0);
	unsigned long pressed = held & ~previous;
	unsigned int pad_pressed = 0;
	short port;
	previous = held;
	/* The remapper owns controller context, capture and release guards. */
	for (port = 0; port < 4; port++)
		pad_pressed |= controller_remap_pressed(port, (1u << PAD_TOGGLE_PERSPECTIVE) | (1u << PAD_SWAP_SHOULDER));
	if (pad_pressed & (1u << PAD_TOGGLE_PERSPECTIVE)) pressed |= 1UL << HALO_KEYBOARD_TOGGLE_PERSPECTIVE;
	if (pad_pressed & (1u << PAD_SWAP_SHOULDER)) pressed |= 1UL << HALO_KEYBOARD_SWAP_SHOULDER;
	/* Keyboard and controller share one camera preference and host policy. */
	if ((pressed & (1UL << HALO_KEYBOARD_TOGGLE_PERSPECTIVE)) &&
		third_person_policy() == _third_person_player_choice)
		config_write("display.third_person", config_boolean("display.third_person") ? "false" : "true");
	if ((pressed & (1UL << HALO_KEYBOARD_SWAP_SHOULDER)) && third_person_enabled())
		config_write("display.third_person_left_shoulder", config_boolean("display.third_person_left_shoulder") ? "false" : "true");
}
