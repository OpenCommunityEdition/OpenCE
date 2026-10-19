/*
ENGINE_NETWORK_TEST.C

Hosting and joining a system link game in the Mac session, which loads a
map and runs it rather than coming through main.c's menu and lobby
(tools/mac_session_interactive.c).

port/linux/game/network_test.c does this on the other ports, but only once
the main menu's scenario is loaded (levels\ui\ui, which main.c loads and
the session does not), and its host path goes through
player_ui_fast_setup_network_server, which makes the server only if the
pregame screen's widget loads. So this makes the server and the client
with the same calls in the same order, leaving out the two that want the
menu, on the map the session already has.

network_test.c still logs every player's state every second, which it does
before that gate, so set debug.network_test as well as this one.

mac.network_test (HALO_MAC_NETWORK_TEST): "host:<map>" or "join".
*/

#include "cseries.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "main/main.h"
#include "interface/player_ui.h"
#include "networking/network_game_globals.h"
#include "networking/network_client_manager.h"
#include "networking/network_server_manager.h"
#include "networking/network_messages.h"
#include "bungie_net/network/transport.h"
#include "bungie_net/network/transport_endpoint_winsock.h"
#include "network_distributed.h" /* port: port/linux/game/network_distributed.c */
#include "engine_network_test.h"
#include "engine_menu.h"

#include <stdio.h>
#include <string.h>

/* port/mac/engine_cache_files.c's */
const char *cache_files_map_directory(void);
int mac_cache_files_map_file_exists(const char *path);

/* the platform layer's (port/linux/src/port_config.c) */
const char *config_string(char const *name);
int config_boolean(char const *name);
/* port/mac/engine_clipboard.c, and port/linux/src/p2p.c */
int platform_clipboard_get(char *text, int size);
int p2p_join_invite(const char *text);
double config_real(char const *name);
long config_integer(char const *name);
void platform_log(char const *format, ...);
/* network_server_message_handler.c's and network_server_manager.c's, as
   port/linux/game/network_distributed.c declares them */
short network_distributed_server_machines(long *machine_indices, short maximum);
unsigned long network_game_server_machine_address(long machine_index);

enum
{
	_mac_network_test_off,
	_mac_network_test_host,
	_mac_network_test_join
};

static struct
{
	int checked;
	int mode;
	char map_name[64];
	real start_delay;
	long score_to_win;
	real seconds;
	int set_up;
	int map_set;
	int player_added;
	int started;
	int joined;
	real joined_at;
	int network_started;
	int game_over;
	real reported_at;
} mac_network_test;

/* (network_client_manager.c's enum network_game_client_state: searching,
   joining, pregame) */
#define MAC_NETWORK_CLIENT_STATE_PREGAME 2

/* port: a game started from the menus. The session holds one map: the
   title's (the menus) or the game's. The map a network game starts on is
   known when the host says begin, in the title's session, so the session
   switches (the menus' START request) and the begin is made again in the
   game's, whose map matches. */
static struct
{
	char session_leaf[64];
	char pending_map[128];
	int pending;
	int frames;
	/* the game's connection while the begin is held: the session sets up a
	   local game (as the test's does) and the connection comes back with the begin */
	short connection;
} mac_network_begin;

static int mac_network_start(void)
{
	if (mac_network_test.network_started)
		return mac_network_test.network_started > 0;
	if (transport_initialize() != 0)
	{
		mac_network_test.network_started = -1;
		platform_log("mac network: the transport would not start");
		return 0;
	}
	initialize_network_game_packets();
	/* The key the game advertisement carries. network_game_server_create
	   calls this only on the Xbox (#ifdef xbox), and no port has an Xbox
	   encryption layer, so the key is unused; but the advertisement that
	   answers a broadcast search still asks for it
	   (network_server_message_handler.c), and transport_get_key_id halts
	   when nothing has been pushed. Without this the host dies the moment
	   another machine searches for a game. It is pushed once, here: the
	   call is not repeatable (it asserts that no key is held). */
	transport_server_initialize();
	mac_network_test.network_started = 1;
	return 1;
}

