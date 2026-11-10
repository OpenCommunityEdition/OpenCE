/*
MATCHMAKING.C

The game side of a MATCHMAKING search, carried out each frame.
*/

#include "cseries.h"
#include "main/main.h"
#include "interface/player_ui.h"
#include "interface/ui_widget.h"
#include "interface/ui_widget_definitions.h"
#include "networking/network_game_globals.h"
#include "networking/network_game_manager.h"
#include "networking/network_client_manager.h"
#include "networking/network_connection.h"
#include "networking/network_server_manager.h"
#include "networking/network_server_manager_internal.h"
#include "game/game_engine.h"
#include "matchmaking.h"

#include "../src/p2p.h"

#include <stdlib.h>

/* one under 5000: a countdown message rounds down, so 4999 reads 4 and the lobby's line adds 1 */
#define MATCHMAKING_START_MILLISECONDS 4999
#define MATCHMAKING_JOIN_SECONDS 25.0f

/* the server's setup clears a map set sooner */
#define MATCHMAKING_MAP_SECONDS 1.0f
#define MATCHMAKING_PLAYER_SECONDS 2.0f

char const *pc_menus_screen(char const *name);
unsigned long system_milliseconds(void);
boolean main_menu_is_loaded(void);
boolean pc_menu_player1_profile(void);
boolean pc_menu_multiplayer_player(short controller);
void platform_log(char const *format, ...);

static char const *const slayer_small_maps[] = { "chillout", "prisoner", "wizard", "longest", "carousel", "damnation", NULL };
static char const *const slayer_maps[] = { "beavercreek", "chillout", "damnation", "hangemhigh", "carousel", "ratrace", "putput", "prisoner", NULL };
static char const *const ctf_small_maps[] = { "chillout", "wizard", "longest", "carousel", NULL };
static char const *const ctf_maps[] = { "beavercreek", "damnation", "hangemhigh", "carousel", "putput", NULL };
static char const *const king_small_maps[] = { "chillout", "prisoner", "wizard", "carousel", NULL };
static char const *const king_maps[] = { "beavercreek", "chillout", "hangemhigh", "carousel", "ratrace", NULL };
static char const *const oddball_small_maps[] = { "chillout", "prisoner", "wizard", "longest", NULL };
static char const *const oddball_maps[] = { "beavercreek", "damnation", "hangemhigh", "carousel", "ratrace", NULL };
static char const *const race_maps[] = { "bloodgulch", "sidewinder", NULL };

static const struct
{
	char const *team_variant, *ffa_variant;
	char const *const *small_maps, *const *maps;
} playlists[NUMBER_OF_P2P_MM_PLAYLISTS] = {
	[P2P_MM_SLAYER] = { "team_slayer", "slayer", slayer_small_maps, slayer_maps },
	[P2P_MM_CTF] = { "ctf", NULL, ctf_small_maps, ctf_maps },
	[P2P_MM_KING] = { "team_king", "king", king_small_maps, king_maps },
	[P2P_MM_ODDBALL] = { "team_oddball", "oddball", oddball_small_maps, oddball_maps },
	[P2P_MM_RACE] = { "team_race", "race", race_maps, race_maps },
};

enum
{
	_matchmaking_idle,
	_matchmaking_hosting,
	_matchmaking_joining,
	_matchmaking_joined,
};

static struct
{
	short role;
	float seconds;
	short playlist, size;
	char map[64];
	boolean map_set, player_added;
	byte host[6];
} matchmaking;

boolean matchmaking_game_locked(void)
{
	return matchmaking.role != _matchmaking_idle;
}

static void matchmaking_start_countdown(struct network_game_server *server, short players, short most_players)
{
	if (players < most_players)
		network_game_server_port_countdown_stop(server);
	else if (network_game_server_port_ok_to_countdown(server) && !network_game_server_port_countdown_paused(server))
		network_game_server_port_countdown_begin(server, MATCHMAKING_START_MILLISECONDS);
}

/* copies of network_server_manager.c's and network_client_manager.c's private state enums: keep the values in step */
enum { SERVER_PREGAME };

enum
{
	_client_state_searching,
	_client_state_joining,
	_client_state_pregame,
};

/* the screens behind are history only: loading the main menu runs
main_menu_initialize, which ends the client's game */
static void matchmaking_open_lobby(void)
{
	char const *const behind[] = {
		"pc\\main_menu\\main_menu",
		"pc\\main_menu\\multiplayer_type_select\\multiplayer_type_select_screen",
	};
	short index;

	ui_widgets_close_all();
	for (index = 0; index < NUMBEROF(behind); index++)
	{
		ui_widget_load_by_name_or_tag(
			pc_menus_screen("ui\\shell\\main_menu\\multiplayer_type_select\\connected\\pregame\\connected_pregame_screen"),
			NONE, NULL, NONE, tag_loaded(UI_WIDGET_DEFINITION_TAG, behind[index]), NONE, NONE);
	}
}

static void matchmaking_begin_host(struct p2p_mm_decision const *decision)
{
	char const *const *maps = decision->size <= P2P_MM_2V2 ? playlists[decision->playlist].small_maps :
		playlists[decision->playlist].maps;
	short count = 0;
	char const *map;

	while (maps[count])
		count++;
	map = maps[((unsigned long)rand() ^ system_milliseconds()) % (unsigned long)count];
	snprintf(matchmaking.map, sizeof(matchmaking.map), "levels\\test\\%s\\%s", map, map);
	matchmaking.playlist = (short)decision->playlist;
	matchmaking.size = (short)decision->size;
	pc_menu_player1_profile();
	player_ui_fast_setup_network_server();
	if (!global_network_game_server_get())
	{
		p2p_mm_cancel();
		return;
	}
	/* before p2p_set_hosting_public: a limit set later is not the one listed */
	network_game_server_port_set_settings(NULL, (short)p2p_mm_size_players(decision->size));
	p2p_set_hosting_allowed(TRUE);
	p2p_set_hosting_public(FALSE);
	matchmaking_open_lobby();
	matchmaking.role = _matchmaking_hosting;
	matchmaking.seconds = 0.0f;
	matchmaking.map_set = matchmaking.player_added = FALSE;
}

