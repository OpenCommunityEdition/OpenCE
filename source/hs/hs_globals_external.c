/* ---------- headers */

#include "cseries.h"
#include "ai/ai_debug.h"
#include "ai/ai_profile.h"
#include "effects/weather_particle_systems.h"
#include "game/cheats.h"
#include "hs.h"
#include "main/main.h"
#include "networking/network_connection.h"
#include "physics/collision_debug.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/rasterizer_console_vars.h"
#include "mcc_campaign.h" /* port: port/linux/game/mcc_campaign.c */

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

typedef char verify_hs_external_global_definition_size[
	sizeof(struct hs_external_global_definition) == 0xC ? 1 : -1];

/* ---------- prototypes */

/* ---------- globals */

short const hs_external_global_count = 443;

extern boolean allow_out_of_sync;
extern boolean breakable_surface_effect_enabled;
extern boolean collision_debug;
extern boolean collision_debug_features;
extern boolean collision_debug_flag_back_facing_surfaces;
extern boolean collision_debug_flag_front_facing_surfaces;
extern boolean collision_debug_flag_ignore_breakable_surfaces;
extern boolean collision_debug_flag_ignore_invisible_surfaces;
extern boolean collision_debug_flag_ignore_two_sided_surfaces;
extern boolean collision_debug_flag_media;
extern boolean collision_debug_flag_objects;
extern boolean collision_debug_flag_objects_bipeds;
extern boolean collision_debug_flag_objects_controls;
extern boolean collision_debug_flag_objects_equipment;
extern boolean collision_debug_flag_objects_light_fixtures;
extern boolean collision_debug_flag_objects_machines;
extern boolean collision_debug_flag_objects_placeholders;
extern boolean collision_debug_flag_objects_projectiles;
extern boolean collision_debug_flag_objects_scenery;
extern boolean collision_debug_flag_objects_vehicles;
extern boolean collision_debug_flag_objects_weapons;
extern boolean collision_debug_flag_skip_passthrough_bipeds;
extern boolean collision_debug_flag_structure;
extern boolean collision_debug_flag_try_to_keep_location_valid;
extern boolean collision_debug_flag_use_vehicle_physics;
extern boolean collision_debug_phantom_bsp;
extern boolean collision_debug_spray;
extern boolean collision_log_detailed;
extern boolean collision_log_extended;
extern boolean collision_log_render_enable;
extern boolean collision_log_time;
extern boolean collision_log_totals_only;
extern boolean console_dump_to_file;
extern boolean controls_enable_crouch;
extern boolean controls_enable_doubled_spin;
extern boolean controls_swap_doubled_spin_state;
extern boolean controls_swapped;
extern boolean debug_bink;
extern boolean debug_biped_limp_body_disable;
extern boolean debug_biped_physics;
extern boolean debug_biped_skip_collision;
extern boolean debug_biped_skip_update;
extern boolean debug_bsp;
extern boolean debug_camera;
extern boolean debug_collision_skip_objects;
extern boolean debug_collision_skip_vectors;
extern boolean debug_damage;
extern boolean debug_damage_taken;
extern boolean debug_decals;
extern boolean debug_detail_objects;
extern boolean debug_effects_nonviolent;
extern boolean debug_fog_planes;
extern boolean debug_inactive_objects;
extern boolean debug_input;
extern short debug_input_target;
extern long debug_leaf_index;
extern long debug_leaf_portal_index;
extern boolean debug_leaf_portals;
extern boolean debug_lights;
extern boolean debug_looping_sound;
extern boolean debug_material_effects;
extern boolean debug_motion_sensor_draw_all_units;
extern boolean debug_no_frustum_clip;
extern boolean debug_object_lights;
extern boolean debug_objects;
extern boolean debug_objects_biped_autoaim_pills;
extern boolean debug_objects_biped_physics_pills;
extern boolean debug_objects_devices;
extern boolean debug_objects_unit_mouth_apeture;
extern boolean debug_objects_unit_seats;
extern boolean debug_objects_unit_vectors;
extern boolean debug_objects_vehicle_powered_mass_points;
extern boolean debug_permanent_decals;
extern boolean debug_physics_disable_penetration_freeze;
extern boolean debug_player;
extern short debug_player_color;
extern boolean debug_point_physics;
extern boolean debug_portals;
extern boolean debug_recording;
extern short debug_recording_newlines;
extern boolean debug_render_player_teleport;
extern boolean debug_scripting;
extern boolean debug_sound;
extern boolean debug_sound_cache;
extern boolean debug_sound_channels;
extern boolean debug_sound_environment;
extern boolean debug_sprites;
extern boolean debug_structure;
extern boolean debug_texture_cache;
extern boolean debug_trigger_volumes;
extern boolean debug_unit_all_animations;
extern boolean debug_unit_animations;
extern boolean debug_unit_illumination;
extern boolean decals_enabled;
extern boolean director_camera_switch_fast;
extern boolean effects_corpse_nonviolent;
extern boolean find_all_fucked_up_shit;
extern short global_screenshot_count;
extern long hs_model_animation_bullshit[4];
extern boolean hs_model_animation_compression_enabled;
extern long hs_model_animation_data_compressed_size;
extern long hs_model_animation_data_compression_savings_in_bytes;
extern long hs_model_animation_data_compression_savings_in_bytes_at_import;
extern real hs_model_animation_data_compression_savings_in_percent;
extern long hs_model_animation_data_uncompressed_size;
extern boolean loud_dialog_hack;
extern real object_light_ambient_base;
extern real object_light_ambient_scale;
extern boolean object_light_interpolate;
extern real object_light_secondary_scale;
extern boolean player_autoaim_flag;
extern real player_look_pitch_rate[MAXIMUM_NUMBER_OF_LOCAL_PLAYERS];
extern real player_look_yaw_rate[MAXIMUM_NUMBER_OF_LOCAL_PLAYERS];
extern boolean player_magnetism_flag;
extern boolean profile_display;
extern boolean profile_dump_frames;
extern boolean profile_dump_lost_frames;
extern boolean profile_graph;
extern boolean profile_timebase_ticks;
extern boolean recover_saved_games_hack;
extern boolean render_camera_debug_this_fucking_frustum;
extern boolean render_contrails_enabled;
extern boolean render_model_index_counts;
extern boolean render_model_markers;
extern boolean render_model_no_geometry;
extern boolean render_model_nodes;
extern boolean render_model_vertex_counts;
extern boolean render_particle_systems_enabled;
extern boolean render_particles_enabled;
extern boolean render_shadows;
extern boolean render_weather_particle_systems_enabled;
extern boolean rider_ejection;
extern real sound_gain_under_dialog;
extern boolean structures_use_pvs_for_vs;
extern boolean stun_enable;
extern boolean temporary_hud;
extern boolean terminal_render_enable;
extern byte texture_cache_debug_options[];

static struct hs_external_global_definition screenshot_size_definition = { "screenshot_size", _hs_type_short_integer, 0, &global_screenshot_size };

static struct hs_external_global_definition screenshot_count_definition = { "screenshot_count", _hs_type_short_integer, 0, &global_screenshot_count };

static struct hs_external_global_definition player_spawn_count_definition = { "player_spawn_count", _hs_type_short_integer, 0, &player_spawn_count };

static struct hs_external_global_definition framerate_throttle_definition = { "framerate_throttle", _hs_type_boolean, 0, &global_frame_rate_throttle };

static struct hs_external_global_definition framerate_lock_definition = { "framerate_lock", _hs_type_boolean, 0, &debug_force_frame_rate_update };

static struct hs_external_global_definition debug_framerate_definition = { "debug_framerate", _hs_type_boolean, 0, &debug_frame_rate };

static struct hs_external_global_definition display_framerate_definition = { "display_framerate", _hs_type_boolean, 0, &display_framerate };

static struct hs_external_global_definition display_vblank_deltas_definition = { "display_vblank_deltas", _hs_type_boolean, 0, &display_vblank_deltas };

static struct hs_external_global_definition display_precache_progress_definition = { "display_precache_progress", _hs_type_boolean, 0, &display_precache_progress };

static struct hs_external_global_definition debug_game_save_definition = { "debug_game_save", _hs_type_boolean, 0, &debug_game_save };

static struct hs_external_global_definition terminal_render_definition = { "terminal_render", _hs_type_boolean, 0, &terminal_render_enable };

static struct hs_external_global_definition console_dump_to_file_definition = { "console_dump_to_file", _hs_type_boolean, 0, &console_dump_to_file };

static struct hs_external_global_definition rasterizer_near_clip_distance_definition = { "rasterizer_near_clip_distance", _hs_type_real, 0, &rasterizer_globals.near_clip_distance };

static struct hs_external_global_definition rasterizer_far_clip_distance_definition = { "rasterizer_far_clip_distance", _hs_type_real, 0, &rasterizer_globals.far_clip_distance };

static struct hs_external_global_definition rasterizer_first_person_weapon_near_clip_distance_definition = { "rasterizer_first_person_weapon_near_clip_distance", _hs_type_real, 0, &rasterizer_globals.first_person_weapon_near_clip_distance };

static struct hs_external_global_definition rasterizer_first_person_weapon_far_clip_distance_definition = { "rasterizer_first_person_weapon_far_clip_distance", _hs_type_real, 0, &rasterizer_globals.first_person_weapon_far_clip_distance };

static struct hs_external_global_definition rasterizer_pushbuffer_size_definition = { "rasterizer_pushbuffer_size", _hs_type_short_integer, 0, &rasterizer_globals.push_buffer_size };

static struct hs_external_global_definition rasterizer_pushbuffer_kickoff_size_definition = { "rasterizer_pushbuffer_kickoff_size", _hs_type_short_integer, 0, &rasterizer_globals.kick_off_size };

static struct hs_external_global_definition rasterizer_floating_point_zbuffer_definition = { "rasterizer_floating_point_zbuffer", _hs_type_boolean, 0, &rasterizer_globals.floating_point_zbuffer };

static struct hs_external_global_definition rasterizer_framerate_throttle_definition = { "rasterizer_framerate_throttle", _hs_type_boolean, 0, &rasterizer_globals.framerate_throttle };

static struct hs_external_global_definition rasterizer_framerate_stabilization_definition = { "rasterizer_framerate_stabilization", _hs_type_boolean, 0, &rasterizer_globals.framerate_throttle_debug };

static struct hs_external_global_definition rasterizer_refresh_rate_definition = { "rasterizer_refresh_rate", _hs_type_short_integer, 0, &rasterizer_globals.framerate_throttle_target };

static struct hs_external_global_definition rasterizer_frame_bounds_left_definition = { "rasterizer_frame_bounds_left", _hs_type_short_integer, 0, &rasterizer_globals.reserved04.frame_bounds.x0 };

static struct hs_external_global_definition rasterizer_frame_bounds_right_definition = { "rasterizer_frame_bounds_right", _hs_type_short_integer, 0, &rasterizer_globals.reserved04.frame_bounds.x1 };

static struct hs_external_global_definition rasterizer_frame_bounds_top_definition = { "rasterizer_frame_bounds_top", _hs_type_short_integer, 0, &rasterizer_globals.reserved04.frame_bounds.y0 };

static struct hs_external_global_definition rasterizer_frame_bounds_bottom_definition = { "rasterizer_frame_bounds_bottom", _hs_type_short_integer, 0, &rasterizer_globals.reserved04.frame_bounds.y1 };

static struct hs_external_global_definition rasterizer_stats_definition = { "rasterizer_stats", _hs_type_short_integer, 0, &rasterizer_debug_options.statistics_mode };

static struct hs_external_global_definition rasterizer_mode_definition = { "rasterizer_mode", _hs_type_short_integer, 0, &rasterizer_debug_options.drawing_mode };

static struct hs_external_global_definition rasterizer_wireframe_definition = { "rasterizer_wireframe", _hs_type_boolean, 0, &rasterizer_debug_options.wireframe_enabled };

static struct hs_external_global_definition rasterizer_smart_definition = { "rasterizer_smart", _hs_type_boolean, 0, &rasterizer_debug_options.smart_states_enabled };

static struct hs_external_global_definition rasterizer_debug_model_vertices_definition = { "rasterizer_debug_model_vertices", _hs_type_boolean, 0, &rasterizer_debug_options.debug_model_vertices_enabled };

static struct hs_external_global_definition rasterizer_debug_model_lod_definition = { "rasterizer_debug_model_lod", _hs_type_short_integer, 0, &rasterizer_debug_options.debug_model_lod };

static struct hs_external_global_definition rasterizer_debug_transparents_definition = { "rasterizer_debug_transparents", _hs_type_boolean, 0, &rasterizer_debug_options.debug_transparent_geometry_enabled };

static struct hs_external_global_definition rasterizer_debug_meter_shader_definition = { "rasterizer_debug_meter_shader", _hs_type_boolean, 0, &rasterizer_debug_options.debug_meter_shader_enabled };

static struct hs_external_global_definition rasterizer_models_definition = { "rasterizer_models", _hs_type_boolean, 0, &rasterizer_debug_options.draw_models };

static struct hs_external_global_definition rasterizer_model_transparents_definition = { "rasterizer_model_transparents", _hs_type_boolean, 0, &rasterizer_debug_options.draw_model_transparent_geometry };

static struct hs_external_global_definition rasterizer_draw_first_person_weapon_first_definition = { "rasterizer_draw_first_person_weapon_first", _hs_type_boolean, 0, &rasterizer_debug_options.draw_first_person_weapon_first };

static struct hs_external_global_definition rasterizer_stencil_mask_definition = { "rasterizer_stencil_mask", _hs_type_boolean, 0, &rasterizer_debug_options.stencil_mask_enabled };

static struct hs_external_global_definition rasterizer_environment_definition = { "rasterizer_environment", _hs_type_boolean, 0, &rasterizer_debug_options.draw_environment };

static struct hs_external_global_definition rasterizer_environment_lightmaps_definition = { "rasterizer_environment_lightmaps", _hs_type_boolean, 0, &rasterizer_debug_options.draw_environment_lightmaps };

static struct hs_external_global_definition rasterizer_environment_shadows_definition = { "rasterizer_environment_shadows", _hs_type_boolean, 0, &rasterizer_debug_options.draw_environment_shadows };

static struct hs_external_global_definition rasterizer_environment_diffuse_lights_definition = { "rasterizer_environment_diffuse_lights", _hs_type_boolean, 0, &rasterizer_debug_options.draw_environment_diffuse_lights };

static struct hs_external_global_definition rasterizer_environment_diffuse_textures_definition = { "rasterizer_environment_diffuse_textures", _hs_type_boolean, 0, &rasterizer_debug_options.draw_environment_textures };

static struct hs_external_global_definition rasterizer_environment_decals_definition = { "rasterizer_environment_decals", _hs_type_boolean, 0, &rasterizer_debug_options.draw_environment_decals };

static struct hs_external_global_definition rasterizer_environment_specular_lights_definition = { "rasterizer_environment_specular_lights", _hs_type_boolean, 0, &rasterizer_debug_options.draw_environment_specular_lights };

static struct hs_external_global_definition rasterizer_environment_specular_lightmaps_definition = { "rasterizer_environment_specular_lightmaps", _hs_type_boolean, 0, &rasterizer_debug_options.draw_environment_specular_lightmaps };

static struct hs_external_global_definition rasterizer_environment_reflection_lightmap_mask_definition = { "rasterizer_environment_reflection_lightmap_mask", _hs_type_boolean, 0, &rasterizer_debug_options.draw_environment_reflection_lightmap_masks };

static struct hs_external_global_definition rasterizer_environment_reflection_mirrors_definition = { "rasterizer_environment_reflection_mirrors", _hs_type_boolean, 0, &rasterizer_debug_options.draw_environment_reflection_mirrors };

