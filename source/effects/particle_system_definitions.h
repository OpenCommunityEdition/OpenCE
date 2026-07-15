/*
PARTICLE_SYSTEM_DEFINITIONS.H

header included in hcex build.
*/

#ifndef __PARTICLE_SYSTEM_DEFINITIONS_H
#define __PARTICLE_SYSTEM_DEFINITIONS_H
#pragma once

/* ---------- headers */

#include "cseries/cseries.h"
#include "tag_files/tag_groups.h"

/* ---------- constants */

enum
{
	PARTICLE_SYSTEM_DEFINITION_TAG = 'pctl',
	PARTICLE_SYSTEM_DEFINITION_VERSION = 4,
};

/* ---------- macros */

#define particle_system_definition_get(index) ((struct particle_system_definition *)tag_get(PARTICLE_SYSTEM_DEFINITION_TAG, (index)))

/* ---------- structures */

struct particle_system_definition
{
	byte opaque00[0x44];
	long point_physics_index;
};

typedef char particle_system_definition_point_physics_index_offset_assert[
	offsetof(struct particle_system_definition, point_physics_index) == 0x44 ? 1 : -1];

/* ---------- prototypes/EXAMPLE.C */

/* ---------- globals */

/* ---------- public code */

#endif // __PARTICLE_SYSTEM_DEFINITIONS_H
