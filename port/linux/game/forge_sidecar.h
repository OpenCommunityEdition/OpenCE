/*
FORGE_SIDECAR.H

Read-only runtime loader and playback for OpenCE Forge .txt map sidecars
using active-map [MAP] tags (Phase 1).
*/

#ifndef __FORGE_SIDECAR_H
#define __FORGE_SIDECAR_H

/* ---------- prototypes/FORGE_SIDECAR.C */

void forge_sidecar_menu_loaded(
	char const *map_name);

void forge_sidecar_menu_unloaded(
	void);

boolean forge_sidecar_choose_multiplayer_map(
	short level_index);

void forge_sidecar_tick(
	void);

int forge_sidecar_should_skip_netgame_equipment_spawn(
	long definition_index,
	void const *position);

#endif
