/*
WEAPON_HUD_INTERFACE_DEFINITION.H

header included in hcex build.
*/

#ifndef __WEAPON_HUD_INTERFACE_DEFINITION_H
#define __WEAPON_HUD_INTERFACE_DEFINITION_H
#pragma once

/* ---------- headers */

#include "interface/hud_definitions.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct weapon_flash_state_definition
{
	short flags;
	short pad;
	short total_ammo;
	short loaded_ammo;
	short heat;
	short age;
	long unused[8];
};

struct weapon_hud_interface_definition
{
	struct tag_reference parent_hud;
	struct weapon_flash_state_definition flash_cutoffs;
	struct hud_absolute_placement_definition absolute_placement;
	struct tag_block statics;
	struct tag_block meters;
	struct tag_block numbers;
	struct tag_block crosshairs;
	struct tag_block overlays;
	unsigned long valid_crosshair_types_flags;
	struct tag_block warning_sounds;
	struct tag_block screen_effects;
	long unused1[33];
	struct icon_hud_element_definition messaging_icon;
	long unused2[12];
};

/* ---------- prototypes/EXAMPLE.C */

/* ---------- globals */

/* ---------- public code */

#endif // __WEAPON_HUD_INTERFACE_DEFINITION_H
