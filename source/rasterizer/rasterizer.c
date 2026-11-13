/* ---------- headers */

#include "cseries.h"
#include "physics/collisions.h"
#include "physics/collision_usage.h"
#include "real_math.h"
#include "rasterizer_widgets.h"
#include "rasterizer.h"
#include "rasterizer/rasterizer_console_vars.h"
#include "rasterizer/rasterizer_globals_internal.h"
#include "rasterizer_geometry.h"
#include "rasterizer_models.h"
#include <xtl.h>
#include "rasterizer/xbox/rasterizer_xbox.h"
#include "rasterizer/xbox/rasterizer_xbox_internal.h"
#include "rasterizer/xbox/rasterizer_xbox_models.h"
#include "rasterizer/xbox/rasterizer_xbox_dynavobgeom.h"
#include "render/render.h"
#include "render/render_debug.h"
#include "saved games/game_state.h"
#include "tag_files/tag_groups.h"

/* ---------- constants */

enum
{
	MAXIMUM_RASTERIZER_DEBUG_MODEL_VERTICES = 2048,
	MAXIMUM_RASTERIZER_DEBUG_MODEL_VERTEX_REFERENCES = 12,
};

/* ---------- macros */

#define rasterizer_model_obscurer_object_index bss_004662ec
#define rasterizer_debug_model_vertices_enabled rasterizer_debug_options.debug_model_vertices_enabled

/* ---------- structures */

struct rasterizer_model_vertex_compressed
{
	real_point3d position;
	unsigned long normal;
	unsigned long binormal;
	unsigned long tangent;
	point2d texture_coordinates;
	char node_indices[2];
	short node_weight;
};

struct model_geometry_part
{
	unsigned long flags;
	short shader_index;
	char previous_part_index;
	char next_part_index;
	short centroid_primary_node_index;
	short centroid_secondary_node_index;
	real centroid_primary_node_weight;
	real centroid_secondary_node_weight;
	real_point3d centroid;
	struct tag_block uncompressed_vertices;
	struct tag_block compressed_vertices;
	struct tag_block triangles;
	struct triangle_buffer triangle_buffer;
	struct vertex_buffer vertex_buffer;
};

struct rasterizer_debug_model_vertex
{
	real_point3d position;
	short triangle_vertex_indices[MAXIMUM_RASTERIZER_DEBUG_MODEL_VERTEX_REFERENCES];
	short model_vertex_indices[MAXIMUM_RASTERIZER_DEBUG_MODEL_VERTEX_REFERENCES];
	byte triangle_vertex_index_count;
	byte model_vertex_index_count;
	word pad;
};

/* ---------- prototypes */

long _rasterizer_dynamic_triangles_new(
	long triangle_count);
short *_rasterizer_dynamic_triangles_lock(
	long triangle_buffer_index);
void _rasterizer_dynamic_triangles_unlock(
	long triangle_buffer_index);
void _rasterizer_dynamic_triangles_delete(
	long triangle_buffer_index);
long _rasterizer_dynamic_vertices_new(
	short type,
	long vertex_count);
short _rasterizer_dynamic_vertices_get_type(
	long dynamic_vertex_buffer_index);
void *_rasterizer_dynamic_vertices_lock(
	long dynamic_vertex_buffer_index);
void _rasterizer_dynamic_vertices_unlock(
	long dynamic_vertex_buffer_index);
void _rasterizer_dynamic_vertices_delete(
	long dynamic_vertex_buffer_index);
void *_rasterizer_decal_vertices_lock(
	short cache_index,
	unsigned long cache_size);
long _rasterizer_decal_vertices_new(
	long size);
void _rasterizer_decal_vertices_delete(
	long decal_vertex_buffer_index);
void _rasterizer_decal_vertices_unlock(
	void);

void _rasterizer_decals_update_function_pointers(void);
void _rasterizer_decals_initialize(
	void);
void _rasterizer_decals_initialize_for_new_map(
	void);
void _rasterizer_decals_dispose_from_old_map(
	void);
void _rasterizer_decals_dispose(
	void);
void _rasterizer_decals_begin(
	short type);
void _rasterizer_decals_end(
	void);
void _rasterizer_decals_draw(
	short cluster_index);
void _rasterizer_decals_flush(
	void);
void _rasterizer_detail_objects_begin(
	void);
void _rasterizer_detail_objects_rebuild_vertices(
	struct detail_object_view_data const *detail_object_view_data);
void _rasterizer_detail_objects_draw(
	struct detail_object_view_data const *detail_object_view_data);
void _rasterizer_detail_objects_end(
	void);
void _rasterizer_screen_effect(
	struct rasterizer_cinematic_screen_effect_parameters *parameters);
void _rasterizer_hud_motion_sensor_blip_begin(
	void);
void _rasterizer_hud_motion_sensor_blip_draw(
	real_point2d const *position,
	real intensity,
	real size,
	real_rgb_color const *color,
	boolean large_blip);
void _rasterizer_hud_motion_sensor_blip_end(
	real_point2d const *center,
	real scale);
void _rasterizer_environment_lightmap_begin(
	struct bitmap_data const *lightmap_bitmap);
void _rasterizer_environment_lightmap_draw(
	struct shader const *shader,
	short bitmap_index,
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer);
void _rasterizer_environment_lightmaps_begin(
	void);
void _rasterizer_environment_lightmap_end(
	void);
void _rasterizer_environment_lightmaps_end(
	void);
void _rasterizer_environment_diffuse_light_end(
	void);
void _rasterizer_environment_diffuse_lights_end(
	void);
void _rasterizer_environment_diffuse_lights_begin(
	void);
void _rasterizer_environment_diffuse_light_begin(
	long light_index);
void _rasterizer_environment_diffuse_light_draw(
	struct shader const *shader,
	short bitmap_index,
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer);
void _rasterizer_environment_diffuse_textures_end(
	void);
void _rasterizer_environment_diffuse_textures_begin(
	void);
void _rasterizer_environment_diffuse_texture_draw(
	struct shader const *shader,
	short bitmap_index,
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer);
void _rasterizer_environment_specular_light_end(
	void);
void _rasterizer_environment_specular_lights_end(
	void);
void _rasterizer_environment_specular_lights_begin(
	void);
void _rasterizer_environment_specular_light_begin(
	long light_index);
void _rasterizer_environment_specular_light_draw(
	struct shader const *shader,
	short bitmap_index,
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer);
void _rasterizer_environment_specular_lightmap_end(
	void);
