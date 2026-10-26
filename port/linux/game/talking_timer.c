/*
TALKING_TIMER.C

The callouts are Halo 1: Final's Neutral Host Edition's, without its beep.
*/

/* ---------- headers */

#include "cseries.h"
#include "../src/talking_timer_audio.h"

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
