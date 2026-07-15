/*
PARTICLE_SYSTEMS.H

header included in hcex build.
*/

#ifndef __PARTICLE_SYSTEMS_H
#define __PARTICLE_SYSTEMS_H
#pragma once

#include "cseries/cseries.h"
#include "memory/data.h"

/* ---------- constants */

enum
{
	_particle_system_attached_bit,
};

/* ---------- macros */

#define particle_system_get(system_index) ((struct particle_system_datum *)datum_get(particle_systems, (system_index)))

/* ---------- structures */

struct particle_system_datum
{
	struct datum_header header;
	short pad;
	unsigned long flags;
	long definition_index;
	long object_index;
	short attachment_index;
	short type_state_index;
	byte opaque[0x144];
};

typedef char particle_system_datum_size_assert[
	sizeof(struct particle_system_datum) == 0x158 ? 1 : -1];

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
long particle_system_new_attached(long definition_index, long object_index, short attachment_index);

void particle_system_orphan(
	long system_index);

/* ---------- globals */

extern struct data_array *particle_systems;
extern struct data_array *system_particles;

/* ---------- public code */

#endif // __PARTICLE_SYSTEMS_H
