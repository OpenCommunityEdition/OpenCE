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
#include "objects/objects.h"
#include "tag_files/tag_groups.h"

/* ---------- constants */

enum particle_datum_flags
{
	_particle_datum_attached_to_local_player_bit = 6
};

enum
{
	PARTICLE_TAG = 'part'
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
	long definition_index;
	long object_index;
	short node_index;
	byte unknownE;
	byte local_player_index;
	byte unknown10[4];
	real age;
	real lifespan;
	byte unknown1C[0x0C];
	struct location location;
	real_point3d position;
	byte unknown3C[0x20];
	real radius;
	byte unknown60[0x10];
};

struct particle_definition
{
	byte unknown0[0x58];
	struct tag_reference effect;
	byte unknown68[0x0C];
	real radius_lower_bound;
	real radius_upper_bound;
};

typedef char particle_datum_size_assert[
	sizeof(struct particle_datum) == 0x70 ? 1 : -1];
typedef char particle_datum_flags_offset_assert[
	offsetof(struct particle_datum, flags) == 0x02 ? 1 : -1];
typedef char particle_datum_definition_index_offset_assert[
	offsetof(struct particle_datum, definition_index) == 0x04 ? 1 : -1];
typedef char particle_datum_object_index_offset_assert[
	offsetof(struct particle_datum, object_index) == 0x08 ? 1 : -1];
typedef char particle_datum_node_index_offset_assert[
	offsetof(struct particle_datum, node_index) == 0x0C ? 1 : -1];
typedef char particle_datum_local_player_index_offset_assert[
	offsetof(struct particle_datum, local_player_index) == 0x0F ? 1 : -1];
typedef char particle_datum_age_offset_assert[
	offsetof(struct particle_datum, age) == 0x14 ? 1 : -1];
typedef char particle_datum_lifespan_offset_assert[
	offsetof(struct particle_datum, lifespan) == 0x18 ? 1 : -1];
typedef char particle_datum_location_offset_assert[
	offsetof(struct particle_datum, location) == 0x28 ? 1 : -1];
typedef char particle_datum_position_offset_assert[
	offsetof(struct particle_datum, position) == 0x30 ? 1 : -1];
typedef char particle_datum_radius_offset_assert[
	offsetof(struct particle_datum, radius) == 0x5C ? 1 : -1];
typedef char particle_definition_radius_lower_bound_offset_assert[
	offsetof(struct particle_definition, radius_lower_bound) == 0x74 ? 1 : -1];
typedef char particle_definition_radius_upper_bound_offset_assert[
	offsetof(struct particle_definition, radius_upper_bound) == 0x78 ? 1 : -1];
typedef char particle_definition_effect_offset_assert[
	offsetof(struct particle_definition, effect) == 0x58 ? 1 : -1];

#define particle_get(index) ((struct particle_datum *)datum_get(particle_data, (index)))
#define particle_definition_get(index) ((struct particle_definition *)tag_get(PARTICLE_TAG, (index)))

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
real particle_get_radius(
	long particle_index);
boolean valid_real_argb_color(
	real_argb_color const *color);
void particles_disconnect_from_structure_bsp(
	void);
void particles_reconnect_to_structure_bsp(
	void);
void particle_new(struct new_particle_data const *data);

/* ---------- globals */

extern struct data_array *particle_data;

/* ---------- public code */

#endif // __PARTICLES_H
