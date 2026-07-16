/*
PLAYER_PROFILE.H

header included in hcex build.
*/

#ifndef __PLAYER_PROFILE_H
#define __PLAYER_PROFILE_H
#pragma once

/* ---------- headers */

#include "cseries/cseries.h"
#include "math/real_math.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/EXAMPLE.C */

long player_profile_get_random_color(
	void);
long player_profile_get_random_good_color(
	void);
boolean player_profile_get_enclosing_directory_path(
	long profile,
	char *full_path);
short player_profile_number_of_available_primary_colors(
	void);
real_rgb_color *player_profile_get_rgb_color(
	real_rgb_color *result,
	long color_index);
void player_profile_save_level_completed(
	short local_player_index);

/* ---------- globals */

/* ---------- public code */

#endif // __PLAYER_PROFILE_H
