/*
SOUND_PREFERENCES.C

symbols in this file:
001BF310 0010:
	_read_sound_preferences (0000)
001BF320 0010:
	_write_sound_preferences (0000)
00317A84 001c:
	_data_00317a84 (0000)
	_sound_channel_type_flags (0014)
*/

/* ---------- headers */

#include "sound_preferences.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

static struct sound_preferences default_sound_preferences =
{
	0,
	/* Native PCM gets dedicated streams so it cannot borrow an ADPCM channel.
	 * Keep the original voice counts for cached Xbox sounds. */
	#ifdef HALO_NATIVE_AUDIO
	{ 8, 43, 10, 10, 2, 8, 8, 4, 8, 4, 8, 4, 8, 4, 8 },
	{ 7, 38, 9, 9, 2, 8, 8, 4, 8, 4, 8, 4, 8, 4, 8 },
	#else
	{ 10, 51, 10, 10 },
	{ 9, 46, 9, 9 },
	#endif
	0,
};

/* ADPCM: mono 22k 2D/3D, stereo 22k/44k 2D, mono 44k 2D/3D,
 * stereo 44k 3D. Native PCM: mono/stereo 22k/44k, each 2D/3D. */
short sound_channel_type_flags[NUMBER_OF_SOUND_CHANNEL_TYPES] =
	{ 8, 9, 10, 14
	#ifdef HALO_NATIVE_AUDIO
	, 12, 13, 15, 0, 1, 2, 3, 4, 5, 6, 7
	#endif
	};

/* ---------- public code */

void read_sound_preferences(struct sound_preferences **preferences)
{
	*preferences = &default_sound_preferences;
}

void write_sound_preferences(void)
{
}

/* ---------- private code */