/* The session has its own initialization and does not run game_initialize
   (source/game/game.c), so the two things it does for the network have not
   been done: the transport's sockets, and the messages the machines send.
   Nothing else in game_initialize is wanted here. */
static int mac_network_test_start_network(void)
{
	return mac_network_start();
}

static void mac_network_test_read_settings(void)
{
	const char *setting = config_string("mac.network_test");

	mac_network_test.checked = 1;
	mac_network_test.mode = _mac_network_test_off;
	mac_network_test.start_delay = (real)config_real("debug.network_test_start");
	mac_network_test.score_to_win = config_integer("debug.network_test_score");
	if (!setting || !*setting)
		return;
	if (!strncmp(setting, "host:", 5))
	{
		char *variants;

		mac_network_test.mode = _mac_network_test_host;
		strncpy(mac_network_test.map_name, setting + 5, sizeof(mac_network_test.map_name) - 1);
		mac_network_test.map_name[sizeof(mac_network_test.map_name) - 1] = 0;
		/* (network_test.c's list of further games after a second colon: this
		plays the first and stops there) */
		variants = strchr(mac_network_test.map_name, ':');
		if (variants)
			*variants = 0;
		if (!mac_network_test.map_name[0])
		{
			mac_network_test.mode = _mac_network_test_off;
			platform_log("mac.network_test: host: names no map");
		}
	}
	else if (!strcmp(setting, "join"))
	{
		mac_network_test.mode = _mac_network_test_join;
	}
	else
	{
		platform_log("mac.network_test: \"%s\" is neither host:<map> nor join", setting);
	}
}

/* the map and the game type on the server, as picking them in the lobby does */
static void mac_network_test_choose_game(void)
{
	char path[128];
	struct game_variant variant;

	/* (a level of its own path, else a multiplayer map's name) */
	if (strchr(mac_network_test.map_name, '\\'))
		snprintf(path, sizeof(path), "%s", mac_network_test.map_name);
	else
		snprintf(path, sizeof(path), "levels\\test\\%s\\%s", mac_network_test.map_name,
			mac_network_test.map_name);
	network_game_server_change_map_name(global_network_game_server_get(), path);
	variant = *game_engine_get_variant_by_name(&variant, "slayer");
	/* debug.network_test_score: a short game */
	if (mac_network_test.score_to_win > 0)
		variant.universal_variant.score_to_win = mac_network_test.score_to_win;
	player_ui_set_game_variant(&variant);
	network_game_server_change_game_variant(global_network_game_server_get(), &variant);
	platform_log("mac network test: %s, slayer", path);
}

/* Which machines the distributed netcode will send to, and the address each
   one carries. The host sends a machine's in-game traffic to its reliable
   connection's address on NETWORK_GAME_CLIENT_PORT and says nothing when it
   decides not to send at all, so this says it instead. A machine recorded at
   127.0.0.1 is the one to look for: that is the sentinel for "the machine I
   am on", and mac_outgoing_address turns a send to it into a send to this
   machine. The addresses are in the host's own order, as get_endpoint_address
   leaves them. */
static void mac_network_test_report_machines(void)
{
	long all[HALO_PORT_MAXIMUM_NETWORK_MACHINES];
	long clients[HALO_PORT_MAXIMUM_NETWORK_MACHINES];
	short all_count, client_count, index, at;

	all_count = network_distributed_server_machines(all, (short)HALO_PORT_MAXIMUM_NETWORK_MACHINES);
	client_count = distributed_client_machines(clients, (short)HALO_PORT_MAXIMUM_NETWORK_MACHINES);
	platform_log("mac network test: %d machines in the game, %d of them sent to",
		(int)all_count, (int)client_count);
	for (index = 0; index < all_count; index++)
	{
		unsigned long address = network_game_server_machine_address(all[index]);
		int sent_to = 0;

		for (at = 0; at < client_count; at++)
			if (clients[at] == all[index])
				sent_to = 1;
		platform_log("mac network test:   machine #%ld at %lu.%lu.%lu.%lu %s",
			all[index], (address >> 24) & 0xFF, (address >> 16) & 0xFF,
			(address >> 8) & 0xFF, address & 0xFF,
			sent_to ? "is sent to" : "is not sent to (counted local)");
	}
}

