/*
PARTICLES.H

header included in hcex build.
*/

#ifndef __PARTICLES_H
#define __PARTICLES_H
#pragma once

#include "cseries/cseries.h"
#include "math/real_math.h"
#include "memory/data.h"

/* ---------- constants */

enum particle_datum_flags
{
	_particle_datum_attached_to_local_player_bit = 6
};

/* ---------- macros */

/* ---------- structures */

struct new_particle_data
{
	long definition_index;
	long object_index;
	short node_index;
	short local_player_index;
	boolean attached_to_local_player;
	boolean dont_draw_first_person;
	boolean dont_draw_third_person;
	real_point3d position;
	real_vector3d direction;
	real_vector3d velocity;
	real_vector3d initial_impulse;
	real rotation;
	real angular_velocity;
	real radius;
	real_argb_color color;
};

struct particle_datum
{
	struct datum_header header;
	word flags;
	byte unknown4[4];
	long object_index;
	byte unknownC[3];
	byte local_player_index;
	byte unknown10[0x60];
};

typedef char particle_datum_size_assert[
	sizeof(struct particle_datum) == 0x70 ? 1 : -1];
typedef char particle_datum_flags_offset_assert[
	offsetof(struct particle_datum, flags) == 0x02 ? 1 : -1];
typedef char particle_datum_object_index_offset_assert[
	offsetof(struct particle_datum, object_index) == 0x08 ? 1 : -1];
typedef char particle_datum_local_player_index_offset_assert[
	offsetof(struct particle_datum, local_player_index) == 0x0F ? 1 : -1];

#define particle_get(index) ((struct particle_datum *)datum_get(particle_data, (index)))

/* ---------- prototypes/PARTICLES.C */

void particles_initialize(
	void);
void particles_initialize_for_new_map(
	void);
void particles_dispose_from_old_map(
	void);
void particles_dispose(
	void);
void particle_delete(
	long particle_index);
void particles_stop_on_first_person_weapon(
	short local_player_index);
void particles_disconnect_from_structure_bsp(
	void);
void particle_new(struct new_particle_data const *data);

/* ---------- globals */

extern struct data_array *particle_data;

/* ---------- public code */

#endif // __PARTICLES_H
