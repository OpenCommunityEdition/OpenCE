/*
GAME_ENGINE_RACE.H
*/

#ifndef __GAME_ENGINE_RACE_H
#define __GAME_ENGINE_RACE_H
#pragma once

/* ---------- headers */

#include "game/game_engine.h"

/* ---------- structures */

struct race_globals;

/* ---------- prototypes/GAME_ENGINE_RACE.C */

void race_flags_make_unique(
	void);

/* ---------- globals */

extern struct game_engine race_engine;

#endif // __GAME_ENGINE_RACE_H