static void mac_network_test_host_update(void)
{
	if (!mac_network_test.set_up)
	{
		mac_network_test.set_up = 1;
		if (!mac_network_test_start_network())
		{
			mac_network_test.mode = _mac_network_test_off;
			return;
		}
		dispose_global_network_game_server();
		dispose_global_network_game_client();
		game_connection_set(_game_connection_local);
		game_engine_playlist_initialize();
		network_game_accept_remote_connections(TRUE);
		if (!create_global_network_game_server())
			platform_log("mac network test: no server connection (WSA %d)", WSAGetLastError());
		else if (!create_global_network_game_client())
			platform_log("mac network test: no client connection");
		if (global_network_game_server_get() && global_network_game_client_get())
		{
			/* player_ui_fast_setup_network_server calls
			   game_engine_playlist_begin here. This does not: it takes the
			   stage's map name, and that precaches, which gives time to
			   cache_files_give_time_to_precache and so to main.c's
			   main_loop_of_death, which presents frames. The session is
			   inside its own frame when this runs, so that loop presents
			   from within a present and never comes back. The map is loaded
			   already and mac_network_test_choose_game below tells the
			   server which map and game type it is, which is what the stage
			   would have said. */
			/* the map and the game type before anything can join: the
			   server tells a client its settings the moment it connects, and
			   the host's own client connects at once, so the stage's default
			   map (carousel) would reach it first and it would refuse the
			   game for a map it has not got */
			mac_network_test_choose_game();
			mac_network_test.map_set = 1;
			game_connection_set(_game_connection_network_server);
			platform_log("mac network test: hosting %s", mac_network_test.map_name);
		}
		else
		{
			dispose_global_network_game_server();
			dispose_global_network_game_client();
			game_connection_set(_game_connection_local);
			mac_network_test.mode = _mac_network_test_off;
			platform_log("mac network test: the server could not be made");
		}
		return;
	}
	/* every five seconds, before and after the game starts */
	if (mac_network_test.seconds - mac_network_test.reported_at >= 5.0f)
	{
		mac_network_test.reported_at = mac_network_test.seconds;
		mac_network_test_report_machines();
	}
	if (mac_network_test.started)
		return;
	/* then a player for controller 1, as pressing A in the lobby adds one,
	then the start */
	if (!mac_network_test.player_added && mac_network_test.seconds >= 2.0f &&
		global_network_game_client_get())
	{
		mac_network_test.player_added =
			network_game_client_add_player(global_network_game_client_get(), 0);
	}
	if (mac_network_test.map_set && mac_network_test.player_added &&
		mac_network_test.seconds >= mac_network_test.start_delay)
	{
		mac_network_test.started = 1;
		network_game_client_request_immediate_start();
		platform_log("mac network test: starting the game");
	}
}

static void mac_network_test_join_update(void)
{
	if (!mac_network_test.set_up)
	{
		mac_network_test.set_up = 1;
		if (!mac_network_test_start_network())
		{
			mac_network_test.mode = _mac_network_test_off;
			return;
		}
		dispose_global_network_game_client();
		dispose_global_network_game_server();
		if (create_global_network_game_client())
		{
			game_connection_set(_game_connection_network_client);
			/* (the player joined to multiplayer first, as a player picking
			their profile does) */
			player_ui_local_player_joined_multiplayer_game(0);
			platform_log("mac network test: searching for games");
		}
		else
		{
			mac_network_test.mode = _mac_network_test_off;
			platform_log("mac network test: the client could not be made");
		}
		return;
	}
	if (!mac_network_test.joined)
	{
		if (network_game_client_join_first_available_game())
		{
			mac_network_test.joined = 1;
			mac_network_test.joined_at = mac_network_test.seconds;
			platform_log("mac network test: joining");
		}
		return;
	}
	/* and this machine's player, once the host has it in the game. On the
	   other ports the pregame screen asks for the player every frame until
	   it is in the settings (network_test.c says so where it joins the
	   player to multiplayer); there is no screen here, so ask once, as the
	   host's own client does. */
	if (!mac_network_test.player_added &&
		mac_network_test.seconds - mac_network_test.joined_at >= 2.0f &&
		global_network_game_client_get())
	{
		mac_network_test.player_added =
			network_game_client_add_player(global_network_game_client_get(), 0);
		if (mac_network_test.player_added)
			platform_log("mac network test: our player is in the game");
	}
}