void _rasterizer_environment_specular_lightmaps_end(
	void);
void _rasterizer_environment_specular_lightmaps_begin(
	void);
void _rasterizer_environment_specular_lightmap_begin(
	struct bitmap_data const *lightmap_bitmap);
void _rasterizer_environment_specular_lightmap_draw(
	struct shader const *shader,
	short bitmap_index,
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer);
void _rasterizer_environment_reflection_lightmap_mask_end(
	void);
void _rasterizer_environment_reflection_lightmap_mask_begin(
	struct bitmap_data const *lightmap_bitmap);
void _rasterizer_environment_reflection_lightmap_mask_draw(
	struct shader const *shader,
	short bitmap_index,
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer);
void _rasterizer_environment_reflection_lightmap_masks_end(
	void);
void _rasterizer_environment_reflection_lightmap_masks_begin(
	void);
void _rasterizer_environment_transparent_geometry_begin(
	void);
void _rasterizer_environment_transparent_geometry_submit(
	struct shader const *shader,
	short shader_permutation_index,
	struct bitmap_data const *lightmap,
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffers,
	real_point3d const *centroid,
	real_plane3d const *plane,
	real_vector3d const *offset,
	void const *lighting,
	unsigned long geometry_flags);
void _rasterizer_environment_transparent_geometry_end(
	void);
void _rasterizer_environment_fog_begin(
	void);
void _rasterizer_environment_fog_end(
	void);
void _rasterizer_environment_fog_draw(
	struct shader const *shader,
	short bitmap_index,
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer);
void _rasterizer_environment_fog_screen_wind_get_vector(
	short wind_index,
	real animation_time,
	real_vector3d *wind_vector);
void _rasterizer_environment_fog_screen_begin(
	boolean render_fog);
void _rasterizer_screen_flash(
	void);
/* ---------- globals */

const struct rasterizer_global_defaults rasterizer_global_defaults =
{
	0.0625f,
	1024.f,
	0.01171875f,
	1024.f
};
struct rasterizer_globals_definition rasterizer_globals =
{
	FALSE,
	0,
	0,
	{ 0 },
	0,
	0,
	{ 0 },
	{ 0 },
	{ 0 },
	768,
	0,
	FALSE,
	TRUE,
	FALSE,
	0,
	0,
	{ 0 },
	0.0625f,
	1024.f,
	0.01171875f,
	1024.f,
	NULL,
	NULL,
	NULL
};
struct rasterizer_debug_options rasterizer_debug_options =
{
	FALSE, /* fps_accumulation */
	0, /* pad01 */
	0, /* statistics_mode */
	0, /* drawing_mode */
	FALSE, /* wireframe_enabled */
	FALSE, /* debug_model_vertices_enabled */
	NONE, /* debug_model_lod */
	FALSE, /* debug_transparent_geometry_enabled */
	FALSE, /* debug_meter_shader_enabled */
	TRUE, /* draw_models */
	TRUE, /* draw_model_transparent_geometry */
	TRUE, /* draw_first_person_weapon_first */
	TRUE, /* stencil_mask_enabled */
	2, /* draw_environment */
	TRUE, /* draw_environment_lightmaps */
	TRUE, /* draw_environment_shadows */
	TRUE, /* draw_environment_diffuse_lights */
	TRUE, /* draw_environment_textures */
	TRUE, /* draw_environment_decals */
	TRUE, /* draw_environment_specular_lights */
	TRUE, /* draw_environment_specular_lightmaps */
	TRUE, /* draw_environment_reflection_lightmap_masks */
	TRUE, /* draw_environment_reflection_mirrors */
	TRUE, /* draw_environment_reflections */
	TRUE, /* draw_environment_transparent_geometry */
	TRUE, /* draw_environment_fog */
	TRUE, /* draw_environment_fog_screen */
	TRUE, /* draw_water */
	TRUE, /* draw_lens_flares */
	TRUE, /* draw_dynamic_unlit_geometry */
	TRUE, /* draw_dynamic_lit_geometry */
	TRUE, /* draw_dynamic_screen_geometry */
	TRUE, /* draw_hud_motion_sensor */
	TRUE, /* draw_detail_objects */
	TRUE, /* draw_debug_geometry */
	FALSE, /* debug_geometry_multipass */
	TRUE, /* fog_atmospheric_enabled */
	TRUE, /* fog_planar_enabled */
	TRUE, /* bump_mapping_enabled */
	{ 0 }, /* pad2A[2] */
	1.0f, /* lightmap_ambient */
	0, /* _lightmap_mode */
	0, /* pad3 */
	TRUE, /* lightmap_incident_radiosity_enabled */
	TRUE, /* lightmap_filtering_enabled */
	{ 0 }, /* pad36[2] */
	0.0f, /* model_lighting_ambient */
	TRUE, /* environment_alpha_testing_enabled */
	TRUE, /* environment_specular_mask_enabled */
	TRUE, /* shadow_convolution_enabled */
	FALSE, /* shadow_debug_enabled */
	FALSE, /* water_mipmapping_enabled */
	TRUE, /* active_camouflage_enabled */
	TRUE, /* active_camouflage_multipass_enabled */
	TRUE, /* plasma_energy_enabled */
	TRUE, /* lens_flare_occlusion_enabled */
	FALSE, /* lens_flare_occlusion_debug */
	TRUE, /* lens_flare_sun_glow_enabled */
	TRUE, /* screen_flash_enabled */
	TRUE, /* screen_effects_enabled */
	FALSE, /* DXTC_noise_enabled */
	FALSE, /* soft_filter_enabled */
	FALSE, /* secondary_render_target_debug_enabled */
	FALSE, /* profile_log_enabled */
	{ 0 }, /* pad4D[3] */
	0.4f, /* detail_object_screen_facing_offset_multiplier */
	8, /* zbias */
	1.0f / 256.0f, /* zoffset */
	FALSE, /* force_all_player_views_to_default_player */
	FALSE, /* safe_frame_bounds_adjust_enabled */
	0, /* freeze_flying_camera */
	TRUE, /* zsprite_enabled */
	TRUE, /* filthy_decal_fog_hack_enabled */
	TRUE, /* smart_states_enabled */
	FALSE, /* splitscreen_VB_optimization_enabled */
	FALSE, /* profile_print_locks */
	{ 0 }, /* pad65[3] */
	0.0f, /* profile_objectlock_time */
	1.0f, /* pad3_scale */
	{ 0 }, /* f[6] */
	FALSE, /* transparent_pixel_counter_active */
	FALSE, /* transparent_pixel_counter */
	{ 0 }, /* pad8A[2] */
};
/* No PDB name survives for this target-owned BSS symbol. */
#ifndef HALO_ARM64_GUEST /* Mach-O section names differ; the default is .bss anyway */
#pragma bss_seg(".bss")
#endif
static long bss_004662ec;
real_argb_color *global_rasterizer_model_ambient_reflection_tint;
#ifndef HALO_ARM64_GUEST
#pragma bss_seg()
#endif

