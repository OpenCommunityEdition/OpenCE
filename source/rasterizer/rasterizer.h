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
struct detail_object_global_runtime_data;
struct detail_object_render_data;
struct bitmap_data;
struct shader;
struct vertex_buffer;
struct rasterizer_dynamic_screen_geometry_parameters;

struct rasterizer_frame_begin_parameters
{
	real game_time_sec;
	real dt;
};

struct rasterizer_globals_definition
{
	boolean initialized;
	byte reserved01[0x27];
	/* updated asynchronously by the rasterizer and vertical-blank callback */
	volatile unsigned long frame_index;
	volatile unsigned long vertical_blank_index;
	byte reserved30[0xD];
	boolean framerate_throttle;
	byte reserved3E[6];
	real near_clip_distance;
	real far_clip_distance;
	real first_person_weapon_near_clip_distance;
	real first_person_weapon_far_clip_distance;
	byte reserved54[0x14];
};

typedef char verify_rasterizer_globals_size[
	sizeof(struct rasterizer_globals_definition) == 0x68 ? 1 : -1];
typedef char verify_rasterizer_globals_initialized_offset[
	offsetof(struct rasterizer_globals_definition, initialized) == 0x00 ? 1 : -1];
typedef char verify_rasterizer_globals_framerate_throttle_offset[
	offsetof(struct rasterizer_globals_definition, framerate_throttle) == 0x3D ? 1 : -1];
typedef char verify_rasterizer_globals_near_clip_distance_offset[
	offsetof(struct rasterizer_globals_definition, near_clip_distance) == 0x44 ? 1 : -1];

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
short *rasterizer_dynamic_triangles_lock(
	long triangle_buffer_index);
void rasterizer_dynamic_triangles_unlock(
	long triangle_buffer_index);
void rasterizer_dynamic_triangles_delete(
	long triangle_buffer_index);
long rasterizer_dynamic_vertices_new(
	short type,
	long vertex_count);
short rasterizer_dynamic_vertices_get_type(
	long dynamic_vertex_buffer_index);
void *rasterizer_dynamic_vertices_lock(
	long dynamic_vertex_buffer_index);
void rasterizer_dynamic_vertices_unlock(
	long dynamic_vertex_buffer_index);
void rasterizer_dynamic_vertices_delete(
	long dynamic_vertex_buffer_index);
void rasterizer_debug_immediate_line(
	real_point3d const *p0,
	real_point3d const *p1,
	real_rgb_color const *color0,
	real_rgb_color const *color1);
void rasterizer_debug_immediate_point(
	real_point3d const *point,
	real radius,
	real_rgb_color const *color);
void rasterizer_debug_immediate_vector(
	real_point3d const *point,
	real_vector3d const *vector,
	real scale,
	real_rgb_color const *color);
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
long rasterizer_decal_vertices_new(
	long size);
void rasterizer_decal_vertices_delete(
	long decal_vertex_buffer_index);

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
void rasterizer_decals_draw(
	short cluster_index);
void rasterizer_decals_update_function_pointers(void);
void rasterizer_detail_objects_begin(
	void);
void rasterizer_detail_objects_rebuild_vertices(
	struct detail_object_global_runtime_data *runtime_data);
void rasterizer_detail_objects_draw(
	struct detail_object_render_data *render_data);
void rasterizer_detail_objects_end(
	void);
void rasterizer_screen_effect(
	struct render_screen_effect const *effect);

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
void rasterizer_hud_motion_sensor_blip_draw(
	real_point2d const *position,
	real intensity,
	real size,
	real_rgb_color const *color,
	boolean large_blip);
void rasterizer_hud_motion_sensor_blip_end(
	real_point2d const *center,
	real scale);
void rasterizer_model_begin(
	struct rasterizer_model_begin_parameters const *parameters,
	boolean is_dynamic);
void rasterizer_model_draw(
	struct shader const *shader,
	short bitmap_index,
	void const *geometry,
	long geometry_index,
	long model_data,
	real_rgb_color const *change_colors,
	long model_effect);
void rasterizer_model_transparent_geometry_submit(
	struct shader const *shader,
	short bitmap_index,
	void const *geometry,
	long geometry_index,
	long model_data,
	real_rgb_color const *change_colors,
	long model_effect,
	void const *lighting,
	void const *effect);
void rasterizer_model_end(
	void);
void rasterizer_models_end(
	void);
void rasterizer_models_begin(
	boolean skip_obscurer_test);
void rasterizer_environment_lightmap_begin(
	struct bitmap_data const *lightmap_bitmap);
void rasterizer_environment_lightmap_draw(
	struct shader const *shader,
	short bitmap_index,
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer);
void rasterizer_environment_lightmap_end(
	void);
void rasterizer_environment_lightmaps_end(
	void);
void rasterizer_environment_diffuse_light_end(
	void);
void rasterizer_environment_diffuse_lights_end(
	void);
void rasterizer_environment_diffuse_light_begin(
	long light_index);
void rasterizer_environment_diffuse_light_draw(
	struct shader const *shader,
	short bitmap_index,
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer);
void rasterizer_environment_shadows_begin(
	void);
void rasterizer_environment_shadow_begin(
	long object_index,
	real_matrix4x3 const *shadow_matrix,
	real_point3d const *object_position,
	real object_bounding_radius,
	real_rgb_color const *shadow_color);
