/*
MATCHMAKING.H

The game side of a MATCHMAKING search (matchmaking.c).
*/

#ifndef __MATCHMAKING_H
#define __MATCHMAKING_H

#include "cseries.h"

void matchmaking_update(float seconds);
boolean matchmaking_game_locked(void);

#endif