/* ---------- public code */

boolean rasterizer_initialize(
	void)
{
	global_rasterizer_model_ambient_reflection_tint = game_state_malloc(
		"rasterizer model ambient reflection tint",
		NULL,
		sizeof(*global_rasterizer_model_ambient_reflection_tint));
	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\rasterizer.c",
		289,
		global_rasterizer_model_ambient_reflection_tint);
	return _rasterizer_initialize();
}

void rasterizer_reset_state(
	void)
{
	_rasterizer_reset_state();
	return;
}

void rasterizer_frame_begin(
	struct rasterizer_frame_begin_parameters const *parameters)
{
	if (rasterizer_debug_options.draw_environment <= 1)
	{
		rasterizer_debug_options.draw_environment_fog = rasterizer_debug_options.draw_environment;
		rasterizer_debug_options.draw_environment_transparent_geometry = rasterizer_debug_options.draw_environment;
		rasterizer_debug_options.draw_environment_reflections = rasterizer_debug_options.draw_environment;
		rasterizer_debug_options.draw_environment_reflection_mirrors = rasterizer_debug_options.draw_environment;
		rasterizer_debug_options.draw_environment_reflection_lightmap_masks = rasterizer_debug_options.draw_environment;
		rasterizer_debug_options.draw_environment_specular_lightmaps = rasterizer_debug_options.draw_environment;
		rasterizer_debug_options.draw_environment_specular_lights = rasterizer_debug_options.draw_environment;
		rasterizer_debug_options.draw_environment_decals = rasterizer_debug_options.draw_environment;
		rasterizer_debug_options.draw_environment_textures = rasterizer_debug_options.draw_environment;
		rasterizer_debug_options.draw_environment_shadows = rasterizer_debug_options.draw_environment;
		rasterizer_debug_options.draw_environment_diffuse_lights = rasterizer_debug_options.draw_environment;
		rasterizer_debug_options.draw_environment_lightmaps = rasterizer_debug_options.draw_environment;
		rasterizer_debug_options.draw_environment_fog_screen = rasterizer_debug_options.draw_environment;
		rasterizer_debug_options.draw_environment = 2;
	}

	if (rasterizer_globals.near_clip_distance == 0.f)
		rasterizer_globals.near_clip_distance = rasterizer_global_defaults.near_clip_distance;
	if (rasterizer_globals.far_clip_distance == 0.f)
		rasterizer_globals.far_clip_distance = rasterizer_global_defaults.far_clip_distance;
	if (rasterizer_globals.first_person_weapon_near_clip_distance == 0.f)
		rasterizer_globals.first_person_weapon_near_clip_distance = rasterizer_global_defaults.first_person_weapon_near_clip_distance;
	if (rasterizer_globals.first_person_weapon_far_clip_distance == 0.f)
		rasterizer_globals.first_person_weapon_far_clip_distance = rasterizer_global_defaults.first_person_weapon_far_clip_distance;

	_rasterizer_frame_begin(parameters);
	return;
}

void rasterizer_present(
	struct bitmap_data *screenshot_bitmap,
	point2d const *screenshot_index)
{
	_rasterizer_present(screenshot_bitmap, screenshot_index);
	return;
}

void rasterizer_window_get_fog(
	struct render_fog *fog)
{
	_rasterizer_window_get_fog(fog);
	return;
}

void rasterizer_window_set_fog(
	struct render_fog const *fog)
{
	_rasterizer_window_set_fog(fog);
	return;
}

void rasterizer_set_vblank_callback(
	void (*callback)(unsigned long))
{
	_rasterizer_set_vblank_callback(callback);
	return;
}

long rasterizer_dynamic_triangles_new(
	long triangle_count)
{
	return _rasterizer_dynamic_triangles_new(triangle_count);
}

short *rasterizer_dynamic_triangles_lock(
	long triangle_buffer_index)
{
	return _rasterizer_dynamic_triangles_lock(triangle_buffer_index);
}

void rasterizer_dynamic_triangles_unlock(
	long triangle_buffer_index)
{
	_rasterizer_dynamic_triangles_unlock(triangle_buffer_index);
	return;
}

void rasterizer_dynamic_triangles_delete(
	long triangle_buffer_index)
{
	_rasterizer_dynamic_triangles_delete(triangle_buffer_index);
	return;
}

long rasterizer_dynamic_vertices_new(
	short type,
	long vertex_count)
{
	return _rasterizer_dynamic_vertices_new(type, vertex_count);
}

short rasterizer_dynamic_vertices_get_type(
	long dynamic_vertex_buffer_index)
{
	return _rasterizer_dynamic_vertices_get_type(dynamic_vertex_buffer_index);
}

void *rasterizer_dynamic_vertices_lock(
	long dynamic_vertex_buffer_index)
{
	return _rasterizer_dynamic_vertices_lock(dynamic_vertex_buffer_index);
}

void rasterizer_dynamic_vertices_unlock(
	long dynamic_vertex_buffer_index)
{
	_rasterizer_dynamic_vertices_unlock(dynamic_vertex_buffer_index);
	return;
}

void rasterizer_dynamic_vertices_delete(
	long dynamic_vertex_buffer_index)
{
	_rasterizer_dynamic_vertices_delete(dynamic_vertex_buffer_index);
	return;
}

long rasterizer_decal_vertices_new(
	long size)
{
	return _rasterizer_decal_vertices_new(size);
}

void rasterizer_decal_vertices_delete(
	long decal_vertex_buffer_index)
{
	_rasterizer_decal_vertices_delete(decal_vertex_buffer_index);
	return;
}

void rasterizer_debug_immediate_line(
	real_point3d const *p0,
	real_point3d const *p1,
	real_rgb_color const *color0,
	real_rgb_color const *color1)
{
	_rasterizer_debug_immediate_line(p0, p1, color0, color1);
	return;
}

