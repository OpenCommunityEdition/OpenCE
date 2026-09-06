/*
PARTICLE_SYSTEMS.H

header included in hcex build.
*/

#ifndef __PARTICLE_SYSTEMS_H
#define __PARTICLE_SYSTEMS_H
#pragma once

#include "cseries/cseries.h"
#include "math/real_math.h"
#include "memory/data.h"
#include "objects/objects.h"

/* ---------- constants */

enum
{
	_particle_system_active_bit,
};

enum
{
	MAXIMUM_PARTICLE_SYSTEM_TYPES_PER_SYSTEM = 4,
};

/* ---------- macros */

#define particle_system_get(system_index) ((struct particle_system_datum *)datum_get(particle_systems, (system_index)))
#define ps_particle_get(particle_index) ((struct ps_particle_datum *)datum_get(system_particles, (particle_index)))

/* ---------- structures */

struct particle_type
{
	byte reserved00[0x3C];
	long first_particle_index;
};

struct particle_system_datum
{
	struct datum_header header;
	short pad;
	unsigned long flags;
	long definition_index;
	long object_index;
	short attachment_index;
	short type_state_index;
	real function_value;
	struct location location;
	real_point3d position;
	real_vector3d velocity;
	byte reserved38[0x20];
	struct particle_type types[MAXIMUM_PARTICLE_SYSTEM_TYPES_PER_SYSTEM];
};

struct ps_particle_datum
{
	struct datum_header header;
	byte reserved02[0x2];
	long next_particle_index;
	byte reserved08[0xC];
	struct location location;
	real_point3d position;
	real_vector3d velocity;
	byte reserved34[0x4C];
};

typedef char particle_type_size_assert[
	sizeof(struct particle_type) == 0x40 ? 1 : -1];
typedef char particle_type_first_particle_index_offset_assert[
	offsetof(struct particle_type, first_particle_index) == 0x3C ? 1 : -1];
typedef char particle_system_datum_size_assert[
	sizeof(struct particle_system_datum) == 0x158 ? 1 : -1];
typedef char particle_system_datum_flags_offset_assert[
	offsetof(struct particle_system_datum, flags) == 0x4 ? 1 : -1];
typedef char particle_system_datum_definition_index_offset_assert[
	offsetof(struct particle_system_datum, definition_index) == 0x8 ? 1 : -1];
typedef char particle_system_datum_object_index_offset_assert[
	offsetof(struct particle_system_datum, object_index) == 0xC ? 1 : -1];
typedef char particle_system_datum_position_offset_assert[
	offsetof(struct particle_system_datum, position) == 0x20 ? 1 : -1];
typedef char particle_system_datum_location_offset_assert[
	offsetof(struct particle_system_datum, location) == 0x18 ? 1 : -1];
typedef char particle_system_datum_velocity_offset_assert[
	offsetof(struct particle_system_datum, velocity) == 0x2C ? 1 : -1];
typedef char particle_system_datum_types_offset_assert[
	offsetof(struct particle_system_datum, types) == 0x58 ? 1 : -1];
typedef char ps_particle_datum_size_assert[
	sizeof(struct ps_particle_datum) == 0x80 ? 1 : -1];
typedef char ps_particle_datum_next_particle_index_offset_assert[
	offsetof(struct ps_particle_datum, next_particle_index) == 0x4 ? 1 : -1];
typedef char ps_particle_datum_location_offset_assert[
	offsetof(struct ps_particle_datum, location) == 0x14 ? 1 : -1];
typedef char ps_particle_datum_position_offset_assert[
	offsetof(struct ps_particle_datum, position) == 0x1C ? 1 : -1];
typedef char ps_particle_datum_velocity_offset_assert[
	offsetof(struct ps_particle_datum, velocity) == 0x28 ? 1 : -1];

/* ---------- prototypes/PARTICLE_SYSTEMS.C */

void particle_systems_initialize(
	void);
void particle_systems_initialize_for_new_map(
	void);
void particle_systems_dispose_from_old_map(
	void);
void particle_systems_dispose(
	void);
void particle_systems_disconnect_from_structure_bsp(
	void);
void particle_systems_reconnect_to_structure_bsp(
	void);
void particle_systems_update(
	real delta_time);
long particle_system_new_attached(
	long definition_index,
	long object_index,
	short attachment_index);
long particle_system_new_unattached(
	long definition_index,
	real_point3d const *position,
	real_vector3d const *velocity,
	real_argb_color const *color,
	real scale);

void particle_system_orphan(
	long system_index);

/* ---------- globals */

extern struct data_array *particle_systems;
extern struct data_array *system_particles;

/* ---------- public code */

#endif // __PARTICLE_SYSTEMS_H
