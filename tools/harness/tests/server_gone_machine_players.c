/*
SERVER_GONE_MACHINE_PLAYERS.C (test)

The players of a machine that has left the game
(source/networking/network_server_manager.c). A machine still downloading
the map when the game starts has its players held out of it
(network_game_server_hold_map_fetcher_players), put back as the game ends
(network_game_server_restore_held_players). One that left meanwhile had its
players put back all the same: players of a machine the game no longer has,
which no kick, ban or AFK kick could take out, kept in the game until the
server stopped. Its held players now go with it; none is put back for a
machine that is not in the game; and a kick takes such a player out.
test_server_gone_machine_players.py takes the constants (config.inc), the
quit message (messages.inc) and the code (under_test.inc).
*/

#include <stdarg.h>
#include "harness.h"

#define NETWORK_SERVER_MANAGER_FILE "server"
#define VALID_INDEX(index, count) ((index) >= 0 && (index) < (count))
#define SET_FLAG(flags, bit, value) ((value) ? ((flags) |= FLAG(bit)) : ((flags) &= ~FLAG(bit)))
#define csmemmove memmove
#define _error_silent 0

#include "config.inc"

struct network_player
{
	signed char machine_index;
	signed char controller_index;
	signed char player_list_index;
};

struct network_machine
{
	signed char machine_index;
};

struct network_game
{
	short player_count;
	struct network_machine machines[MAXIMUM_NETWORK_MACHINE_COUNT];
	struct network_player players[MAXIMUM_NETWORK_PLAYER_COUNT];
};

struct network_connection { int unused; };
struct network_message { int unused; };

struct network_game_server_client_machine
{
	struct network_connection *connection;
	long last_received_update_sequence_number;
	unsigned long last_heard_time;
	short machine_index;
	unsigned long flags;
};

struct network_game_server
{
	short state;
	boolean sent_start_game_message;
	struct network_connection *connection;
	struct network_game game;
	struct network_game_server_client_machine client_machines[MAXIMUM_NETWORK_MACHINE_COUNT];
	boolean queued_player_valid;
	struct network_player queued_player;
	short waiting_player_count;
	struct network_player waiting_players[MAXIMUM_NETWORK_PLAYER_COUNT];
};

#include "messages.inc"

static struct network_game_server server;
static struct network_game_server *server_pointer = &server;
static struct network_connection connection;
static unsigned long network_game_server_client_machine_addresses[MAXIMUM_NETWORK_MACHINE_COUNT];
static boolean network_game_server_map_pending[MAXIMUM_NETWORK_MACHINE_COUNT];
static struct network_player network_game_server_held_players[MAXIMUM_NETWORK_PLAYER_COUNT];
static long network_game_server_held_player_count;

/* the machines downloading the map (port/linux/game/map_share.c) */
static boolean downloading[MAXIMUM_NETWORK_MACHINE_COUNT];
static boolean map_share_machine_needs_map(long machine_index)
{
	return VALID_INDEX(machine_index, MAXIMUM_NETWORK_MACHINE_COUNT) && downloading[machine_index];
}

static boolean splitscreen;
static boolean network_game_is_splitscreen_local(void) { return splitscreen; }
static struct network_game_server *global_network_game_server_get(void) { return server_pointer; }
static long game_time_get(void) { return 1000; }
static void network_event(char const *text, ...) { (void)text; }
static void error(int level, char const *text, ...) { (void)level; (void)text; }

static char warning[256];
static void console_warning(char const *format, ...)
{
	va_list arguments;

	va_start(arguments, format);
	vsnprintf(warning, sizeof(warning), format, arguments);
	va_end(arguments);
}

