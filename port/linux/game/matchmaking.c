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

#define MATCHMAKING_PLAYER_SECONDS 2.0f

char const *pc_menus_screen(char const *name);
boolean main_menu_is_loaded(void);
boolean pc_menu_player1_profile(void);
void platform_log(char const *format, ...);

enum
{
	_matchmaking_idle,
	_matchmaking_hosting,
};

static struct
{
	short role;
	float seconds;
	boolean player_added;
} matchmaking;

/* copies of network_server_manager.c's and network_client_manager.c's private state enums: keep the values in step */
enum { SERVER_PREGAME };

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
	matchmaking.player_added = FALSE;
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
		if (!matchmaking.player_added && client && matchmaking.seconds >= MATCHMAKING_PLAYER_SECONDS)
			matchmaking.player_added = network_game_client_add_player(client, 0);
	}
	/* (from the host's own player on: no hosting record says 0 players) */
	if (game->player_count)
		/* a machine joining a game that is over waits out the whole join */
		p2p_mm_game(game_engine_running() && !game_engine_can_score() ? game->maximum_players : game->player_count,
			game->maximum_players, state != SERVER_PREGAME);
}

void matchmaking_update(float seconds)
{
	struct p2p_mm_status status;
	struct p2p_mm_decision decision;

	p2p_mm_get_status(&status);
	if (status.state == P2P_MM_OFF && matchmaking.role == _matchmaking_hosting)
		matchmaking_end_host();
	switch (matchmaking.role)
	{
	case _matchmaking_hosting:
		matchmaking_update_host(seconds);
		break;
	}
	if (matchmaking.role == _matchmaking_idle && status.state != P2P_MM_OFF && main_menu_is_loaded())
	{
		p2p_mm_take_decision(&decision);
		if (decision.kind == P2P_MM_DECIDE_HOST)
			matchmaking_begin_host(&decision);
	}
}
