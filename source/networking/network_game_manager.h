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
struct network_machine;
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
boolean network_game_update_machine(
	struct network_game *game,
	struct network_machine *machine);
void xbox_set_machine_name(
	char const *machine_name);
void network_game_generate_local_machine_name(
	wchar_t *machine_name);
boolean network_game_spawn_player(
	struct network_player *player);
boolean network_game_player_is_valid(
	struct network_player *player,
	struct network_game *game);
void network_game_reset_for_next_round(
	struct network_game *game,
	boolean load_ui);

/* ---------- globals */

/* ---------- public code */

#endif // __NETWORK_GAME_MANAGER_H
