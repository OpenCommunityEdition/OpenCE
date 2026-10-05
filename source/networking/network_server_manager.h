/*
NETWORK_SERVER_MANAGER.H

header included in hcex build.
*/

#ifndef __NETWORK_SERVER_MANAGER_H
#define __NETWORK_SERVER_MANAGER_H
#pragma once

/* ---------- headers */

#include "cseries.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/NETWORK_SERVER_MANAGER.C */

/* port: whether the host's game is being played (not its lobby) */
boolean network_game_server_playing(
	struct network_game_server *server);

struct network_game_server;
struct game_variant;

struct network_game_server *network_game_server_create(
	void);
void network_game_server_dispose(
	struct network_game_server *server);
boolean network_game_server_idle(
	struct network_game_server *server);
void network_game_server_open_game(
	struct network_game_server *server);
void network_game_server_switch_to_postgame(
	struct network_game_server *server);
boolean network_game_server_graceful_shutdown(
	struct network_game_server *server);
boolean network_game_server_reset_to_pregame(
	struct network_game_server *server);
void network_game_server_pause_countdown(
	struct network_game_server *server,
	boolean pause_countdown);
void network_game_generate_join_game_token(
	byte *join_token);
void network_game_server_kick_machine(
	long machine_index);
/* the host's ban command (console.c, hs.c) */
enum
{
	NETWORK_GAME_SERVER_NAME_TEXT_SIZE = 16,
};
/* port: the PC menus' server settings: the game's name (empty: the
machine's) and the most players (0: every player the build holds), for
every game the server sets up */
void network_game_server_port_set_settings(
	wchar_t const *name,
	long maximum_players);
/* port: a co-op level was won (main.c). Ends the round for everyone as in
multiplayer; the next round is on next_map (NULL repeats the level). */
void network_game_server_port_cooperative_won(
	char const *next_map);
/* port: co-op server settings: the difficulty, sent to clients, and the
most players a co-op game starts with (Server Setup sets its own) */
void network_game_server_port_set_cooperative(
	struct network_game_server *server,
	short difficulty);
/* port: co-op's friendly fire between its players (a _friendly_fire_ mode:
Server Setup's FRIENDLY FIRE), kept for the levels after */
void network_game_server_port_set_cooperative_friendly_fire(
	short friendly_fire);
boolean network_game_server_ban_player(
	char const *text);
/* port: the host's kick command: as the ban command, but nothing kept (no
bans.txt line, the address not kept out): the player may join again at once */
boolean network_game_server_kick_player(
	char const *text);
short network_game_server_matching_player_names(
	char const *text,
	char (*names)[NETWORK_GAME_SERVER_NAME_TEXT_SIZE],
	short maximum_count);
unsigned long network_game_server_machine_address(
	long machine_index);
char const *network_game_server_machine_hardware_id(
	long machine_index);
void network_game_server_update_ticks(
	struct network_game_server *server,
	short tick_count);
void network_game_server_change_map_name(
	struct network_game_server *server,
	char const *map_name);
void network_game_server_change_game_variant(
	struct network_game_server *server,
	struct game_variant *variant);

/* ---------- globals */

/* ---------- public code */

#endif // __NETWORK_SERVER_MANAGER_H
