/* Host-owned bot admission and input replication. The session uses ordinary
player records and actions; unmodified OpenCE clients need no bot logic.
Once enabled, the machine reservation lasts until the host closes. */
#include "cseries.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "memory/data.h"
#include "game/player_queues_new.h"
#include "networking/network_game_globals.h"
#include "networking/network_game_manager.h"
#include "bots.h"
#include "main/console.h"
#include <stdio.h>

int config_boolean(const char *name);

static int network_bots;
static int network_bots_enabled;
static byte owned[BOTS_MAXIMUM];
static long population_target = -1;
static long population_configured = -1;
static long population_limit;

int bots_console_command(char const *expression)
{
	static char const *const commands[] = { "add_bot", "add_bot_red", "add_bot_blue",
		"drop_bot", "drop_bot_red", "drop_bot_blue" };
	char command[32], extra, status[180];
	long index;
	int result;
	if (sscanf(expression, " %31s %c", command, &extra) != 1) return -1;
	for (index = 0; index < (long)NUMBEROF(commands); index++)
		if (!strcmp(command, commands[index]))
		{
			result = bots_manage(index < 3, index % 3 - 1, status, sizeof(status));
			console_printf(TRUE, "%s", status);
			return result;
		}
	return -1;
}

void bots_network_session_begin(void)
{
	bots_management_reset();
	population_target = population_configured = -1;
	network_bots = config_boolean("bots.network");
	network_bots_enabled = network_bots;
	csmemset(owned, 0, sizeof(owned));
}

void bots_network_session_end(void)
{
	bots_management_reset();
	population_target = population_configured = -1;
	network_bots = FALSE;
	network_bots_enabled = FALSE;
	csmemset(owned, 0, sizeof(owned));
}

void bots_player_ownership(long machine_index, int present)
{
	if (bots_machine_is_bot(machine_index))
		owned[machine_index - BOTS_FIRST_MACHINE] = present != 0;
}

int bots_player_owned(long machine_index)
{
	return bots_machine_is_bot(machine_index) && owned[machine_index - BOTS_FIRST_MACHINE];
}

int bots_network_reserved(void)
{
	return network_bots;
}

int bots_session_allowed(void)
{
	return global_network_game_server_get() && (network_game_is_splitscreen_local() || network_bots_enabled);
}

/* Called only after the lobby has checked that no human holds a bot ID.
Keep the reservation when disabled: peers may still be processing removals. */
void bots_network_lobby_enable(int enabled)
{
	if (network_bots_enabled != (enabled != 0))
		population_target = population_configured = -1;
	network_bots_enabled = enabled != 0;
	if (enabled) network_bots = TRUE;
}

long bots_human_count(struct network_game *game)
{
	long index, count = 0;
	for (index = 0; game && index < (long)NUMBEROF(game->players); index++)
		if (network_player_is_valid(&game->players[index]) &&
			!bots_player_owned(game->players[index].machine_index))
			count++;
	return count;
}

/* The first admitted humans and the requested bots establish the session's
population. Later humans replace bots, including across successive maps.
Changing Bot Count deliberately establishes a new target from this lobby. */
long bots_population_wanted(struct network_game *game)
{
	long humans = bots_human_count(game);
	long configured = bots_wanted_count();
	if (network_game_is_splitscreen_local())
		return configured;
	if (!bots_session_allowed())
		return 0;
	if (humans > 0 && (population_target < 0 || population_configured != configured))
	{
		population_configured = configured;
		population_limit = configured;
		population_target = MIN(game->maximum_players, humans + configured);
	}
	return population_target < 0 ? 0 :
		MAX(0, MIN(population_limit, MIN(population_target, game->maximum_players) - humans));
}

void bots_population_changed(struct network_game *game, int change)
{
	if (network_game_is_splitscreen_local() || !bots_session_allowed())
		return;
	bots_population_wanted(game);
	/* Humans may have grown past the original target. An explicit addition
	must retain the bot just admitted rather than immediately removing it. */
	population_target = PIN(MAX(population_target + change, game->player_count), 0, game->maximum_players);
	population_limit = PIN(population_limit + change, 0, BOTS_MAXIMUM);
}

long bots_population_after_join(struct network_game *game)
{
	bots_population_wanted(game);
	return population_target < 0 ? 0 : MAX(0,
		MIN(population_limit, MIN(population_target, game->maximum_players) - bots_human_count(game) - 1));
}

int bots_lobby_can_start(struct network_game *game)
{
	long index;
	short human_machine = NONE;
	int has_bot = FALSE;
	if (!game || !bots_session_allowed()) return FALSE;
	/* Synthetic players have distinct machine IDs, but no connections. Only
	owned bots may extend the lone human machine's countdown permission. */
	for (index = 0; index < (long)NUMBEROF(game->players); index++)
	{
		struct network_player *player = &game->players[index];
		if (!network_player_is_valid(player)) continue;
		if (bots_player_owned(player->machine_index))
			has_bot = TRUE;
		else if (human_machine == NONE)
			human_machine = player->machine_index;
		else if (human_machine != player->machine_index)
			return FALSE;
	}
	return has_bot && human_machine != NONE;
}

void bots_network_record_actions(struct player_action *actions)
{
	struct data_iterator iterator;
	struct player_datum *player;
	if (!game_engine_running() || !bots_session_allowed() || network_game_is_splitscreen_local())
		return;
	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
	{
		if (player->local_player_index == NONE && bots_player_owned(player->network_player_data.machine_index))
		{
			long index = DATUM_INDEX_TO_ABSOLUTE_INDEX(iterator.datum_index);
			if (index >= 0 && index < HALO_PORT_MAXIMUM_NETWORK_PLAYERS)
				update_server_note_bot_action((short)index, &actions[index]);
		}
	}
}
