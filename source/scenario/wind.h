/*
WIND.H
*/

#ifndef __WIND_H
#define __WIND_H
#pragma once

/* ---------- headers */

#include "cseries/cseries.h"

/* ---------- structures */

struct wind_globals
{
	boolean initialized;
};

/* ---------- prototypes/WIND.C */

void wind_dispose_from_old_map(
	void);

/* ---------- globals */

extern struct wind_globals wind_globals;

#endif // __WIND_H