/* the game's players (network_game_manager.c's, as far as these need) */
static boolean network_player_is_valid(struct network_player *player)
{
	return player->controller_index >= 0 && player->controller_index < 4 &&
		player->machine_index >= 0;
}
static void invalidate_player(struct network_player *player)
{
	memset(player, NONE, sizeof(*player));
}
static boolean network_game_add_player(struct network_game *game, struct network_player *player)
{
	long index;

	for (index = 0; index < MAXIMUM_NETWORK_PLAYER_COUNT; index++)
	{
		if (!network_player_is_valid(&game->players[index]))
		{
			game->players[index] = *player;
			game->players[index].player_list_index = (signed char)index;
			game->player_count++;
			return TRUE;
		}
	}
	return FALSE;
}
static boolean network_game_remove_player(struct network_game *game, struct network_player *player)
{
	long index;

	for (index = 0; index < MAXIMUM_NETWORK_PLAYER_COUNT; index++)
	{
		if (network_player_is_valid(&game->players[index]) &&
			game->players[index].machine_index == player->machine_index &&
			game->players[index].controller_index == player->controller_index)
		{
			invalidate_player(&game->players[index]);
			game->player_count--;
			return TRUE;
		}
	}
	return FALSE;
}
static boolean network_game_remove_machine(struct network_game *game, struct network_machine *machine)
{
	long index;

	for (index = 0; index < MAXIMUM_NETWORK_PLAYER_COUNT; index++)
	{
		if (network_player_is_valid(&game->players[index]) && game->players[index].machine_index == machine->machine_index)
		{
			invalidate_player(&game->players[index]);
			game->player_count--;
		}
	}
	machine->machine_index = NONE;
	return TRUE;
}

/* what the server sends */
static struct network_message message;
static struct message_server_remove_player_ingame quits[8];
static int quit_count, broadcasts, lobby_updates;
static struct network_message *create_network_game_message(short type, void const *packet, long size)
{
	if (type == _message_server_remove_player_ingame && size == sizeof(quits[0]) && quit_count < 8)
		memcpy(&quits[quit_count++], packet, sizeof(quits[0]));
	return &message;
}
static boolean network_game_server_send_message_to_all_machines(struct network_game_server *s, struct network_message *m)
{
	(void)s; (void)m;
	broadcasts++;
	return TRUE;
}
static boolean network_game_server_send_game_data_pregame(struct network_game_server *s)
{
	(void)s;
	lobby_updates++;
	return TRUE;
}
static void network_game_server_send_player_quit_messages_ingame(struct network_game_server *s,
	struct network_game_server_client_machine *machine)
{
	(void)s; (void)machine;
}
static boolean network_server_close_client_connection(struct network_connection *a, struct network_connection *b)
{
	(void)a; (void)b;
	return TRUE;
}
static void network_game_server_check_loading_complete(struct network_game_server *s) { (void)s; }

/* the host's kick and ban of a machine in the game (not under test here) */
static int kicked = NONE, banned = NONE;
static void network_game_server_machine_names(struct network_game_server *s, long machine_index, char *names, long size)
{
	(void)s; (void)machine_index;
	snprintf(names, (size_t)size, "a player");
}
static void network_game_server_kick_named_machine(long machine_index, char const *names)
{
	(void)names;
	kicked = (int)machine_index;
}
static void network_game_server_ban_machine(long machine_index, char const *names)
{
	(void)names;
	banned = (int)machine_index;
}

static boolean network_game_server_client_machine_is_joined_to_game(
	struct network_game_server *s,
	struct network_game_server_client_machine *machine);
/* the host's own machine is 0 */
static boolean network_game_server_client_machine_is_local(
	struct network_game_server *s,
	struct network_game_server_client_machine *machine)
{
	return network_game_server_client_machine_is_joined_to_game(s, machine) && machine == &s->client_machines[0];
}

/* No MCC transfer is active in these legacy disconnection cases. */
void mcc_share_game_peer_left(struct network_game_server_client_machine *client) { (void)client; }
#include "under_test.inc"

enum { GONE = 5, OTHER = 6 };