void rasterizer_environment_shadow_model_begin(
	struct rasterizer_model_begin_parameters const *parameters);
void rasterizer_environment_shadow_model_draw(
	struct shader const *shader,
	short bitmap_index,
	void const *geometry,
	real_rgb_color const *change_colors);
void rasterizer_environment_shadow_model_end(
	void);
void rasterizer_environment_shadow_end(
	void);
void rasterizer_environment_shadow_draw(
	struct shader const *shader,
	short bitmap_index,
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer);
void rasterizer_environment_shadows_end(
	void);
void rasterizer_environment_diffuse_textures_end(
	void);
void rasterizer_environment_diffuse_texture_draw(
	struct shader const *shader,
	short bitmap_index,
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer);
void rasterizer_environment_specular_light_begin(
	long light_index);
void rasterizer_environment_specular_light_draw(
	struct shader const *shader,
	short bitmap_index,
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer);
void rasterizer_environment_specular_light_end(
	void);
void rasterizer_environment_specular_lights_end(
	void);
void rasterizer_environment_specular_lightmap_end(
	void);
void rasterizer_environment_specular_lightmaps_end(
	void);
void rasterizer_environment_specular_lightmap_begin(
	struct bitmap_data const *lightmap_bitmap);
void rasterizer_environment_specular_lightmap_draw(
	struct shader const *shader,
	short bitmap_index,
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer);
void rasterizer_environment_reflection_lightmap_mask_end(
	void);
void rasterizer_environment_reflection_lightmap_mask_begin(
	struct bitmap_data const *lightmap_bitmap);
void rasterizer_environment_reflection_lightmap_mask_draw(
	struct shader const *shader,
	short bitmap_index,
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer);
void rasterizer_environment_reflection_lightmap_masks_end(
	void);
void rasterizer_environment_reflection_mirrors_begin(
	void);
void rasterizer_environment_reflection_mirror_draw(
	struct shader const *shader,
	short bitmap_index,
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer);
void rasterizer_environment_reflection_mirrors_end(
	void);
void rasterizer_environment_reflections_begin(
	void);
void rasterizer_environment_reflection_draw(
	struct shader const *shader,
	short bitmap_index,
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer);
void rasterizer_environment_reflections_end(
	void);
void rasterizer_environment_transparent_geometry_begin(
	void);
void rasterizer_environment_transparent_geometry_submit(
	struct shader const *shader,
	short bitmap_index,
	struct bitmap_data const *bitmap,
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer);
void rasterizer_environment_transparent_geometry_end(
	void);
void rasterizer_dynamic_screen_geometry_draw(
	long primitive_type,
	long vertex_type,
	long dynamic_vertex_buffer_index,
	long vertex_count);
void rasterizer_dynamic_screen_geometry_add_multitexture_params_to_base(
	struct rasterizer_dynamic_screen_geometry_parameters *base,
	struct rasterizer_dynamic_screen_geometry_parameters const *multitex_params);
void rasterizer_dynamic_unlit_geometry_draw(
	struct shader const *shader,
	short bitmap_index,
	struct bitmap_data const *bitmap,
	long dynamic_triangle_buffer_index,
	long dynamic_vertex_buffer_index,
	long vertex_count,
	real_point3d const *centroid,
	unsigned long geometry_flags);
void rasterizer_dynamic_lit_geometry_draw(
	void const *vertices,
	void const *parameters);
void rasterizer_psuedo_dynamic_screen_quad_draw(
	long dynamic_vertex_buffer_index);
long rasterizer_widget_submit(
	long mode);
void *rasterizer_widget_begin(
	long handle);
void rasterizer_profile_enable(
	boolean enable);
void rasterizer_screen_flash(
	void);
void rasterizer_environment_fog_screen_begin(
	boolean render_fog);
void rasterizer_environment_fog_screen_wind_get_vector(
	short wind_index,
	real animation_time,
	real_vector3d *wind_vector);
void rasterizer_environment_fog_draw(
	struct shader const *shader,
	short bitmap_index,
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer);
void rasterizer_environment_fog_screen_draw(
	struct shader const *shader,
	short bitmap_index,
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer);
void rasterizer_widget_set_texture(
	long handle);
void rasterizer_widget_set_tint_factor(
	long handle);
long rasterizer_widget_set_zbuffer_enable(
	long handle,
	long enable);
short rasterizer_widget_draw_sprite2d(
	long dynamic_vertex_buffer_index);
long rasterizer_widget_draw_sprite3d(
	long zbuffer_result);
void rasterizer_widget_submit_occlusion_test(
	long handle);
long rasterizer_widget_get_occlusion_test_result(
	long handle);

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

void rasterizer_text_set_shadow_color(
	pixel32 shadow_color);
void rasterizer_text_cache_flush(
	void);
void rasterizer_text_cache_dispose(
	void);

/* ---------- globals */

extern real_argb_color *global_rasterizer_model_ambient_reflection_tint;
extern struct rasterizer_globals_definition rasterizer_globals;

/* comm. not sure where this should be */
struct rasterizer_frame_begin_parameters global_frame_parameters;

/* ---------- public code */

#endif // __RASTERIZER_H
