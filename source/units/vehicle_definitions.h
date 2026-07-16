/*
VEHICLE_DEFINITIONS.H

header included in hcex build.
*/

#ifndef __VEHICLE_DEFINITIONS_H
#define __VEHICLE_DEFINITIONS_H
#pragma once

/* ---------- headers */

#include "unit_definitions.h"

/* ---------- constants */

enum
{
	VEHICLE_DEFINITION_TAG = 'vehi'
};

/* ---------- macros */

#define vehicle_definition_get(index) ((struct unit_definition *)tag_get(VEHICLE_DEFINITION_TAG, index))

/* ---------- structures */

/* ---------- prototypes/EXAMPLE.C */

/* ---------- globals */

/* ---------- public code */

#endif // __VEHICLE_DEFINITIONS_H
