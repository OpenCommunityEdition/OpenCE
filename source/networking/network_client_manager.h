/*
NETWORK_CLIENT_MANAGER.H

header included in hcex build.
*/

#ifndef __NETWORK_CLIENT_MANAGER_H
#define __NETWORK_CLIENT_MANAGER_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct network_game_client;

/* ---------- prototypes/NETWORK_CLIENT_MANAGER.C */

boolean transport_network_available(
	void);
short network_game_client_get_state(
	struct network_game_client *client,
	short *state_data);
short network_game_client_get_seconds_to_game_start(
	struct network_game_client *client);
boolean network_game_client_add_player(
	struct network_game_client *client,
	short controller_index);

/* ---------- globals */

/* ---------- public code */

#endif // __NETWORK_CLIENT_MANAGER_H
