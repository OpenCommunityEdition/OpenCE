/*
BOTS_LOBBY.C (test)

The real lobby of the bots (network_server_manager.c's
network_game_server_port_bots, with network_game_manager.c's players and
bots.c's machines and names): who joins a local game's lobby, on which
team, who leaves, and where no bot ever joins. test_bots_lobby.py takes the
constants and structures (config.inc) and the code (under_test.inc).
*/

#include "harness.h"

#include <wchar.h>

typedef unsigned short word;
#define VALID_INDEX(index, count) ((index) >= 0 && (index) < (count))
#define PIN(value, low, high) ((value) < (low) ? (low) : (value) > (high) ? (high) : (value))
#define NUMBEROF(array) (sizeof(array) / sizeof((array)[0]))
#define csmemset memset
#define csmemcpy memcpy
#define error(priority, ...) ((void)0)
#define network_event(...) ((void)0)

#include "config.inc"

#define MAXIMUM_NETWORK_MACHINE_COUNT HALO_PORT_MAXIMUM_NETWORK_MACHINES
#define MAXIMUM_NETWORK_PLAYER_COUNT HALO_PORT_MAXIMUM_NETWORK_PLAYERS
#define NETWORK_GAME_PLAYER_SLOTS HALO_PORT_MAXIMUM_NETWORK_PLAYERS

struct game_variant
{
	short game_engine_index;
};

struct network_game
{
	struct game_variant variant;
	byte maximum_players;
	short player_count;
	struct network_player players[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
};

struct network_game_server
{
	word state;
	struct network_game game;
	boolean sent_start_game_message;
	struct { short machine_index; } client_machines[HALO_PORT_MAXIMUM_NETWORK_MACHINES];
};

/* the world the lobby asks */
static boolean local_game;
static boolean enhanced;
static boolean network_option;
static long wanted;
static short teams_setting;
static long sends;
static short next_color;
static struct network_game_server server;
static boolean is_host = TRUE, has_teams = TRUE, encode_ok = TRUE;
static long tick = 100, messages, joined, removed;
static int last_message_type;
enum { _message_server_add_player_ingame, _message_server_remove_player_ingame };
struct message_server_remove_player_ingame { struct network_player player; long reason; };
static long game_time_get(void) { return tick; }
static boolean game_engine_has_teams(void) { return has_teams; }
static int bots_enhanced_running;
void bots_management_reset(void);
static void *create_network_game_message(int type, const void *record, short size)
{
	(void)size;
	last_message_type = type;
	if (type == _message_server_add_player_ingame) {
		const struct network_player *joining = record;
		long player_slot = joining->player_list_index;
		CHECK(VALID_INDEX(player_slot, NETWORK_GAME_PLAYER_SLOTS), "missing authoritative join slot");
		CHECK(server.game.players[(unsigned char)joining->player_list_index].machine_index == joining->machine_index,
			"join slot differs from host");
	}
	if (type == _message_server_remove_player_ingame) {
		const struct message_server_remove_player_ingame *leaving = record;
		CHECK(leaving->reason == tick + NETWORK_GAME_PLAYER_QUIT_DELAY, "quit tick");
	}
	return encode_ok ? (void *)1 : NULL;
}
static boolean network_game_server_send_message_to_all_machines(struct network_game_server *game, void *message)
{
	(void)game; (void)message;
	messages++;
	if (last_message_type == _message_server_add_player_ingame) joined++;
	else removed++;
	return TRUE;
}

static boolean network_game_is_splitscreen_local(void) { return local_game; }
static int config_boolean(const char *name) { return !strcmp(name, "bots.network") ? network_option : enhanced; }
static struct network_game_server *global_network_game_server_get(void) { return is_host ? &server : NULL; }
static long bots_wanted_count(void) { return wanted; }
static short bots_teams(void) { return teams_setting; }
static boolean network_game_player_slot_held(long player_index) { (void)player_index; return FALSE; }
static void get_unique_random_color(struct network_game_server *server, struct network_player *player)
{
	(void)server;
	player->primary_color_index = next_color++;
}
static boolean network_game_server_send_game_data_pregame(struct network_game_server *server)
{
	(void)server;
	sends++;
	return TRUE;
}

/* (the game's own code, as its build compiles it: a char index, a char
machine index against the session's 128) */
#pragma clang diagnostic ignored "-Wchar-subscripts"
#pragma clang diagnostic ignored "-Wtautological-constant-out-of-range-compare"
#include "under_test.inc"


/* a pregame lobby of the game engine, room for `maximum`, and its players */
static void lobby(short game_engine_index, long maximum)
{
	long index;

	memset(&server, 0, sizeof(server));
	server.state = _network_game_server_state_pregame;
	server.game.variant.game_engine_index = game_engine_index;
	server.game.maximum_players = (byte)maximum;
	for (index = 0; index < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; index++)
		network_game_invalidate_player(&server.game.players[index]);
	for (index = 0; index < HALO_PORT_MAXIMUM_NETWORK_MACHINES; index++)
		server.client_machines[index].machine_index = NONE;
	local_game = TRUE;
	teams_setting = _bots_teams_even;
	sends = 0;
}

/* a player of this machine (0) on the team */
static struct network_player *player(long controller_index, long team_index)
{
	struct network_player joining;
	long index;

