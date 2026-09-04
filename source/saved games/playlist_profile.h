/*
PLAYLIST_PROFILE.H

header included in hcex build.
*/

#ifndef __PLAYLIST_PROFILE_H
#define __PLAYLIST_PROFILE_H
#pragma once

/* ---------- headers */

#include "cseries/cseries.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct playlist_profile;

/* ---------- prototypes/PLAYLIST_PROFILE.C */

void playlist_profiles_initialize(
	void);
void playlist_profiles_dispose(
	void);
word playlist_profile_number_of_default_profiles_on_disk(
	void);
boolean playlist_profile_get(
	long playlist_profile_index,
	struct playlist_profile *profile);
long playlist_profile_new(
	short local_player_index,
	wchar_t *name);
void playlist_profile_save(
	long playlist_profile_index,
	struct playlist_profile *profile);
void playlist_profiles_enumerate_available_to_local_player_index(
	short local_player_index,
	word *number_of_profiles,
	long *playlist_profile_indices);

/* ---------- globals */

/* ---------- public code */

#endif // __PLAYLIST_PROFILE_H
