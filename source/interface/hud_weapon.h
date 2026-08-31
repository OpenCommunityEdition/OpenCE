#ifndef __HUD_WEAPON_H
#define __HUD_WEAPON_H
#pragma once

void hud_dispose_weapon_interface_from_old_map(
	void);
void hud_dispose_weapon_interface(
	void);

void hud_fix_weapon_data(
	short old_local_player_index,
	short new_local_player_index);

#endif // __HUD_WEAPON_H