void rasterizer_debug_immediate_triangle(
	real_point3d const *p0,
	real_point3d const *p1,
	real_point3d const *p2,
	real_rgb_color const *color0,
	real_rgb_color const *color1,
	real_rgb_color const *color2)
{
	_rasterizer_debug_immediate_triangle(p0, p1, p2, color0, color1, color2);
	return;
}

void rasterizer_debug_immediate_line_screenspace(
	point2d const *p0,
	point2d const *p1,
	real_rgb_color const *color0,
	real_rgb_color const *color1)
{
	_rasterizer_debug_immediate_line_screenspace(p0, p1, color0, color1);
	return;
}

void rasterizer_debug_immediate_linestrip_screenspace(
	point2d const *points,
	short point_count,
	real_rgb_color const *color)
{
	_rasterizer_debug_immediate_linestrip_screenspace(points, point_count, color);
	return;
}

void *rasterizer_decal_vertices_lock(
	short cache_index,
	unsigned long cache_size)
{
	return _rasterizer_decal_vertices_lock(cache_index, cache_size);
}

void rasterizer_decal_vertices_unlock(
	void)
{
	_rasterizer_decal_vertices_unlock();
	return;
}

void rasterizer_widget_end(
	void)
{
	_rasterizer_widget_end();
	return;
}

void rasterizer_decals_initialize(
	void)
{
	_rasterizer_decals_initialize();
	return;
}

void rasterizer_decals_update_function_pointers(void)
{
	_rasterizer_decals_update_function_pointers();
}

void rasterizer_decals_initialize_for_new_map(
	void)
{
	_rasterizer_decals_initialize_for_new_map();
	return;
}

void rasterizer_decals_dispose_from_old_map(
	void)
{
	_rasterizer_decals_dispose_from_old_map();
	return;
}

void rasterizer_decals_dispose(
	void)
{
	_rasterizer_decals_dispose();
	return;
}

void rasterizer_decals_begin(
	short type)
{
	_rasterizer_decals_begin(type);
	return;
}

void rasterizer_decals_end(
	void)
{
	_rasterizer_decals_end();
	return;
}

void rasterizer_decals_draw(
	short cluster_index)
{
	_rasterizer_decals_draw(cluster_index);
	return;
}

void rasterizer_decals_flush(
	void)
{
	_rasterizer_decals_flush();
	return;
}

void rasterizer_detail_objects_begin(
	void)
{
	_rasterizer_detail_objects_begin();
	return;
}

void rasterizer_detail_objects_rebuild_vertices(
	struct detail_object_view_data *view_data)
{
	_rasterizer_detail_objects_rebuild_vertices(view_data);
	return;
}

void rasterizer_detail_objects_draw(
	struct detail_object_view_data *view_data)
{
	_rasterizer_detail_objects_draw(view_data);
	return;
}

void rasterizer_detail_objects_end(
	void)
{
	_rasterizer_detail_objects_end();
	return;
}

void rasterizer_screen_effect(
	struct rasterizer_cinematic_screen_effect_parameters *parameters)
{
	_rasterizer_screen_effect(parameters);
	return;
}

void rasterizer_debug_immediate_begin(
	void)
{
	_rasterizer_debug_immediate_begin();
	return;
}

void rasterizer_debug_immediate_end(
	void)
{
	_rasterizer_debug_immediate_end();
	return;
}

void rasterizer_debug_immediate_begin_screenspace(
	void)
{
	_rasterizer_debug_immediate_begin_screenspace();
	return;
}

void rasterizer_debug_immediate_end_screenspace(
	void)
{
	_rasterizer_debug_immediate_end_screenspace();
	return;
}

void rasterizer_hud_begin(
	void)
{
	_rasterizer_hud_begin();
	return;
}

void rasterizer_hud_end(
	void)
{
	_rasterizer_hud_end();
	return;
}

void rasterizer_hud_motion_sensor_blip_begin(
	void)
{
	_rasterizer_hud_motion_sensor_blip_begin();
	return;
}

void rasterizer_hud_motion_sensor_blip_draw(
	real_point2d const *position,
	real intensity,
	real size,
	real_rgb_color const *color,
	boolean large_blip)
{
	_rasterizer_hud_motion_sensor_blip_draw(position, intensity, size, color, large_blip);
	return;
}

void rasterizer_hud_motion_sensor_blip_end(
	real_point2d const *center,
	real scale)
{
	_rasterizer_hud_motion_sensor_blip_end(center, scale);
	return;
}

void rasterizer_model_begin(
	struct rasterizer_model_begin_parameters const *parameters,
	boolean is_dynamic)
{
	_rasterizer_model_begin(parameters, is_dynamic);
	return;
}

void rasterizer_model_draw(
	struct shader *shader,
	short shader_permutation_index,
	struct triangle_buffer const *triangle_buffer,
	long dynamic_triangle_buffer_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer,
	long dynamic_vertex_buffer_index)
{
	_rasterizer_model_draw(
		shader,
		shader_permutation_index,
		triangle_buffer,
		dynamic_triangle_buffer_index,
		triangle_count,
		vertex_buffer,
		dynamic_vertex_buffer_index);
	return;
}

void rasterizer_model_transparent_geometry_submit(
	struct shader *shader,
	short shader_permutation_index,
	struct triangle_buffer const *triangle_buffer,
	long dynamic_triangle_buffer_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer,
	long dynamic_vertex_buffer_index,
	real_point3d const *centroid,
	struct render_sort_filth *sort_filth)
{
	_rasterizer_model_transparent_geometry_submit(
		shader,
		shader_permutation_index,
		triangle_buffer,
		dynamic_triangle_buffer_index,
		triangle_count,
		vertex_buffer,
		dynamic_vertex_buffer_index,
		centroid,
		sort_filth);
	return;
}

void rasterizer_model_end(
	void)
{
	_rasterizer_model_end();
	return;
}

void rasterizer_models_end(
	void)
{
	_rasterizer_models_end();
	return;
}

void rasterizer_environment_lightmaps_begin(
	void)
{
	_rasterizer_environment_lightmaps_begin();
	return;
}

void rasterizer_environment_lightmap_begin(
	struct bitmap_data const *lightmap_bitmap)
{
	_rasterizer_environment_lightmap_begin(lightmap_bitmap);
	return;
}

void rasterizer_environment_lightmap_draw(
	struct shader const *shader,
	short bitmap_index,
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer)
{
	_rasterizer_environment_lightmap_draw(shader, bitmap_index, dynamic_triangle_buffer_index, first_triangle_index, triangle_count, vertex_buffer);
	return;
}

