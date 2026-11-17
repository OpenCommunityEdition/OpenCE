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

/* port: the port's channels of each type, past the Xbox's hardware voices
(halo_port_capacity.h: the platform budgets).
A sound mod may make any class stereo (the Uncompressed Audio mod's weapons,
reloads and engines are stereo 44 kHz), which then shared the music's 9
channels and took them (#1125). */
static struct sound_preferences default_sound_preferences =
{
	0,
	HALO_PORT_SOUND_ACTUAL_CHANNELS,
	HALO_PORT_SOUND_VIRTUAL_CHANNELS,
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
