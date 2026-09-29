/* Browser quick play uses the normal session and player APIs. A host recovery
request restarts the session on this thread; it never generates player input.
Cancellation and ordinary match endings leave the menus in control. */
#ifdef HALO_WEB

#ifdef HALO_QUICK_PLAY_TEST
#include "quick_play_test_stubs.h"
#else
#include "cseries.h"
#include "main/main.h"
#include "interface/player_ui.h"
#include "interface/ui_widget.h"
#include "networking/network_game_globals.h"
#include "networking/network_client_manager.h"
#include "networking/network_server_manager.h"
#include "game/game.h"
#include "game/game_engine.h"
#endif

#include <string.h>

const char *config_string(const char *name);
int web_quick_play_take_cancel(void);
int web_quick_play_take_restart(int *host, unsigned long *target);
void web_quick_play_report(const char *phase, const char *message);

enum { QUICK_OFF, QUICK_SETTLING, QUICK_SEARCHING, QUICK_JOINING, QUICK_PREGAME,
	QUICK_STARTING, QUICK_PLAYING, QUICK_DONE };

static struct
{
	boolean checked, host, owned, map_set, player_confirmed;
	short phase;
	unsigned long target, phase_at, player_at, retry_at, menu_at;
	boolean menu_seen;
} quick_play;

/* transport_client_start converts the socket's network-order word with
SWAP4 before writing transport_address. Parse into that engine address order
(100.86.56.19 -> 0x64563813), not the browser ring/socket order. */
static boolean quick_play_parse_address(const char *text, unsigned long *address)
{
	unsigned int octet;
	int index;
	*address = 0;
	if (!text[0])
		return TRUE;
	for (index = 0; index < 4; index++)
	{
		int digits = 0;
		octet = 0;
		while (*text >= '0' && *text <= '9')
		{
			if (++digits > 3)
				return FALSE;
			octet = octet * 10 + (unsigned int)(*text++ - '0');
		}
		if (!digits || octet > 255 || (!index && (!octet || octet >= 224)))
			return FALSE;
		*address = (*address << 8) | octet;
		if (index < 3 ? *text++ != '.' : *text != 0)
			return FALSE;
	}
	return TRUE;
}

static void quick_play_phase(short phase, unsigned long now, const char *name, const char *message)
{
	quick_play.phase = phase;
	quick_play.phase_at = now;
	web_quick_play_report(name, message);
}

static void quick_play_finish(const char *phase, const char *message, boolean leave)
{
	quick_play.phase = QUICK_DONE;
	if (leave)
	{
		if (quick_play.owned && global_network_game_client_get())
			network_game_abort();
		main_goto_main_menu();
	}
	web_quick_play_report(phase, message);
}

static void quick_play_show_lobby(void)
{
	ui_widgets_close_all();
	ui_widget_load_by_name_or_tag(
		"ui\\shell\\main_menu\\multiplayer_type_select\\connected\\pregame\\connected_pregame_screen",
		NONE, NULL, NONE, NONE, NONE, NONE);
}

/* SDL's swap wait runs on the game thread, including in manual System Link. */
int web_multiplayer_active(void)
{
	return global_network_game_client_get() &&
		(game_connection() == _game_connection_network_server ||
		 game_connection() == _game_connection_network_client);
}

/* The quick-play host owns every spawn, including late joins and respawns. */
boolean web_quick_play_pistol_starts(void)
{
	return quick_play.host && quick_play.owned && quick_play.map_set &&
		(quick_play.phase == QUICK_STARTING || quick_play.phase == QUICK_PLAYING);
}