static struct hs_external_global_definition rasterizer_environment_reflections_definition = { "rasterizer_environment_reflections", _hs_type_boolean, 0, &rasterizer_debug_options.draw_environment_reflections };

static struct hs_external_global_definition rasterizer_environment_transparents_definition = { "rasterizer_environment_transparents", _hs_type_boolean, 0, &rasterizer_debug_options.draw_environment_transparent_geometry };

static struct hs_external_global_definition rasterizer_environment_fog_definition = { "rasterizer_environment_fog", _hs_type_boolean, 0, &rasterizer_debug_options.draw_environment_fog };

static struct hs_external_global_definition rasterizer_environment_fog_screen_definition = { "rasterizer_environment_fog_screen", _hs_type_boolean, 0, &rasterizer_debug_options.draw_environment_fog_screen };

static struct hs_external_global_definition rasterizer_water_definition = { "rasterizer_water", _hs_type_boolean, 0, &rasterizer_debug_options.draw_water };

static struct hs_external_global_definition rasterizer_lens_flares_definition = { "rasterizer_lens_flares", _hs_type_boolean, 0, &rasterizer_debug_options.draw_lens_flares };

static struct hs_external_global_definition rasterizer_dynamic_unlit_geometry_definition = { "rasterizer_dynamic_unlit_geometry", _hs_type_boolean, 0, &rasterizer_debug_options.draw_dynamic_unlit_geometry };

static struct hs_external_global_definition rasterizer_dynamic_lit_geometry_definition = { "rasterizer_dynamic_lit_geometry", _hs_type_boolean, 0, &rasterizer_debug_options.draw_dynamic_lit_geometry };

static struct hs_external_global_definition rasterizer_dynamic_screen_geometry_definition = { "rasterizer_dynamic_screen_geometry", _hs_type_boolean, 0, &rasterizer_debug_options.draw_dynamic_screen_geometry };

static struct hs_external_global_definition rasterizer_hud_motion_sensor_definition = { "rasterizer_hud_motion_sensor", _hs_type_boolean, 0, &rasterizer_debug_options.draw_hud_motion_sensor };

static struct hs_external_global_definition rasterizer_detail_objects_definition = { "rasterizer_detail_objects", _hs_type_boolean, 0, &rasterizer_debug_options.draw_detail_objects };

static struct hs_external_global_definition rasterizer_debug_geometry_definition = { "rasterizer_debug_geometry", _hs_type_boolean, 0, &rasterizer_debug_options.draw_debug_geometry };

static struct hs_external_global_definition rasterizer_debug_geometry_multipass_definition = { "rasterizer_debug_geometry_multipass", _hs_type_boolean, 0, &rasterizer_debug_options.debug_geometry_multipass };

static struct hs_external_global_definition rasterizer_fog_atmosphere_definition = { "rasterizer_fog_atmosphere", _hs_type_boolean, 0, &rasterizer_debug_options.fog_atmospheric_enabled };

static struct hs_external_global_definition rasterizer_fog_plane_definition = { "rasterizer_fog_plane", _hs_type_boolean, 0, &rasterizer_debug_options.fog_planar_enabled };

static struct hs_external_global_definition rasterizer_bump_mapping_definition = { "rasterizer_bump_mapping", _hs_type_boolean, 0, &rasterizer_debug_options.bump_mapping_enabled };

static struct hs_external_global_definition rasterizer_lightmap_ambient_definition = { "rasterizer_lightmap_ambient", _hs_type_real, 0, &rasterizer_debug_options.lightmap_ambient };

static struct hs_external_global_definition rasterizer_lightmap_mode_definition = { "rasterizer_lightmap_mode", _hs_type_short_integer, 0, &rasterizer_globals.lightmap_mode };

static struct hs_external_global_definition rasterizer_lightmaps_incident_radiosity_definition = { "rasterizer_lightmaps_incident_radiosity", _hs_type_boolean, 0, &rasterizer_debug_options.lightmap_incident_radiosity_enabled };

static struct hs_external_global_definition rasterizer_lightmaps_filtering_definition = { "rasterizer_lightmaps_filtering", _hs_type_boolean, 0, &rasterizer_debug_options.lightmap_filtering_enabled };

static struct hs_external_global_definition rasterizer_model_lighting_ambient_definition = { "rasterizer_model_lighting_ambient", _hs_type_real, 0, &rasterizer_debug_options.model_lighting_ambient };

static struct hs_external_global_definition rasterizer_environment_alpha_testing_definition = { "rasterizer_environment_alpha_testing", _hs_type_boolean, 0, &rasterizer_debug_options.environment_alpha_testing_enabled };

static struct hs_external_global_definition rasterizer_environment_specular_mask_definition = { "rasterizer_environment_specular_mask", _hs_type_boolean, 0, &rasterizer_debug_options.environment_specular_mask_enabled };

static struct hs_external_global_definition rasterizer_shadows_convolution_definition = { "rasterizer_shadows_convolution", _hs_type_boolean, 0, &rasterizer_debug_options.shadow_convolution_enabled };

static struct hs_external_global_definition rasterizer_shadows_debug_definition = { "rasterizer_shadows_debug", _hs_type_boolean, 0, &rasterizer_debug_options.shadow_debug_enabled };

static struct hs_external_global_definition rasterizer_water_mipmapping_definition = { "rasterizer_water_mipmapping", _hs_type_boolean, 0, &rasterizer_debug_options.water_mipmapping_enabled };

static struct hs_external_global_definition rasterizer_active_camouflage_definition = { "rasterizer_active_camouflage", _hs_type_boolean, 0, &rasterizer_debug_options.active_camouflage_enabled };

static struct hs_external_global_definition rasterizer_active_camouflage_multipass_definition = { "rasterizer_active_camouflage_multipass", _hs_type_boolean, 0, &rasterizer_debug_options.active_camouflage_multipass_enabled };

static struct hs_external_global_definition rasterizer_plasma_energy_definition = { "rasterizer_plasma_energy", _hs_type_boolean, 0, &rasterizer_debug_options.plasma_energy_enabled };

static struct hs_external_global_definition rasterizer_lens_flares_occlusion_definition = { "rasterizer_lens_flares_occlusion", _hs_type_boolean, 0, &rasterizer_debug_options.lens_flare_occlusion_enabled };

static struct hs_external_global_definition rasterizer_lens_flares_occlusion_debug_definition = { "rasterizer_lens_flares_occlusion_debug", _hs_type_boolean, 0, &rasterizer_debug_options.lens_flare_occlusion_debug };

static struct hs_external_global_definition rasterizer_ray_of_buddha_definition = { "rasterizer_ray_of_buddha", _hs_type_boolean, 0, &rasterizer_debug_options.lens_flare_sun_glow_enabled };

static struct hs_external_global_definition rasterizer_screen_flashes_definition = { "rasterizer_screen_flashes", _hs_type_boolean, 0, &rasterizer_debug_options.screen_flash_enabled };

static struct hs_external_global_definition rasterizer_screen_effects_definition = { "rasterizer_screen_effects", _hs_type_boolean, 0, &rasterizer_debug_options.screen_effects_enabled };

static struct hs_external_global_definition rasterizer_DXTC_noise_definition = { "rasterizer_DXTC_noise", _hs_type_boolean, 0, &rasterizer_debug_options.DXTC_noise_enabled };

static struct hs_external_global_definition rasterizer_soft_filter_definition = { "rasterizer_soft_filter", _hs_type_boolean, 0, &rasterizer_debug_options.soft_filter_enabled };

static struct hs_external_global_definition rasterizer_secondary_render_target_debug_definition = { "rasterizer_secondary_render_target_debug", _hs_type_boolean, 0, &rasterizer_debug_options.secondary_render_target_debug_enabled };

static struct hs_external_global_definition rasterizer_profile_log_definition = { "rasterizer_profile_log", _hs_type_boolean, 0, &rasterizer_debug_options.profile_log_enabled };

static struct hs_external_global_definition rasterizer_detail_objects_offset_multiplier_definition = { "rasterizer_detail_objects_offset_multiplier", _hs_type_real, 0, &rasterizer_debug_options.detail_object_screen_facing_offset_multiplier };

static struct hs_external_global_definition rasterizer_zbias_definition = { "rasterizer_zbias", _hs_type_long_integer, 0, &rasterizer_debug_options.zbias };

static struct hs_external_global_definition rasterizer_zoffset_definition = { "rasterizer_zoffset", _hs_type_real, 0, &rasterizer_debug_options.zoffset };

static struct hs_external_global_definition force_all_player_views_to_default_player_definition = { "force_all_player_views_to_default_player", _hs_type_boolean, 0, &rasterizer_debug_options.force_all_player_views_to_default_player };

static struct hs_external_global_definition rasterizer_safe_frame_bounds_definition = { "rasterizer_safe_frame_bounds", _hs_type_boolean, 0, &rasterizer_debug_options.safe_frame_bounds_adjust_enabled };

static struct hs_external_global_definition freeze_flying_camera_definition = { "freeze_flying_camera", _hs_type_short_integer, 0, &rasterizer_debug_options.freeze_flying_camera };

static struct hs_external_global_definition rasterizer_zsprites_definition = { "rasterizer_zsprites", _hs_type_boolean, 0, &rasterizer_debug_options.zsprite_enabled };

static struct hs_external_global_definition rasterizer_filthy_decal_fog_hack_definition = { "rasterizer_filthy_decal_fog_hack", _hs_type_boolean, 0, &rasterizer_debug_options.filthy_decal_fog_hack_enabled };

static struct hs_external_global_definition rasterizer_splitscreen_VB_optimization_definition = { "rasterizer_splitscreen_VB_optimization", _hs_type_boolean, 0, &rasterizer_debug_options.splitscreen_VB_optimization_enabled };

static struct hs_external_global_definition rasterizer_profile_print_locks_definition = { "rasterizer_profile_print_locks", _hs_type_boolean, 0, &rasterizer_debug_options.profile_print_locks };

static struct hs_external_global_definition rasterizer_profile_objectlock_time_definition = { "rasterizer_profile_objectlock_time", _hs_type_real, 0, &rasterizer_debug_options.profile_objectlock_time };

static struct hs_external_global_definition pad3_definition = { "pad3", _hs_type_short_integer, 0, &rasterizer_debug_options.pad3 };

static struct hs_external_global_definition pad3_scale_definition = { "pad3_scale", _hs_type_real, 0, &rasterizer_debug_options.pad3_scale };

static struct hs_external_global_definition f0_definition = { "f0", _hs_type_real, 0, &rasterizer_debug_options.f[0] };

static struct hs_external_global_definition f1_definition = { "f1", _hs_type_real, 0, &rasterizer_debug_options.f[1] };

static struct hs_external_global_definition f2_definition = { "f2", _hs_type_real, 0, &rasterizer_debug_options.f[2] };

static struct hs_external_global_definition f3_definition = { "f3", _hs_type_real, 0, &rasterizer_debug_options.f[3] };

static struct hs_external_global_definition f4_definition = { "f4", _hs_type_real, 0, &rasterizer_debug_options.f[4] };

static struct hs_external_global_definition f5_definition = { "f5", _hs_type_real, 0, &rasterizer_debug_options.f[5] };

static struct hs_external_global_definition rasterizer_transparent_pixel_counter_definition = { "rasterizer_transparent_pixel_counter", _hs_type_boolean, 0, &rasterizer_debug_options.transparent_pixel_counter };

static struct hs_external_global_definition debug_no_frustum_clip_definition = { "debug_no_frustum_clip", _hs_type_boolean, 0, &debug_no_frustum_clip };

static struct hs_external_global_definition debug_frustum_definition = { "debug_frustum", _hs_type_boolean, 0, &render_camera_debug_this_fucking_frustum };

static struct hs_external_global_definition debug_bink_definition = { "debug_bink", _hs_type_boolean, 0, &debug_bink };

static struct hs_external_global_definition recover_saved_games_hack_definition = { "recover_saved_games_hack", _hs_type_boolean, 0, &recover_saved_games_hack };

static struct hs_external_global_definition radiosity_quality_definition = { "radiosity_quality", _hs_type_short_integer, 0, NULL };

static struct hs_external_global_definition radiosity_step_count_definition = { "radiosity_step_count", _hs_type_short_integer, 0, NULL };

static struct hs_external_global_definition radiosity_lines_definition = { "radiosity_lines", _hs_type_boolean, 0, NULL };

static struct hs_external_global_definition radiosity_normals_definition = { "radiosity_normals", _hs_type_boolean, 0, NULL };

static struct hs_external_global_definition structures_use_pvs_for_vs_definition = { "structures_use_pvs_for_vs", _hs_type_boolean, 0, &structures_use_pvs_for_vs };

static struct hs_external_global_definition debug_detail_objects_definition = { "debug_detail_objects", _hs_type_boolean, 0, &debug_detail_objects };

static struct hs_external_global_definition debug_texture_cache_definition = { "debug_texture_cache", _hs_type_boolean, 0, &debug_texture_cache };

static struct hs_external_global_definition temporary_hud_definition = { "temporary_hud", _hs_type_boolean, 0, &temporary_hud };

static struct hs_external_global_definition debug_object_garbage_collection_definition = { "debug_object_garbage_collection", _hs_type_boolean, 0, &debug_object_garbage_collection };

static struct hs_external_global_definition debug_render_freeze_definition = { "debug_render_freeze", _hs_type_boolean, 0, &debug_render_freeze };

static struct hs_external_global_definition debug_no_drawing_definition = { "debug_no_drawing", _hs_type_boolean, 0, &debug_no_drawing };

static struct hs_external_global_definition debug_input_target_definition = { "debug_input_target", _hs_type_short_integer, 0, &debug_input_target };

static struct hs_external_global_definition debug_leaf_index_definition = { "debug_leaf_index", _hs_type_long_integer, 0, &debug_leaf_index };

static struct hs_external_global_definition debug_leaf_portal_index_definition = { "debug_leaf_portal_index", _hs_type_long_integer, 0, &debug_leaf_portal_index };

static struct hs_external_global_definition debug_leaf_portals_definition = { "debug_leaf_portals", _hs_type_boolean, 0, &debug_leaf_portals };

static struct hs_external_global_definition debug_unit_all_animations_definition = { "debug_unit_all_animations", _hs_type_boolean, 0, &debug_unit_all_animations };

static struct hs_external_global_definition debug_unit_animations_definition = { "debug_unit_animations", _hs_type_boolean, 0, &debug_unit_animations };

static struct hs_external_global_definition debug_unit_illumination_definition = { "debug_unit_illumination", _hs_type_boolean, 0, &debug_unit_illumination };

static struct hs_external_global_definition debug_damage_taken_definition = { "debug_damage_taken", _hs_type_boolean, 0, &debug_damage_taken };

static struct hs_external_global_definition cheat_deathless_player_definition = { "cheat_deathless_player", _hs_type_boolean, 0, &cheat.deathless_player };

static struct hs_external_global_definition cheat_jetpack_definition = { "cheat_jetpack", _hs_type_boolean, 0, &cheat.jetpack };

static struct hs_external_global_definition cheat_infinite_ammo_definition = { "cheat_infinite_ammo", _hs_type_boolean, 0, &cheat.infinite_ammo };

static struct hs_external_global_definition cheat_bottomless_clip_definition = { "cheat_bottomless_clip", _hs_type_boolean, 0, &cheat.bottomless_clip };

static struct hs_external_global_definition cheat_bump_possession_definition = { "cheat_bump_possession", _hs_type_boolean, 0, &cheat.bump_possession };

static struct hs_external_global_definition cheat_super_jump_definition = { "cheat_super_jump", _hs_type_boolean, 0, &cheat.super_jump };

static struct hs_external_global_definition cheat_reflexive_damage_effects_definition = { "cheat_reflexive_damage_effects", _hs_type_boolean, 0, &cheat.reflexive_damage_effects };

static struct hs_external_global_definition cheat_medusa_definition = { "cheat_medusa", _hs_type_boolean, 0, &cheat.medusa };

