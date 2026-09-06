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

struct particle_system_type_state
{
	byte reserved00[0x20];
	real duration_lower_bound;
	real duration_upper_bound;
	byte reserved28[0x98];
};

struct old_particle_system_type
{
	byte reserved00[0x68];
	struct tag_block type_states;
	byte reserved74[0xC];
};

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
typedef char old_particle_system_type_size_assert[
	sizeof(struct old_particle_system_type) == 0x80 ? 1 : -1];
typedef char old_particle_system_type_type_states_offset_assert[
	offsetof(struct old_particle_system_type, type_states) == 0x68 ? 1 : -1];
typedef char particle_system_type_state_size_assert[
	sizeof(struct particle_system_type_state) == 0xC0 ? 1 : -1];
typedef char particle_system_type_state_duration_lower_bound_offset_assert[
	offsetof(struct particle_system_type_state, duration_lower_bound) == 0x20 ? 1 : -1];
typedef char particle_system_type_state_duration_upper_bound_offset_assert[
	offsetof(struct particle_system_type_state, duration_upper_bound) == 0x24 ? 1 : -1];

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

#endif // __PARTICLE_SYSTEM_DEFINITIONS_H
