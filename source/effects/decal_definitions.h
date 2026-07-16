/*
DECAL_DEFINITIONS.H

header included in hcex build.
*/

#ifndef __DECAL_DEFINITIONS_H
#define __DECAL_DEFINITIONS_H
#pragma once

/* ---------- headers */

#include "tag_files/tag_groups.h"

/* ---------- constants */

enum
{
	DECAL_GROUP_TAG = 'deca',
};

/* ---------- macros */

#define decal_definition_get(index) ((struct decal_definition *)tag_get(DECAL_GROUP_TAG, (index)))

/* ---------- structures */

struct decal_definition;

/* ---------- prototypes/EXAMPLE.C */

/* ---------- globals */

/* ---------- public code */

#endif // __DECAL_DEFINITIONS_H