static struct hs_external_global_definition cheat_omnipotent_definition = { "cheat_omnipotent", _hs_type_boolean, 0, &cheat.omnipotent };

static struct hs_external_global_definition cheat_controller_definition = { "cheat_controller", _hs_type_boolean, 0, &cheat.controller_enabled };

static struct hs_external_global_definition effects_corpse_nonviolent_definition = { "effects_corpse_nonviolent", _hs_type_boolean, 0, &effects_corpse_nonviolent };

static struct hs_external_global_definition debug_effects_nonviolent_definition = { "debug_effects_nonviolent", _hs_type_boolean, 0, &debug_effects_nonviolent };

static struct hs_external_global_definition debug_sound_cache_definition = { "debug_sound_cache", _hs_type_boolean, 0, &debug_sound_cache };

static struct hs_external_global_definition debug_sound_definition = { "debug_sound", _hs_type_boolean, 0, &debug_sound };

static struct hs_external_global_definition debug_looping_sound_definition = { "debug_looping_sound", _hs_type_boolean, 0, &debug_looping_sound };

static struct hs_external_global_definition debug_sound_channels_definition = { "debug_sound_channels", _hs_type_boolean, 0, &debug_sound_channels };

static struct hs_external_global_definition loud_dialog_hack_definition = { "loud_dialog_hack", _hs_type_boolean, 0, &loud_dialog_hack };

static struct hs_external_global_definition sound_gain_under_dialog_definition = { "sound_gain_under_dialog", _hs_type_real, 0, &sound_gain_under_dialog };

static struct hs_external_global_definition debug_sound_environment_definition = { "debug_sound_environment", _hs_type_boolean, 0, &debug_sound_environment };

static struct hs_external_global_definition object_light_ambient_base_definition = { "object_light_ambient_base", _hs_type_real, 0, &object_light_ambient_base };

static struct hs_external_global_definition object_light_ambient_scale_definition = { "object_light_ambient_scale", _hs_type_real, 0, &object_light_ambient_scale };

static struct hs_external_global_definition object_light_secondary_scale_definition = { "object_light_secondary_scale", _hs_type_real, 0, &object_light_secondary_scale };

static struct hs_external_global_definition object_light_interpolate_definition = { "object_light_interpolate", _hs_type_boolean, 0, &object_light_interpolate };

static struct hs_external_global_definition rider_ejection_definition = { "rider_ejection", _hs_type_boolean, 0, &rider_ejection };

static struct hs_external_global_definition stun_enable_definition = { "stun_enable", _hs_type_boolean, 0, &stun_enable };

static struct hs_external_global_definition collision_log_render_definition = { "collision_log_render", _hs_type_boolean, 0, &collision_log_render_enable };

static struct hs_external_global_definition collision_log_detailed_definition = { "collision_log_detailed", _hs_type_boolean, 0, &collision_log_detailed };

static struct hs_external_global_definition collision_log_extended_definition = { "collision_log_extended", _hs_type_boolean, 0, &collision_log_extended };

static struct hs_external_global_definition collision_log_totals_only_definition = { "collision_log_totals_only", _hs_type_boolean, 0, &collision_log_totals_only };

static struct hs_external_global_definition collision_log_time_definition = { "collision_log_time", _hs_type_boolean, 0, &collision_log_time };

static struct hs_external_global_definition profile_graph_definition = { "profile_graph", _hs_type_boolean, 0, &profile_graph };

static struct hs_external_global_definition profile_display_definition = { "profile_display", _hs_type_boolean, 0, &profile_display };

static struct hs_external_global_definition profile_timebase_ticks_definition = { "profile_timebase_ticks", _hs_type_boolean, 0, &profile_timebase_ticks };

static struct hs_external_global_definition profile_dump_frames_definition = { "profile_dump_frames", _hs_type_boolean, 0, &profile_dump_frames };

static struct hs_external_global_definition profile_dump_lost_frames_definition = { "profile_dump_lost_frames", _hs_type_boolean, 0, &profile_dump_lost_frames };

static struct hs_external_global_definition model_animation_compression_definition = { "model_animation_compression", _hs_type_boolean, 0, &hs_model_animation_compression_enabled };

static struct hs_external_global_definition model_animation_data_compressed_size_definition = { "model_animation_data_compressed_size", _hs_type_long_integer, 0, &hs_model_animation_data_compressed_size };

static struct hs_external_global_definition model_animation_data_uncompressed_size_definition = { "model_animation_data_uncompressed_size", _hs_type_long_integer, 0, &hs_model_animation_data_uncompressed_size };

static struct hs_external_global_definition model_animation_data_compression_savings_in_bytes_definition = { "model_animation_data_compression_savings_in_bytes", _hs_type_long_integer, 0, &hs_model_animation_data_compression_savings_in_bytes };

static struct hs_external_global_definition model_animation_data_compression_savings_in_bytes_at_import_definition = { "model_animation_data_compression_savings_in_bytes_at_import", _hs_type_long_integer, 0, &hs_model_animation_data_compression_savings_in_bytes_at_import };

static struct hs_external_global_definition model_animation_data_compression_savings_in_percent_definition = { "model_animation_data_compression_savings_in_percent", _hs_type_real, 0, &hs_model_animation_data_compression_savings_in_percent };

static struct hs_external_global_definition model_animation_bullshit0_definition = { "model_animation_bullshit0", _hs_type_long_integer, 0, &hs_model_animation_bullshit[0] };

static struct hs_external_global_definition model_animation_bullshit1_definition = { "model_animation_bullshit1", _hs_type_long_integer, 0, &hs_model_animation_bullshit[1] };

static struct hs_external_global_definition model_animation_bullshit2_definition = { "model_animation_bullshit2", _hs_type_long_integer, 0, &hs_model_animation_bullshit[2] };

static struct hs_external_global_definition model_animation_bullshit3_definition = { "model_animation_bullshit3", _hs_type_long_integer, 0, &hs_model_animation_bullshit[3] };

static struct hs_external_global_definition debug_portals_definition = { "debug_portals", _hs_type_boolean, 0, &debug_portals };

static struct hs_external_global_definition debug_sprites_definition = { "debug_sprites", _hs_type_boolean, 0, &debug_sprites };

static struct hs_external_global_definition debug_inactive_objects_definition = { "debug_inactive_objects", _hs_type_boolean, 0, &debug_inactive_objects };

static struct hs_external_global_definition render_contrails_definition = { "render_contrails", _hs_type_boolean, 0, &render_contrails_enabled };

static struct hs_external_global_definition render_particles_definition = { "render_particles", _hs_type_boolean, 0, &render_particles_enabled };

static struct hs_external_global_definition render_psystems_definition = { "render_psystems", _hs_type_boolean, 0, &render_particle_systems_enabled };

static struct hs_external_global_definition render_wsystems_definition = { "render_wsystems", _hs_type_boolean, 0, &render_weather_particle_systems_enabled };

static struct hs_external_global_definition debug_objects_definition = { "debug_objects", _hs_type_boolean, 0, &debug_objects };

static struct hs_external_global_definition debug_objects_position_velocity_definition = { "debug_objects_position_velocity", _hs_type_boolean, 0, &debug_objects_position_velocity };

static struct hs_external_global_definition debug_objects_root_node_definition = { "debug_objects_root_node", _hs_type_boolean, 0, &debug_objects_root_node };

static struct hs_external_global_definition debug_objects_bounding_spheres_definition = { "debug_objects_bounding_spheres", _hs_type_boolean, 0, &debug_objects_bounding_spheres };

static struct hs_external_global_definition debug_objects_collision_models_definition = { "debug_objects_collision_models", _hs_type_boolean, 0, &debug_objects_collision_models };

static struct hs_external_global_definition debug_objects_physics_definition = { "debug_objects_physics", _hs_type_boolean, 0, &debug_objects_physics };

static struct hs_external_global_definition debug_objects_names_definition = { "debug_objects_names", _hs_type_boolean, 0, &debug_objects_names };

static struct hs_external_global_definition debug_objects_pathfinding_spheres_definition = { "debug_objects_pathfinding_spheres", _hs_type_boolean, 0, &debug_objects_pathfinding_spheres };

static struct hs_external_global_definition debug_objects_unit_vectors_definition = { "debug_objects_unit_vectors", _hs_type_boolean, 0, &debug_objects_unit_vectors };

static struct hs_external_global_definition debug_objects_unit_seats_definition = { "debug_objects_unit_seats", _hs_type_boolean, 0, &debug_objects_unit_seats };

static struct hs_external_global_definition debug_objects_unit_mouth_apeture_definition = { "debug_objects_unit_mouth_apeture", _hs_type_boolean, 0, &debug_objects_unit_mouth_apeture };

static struct hs_external_global_definition debug_objects_biped_physics_pills_definition = { "debug_objects_biped_physics_pills", _hs_type_boolean, 0, &debug_objects_biped_physics_pills };

static struct hs_external_global_definition debug_objects_biped_autoaim_pills_definition = { "debug_objects_biped_autoaim_pills", _hs_type_boolean, 0, &debug_objects_biped_autoaim_pills };

static struct hs_external_global_definition debug_objects_vehicle_powered_mass_points_definition = { "debug_objects_vehicle_powered_mass_points", _hs_type_boolean, 0, &debug_objects_vehicle_powered_mass_points };

static struct hs_external_global_definition debug_objects_devices_definition = { "debug_objects_devices", _hs_type_boolean, 0, &debug_objects_devices };

static struct hs_external_global_definition render_model_nodes_definition = { "render_model_nodes", _hs_type_boolean, 0, &render_model_nodes };

static struct hs_external_global_definition render_model_vertex_counts_definition = { "render_model_vertex_counts", _hs_type_boolean, 0, &render_model_vertex_counts };

static struct hs_external_global_definition render_model_index_counts_definition = { "render_model_index_counts", _hs_type_boolean, 0, &render_model_index_counts };

static struct hs_external_global_definition render_model_markers_definition = { "render_model_markers", _hs_type_boolean, 0, &render_model_markers };

static struct hs_external_global_definition render_model_no_geometry_definition = { "render_model_no_geometry", _hs_type_boolean, 0, &render_model_no_geometry };

static struct hs_external_global_definition render_shadows_definition = { "render_shadows", _hs_type_boolean, 0, &render_shadows };

static struct hs_external_global_definition debug_damage_definition = { "debug_damage", _hs_type_boolean, 0, &debug_damage };

static struct hs_external_global_definition debug_scripting_definition = { "debug_scripting", _hs_type_boolean, 0, &debug_scripting };

static struct hs_external_global_definition debug_trigger_volumes_definition = { "debug_trigger_volumes", _hs_type_boolean, 0, &debug_trigger_volumes };

static struct hs_external_global_definition debug_point_physics_definition = { "debug_point_physics", _hs_type_boolean, 0, &debug_point_physics };

static struct hs_external_global_definition debug_physics_disable_penetration_freeze_definition = { "debug_physics_disable_penetration_freeze", _hs_type_boolean, 0, &debug_physics_disable_penetration_freeze };

static struct hs_external_global_definition debug_motion_sensor_draw_all_units_definition = { "debug_motion_sensor_draw_all_units", _hs_type_boolean, 0, &debug_motion_sensor_draw_all_units };

static struct hs_external_global_definition collision_debug_definition = { "collision_debug", _hs_type_boolean, 0, &collision_debug };

static struct hs_external_global_definition collision_debug_spray_definition = { "collision_debug_spray", _hs_type_boolean, 0, &collision_debug_spray };

static struct hs_external_global_definition collision_debug_features_definition = { "collision_debug_features", _hs_type_boolean, 0, &collision_debug_features };

static struct hs_external_global_definition collision_debug_repeat_definition = { "collision_debug_repeat", _hs_type_boolean, 0, &collision_debug_repeat };

static struct hs_external_global_definition collision_debug_flag_front_facing_surfaces_definition = { "collision_debug_flag_front_facing_surfaces", _hs_type_boolean, 0, &collision_debug_flag_front_facing_surfaces };

static struct hs_external_global_definition collision_debug_flag_back_facing_surfaces_definition = { "collision_debug_flag_back_facing_surfaces", _hs_type_boolean, 0, &collision_debug_flag_back_facing_surfaces };

static struct hs_external_global_definition collision_debug_flag_ignore_two_sided_surfaces_definition = { "collision_debug_flag_ignore_two_sided_surfaces", _hs_type_boolean, 0, &collision_debug_flag_ignore_two_sided_surfaces };

static struct hs_external_global_definition collision_debug_flag_ignore_invisible_surfaces_definition = { "collision_debug_flag_ignore_invisible_surfaces", _hs_type_boolean, 0, &collision_debug_flag_ignore_invisible_surfaces };

static struct hs_external_global_definition collision_debug_flag_ignore_breakable_surfaces_definition = { "collision_debug_flag_ignore_breakable_surfaces", _hs_type_boolean, 0, &collision_debug_flag_ignore_breakable_surfaces };

static struct hs_external_global_definition collision_debug_flag_structure_definition = { "collision_debug_flag_structure", _hs_type_boolean, 0, &collision_debug_flag_structure };

static struct hs_external_global_definition collision_debug_flag_media_definition = { "collision_debug_flag_media", _hs_type_boolean, 0, &collision_debug_flag_media };

static struct hs_external_global_definition collision_debug_flag_objects_definition = { "collision_debug_flag_objects", _hs_type_boolean, 0, &collision_debug_flag_objects };

static struct hs_external_global_definition collision_debug_flag_objects_bipeds_definition = { "collision_debug_flag_objects_bipeds", _hs_type_boolean, 0, &collision_debug_flag_objects_bipeds };

static struct hs_external_global_definition collision_debug_flag_objects_vehicles_definition = { "collision_debug_flag_objects_vehicles", _hs_type_boolean, 0, &collision_debug_flag_objects_vehicles };

static struct hs_external_global_definition collision_debug_flag_objects_weapons_definition = { "collision_debug_flag_objects_weapons", _hs_type_boolean, 0, &collision_debug_flag_objects_weapons };

static struct hs_external_global_definition collision_debug_flag_objects_equipment_definition = { "collision_debug_flag_objects_equipment", _hs_type_boolean, 0, &collision_debug_flag_objects_equipment };

static struct hs_external_global_definition collision_debug_flag_objects_projectiles_definition = { "collision_debug_flag_objects_projectiles", _hs_type_boolean, 0, &collision_debug_flag_objects_projectiles };

static struct hs_external_global_definition collision_debug_flag_objects_scenery_definition = { "collision_debug_flag_objects_scenery", _hs_type_boolean, 0, &collision_debug_flag_objects_scenery };

static struct hs_external_global_definition collision_debug_flag_objects_machines_definition = { "collision_debug_flag_objects_machines", _hs_type_boolean, 0, &collision_debug_flag_objects_machines };

static struct hs_external_global_definition collision_debug_flag_objects_controls_definition = { "collision_debug_flag_objects_controls", _hs_type_boolean, 0, &collision_debug_flag_objects_controls };

static struct hs_external_global_definition collision_debug_flag_objects_light_fixtures_definition = { "collision_debug_flag_objects_light_fixtures", _hs_type_boolean, 0, &collision_debug_flag_objects_light_fixtures };

static struct hs_external_global_definition collision_debug_flag_objects_placeholders_definition = { "collision_debug_flag_objects_placeholders", _hs_type_boolean, 0, &collision_debug_flag_objects_placeholders };

static struct hs_external_global_definition collision_debug_flag_try_to_keep_location_valid_definition = { "collision_debug_flag_try_to_keep_location_valid", _hs_type_boolean, 0, &collision_debug_flag_try_to_keep_location_valid };

static struct hs_external_global_definition collision_debug_flag_skip_passthrough_bipeds_definition = { "collision_debug_flag_skip_passthrough_bipeds", _hs_type_boolean, 0, &collision_debug_flag_skip_passthrough_bipeds };