void mac_engine_network_test_update(
	real seconds)
{
	if (!mac_network_test.checked)
		mac_network_test_read_settings();
	if (mac_network_test.mode == _mac_network_test_off)
		return;
	mac_network_test.seconds += seconds;
	if (mac_network_test.mode == _mac_network_test_host)
		mac_network_test_host_update();
	else
		mac_network_test_join_update();
}

/* ---------- the menus' network games (port: the lobby, the browser and the
   games they start are the shared menus' and netcode, as on the other
   ports; the session has to do what main.c's loop does around them) */

/* the netcode started (the transport's sockets and the packets) once the
   menus are up, before the first hosted or joined game: they ask for it
   from the lobby and the browser */
void mac_engine_network_menu_update(void)
{
	if (mac_engine_menu_enabled())
		mac_network_start();
}

/* the lower-cased last part of a map's path, without ".map" */
static void mac_network_map_leaf(const char *path, char *leaf, size_t size)
{
	const char *last = path ? path : "";
	size_t length, index;

	while (*path)
	{
		if (*path == '\\' || *path == '/')
			last = path + 1;
		path++;
	}
	length = strlen(last);
	if (length > 4 && !strcmp(last + length - 4, ".map"))
		length -= 4;
	if (length >= size)
		length = size - 1;
	for (index = 0; index < length; index++)
		leaf[index] = (char)((last[index] >= 'A' && last[index] <= 'Z') ? last[index] + 32 : last[index]);
	leaf[length] = 0;
}

/* a session opened on this map file (tools/mac_engine_session_probe.c) */
void mac_engine_network_session_open(const char *map_path)
{
	mac_network_map_leaf(map_path, mac_network_begin.session_leaf, sizeof(mac_network_begin.session_leaf));
	mac_network_begin.frames = 0;
}

/* a game's map this machine has, in the folder of the session's map: as
   precached as it can be in the menus' session, whose game session loads it
   (cache_files_give_time_to_precache) */
int mac_engine_network_map_precached(const char *map_name)
{
	char leaf[64], path[512];

	if (!mac_engine_menu_enabled() || !map_name || !*map_name || strchr(map_name, '/'))
		return 0;
	mac_network_map_leaf(map_name, leaf, sizeof(leaf));
	if (!leaf[0] || strlen(cache_files_map_directory()) + strlen(leaf) + 5 > sizeof(path))
		return 0;
	sprintf(path, "%s%s.map", cache_files_map_directory(), leaf);
	return mac_cache_files_map_file_exists(path);
}

/* ---------- an invite link copied before switching to the game

port/linux/src/sdl_platform.c watches the clipboard for the other builds.
The Mac does not compile it - it has its own window and platform layer - so
until now network.join_from_clipboard was a setting nothing on the Mac
read, and its menu row stayed hidden. This is the same watch, over the
Mac's own Carbon clipboard.

An invite is joined once: the text that was acted on is remembered, so a
link left on the clipboard does not rejoin every second, and copying the
same link again after leaving the game does. */

/* "halo://join/" anywhere in the text, in either case, as
platform_text_has_invite_link has it in sdl_platform.c. A bare code is not
enough: 64 hex digits alone are as often a checksum copied for something
else. */
static int mac_text_has_invite_link(const char *text)
{
	static const char prefix[] = "halo://join/";
	int length = (int)(sizeof(prefix) - 1);

	for (; *text; text++)
	{
		int index;

		for (index = 0; index < length && text[index] &&
			(text[index] | 0x20) == prefix[index]; index++)
		{
		}
		if (index == length)
			return 1;
	}
	return 0;
}

