/*
RASTERIZER_CONSOLE_VARS.H

Rasterizer debug options (the rasterizer_* script and console globals).

Header ownership is INFERRED: every HCEX (2011) compiland that corresponds to a
January user of these globals lists rasterizer_console_vars.h, and no
first-party build lists a rasterizer_debug_options.h.
Type and member names are LATER-BUILD-ATTESTED: the Halo PC demo PDB (2003) and
the HCEX PDB (2011) agree on every member name; the type name follows the demo
(HCEX spells it rasterizer_debug_options_struct).
Offsets, widths and types are January's: January's script-global table registers
zbias as a long integer (the later PDBs say float); January has pad3_scale at
0x6C and f at 0x70, where the later builds insert three shorts; the transparent
pixel counter bytes at 0x88 exist only in January. Bytes no PDB names keep
descriptive names.
*/

#ifndef __RASTERIZER_CONSOLE_VARS_H
#define __RASTERIZER_CONSOLE_VARS_H
#pragma once

/* ---------- headers */

#include "cseries.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct rasterizer_debug_options
{
	boolean fps_accumulation; /* 0x00 */
	byte pad01; /* 0x01 */
	short statistics_mode; /* 0x02 */
	short drawing_mode; /* 0x04 */
	boolean wireframe_enabled; /* 0x06 */
	boolean debug_model_vertices_enabled; /* 0x07 */
	short debug_model_lod; /* 0x08 */
	boolean debug_transparent_geometry_enabled; /* 0x0A */
	boolean debug_meter_shader_enabled; /* 0x0B */
	boolean draw_models; /* 0x0C */
	boolean draw_model_transparent_geometry; /* 0x0D */
	boolean draw_first_person_weapon_first; /* 0x0E */
	boolean stencil_mask_enabled; /* 0x0F */
	byte draw_environment; /* 0x10 */
	boolean draw_environment_lightmaps; /* 0x11 */
	boolean draw_environment_shadows; /* 0x12 */
	boolean draw_environment_diffuse_lights; /* 0x13 */
	boolean draw_environment_textures; /* 0x14 */
	boolean draw_environment_decals; /* 0x15 */
	boolean draw_environment_specular_lights; /* 0x16 */
	boolean draw_environment_specular_lightmaps; /* 0x17 */
	boolean draw_environment_reflection_lightmap_masks; /* 0x18 */
	boolean draw_environment_reflection_mirrors; /* 0x19 */
	boolean draw_environment_reflections; /* 0x1A */
	boolean draw_environment_transparent_geometry; /* 0x1B */
	boolean draw_environment_fog; /* 0x1C */
	boolean draw_environment_fog_screen; /* 0x1D */
	boolean draw_water; /* 0x1E */
	boolean draw_lens_flares; /* 0x1F */
	boolean draw_dynamic_unlit_geometry; /* 0x20 */
	boolean draw_dynamic_lit_geometry; /* 0x21 */
	boolean draw_dynamic_screen_geometry; /* 0x22 */
	boolean draw_hud_motion_sensor; /* 0x23 */
	boolean draw_detail_objects; /* 0x24 */
	boolean draw_debug_geometry; /* 0x25 */
	boolean debug_geometry_multipass; /* 0x26 */
	boolean fog_atmospheric_enabled; /* 0x27 */
	boolean fog_planar_enabled; /* 0x28 */
	boolean bump_mapping_enabled; /* 0x29 */
	byte pad2A[2]; /* 0x2A */
	real lightmap_ambient; /* 0x2C */
	short _lightmap_mode; /* 0x30 */
	short pad3; /* 0x32 */
	boolean lightmap_incident_radiosity_enabled; /* 0x34 */
	boolean lightmap_filtering_enabled; /* 0x35 */
	byte pad36[2]; /* 0x36 */
	real model_lighting_ambient; /* 0x38 */
	boolean environment_alpha_testing_enabled; /* 0x3C */
	boolean environment_specular_mask_enabled; /* 0x3D */
	boolean shadow_convolution_enabled; /* 0x3E */
	boolean shadow_debug_enabled; /* 0x3F */
	boolean water_mipmapping_enabled; /* 0x40 */
	boolean active_camouflage_enabled; /* 0x41 */
	boolean active_camouflage_multipass_enabled; /* 0x42 */
	boolean plasma_energy_enabled; /* 0x43 */
	boolean lens_flare_occlusion_enabled; /* 0x44 */
	boolean lens_flare_occlusion_debug; /* 0x45 */
	boolean lens_flare_sun_glow_enabled; /* 0x46 */
	boolean screen_flash_enabled; /* 0x47 */
	boolean screen_effects_enabled; /* 0x48 */
	boolean DXTC_noise_enabled; /* 0x49 */
	boolean soft_filter_enabled; /* 0x4A */
	boolean secondary_render_target_debug_enabled; /* 0x4B */
	boolean profile_log_enabled; /* 0x4C */
	byte pad4D[3]; /* 0x4D */
	real detail_object_screen_facing_offset_multiplier; /* 0x50 */
	long zbias; /* 0x54 */
	real zoffset; /* 0x58 */
	boolean force_all_player_views_to_default_player; /* 0x5C */
	boolean safe_frame_bounds_adjust_enabled; /* 0x5D */
	short freeze_flying_camera; /* 0x5E */
	boolean zsprite_enabled; /* 0x60 */
	boolean filthy_decal_fog_hack_enabled; /* 0x61 */
	boolean smart_states_enabled; /* 0x62 */
	boolean splitscreen_VB_optimization_enabled; /* 0x63 */
	boolean profile_print_locks; /* 0x64 */
	byte pad65[3]; /* 0x65 */
	real profile_objectlock_time; /* 0x68 */
	real pad3_scale; /* 0x6C */
	real f[6]; /* 0x70 */
	boolean transparent_pixel_counter_active; /* 0x88 */
	boolean transparent_pixel_counter; /* 0x89 */
	byte pad8A[2]; /* 0x8A */
};

/* ---------- prototypes/EXAMPLE.C */

/* ---------- globals */

extern struct rasterizer_debug_options rasterizer_debug_options;
extern boolean debug_render_freeze;

/* ---------- public code */

#endif // __RASTERIZER_CONSOLE_VARS_H