static struct hs_external_global_definition collision_debug_flag_use_vehicle_physics_definition = { "collision_debug_flag_use_vehicle_physics", _hs_type_boolean, 0, &collision_debug_flag_use_vehicle_physics };

static struct hs_external_global_definition collision_debug_point_x_definition = { "collision_debug_point_x", _hs_type_real, 0, &collision_debug_point.x };

static struct hs_external_global_definition collision_debug_point_y_definition = { "collision_debug_point_y", _hs_type_real, 0, &collision_debug_point.y };

static struct hs_external_global_definition collision_debug_point_z_definition = { "collision_debug_point_z", _hs_type_real, 0, &collision_debug_point.z };

static struct hs_external_global_definition collision_debug_vector_i_definition = { "collision_debug_vector_i", _hs_type_real, 0, &collision_debug_vector.i };

static struct hs_external_global_definition collision_debug_vector_j_definition = { "collision_debug_vector_j", _hs_type_real, 0, &collision_debug_vector.j };

static struct hs_external_global_definition collision_debug_vector_k_definition = { "collision_debug_vector_k", _hs_type_real, 0, &collision_debug_vector.k };

static struct hs_external_global_definition collision_debug_length_definition = { "collision_debug_length", _hs_type_real, 0, &collision_debug_length };

static struct hs_external_global_definition collision_debug_width_definition = { "collision_debug_width", _hs_type_real, 0, &collision_debug_width };

static struct hs_external_global_definition collision_debug_height_definition = { "collision_debug_height", _hs_type_real, 0, &collision_debug_height };

static struct hs_external_global_definition collision_debug_phantom_bsp_definition = { "collision_debug_phantom_bsp", _hs_type_boolean, 0, &collision_debug_phantom_bsp };

static struct hs_external_global_definition debug_obstacle_path_definition = { "debug_obstacle_path", _hs_type_boolean, 0, &debug_obstacle_path };

static struct hs_external_global_definition debug_obstacle_path_on_failure_definition = { "debug_obstacle_path_on_failure", _hs_type_boolean, 0, &debug_obstacle_path_on_failure };

static struct hs_external_global_definition debug_obstacle_path_start_point_x_definition = { "debug_obstacle_path_start_point_x", _hs_type_real, 0, &debug_obstacle_path_start_point.x };

static struct hs_external_global_definition debug_obstacle_path_start_point_y_definition = { "debug_obstacle_path_start_point_y", _hs_type_real, 0, &debug_obstacle_path_start_point.y };

static struct hs_external_global_definition debug_obstacle_path_start_surface_index_definition = { "debug_obstacle_path_start_surface_index", _hs_type_long_integer, 0, &debug_obstacle_path_start_surface_index };

static struct hs_external_global_definition debug_obstacle_path_goal_point_x_definition = { "debug_obstacle_path_goal_point_x", _hs_type_real, 0, &debug_obstacle_path_goal_point.x };

static struct hs_external_global_definition debug_obstacle_path_goal_point_y_definition = { "debug_obstacle_path_goal_point_y", _hs_type_real, 0, &debug_obstacle_path_goal_point.y };

static struct hs_external_global_definition debug_obstacle_path_goal_surface_index_definition = { "debug_obstacle_path_goal_surface_index", _hs_type_long_integer, 0, &debug_obstacle_path_goal_surface_index };

static struct hs_external_global_definition debug_camera_definition = { "debug_camera", _hs_type_boolean, 0, &debug_camera };

static struct hs_external_global_definition debug_player_definition = { "debug_player", _hs_type_boolean, 0, &debug_player };

static struct hs_external_global_definition debug_structure_definition = { "debug_structure", _hs_type_boolean, 0, &debug_structure };

static struct hs_external_global_definition debug_bsp_definition = { "debug_bsp", _hs_type_boolean, 0, &debug_bsp };

static struct hs_external_global_definition debug_input_definition = { "debug_input", _hs_type_boolean, 0, &debug_input };

static struct hs_external_global_definition debug_permanent_decals_definition = { "debug_permanent_decals", _hs_type_boolean, 0, &debug_permanent_decals };

static struct hs_external_global_definition debug_fog_planes_definition = { "debug_fog_planes", _hs_type_boolean, 0, &debug_fog_planes };

static struct hs_external_global_definition breakable_surfaces_definition = { "breakable_surfaces", _hs_type_boolean, 0, &breakable_surface_effect_enabled };

static struct hs_external_global_definition decals_definition = { "decals", _hs_type_boolean, 0, &decals_enabled };

static struct hs_external_global_definition debug_decals_definition = { "debug_decals", _hs_type_boolean, 0, &debug_decals };

static struct hs_external_global_definition debug_object_lights_definition = { "debug_object_lights", _hs_type_boolean, 0, &debug_object_lights };

static struct hs_external_global_definition debug_lights_definition = { "debug_lights", _hs_type_boolean, 0, &debug_lights };

static struct hs_external_global_definition debug_biped_physics_definition = { "debug_biped_physics", _hs_type_boolean, 0, &debug_biped_physics };

static struct hs_external_global_definition debug_biped_skip_update_definition = { "debug_biped_skip_update", _hs_type_boolean, 0, &debug_biped_skip_update };

static struct hs_external_global_definition debug_biped_skip_collision_definition = { "debug_biped_skip_collision", _hs_type_boolean, 0, &debug_biped_skip_collision };

static struct hs_external_global_definition debug_biped_limp_body_disable_definition = { "debug_biped_limp_body_disable", _hs_type_boolean, 0, &debug_biped_limp_body_disable };

static struct hs_external_global_definition debug_collision_skip_objects_definition = { "debug_collision_skip_objects", _hs_type_boolean, 0, &debug_collision_skip_objects };

static struct hs_external_global_definition debug_collision_skip_vectors_definition = { "debug_collision_skip_vectors", _hs_type_boolean, 0, &debug_collision_skip_vectors };

static struct hs_external_global_definition debug_material_effects_definition = { "debug_material_effects", _hs_type_boolean, 0, &debug_material_effects };

static struct hs_external_global_definition weather_definition = { "weather", _hs_type_boolean, 0, &weather };

static struct hs_external_global_definition ai_profile_disable_definition = { "ai_profile_disable", _hs_type_boolean, 0, &ai_profile.disabled };

static struct hs_external_global_definition ai_profile_random_definition = { "ai_profile_random", _hs_type_boolean, 0, &ai_profile.move_actors_randomly };

static struct hs_external_global_definition ai_show_definition = { "ai_show", _hs_type_boolean, 0, &ai_profile.show };

static struct hs_external_global_definition ai_show_stats_definition = { "ai_show_stats", _hs_type_boolean, 0, &ai_profile.show_stats };

static struct hs_external_global_definition ai_show_actors_definition = { "ai_show_actors", _hs_type_boolean, 0, &ai_profile.show_actors };

static struct hs_external_global_definition ai_show_swarms_definition = { "ai_show_swarms", _hs_type_boolean, 0, &ai_profile.show_swarms };

static struct hs_external_global_definition ai_show_paths_definition = { "ai_show_paths", _hs_type_boolean, 0, &ai_profile.show_paths };

static struct hs_external_global_definition ai_show_line_of_sight_definition = { "ai_show_line_of_sight", _hs_type_boolean, 0, &ai_profile.show_line_of_sight };

static struct hs_external_global_definition ai_show_prop_types_definition = { "ai_show_prop_types", _hs_type_boolean, 0, &ai_profile.show_prop_types };

static struct hs_external_global_definition ai_show_sound_distance_definition = { "ai_show_sound_distance", _hs_type_boolean, 0, &ai_profile.show_sound_distance };

static struct hs_external_global_definition ai_render_definition = { "ai_render", _hs_type_boolean, 0, &ai_debug.render };

static struct hs_external_global_definition ai_render_all_actors_definition = { "ai_render_all_actors", _hs_type_boolean, 0, &ai_debug.render_all_actors };

static struct hs_external_global_definition ai_render_inactive_actors_definition = { "ai_render_inactive_actors", _hs_type_boolean, 0, &ai_debug.render_inactive_actors };

static struct hs_external_global_definition ai_render_lineoffire_crouching_definition = { "ai_render_lineoffire_crouching", _hs_type_boolean, 0, &ai_debug.render_lineoffire_crouching };

static struct hs_external_global_definition ai_render_lineoffire_definition = { "ai_render_lineoffire", _hs_type_boolean, 0, &ai_debug.render_lineoffire };

static struct hs_external_global_definition ai_render_lineofsight_definition = { "ai_render_lineofsight", _hs_type_boolean, 0, &ai_debug.render_lineofsight };

static struct hs_external_global_definition ai_render_ballistic_lineoffire_definition = { "ai_render_ballistic_lineoffire", _hs_type_boolean, 0, &ai_debug.render_ballistic_lineoffire };

static struct hs_external_global_definition ai_render_encounter_activeregion_definition = { "ai_render_encounter_activeregion", _hs_type_boolean, 0, &ai_debug.render_encounter_activeregion };

static struct hs_external_global_definition ai_render_vision_cones_definition = { "ai_render_vision_cones", _hs_type_boolean, 0, &ai_debug.render_vision_cones };

static struct hs_external_global_definition ai_render_current_state_definition = { "ai_render_current_state", _hs_type_boolean, 0, &ai_debug.render_current_state };

static struct hs_external_global_definition ai_render_detailed_state_definition = { "ai_render_detailed_state", _hs_type_boolean, 0, &ai_debug.render_detailed_state };

static struct hs_external_global_definition ai_render_props_definition = { "ai_render_props", _hs_type_boolean, 0, &ai_debug.render_props };

static struct hs_external_global_definition ai_render_props_web_definition = { "ai_render_props_web", _hs_type_boolean, 0, &ai_debug.render_props_web };

static struct hs_external_global_definition ai_render_props_no_friends_definition = { "ai_render_props_no_friends", _hs_type_boolean, 0, &ai_debug.render_props_no_friends };

static struct hs_external_global_definition ai_render_props_unreachable_definition = { "ai_render_props_unreachable", _hs_type_boolean, 0, &ai_debug.render_props_unopposable };

static struct hs_external_global_definition ai_render_props_unopposable_definition = { "ai_render_props_unopposable", _hs_type_boolean, 0, &ai_debug.render_props_target_weight };

static struct hs_external_global_definition ai_render_props_target_weight_definition = { "ai_render_props_target_weight", _hs_type_boolean, 0, &ai_debug.render_props_unreachable };

static struct hs_external_global_definition ai_render_idle_look_definition = { "ai_render_idle_look", _hs_type_boolean, 0, &ai_debug.render_idle_look };

static struct hs_external_global_definition ai_render_support_surfaces_definition = { "ai_render_support_surfaces", _hs_type_boolean, 0, &ai_debug.render_support_surfaces };

static struct hs_external_global_definition ai_render_recent_damage_definition = { "ai_render_recent_damage", _hs_type_boolean, 0, &ai_debug.render_recent_damage };

static struct hs_external_global_definition ai_render_threats_definition = { "ai_render_threats", _hs_type_boolean, 0, &ai_debug.render_threats };

static struct hs_external_global_definition ai_render_emotions_definition = { "ai_render_emotions", _hs_type_boolean, 0, &ai_debug.render_emotions };

static struct hs_external_global_definition ai_render_audibility_definition = { "ai_render_audibility", _hs_type_boolean, 0, &ai_debug.render_audibility };

static struct hs_external_global_definition ai_render_aiming_vectors_definition = { "ai_render_aiming_vectors", _hs_type_boolean, 0, &ai_debug.render_aiming_vectors };

static struct hs_external_global_definition ai_render_secondary_looking_definition = { "ai_render_secondary_looking", _hs_type_boolean, 0, &ai_debug.render_secondary_looking };

static struct hs_external_global_definition ai_render_targets_definition = { "ai_render_targets", _hs_type_boolean, 0, &ai_debug.render_targets };

static struct hs_external_global_definition ai_render_targets_last_visible_definition = { "ai_render_targets_last_visible", _hs_type_boolean, 0, &ai_debug.render_targets_last_visible };

static struct hs_external_global_definition ai_render_states_definition = { "ai_render_states", _hs_type_boolean, 0, &ai_debug.render_states };

static struct hs_external_global_definition ai_render_vitality_definition = { "ai_render_vitality", _hs_type_boolean, 0, &ai_debug.render_vitality };

static struct hs_external_global_definition ai_render_active_cover_seeking_definition = { "ai_render_active_cover_seeking", _hs_type_boolean, 0, &ai_debug.render_active_cover_seeking };

static struct hs_external_global_definition ai_render_evaluations_definition = { "ai_render_evaluations", _hs_type_boolean, 0, &ai_debug.render_evaluations };

static struct hs_external_global_definition ai_render_pursuit_definition = { "ai_render_pursuit", _hs_type_boolean, 0, &ai_debug.render_pursuit };

static struct hs_external_global_definition ai_render_shooting_definition = { "ai_render_shooting", _hs_type_boolean, 0, &ai_debug.render_shooting };

static struct hs_external_global_definition ai_render_trigger_definition = { "ai_render_trigger", _hs_type_boolean, 0, &ai_debug.render_trigger };

static struct hs_external_global_definition ai_render_projectile_aiming_definition = { "ai_render_projectile_aiming", _hs_type_boolean, 0, &ai_debug.render_projectile_aiming };

static struct hs_external_global_definition ai_render_aiming_validity_definition = { "ai_render_aiming_validity", _hs_type_boolean, 0, &ai_debug.render_aiming_validity };

static struct hs_external_global_definition ai_render_speech_definition = { "ai_render_speech", _hs_type_boolean, 0, &ai_debug.render_speech };

static struct hs_external_global_definition ai_render_teams_definition = { "ai_render_teams", _hs_type_boolean, 0, &ai_debug.render_teams };

static struct hs_external_global_definition ai_render_player_ratings_definition = { "ai_render_player_ratings", _hs_type_boolean, 0, &ai_debug.render_player_ratings };

static struct hs_external_global_definition ai_render_spatial_effects_definition = { "ai_render_spatial_effects", _hs_type_boolean, 0, &ai_debug.render_spatial_effects };

static struct hs_external_global_definition ai_render_firing_positions_definition = { "ai_render_firing_positions", _hs_type_boolean, 0, &ai_debug.render_firing_positions };

static struct hs_external_global_definition ai_render_gun_positions_definition = { "ai_render_gun_positions", _hs_type_boolean, 0, &ai_debug.render_gun_positions };

static struct hs_external_global_definition ai_render_burst_geometry_definition = { "ai_render_burst_geometry", _hs_type_boolean, 0, &ai_debug.render_burst_geometry };

static struct hs_external_global_definition ai_render_vehicle_avoidance_definition = { "ai_render_vehicle_avoidance", _hs_type_boolean, 0, &ai_debug.render_vehicle_avoidance };

static struct hs_external_global_definition ai_render_vehicles_enterable_definition = { "ai_render_vehicles_enterable", _hs_type_boolean, 0, &ai_debug.render_vehicles_enterable };

static struct hs_external_global_definition ai_render_melee_check_definition = { "ai_render_melee_check", _hs_type_boolean, 0, &ai_debug.render_melee_check };

static struct hs_external_global_definition ai_render_dialogue_variants_definition = { "ai_render_dialogue_variants", _hs_type_boolean, 0, &ai_debug.render_dialogue_variants };

static struct hs_external_global_definition ai_render_grenade_decisions_definition = { "ai_render_grenade_decisions", _hs_type_boolean, 0, &ai_debug.render_grenade_decisions };

static struct hs_external_global_definition ai_render_danger_zones_definition = { "ai_render_danger_zones", _hs_type_boolean, 0, &ai_debug.render_danger_zones };