void rasterizer_environment_lightmap_end(
	void)
{
	_rasterizer_environment_lightmap_end();
	return;
}

void rasterizer_environment_lightmaps_end(
	void)
{
	_rasterizer_environment_lightmaps_end();
	return;
}

void rasterizer_environment_diffuse_light_end(
	void)
{
	_rasterizer_environment_diffuse_light_end();
	return;
}

void rasterizer_environment_diffuse_lights_end(
	void)
{
	_rasterizer_environment_diffuse_lights_end();
	return;
}

void rasterizer_environment_diffuse_lights_begin(
	void)
{
	_rasterizer_environment_diffuse_lights_begin();
	return;
}

void rasterizer_environment_diffuse_light_begin(
	long light_index)
{
	_rasterizer_environment_diffuse_light_begin(light_index);
	return;
}

void rasterizer_environment_diffuse_light_draw(
	struct shader const *shader,
	short bitmap_index,
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer)
{
	_rasterizer_environment_diffuse_light_draw(shader, bitmap_index, dynamic_triangle_buffer_index, first_triangle_index, triangle_count, vertex_buffer);
	return;
}

void rasterizer_environment_shadows_begin(
	void)
{
	_rasterizer_environment_shadows_begin();
	return;
}

boolean rasterizer_environment_shadow_begin(
	long object_index,
	real_matrix4x3 const *shadow_matrix,
	real_rgb_color const *shadow_color,
	real object_bounding_radius,
	real *shadow_volume_bounding_radius)
{
	return _rasterizer_environment_shadow_begin(
		object_index,
		shadow_matrix,
		shadow_color,
		object_bounding_radius,
		shadow_volume_bounding_radius);
}

void rasterizer_environment_shadow_model_begin(
	struct rasterizer_model_begin_parameters const *parameters)
{
	_rasterizer_environment_shadow_model_begin(parameters);
	return;
}

void rasterizer_environment_shadow_model_draw(
	struct shader const *shader,
	short bitmap_index,
	struct triangle_buffer const *triangle_buffer,
	struct vertex_buffer const *vertex_buffer)
{
	_rasterizer_environment_shadow_model_draw(
		shader,
		bitmap_index,
		triangle_buffer,
		vertex_buffer);
	return;
}

void rasterizer_environment_shadow_model_end(
	void)
{
	_rasterizer_environment_shadow_model_end();
	return;
}

void rasterizer_environment_shadow_end(
	void)
{
	_rasterizer_environment_shadow_end();
	return;
}

void rasterizer_environment_shadow_draw(
	struct shader const *shader,
	short bitmap_index,
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer)
{
	_rasterizer_environment_shadow_draw(shader, bitmap_index, dynamic_triangle_buffer_index, first_triangle_index, triangle_count, vertex_buffer);
	return;
}

void rasterizer_environment_shadows_end(
	void)
{
	_rasterizer_environment_shadows_end();
	return;
}

void rasterizer_environment_diffuse_textures_end(
	void)
{
	_rasterizer_environment_diffuse_textures_end();
	return;
}

void rasterizer_environment_diffuse_textures_begin(
	void)
{
	_rasterizer_environment_diffuse_textures_begin();
	return;
}

void rasterizer_environment_diffuse_texture_draw(
	struct shader const *shader,
	short bitmap_index,
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer)
{
	_rasterizer_environment_diffuse_texture_draw(shader, bitmap_index, dynamic_triangle_buffer_index, first_triangle_index, triangle_count, vertex_buffer);
	return;
}

void rasterizer_environment_specular_light_end(
	void)
{
	_rasterizer_environment_specular_light_end();
	return;
}

void rasterizer_environment_specular_lights_end(
	void)
{
	_rasterizer_environment_specular_lights_end();
	return;
}

void rasterizer_environment_specular_lights_begin(
	void)
{
	_rasterizer_environment_specular_lights_begin();
	return;
}

void rasterizer_environment_specular_light_begin(
	long light_index)
{
	_rasterizer_environment_specular_light_begin(light_index);
	return;
}

void rasterizer_environment_specular_light_draw(
	struct shader const *shader,
	short bitmap_index,
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer)
{
	_rasterizer_environment_specular_light_draw(shader, bitmap_index, dynamic_triangle_buffer_index, first_triangle_index, triangle_count, vertex_buffer);
	return;
}

void rasterizer_environment_specular_lightmap_end(
	void)
{
	_rasterizer_environment_specular_lightmap_end();
	return;
}

void rasterizer_environment_specular_lightmaps_end(
	void)
{
	_rasterizer_environment_specular_lightmaps_end();
	return;
}

void rasterizer_environment_specular_lightmaps_begin(
	void)
{
	_rasterizer_environment_specular_lightmaps_begin();
	return;
}

void rasterizer_environment_specular_lightmap_begin(
	struct bitmap_data const *lightmap_bitmap)
{
	_rasterizer_environment_specular_lightmap_begin(lightmap_bitmap);
	return;
}

void rasterizer_environment_specular_lightmap_draw(
	struct shader const *shader,
	short bitmap_index,
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer)
{
	_rasterizer_environment_specular_lightmap_draw(shader, bitmap_index, dynamic_triangle_buffer_index, first_triangle_index, triangle_count, vertex_buffer);
	return;
}

void rasterizer_environment_reflection_lightmap_mask_end(
	void)
{
	_rasterizer_environment_reflection_lightmap_mask_end();
	return;
}

void rasterizer_environment_reflection_lightmap_masks_end(
	void)
{
	_rasterizer_environment_reflection_lightmap_masks_end();
	return;
}

void rasterizer_environment_reflection_lightmap_masks_begin(
	void)
{
	_rasterizer_environment_reflection_lightmap_masks_begin();
	return;
}

void rasterizer_environment_reflection_mirrors_begin(
	void)
{
	_rasterizer_environment_reflection_mirrors_begin();
	return;
}

void rasterizer_environment_reflection_mirror_draw(
	struct shader const *shader,
	short bitmap_index,
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer)
{
	_rasterizer_environment_reflection_mirror_draw(shader, bitmap_index, dynamic_triangle_buffer_index, first_triangle_index, triangle_count, vertex_buffer);
	return;
}

void rasterizer_environment_reflection_mirrors_end(
	void)
{
	_rasterizer_environment_reflection_mirrors_end();
	return;
}