/* a lobby: the host's machine 0, machines 5 and 6 joined, one player each */
static void lobby(void)
{
	long index;

	memset(&server, 0, sizeof(server));
	memset(downloading, 0, sizeof(downloading));
	memset(network_game_server_map_pending, 0, sizeof(network_game_server_map_pending));
	network_game_server_held_player_count = 0;
	for (index = 0; index < MAXIMUM_NETWORK_PLAYER_COUNT; index++)
		invalidate_player(&server.game.players[index]);
	for (index = 0; index < MAXIMUM_NETWORK_MACHINE_COUNT; index++)
	{
		server.client_machines[index].machine_index = NONE;
		server.game.machines[index].machine_index = NONE;
	}
	server.connection = &connection;
	server.state = _network_game_server_state_pregame;
	for (index = 0; index <= OTHER; index += index ? 1 : GONE)
	{
		struct network_player player = { (signed char)index, 0, NONE };

		server.client_machines[index].connection = &connection;
		server.client_machines[index].machine_index = (short)index;
		server.client_machines[index].flags = FLAG(_network_client_machine_connected_bit) |
			FLAG(_network_client_machine_validated_bit);
		server.game.machines[index].machine_index = (signed char)index;
		CHECK(network_game_add_player(&server.game, &player), "fixture: no room");
	}
	splitscreen = FALSE;
	warning[0] = 0;
	quit_count = broadcasts = lobby_updates = 0;
	kicked = banned = NONE;
}

static long players_of(long machine_index)
{
	long index, count = 0;

	for (index = 0; index < MAXIMUM_NETWORK_PLAYER_COUNT; index++)
	{
		if (network_player_is_valid(&server.game.players[index]) && server.game.players[index].machine_index == machine_index)
			count++;
	}
	return count;
}

/* the game starts while machine 5 downloads the map: its player held out */
static void start_while_downloading(void)
{
	downloading[GONE] = TRUE;
	network_game_server_hold_map_fetcher_players(&server);
	network_game_server_map_pending[GONE] = TRUE;
	server.sent_start_game_message = TRUE;
	CHECK(players_of(GONE) == 0 && network_game_server_held_player_count == 1, "fixture: the player was not held");
}

/* machine 5's connection goes (network_game_server_drop_client_machine) */
static void machine_gone(void)
{
	CHECK(network_game_server_remove_client_machine_from_game(&server, &server.client_machines[GONE]),
		"the machine was not removed");
	downloading[GONE] = FALSE;
}

/* the game over, back to the lobby (network_game_server_reset_to_pregame) */
static void back_to_lobby(void)
{
	server.state = _network_game_server_state_pregame;
	server.sent_start_game_message = FALSE;
	memset(network_game_server_map_pending, 0, sizeof(network_game_server_map_pending));
	network_game_server_restore_held_players(&server);
}

/* a player of machine 5 still in the game, its machine gone */
static void player_of_gone_machine(void)
{
	server.client_machines[GONE].connection = NULL;
	server.client_machines[GONE].machine_index = NONE;
	server.client_machines[GONE].flags = 0;
	server.game.machines[GONE].machine_index = NONE;
}

