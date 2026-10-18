/* Generated from tag_schema_render.c; do not edit. */
#ifndef HALO_MAC_NATIVE_SHADER_LAYOUT_H
#define HALO_MAC_NATIVE_SHADER_LAYOUT_H
#include "shaders/shader_definitions.h"
struct shader_layer
{
	struct tag_reference shader;
};
struct shader_transparent
{
	byte numeric_counter_limit;
	byte flags;
	short first_map_type;
	short framebuffer_blend_function;
	short framebuffer_fade_mode;
	short framebuffer_fade_source;
	short pad32;
	real lens_flare_spacing;
	struct tag_reference lens_flare;
	struct tag_block extra_layers;
	struct tag_block maps;
};
struct shader_transparent_chicago_map
{
	word flags;
	byte reserved02[42];
	short color_function;
	short alpha_function;
	byte reserved30[36];
	real map_u_scale;
	real map_v_scale;
	real map_u_offset;
	real map_v_offset;
	real map_rotation;
	real mipmap_bias;
	struct tag_reference map;
	byte reserved7C[40];
	struct shader_texture_animation map_animation;
};
struct shader_transparent_chicago_definition
{
	struct shader shader;
	struct shader_transparent transparent;
	unsigned long extra_flags;
	byte reserved64[8];
};
struct shader_transparent_chicago_extended_definition
{
	struct shader shader;
	struct shader_transparent transparent;
	struct tag_block two_stage_maps;
	unsigned long extra_flags;
	byte reserved70[8];
};
struct shader_transparent_glass_definition
{
	struct shader shader;
	word flags;
	short pad2A;
	byte reserved2C[40];
	real_rgb_color tint_color;
	real tint_map_scale;
	struct tag_reference tint_map;
	byte reserved74[20];
	word reflection_flags;
	short reflection_type;
	real_argb_color reflection_view_perpendicular_color;
	real_argb_color reflection_view_parallel_color;
	struct tag_reference reflection_map;
	real reflection_bump_map_scale;
	struct tag_reference reflection_bump_map;
	byte reservedD0[128];
	word diffuse_flags;
	word pad152;
	real diffuse_map_scale;
	struct tag_reference diffuse_map;
	real diffuse_detail_map_scale;
	struct tag_reference diffuse_detail_map;
	byte reserved17C[100];
};
struct shader_transparent_meter_definition
{
	struct shader shader;
	word flags;
	short pad2A;
	byte reserved_before_map[32];
	struct tag_reference map;
	byte reserved_before_gradient_min_color[32];
	real_rgb_color gradient_min_color;
	real_rgb_color gradient_max_color;
	real_rgb_color background_color;
	real_rgb_color flash_color;
	real_rgb_color tint_color;
	real meter_transparency;
	real background_transparency;
	byte reserved_before_meter_brightness_source[24];
	short meter_brightness_source;
	short flash_brightness_source;
	short value_source;
	short gradient_source;
	short flash_extension_source;
	word padE2;
	byte reserved_after_flash_extension_source[32];
};
struct shader_environment_definition
{
	struct shader shader;
	word flags;
	short type;
	real lens_flare_spacing;
	struct tag_reference lens_flare;
	byte reserved40[0x2C];
	/* diffuse */
	word diffuse_flags;
	short pad6E;
	byte reserved70[0x18];
	struct tag_reference base_map;
	byte reserved98[0x18];
	short detail_map_function;
	short padB2;
	real primary_detail_map_scale;
	struct tag_reference primary_detail_map;
	real secondary_detail_map_scale;
	struct tag_reference secondary_detail_map;
	byte reservedDC[0x18];
	short micro_detail_map_function;
	short padF6;
	real micro_detail_map_scale;
	struct tag_reference micro_detail_map;
	real_rgb_color material_color;
	byte reserved118[0xC];
	real bump_map_scale;
	struct tag_reference bump_map;
	real_vector2d runtime_bump_map_scale;
	byte reserved140[0x10];
	short u_animation_function;
	short pad152;
	real u_animation_period;
	real u_animation_scale;
	short v_animation_function;
	short pad15E;
	real v_animation_period;
	real v_animation_scale;
	byte reserved168[0x18];
	/* self-illumination */
	word self_illumination_flags;
	short pad182;
	byte reserved184[0x18];
	real_rgb_color primary_on_color;
	real_rgb_color primary_off_color;
	short primary_animation_function;
	short pad1B6;
	real primary_animation_period;
	real primary_animation_phase;
	byte reserved1C0[0x18];
	real_rgb_color secondary_on_color;
	real_rgb_color secondary_off_color;
	short secondary_animation_function;
	short pad1F2;
	real secondary_animation_period;
	real secondary_animation_phase;
	byte reserved1FC[0x18];
	real_rgb_color plasma_on_color;
	real_rgb_color plasma_off_color;
	short plasma_animation_function;
	short pad22E;
	real plasma_animation_period;
	real plasma_animation_phase;
	byte reserved238[0x18];
	real self_illumination_map_scale;
	struct tag_reference self_illumination_map;
	byte reserved264[0x18];
	/* specular */
	word specular_flags;
	short specular_type;
	byte reserved280[0x10];
	real specular_brightness;
	byte reserved294[0x14];
	real_rgb_color specular_view_perpendicular_color;
	real_rgb_color specular_view_parallel_color;
	byte reserved2C0[0x10];
	/* reflection */
	word reflection_flags;
	short reflection_type;
	real reflection_lightmap_brightness_scale;
	byte reserved2D8[0x1C];
	real reflection_view_perpendicular_brightness;
	real reflection_view_parallel_brightness;
	byte reserved2FC[0x28];
	struct tag_reference reflection_cube_map;
	byte reserved334[0x10];
};
struct shader_model_definition
{
	struct shader shader;
	word flags;
	short type;
	byte reserved2C[0xC];
	real translucency;
	byte reserved3C[0x10];
	short change_color_source;
	byte reserved4E[0x1E];
	word self_illumination_flags;
	short pad6E;
	short self_illumination_color_source;
	short self_illumination_animation_function;
	real self_illumination_animation_period;
	real_rgb_color self_illumination_color_lower_bound;
	real_rgb_color self_illumination_color_upper_bound;
	byte reserved90[0xC];
	real map_u_scale;
	real map_v_scale;
	struct tag_reference base_map;
	byte reservedB4[0x8];
	struct tag_reference multipurpose_map;
	byte reservedCC[0x8];
	short detail_function;
	short detail_mask;
	real detail_map_scale;
	struct tag_reference detail_map;
	real detail_map_v_scale;
	byte reservedF0[0xC];
	struct shader_texture_animation texture_animation;
	byte reserved134[0x8];
	real reflection_falloff_distance;
	real reflection_cutoff_distance;
	real perpendicular_brightness;
	real_rgb_color perpendicular_tint_color;
	real parallel_brightness;
	real_rgb_color parallel_tint_color;
	struct tag_reference reflection_cube_map;
	byte reserved174[0x44];
};
typedef char native_environment_shader_size[sizeof(struct shader_environment_definition)==0x344?1:-1];
typedef char native_environment_shader_base_map[offsetof(struct shader_environment_definition,base_map)==0x88?1:-1];
/* Offsets follow the nested source views in rasterizer_xbox_environment.c. */
typedef char native_environment_specular_brightness[offsetof(struct shader_environment_definition,specular_brightness)==0x290?1:-1];
typedef char native_environment_specular_perpendicular_color[offsetof(struct shader_environment_definition,specular_view_perpendicular_color)==0x2A8?1:-1];
typedef char native_environment_specular_parallel_color[offsetof(struct shader_environment_definition,specular_view_parallel_color)==0x2B4?1:-1];
typedef char native_environment_reflection_lightmap_scale[offsetof(struct shader_environment_definition,reflection_lightmap_brightness_scale)==0x2D4?1:-1];
typedef char native_environment_reflection_perpendicular_brightness[offsetof(struct shader_environment_definition,reflection_view_perpendicular_brightness)==0x2F4?1:-1];
typedef char native_environment_reflection_parallel_brightness[offsetof(struct shader_environment_definition,reflection_view_parallel_brightness)==0x2F8?1:-1];
typedef char native_chicago_shader_size[sizeof(struct shader_transparent_chicago_definition)==0x6c?1:-1];
typedef char native_chicago_extended_size[sizeof(struct shader_transparent_chicago_extended_definition)==0x78?1:-1];
typedef char native_glass_shader_size[sizeof(struct shader_transparent_glass_definition)==0x1E0?1:-1];
/* shader_transparent_water (swat): rasterizer_xbox_water.c's own layout,
   with the tag's trailing 16 bytes (tag_schema_render.c), which that file's
   copy leaves off. Its reserved spans are not read by the game. */
