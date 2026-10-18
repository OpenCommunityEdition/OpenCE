/* Retail grenade HUD records as consumed by hud_weapon.c and hud_draw.c. */
#ifndef HALO_MAC_NATIVE_GRENADE_HUD_LAYOUT_H
#define HALO_MAC_NATIVE_GRENADE_HUD_LAYOUT_H
#include "interface/hud_definitions.h"
#include "interface/unit_hud_interface_definition.h"
#include "native_hud_component_layout.h"

struct grenade_count_panel_definition
{
	struct static_hud_element_definition background;
	struct number_hud_element_definition numbers;
	short flash_cutoff;
	short pad;
};

struct grenade_hud_interface_definition
{
	struct hud_absolute_placement_definition absolute_placement;
	struct static_hud_element_definition background;
	struct grenade_count_panel_definition grenade_count_panel;
	struct weapon_hud_overlay_definition overlays;
	struct tag_block warning_sounds;
	long unused0[17];
	struct tag_reference messaging_icon_bitmap;
	long unused1[12];
};

typedef char native_grenade_hud_size[sizeof(struct grenade_hud_interface_definition)==0x1F8?1:-1];
typedef char native_grenade_hud_absolute_placement_offset[
	offsetof(struct grenade_hud_interface_definition,absolute_placement)==0x00?1:-1];
typedef char native_grenade_hud_background_offset[
	offsetof(struct grenade_hud_interface_definition,background)==0x24?1:-1];
typedef char native_grenade_hud_count_panel_offset[
	offsetof(struct grenade_hud_interface_definition,grenade_count_panel)==0x8C?1:-1];
typedef char native_grenade_hud_flash_cutoff_offset[
	offsetof(struct grenade_hud_interface_definition,grenade_count_panel.flash_cutoff)==0x148?1:-1];
typedef char native_grenade_hud_overlay_offset[
	offsetof(struct grenade_hud_interface_definition,overlays)==0x14C?1:-1];
typedef char native_grenade_hud_overlay_items_offset[
	offsetof(struct grenade_hud_interface_definition,overlays.items)==0x15C?1:-1];
typedef char native_grenade_hud_warning_sounds_offset[
	offsetof(struct grenade_hud_interface_definition,warning_sounds)==0x168?1:-1];
typedef char native_grenade_hud_messaging_icon_offset[
	offsetof(struct grenade_hud_interface_definition,messaging_icon_bitmap)==0x1B8?1:-1];
#endif