void rasterizer_environment_reflections_begin(
	void)
{
	_rasterizer_environment_reflections_begin();
	return;
}

void rasterizer_environment_reflection_draw(
	struct shader const *shader,
	short bitmap_index,
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer)
{
	_rasterizer_environment_reflection_draw(shader, bitmap_index, dynamic_triangle_buffer_index, first_triangle_index, triangle_count, vertex_buffer);
	return;
}

void rasterizer_environment_reflections_end(
	void)
{
	_rasterizer_environment_reflections_end();
	return;
}

void rasterizer_environment_transparent_geometry_begin(
	void)
{
	_rasterizer_environment_transparent_geometry_begin();
	return;
}

void rasterizer_environment_transparent_geometry_submit(
	struct shader const *shader,
	short shader_permutation_index,
	struct bitmap_data const *lightmap,
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffers,
	real_point3d const *centroid,
	real_plane3d const *plane,
	real_vector3d const *offset,
	void const *lighting,
	unsigned long geometry_flags)
{
	_rasterizer_environment_transparent_geometry_submit(
		shader,
		shader_permutation_index,
		lightmap,
		dynamic_triangle_buffer_index,
		first_triangle_index,
		triangle_count,
		vertex_buffers,
		centroid,
		plane,
		offset,
		lighting,
		geometry_flags);
	return;
}

void rasterizer_environment_transparent_geometry_end(
	void)
{
	_rasterizer_environment_transparent_geometry_end();
	return;
}

void rasterizer_environment_reflection_lightmap_mask_begin(
	struct bitmap_data const *lightmap_bitmap)
{
	_rasterizer_environment_reflection_lightmap_mask_begin(lightmap_bitmap);
	return;
}

void rasterizer_environment_reflection_lightmap_mask_draw(
	struct shader const *shader,
	short bitmap_index,
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer)
{
	_rasterizer_environment_reflection_lightmap_mask_draw(shader, bitmap_index, dynamic_triangle_buffer_index, first_triangle_index, triangle_count, vertex_buffer);
	return;
}

void rasterizer_dynamic_screen_geometry_draw(
	long primitive_type,
	long vertex_type,
	long dynamic_vertex_buffer_index,
	long vertex_count)
{
	_rasterizer_dynamic_screen_geometry_draw(primitive_type, vertex_type, dynamic_vertex_buffer_index, vertex_count);
	return;
}

void rasterizer_dynamic_screen_geometry_add_multitexture_params_to_base(
	struct rasterizer_dynamic_screen_geometry_parameters *base,
	struct rasterizer_dynamic_screen_geometry_parameters const *multitex_params)
{
	_rasterizer_dynamic_screen_geometry_add_multitexture_params_to_base(base, multitex_params);
	return;
}

void rasterizer_profile_enable(
	boolean enable)
{
	_rasterizer_profile_enable(enable);
	return;
}

void rasterizer_screen_flash(
	void)
{
	_rasterizer_screen_flash();
	return;
}

void rasterizer_environment_fog_screen_begin(
	boolean render_fog)
{
	_rasterizer_environment_fog_screen_begin(render_fog);
	return;
}

void rasterizer_environment_fog_screen_wind_get_vector(
	short wind_index,
	real animation_time,
	real_vector3d *wind_vector)
{
	_rasterizer_environment_fog_screen_wind_get_vector(wind_index, animation_time, wind_vector);
	return;
}

void rasterizer_environment_fog_screen_draw(
	struct shader const *shader,
	short bitmap_index,
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer)
{
	_rasterizer_environment_fog_screen_draw(shader, bitmap_index, dynamic_triangle_buffer_index, first_triangle_index, triangle_count, vertex_buffer);
	return;
}

void rasterizer_dynamic_lit_geometry_draw(
	void const *vertices,
	void const *parameters)
{
	_rasterizer_dynamic_lit_geometry_draw(vertices, parameters);
	return;
}

void rasterizer_dynamic_unlit_geometry_draw(
	struct shader const *shader,
	struct bitmap_data const *bitmap,
	struct render_animation const *animation,
	long dynamic_triangle_buffer_index,
	long dynamic_vertex_buffer_index,
	long vertex_count,
	real_point3d const *centroid,
	unsigned long geometry_flags)
{
	_rasterizer_dynamic_unlit_geometry_draw(shader, bitmap, animation, dynamic_triangle_buffer_index, dynamic_vertex_buffer_index, vertex_count, centroid, geometry_flags);
	return;
}

void rasterizer_psuedo_dynamic_screen_quad_draw(
	struct rasterizer_dynamic_screen_geometry_parameters *parameters,
	struct dynamic_screen_vertex *vertices)
{
	_rasterizer_psuedo_dynamic_screen_quad_draw(parameters, vertices);
	return;
}

void rasterizer_widget_submit(
	long object_index,
	long widget_index,
	real_point3d const *centroid,
	rasterizer_widget_render_proc render_proc)
{
	_rasterizer_widget_submit(
		object_index,
		widget_index,
		centroid,
		render_proc);
	return;
}

void rasterizer_widget_begin(
	short type,
	word flags)
{
	_rasterizer_widget_begin(
		type,
		flags);
	return;
}

boolean rasterizer_widget_set_texture(
	short stage_index,
	long bitmap_group_index,
	short sequence_index)
{
	return _rasterizer_widget_set_texture(
		stage_index,
		bitmap_group_index,
		sequence_index);
}

void rasterizer_widget_set_tint_factor(
	real tint_factor)
{
	_rasterizer_widget_set_tint_factor(tint_factor);
	return;
}

void rasterizer_widget_set_zbuffer_enable(
	boolean zbuffer_enable)
{
	_rasterizer_widget_set_zbuffer_enable(zbuffer_enable);
	return;
}

void rasterizer_widget_draw_sprite2d(
	real_point2d const *point,
	real radius,
	real_vector2d const *scale,
	real_vector2d const *texture_scale,
	real rotation,
	unsigned long color)
{
	_rasterizer_widget_draw_sprite2d(
		point,
		radius,
		scale,
		texture_scale,
		rotation,
		color);
	return;
}

void rasterizer_widget_draw_sprite3d(
	real_point3d const *point,
	real radius,
	real_vector2d const *scale,
	real rotation,
	unsigned long color)
{
	_rasterizer_widget_draw_sprite3d(
		point,
		radius,
		scale,
		rotation,
		color);
	return;
}

