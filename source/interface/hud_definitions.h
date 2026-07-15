/*
HUD_DEFINITIONS.H

header included in hcex build.
*/

#ifndef __HUD_DEFINITIONS_H
#define __HUD_DEFINITIONS_H
#pragma once

/* ---------- constants */

enum hud_anchor
{
	_hud_anchor_top_left,
	_hud_anchor_top_right,
	_hud_anchor_bottom_left,
	_hud_anchor_bottom_right,
	_hud_anchor_center,

	NUMBER_OF_HUD_ANCHORS
};

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/EXAMPLE.C */

/* ---------- globals */

extern char const *global_hud_anchor_names[NUMBER_OF_HUD_ANCHORS];

/* ---------- public code */

#endif // __HUD_DEFINITIONS_H
