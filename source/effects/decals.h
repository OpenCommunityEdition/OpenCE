/*
DECALS.H

header included in hcex build.
*/

#ifndef __DECALS_H
#define __DECALS_H
#pragma once

/* ---------- headers */

#include "cseries/cseries.h"
#include "math/real_math.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct collision_result;
struct decal_editor_geometry;

/* ---------- prototypes/DECALS.C */

void decals_initialize(
	void);
void decals_initialize_for_new_map(
	void);
void decals_unlock(
	boolean permanent);
void decal_delete(
	long decal_index);
long decal_get_first_decal_index(
	short cluster_index,
	short layer);
void decal_new_from_media_collision(
	long decal_definition_index,
	struct collision_result const *collision,
	real_vector3d const *velocity,
	real radius_modifier,
	boolean permanent,
	short forced_sequence_index,
	struct decal_editor_geometry *editor_geometry);
void render_debug_decals(
	void);
void decal_new_from_collision(
	long decal_definition_index,
	struct collision_result const *collision,
	real_vector3d const *velocity,
	real radius_modifier,
	boolean permanent,
	short forced_sequence_index,
	struct decal_editor_geometry *editor_geometry);
void decal_new(
	long decal_definition_index,
	real_point3d const *origin,
	real_vector3d const *velocity,
	real radius_modifier,
	boolean permanent,
	short forced_sequence_index,
	struct decal_editor_geometry *editor_geometry);
pixel32 real_a_rgb_color_to_pixel32(
	real alpha,
	real_rgb_color const *color);

/* port: decals of one quad the caller places and moves (blood's drips:
port/linux/game/blood.c) */
/* (port: while set, decal_new_quad maps only this middle share of its
picture along and across the quad, and quads are made at their
definition's fullest intensity: a drip's run, a thin slice down the middle
of a splat stretched along it, not its faint edge stretched thin) */
extern real decal_port_texture_core_along;
extern real decal_port_texture_core_across;
extern boolean decal_port_full_intensity;
long decal_new_quad(
	long decal_definition_index,
	real_point3d const *point,
	real_vector3d const *normal,
	real_vector3d const *axis,
	real half_length,
	real half_width,
	short cluster_index,
	real_point3d corners[4]);
boolean decal_move_quad(
	long decal_index,
	long decal_definition_index,
	real_point3d const corners[4],
	short cluster_index);
void decal_definition_color(
	long decal_definition_index,
	real_rgb_color *color);
real decal_definition_radius(
	long decal_definition_index);

/* ---------- globals */

extern boolean decals_enabled;
extern boolean debug_decals;

/* ---------- public code */

void decals_dispose(
	void);
void decals_dispose_from_old_map(
	void);
void decals_update(
	void);
void decals_delete_permanent_from_cluster(
	short cluster_index);
void decals_disconnect_from_structure_bsp(
	void);
void decals_reconnect_to_structure_bsp(
	void);

#endif // __DECALS_H
