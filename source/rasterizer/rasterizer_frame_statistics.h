/*
RASTERIZER_FRAME_STATISTICS.H

Narrow cross-translation-unit interface owned by RASTERIZER_FRAME_STATISTICS.C.
*/

#ifndef __RASTERIZER_FRAME_STATISTICS_H
#define __RASTERIZER_FRAME_STATISTICS_H
#pragma once

#include "cseries.h"

/* January's 0x170-byte linker-common record. Only counters already used by
 * reconstructed writers are named here; unreviewed interiors stay reserved. */
struct rasterizer_frame_statistics_globals
{
	byte reserved000[0x30];
	unsigned long shadow_count;
	unsigned long shadow_vertex_count;
	unsigned long shadow_triangle_count;
	unsigned long shadow_draw_count;
	unsigned long environment_dynamic_vertex_count;
	unsigned long environment_dynamic_triangle_count;
	unsigned long environment_dynamic_draw_count;
	unsigned long decal_vertex_count;
	unsigned long decal_triangle_count;
	unsigned long decal_draw_count;
	unsigned long decal_shader_change_count;
	unsigned long decal_texture_change_count;
	unsigned long diffuse_texture_dynamic_vertex_count;
	unsigned long diffuse_texture_dynamic_triangle_count;
	unsigned long diffuse_texture_dynamic_draw_count;
	unsigned long specular_light_dynamic_vertex_count;
	unsigned long specular_light_dynamic_triangle_count;
	unsigned long specular_light_dynamic_draw_count;
	unsigned long specular_lightmap_dynamic_vertex_count;
	unsigned long specular_lightmap_dynamic_triangle_count;
	unsigned long specular_lightmap_dynamic_draw_count;
	unsigned long reflection_mask_dynamic_vertex_count;
	unsigned long reflection_mask_dynamic_triangle_count;
	unsigned long reflection_mask_dynamic_draw_count;
	unsigned long reflection_dynamic_vertex_count;
	unsigned long reflection_dynamic_triangle_count;
	unsigned long reflection_dynamic_draw_count;
	unsigned long transparent_geometry_dynamic_vertex_count;
	unsigned long transparent_geometry_dynamic_triangle_count;
	long transparent_geometry_largest_dynamic_triangle_count;
	unsigned long transparent_geometry_dynamic_draw_count;
	unsigned long environment_fog_dynamic_vertex_count;
	unsigned long environment_fog_dynamic_triangle_count;
	unsigned long environment_fog_dynamic_draw_count;
	byte reserved0B8[0x1C];
	unsigned long model_count;
	byte reserved0D8[0x1C];
	unsigned long model_shadow_count;
	unsigned long model_shadow_vertex_count;
	unsigned long model_shadow_triangle_count;
	unsigned long model_shadow_draw_count;
	unsigned long dynamic_unlit_draw_count;
	unsigned long dynamic_unlit_triangle_count;
	long largest_dynamic_unlit_triangle_count;
	unsigned long dynamic_unlit_vertex_count;
	byte reserved114[0x1C];
	long dynamic_vertex_count;
	long dynamic_vertex_buffer_count;
	long dynamic_triangle_count;
	long dynamic_triangle_buffer_count;
	long debug_primitive_count;
	byte reserved144[4];
	long dynamic_light_count;
	long lens_flare_count;
	long vertex_shader_skinning_constant_bytes;
	long vertex_shader_lighting_constant_bytes;
	long vertex_shader_instruction_count;
	long pixel_shader_pushbuffer_bytes;
	unsigned long model_skinning_constant_bytes;
	unsigned long model_lighting_constant_bytes;
	byte reserved168[8];
};

extern struct rasterizer_frame_statistics_globals rasterizer_frame_statistics;

struct triangle_buffer;
struct vertex_buffer;

void rasterizer_frame_statistics_get_fps(
	struct rasterizer_frame_statistics_globals *frame_statistics);
void rasterizer_frame_statistics_draw(
	void);

long rasterizer_frame_statistics_count_static_vertices(
	struct triangle_buffer const *triangle_buffer,
	struct vertex_buffer const *vertex_buffer);
long rasterizer_frame_statistics_count_dynamic_vertices(
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count);

#endif /* __RASTERIZER_FRAME_STATISTICS_H */
