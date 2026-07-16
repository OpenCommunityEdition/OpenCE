/*
RASTERIZER.H

header included in hcex build.
*/

#ifndef __RASTERIZER_H
#define __RASTERIZER_H
#pragma once

/* ---------- headers */

#include "real_math.h"
#include "render_cameras.h"

/* ---------- constants */

enum
{
	MAXIMUM_WINDOWS = 4,
	MAXIMUM_LENS_FLARES_PER_FRAME = 1024,
	MAXIMUM_LIGHTS_PER_WINDOW = 128,
};

enum
{
	RASTERIZER_MEMORY_POOL_SIZE = 0x18000,
	RASTERIZER_MAXIMUM_TRIANGLES_PER_TRIANGLE_BUFFER = 24576,
	RASTERIZER_MAXIMUM_DEBUG_PRIMITIVES = 131072,
	RASTERIZER_MAXIMUM_DEBUG_VERTICES = 393216,
	RASTERIZER_MAXIMUM_TRANSPARENT_GEOMETRY_GROUPS = 384,
	RASTERIZER_MAXIMUM_TRANSPARENT_GEOMETRY_GROUPS2 = 32,
	RASTERIZER_MAXIMUM_DYNAMIC_TRIANGLES = 32768,
	RASTERIZER_MAXIMUM_DYNAMIC_TRIANGLE_BUFFERS = 1024,
	RASTERIZER_MAXIMUM_DYNAMIC_UNLIT_VERTICES = 8192,
	RASTERIZER_MAXIMUM_DYNAMIC_LIT_VERTICES = 2,
	RASTERIZER_MAXIMUM_DYNAMIC_SCREEN_VERTICES = 16384,
	RASTERIZER_MAXIMUM_DYNAMIC_MODEL_VERTICES = 2048,
	RASTERIZER_MAXIMUM_DYNAMIC_MODEL_PROCESSED_VERTICES = 8192,
	RASTERIZER_MAXIMUM_DYNAMIC_VERTEX_BUFFERS = 1024,
	RASTERIZER_MAXIMUM_DETAIL_OBJECTS_PER_FRAME = 4096,
	RASTERIZER_NODES_PER_MODEL_VERTEX = 2,
	RASTERIZER_MAXIMUM_NODES_PER_MODEL = 44,
	RASTERIZER_MAXIMUM_NEARBY_OPAQUE_MODEL_GEOMETRY_GROUPS_THAT_MIGHT_OBSCURE_THE_ENVIRONMENT_FOG_SCREEN_EFFECT = 1
};

enum
{
	_rasterizer_profile_clear = 0,
	_rasterizer_profile_model_sky,
	_rasterizer_profile_models,
	_rasterizer_profile_environment_lightmaps,
	_rasterizer_profile_environment_shadows,
	_rasterizer_profile_environment_diffuse_lights,
	_rasterizer_profile_environment_decals_light,
	_rasterizer_profile_environment_decals_alpha_tested,
	_rasterizer_profile_environment_textures,
	_rasterizer_profile_environment_decals_primary,
	_rasterizer_profile_environment_decals_secondary,
	_rasterizer_profile_environment_specular_lights,
	_rasterizer_profile_environment_specular_lightmaps,
	_rasterizer_profile_environment_reflection_lightmap_masks,
	_rasterizer_profile_environment_reflection_mirrors,
	_rasterizer_profile_environment_reflections,
	_rasterizer_profile_environment_transparents,
	_rasterizer_profile_environment_fog,
	_rasterizer_profile_environment_fog_screen,
	_rasterizer_profile_water,
	_rasterizer_profile_environment_decals_water,
	_rasterizer_profile_detail_objects,
	_rasterizer_profile_queued_transparents,
	_rasterizer_profile_lens_flare_occlusion_submit,
	_rasterizer_profile_lens_flare_occlusion_query,
	_rasterizer_profile_lens_flares,
	_rasterizer_profile_screen_effect,
	_rasterizer_profile_hud,
	_rasterizer_profile_screen_flash,
	NUMBER_OF_RASTERIZER_PROFILES,
};


/* ---------- macros */

/* ---------- structures */

struct rasterizer_model_begin_parameters;

struct rasterizer_frame_begin_parameters
{
	real game_time_sec;
	real dt;
};

struct rasterizer_window_begin_parameters
{
	short rasterizer_target;
	short window_index;
	boolean has_mirror;
	boolean suppress_clear;
	struct render_camera camera;
	struct render_frustum frustum;
	struct render_fog fog;
	struct render_screen_flash screen_flash;
	struct render_screen_effect screen_effect;
};

/* ---------- prototypes/RASTERIZER.C */

void rasterizer_reset_state(
	void);
void rasterizer_widget_end(
	void);

boolean rasterizer_initialize(void);

void rasterizer_frame_begin(const struct rasterizer_frame_begin_parameters *parameters);
boolean rasterizer_windows_begin(void);
void rasterizer_window_begin(
	struct rasterizer_window_begin_parameters const *parameters);
void rasterizer_window_end(
	void);
void rasterizer_windows_end(void);
void rasterizer_frame_end(void);

void rasterizer_present(struct bitmap_data *screenshot_bitmap, const point2d *screenshot_index);
void rasterizer_dispose(void);
void rasterizer_window_get_fog(
	struct render_fog *fog);
void rasterizer_window_set_fog(
	struct render_fog const *fog);
