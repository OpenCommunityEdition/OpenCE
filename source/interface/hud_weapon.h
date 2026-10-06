#ifndef __HUD_WEAPON_H
#define __HUD_WEAPON_H
#pragma once

struct player_datum;

void hud_initialize_weapon_interface(
	void);
void hud_initialize_weapon_interface_for_new_map(
	void);

void hud_dispose_weapon_interface_from_old_map(
	void);
void hud_dispose_weapon_interface(
	void);

/* port: network co-op (port/linux/game/network_coop.c) */
long hud_weapon_port_script_flags(
	void);
void hud_weapon_port_set_script_flags(
	long flags);
void scripted_hud_show_crosshair(
	boolean show);

void hud_fix_weapon_data(
	short old_local_player_index,
	short new_local_player_index);

void hud_update_weapon(
	void);
void hud_render_weapon_interface(
	struct player_datum *player);
#ifdef HALO_VR
void hud_render_weapon_crosshairs(
	struct player_datum *player);
#endif

#endif // __HUD_WEAPON_H
