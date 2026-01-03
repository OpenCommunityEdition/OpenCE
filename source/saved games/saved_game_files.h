/*
SAVED_GAME_FILES.H

header included in hcex build.
*/

#ifndef __SAVED_GAME_FILES_H
#define __SAVED_GAME_FILES_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/SAVED_GAME_FILES.C */

void saved_game_files_take_mutex(void);
void saved_game_files_release_mutex(void);

/* ---------- globals */

/* ---------- public code */

#endif // __SAVED_GAME_FILES_H
