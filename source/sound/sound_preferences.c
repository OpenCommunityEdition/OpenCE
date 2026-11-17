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

/* port: the stereo types (22 and 44 kHz) have as many channels as the mono
3D type, not the Xbox's 9 voices. The port mixes in software, with no
hardware voice limit, and a sound mod may make any class stereo (the
Uncompressed Audio mod's weapons, reloads and engines are stereo 44 kHz),
which then shared the music's 9 channels and took them (#1125). A channel no
sound plays in costs nothing to mix. */
static struct sound_preferences default_sound_preferences =
{
	0,
	{ 10, 51, 51, 51 },
	{ 9, 46, 46, 46 },
	0,
};

short sound_channel_type_flags[4] = { 8, 9, 10, 14 };

/* ---------- public code */

void read_sound_preferences(struct sound_preferences **preferences)
{
	*preferences = &default_sound_preferences;
}

void write_sound_preferences(void)
{
}

/* ---------- private code */
