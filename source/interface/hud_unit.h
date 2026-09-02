#ifndef __HUD_UNIT_H
#define __HUD_UNIT_H
#pragma once

struct player_datum;

void hud_initialize_unit_interface(
	void);
void hud_initialize_unit_interface_for_new_map(
	void);

void hud_dispose_unit_interface_from_old_map(
	void);
void hud_dispose_unit_interface(
	void);

void hud_fix_unit_data(
	short old_local_player_index,
	short new_local_player_index);

void hud_update_unit(
	void);
void hud_play_unit_sounds(
	struct player_datum const *player,
	boolean show_hud);

#endif // __HUD_UNIT_H
