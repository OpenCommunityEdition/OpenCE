/*
PLAYER_UI.H

header included in hcex build.
*/

#ifndef __PLAYER_UI_H
#define __PLAYER_UI_H
#pragma once

/* ---------- headers */

#include "cseries/cseries.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct player_profile;
struct playlist_profile;

/* ---------- prototypes/PLAYER_UI.C */

void player_ui_dispose(
	void);
void player_ui_reset_single_player_local_player_controllers(
	void);
short player_ui_get_single_player_local_player_from_controller(
	short controller_index);
void player_ui_autojoin_players_to_next_multiplayer_game(
	void);
void player_ui_clear_multiplayer_variant(
	void);
long player_ui_get_active_player_profile_index(
	short local_player_index);
struct player_profile *player_ui_get_edit_player_profile(
	void);
struct playlist_profile *player_ui_get_edit_playlist_profile(
	void);
boolean player0_look_pitch_is_inverted(
	void);
boolean player0_joystick_set_is_normal(
	void);
void player_ui_end_editing_profile(
	void);

/* ---------- globals */

/* ---------- public code */

#endif // __PLAYER_UI_H
