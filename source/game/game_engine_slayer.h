/*
GAME_ENGINE_SLAYER.H
*/

#ifndef __GAME_ENGINE_SLAYER_H
#define __GAME_ENGINE_SLAYER_H
#pragma once

/* ---------- headers */

#include "game/game_engine.h"

/* ---------- prototypes/GAME_ENGINE_SLAYER.C */

void slayer_engine_dispose(
	void);

boolean slayer_engine_initialize_for_new_map(
	void);

void slayer_engine_dispose_from_old_map(
	void);

void slayer_engine_player_added(
	long player_index);

void slayer_engine_game_ending(
	void);

void slayer_engine_game_starting(
	void);

void slayer_engine_statistics_append(
	long statistic);

void slayer_engine_handle_client_message(
	void *message);

void slayer_engine_handle_server_message(
	void *message);

void slayer_engine_pregame_post_rasterize(
	void);

void slayer_engine_post_rasterize(
	void);

void slayer_engine_update(
	void);

boolean slayer_engine_allow_pick_up(
	long unit_index,
	long weapon_index);

void slayer_engine_player_damaged_player(
	long damaging_player_index,
	long dead_player_index,
	boolean damage_type);

void update_speed_for_score(
	long dead_player_index,
	long killing_player_index);

void slayer_engine_prespawn_player_update(
	long player_index);

long slayer_get_score(
	long player_index,
	enum get_score_type score_type);

boolean slayer_test_flag(
	long flag);

wchar_t *slayer_get_score_string(
	long player_index,
	wchar_t *buffer);

wchar_t *slayer_get_score_header_string(
	wchar_t *buffer);

wchar_t *slayer_get_team_score_string(
	long team_index,
	wchar_t *buffer);

void slayer_engine_player_killed_player(
	long killing_player_index,
	long killing_object_index,
	long dead_player_index,
	boolean friendly_fire);

void slayer_player_update(
	long player_index);

/* ---------- globals */

extern struct game_engine slayer_engine;

#endif // __GAME_ENGINE_SLAYER_H
