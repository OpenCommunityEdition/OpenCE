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
	{ 10, 51, 10, 10 },
	{ 9, 46, 9, 9 },
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
