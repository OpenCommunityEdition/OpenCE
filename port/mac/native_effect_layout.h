/* Generated from private effects.c declarations; do not edit. */
#ifndef HALO_MAC_NATIVE_EFFECT_LAYOUT_H
#define HALO_MAC_NATIVE_EFFECT_LAYOUT_H
#include "effects/effect_definitions.h"
#include "shaders/shader_definitions.h"
struct effect_particles_definition
{
	short environment;
	short disposition;
	short camera_mode;
	short unused006;
	short location_index;
	short unused00a;
	real_euler_angles2d direction;
	real_vector3d offset;
	real_vector3d runtime_direction;
	long unused02c[10];
	struct tag_reference particle;
	unsigned long flags;
	short distribution_function;
	short unused06a;
	short count_lower_bound;
	short count_upper_bound;
	real distribution_radius_lower_bound;
	real distribution_radius_upper_bound;
	long unused078[3];
	real velocity_lower_bound;
	real velocity_upper_bound;
	real velocity_cone_angle;
	real angular_velocity_lower_bound;
	real angular_velocity_upper_bound;
	long unused098[2];
	real radius_lower_bound;
	real radius_upper_bound;
	long unused0a8[2];
	real_argb_color tint_lower_bound;
	real_argb_color tint_upper_bound;
	long unused0d0[4];
	unsigned long scale_a_flags;
	unsigned long scale_b_flags;
};
struct effect_location_definition
{
	char marker_name[32];
};
struct particle_definition
{
	unsigned long flags;
	struct tag_reference bitmap;
	struct tag_reference physics;
	struct tag_reference collision_material_effects;
	long reserved34;
	real life_span_lower_bound;
	real life_span_upper_bound;
	real fade_in_time;
	real fade_out_time;
	struct tag_reference collision_effect;
	struct tag_reference effect;
	real minimum_pixels;
	long reserved6C[2];
	real radius_lower_bound;
	real radius_upper_bound;
	long reserved7C;
	real frames_per_second_lower_bound;
	real frames_per_second_upper_bound;
	real frames_per_second_contact_deterioration;
	real lod_falloff;
	real lod_cutoff;
	long reserved94;
	short first_sequence_index;
	short initial_sequence_count;
	short looping_sequence_count;
	short final_sequence_count;
	long reservedA0[2];
	real runtime_oo_width;
	short sprite_orientation;
	word padAE;
	struct shader_effect_definition shader;
};
typedef char native_particle_definition_size[sizeof(struct particle_definition)==0x164?1:-1];
typedef char native_effect_particles_size[sizeof(struct effect_particles_definition)==0xe8?1:-1];
typedef char native_effect_locations_size[sizeof(struct effect_location_definition)==32?1:-1];
#endif
