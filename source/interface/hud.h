/*
HUD.H

header included in hcex build.
*/

#ifndef __HUD_H
#define __HUD_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/HUD.C */

void hud_initialize_for_new_map(
	void);
void hud_initialize_nav_points(
	void);
void hud_initialize_nav_points_for_new_map(
	void);
void hud_dispose_nav_points_from_old_map(
	void);
void hud_dispose_nav_points(
	void);
void hud_update_nav_points(
	void);

wchar_t const *hud_get_item_string(
	long string_index);

void hud_load(
	boolean load);
void hud_autosave(
	boolean active);
void hud_draw_players(
	void);

void hud_picked_up_powerup(
	short local_player_index,
	long powerup_definition_index);
void hud_picked_up_grenade(
	short local_player_index,
	long grenade_definition_index);
void hud_picked_up_ammunition(
	short local_player_index,
	long weapon_definition_index,
	short ammunition_count);
void hud_picked_up_weapon(
	short local_player_index,
	long weapon_definition_index);

/* ---------- globals */

/* ---------- public code */

#endif // __HUD_H