/* every frame; it looks at the clipboard about once a second. The count is
frames rather than a clock because this file has no clock of its own and
the rate it is called at is the session's own tick. */
void mac_engine_network_clipboard_update(void)
{
	static char acted_on[1024];
	static int frames;
	char text[1024];

	if (++frames < 30)
		return;
	frames = 0;
	if (!config_boolean("network.join_from_clipboard"))
		return;
	if (!platform_clipboard_get(text, (int)sizeof(text)) || !text[0])
		return;
	if (!strcmp(text, acted_on) || !mac_text_has_invite_link(text))
		return;
	/* (remembered whether or not the join is taken, so that a link the game
	   refuses is not tried again every second) */
	strncpy(acted_on, text, sizeof(acted_on) - 1);
	acted_on[sizeof(acted_on) - 1] = 0;
	if (p2p_join_invite(text))
		platform_log("Internet play: joining the invite link on the clipboard");
}

/* the host said begin, for this scenario (network_client_manager.c): TRUE if
   this session cannot make the game, whose own can; the begin is then made
   again once that session is up (mac_engine_network_begin_resume) */
int mac_engine_network_begin_deferred(const char *map_name)
{
	char leaf[64];

	if (!mac_engine_menu_enabled() || !mac_network_begin.session_leaf[0] || !map_name || !*map_name)
		return 0;
	mac_network_map_leaf(map_name, leaf, sizeof(leaf));
	if (!strcmp(leaf, mac_network_begin.session_leaf))
		return 0;
	if (!mac_network_begin.pending)
	{
		mac_network_begin.pending = 1;
		mac_network_begin.connection = game_connection();
		strncpy(mac_network_begin.pending_map, map_name, sizeof(mac_network_begin.pending_map) - 1);
		mac_network_begin.pending_map[sizeof(mac_network_begin.pending_map) - 1] = 0;
		platform_log("mac network: the game %s begins; the session is %s's, switching",
			mac_network_begin.pending_map, mac_network_begin.session_leaf);
		mac_engine_menu_request_game(mac_network_begin.pending_map);
	}
	return 1;
}

/* every frame: the begin that was held, in the session of its map */
void mac_engine_network_begin_resume(void)
{
	char leaf[64];
	struct network_game_client *client;

	if (!mac_network_begin.pending)
		return;
	mac_network_map_leaf(mac_network_begin.pending_map, leaf, sizeof(leaf));
	if (strcmp(leaf, mac_network_begin.session_leaf))
		return;
	/* (a few frames of the new session first) */
	if (++mac_network_begin.frames < 3)
		return;
	mac_network_begin.pending = 0;
	client = global_network_game_client_get();
	if (!client || network_game_client_get_state(client, NULL) != MAC_NETWORK_CLIENT_STATE_PREGAME)
	{
		platform_log("mac network: the game began, but this machine is no longer in the lobby");
		network_game_abort();
		return;
	}
	platform_log("mac network: beginning %s", mac_network_begin.pending_map);
	game_connection_set(mac_network_begin.connection);
	if (!network_game_client_game_has_started(client))
	{
		platform_log("mac network: the game could not begin");
		network_game_abort();
	}
}

/* the game the menus were to start cannot (its map is not here) */
void mac_engine_network_begin_failed(void)
{
	if (mac_network_begin.pending)
	{
		mac_network_begin.pending = 0;
		platform_log("mac network: the game %s cannot begin on this machine", mac_network_begin.pending_map);
		network_game_abort();
	}
}

/* A network game ending (or its host going) loads the title map, levels\ui\ui
   (main_load_ui_scenario, mac_main_game_render_view.py), which the session
   does not have: the one map it has is the game's. In the test the game just
   ends there, and the session's loop sees it and stops. With the menus the
   game is left: the network game is ended (the next frame disposes the
   client and the server) and the title's session is asked for. */
int mac_engine_network_test_leave_game(void)
{
	if (!mac_network_test.checked)
		mac_network_test_read_settings();
	if (mac_network_test.mode == _mac_network_test_off)
	{
		if (!mac_engine_menu_enabled())
			return 0;
		platform_log("mac network: the network game is over; back to the title");
		network_game_abort();
		main_goto_main_menu();
		return 1;
	}
	if (!mac_network_test.game_over)
		platform_log("mac network test: the game is over (the session has no title map to go back to)");
	mac_network_test.game_over = 1;
	return 1;
}

int mac_engine_network_test_game_over(void)
{
	return mac_network_test.game_over;
}
