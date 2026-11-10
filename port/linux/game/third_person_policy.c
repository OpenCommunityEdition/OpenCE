/* Match camera rules never overwrite a player's saved camera preference. */
#include "cseries.h"
#include "third_person.h"
#include "networking/network_game_globals.h"
#include "networking/network_game_manager.h"
#include "networking/network_server_manager_internal.h"
#include "networking/network_server_message_handler.h"
#include "networking/network_messages.h"

char const *config_string(char const *name);
int config_boolean(char const *name);

boolean third_person_match_flags_valid(unsigned long flags)
{
	/* Bits zero through two are upstream co-op flags; policy value three is reserved. */
	return !(flags & ~(7UL | THIRD_PERSON_POLICY_MASK)) &&
		((flags & THIRD_PERSON_POLICY_MASK) >> THIRD_PERSON_POLICY_SHIFT) <= _third_person_forced_off;
}

short third_person_host_policy(void)
{
	char const *setting = config_string("network.camera_policy");
	if (setting && !strcmp(setting, "forced_off")) return _third_person_forced_off;
	return _third_person_player_choice;
}

short third_person_policy(void)
{
	struct network_game const *game = network_game_get_game();
	short policy = game ? (game->cooperative_flags & THIRD_PERSON_POLICY_MASK) >> THIRD_PERSON_POLICY_SHIFT : 0;
	/* Invalid policy values fail closed; receivers reject them before applying settings. */
	return policy <= _third_person_forced_off ? policy : _third_person_forced_off;
}

boolean third_person_enabled(void)
{
	short policy = third_person_policy();
	return policy == _third_person_forced_on ||
		(policy == _third_person_player_choice && config_boolean("display.third_person"));
}

/* Update the authoritative pregame record, including late joiners. */
void third_person_set_match_policy(short policy)
{
	struct network_game_server *server = global_network_game_server_get();
	struct network_game *game;
	/* The server manager's pregame state is zero; keep its record private. */
	if (!server || network_game_server_get_state(server, NULL) != 0 ||
		(policy != _third_person_player_choice && policy != _third_person_forced_off))
		return;
	game = network_game_server_get_game(server);
	game->cooperative_flags = (game->cooperative_flags & ~THIRD_PERSON_POLICY_MASK) |
		(policy << THIRD_PERSON_POLICY_SHIFT);
	if (!network_game_server_send_game_data_pregame(server))
		network_event("failed to send updated camera policy to clients");
}
