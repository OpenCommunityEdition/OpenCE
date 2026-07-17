/*
NETWORK_GAME_MANAGER.H

header included in hcex build.
*/

#ifndef __NETWORK_GAME_MANAGER_H
#define __NETWORK_GAME_MANAGER_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct network_game;
struct network_player;

/* ---------- prototypes/EXAMPLE.C */

void network_game_invalidate_player(
	struct network_player *player);
void network_game_end_and_load_ui(
	struct network_game *game);
void code_0011a4c0(
	void);
boolean network_player_is_valid(
	struct network_player *player);

/* ---------- globals */

/* ---------- public code */

#endif // __NETWORK_GAME_MANAGER_H
