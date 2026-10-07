/*
SOUND_PREFERENCES.H

header included in hcex build.
*/

#ifndef __SOUND_PREFERENCES_H
#define __SOUND_PREFERENCES_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

enum
{
	#ifdef HALO_NATIVE_AUDIO
		NUMBER_OF_SOUND_CHANNEL_TYPES = 15,
	#else
	NUMBER_OF_SOUND_CHANNEL_TYPES = 4,
	#endif
	ORIGINAL_SOUND_CHANNEL_TYPES = 4,
};

struct sound_preferences
{
	short platform;
	short actual_channel_counts[NUMBER_OF_SOUND_CHANNEL_TYPES];
	short virtual_channel_counts[NUMBER_OF_SOUND_CHANNEL_TYPES];
	short unused;
};

/* ---------- prototypes/SOUND_PREFERENCES.C */

void read_sound_preferences(
	struct sound_preferences **preferences);
void write_sound_preferences(
	void);

/* ---------- globals */

extern short sound_channel_type_flags[NUMBER_OF_SOUND_CHANNEL_TYPES];

/* ---------- public code */

#endif // __SOUND_PREFERENCES_H
