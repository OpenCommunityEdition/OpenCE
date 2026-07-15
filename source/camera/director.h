/*
DIRECTOR.H

header included in hcex build.
*/

#ifndef __DIRECTOR_H
#define __DIRECTOR_H
#pragma once

/* ---------- headers */

#include "cseries/cseries.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct director_scripting_globals
{
	boolean camera_scripted;
	byte pad[3];
};

/* ---------- prototypes/DIRECTOR.C */

void director_initialize(
	void);
void director_dispose(
	void);
void director_initialize_for_saved_game(
	void);

/* ---------- globals */

extern struct director_scripting_globals *director_camera_scripted;

/* ---------- public code */

#endif // __DIRECTOR_H
