/*
LOADING_TIME.C

The least time a level's loading screen stays up (display.loading_seconds,
0 by default: none). The Xbox copied a level from its disc to its hard disk
in several seconds, which the loading screen's sweep follows: it brightens,
crosses the picture and fades out, and the chant swells and falls with it.
A computer copies a level in under a second, so the sweep is cut off part of
the way across. With the setting, the loading screen stays up until that
time has gone and shows the load's progress no faster than the time goes,
so the sweep plays out at that pace.

- A level already in the cache shows no loading screen, as on the Xbox, and
  is not held.
- Only a game on this machine alone is held: in a network game the other
  machines would wait for this one.
*/

#include "cseries.h"
#include "game/game.h"
#include "loading_time.h"
#include <xtl.h>

/* port_config.c's */
double config_real(const char *name);

/* (a value above it is taken as it) */
#define LOADING_TIME_MAXIMUM_SECONDS 30.0

static struct
{
	unsigned long start;
	/* in milliseconds; 0 when this load is not held */
	unsigned long length;
} loading_time;

void loading_time_begin(
	void)
{
	double seconds= config_real("display.loading_seconds");

	loading_time.start= GetTickCount();
	loading_time.length= 0;
	if (game_connection()==_game_connection_local && seconds>0.0)
	{
		if (seconds>LOADING_TIME_MAXIMUM_SECONDS)
			seconds= LOADING_TIME_MAXIMUM_SECONDS;
		loading_time.length= (unsigned long)(seconds*1000.0);
	}

	return;
}

real loading_time_progress(
	real progress)
{
	if (loading_time.length)
	{
		real gone= (real)(GetTickCount() - loading_time.start)/(real)loading_time.length;

		if (gone<progress)
			progress= gone;
	}

	return progress;
}

boolean loading_time_holding(
	void)
{
	return loading_time.length && GetTickCount() - loading_time.start<loading_time.length;
}

void loading_time_end(
	void)
{
	loading_time.length= 0;

	return;
}