static struct hs_external_global_definition ai_render_charge_decisions_definition = { "ai_render_charge_decisions", _hs_type_boolean, 0, &ai_debug.render_charge_decisions };

static struct hs_external_global_definition ai_render_control_definition = { "ai_render_control", _hs_type_boolean, 0, &ai_debug.render_control };

static struct hs_external_global_definition ai_render_activation_definition = { "ai_render_activation", _hs_type_boolean, 0, &ai_debug.render_activation };

static struct hs_external_global_definition ai_render_paths_definition = { "ai_render_paths", _hs_type_boolean, 0, &ai_debug.render_paths };

static struct hs_external_global_definition ai_render_paths_selected_only_definition = { "ai_render_paths_selected_only", _hs_type_boolean, 0, &ai_debug.render_paths_selected_only };

static struct hs_external_global_definition ai_render_paths_destination_definition = { "ai_render_paths_destination", _hs_type_boolean, 0, &ai_debug.render_paths_destination };

static struct hs_external_global_definition ai_render_paths_raw_definition = { "ai_render_paths_raw", _hs_type_boolean, 0, &ai_debug.render_paths_raw };

static struct hs_external_global_definition ai_render_paths_current_definition = { "ai_render_paths_current", _hs_type_boolean, 0, &ai_debug.render_paths_current };

static struct hs_external_global_definition ai_render_paths_failed_definition = { "ai_render_paths_failed", _hs_type_boolean, 0, &ai_debug.render_paths_failed };

static struct hs_external_global_definition ai_render_paths_smoothed_definition = { "ai_render_paths_smoothed", _hs_type_boolean, 0, &ai_debug.render_paths_smoothed };

static struct hs_external_global_definition ai_render_paths_avoided_definition = { "ai_render_paths_avoided", _hs_type_boolean, 0, &ai_debug.render_paths_avoided };

static struct hs_external_global_definition ai_render_paths_avoidance_segment_definition = { "ai_render_paths_avoidance_segment", _hs_type_short_integer, 0, &ai_debug.render_paths_avoidance_segment };

static struct hs_external_global_definition ai_render_paths_avoidance_obstacles_definition = { "ai_render_paths_avoidance_obstacles", _hs_type_boolean, 0, &ai_debug.render_paths_avoidance_obstacles };

static struct hs_external_global_definition ai_render_paths_avoidance_search_definition = { "ai_render_paths_avoidance_search", _hs_type_boolean, 0, &ai_debug.render_paths_avoidance_search };

static struct hs_external_global_definition ai_render_paths_nodes_definition = { "ai_render_paths_nodes", _hs_type_boolean, 0, &ai_debug.render_paths_nodes };

static struct hs_external_global_definition ai_render_paths_nodes_all_definition = { "ai_render_paths_nodes_all", _hs_type_boolean, 0, &ai_debug.render_paths_nodes_all };

static struct hs_external_global_definition ai_render_paths_nodes_polygons_definition = { "ai_render_paths_nodes_polygons", _hs_type_boolean, 0, &ai_debug.render_paths_nodes_polygons };

static struct hs_external_global_definition ai_render_paths_nodes_costs_definition = { "ai_render_paths_nodes_costs", _hs_type_boolean, 0, &ai_debug.render_paths_nodes_costs };

static struct hs_external_global_definition ai_render_paths_nodes_closest_definition = { "ai_render_paths_nodes_closest", _hs_type_boolean, 0, &ai_debug.render_paths_nodes_closest };

static struct hs_external_global_definition ai_render_player_aiming_blocked_definition = { "ai_render_player_aiming_blocked", _hs_type_boolean, 0, &ai_debug.render_player_aiming_blocked };

static struct hs_external_global_definition ai_render_vector_avoidance_definition = { "ai_render_vector_avoidance", _hs_type_boolean, 0, &ai_debug.render_vector_avoidance };

static struct hs_external_global_definition ai_render_vector_avoidance_rays_definition = { "ai_render_vector_avoidance_rays", _hs_type_boolean, 0, &ai_debug.render_vector_avoidance_rays };

static struct hs_external_global_definition ai_render_vector_avoidance_sense_t_definition = { "ai_render_vector_avoidance_sense_t", _hs_type_boolean, 0, &ai_debug.render_vector_avoidance_sense_t };

static struct hs_external_global_definition ai_render_vector_avoidance_avoid_t_definition = { "ai_render_vector_avoidance_avoid_t", _hs_type_boolean, 0, &ai_debug.render_vector_avoidance_avoid_t };

static struct hs_external_global_definition ai_render_vector_avoidance_clear_time_definition = { "ai_render_vector_avoidance_clear_time", _hs_type_boolean, 0, &ai_debug.render_vector_avoidance_clear_time };

static struct hs_external_global_definition ai_render_vector_avoidance_weights_definition = { "ai_render_vector_avoidance_weights", _hs_type_boolean, 0, &ai_debug.render_vector_avoidance_weights };

static struct hs_external_global_definition ai_render_vector_avoidance_objects_definition = { "ai_render_vector_avoidance_objects", _hs_type_boolean, 0, &ai_debug.render_vector_avoidance_objects };

static struct hs_external_global_definition ai_render_vector_avoidance_intermediate_definition = { "ai_render_vector_avoidance_intermediate", _hs_type_boolean, 0, &ai_debug.render_vector_avoidance_intermediate };

static struct hs_external_global_definition ai_render_postcombat_definition = { "ai_render_postcombat", _hs_type_boolean, 0, &ai_debug.render_postcombat };

static struct hs_external_global_definition ai_print_pursuit_checks_definition = { "ai_print_pursuit_checks", _hs_type_boolean, 0, &ai_debug.print_pursuit_checks };

static struct hs_external_global_definition ai_print_rules_definition = { "ai_print_rules", _hs_type_boolean, 0, &ai_debug.print_rules };

static struct hs_external_global_definition ai_print_rule_values_definition = { "ai_print_rule_values", _hs_type_boolean, 0, &ai_debug.print_rule_values };

static struct hs_external_global_definition ai_print_major_upgrade_definition = { "ai_print_major_upgrade", _hs_type_boolean, 0, &ai_debug.print_major_upgrade };

static struct hs_external_global_definition ai_print_respawn_definition = { "ai_print_respawn", _hs_type_boolean, 0, &ai_debug.print_respawn };

static struct hs_external_global_definition ai_print_evaluation_statistics_definition = { "ai_print_evaluation_statistics", _hs_type_boolean, 0, &ai_debug.print_evaluation_statistics };

static struct hs_external_global_definition ai_print_communication_definition = { "ai_print_communication", _hs_type_boolean, 0, &ai_debug.print_communication };

static struct hs_external_global_definition ai_print_communication_player_definition = { "ai_print_communication_player", _hs_type_boolean, 0, &ai_debug.print_communication_player };

static struct hs_external_global_definition ai_print_vocalizations_definition = { "ai_print_vocalizations", _hs_type_boolean, 0, &ai_debug.print_vocalizations };

static struct hs_external_global_definition ai_print_placement_definition = { "ai_print_placement", _hs_type_boolean, 0, &ai_debug.print_placement };

static struct hs_external_global_definition ai_print_speech_definition = { "ai_print_speech", _hs_type_boolean, 0, &ai_debug.print_speech };

static struct hs_external_global_definition ai_print_speech_timers_definition = { "ai_print_speech_timers", _hs_type_boolean, 0, &ai_debug.print_speech_timers };

static struct hs_external_global_definition ai_print_allegiance_definition = { "ai_print_allegiance", _hs_type_boolean, 0, &ai_debug.print_allegiance };

static struct hs_external_global_definition ai_print_lost_speech_definition = { "ai_print_lost_speech", _hs_type_boolean, 0, &ai_debug.print_lost_speech };

static struct hs_external_global_definition ai_print_migration_definition = { "ai_print_migration", _hs_type_boolean, 0, &ai_debug.print_migration };

static struct hs_external_global_definition ai_print_automatic_migration_definition = { "ai_print_automatic_migration", _hs_type_boolean, 0, &ai_debug.print_automatic_migration };

static struct hs_external_global_definition ai_print_scripting_definition = { "ai_print_scripting", _hs_type_boolean, 0, &ai_debug.print_scripting };

static struct hs_external_global_definition ai_print_surprise_definition = { "ai_print_surprise", _hs_type_boolean, 0, &ai_debug.print_surprise };

static struct hs_external_global_definition ai_print_command_lists_definition = { "ai_print_command_lists", _hs_type_boolean, 0, &ai_debug.print_command_lists };

static struct hs_external_global_definition ai_print_damage_modifiers_definition = { "ai_print_damage_modifiers", _hs_type_boolean, 0, &ai_debug.print_damage_modifiers };

static struct hs_external_global_definition ai_print_secondary_looking_definition = { "ai_print_secondary_looking", _hs_type_boolean, 0, &ai_debug.print_secondary_looking };

static struct hs_external_global_definition ai_print_oversteer_definition = { "ai_print_oversteer", _hs_type_boolean, 0, &ai_debug.print_oversteer };

static struct hs_external_global_definition ai_print_conversations_definition = { "ai_print_conversations", _hs_type_boolean, 0, &ai_debug.print_conversations };

static struct hs_external_global_definition ai_print_killing_sprees_definition = { "ai_print_killing_sprees", _hs_type_boolean, 0, &ai_debug.print_killing_sprees };

static struct hs_external_global_definition ai_print_acknowledgement_definition = { "ai_print_acknowledgement", _hs_type_boolean, 0, &ai_debug.print_acknowledgement };

static struct hs_external_global_definition ai_print_unfinished_paths_definition = { "ai_print_unfinished_paths", _hs_type_boolean, 0, &ai_debug.print_unfinished_paths };

static struct hs_external_global_definition ai_print_bsp_transition_definition = { "ai_print_bsp_transition", _hs_type_boolean, 0, &ai_debug.print_bsp_transition };

static struct hs_external_global_definition ai_print_uncovering_definition = { "ai_print_uncovering", _hs_type_boolean, 0, &ai_debug.print_uncovering };

static struct hs_external_global_definition ai_debug_fast_los_definition = { "ai_debug_fast_los", _hs_type_boolean, 0, &ai_debug.fast_los };

static struct hs_external_global_definition ai_debug_oversteer_disable_definition = { "ai_debug_oversteer_disable", _hs_type_boolean, 0, &ai_debug.oversteer_disable };

static struct hs_external_global_definition ai_debug_evaluate_all_positions_definition = { "ai_debug_evaluate_all_positions", _hs_type_boolean, 0, &ai_debug.evaluate_all_positions };

static struct hs_external_global_definition ai_debug_path_definition = { "ai_debug_path", _hs_type_boolean, 0, &ai_debug.path };

static struct hs_external_global_definition ai_debug_path_start_freeze_definition = { "ai_debug_path_start_freeze", _hs_type_boolean, 0, &ai_debug.path_start_freeze };

static struct hs_external_global_definition ai_debug_path_end_freeze_definition = { "ai_debug_path_end_freeze", _hs_type_boolean, 0, &ai_debug.path_end_freeze };

static struct hs_external_global_definition ai_debug_path_flood_definition = { "ai_debug_path_flood", _hs_type_boolean, 0, &ai_debug.path_flood };

static struct hs_external_global_definition ai_debug_path_maximum_radius_definition = { "ai_debug_path_maximum_radius", _hs_type_real, 0, &ai_debug.path_maximum_radius };

static struct hs_external_global_definition ai_debug_path_attractor_definition = { "ai_debug_path_attractor", _hs_type_boolean, 0, &ai_debug.path_attractor };

static struct hs_external_global_definition ai_debug_path_attractor_radius_definition = { "ai_debug_path_attractor_radius", _hs_type_real, 0, &ai_debug.path_attractor_radius };

static struct hs_external_global_definition ai_debug_path_attractor_weight_definition = { "ai_debug_path_attractor_weight", _hs_type_real, 0, &ai_debug.path_attractor_weight };

static struct hs_external_global_definition ai_debug_path_accept_radius_definition = { "ai_debug_path_accept_radius", _hs_type_real, 0, &ai_debug.path_accept_radius };

static struct hs_external_global_definition ai_debug_ballistic_lineoffire_freeze_definition = { "ai_debug_ballistic_lineoffire_freeze", _hs_type_boolean, 0, &ai_debug.ballistic_lineoffire_freeze };

static struct hs_external_global_definition ai_debug_communication_random_disabled_definition = { "ai_debug_communication_random_disabled", _hs_type_boolean, 0, &ai_debug.communication_random_disabled };

static struct hs_external_global_definition ai_debug_communication_timeout_disabled_definition = { "ai_debug_communication_timeout_disabled", _hs_type_boolean, 0, &ai_debug.communication_timeout_disabled };

static struct hs_external_global_definition ai_debug_communication_unit_repeat_disabled_definition = { "ai_debug_communication_unit_repeat_disabled", _hs_type_boolean, 0, &ai_debug.communication_unit_repeat_disabled };

static struct hs_external_global_definition ai_debug_communication_focus_enable_definition = { "ai_debug_communication_focus_enable", _hs_type_boolean, 0, &ai_debug.communication_focus_enable };

static struct hs_external_global_definition ai_debug_blind_definition = { "ai_debug_blind", _hs_type_boolean, 0, &ai_debug.blind };

static struct hs_external_global_definition ai_debug_deaf_definition = { "ai_debug_deaf", _hs_type_boolean, 0, &ai_debug.deaf };

static struct hs_external_global_definition ai_debug_invisible_player_definition = { "ai_debug_invisible_player", _hs_type_boolean, 0, &ai_debug.invisible_player };

static struct hs_external_global_definition ai_debug_ignore_player_definition = { "ai_debug_ignore_player", _hs_type_boolean, 0, &ai_debug.ignore_player };

static struct hs_external_global_definition ai_debug_flee_always_definition = { "ai_debug_flee_always", _hs_type_boolean, 0, &ai_debug.flee_always };

static struct hs_external_global_definition ai_debug_force_all_active_definition = { "ai_debug_force_all_active", _hs_type_boolean, 0, &ai_debug.force_all_active };

static struct hs_external_global_definition ai_debug_disable_wounded_sounds_definition = { "ai_debug_disable_wounded_sounds", _hs_type_boolean, 0, &ai_debug.disable_wounded_sounds };

static struct hs_external_global_definition ai_debug_force_vocalizations_definition = { "ai_debug_force_vocalizations", _hs_type_boolean, 0, &ai_debug.force_vocalizations };

static struct hs_external_global_definition ai_debug_force_crouch_definition = { "ai_debug_force_crouch", _hs_type_boolean, 0, &ai_debug.force_crouch };

static struct hs_external_global_definition ai_debug_path_disable_smoothing_definition = { "ai_debug_path_disable_smoothing", _hs_type_boolean, 0, &ai_debug.path_disable_smoothing };

static struct hs_external_global_definition ai_debug_path_disable_obstacle_avoidance_definition = { "ai_debug_path_disable_obstacle_avoidance", _hs_type_boolean, 0, &ai_debug.path_disable_obstacle_avoidance };

static struct hs_external_global_definition ai_fix_defending_guard_firing_positions_definition = { "ai_fix_defending_guard_firing_positions", _hs_type_boolean, 0, &ai_debug.fix_defending_guard_firing_positions };

static struct hs_external_global_definition ai_fix_actor_variants_definition = { "ai_fix_actor_variants", _hs_type_boolean, 0, &ai_debug.fix_actor_variants };

static struct hs_external_global_definition controls_enable_crouch_definition = { "controls_enable_crouch", _hs_type_boolean, 0, &controls_enable_crouch };

static struct hs_external_global_definition controls_swapped_definition = { "controls_swapped", _hs_type_boolean, 0, &controls_swapped };

static struct hs_external_global_definition controls_enable_doubled_spin_definition = { "controls_enable_doubled_spin", _hs_type_boolean, 0, &controls_enable_doubled_spin };