void quick_play_update(boolean main_menu_loaded)
{
	unsigned long now = system_milliseconds();
	struct network_game_client *client;
	short state;
	int restart_host;
	unsigned long restart_target;

	if (web_quick_play_take_restart(&restart_host, &restart_target))
	{
		if (quick_play.owned && global_network_game_client_get())
			network_game_abort();
		main_goto_main_menu();
		memset(&quick_play, 0, sizeof(quick_play));
		quick_play.checked = TRUE;
		quick_play.host = restart_host;
		quick_play.target = restart_target;
		quick_play_phase(QUICK_SETTLING, now, "loading", "Restarting multiplayer with the replacement host...");
	}

	if (!quick_play.checked)
	{
		const char *mode = config_string("network.quick_play");
		quick_play.checked = TRUE;
		if (!mode[0])
			return;
		if (strcmp(mode, "host") && strcmp(mode, "join"))
		{
			quick_play_finish("error", "Invalid quick-play mode. Use the game menus or reload to try again.", FALSE);
			return;
		}
		quick_play.host = !strcmp(mode, "host");
		if (!quick_play_parse_address(config_string("network.quick_play_target"), &quick_play.target))
		{
			quick_play_finish("error", "Invalid quick-play host address.", FALSE);
			return;
		}
		quick_play_phase(QUICK_SETTLING, now, "loading", "Opening multiplayer...");
	}
	/* The page writes an atomic request; only this game thread touches game state. */
	if (web_quick_play_take_cancel() && quick_play.phase != QUICK_OFF)
	{
		quick_play_finish("menu", "Returned to the main menu.", TRUE);
		return;
	}
	if (quick_play.phase == QUICK_OFF || quick_play.phase == QUICK_DONE)
		return;
	if (quick_play.phase == QUICK_SETTLING)
	{
		if (now - quick_play.phase_at > 120000UL)
		{
			quick_play_finish("error", "The game took too long to open multiplayer.", TRUE);
			return;
		}
		if (!main_menu_loaded)
			return;
		if (!quick_play.menu_seen)
		{
			quick_play.menu_seen = TRUE;
			quick_play.menu_at = now;
		}
		if (now - quick_play.menu_at < 2000UL)
			return;
		quick_play.owned = TRUE;
		if (quick_play.host)
		{
			player_ui_fast_setup_network_server();
			if (!global_network_game_server_get() || !global_network_game_client_get())
			{
				quick_play_finish("error", "Could not create the multiplayer game.", TRUE);
				return;
			}
			quick_play_phase(QUICK_JOINING, now, "hosting", "Creating Blood Gulch Slayer...");
		}
		else
		{
			dispose_global_network_game_client();
			dispose_global_network_game_server();
			if (!create_global_network_game_client())
			{
				quick_play_finish("error", "Could not open multiplayer networking.", TRUE);
				return;
			}
			game_connection_set(_game_connection_network_client);
			quick_play_phase(QUICK_SEARCHING, now, "searching", "Finding the multiplayer game...");
		}
		quick_play.retry_at = now;
		return;
	}

	client = global_network_game_client_get();
	if (!client || game_connection() != (quick_play.host ? _game_connection_network_server : _game_connection_network_client))
	{
		/* The player or normal engine error handling already left this session. */
		quick_play_finish(!quick_play.host && quick_play.phase == QUICK_PLAYING ? "disconnected" : "menu",
			"Multiplayer closed. Use the game menus or reload to play again.", FALSE);
		return;
	}
	if (network_game_client_get_error(client))
	{
		quick_play_finish(!quick_play.host && quick_play.phase == QUICK_PLAYING ? "disconnected" : "error",
			"The multiplayer connection failed. Use the game menus or reload to try again.", TRUE);
		return;
	}
	state = network_game_client_get_state(client, NULL);
	if (state == _network_game_client_state_ingame && !main_menu_loaded)
	{
		if (quick_play.phase != QUICK_PLAYING)
			quick_play_phase(QUICK_PLAYING, now, "playing", "Multiplayer is ready.");
		return;
	}
	if (quick_play.phase == QUICK_PLAYING || state == _network_game_client_state_postgame)
	{
		quick_play_finish("menu", "The match ended. Use the game menus or reload to play again.", TRUE);
		return;
	}
	if (now - quick_play.phase_at > (quick_play.phase == QUICK_SEARCHING ? 60000UL : 120000UL))
	{
		quick_play_finish("error", quick_play.phase == QUICK_SEARCHING ?
			"No open game was found. The host may have left or the game may be full." :
			"The multiplayer game did not start in time. Use the game menus or reload to try again.", TRUE);
		return;
	}
	if (quick_play.phase == QUICK_SEARCHING)
	{
		if (now - quick_play.retry_at >= 500UL)
		{
			short result;
			quick_play.retry_at = now;
			result = network_game_client_quick_join(quick_play.target);
			if (result < 0)
			{
				quick_play_finish("error", result == -1 ? "The host uses a different game version." :
					result == -2 ? "This game is full or no longer accepting players." :
					"Could not connect to the multiplayer game.", TRUE);
			}
			else if (result > 0)
			{
				quick_play_show_lobby();
				quick_play_phase(QUICK_JOINING, now, "joining", "Joining the multiplayer game...");
			}
		}
		return;
	}
	if (state == _network_game_client_state_pregame)
	{
		if (quick_play.phase == QUICK_JOINING)
			quick_play_phase(QUICK_PREGAME, now, "waiting", "Adding your player...");
		if (quick_play.host && !quick_play.map_set)
		{
			struct game_variant variant;
			struct network_game_server *server = global_network_game_server_get();
			if (!server)
			{
				quick_play_finish("error", "The multiplayer host closed.", TRUE);
				return;
			}
			network_game_server_change_map_name(server, "levels\\test\\bloodgulch\\bloodgulch");
			variant = *game_engine_get_variant_by_name(&variant, "slayer");
			player_ui_set_game_variant(&variant);
			network_game_server_change_game_variant(server, &variant);
			if (!network_game_server_enable_quick_play(server))
			{
				quick_play_finish("error", "Quick play requires the distributed Slayer game mode.", TRUE);
				return;
			}
			quick_play.map_set = TRUE;
		}
		if (!quick_play.player_confirmed && network_game_client_has_local_player(client, 0))
		{
			quick_play.player_confirmed = TRUE;
			quick_play.player_at = now;
			web_quick_play_report("waiting", quick_play.host ? "Starting Blood Gulch Slayer..." : "Waiting for the host to start...");
		}
		if (!quick_play.player_confirmed && now - quick_play.retry_at >= 500UL)
		{
			quick_play.retry_at = now;
			network_game_client_add_player(client, 0);
		}
		if (quick_play.host && quick_play.player_confirmed && now - quick_play.player_at >= 3000UL &&
			(quick_play.phase != QUICK_STARTING || (now - quick_play.retry_at >= 1000UL &&
				network_game_client_get_seconds_to_game_start(client) < 0)))
		{
			/* A start request can arrive while lobby changes are still pending.
			Retry only until the normal countdown acknowledges it; retain the
			overall deadline, precache checks and remote-player readiness gates. */
			network_game_server_pause_countdown(global_network_game_server_get(), FALSE);
			network_game_client_request_immediate_start();
			quick_play.retry_at = now;
			if (quick_play.phase != QUICK_STARTING)
				quick_play_phase(QUICK_STARTING, now, "loading", "Loading Blood Gulch...");
		}
		else if (!quick_play.host && quick_play.phase != QUICK_STARTING && network_game_client_server_has_started_game(client))
			quick_play_phase(QUICK_STARTING, now, "loading", "Loading the multiplayer map...");
	}
}
#endif