struct water_ripple
{
	byte reserved00[0x4];
	real contibution_factor;
	byte reserved08[0x20];
	real animation_angle;
	real animation_velocity;
	real_point2d map_offset;
	short map_repeats;
	short map_index;
	byte reserved3C[0x10];
};
struct shader_transparent_water_definition
{
	struct shader shader;
	unsigned short flags;
	short type;
	byte reserved2C[0x20];
	struct tag_reference base_map;
	byte reserved5C[0x10];
	real_argb_color view_perpendicular_tint_color;
	real_argb_color view_parallel_tint_color;
	byte reserved8C[0x10];
	struct tag_reference reflection_map;
	byte reservedAC[0x10];
	real ripple_animation_angle;
	real ripple_animation_velocity;
	real ripple_scale;
	struct tag_reference ripple_maps;
	short ripple_mipmap_levels;
	short pad0DA;
	real ripple_mipmap_fade_factor;
	real ripple_mipmap_lod_bias;
	byte reserved0E4[0x40];
	struct tag_block ripples;
	byte reserved130[0x10];
};
typedef char native_water_ripple_size[sizeof(struct water_ripple)==0x4C?1:-1];
typedef char native_water_shader_size[sizeof(struct shader_transparent_water_definition)==0x140?1:-1];
/* plasma (spla): the whole tag, as tag_schema_render.c gives it; the shield
   shimmer (an object's modifier shader) and plasma effects */
