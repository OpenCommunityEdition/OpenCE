/*
NETWORK_SERVER_MANAGER.H

header included in hcex build.
*/

#ifndef __NETWORK_SERVER_MANAGER_H
#define __NETWORK_SERVER_MANAGER_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/NETWORK_SERVER_MANAGER.C */

struct network_game_server;

void network_game_server_reset_to_pregame(
	struct network_game_server *server);
void network_game_server_pause_countdown(
	struct network_game_server *server,
	boolean pause_countdown);

/* ---------- globals */

/* ---------- public code */

#endif // __NETWORK_SERVER_MANAGER_H