static struct hs_external_global_definition controls_swap_doubled_spin_state_definition = { "controls_swap_doubled_spin_state", _hs_type_boolean, 0, &controls_swap_doubled_spin_state };

static struct hs_external_global_definition player0_look_yaw_rate_definition = { "player0_look_yaw_rate", _hs_type_real, 0, &player_look_yaw_rate[0] };

static struct hs_external_global_definition player1_look_yaw_rate_definition = { "player1_look_yaw_rate", _hs_type_real, 0, &player_look_yaw_rate[1] };

static struct hs_external_global_definition player2_look_yaw_rate_definition = { "player2_look_yaw_rate", _hs_type_real, 0, &player_look_yaw_rate[2] };

static struct hs_external_global_definition player3_look_yaw_rate_definition = { "player3_look_yaw_rate", _hs_type_real, 0, &player_look_yaw_rate[3] };

static struct hs_external_global_definition player0_look_pitch_rate_definition = { "player0_look_pitch_rate", _hs_type_real, 0, &player_look_pitch_rate[0] };

static struct hs_external_global_definition player1_look_pitch_rate_definition = { "player1_look_pitch_rate", _hs_type_real, 0, &player_look_pitch_rate[1] };

static struct hs_external_global_definition player2_look_pitch_rate_definition = { "player2_look_pitch_rate", _hs_type_real, 0, &player_look_pitch_rate[2] };

static struct hs_external_global_definition player3_look_pitch_rate_definition = { "player3_look_pitch_rate", _hs_type_real, 0, &player_look_pitch_rate[3] };

static struct hs_external_global_definition player_autoaim_definition = { "player_autoaim", _hs_type_boolean, 0, &player_autoaim_flag };

static struct hs_external_global_definition player_magnetism_definition = { "player_magnetism", _hs_type_boolean, 0, &player_magnetism_flag };

static struct hs_external_global_definition debug_player_teleport_definition = { "debug_player_teleport", _hs_type_boolean, 0, &debug_render_player_teleport };

static struct hs_external_global_definition texture_cache_graph_definition = { "texture_cache_graph", _hs_type_boolean, 0, texture_cache_debug_options };

static struct hs_external_global_definition texture_cache_list_definition = { "texture_cache_list", _hs_type_boolean, 0, texture_cache_debug_options + 0x1 };

static struct hs_external_global_definition director_camera_switch_fast_definition = { "director_camera_switch_fast", _hs_type_boolean, 0, &director_camera_switch_fast };

static struct hs_external_global_definition debug_recording_definition = { "debug_recording", _hs_type_boolean, 0, &debug_recording };

static struct hs_external_global_definition debug_recording_newlines_definition = { "debug_recording_newlines", _hs_type_short_integer, 0, &debug_recording_newlines };

static struct hs_external_global_definition debug_player_color_definition = { "debug_player_color", _hs_type_short_integer, 0, &debug_player_color };

static struct hs_external_global_definition find_all_fucked_up_shit_definition = { "find_all_fucked_up_shit", _hs_type_boolean, 0, &find_all_fucked_up_shit };

static struct hs_external_global_definition allow_out_of_sync_definition = { "allow_out_of_sync", _hs_type_boolean, 0, &allow_out_of_sync };

static struct hs_external_global_definition global_connection_dont_timeout_definition = { "global_connection_dont_timeout", _hs_type_boolean, 0, &global_connection_dont_timeout };

static struct hs_external_global_definition run_game_scripts_definition = { "run_game_scripts", _hs_type_boolean, 0, NULL };

struct hs_external_global_definition *hs_external_globals[443] =
{
	&rasterizer_near_clip_distance_definition,
	&rasterizer_far_clip_distance_definition,
	&rasterizer_first_person_weapon_near_clip_distance_definition,
	&rasterizer_first_person_weapon_far_clip_distance_definition,
	&rasterizer_pushbuffer_size_definition,
	&rasterizer_pushbuffer_kickoff_size_definition,
	&rasterizer_floating_point_zbuffer_definition,
	&rasterizer_framerate_throttle_definition,
	&rasterizer_framerate_stabilization_definition,
	&rasterizer_refresh_rate_definition,
	&rasterizer_frame_bounds_left_definition,
	&rasterizer_frame_bounds_right_definition,
	&rasterizer_frame_bounds_top_definition,
	&rasterizer_frame_bounds_bottom_definition,
	&rasterizer_stats_definition,
	&rasterizer_mode_definition,
	&rasterizer_wireframe_definition,
	&rasterizer_smart_definition,
	&rasterizer_debug_model_vertices_definition,
	&rasterizer_debug_model_lod_definition,
	&rasterizer_debug_transparents_definition,
	&rasterizer_debug_meter_shader_definition,
	&rasterizer_models_definition,
	&rasterizer_model_transparents_definition,
	&rasterizer_draw_first_person_weapon_first_definition,
	&rasterizer_stencil_mask_definition,
	&rasterizer_environment_definition,
	&rasterizer_environment_lightmaps_definition,
	&rasterizer_environment_shadows_definition,
	&rasterizer_environment_diffuse_lights_definition,
	&rasterizer_environment_diffuse_textures_definition,
	&rasterizer_environment_decals_definition,
	&rasterizer_environment_specular_lights_definition,
	&rasterizer_environment_specular_lightmaps_definition,
	&rasterizer_environment_reflection_lightmap_mask_definition,
	&rasterizer_environment_reflection_mirrors_definition,
	&rasterizer_environment_reflections_definition,
	&rasterizer_environment_transparents_definition,
	&rasterizer_environment_fog_definition,
	&rasterizer_environment_fog_screen_definition,
	&rasterizer_water_definition,
	&rasterizer_lens_flares_definition,
	&rasterizer_dynamic_unlit_geometry_definition,
	&rasterizer_dynamic_lit_geometry_definition,
	&rasterizer_dynamic_screen_geometry_definition,
	&rasterizer_hud_motion_sensor_definition,
	&rasterizer_detail_objects_definition,
	&rasterizer_debug_geometry_definition,
	&rasterizer_debug_geometry_multipass_definition,
	&rasterizer_fog_atmosphere_definition,
	&rasterizer_fog_plane_definition,
	&rasterizer_bump_mapping_definition,
	&rasterizer_lightmap_ambient_definition,
	&rasterizer_lightmap_mode_definition,
	&rasterizer_lightmaps_incident_radiosity_definition,
	&rasterizer_lightmaps_filtering_definition,
	&rasterizer_model_lighting_ambient_definition,
	&rasterizer_environment_alpha_testing_definition,
	&rasterizer_environment_specular_mask_definition,
	&rasterizer_shadows_convolution_definition,
	&rasterizer_shadows_debug_definition,
	&rasterizer_water_mipmapping_definition,
	&rasterizer_active_camouflage_definition,
	&rasterizer_active_camouflage_multipass_definition,
	&rasterizer_plasma_energy_definition,
	&rasterizer_lens_flares_occlusion_definition,
	&rasterizer_lens_flares_occlusion_debug_definition,
	&rasterizer_ray_of_buddha_definition,
	&rasterizer_screen_flashes_definition,
	&rasterizer_screen_effects_definition,
	&rasterizer_DXTC_noise_definition,
	&rasterizer_soft_filter_definition,
	&rasterizer_secondary_render_target_debug_definition,
	&rasterizer_profile_log_definition,
	&rasterizer_detail_objects_offset_multiplier_definition,
	&rasterizer_zbias_definition,
	&rasterizer_zoffset_definition,
	&force_all_player_views_to_default_player_definition,
	&rasterizer_safe_frame_bounds_definition,
	&freeze_flying_camera_definition,
	&rasterizer_zsprites_definition,
	&rasterizer_filthy_decal_fog_hack_definition,
	&rasterizer_splitscreen_VB_optimization_definition,
	&rasterizer_profile_print_locks_definition,
	&rasterizer_profile_objectlock_time_definition,
	&pad3_definition,
	&pad3_scale_definition,
	&f0_definition,
	&f1_definition,
	&f2_definition,
	&f3_definition,
	&f4_definition,
	&f5_definition,
	&rasterizer_transparent_pixel_counter_definition,
	&debug_no_frustum_clip_definition,
	&debug_frustum_definition,
	&debug_bink_definition,
	&screenshot_size_definition,
	&screenshot_count_definition,
	&terminal_render_definition,
	&console_dump_to_file_definition,
	&player_spawn_count_definition,
	&debug_object_garbage_collection_definition,
	&debug_render_freeze_definition,
	&debug_no_drawing_definition,
	&debug_input_target_definition,
	&temporary_hud_definition,
	&debug_leaf_index_definition,
	&debug_leaf_portal_index_definition,
	&debug_leaf_portals_definition,
	&debug_unit_all_animations_definition,
	&debug_unit_animations_definition,
	&debug_unit_illumination_definition,
	&debug_damage_taken_definition,
	&cheat_deathless_player_definition,
	&cheat_jetpack_definition,
	&cheat_infinite_ammo_definition,
	&cheat_bottomless_clip_definition,
	&cheat_bump_possession_definition,
	&cheat_super_jump_definition,
	&cheat_reflexive_damage_effects_definition,
	&cheat_medusa_definition,
	&cheat_omnipotent_definition,
	&cheat_controller_definition,
	&effects_corpse_nonviolent_definition,
	&debug_effects_nonviolent_definition,
	&debug_sound_cache_definition,
	&debug_sound_definition,
	&debug_looping_sound_definition,
	&debug_sound_channels_definition,
	&loud_dialog_hack_definition,
	&sound_gain_under_dialog_definition,
	&debug_sound_environment_definition,
	&object_light_ambient_base_definition,
	&object_light_ambient_scale_definition,
	&object_light_secondary_scale_definition,
	&object_light_interpolate_definition,
	&model_animation_compression_definition,
	&model_animation_data_compressed_size_definition,
	&model_animation_data_uncompressed_size_definition,
	&model_animation_data_compression_savings_in_bytes_definition,
	&model_animation_data_compression_savings_in_bytes_at_import_definition,
	&model_animation_data_compression_savings_in_percent_definition,
	&model_animation_bullshit0_definition,
	&model_animation_bullshit1_definition,
	&model_animation_bullshit2_definition,
	&model_animation_bullshit3_definition,
	&rider_ejection_definition,
	&stun_enable_definition,
	&debug_sprites_definition,
	&debug_portals_definition,
	&debug_inactive_objects_definition,
	&render_contrails_definition,
	&render_particles_definition,
	&render_psystems_definition,
	&render_wsystems_definition,
	&debug_objects_definition,
	&debug_objects_position_velocity_definition,
	&debug_objects_root_node_definition,
	&debug_objects_bounding_spheres_definition,
	&debug_objects_collision_models_definition,
	&debug_objects_physics_definition,
	&debug_objects_names_definition,
	&debug_objects_pathfinding_spheres_definition,
	&debug_objects_unit_vectors_definition,
	&debug_objects_unit_seats_definition,
	&debug_objects_unit_mouth_apeture_definition,
	&debug_objects_biped_physics_pills_definition,
	&debug_objects_biped_autoaim_pills_definition,
	&debug_objects_vehicle_powered_mass_points_definition,
	&debug_objects_devices_definition,
	&render_model_nodes_definition,
	&render_model_vertex_counts_definition,
	&render_model_index_counts_definition,
	&render_model_markers_definition,
	&render_model_no_geometry_definition,
	&render_shadows_definition,
	&debug_damage_definition,
	&debug_scripting_definition,
	&debug_trigger_volumes_definition,
	&debug_point_physics_definition,
	&debug_physics_disable_penetration_freeze_definition,
	&debug_motion_sensor_draw_all_units_definition,
	&collision_debug_definition,
	&collision_debug_spray_definition,
	&collision_debug_features_definition,
	&collision_debug_repeat_definition,
	&collision_debug_flag_front_facing_surfaces_definition,
	&collision_debug_flag_back_facing_surfaces_definition,
	&collision_debug_flag_ignore_two_sided_surfaces_definition,
	&collision_debug_flag_ignore_invisible_surfaces_definition,
	&collision_debug_flag_ignore_breakable_surfaces_definition,
	&collision_debug_flag_structure_definition,
	&collision_debug_flag_media_definition,
	&collision_debug_flag_objects_definition,
	&collision_debug_flag_objects_bipeds_definition,
	&collision_debug_flag_objects_vehicles_definition,
	&collision_debug_flag_objects_weapons_definition,
	&collision_debug_flag_objects_equipment_definition,
	&collision_debug_flag_objects_projectiles_definition,
	&collision_debug_flag_objects_scenery_definition,
	&collision_debug_flag_objects_machines_definition,
	&collision_debug_flag_objects_controls_definition,
	&collision_debug_flag_objects_light_fixtures_definition,
	&collision_debug_flag_objects_placeholders_definition,
	&collision_debug_flag_try_to_keep_location_valid_definition,
	&collision_debug_flag_skip_passthrough_bipeds_definition,
	&collision_debug_flag_use_vehicle_physics_definition,
	&collision_debug_point_x_definition,
	&collision_debug_point_y_definition,
	&collision_debug_point_z_definition,
	&collision_debug_vector_i_definition,
	&collision_debug_vector_j_definition,
	&collision_debug_vector_k_definition,
	&collision_debug_length_definition,
	&collision_debug_width_definition,
	&collision_debug_height_definition,
	&collision_debug_phantom_bsp_definition,
	&collision_log_render_definition,
	&collision_log_detailed_definition,
	&collision_log_extended_definition,
	&collision_log_totals_only_definition,
	&collision_log_time_definition,
	&debug_obstacle_path_definition,
	&debug_obstacle_path_on_failure_definition,
	&debug_obstacle_path_start_point_x_definition,
	&debug_obstacle_path_start_point_y_definition,
	&debug_obstacle_path_start_surface_index_definition,
	&debug_obstacle_path_goal_point_x_definition,
	&debug_obstacle_path_goal_point_y_definition,
	&debug_obstacle_path_goal_surface_index_definition,
	&debug_camera_definition,
	&debug_player_definition,
	&debug_structure_definition,
	&debug_bsp_definition,
	&debug_input_definition,
	&debug_permanent_decals_definition,
	&debug_fog_planes_definition,
	&decals_definition,
	&debug_decals_definition,
	&debug_object_lights_definition,
	&debug_lights_definition,
	&debug_biped_physics_definition,
	&debug_biped_skip_update_definition,
	&debug_biped_skip_collision_definition,
	&debug_biped_limp_body_disable_definition,
	&debug_collision_skip_objects_definition,
	&debug_collision_skip_vectors_definition,
	&debug_material_effects_definition,
	&weather_definition,
	&breakable_surfaces_definition,
	&decals_definition,
	&profile_graph_definition,
	&profile_display_definition,
	&profile_timebase_ticks_definition,
	&profile_dump_frames_definition,
	&profile_dump_lost_frames_definition,
	&recover_saved_games_hack_definition,
	&radiosity_quality_definition,
	&radiosity_step_count_definition,
	&radiosity_lines_definition,
	&radiosity_normals_definition,
	&structures_use_pvs_for_vs_definition,
	&debug_texture_cache_definition,
	&debug_detail_objects_definition,
	&ai_render_definition,
	&ai_render_all_actors_definition,
	&ai_render_inactive_actors_definition,
	&ai_render_lineoffire_crouching_definition,
	&ai_render_lineoffire_definition,
	&ai_render_lineofsight_definition,
	&ai_render_ballistic_lineoffire_definition,
	&ai_render_encounter_activeregion_definition,
	&ai_render_vision_cones_definition,
	&ai_render_current_state_definition,
	&ai_render_detailed_state_definition,
	&ai_render_props_definition,
	&ai_render_props_web_definition,
	&ai_render_props_no_friends_definition,
	&ai_render_props_target_weight_definition,
	&ai_render_props_unreachable_definition,
	&ai_render_props_unopposable_definition,
	&ai_render_idle_look_definition,
	&ai_render_support_surfaces_definition,
	&ai_render_recent_damage_definition,
	&ai_render_threats_definition,
	&ai_render_emotions_definition,
	&ai_render_audibility_definition,
	&ai_render_aiming_vectors_definition,
	&ai_render_secondary_looking_definition,
	&ai_render_targets_definition,
	&ai_render_targets_last_visible_definition,
	&ai_render_states_definition,
	&ai_render_vitality_definition,
	&ai_render_active_cover_seeking_definition,
	&ai_render_evaluations_definition,
	&ai_render_pursuit_definition,
	&ai_render_shooting_definition,
	&ai_render_trigger_definition,
	&ai_render_projectile_aiming_definition,
	&ai_render_aiming_validity_definition,
	&ai_render_speech_definition,
	&ai_render_teams_definition,
	&ai_render_player_ratings_definition,
	&ai_render_spatial_effects_definition,
	&ai_render_firing_positions_definition,
	&ai_render_gun_positions_definition,
	&ai_render_burst_geometry_definition,
	&ai_render_vehicle_avoidance_definition,
	&ai_render_vehicles_enterable_definition,
	&ai_render_melee_check_definition,
	&ai_render_dialogue_variants_definition,
	&ai_render_grenade_decisions_definition,
	&ai_render_danger_zones_definition,
	&ai_render_charge_decisions_definition,
	&ai_render_control_definition,
	&ai_render_activation_definition,
	&ai_render_paths_definition,
	&ai_render_paths_selected_only_definition,
	&ai_render_paths_destination_definition,
	&ai_render_paths_current_definition,
	&ai_render_paths_failed_definition,
	&ai_render_paths_raw_definition,
	&ai_render_paths_smoothed_definition,
	&ai_render_paths_avoided_definition,
	&ai_render_paths_avoidance_segment_definition,
	&ai_render_paths_avoidance_obstacles_definition,
	&ai_render_paths_avoidance_search_definition,
	&ai_render_paths_nodes_definition,
	&ai_render_paths_nodes_all_definition,
	&ai_render_paths_nodes_polygons_definition,
	&ai_render_paths_nodes_costs_definition,
	&ai_render_paths_nodes_closest_definition,
	&ai_render_player_aiming_blocked_definition,
	&ai_render_vector_avoidance_definition,
	&ai_render_vector_avoidance_rays_definition,
	&ai_render_vector_avoidance_sense_t_definition,
	&ai_render_vector_avoidance_avoid_t_definition,
	&ai_render_vector_avoidance_clear_time_definition,
	&ai_render_vector_avoidance_weights_definition,
	&ai_render_vector_avoidance_objects_definition,
	&ai_render_vector_avoidance_intermediate_definition,
	&ai_render_postcombat_definition,
	&ai_print_pursuit_checks_definition,
	&ai_print_rules_definition,
	&ai_print_rule_values_definition,
	&ai_print_major_upgrade_definition,
	&ai_print_respawn_definition,
	&ai_print_evaluation_statistics_definition,
	&ai_print_communication_definition,
	&ai_print_communication_player_definition,
	&ai_print_vocalizations_definition,
	&ai_print_placement_definition,
	&ai_print_speech_definition,
	&ai_print_speech_timers_definition,
	&ai_print_allegiance_definition,
	&ai_print_lost_speech_definition,
	&ai_print_migration_definition,
	&ai_print_automatic_migration_definition,
	&ai_print_scripting_definition,
	&ai_print_surprise_definition,
	&ai_print_command_lists_definition,
	&ai_print_damage_modifiers_definition,
	&ai_print_secondary_looking_definition,
	&ai_print_oversteer_definition,
	&ai_print_conversations_definition,
	&ai_print_killing_sprees_definition,
	&ai_print_acknowledgement_definition,
	&ai_print_unfinished_paths_definition,
	&ai_print_bsp_transition_definition,
	&ai_print_uncovering_definition,
	&ai_profile_disable_definition,
	&ai_profile_random_definition,
	&ai_show_definition,
	&ai_show_stats_definition,
	&ai_show_actors_definition,
	&ai_show_swarms_definition,
	&ai_show_paths_definition,
	&ai_show_line_of_sight_definition,
	&ai_show_prop_types_definition,
	&ai_show_sound_distance_definition,
	&ai_debug_fast_los_definition,
	&ai_debug_oversteer_disable_definition,
	&ai_debug_evaluate_all_positions_definition,
	&ai_debug_path_definition,
	&ai_debug_path_start_freeze_definition,
	&ai_debug_path_end_freeze_definition,
	&ai_debug_path_flood_definition,
	&ai_debug_path_maximum_radius_definition,
	&ai_debug_path_attractor_definition,
	&ai_debug_path_attractor_radius_definition,
	&ai_debug_path_attractor_weight_definition,
	&ai_debug_path_accept_radius_definition,
	&ai_debug_ballistic_lineoffire_freeze_definition,
	&ai_debug_communication_random_disabled_definition,
	&ai_debug_communication_timeout_disabled_definition,
	&ai_debug_communication_unit_repeat_disabled_definition,
	&ai_debug_communication_focus_enable_definition,
	&ai_debug_blind_definition,
	&ai_debug_deaf_definition,
	&ai_debug_invisible_player_definition,
	&ai_debug_ignore_player_definition,
	&ai_debug_flee_always_definition,
	&ai_debug_force_all_active_definition,
	&ai_debug_disable_wounded_sounds_definition,
	&ai_debug_force_vocalizations_definition,
	&ai_debug_force_crouch_definition,
	&ai_debug_path_disable_smoothing_definition,
	&ai_debug_path_disable_obstacle_avoidance_definition,
	&ai_fix_defending_guard_firing_positions_definition,
	&ai_fix_actor_variants_definition,
	&controls_enable_crouch_definition,
	&controls_swapped_definition,
	&controls_enable_doubled_spin_definition,
	&controls_swap_doubled_spin_state_definition,
	&player0_look_yaw_rate_definition,
	&player1_look_yaw_rate_definition,
	&player2_look_yaw_rate_definition,
	&player3_look_yaw_rate_definition,
	&player0_look_pitch_rate_definition,
	&player1_look_pitch_rate_definition,
	&player2_look_pitch_rate_definition,
	&player3_look_pitch_rate_definition,
	&player_autoaim_definition,
	&player_magnetism_definition,
	&debug_player_teleport_definition,
	&texture_cache_graph_definition,
	&texture_cache_list_definition,
	&director_camera_switch_fast_definition,
	&debug_recording_definition,
	&debug_recording_newlines_definition,
	&debug_player_color_definition,
	&debug_framerate_definition,
	&display_framerate_definition,
	&display_vblank_deltas_definition,
	&framerate_throttle_definition,
	&framerate_lock_definition,
	&display_precache_progress_definition,
	&debug_game_save_definition,
	&allow_out_of_sync_definition,
	&global_connection_dont_timeout_definition,
	&find_all_fucked_up_shit_definition,
	&run_game_scripts_definition,
};