	memset(&joining, 0, sizeof(joining));
	joining.name[0] = L'P';
	joining.machine_index = 0;
	joining.controller_index = (char)controller_index;
	joining.team_index = (char)team_index;
	joining.player_list_index = NONE;
	CHECK(network_game_add_player(&server.game, &joining), "player %ld not added", controller_index);
	for (index = 0; index < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; index++)
	{
		if (server.game.players[index].machine_index == 0 && server.game.players[index].controller_index == controller_index)
			return &server.game.players[index];
	}
	return NULL;
}

/* the bots in the lobby, and on each team */
static long bots_in(long *team0, long *team1)
{
	long index, count = 0;

	*team0 = *team1 = 0;
	for (index = 0; index < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; index++)
	{
		struct network_player *slot = &server.game.players[index];

		if (!network_player_is_valid(slot) || !bots_machine_is_bot(slot->machine_index))
			continue;
		count++;
		if (slot->team_index == 0)
			(*team0)++;
		else if (slot->team_index == 1)
			(*team1)++;
	}
	return count;
}

static boolean bot_in(long bot_index)
{
	long index;

	for (index = 0; index < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; index++)
	{
		if (network_player_is_valid(&server.game.players[index]) &&
			server.game.players[index].machine_index == BOTS_FIRST_MACHINE + bot_index)
		{
			return TRUE;
		}
	}
	return FALSE;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	long team0, team1, count;
	char status[180];

	CASE("lobby-enable")
	{
		lobby(game_engine_slayer, 128);
		player(0, 0);
		local_game = FALSE;
		bots_network_session_begin();
		network_option = TRUE;
		wanted = 63;
		CHECK(bots_lobby_settings_apply(status, sizeof(status)), "lobby apply refused: %s", status);
		CHECK(server.game.player_count == 64, "settings saved in lobby did not add 63 bots");
		CHECK(bots_network_reserved() && bots_session_allowed(), "IDs were not reserved before admission");
		CHECK(sends == 1, "new lobby not published once");
		CHECK(bots_lobby_settings_apply(status, sizeof(status)) && sends == 1, "reapply changed unchanged lobby");
		CHECK(bots_player_owned(BOTS_FIRST_MACHINE + 62), "last bot ownership lost");
		wanted = 1;
		CHECK(bots_lobby_settings_apply(status, sizeof(status)) && server.game.player_count == 2, "count reduction ignored");
		CHECK(bots_player_owned(BOTS_FIRST_MACHINE), "remaining ownership reset");
		bots_network_session_end();
		CHECK(!bots_network_reserved() && !bots_session_allowed(), "session state leaked");
		return 0;
	}
	CASE("lobby-disable")
	{
		lobby(game_engine_slayer, 128); player(0, 0); local_game = FALSE;
		network_option = TRUE; bots_network_session_begin(); wanted = 63;
		CHECK(bots_lobby_settings_apply(status, sizeof(status)) && server.game.player_count == 64, "initial bots absent");
		network_option = FALSE;
		CHECK(bots_lobby_settings_apply(status, sizeof(status)), "disable refused");
		CHECK(server.game.player_count == 1 && !bots_session_allowed(), "disabled bots stayed");
		CHECK(bots_network_reserved(), "identity reservation released before session end");
		network_option = TRUE;
		CHECK(bots_lobby_settings_apply(status, sizeof(status)) && server.game.player_count == 64, "reenable failed");
		wanted = 0;
		CHECK(bots_lobby_settings_apply(status, sizeof(status)) && server.game.player_count == 1, "zero count ignored");
		return 0;
	}
	CASE("lobby-enable-conflict")
	{
		struct network_player *human;
		lobby(game_engine_slayer, 128); human = player(0, 0); local_game = FALSE;
		bots_network_session_begin(); network_option = TRUE; wanted = 63;
		server.client_machines[BOTS_FIRST_MACHINE].machine_index = BOTS_FIRST_MACHINE;
		CHECK(!bots_lobby_settings_apply(status, sizeof(status)) && strstr(status, "Re-host"), "pending client conflict hidden");
		CHECK(!bots_network_reserved() && !bots_session_allowed() && server.game.player_count == 1, "pending client stolen");
		server.client_machines[BOTS_FIRST_MACHINE].machine_index = NONE;
		human->machine_index = BOTS_FIRST_MACHINE + 62;
		CHECK(!bots_lobby_settings_apply(status, sizeof(status)), "high-ID human stolen");
		CHECK(!bots_player_owned(human->machine_index) && sends == 0, "human ownership changed");
		human->machine_index = 0;
		CHECK(bots_lobby_settings_apply(status, sizeof(status)) && server.game.player_count == 64, "released IDs unusable");
		return 0;
	}
	CASE("lobby-apply-guards")
	{
		lobby(game_engine_slayer, 128); player(0, 0); local_game = FALSE;
		bots_network_session_begin(); network_option = TRUE; wanted = 63;
		is_host = FALSE;
		CHECK(bots_lobby_settings_apply(status, sizeof(status)) && !bots_network_reserved(), "client changed host policy");
		is_host = TRUE; server.sent_start_game_message = TRUE;
		CHECK(bots_lobby_settings_apply(status, sizeof(status)) && !bots_network_reserved(), "loading changed policy");
		server.sent_start_game_message = FALSE; server.state = _network_game_server_state_ingame;
		CHECK(bots_lobby_settings_apply(status, sizeof(status)) && !bots_network_reserved(), "ingame enabled policy");
		server.state = _network_game_server_state_pregame;
		CHECK(bots_lobby_settings_apply(status, sizeof(status)) && server.game.player_count == 64, "pregame not applied");
		server.state = _network_game_server_state_ingame; network_option = FALSE;
		CHECK(bots_lobby_settings_apply(status, sizeof(status)) && bots_session_allowed(), "live bot AI stopped");
		CHECK(server.game.player_count == 64 && bots_player_owned(BOTS_FIRST_MACHINE), "live roster reset");
		bots_network_session_end(); bots_network_session_begin();
		CHECK(!bots_network_reserved() && !bots_session_allowed(), "disabled policy not read on next host");
		lobby(game_engine_slayer, 16); player(0, 0);
		CHECK(bots_lobby_settings_apply(status, sizeof(status)) && server.game.player_count == 16, "local capacity changed");
		CHECK(!bots_network_reserved(), "local settings reserved network IDs");
		return 0;
	}

	CASE("live")
	{
		long i;
		lobby(game_engine_slayer, 128);
		player(0, 0);
		server.state = _network_game_server_state_ingame;
		for (i = 0; i < 63; i++) CHECK(bots_manage(TRUE, NONE, status, sizeof(status)), "join %ld: %s", i, status);
		CHECK(!bots_manage(TRUE, NONE, status, sizeof(status)) && joined == 63, "over capacity");
		CHECK(bots_in(&team0, &team1) == 63 && team0 == 31 && team1 == 32, "unbalanced live bots");
		CHECK(bots_manage(FALSE, 1, status, sizeof(status)), "team removal");
		CHECK(removed == 1 && !bots_manage(TRUE, NONE, status, sizeof(status)), "premature identity reuse");
		tick += NETWORK_GAME_PLAYER_QUIT_DELAY + 1;
		CHECK(bots_manage(TRUE, 0, status, sizeof(status)) && joined == 64, "identity never reusable");
		CHECK(bots_in(&team0, &team1) == 63 && team0 == 32 && team1 == 31, "explicit team ignored");
		return 0;
	}
	CASE("live-permissions")
	{
		lobby(game_engine_slayer, 128);
		player(0, 0);
		CHECK(!bots_manage(TRUE, NONE, status, sizeof(status)), "lobby used live joins");
		server.state = _network_game_server_state_ingame;
		is_host = FALSE;
		CHECK(!bots_manage(TRUE, NONE, status, sizeof(status)), "client changed host");
		is_host = TRUE; local_game = FALSE;
		CHECK(!bots_manage(TRUE, NONE, status, sizeof(status)), "network default ignored");
		network_option = TRUE; bots_network_session_begin();
		has_teams = FALSE;
		CHECK(!bots_manage(TRUE, 0, status, sizeof(status)) && !bots_manage(TRUE, 2, status, sizeof(status)), "invalid team");
		CHECK(bots_manage(TRUE, NONE, status, sizeof(status)), "FFA bot refused");
		CHECK(bots_manage(FALSE, NONE, status, sizeof(status)), "FFA removal refused");
		CHECK(!bots_manage(FALSE, NONE, status, sizeof(status)), "removed a human");
		CHECK(server.game.player_count == 1, "lost human");
		server.game.variant.game_engine_index = game_engine_ctf;
		enhanced = TRUE;
		CHECK(!bots_manage(TRUE, NONE, status, sizeof(status)), "unloaded tactics enabled by live setting");
		bots_enhanced_running = TRUE;
		CHECK(bots_manage(TRUE, NONE, status, sizeof(status)), "loaded CTF tactics refused");
		bots_network_session_end();
		CHECK(!bots_manage(TRUE, NONE, status, sizeof(status)), "host policy survived session end");
		return 0;
	}
	CASE("live-encode-failure")
	{
		lobby(game_engine_slayer, 128); player(0, 0);
		server.state = _network_game_server_state_ingame;
		encode_ok = FALSE;
		CHECK(!bots_manage(TRUE, NONE, status, sizeof(status)) && server.game.player_count == 1, "failed join changed host");
		encode_ok = TRUE;
		CHECK(bots_manage(TRUE, NONE, status, sizeof(status)), "join refused");
		encode_ok = FALSE;
		CHECK(!bots_manage(FALSE, NONE, status, sizeof(status)) && server.game.player_count == 2, "failed quit changed host");
		CHECK(messages == 1, "failed encode sent a message");
		return 0;
	}

	/* as many as asked for, once: the lobby unchanged after (nothing sent
	again); a 16-player lobby still has room for only 15 bots */
	CASE("fills")
	{
		lobby(game_engine_slayer, 16);
		player(0, 0);
		wanted = 6;
		network_game_server_port_bots(&server);
		CHECK(bots_in(&team0, &team1) == 6, "%ld bots, not 6", bots_in(&team0, &team1));
		CHECK(server.game.player_count == 7, "%d players, not 7", server.game.player_count);
		CHECK(sends == 1, "the lobby sent %ld times, not once", sends);
		network_game_server_port_bots(&server);
		CHECK(sends == 1 && server.game.player_count == 7, "the lobby changed again (%ld sends)", sends);
		wanted = 99;
		network_game_server_port_bots(&server);
		CHECK(bots_in(&team0, &team1) == 15, "%ld bots for 99, not 15", bots_in(&team0, &team1));
		return 0;
	}
	/* as far as the game has room */
	CASE("room")
	{
		lobby(game_engine_slayer, 4);
		player(0, 0);
		wanted = 6;
		network_game_server_port_bots(&server);
		CHECK(bots_in(&team0, &team1) == 3, "%ld bots in a game of 4, not 3", bots_in(&team0, &team1));
		CHECK(server.game.player_count == 4, "%d players in a game of 4", server.game.player_count);
		return 0;
	}
	/* fewer asked for: the last leave */
	CASE("fewer")
	{
		lobby(game_engine_slayer, 16);
		player(0, 0);
		wanted = 6;
		network_game_server_port_bots(&server);
		wanted = 2;
		network_game_server_port_bots(&server);
		CHECK(bots_in(&team0, &team1) == 2, "%ld bots, not 2", bots_in(&team0, &team1));
		CHECK(bot_in(0) && bot_in(1) && !bot_in(2) && !bot_in(5), "not the first two that stayed");
		CHECK(server.game.player_count == 3, "%d players, not 3", server.game.player_count);
		wanted = 0;
		network_game_server_port_bots(&server);
		CHECK(bots_in(&team0, &team1) == 0 && server.game.player_count == 1, "bots stayed for none");
		return 0;
	}
	/* "even": the teams evened with the players */
	CASE("even")
	{
		lobby(game_engine_slayer, 16);
		player(0, 0);
		player(1, 0);
		wanted = 5;
		network_game_server_port_bots(&server);
		count = bots_in(&team0, &team1);
		CHECK(count == 5, "%ld bots, not 5", count);
		CHECK(team0 + 2 - team1 <= 1 && team1 - (team0 + 2) <= 1, "teams of %ld and %ld", team0 + 2, team1);
		return 0;
	}
	/* "against": every bot on the team the players are not on */
	CASE("against")
	{
		lobby(game_engine_slayer, 16);
		player(0, 1);
		teams_setting = _bots_teams_against;
		wanted = 4;
		network_game_server_port_bots(&server);
		count = bots_in(&team0, &team1);
		CHECK(count == 4 && team0 == 4, "%ld of %ld bots against the players' team 1", team0, count);
		return 0;
	}
	/* the players change teams in the lobby: the bots follow */
	CASE("follows-players")
	{
		struct network_player *human;

		lobby(game_engine_slayer, 16);
		human = player(0, 0);
		teams_setting = _bots_teams_against;
		wanted = 3;
		network_game_server_port_bots(&server);
		CHECK(bots_in(&team0, &team1) == 3 && team1 == 3, "%ld bots on team 1, not 3", team1);
		human->team_index = 1;
		network_game_server_port_bots(&server);
		CHECK(bots_in(&team0, &team1) == 3 && team0 == 3, "%ld bots on team 0 once the player moved, not 3", team0);
		CHECK(sends == 2, "the move sent %ld times", sends - 1);
		return 0;
	}
	/* a game other machines may join (system link, internet): none, and any
	there leave */
	CASE("never-network")
	{
		lobby(game_engine_slayer, 16);
		player(0, 0);
		wanted = 6;
		local_game = FALSE;
		network_game_server_port_bots(&server);
		CHECK(bots_in(&team0, &team1) == 0, "%ld bots in a network game", bots_in(&team0, &team1));
		CHECK(sends == 0, "a network game's lobby sent for bots");
		local_game = TRUE;
		network_game_server_port_bots(&server);
		local_game = FALSE;
		network_game_server_port_bots(&server);
		CHECK(bots_in(&team0, &team1) == 0, "%ld bots stayed in a network game", bots_in(&team0, &team1));
		return 0;
	}
	/* Slayer alone (the other gametypes' goals are not the bots' yet): none
	in CTF, and those of a Slayer lobby leave when it becomes CTF */
	CASE("slayer-only")
	{
		lobby(game_engine_ctf, 16);
		player(0, 0);
		wanted = 6;
		network_game_server_port_bots(&server);
		CHECK(bots_in(&team0, &team1) == 0, "%ld bots in CTF", bots_in(&team0, &team1));
		server.game.variant.game_engine_index = game_engine_slayer;
		network_game_server_port_bots(&server);
		CHECK(bots_in(&team0, &team1) == 6, "%ld bots in Slayer, not 6", bots_in(&team0, &team1));
		server.game.variant.game_engine_index = game_engine_king;
		network_game_server_port_bots(&server);
		CHECK(bots_in(&team0, &team1) == 0, "%ld bots stayed for King of the Hill", bots_in(&team0, &team1));
		return 0;
	}
	/* nothing changes once the game is started, nor in the game */
	CASE("enhanced-objectives")
	{
		lobby(game_engine_ctf, 16);
		player(0, 0);
		wanted = 4;
		enhanced = TRUE;
		network_game_server_port_bots(&server);
		CHECK(bots_in(&team0, &team1) == 4, "enhanced CTF bots absent");
		server.game.variant.game_engine_index = game_engine_king;
		network_game_server_port_bots(&server);
		CHECK(bots_in(&team0, &team1) == 4, "enhanced King bots absent");
		enhanced = FALSE;
		network_game_server_port_bots(&server);
		CHECK(bots_in(&team0, &team1) == 0, "disabled objectives retained bots");
		return 0;
	}
	CASE("network-opt-in")
	{
		lobby(game_engine_slayer, 128);
		player(0, 0);
		local_game = FALSE;
		wanted = 6;
		network_option = TRUE;
		bots_network_session_begin();
		network_game_server_port_bots(&server);
		CHECK(bots_in(&team0, &team1) == 6, "opted-in host has no bots");
		network_option = FALSE;
		CHECK(bots_network_reserved(), "IDs changed in a live session");
		bots_network_session_end();
		bots_network_session_begin();
		CHECK(!bots_network_reserved() && !bots_session_allowed(), "policy leaked into next session");
		return 0;
	}
	CASE("human-high-id")
	{
		lobby(game_engine_slayer, 128);
		player(0, 0)->machine_index = BOTS_FIRST_MACHINE;
		local_game = FALSE;
		wanted = 6;
		bots_network_session_begin();
		CHECK(!bots_player_owned(BOTS_FIRST_MACHINE), "human high ID acquired bot ownership");
		network_game_server_port_bots(&server);
		CHECK(server.game.player_count == 1, "ordinary host removed a high-ID human");
		return 0;
	}
	CASE("not-once-started")
	{
		lobby(game_engine_slayer, 16);
		player(0, 0);
		wanted = 6;
		server.sent_start_game_message = TRUE;
		network_game_server_port_bots(&server);
		CHECK(bots_in(&team0, &team1) == 0, "%ld bots joined a starting game", bots_in(&team0, &team1));
		server.sent_start_game_message = FALSE;
		server.state = _network_game_server_state_ingame;
		network_game_server_port_bots(&server);
		CHECK(bots_in(&team0, &team1) == 0, "%ld bots joined a game in progress", bots_in(&team0, &team1));
		return 0;
	}
	/* no bots before a player is in */
	CASE("no-player-no-bots")
	{
		lobby(game_engine_slayer, 16);
		wanted = 6;
		network_game_server_port_bots(&server);
		CHECK(bots_in(&team0, &team1) == 0 && server.game.player_count == 0, "bots in a lobby with no player");
		return 0;
	}
	/* each bot the one player of a machine no real machine is, named, a
	colour of its own */
	CASE("names-and-machines")
	{
		long index, other, named = 0;

		lobby(game_engine_slayer, 128);
		player(0, 0);
		wanted = 99;
		network_game_server_port_bots(&server);
		for (index = 0; index < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; index++)
		{
			struct network_player *slot = &server.game.players[index];

			if (!network_player_is_valid(slot) || !bots_machine_is_bot(slot->machine_index))
				continue;
			CHECK(slot->controller_index == 0, "bot of machine %d on controller %d", slot->machine_index,
				slot->controller_index);
			CHECK(slot->machine_index >= BOTS_FIRST_MACHINE && slot->machine_index < BOTS_FIRST_MACHINE + BOTS_MAXIMUM,
				"bot of machine %d", slot->machine_index);
			CHECK(slot->primary_color_index != NONE, "bot of machine %d has no colour", slot->machine_index);
			if (slot->name[0])
				named++;
			CHECK(slot->name[NUMBEROF(slot->name) - 1] == 0, "unterminated bot name");
			for (other = 0; other < index; other++)
				if (bots_player_owned(server.game.players[other].machine_index))
					CHECK(wcscmp(slot->name, server.game.players[other].name), "duplicate bot name");
		}
		CHECK(named == 63, "%ld of 63 bots named", named);
		CHECK(server.game.player_count == 64, "capacity was not capped at 63 bots");
		CHECK(!bots_machine_is_bot(0) && !bots_machine_is_bot(BOTS_FIRST_MACHINE - 1), "a real machine is a bot");
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