struct shader_transparent_plasma_definition
{
	struct shader shader;
	byte reserved28[4];
	short intensity_exponent_source;
	short pad2E;
	real intensity_exponent;
	short offset_exponent_source;
	short pad36;
	real offset_amount;
	real offset_exponent;
	byte reserved40[0x20];
	real perpendicular_alpha;
	real_rgb_color perpendicular_color;
	real parallel_alpha;
	real_rgb_color parallel_color;
	short color_source;
	byte reserved82[0x3E];
	real primary_noise_map_animation_period;
	real_vector3d primary_noise_map_animation_direction;
	real primary_noise_map_scale;
	struct tag_reference primary_noise_map;
	byte reservedE4[0x24];
	real secondary_noise_map_animation_period;
	real_vector3d secondary_noise_map_animation_direction;
	real secondary_noise_map_scale;
	struct tag_reference secondary_noise_map;
	byte reserved12C[0x20];
};
typedef char native_plasma_shader_size[sizeof(struct shader_transparent_plasma_definition)==0x14C?1:-1];
typedef char native_plasma_shader_primary[offsetof(struct shader_transparent_plasma_definition,primary_noise_map)==0xD4?1:-1];
typedef char native_plasma_shader_secondary[offsetof(struct shader_transparent_plasma_definition,secondary_noise_map)==0x11C?1:-1];
typedef char native_water_shader_ripples[offsetof(struct shader_transparent_water_definition,ripples)==0x124?1:-1];
/* fog (fog ): the whole tag, as tag_schema_render.c gives it; structures.c
   and rasterizer_xbox_environment_fog.c read views of its head */