void rasterizer_set_vblank_callback(
	void (*callback)(unsigned long));
long rasterizer_dynamic_triangles_new(
	long triangle_count);
void rasterizer_dynamic_triangles_delete(
	long triangle_buffer_index);
void rasterizer_dynamic_vertices_delete(
	long dynamic_vertex_buffer_index);
void rasterizer_debug_immediate_line(
	real_point3d const *p0,
	real_point3d const *p1,
	real_rgb_color const *color0,
	real_rgb_color const *color1);
void rasterizer_debug_immediate_triangle(
	real_point3d const *p0,
	real_point3d const *p1,
	real_point3d const *p2,
	real_rgb_color const *color0,
	real_rgb_color const *color1,
	real_rgb_color const *color2);
void rasterizer_debug_immediate_line_screenspace(
	point2d const *p0,
	point2d const *p1,
	real_rgb_color const *color0,
	real_rgb_color const *color1);
void rasterizer_debug_immediate_linestrip_screenspace(
	point2d const *points,
	short point_count,
	real_rgb_color const *color);
void *rasterizer_decal_vertices_lock(
	short cache_index,
	unsigned long cache_size);
void rasterizer_decal_vertices_unlock(
	void);

void rasterizer_decals_initialize(
	void);
void rasterizer_decals_initialize_for_new_map(
	void);
void rasterizer_decals_dispose_from_old_map(
	void);
void rasterizer_decals_dispose(
	void);
void rasterizer_decals_begin(
	short type);
void rasterizer_decals_end(
	void);
void rasterizer_decals_update_function_pointers(void);
void rasterizer_detail_objects_begin(
	void);
void rasterizer_detail_objects_end(
	void);

void rasterizer_debug_immediate_begin(
	void);
void rasterizer_debug_immediate_end(
	void);
void rasterizer_debug_immediate_begin_screenspace(
	void);
void rasterizer_debug_immediate_end_screenspace(
	void);

void rasterizer_hud_begin(
	void);
void rasterizer_hud_end(
	void);
void rasterizer_hud_motion_sensor_blip_begin(
	void);
void rasterizer_hud_motion_sensor_blip_end(
	real_point2d const *center,
	real scale);
void rasterizer_model_begin(
	struct rasterizer_model_begin_parameters const *parameters,
	boolean is_dynamic);
void rasterizer_model_end(
	void);
void rasterizer_models_end(
	void);
void rasterizer_environment_lightmap_end(
	void);
void rasterizer_environment_lightmaps_end(
	void);
void rasterizer_environment_diffuse_light_end(
	void);
void rasterizer_environment_diffuse_lights_end(
	void);
void rasterizer_environment_shadows_begin(
	void);
void rasterizer_environment_shadow_model_end(
	void);
void rasterizer_environment_shadow_end(
	void);
void rasterizer_environment_shadows_end(
	void);
void rasterizer_environment_diffuse_textures_end(
	void);
void rasterizer_environment_specular_light_end(
	void);
void rasterizer_environment_specular_lights_end(
	void);
void rasterizer_environment_specular_lightmap_end(
	void);
void rasterizer_environment_specular_lightmaps_end(
	void);
void rasterizer_environment_reflection_lightmap_mask_end(
	void);
void rasterizer_environment_reflection_lightmap_masks_end(
	void);
void rasterizer_environment_reflection_mirrors_begin(
	void);
void rasterizer_environment_reflection_mirrors_end(
	void);
void rasterizer_environment_reflections_begin(
	void);
void rasterizer_environment_reflections_end(
	void);
void rasterizer_environment_transparent_geometry_begin(
	void);
void rasterizer_environment_transparent_geometry_end(
	void);

/* ---------- prototypes/RASTERIZER_TEXT.C */

void rasterizer_draw_string(union rectangle2d const *bounds, union rectangle2d const *clip, union point2d *cursor_reference, short height_adjust, char const *string);

/* ---------- prototypes/RASTERIZER_MEMORY_POOL.C */

boolean rasterizer_memory_pool_initialize(void);
void rasterizer_memory_pool_begin(void);
void *rasterizer_memory_alloc(const void *src, unsigned long size);
const void *rasterizer_memory_alloc_const(const void *src, unsigned long size);
void rasterizer_memory_pool_end(void);
void rasterizer_memory_pool_dispose(void);

/* ---------- prototypes/RASTERIZER_LIGHTS.C */

void rasterizer_lights_reset_for_new_map(void);
void rasterizer_lights_begin(
	void);
void rasterizer_lights_end(
	void);

/* ---------- prototypes/RASTERIZER_FRAME_STATISTICS.C */

void rasterizer_frame_statistics_begin(
	void);
void rasterizer_frame_statistics_end(
	void);

/* ---------- prototypes/RASTERIZER_TRANSPARENT_GEOMETRY.C */

void rasterizer_transparent_geometry_begin(
	void);
void rasterizer_transparent_geometry_end(
	void);

/* ---------- prototypes/RASTERIZER_DEBUG.C */

void rasterizer_debug_begin(
	void);
void rasterizer_debug_end(
	void);

/* ---------- prototypes/RASTERIZER_TEXT.C */

void rasterizer_text_cache_flush(void);

/* ---------- globals */

extern real_argb_color *global_rasterizer_model_ambient_reflection_tint;

/* comm. not sure where this should be */
struct rasterizer_frame_begin_parameters global_frame_parameters;

/* ---------- public code */

#endif // __RASTERIZER_H