long rasterizer_widget_submit_occlusion_test(
	real_point3d const *point,
	real radius,
	long index)
{
	return _rasterizer_widget_submit_occlusion_test(
		point,
		radius,
		index);
}

long rasterizer_widget_get_occlusion_test_result(
	long index)
{
	return _rasterizer_widget_get_occlusion_test_result(index);
}

void rasterizer_environment_fog_begin(
	void)
{
	_rasterizer_environment_fog_begin();
	return;
}

void rasterizer_environment_fog_end(
	void)
{
	_rasterizer_environment_fog_end();
	return;
}

void rasterizer_environment_fog_draw(
	struct shader const *shader,
	short bitmap_index,
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer)
{
	_rasterizer_environment_fog_draw(shader, bitmap_index, dynamic_triangle_buffer_index, first_triangle_index, triangle_count, vertex_buffer);
	return;
}

void rasterizer_environment_fog_screen_end(
	void)
{
	_rasterizer_environment_fog_screen_end();
	return;
}

void rasterizer_dispose(void)
{
	_rasterizer_dispose();
}

void rasterizer_frame_end(void)
{
	_rasterizer_frame_end();
}

void rasterizer_window_begin(
	struct rasterizer_window_begin_parameters const *parameters)
{
	_rasterizer_window_begin(parameters);
	return;
}

void rasterizer_window_end(
	void)
{
	_rasterizer_window_end();
	return;
}

void rasterizer_windows_begin(
	void)
{
	_rasterizer_windows_begin();

	return;
}

void rasterizer_windows_end(void)
{
	_rasterizer_windows_end();
}

void rasterizer_models_begin(
	boolean skip_obscurer_test)
{
	struct collision_result collision;
	real_vector3d vector;

	if (!skip_obscurer_test)
	{
		match_assert(
			"c:\\halo\\SOURCE\\rasterizer\\rasterizer.c",
			740,
			global_current_collision_user_depth < MAXIMUM_COLLISION_USER_STACK_DEPTH);
		global_current_collision_users[global_current_collision_user_depth++] = 21;
		vector.i = global_window_parameters.camera.forward.i * 10000.f;
		vector.j = global_window_parameters.camera.forward.j * 10000.f;
		vector.k = global_window_parameters.camera.forward.k * 10000.f;
		if (collision_test_vector(
			0xFFF80,
			&global_window_parameters.camera.position,
			&vector,
			render.local_player_index,
			&collision))
		{
			rasterizer_model_obscurer_object_index = collision.object_index;
		}
		match_assert(
			"c:\\halo\\SOURCE\\rasterizer\\rasterizer.c",
			756,
			global_current_collision_user_depth > 1);
		--global_current_collision_user_depth;
	}
	else
	{
		rasterizer_model_obscurer_object_index = NONE;
	}
	_rasterizer_models_begin(skip_obscurer_test);
	return;
}

void rasterizer_debug_immediate_point(
	real_point3d const *point,
	real radius,
	real_rgb_color const *color)
{
	real_point3d p0;
	real_point3d p1;

	radius *= 0.5f;
	set_real_point3d(&p0, point->x - radius, point->y, point->z);
	set_real_point3d(&p1, point->x + radius, point->y, point->z);
	_rasterizer_debug_immediate_line(&p0, &p1, color, color);
	set_real_point3d(&p0, point->x, point->y - radius, point->z);
	set_real_point3d(&p1, point->x, point->y + radius, point->z);
	_rasterizer_debug_immediate_line(&p0, &p1, color, color);
	set_real_point3d(&p0, point->x, point->y, point->z - radius);
	set_real_point3d(&p1, point->x, point->y, point->z + radius);
	_rasterizer_debug_immediate_line(&p0, &p1, color, color);
	return;
}

void rasterizer_debug_immediate_vector(
	real_point3d const *point,
	real_vector3d const *vector,
	real scale,
	real_rgb_color const *color)
{
	real_point3d endpoint;

	endpoint.x = point->x + scale * vector->i;
	endpoint.y = point->y + scale * vector->j;
	endpoint.z = point->z + scale * vector->k;
	_rasterizer_debug_immediate_line(point, &endpoint, color, color);
	return;
}

