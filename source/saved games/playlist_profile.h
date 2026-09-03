/*
PLAYLIST_PROFILE.H

header included in hcex build.
*/

#ifndef __PLAYLIST_PROFILE_H
#define __PLAYLIST_PROFILE_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct playlist_profile;

/* ---------- prototypes/PLAYLIST_PROFILE.C */

boolean playlist_profile_get(
	long playlist_profile_index,
	struct playlist_profile *profile);
void playlist_profiles_enumerate_available_to_local_player_index(
	short local_player_index,
	word *number_of_profiles,
	long *playlist_profile_indices);

/* ---------- globals */

/* ---------- public code */

#endif // __PLAYLIST_PROFILE_H
