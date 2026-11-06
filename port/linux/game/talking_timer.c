/*
TALKING_TIMER.C

The callouts are Halo 1: Final's Neutral Host Edition's, without its beep,
at the time the match clock shows (hud.c).
*/

/* ---------- headers */

#include "cseries.h"
#include "game/game_engine.h"
#include "game/game.h"
#include "talking_timer.h"
#include "../src/talking_timer_audio.h"

/* ---------- constants */

#define TALKING_TIMER_MAXIMUM_GAIN 1.25f

int config_boolean(const char *name);
double config_real(const char *name);
unsigned long config_changes(void);

/* ---------- private code */

static long talking_timer_line_at(
	long second)
{
	long line = NONE;
	long within = second % 60;

	if (second <= 0)
		return NONE;
	if (within == 0)
	{
		if (second / 60 <= 30)
			line = TALKING_TIMER_MINUTE_1 + second / 60 - 1;
	}
	else if (within == 30)
	{
		line = TALKING_TIMER_30_SECONDS;
	}
	else if (within == 40)
	{
		line = TALKING_TIMER_20_SECONDS;
	}
	else if (within >= 50)
	{
		line = TALKING_TIMER_COUNTDOWN_10 + within - 50;
	}

	return line;
}

/* ---------- public code */

void talking_timer_update(
	void)
{
	static unsigned long read_at = (unsigned long)-1;
	static boolean setting = FALSE;
	static float volume = 1.0f;
	/* (a game's first second seen is only noted, so no callout is late) */
	static long watching = NONE;
	long second;
	long line;

	if (read_at != config_changes())
	{
		read_at = config_changes();
		setting = config_boolean("audio.talking_timer") != 0;
		volume = (float)(config_real("audio.volume") * PIN(config_real("audio.timer_volume"), 0.0, 1.0))
			* TALKING_TIMER_MAXIMUM_GAIN;
	}
	if (!setting || !game_engine_running() || !game_engine_in_play())
	{
		watching = NONE;
		return;
	}
	second = game_time_get() / TICKS_PER_SECOND;
	if (watching == NONE)
	{
		watching = second;
		return;
	}
	if (second != watching)
	{
		watching = second;
		line = talking_timer_line_at(second);
		if (line != NONE && volume > 0.0f)
			talking_timer_audio_play(line, volume);
	}

	return;
}