int main(int argc, char **argv)
{
	char const *name = argc > 1 ? argv[1] : "";

	lobby();
	if (!strcmp(name, "gone-while-held"))
	{
		/* it leaves while held, the game ends: its player is not put back */
		start_while_downloading();
		server.state = _network_game_server_state_postgame;
		machine_gone();
		CHECK(network_game_server_held_player_count == 0, "its held player outlived the machine");
		CHECK(!network_game_server_map_pending[GONE], "its pending download outlived the machine");
		back_to_lobby();
		CHECK(players_of(GONE) == 0, "a player of a machine that had gone was put back in the lobby");
		CHECK(players_of(0) == 1 && players_of(OTHER) == 1, "the others' players were lost");
	}
	else if (!strcmp(name, "index-taken-again"))
	{
		/* it leaves while held, and another machine joins at its index before
		the game ends: the new machine is not given the old one's player */
		start_while_downloading();
		machine_gone();
		server.client_machines[GONE].connection = &connection;
		server.client_machines[GONE].machine_index = GONE;
		server.client_machines[GONE].flags = FLAG(_network_client_machine_connected_bit) |
			FLAG(_network_client_machine_validated_bit);
		server.game.machines[GONE].machine_index = GONE;
		back_to_lobby();
		CHECK(players_of(GONE) == 0, "the machine that joined at the index was given the old machine's player");
	}
	else if (!strcmp(name, "held-not-joined"))
	{
		/* a held player whose machine is not in the game is not put back */
		start_while_downloading();
		server.client_machines[GONE].flags = FLAG(_network_client_machine_connected_bit);
		back_to_lobby();
		CHECK(players_of(GONE) == 0, "a player of a machine not in the game was put back");
	}
	else if (!strcmp(name, "held-put-back"))
	{
		/* one still there (its download not done when the game ended) is put back */
		start_while_downloading();
		back_to_lobby();
		CHECK(players_of(GONE) == 1 && network_game_server_held_player_count == 0, "a held player was not put back");
	}
	else if (!strcmp(name, "other-machine-gone"))
	{
		/* another machine leaving takes nothing held of machine 5 */
		start_while_downloading();
		CHECK(network_game_server_remove_client_machine_from_game(&server, &server.client_machines[OTHER]), "not removed");
		CHECK(network_game_server_held_player_count == 1 && network_game_server_map_pending[GONE],
			"another machine's leaving took machine 5's held player");
	}
	else if (!strcmp(name, "kick-in-game"))
	{
		/* in the game: kicked, out, and every machine told it quit */
		server.state = _network_game_server_state_ingame;
		player_of_gone_machine();
		CHECK(network_game_server_kick_machine_of_player(GONE, FALSE), "refused: %s", warning);
		CHECK(players_of(GONE) == 0, "the player is still in the game");
		CHECK(quit_count == 1 && broadcasts == 1 && quits[0].player.machine_index == GONE &&
			quits[0].reason == game_time_get() + NETWORK_GAME_PLAYER_QUIT_DELAY,
			"the machines were not told it quit (%d quits)", quit_count);
		CHECK(kicked == NONE && lobby_updates == 0, "a machine was kicked, or the lobby updated in the game");
		CHECK(players_of(0) == 1 && players_of(OTHER) == 1, "the others' players were taken out");
	}
	else if (!strcmp(name, "kick-in-lobby"))
	{
		/* in the lobby: out, and the lobby sent again (no quit, which is the game's) */
		player_of_gone_machine();
		CHECK(network_game_server_kick_machine_of_player(GONE, FALSE), "refused: %s", warning);
		CHECK(players_of(GONE) == 0 && quit_count == 0 && lobby_updates == 1, "not taken out of the lobby as a leaver is");
	}
	else if (!strcmp(name, "ban-refused"))
	{
		/* a ban has nothing to ban: refused, and said why */
		server.state = _network_game_server_state_ingame;
		player_of_gone_machine();
		CHECK(!network_game_server_kick_machine_of_player(GONE, TRUE), "a ban of a machine that had gone succeeded");
		CHECK(players_of(GONE) == 1 && banned == NONE, "the ban did something");
		CHECK(strstr(warning, "left the game") != NULL, "not said why: %s", warning);
	}
	else if (!strcmp(name, "kick-joined"))
	{
		/* a machine in the game is kicked as before, its player left to its leaving */
		CHECK(network_game_server_kick_machine_of_player(GONE, FALSE), "refused: %s", warning);
		CHECK(kicked == GONE && players_of(GONE) == 1 && quit_count == 0, "not kicked as a machine in the game is");
	}
	else if (!strcmp(name, "kick-nobody"))
	{
		/* an index with no machine and no player: refused as before */
		CHECK(!network_game_server_kick_machine_of_player(20, FALSE), "a kick of nobody succeeded");
		CHECK(strstr(warning, "nor one not joined") != NULL, "%s", warning);
	}
	else if (!strcmp(name, "kick-host"))
	{
		/* the host's own machine is never kicked, nor its players taken out */
		CHECK(!network_game_server_kick_machine_of_player(0, FALSE), "the host's machine was kicked");
		CHECK(players_of(0) == 1, "the host's player was taken out");
	}
	else if (!strcmp(name, "split-screen"))
	{
		/* a split screen game's players (its bots) have machines of their own: not taken out */
		splitscreen = TRUE;
		player_of_gone_machine();
		CHECK(!network_game_server_kick_machine_of_player(GONE, FALSE), "a split screen game's player was kicked");
		CHECK(players_of(GONE) == 1, "a split screen game's player was taken out");
	}
	else
	{
		fprintf(stderr, "unknown case %s\n", name);
		return 2;
	}
	return 0;
}