struct fog_screen_definition
{
	word flags;
	short layer_count;
	real near_distance;
	real far_distance;
	real near_density;
	real far_density;
	real start_distance_from_fog_plane;
	byte reserved18[4];
	pixel32 color;
	real rotation_multiplier;
	real strafing_multiplier;
	real zoom_multiplier;
	byte reserved2C[8];
	real map_scale;
	struct tag_reference map;
	real animation_period;
	real animation_unused;
	real wind_velocity[2];
	real wind_period[2];
	real wind_acceleration_weight;
	real wind_perpendicular_weight;
};
struct fog_definition_full
{
	word flags;
	word pad02;
	real animation_distance;
	byte unused08[0x50];
	real maximum_density;
	byte unused5C[4];
	real maximum_distance;
	byte unused64[4];
	real maximum_depth;
	byte unused6C[8];
	real plane_distance;
	real_rgb_color color;
	struct fog_screen_definition screen;
	byte reservedEC[8];
	struct tag_reference background_sound;
	struct tag_reference sound_environment;
	byte reserved114[0x78];
};
typedef char native_fog_screen_offset[offsetof(struct fog_definition_full,screen)==0x84?1:-1];
typedef char native_fog_screen_map_offset[offsetof(struct fog_definition_full,screen.map)==0xBC?1:-1];
typedef char native_fog_size[sizeof(struct fog_definition_full)==0x18C?1:-1];
typedef char native_meter_shader_size[sizeof(struct shader_transparent_meter_definition)==0x104?1:-1];
typedef char native_meter_shader_flags_offset[offsetof(struct shader_transparent_meter_definition,flags)==0x28?1:-1];
typedef char native_meter_shader_map_offset[offsetof(struct shader_transparent_meter_definition,map)==0x4C?1:-1];
typedef char native_meter_shader_gradient_min_offset[offsetof(struct shader_transparent_meter_definition,gradient_min_color)==0x7C?1:-1];
typedef char native_meter_shader_gradient_max_offset[offsetof(struct shader_transparent_meter_definition,gradient_max_color)==0x88?1:-1];
typedef char native_meter_shader_background_offset[offsetof(struct shader_transparent_meter_definition,background_color)==0x94?1:-1];
typedef char native_meter_shader_flash_offset[offsetof(struct shader_transparent_meter_definition,flash_color)==0xA0?1:-1];
typedef char native_meter_shader_tint_offset[offsetof(struct shader_transparent_meter_definition,tint_color)==0xAC?1:-1];
typedef char native_meter_shader_meter_alpha_offset[offsetof(struct shader_transparent_meter_definition,meter_transparency)==0xB8?1:-1];
typedef char native_meter_shader_background_alpha_offset[offsetof(struct shader_transparent_meter_definition,background_transparency)==0xBC?1:-1];
typedef char native_meter_shader_brightness_source_offset[offsetof(struct shader_transparent_meter_definition,meter_brightness_source)==0xD8?1:-1];
typedef char native_meter_shader_flash_source_offset[offsetof(struct shader_transparent_meter_definition,flash_brightness_source)==0xDA?1:-1];
typedef char native_meter_shader_value_source_offset[offsetof(struct shader_transparent_meter_definition,value_source)==0xDC?1:-1];
typedef char native_meter_shader_gradient_source_offset[offsetof(struct shader_transparent_meter_definition,gradient_source)==0xDE?1:-1];
typedef char native_meter_shader_flash_extension_source_offset[offsetof(struct shader_transparent_meter_definition,flash_extension_source)==0xE0?1:-1];
typedef char native_model_shader_size[sizeof(struct shader_model_definition)==0x1B8?1:-1];
typedef char native_model_shader_base_map[offsetof(struct shader_model_definition,base_map)==0xA4?1:-1];
#endif
