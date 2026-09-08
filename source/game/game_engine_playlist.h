/*
GAME_ENGINE_PLAYLIST.H
*/

#ifndef __GAME_ENGINE_PLAYLIST_H
#define __GAME_ENGINE_PLAYLIST_H
#pragma once

/* ---------- headers */

#include "game/game_engine.h"

/* ---------- prototypes/GAME_ENGINE.C */

void game_engine_playlist_next(
	long parameter0,
	long parameter1,
	long playlist_type);

struct game_variant *build_game_variant_slayer(
	struct game_variant *variant);

#endif // __GAME_ENGINE_PLAYLIST_H