static void matchmaking_end_host(void)
{
	network_game_server_port_set_settings(NULL, 0);
	matchmaking.role = _matchmaking_idle;
}

static void matchmaking_update_host(float seconds)
{
	struct network_game_server *server = global_network_game_server_get();
	struct network_game_client *client = global_network_game_client_get();
	struct network_game *game = server ? network_game_server_get_game(server) : NULL;
	short state;

	if (!game)
	{
		p2p_mm_cancel();
		matchmaking_end_host();
		return;
	}
	state = network_game_server_get_state(server, NULL);
	matchmaking.seconds += seconds;
	if (state == SERVER_PREGAME)
	{
		if (!matchmaking.map_set && matchmaking.seconds >= MATCHMAKING_MAP_SECONDS)
		{
			struct game_variant variant;
			char const *name = matchmaking.size == P2P_MM_FFA_6 ? playlists[matchmaking.playlist].ffa_variant :
				playlists[matchmaking.playlist].team_variant;

			if (!name)
			{
				p2p_mm_cancel();
				matchmaking_end_host();
				return;
			}
			network_game_server_change_map_name(server, matchmaking.map);
			player_ui_set_game_variant(game_engine_get_variant_by_name(&variant, name));
			network_game_server_change_game_variant(server, &variant);
			matchmaking.map_set = TRUE;
		}
		if (!matchmaking.player_added && client && matchmaking.seconds >= MATCHMAKING_PLAYER_SECONDS)
			matchmaking.player_added = network_game_client_add_player(client, 0);
	}
	/* (from the host's own player on: no hosting record says 0 players) */
	if (game->player_count)
		/* a machine joining a game that is over waits out the whole join */
		p2p_mm_game(game_engine_running() && !game_engine_can_score() ? game->maximum_players : game->player_count,
			game->maximum_players, state != SERVER_PREGAME);
	if (state != SERVER_PREGAME)
		matchmaking.map_set = FALSE;
	else if (matchmaking.player_added)
		matchmaking_start_countdown(server, game->player_count, game->maximum_players);
}

static void matchmaking_begin_join(struct p2p_mm_decision const *decision)
{
	dispose_global_network_game_client();
	dispose_global_network_game_server();
	network_game_client_port_connection_ended();
	if (!create_global_network_game_client())
	{
		p2p_mm_joined(0);
		return;
	}
	game_connection_set(_game_connection_network_client);
	if (!pc_menu_multiplayer_player(0) || !p2p_invite_host(decision->invite, matchmaking.host) || !p2p_join_invite(decision->invite))
	{
		dispose_global_network_game_client();
		p2p_mm_joined(0);
		return;
	}
	matchmaking.role = _matchmaking_joining;
	matchmaking.seconds = 0.0f;
}

static void matchmaking_update_join(float seconds)
{
	struct network_game_client *client = global_network_game_client_get();
	short state_data;
	int stage;

	matchmaking.seconds += seconds;
	p2p_join_progress(&stage, NULL, 0);
	if (client && network_game_client_get_state(client, &state_data) == _client_state_pregame)
	{
		matchmaking_open_lobby();
		p2p_mm_joined(1);
		matchmaking.role = _matchmaking_joined;
		return;
	}
	network_game_client_port_join_host(matchmaking.host);
	if (!client || matchmaking.seconds >= MATCHMAKING_JOIN_SECONDS || stage == P2P_JOIN_FAILED)
	{
		dispose_global_network_game_client();
		p2p_mm_joined(0);
		matchmaking.role = _matchmaking_idle;
	}
}

static void matchmaking_update_joined(void)
{
	if (network_game_client_port_connection_ended())
		p2p_mm_game_lost();
	else if (global_network_game_client_get())
		return;
	else
		p2p_mm_cancel();
	matchmaking.role = _matchmaking_idle;
}

void matchmaking_update(float seconds)
{
	struct p2p_mm_status status;
	struct p2p_mm_decision decision;

	p2p_mm_get_status(&status);
	/* the search is off while joined: a joined game must not end here */
	if (status.state == P2P_MM_OFF && (matchmaking.role == _matchmaking_hosting || matchmaking.role == _matchmaking_joining))
	{
		if (matchmaking.role == _matchmaking_hosting)
			matchmaking_end_host();
		else
			dispose_global_network_game_client();
		matchmaking.role = _matchmaking_idle;
	}
	switch (matchmaking.role)
	{
	case _matchmaking_hosting:
		matchmaking_update_host(seconds);
		break;
	case _matchmaking_joining:
		matchmaking_update_join(seconds);
		break;
	case _matchmaking_joined:
		matchmaking_update_joined();
		break;
	}
	if ((matchmaking.role == _matchmaking_idle || matchmaking.role == _matchmaking_hosting) &&
		status.state != P2P_MM_OFF && main_menu_is_loaded())
	{
		p2p_mm_take_decision(&decision);
		if (decision.kind == P2P_MM_DECIDE_HOST && matchmaking.role == _matchmaking_idle)
			matchmaking_begin_host(&decision);
		else if (decision.kind == P2P_MM_DECIDE_JOIN)
		{
			if (matchmaking.role == _matchmaking_hosting)
				matchmaking_end_host();
			matchmaking_begin_join(&decision);
		}
	}
}
