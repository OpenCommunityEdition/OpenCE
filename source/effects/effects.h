/*
EFFECTS.H

header included in hcex build.
*/

#ifndef __EFFECTS_H
#define __EFFECTS_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/EFFECTS.C */

void effects_disconnect_from_structure_bsp(
	void);
void effects_reconnect_to_structure_bsp(
	void);
boolean dangerous_effects_near_player(
	void);
long effect_new_looping(
	long definition_index,
	long object_index,
	short scale_a_function_index,
	short scale_b_function_index,
	short change_color_index);

long effect_new_from_object(
	long definition_index,
	long owner_object_index,
	long object_index,
	short force_local_player_index,
	real scale_a,
	real scale_b,
	real_rgb_color const *color,
	struct effect_vector_field const *impulse_field);

long effect_new_unattached_from_markers(
	long definition_index,
	long argument_1,
	real_vector3d const *translational_velocity,
	short marker_count,
	long argument_4,
	real_point3d const *marker_points,
	real_vector3d const *marker_forwards,
	real scale_a,
	real scale_b,
	real_rgb_color const *color,
	struct effect_vector_field const *impulse_field,
	long argument_11);

void effect_delete(long effect_index);

/* ---------- globals */

/* ---------- public code */

#endif // __EFFECTS_H