void rasterizer_debug_model_vertices(
	long object_index,
	struct render_skinning const *skinning,
	struct model_geometry_part const *part)
{
	long debug_vertex_count;
	long closest_debug_vertex_index;
	real closest_debug_vertex_dot;
	short triangle_vertex_index;
	char model_vertex_string[256];
	char triangle_vertex_string[256];
	/* port: static. render_model calls this for each part it draws, and
	the compiler inlines it there: on the stack, the Windows build probed
	the 128 KB the debug display rarely uses a page at a time (_chkstk) for
	every model drawn */
	static struct rasterizer_debug_model_vertex debug_vertices[MAXIMUM_RASTERIZER_DEBUG_MODEL_VERTICES];

	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\rasterizer.c",
		830,
		part);

	if (rasterizer_debug_model_vertices_enabled &&
		object_index == rasterizer_model_obscurer_object_index)
	{
		debug_vertex_count = 0;
		closest_debug_vertex_index = NONE;
		closest_debug_vertex_dot = -1.f;
		match_assert(
			"c:\\halo\\SOURCE\\rasterizer\\rasterizer.c",
			857,
			part->triangle_buffer.type==_triangle_buffer_type_precompiled_strip);

		for (triangle_vertex_index = 0;
			triangle_vertex_index < part->triangle_buffer.count + 2;
			triangle_vertex_index++)
		{
			word model_vertex_index = ((word const *)part->triangles.address)[triangle_vertex_index];
			struct rasterizer_model_vertex_compressed const *vertex =
				&((struct rasterizer_model_vertex_compressed const *)part->compressed_vertices.address)[model_vertex_index];
			short node_index0;
			short node_index1;
			real node_weight0;
			real node_weight1;
			real_vector3d vertex_normal;
			real_point3d point;
			real_vector3d normal;

			node_index0 = vertex->node_indices[0] / 3;
			node_weight0 = (real)vertex->node_weight * (1.f / 32767.f);
			node_weight1 = 1.f - node_weight0;
			node_index1 = vertex->node_indices[1] / 3;
			{
				real_point3d node_point0 = { 0.f, 0.f, 0.f };
				real_vector3d node_normal0 = { 0.f, 0.f, 0.f };
				real_point3d node_point1 = { 0.f, 0.f, 0.f };
				real_vector3d node_normal1 = { 0.f, 0.f, 0.f };
				long debug_vertex_index;

			vertex_normal = uncompress_int32_to_real_vector3d(vertex->normal);
			match_assert(
				"c:\\halo\\SOURCE\\rasterizer\\rasterizer.c",
				878,
				node_index0<skinning->node_matrix_count);
			match_assert(
				"c:\\halo\\SOURCE\\rasterizer\\rasterizer.c",
				879,
				node_index1<skinning->node_matrix_count);
			match_assert(
				"c:\\halo\\SOURCE\\rasterizer\\rasterizer.c",
				880,
				node_weight0>=0.0f && node_weight0<=1.0f);

			if (node_index0 >= 0)
			{
				matrix4x3_transform_point(&skinning->node_matrices[node_index0], &vertex->position, &node_point0);
				matrix4x3_transform_vector(&skinning->node_matrices[node_index0], &vertex_normal, &node_normal0);
			}
			if (node_index1 >= 0)
			{
				matrix4x3_transform_point(&skinning->node_matrices[node_index1], &vertex->position, &node_point1);
				matrix4x3_transform_vector(&skinning->node_matrices[node_index1], &vertex_normal, &node_normal1);
			}

			point.x = node_point0.x * node_weight0 + node_point1.x * node_weight1;
			point.y = node_point0.y * node_weight0 + node_point1.y * node_weight1;
			point.z = node_point0.z * node_weight0 + node_point1.z * node_weight1;
			normal.i = node_normal0.i * node_weight0 + node_normal1.i * node_weight1;
			normal.j = node_normal0.j * node_weight0 + node_normal1.j * node_weight1;
			normal.k = node_normal0.k * node_weight0 + node_normal1.k * node_weight1;
			/* Preserve January VC7 scheduling: this inline self-copy is eliminated,
			 * but keeps the x87 result pop ahead of the loop-bound reload. */
			normalize3d(set_real_vector3d(
				&normal,
				normal.i,
				normal.j,
				normal.k));

			for (debug_vertex_index = 0; debug_vertex_index < debug_vertex_count; debug_vertex_index++)
			{
				struct rasterizer_debug_model_vertex *debug_vertex = &debug_vertices[debug_vertex_index];
				if (point.x == debug_vertex->position.x &&
					point.y == debug_vertex->position.y &&
					point.z == debug_vertex->position.z)
				{
					short reference_index;

					if (debug_vertex->triangle_vertex_index_count < MAXIMUM_RASTERIZER_DEBUG_MODEL_VERTEX_REFERENCES)
					{
						for (reference_index = 0;
							reference_index < debug_vertex->triangle_vertex_index_count &&
							debug_vertex->triangle_vertex_indices[reference_index] != triangle_vertex_index;
							reference_index++)
						{
						}
						if (reference_index == debug_vertex->triangle_vertex_index_count)
						{
							debug_vertex->triangle_vertex_indices[debug_vertex->triangle_vertex_index_count++] = triangle_vertex_index;
						}
					}

					if (debug_vertex->model_vertex_index_count < MAXIMUM_RASTERIZER_DEBUG_MODEL_VERTEX_REFERENCES)
					{
						for (reference_index = 0;
							reference_index < debug_vertex->model_vertex_index_count &&
							debug_vertex->model_vertex_indices[reference_index] != model_vertex_index;
							reference_index++)
						{
						}
						if (reference_index == debug_vertex->model_vertex_index_count)
						{
							debug_vertex->model_vertex_indices[debug_vertex->model_vertex_index_count++] = model_vertex_index;
						}
					}
					break;
				}
			}

			if (debug_vertex_index == debug_vertex_count &&
				debug_vertex_count < MAXIMUM_RASTERIZER_DEBUG_MODEL_VERTICES)
			{
				struct rasterizer_debug_model_vertex *debug_vertex = &debug_vertices[debug_vertex_count];
				real_vector3d camera_to_vertex;
				real camera_dot;

				debug_vertex->position = point;
				debug_vertex->triangle_vertex_indices[0] = triangle_vertex_index;
				debug_vertex->model_vertex_indices[0] = model_vertex_index;
				debug_vertex->triangle_vertex_index_count = 1;
				debug_vertex->model_vertex_index_count = 1;

				vector_from_points3d(
					&global_window_parameters.camera.position,
					&point,
					&camera_to_vertex);
				normalize3d(&camera_to_vertex);
				camera_dot = dot_product3d(
					&camera_to_vertex,
					&global_window_parameters.camera.forward);
				if ((dot_product3d(&camera_to_vertex, &normal) < 0.f &&
					closest_debug_vertex_dot < camera_dot) ||
					closest_debug_vertex_dot == -1.f)
				{
					closest_debug_vertex_index = debug_vertex_index;
					closest_debug_vertex_dot = camera_dot;
				}
				debug_vertex_count++;
			}
			}
		}

		{
			long debug_vertex_index;
			for (debug_vertex_index = 0; debug_vertex_index < debug_vertex_count; debug_vertex_index++)
			{
				struct rasterizer_debug_model_vertex *debug_vertex = &debug_vertices[debug_vertex_index];
				if (debug_vertex_index == closest_debug_vertex_index)
				{
					short reference_index;

					csstrcpy(temporary, "I=");
					for (reference_index = 0; reference_index < debug_vertex->triangle_vertex_index_count; reference_index++)
					{
						sprintf(
							triangle_vertex_string,
							"%d%c",
							debug_vertex->triangle_vertex_indices[reference_index],
							reference_index == debug_vertex->triangle_vertex_index_count - 1 ? ' ' : ',');
						csstrcat(temporary, triangle_vertex_string);
					}
					csstrcat(temporary, "\nV=");
					for (reference_index = 0; reference_index < debug_vertex->model_vertex_index_count; reference_index++)
					{
						sprintf(
							model_vertex_string,
							"%d%c",
							debug_vertex->model_vertex_indices[reference_index],
							reference_index == debug_vertex->model_vertex_index_count - 1 ? ' ' : ',');
						csstrcat(temporary, model_vertex_string);
					}
					render_debug_point(FALSE, &debug_vertex->position, 0.03125f, global_real_argb_red);
					render_debug_string_at_point(FALSE, &debug_vertex->position, temporary, global_real_argb_yellow);
				}
				else
				{
					render_debug_point(FALSE, &debug_vertex->position, 0.03125f, global_real_argb_white);
				}
			}
		}
	}
	return;
}

/* ---------- private code */
