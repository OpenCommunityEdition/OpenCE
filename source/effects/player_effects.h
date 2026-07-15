/*
PLAYER_EFFECTS.H

header included in hcex build.
*/

#ifndef __PLAYER_EFFECTS_H
#define __PLAYER_EFFECTS_H
#pragma once

/* ---------- headers */

#include "cseries/cseries.h"
#include "math/real_math.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/EXAMPLE.C */

/* ---------- globals */

/* ---------- public code */

void player_effect_dispose(
	void);
void player_effect_dispose_from_old_map(
	void);
void scripted_player_effect_set_rumble(
	real left_motor,
	real right_motor);

#endif // __PLAYER_EFFECTS_H
