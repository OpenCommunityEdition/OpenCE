/*
DIALOGUE_DEFINITIONS.H

header included in hcex build.
*/

#ifndef __DIALOGUE_DEFINITIONS_H
#define __DIALOGUE_DEFINITIONS_H
#pragma once

/* ---------- headers */

#include "cseries/cseries.h"

/* ---------- constants */

enum
{
	NUMBER_OF_DIALOGUE_VOCALIZATION_TYPES = 209,
};

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/DIALOGUE_DEFINITIONS.C */

char const *dialogue_get_vocalization_name(short vocalization_type, boolean abbreviated);
short dialogue_get_vocalization_type_by_name(char const *name);

/* ---------- globals */

/* ---------- public code */

#endif // __DIALOGUE_DEFINITIONS_H