/* port: the external globals a map's scripts may set, by
hs_external_globals' index (hs_scenario_functions_check). Any may be read
(that copies a value), and the console's expressions may set any. Allowed
are the three the shipped maps' scripts set (c10's near clip distance and
weather, b30's cheat_deathless_player) and a few of gameplay's. Not
allowed are the rasterizer's, the debug displays and switches, profiling,
cheats, the console, the network, and the player's own settings. A global
added to the table may not be set until it is listed here */
static boolean const hs_external_global_settable_in_maps[]=
{
	/* the camera's clip distances (c10 sets the near one) */
	TRUE, /* rasterizer_near_clip_distance */
	TRUE, /* rasterizer_far_clip_distance */
	TRUE, /* rasterizer_first_person_weapon_near_clip_distance */
	TRUE, /* rasterizer_first_person_weapon_far_clip_distance */

	/* the rasterizer: buffers, the frame rate, the frame bounds, its debug and profile toggles */
	FALSE, /* rasterizer_pushbuffer_size */
	FALSE, /* rasterizer_pushbuffer_kickoff_size */
	FALSE, /* rasterizer_floating_point_zbuffer */
	FALSE, /* rasterizer_framerate_throttle */
	FALSE, /* rasterizer_framerate_stabilization */
	FALSE, /* rasterizer_refresh_rate */
	FALSE, /* rasterizer_frame_bounds_left */
	FALSE, /* rasterizer_frame_bounds_right */
	FALSE, /* rasterizer_frame_bounds_top */
	FALSE, /* rasterizer_frame_bounds_bottom */
	FALSE, /* rasterizer_stats */
	FALSE, /* rasterizer_mode */
	FALSE, /* rasterizer_wireframe */
	FALSE, /* rasterizer_smart */
	FALSE, /* rasterizer_debug_model_vertices */
	FALSE, /* rasterizer_debug_model_lod */
	FALSE, /* rasterizer_debug_transparents */
	FALSE, /* rasterizer_debug_meter_shader */
	FALSE, /* rasterizer_models */
	FALSE, /* rasterizer_model_transparents */
	FALSE, /* rasterizer_draw_first_person_weapon_first */
	FALSE, /* rasterizer_stencil_mask */
	FALSE, /* rasterizer_environment */
	FALSE, /* rasterizer_environment_lightmaps */
	FALSE, /* rasterizer_environment_shadows */
	FALSE, /* rasterizer_environment_diffuse_lights */
	FALSE, /* rasterizer_environment_diffuse_textures */
	FALSE, /* rasterizer_environment_decals */
	FALSE, /* rasterizer_environment_specular_lights */
	FALSE, /* rasterizer_environment_specular_lightmaps */
	FALSE, /* rasterizer_environment_reflection_lightmap_mask */
	FALSE, /* rasterizer_environment_reflection_mirrors */
	FALSE, /* rasterizer_environment_reflections */
	FALSE, /* rasterizer_environment_transparents */
	FALSE, /* rasterizer_environment_fog */
	FALSE, /* rasterizer_environment_fog_screen */
	FALSE, /* rasterizer_water */
	FALSE, /* rasterizer_lens_flares */
	FALSE, /* rasterizer_dynamic_unlit_geometry */
	FALSE, /* rasterizer_dynamic_lit_geometry */
	FALSE, /* rasterizer_dynamic_screen_geometry */
	FALSE, /* rasterizer_hud_motion_sensor */
	FALSE, /* rasterizer_detail_objects */
	FALSE, /* rasterizer_debug_geometry */
	FALSE, /* rasterizer_debug_geometry_multipass */
	FALSE, /* rasterizer_fog_atmosphere */
	FALSE, /* rasterizer_fog_plane */
	FALSE, /* rasterizer_bump_mapping */
	FALSE, /* rasterizer_lightmap_ambient */
	FALSE, /* rasterizer_lightmap_mode */
	FALSE, /* rasterizer_lightmaps_incident_radiosity */
	FALSE, /* rasterizer_lightmaps_filtering */
	FALSE, /* rasterizer_model_lighting_ambient */
	FALSE, /* rasterizer_environment_alpha_testing */
	FALSE, /* rasterizer_environment_specular_mask */
	FALSE, /* rasterizer_shadows_convolution */
	FALSE, /* rasterizer_shadows_debug */
	FALSE, /* rasterizer_water_mipmapping */
	FALSE, /* rasterizer_active_camouflage */
	FALSE, /* rasterizer_active_camouflage_multipass */
	FALSE, /* rasterizer_plasma_energy */
	FALSE, /* rasterizer_lens_flares_occlusion */
	FALSE, /* rasterizer_lens_flares_occlusion_debug */
	FALSE, /* rasterizer_ray_of_buddha */
	FALSE, /* rasterizer_screen_flashes */
	FALSE, /* rasterizer_screen_effects */
	FALSE, /* rasterizer_DXTC_noise */
	FALSE, /* rasterizer_soft_filter */
	FALSE, /* rasterizer_secondary_render_target_debug */
	FALSE, /* rasterizer_profile_log */
	FALSE, /* rasterizer_detail_objects_offset_multiplier */
	FALSE, /* rasterizer_zbias */
	FALSE, /* rasterizer_zoffset */
	FALSE, /* force_all_player_views_to_default_player */
	FALSE, /* rasterizer_safe_frame_bounds */
	FALSE, /* freeze_flying_camera */
	FALSE, /* rasterizer_zsprites */
	FALSE, /* rasterizer_filthy_decal_fog_hack */
	FALSE, /* rasterizer_splitscreen_VB_optimization */
	FALSE, /* rasterizer_profile_print_locks */
	FALSE, /* rasterizer_profile_objectlock_time */

	/* the rasterizer: tuning values */
	FALSE, /* pad3 */
	FALSE, /* pad3_scale */
	FALSE, /* f0 */
	FALSE, /* f1 */
	FALSE, /* f2 */
	FALSE, /* f3 */
	FALSE, /* f4 */
	FALSE, /* f5 */

	/* debug displays, screenshots, the console */
	FALSE, /* rasterizer_transparent_pixel_counter */
	FALSE, /* debug_no_frustum_clip */
	FALSE, /* debug_frustum */
	FALSE, /* debug_bink */
	FALSE, /* screenshot_size */
	FALSE, /* screenshot_count */
	FALSE, /* terminal_render */
	FALSE, /* console_dump_to_file */
	FALSE, /* player_spawn_count */
	FALSE, /* debug_object_garbage_collection */
	FALSE, /* debug_render_freeze */
	FALSE, /* debug_no_drawing */
	FALSE, /* debug_input_target */
	FALSE, /* temporary_hud */
	FALSE, /* debug_leaf_index */
	FALSE, /* debug_leaf_portal_index */
	FALSE, /* debug_leaf_portals */
	FALSE, /* debug_unit_all_animations */
	FALSE, /* debug_unit_animations */
	FALSE, /* debug_unit_illumination */
	FALSE, /* debug_damage_taken */

	/* cheats: b30 sets cheat_deathless_player (around its cinematics) */
	TRUE, /* cheat_deathless_player */
	FALSE, /* cheat_jetpack */
	FALSE, /* cheat_infinite_ammo */
	FALSE, /* cheat_bottomless_clip */
	FALSE, /* cheat_bump_possession */
	FALSE, /* cheat_super_jump */
	FALSE, /* cheat_reflexive_damage_effects */
	FALSE, /* cheat_medusa */
	FALSE, /* cheat_omnipotent */
	FALSE, /* cheat_controller */

	/* the player's content setting, sound debugging and tuning */
	FALSE, /* effects_corpse_nonviolent */
	FALSE, /* debug_effects_nonviolent */
	FALSE, /* debug_sound_cache */
	FALSE, /* debug_sound */
	FALSE, /* debug_looping_sound */
	FALSE, /* debug_sound_channels */
	FALSE, /* loud_dialog_hack */
	FALSE, /* sound_gain_under_dialog */
	FALSE, /* debug_sound_environment */

	/* object lighting tuning, animation compression statistics */
	FALSE, /* object_light_ambient_base */
	FALSE, /* object_light_ambient_scale */
	FALSE, /* object_light_secondary_scale */
	FALSE, /* object_light_interpolate */
	FALSE, /* model_animation_compression */
	FALSE, /* model_animation_data_compressed_size */
	FALSE, /* model_animation_data_uncompressed_size */
	FALSE, /* model_animation_data_compression_savings_in_bytes */
	FALSE, /* model_animation_data_compression_savings_in_bytes_at_import */
	FALSE, /* model_animation_data_compression_savings_in_percent */
	FALSE, /* model_animation_bullshit0 */
	FALSE, /* model_animation_bullshit1 */
	FALSE, /* model_animation_bullshit2 */
	FALSE, /* model_animation_bullshit3 */

	/* gameplay */
	TRUE, /* rider_ejection */
	TRUE, /* stun_enable */

	/* debug displays: sprites, portals, objects, models, damage, scripting */
	FALSE, /* debug_sprites */
	FALSE, /* debug_portals */
	FALSE, /* debug_inactive_objects */
	FALSE, /* render_contrails */
	FALSE, /* render_particles */
	FALSE, /* render_psystems */
	FALSE, /* render_wsystems */
	FALSE, /* debug_objects */
	FALSE, /* debug_objects_position_velocity */
	FALSE, /* debug_objects_root_node */
	FALSE, /* debug_objects_bounding_spheres */
	FALSE, /* debug_objects_collision_models */
	FALSE, /* debug_objects_physics */
	FALSE, /* debug_objects_names */
	FALSE, /* debug_objects_pathfinding_spheres */
	FALSE, /* debug_objects_unit_vectors */
	FALSE, /* debug_objects_unit_seats */
	FALSE, /* debug_objects_unit_mouth_apeture */
	FALSE, /* debug_objects_biped_physics_pills */
	FALSE, /* debug_objects_biped_autoaim_pills */
	FALSE, /* debug_objects_vehicle_powered_mass_points */
	FALSE, /* debug_objects_devices */
	FALSE, /* render_model_nodes */
	FALSE, /* render_model_vertex_counts */
	FALSE, /* render_model_index_counts */
	FALSE, /* render_model_markers */
	FALSE, /* render_model_no_geometry */
	FALSE, /* render_shadows */
	FALSE, /* debug_damage */
	FALSE, /* debug_scripting */
	FALSE, /* debug_trigger_volumes */

	/* physics and collision debugging */
	FALSE, /* debug_point_physics */
	FALSE, /* debug_physics_disable_penetration_freeze */
	FALSE, /* debug_motion_sensor_draw_all_units */
	FALSE, /* collision_debug */
	FALSE, /* collision_debug_spray */
	FALSE, /* collision_debug_features */
	FALSE, /* collision_debug_repeat */
	FALSE, /* collision_debug_flag_front_facing_surfaces */
	FALSE, /* collision_debug_flag_back_facing_surfaces */
	FALSE, /* collision_debug_flag_ignore_two_sided_surfaces */
	FALSE, /* collision_debug_flag_ignore_invisible_surfaces */
	FALSE, /* collision_debug_flag_ignore_breakable_surfaces */
	FALSE, /* collision_debug_flag_structure */
	FALSE, /* collision_debug_flag_media */
	FALSE, /* collision_debug_flag_objects */
	FALSE, /* collision_debug_flag_objects_bipeds */
	FALSE, /* collision_debug_flag_objects_vehicles */
	FALSE, /* collision_debug_flag_objects_weapons */
	FALSE, /* collision_debug_flag_objects_equipment */
	FALSE, /* collision_debug_flag_objects_projectiles */
	FALSE, /* collision_debug_flag_objects_scenery */
	FALSE, /* collision_debug_flag_objects_machines */
	FALSE, /* collision_debug_flag_objects_controls */
	FALSE, /* collision_debug_flag_objects_light_fixtures */
	FALSE, /* collision_debug_flag_objects_placeholders */
	FALSE, /* collision_debug_flag_try_to_keep_location_valid */
	FALSE, /* collision_debug_flag_skip_passthrough_bipeds */
	FALSE, /* collision_debug_flag_use_vehicle_physics */
	FALSE, /* collision_debug_point_x */
	FALSE, /* collision_debug_point_y */
	FALSE, /* collision_debug_point_z */
	FALSE, /* collision_debug_vector_i */
	FALSE, /* collision_debug_vector_j */
	FALSE, /* collision_debug_vector_k */
	FALSE, /* collision_debug_length */
	FALSE, /* collision_debug_width */
	FALSE, /* collision_debug_height */
	FALSE, /* collision_debug_phantom_bsp */
	FALSE, /* collision_log_render */
	FALSE, /* collision_log_detailed */
	FALSE, /* collision_log_extended */
	FALSE, /* collision_log_totals_only */
	FALSE, /* collision_log_time */

	/* path and camera debugging, the world's debug displays */
	FALSE, /* debug_obstacle_path */
	FALSE, /* debug_obstacle_path_on_failure */
	FALSE, /* debug_obstacle_path_start_point_x */
	FALSE, /* debug_obstacle_path_start_point_y */
	FALSE, /* debug_obstacle_path_start_surface_index */
	FALSE, /* debug_obstacle_path_goal_point_x */
	FALSE, /* debug_obstacle_path_goal_point_y */
	FALSE, /* debug_obstacle_path_goal_surface_index */
	FALSE, /* debug_camera */
	FALSE, /* debug_player */
	FALSE, /* debug_structure */
	FALSE, /* debug_bsp */
	FALSE, /* debug_input */
	FALSE, /* debug_permanent_decals */
	FALSE, /* debug_fog_planes */
	FALSE, /* decals */
	FALSE, /* debug_decals */
	FALSE, /* debug_object_lights */
	FALSE, /* debug_lights */
	FALSE, /* debug_biped_physics */
	FALSE, /* debug_biped_skip_update */
	FALSE, /* debug_biped_skip_collision */
	FALSE, /* debug_biped_limp_body_disable */
	FALSE, /* debug_collision_skip_objects */
	FALSE, /* debug_collision_skip_vectors */
	FALSE, /* debug_material_effects */

	/* the weather (c10 sets it), breakable surfaces, decals */
	TRUE, /* weather */
	FALSE, /* breakable_surfaces */
	FALSE, /* decals */

	/* profiling, the saved-game hack, radiosity, the texture cache */
	FALSE, /* profile_graph */
	FALSE, /* profile_display */
	FALSE, /* profile_timebase_ticks */
	FALSE, /* profile_dump_frames */
	FALSE, /* profile_dump_lost_frames */
	FALSE, /* recover_saved_games_hack */
	FALSE, /* radiosity_quality */
	FALSE, /* radiosity_step_count */
	FALSE, /* radiosity_lines */
	FALSE, /* radiosity_normals */
	FALSE, /* structures_use_pvs_for_vs */
	FALSE, /* debug_texture_cache */
	FALSE, /* debug_detail_objects */

	/* AI debug displays, prints and switches */
	FALSE, /* ai_render */
	FALSE, /* ai_render_all_actors */
	FALSE, /* ai_render_inactive_actors */
	FALSE, /* ai_render_lineoffire_crouching */
	FALSE, /* ai_render_lineoffire */
	FALSE, /* ai_render_lineofsight */
	FALSE, /* ai_render_ballistic_lineoffire */
	FALSE, /* ai_render_encounter_activeregion */
	FALSE, /* ai_render_vision_cones */
	FALSE, /* ai_render_current_state */
	FALSE, /* ai_render_detailed_state */
	FALSE, /* ai_render_props */
	FALSE, /* ai_render_props_web */
	FALSE, /* ai_render_props_no_friends */
	FALSE, /* ai_render_props_target_weight */
	FALSE, /* ai_render_props_unreachable */
	FALSE, /* ai_render_props_unopposable */
	FALSE, /* ai_render_idle_look */
	FALSE, /* ai_render_support_surfaces */
	FALSE, /* ai_render_recent_damage */
	FALSE, /* ai_render_threats */
	FALSE, /* ai_render_emotions */
	FALSE, /* ai_render_audibility */
	FALSE, /* ai_render_aiming_vectors */
	FALSE, /* ai_render_secondary_looking */
	FALSE, /* ai_render_targets */
	FALSE, /* ai_render_targets_last_visible */
	FALSE, /* ai_render_states */
	FALSE, /* ai_render_vitality */
	FALSE, /* ai_render_active_cover_seeking */
	FALSE, /* ai_render_evaluations */
	FALSE, /* ai_render_pursuit */
	FALSE, /* ai_render_shooting */
	FALSE, /* ai_render_trigger */
	FALSE, /* ai_render_projectile_aiming */
	FALSE, /* ai_render_aiming_validity */
	FALSE, /* ai_render_speech */
	FALSE, /* ai_render_teams */
	FALSE, /* ai_render_player_ratings */
	FALSE, /* ai_render_spatial_effects */
	FALSE, /* ai_render_firing_positions */
	FALSE, /* ai_render_gun_positions */
	FALSE, /* ai_render_burst_geometry */
	FALSE, /* ai_render_vehicle_avoidance */
	FALSE, /* ai_render_vehicles_enterable */
	FALSE, /* ai_render_melee_check */
	FALSE, /* ai_render_dialogue_variants */
	FALSE, /* ai_render_grenade_decisions */
	FALSE, /* ai_render_danger_zones */
	FALSE, /* ai_render_charge_decisions */
	FALSE, /* ai_render_control */
	FALSE, /* ai_render_activation */
	FALSE, /* ai_render_paths */
	FALSE, /* ai_render_paths_selected_only */
	FALSE, /* ai_render_paths_destination */
	FALSE, /* ai_render_paths_current */
	FALSE, /* ai_render_paths_failed */
	FALSE, /* ai_render_paths_raw */
	FALSE, /* ai_render_paths_smoothed */
	FALSE, /* ai_render_paths_avoided */
	FALSE, /* ai_render_paths_avoidance_segment */
	FALSE, /* ai_render_paths_avoidance_obstacles */
	FALSE, /* ai_render_paths_avoidance_search */
	FALSE, /* ai_render_paths_nodes */
	FALSE, /* ai_render_paths_nodes_all */
	FALSE, /* ai_render_paths_nodes_polygons */
	FALSE, /* ai_render_paths_nodes_costs */
	FALSE, /* ai_render_paths_nodes_closest */
	FALSE, /* ai_render_player_aiming_blocked */
	FALSE, /* ai_render_vector_avoidance */
	FALSE, /* ai_render_vector_avoidance_rays */
	FALSE, /* ai_render_vector_avoidance_sense_t */
	FALSE, /* ai_render_vector_avoidance_avoid_t */
	FALSE, /* ai_render_vector_avoidance_clear_time */
	FALSE, /* ai_render_vector_avoidance_weights */
	FALSE, /* ai_render_vector_avoidance_objects */
	FALSE, /* ai_render_vector_avoidance_intermediate */
	FALSE, /* ai_render_postcombat */
	FALSE, /* ai_print_pursuit_checks */
	FALSE, /* ai_print_rules */
	FALSE, /* ai_print_rule_values */
	FALSE, /* ai_print_major_upgrade */
	FALSE, /* ai_print_respawn */
	FALSE, /* ai_print_evaluation_statistics */
	FALSE, /* ai_print_communication */
	FALSE, /* ai_print_communication_player */
	FALSE, /* ai_print_vocalizations */
	FALSE, /* ai_print_placement */
	FALSE, /* ai_print_speech */
	FALSE, /* ai_print_speech_timers */
	FALSE, /* ai_print_allegiance */
	FALSE, /* ai_print_lost_speech */
	FALSE, /* ai_print_migration */
	FALSE, /* ai_print_automatic_migration */
	FALSE, /* ai_print_scripting */
	FALSE, /* ai_print_surprise */
	FALSE, /* ai_print_command_lists */
	FALSE, /* ai_print_damage_modifiers */
	FALSE, /* ai_print_secondary_looking */
	FALSE, /* ai_print_oversteer */
	FALSE, /* ai_print_conversations */
	FALSE, /* ai_print_killing_sprees */
	FALSE, /* ai_print_acknowledgement */
	FALSE, /* ai_print_unfinished_paths */
	FALSE, /* ai_print_bsp_transition */
	FALSE, /* ai_print_uncovering */
	FALSE, /* ai_profile_disable */
	FALSE, /* ai_profile_random */
	FALSE, /* ai_show */
	FALSE, /* ai_show_stats */
	FALSE, /* ai_show_actors */
	FALSE, /* ai_show_swarms */
	FALSE, /* ai_show_paths */
	FALSE, /* ai_show_line_of_sight */
	FALSE, /* ai_show_prop_types */
	FALSE, /* ai_show_sound_distance */
	FALSE, /* ai_debug_fast_los */
	FALSE, /* ai_debug_oversteer_disable */
	FALSE, /* ai_debug_evaluate_all_positions */
	FALSE, /* ai_debug_path */
	FALSE, /* ai_debug_path_start_freeze */
	FALSE, /* ai_debug_path_end_freeze */
	FALSE, /* ai_debug_path_flood */
	FALSE, /* ai_debug_path_maximum_radius */
	FALSE, /* ai_debug_path_attractor */
	FALSE, /* ai_debug_path_attractor_radius */
	FALSE, /* ai_debug_path_attractor_weight */
	FALSE, /* ai_debug_path_accept_radius */
	FALSE, /* ai_debug_ballistic_lineoffire_freeze */
	FALSE, /* ai_debug_communication_random_disabled */
	FALSE, /* ai_debug_communication_timeout_disabled */
	FALSE, /* ai_debug_communication_unit_repeat_disabled */
	FALSE, /* ai_debug_communication_focus_enable */
	FALSE, /* ai_debug_blind */
	FALSE, /* ai_debug_deaf */
	FALSE, /* ai_debug_invisible_player */
	FALSE, /* ai_debug_ignore_player */
	FALSE, /* ai_debug_flee_always */
	FALSE, /* ai_debug_force_all_active */
	FALSE, /* ai_debug_disable_wounded_sounds */
	FALSE, /* ai_debug_force_vocalizations */
	FALSE, /* ai_debug_force_crouch */
	FALSE, /* ai_debug_path_disable_smoothing */
	FALSE, /* ai_debug_path_disable_obstacle_avoidance */
	FALSE, /* ai_fix_defending_guard_firing_positions */
	FALSE, /* ai_fix_actor_variants */

	/* the player's controls, look rates and aiming assists */
	FALSE, /* controls_enable_crouch */
	FALSE, /* controls_swapped */
	FALSE, /* controls_enable_doubled_spin */
	FALSE, /* controls_swap_doubled_spin_state */
	FALSE, /* player0_look_yaw_rate */
	FALSE, /* player1_look_yaw_rate */
	FALSE, /* player2_look_yaw_rate */
	FALSE, /* player3_look_yaw_rate */
	FALSE, /* player0_look_pitch_rate */
	FALSE, /* player1_look_pitch_rate */
	FALSE, /* player2_look_pitch_rate */
	FALSE, /* player3_look_pitch_rate */
	FALSE, /* player_autoaim */
	FALSE, /* player_magnetism */

	/* debug toggles, the frame rate display, the network, the game's scripts */
	FALSE, /* debug_player_teleport */
	FALSE, /* texture_cache_graph */
	FALSE, /* texture_cache_list */
	FALSE, /* director_camera_switch_fast */
	FALSE, /* debug_recording */
	FALSE, /* debug_recording_newlines */
	FALSE, /* debug_player_color */
	FALSE, /* debug_framerate */
	FALSE, /* display_framerate */
	FALSE, /* display_vblank_deltas */
	FALSE, /* framerate_throttle */
	FALSE, /* framerate_lock */
	FALSE, /* display_precache_progress */
	FALSE, /* debug_game_save */
	FALSE, /* allow_out_of_sync */
	FALSE, /* global_connection_dont_timeout */
	FALSE, /* find_all_fucked_up_shit */
	FALSE, /* run_game_scripts */
};
typedef char verify_hs_external_global_settable_in_maps_size[
	NUMBEROF(hs_external_global_settable_in_maps) == NUMBEROF(hs_external_globals) ? 1 : -1];

/* ---------- public code */

/* port: whether a map's scripts may set the external global */
boolean hs_external_global_settable_by_maps(
	short global_index)
{
	/* port: MCC's separately scoped campaign controls (mcc_campaign.c) */
	if (mcc_campaign_global_settable(global_index))
		return TRUE;
	return global_index>=0 &&
		global_index<(short)NUMBEROF(hs_external_global_settable_in_maps) &&
		hs_external_global_settable_in_maps[global_index];
}

/* ---------- private code */
