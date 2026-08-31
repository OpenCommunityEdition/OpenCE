#ifndef __HUD_UNIT_H
#define __HUD_UNIT_H
#pragma once

void hud_dispose_unit_interface_from_old_map(
	void);
void hud_dispose_unit_interface(
	void);

void hud_fix_unit_data(
	short old_local_player_index,
	short new_local_player_index);

#endif // __HUD_UNIT_H
