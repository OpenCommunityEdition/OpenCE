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
	byte reserved00[0x38];
	struct tag_reference system_update_point_physics;
	byte reserved48[0x14];
	struct tag_block types;
};

typedef char particle_system_definition_size_assert[
	sizeof(struct particle_system_definition) == 0x68 ? 1 : -1];
typedef char particle_system_definition_system_update_point_physics_offset_assert[
	offsetof(struct particle_system_definition, system_update_point_physics) == 0x38 ? 1 : -1];
typedef char particle_system_definition_system_update_point_physics_index_offset_assert[
	offsetof(struct particle_system_definition, system_update_point_physics.index) == 0x44 ? 1 : -1];
typedef char particle_system_definition_types_offset_assert[
	offsetof(struct particle_system_definition, types) == 0x5C ? 1 : -1];

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

#endif // __PARTICLE_SYSTEM_DEFINITIONS_H
