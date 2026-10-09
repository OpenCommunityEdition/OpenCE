/*
PROFILE_CONSOLE_GAMETYPE.H

Name the current scenario and running game engine for recordings.
*/

#ifndef __PROFILE_CONSOLE_GAMETYPE_H
#define __PROFILE_CONSOLE_GAMETYPE_H

static inline const char *profile_console_scenario_map(const char *pending, const char *current)
{
	return pending && pending[0] ? pending : current;
}

/* (engine indices none, ctf, slayer, oddball, king, race; the check program
cannot include game_engine.h. The variant outlives the engine into campaign
and co-op, so it is read only while an engine runs) */
static inline const char *profile_console_engine_gametype(int running, long engine)
{
	static const char *const names[] = { "none", "ctf", "slayer", "oddball", "king", "race" };
	if (!running)
		return "campaign";
	if (engine == 0)
		return "none";
	if (engine >= 1 && engine <= 5)
		return names[engine];
	return "none";
}

#endif
