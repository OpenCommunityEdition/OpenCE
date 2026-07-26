/*
PLAYER_RUMBLE.H
*/

#ifndef __PLAYER_RUMBLE_H
#define __PLAYER_RUMBLE_H
#pragma once

/* ---------- headers */

#include "cseries/cseries.h"
#include "math/real_math.h"

/* ---------- prototypes/PLAYER_RUMBLE.C */

void rumble_dispose(
	void);
void rumble_player_set_scripted_values(
	real left_motor,
	real right_motor);
void rumble_player_clear(
	short local_player_index);

#endif // __PLAYER_RUMBLE_H
