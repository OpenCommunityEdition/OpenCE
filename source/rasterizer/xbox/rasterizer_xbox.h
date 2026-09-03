/*
RASTERIZER_XBOX.H
*/

#ifndef __RASTERIZER_XBOX_H
#define __RASTERIZER_XBOX_H
#pragma once

#include "cseries.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct bitmap_data;
struct pixel_shader_definition;
struct shader;
struct vertex_buffer;

/* ---------- prototypes/RASTERIZER_XBOX.C */

void rasterizer_preinitialize__fill_you_up_with_the_devils_cock(
	void);
void rasterizer_spin_begin(
	void);
void rasterizer_spin_end(
	void);

/* ---------- prototypes/RASTERIZER_XBOX_ERRORS.C */

void rasterizer_error(
	long error_result,
	char const *format,
	...);


/* ---------- prototypes/RASTERIZER_XBOX_PROFILE.C */

void rasterizer_profile_begin(
	short profile);
void rasterizer_profile_end(
	short profile);

void rasterizer_set_stencil_mode(
	long stencil_mode);
void rasterizer_set_texture_bitmap_data(
	short stage,
	struct bitmap_data const *bitmap);
long rasterizer_set_texture_direct(
	short stage,
	long bitmap_group_index,
	short bitmap_index);
void rasterizer_set_pixel_shader(
	struct pixel_shader_definition const *definition);
void rasterizer_set_texture(
	short stage,
	short bitmap_type,
	short bitmap_usage,
	long bitmap_definition_index,
	short bitmap_sequence_index);
void rasterizer_set_vertex_shader_permutation(
	short vertex_shader_index,
	short vertex_type,
	short permutation_index);
void rasterizer_draw_dynamic_triangles_static_vertices(
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer);
void rasterizer_draw_dynamic_triangles_static_vertices2(
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer0,
	struct vertex_buffer const *vertex_buffer1);
void rasterizer_water_set_visibility_for_window(
	boolean visibility);

/* ---------- prototypes/SHADER_TRANSPARENT_GENERIC_PREPROCESSOR.C */

boolean shader_transparent_generic_create(
	struct shader *shader,
	struct pixel_shader_definition *pixel_shader);

/* ---------- globals */

extern D3DDevice *global_d3d_device;
extern struct pixel_shader_definition pixel_shader;

/* ---------- public code */

#endif // __RASTERIZER_XBOX_H
