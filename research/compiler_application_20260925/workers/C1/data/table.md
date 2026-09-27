| # | pool off | symbol | size | our tree | decl ext/tent | declaring header(s) | Jan refs obj/rel | HCEX 2011 | pool-order candidate (H_order+H_ref, NOT evidence) | verdict | established owner / classes |
|---:|---:|---|---:|---|---|---|---|---|---|---|---|
| 1 | - | `_rdata_00242ed0` | 28 | absent | 0/0 | - | 0/0 | - | n/a (vendor/linker) | **PLACED** | * Linker * (linker-generated; no source TU) [EC-LNK] |
| 2 | - | `_rdata_002b6a2c` | 60 | absent | 0/0 | - | 0/0 | - | n/a (vendor/linker) | **PLACED** | * Linker * (linker-generated; no source TU) [EC-LNK] |
| 3 | 0 | `_debug_sound_channels` | 1 | extern-only | 2/0 | - | 2/2 | unsigned char; pooled | H_ref FAILS; interval [0, 11] | **UNPLACED** | - |
| 4 | 1 | `_debug_looping_sound` | 1 | extern-only | 2/0 | - | 2/2 | unsigned char; pooled | 11 sound_manager | **UNPLACED** | - |
| 5 | 2 | `_debug_sound` | 1 | extern-only | 2/0 | sound_manager.h | 2/2 | unsigned char; pooled | 11 sound_manager | **UNPLACED** | - |
| 6 | 3 | `_loud_dialog_hack` | 1 | extern-only | 2/0 | - | 2/2 | unsigned char; pooled | 11 sound_manager | **UNPLACED** | - |
| 7 | 32 | `_sound_channels` | 6144 | extern-only | 1/0 | - | 1/23 | struct sound_channel_datum[0x100]; pooled | 11 sound_manager | **UNPLACED** | - |
| 8 | 6176 | `_looping_sound_data` | 4 | extern-only | 1/0 | - | 1/23 | struct data_array *; pooled | 11 sound_manager | **UNPLACED** | - |
| 9 | 6180 | `_sound_data` | 4 | extern-only | 1/0 | - | 1/56 | struct data_array *; pooled | 11 sound_manager | **UNPLACED** | - |
| 10 | 6184 | `_interrupt_result` | 4 | extern-only | 1/0 | - | 1/3 | absent | 13 sound_dsound_xbox | **UNPLACED** | - |
| 11 | 6208 | `_dsound_globals` | 30924 | extern-only | 1/0 | - | 1/138 | absent | 13 sound_dsound_xbox | **UNPLACED** | - |
| 12 | 37132 | `_sound_class_data` | 4 | extern-only | 1/0 | - | 1/6 | struct sound_class_datum *; pooled | 15 sound_classes | **UNPLACED** | - |
| 13 | 37152 | `_combined_pas` | 64 | COMMON game_sound | 0/1 | - | 1/4 | unsigned long[0x10]; pooled | 17 game_sound | **UNPLACED** | - |
| 14 | 37216 | `_game_sound_globals` | 4 | COMMON game_sound | 0/1 | - | 1/15 | struct game_sound_global_data *; pooled | 17 game_sound | **UNPLACED** | - |
| 15 | 37220 | `_game_looping_sound_data` | 4 | COMMON game_sound | 0/1 | - | 1/41 | struct data_array *; pooled | 17 game_sound | **UNPLACED** | - |
| 16 | 37224 | `_recover_saved_games_hack` | 1 | COMMON game_state | 2/1 | game_state.h | 3/3 | unsigned char; pooled | 26 game_state_xbox, 27 game_state | **UNPLACED** | - |
| 17 | 37225 | `_debug_sound_cache` | 1 | extern-only | 2/0 | - | 2/2 | unsigned char; pooled | 29 xbox_sound_cache | **UNPLACED** | - |
| 18 | 37226 | `_assertion_count` | 2 | extern-only | 1/0 | - | 1/3 | short; discarded | 29 xbox_sound_cache | **UNPLACED** | - |
| 19 | 37228 | `_debug_sound_reference_counts` | 1 | extern-only | 1/0 | - | 1/2 | unsigned char; DEF pc_sound_cache.obj | 29 xbox_sound_cache | **CONSTRAINED** | EC-HCEXDEF |
| 20 | 37232 | `_global_tag_instances` | 4 | extern-only | 1/0 | - | 1/8 | struct cache_file_tag_instance *; pooled | 34 cache_files | **UNPLACED** | - |
| 21 | 37236 | `_debug_objects_vehicle_powered_mass_points` | 1 | extern-only | 2/0 | - | 2/2 | absent | 35 vehicles | **UNPLACED** | - |
| 22 | 37237 | `_debug_objects_unit_mouth_apeture` | 1 | COMMON units | 2/1 | units.h | 2/2 | absent | 37 units | **UNPLACED** | - |
| 23 | 37238 | `_debug_objects_unit_seats` | 1 | COMMON units | 2/1 | units.h | 2/2 | absent | 37 units | **UNPLACED** | - |
| 24 | 37239 | `_debug_objects_unit_vectors` | 1 | COMMON units | 2/1 | units.h | 2/2 | absent | 37 units | **UNPLACED** | - |
| 25 | 37240 | `_stun_enable` | 1 | COMMON units | 2/1 | units.h | 2/2 | unsigned char; pooled | 37 units | **UNPLACED** | - |
| 26 | 37241 | `_debug_damage_taken` | 1 | COMMON units | 2/1 | units.h | 2/2 | unsigned char; discarded | 37 units | **UNPLACED** | - |
| 27 | 37242 | `_debug_unit_illumination` | 1 | COMMON units | 2/1 | units.h | 2/2 | unsigned char; discarded | 37 units | **UNPLACED** | - |
| 28 | 37243 | `_debug_unit_animations` | 1 | COMMON units | 2/1 | units.h | 2/5 | unsigned char; discarded | 37 units | **UNPLACED** | - |
| 29 | 37244 | `_debug_unit_all_animations` | 1 | COMMON units | 2/1 | units.h | 2/2 | unsigned char; discarded | 37 units | **UNPLACED** | - |
| 30 | 37245 | `_debug_objects_biped_autoaim_pills` | 1 | extern-only | 2/0 | - | 2/2 | absent | 42 bipeds | **UNPLACED** | - |
| 31 | 37246 | `_debug_objects_biped_physics_pills` | 1 | extern-only | 2/0 | - | 2/2 | absent | 42 bipeds | **UNPLACED** | - |
| 32 | 37248 | `_file_location_volume_names` | 512 | COMMON files (SIZE 256) | 1/1 | files.h | 1/2 | char[0x100][0x3]; discarded | 54 files | **UNPLACED** | - |
| 33 | 37760 | `_debug_fog_planes` | 1 | extern-only | 2/0 | - | 2/2 | unsigned char; discarded | 55 structures | **UNPLACED** | - |
| 34 | 37761 | `_structures_use_pvs_for_vs` | 1 | COMMON structure_visibility | 2/1 | structure_visibility.h | 2/5 | unsigned char; pooled | 56 structure_visibility | **UNPLACED** | - |
| 35 | 37762 | `_debug_portals` | 1 | COMMON structure_visibility | 2/1 | structure_visibility.h | 2/3 | unsigned char; discarded | 56 structure_visibility | **UNPLACED** | - |
| 36 | 37763 | `_debug_leaf_portals` | 1 | extern-only | 2/0 | leaf_map.h | 2/2 | unsigned char; pooled | 58 structure_render | **UNPLACED** | - |
| 37 | 37792 | `_window_globals` | 148 | extern-only | 1/0 | - | 1/1 | struct window_data; discarded | H_ref FAILS; interval [58, 74] | **UNPLACED** | - |
| 38 | 37952 | `_wind_globals` | 3340 | extern-only | 1/0 | - | 1/21 | struct <unnamed-tag>; pooled | 74 wind | **UNPLACED** | - |
| 39 | 41292 | `_debug_sound_environment` | 1 | extern-only | 2/0 | - | 2/2 | absent | 77 scenario | **UNPLACED** | - |
| 40 | 41296 | `_scenario_globals` | 4 | extern-only | 1/0 | scenario.h | 1/12 | struct scenario_global_data *; pooled | 77 scenario | **UNPLACED** | - |
| 41 | 41300 | `_global_game_globals` | 4 | COMMON scenario | 1/1 | scenario.h | 1/6 | struct game_globals *; pooled | 77 scenario | **UNPLACED** | - |
| 42 | 41304 | `_global_bsp3d` | 4 | COMMON scenario | 1/1 | scenario.h | 1/10 | struct bsp3d *; pooled | 77 scenario | **UNPLACED** | - |
| 43 | 41308 | `_global_collision_bsp` | 4 | COMMON scenario | 1/1 | scenario.h | 2/6 | struct collision_bsp *; pooled | 77 scenario | **UNPLACED** | - |
| 44 | 41312 | `_global_structure_bsp` | 4 | COMMON scenario | 0/1 | - | 1/29 | struct structure_bsp *; pooled | 77 scenario | **UNPLACED** | - |
| 45 | 41316 | `_global_scenario` | 4 | COMMON scenario | 1/1 | scenario.h | 1/22 | struct scenario *; pooled | 77 scenario | **UNPLACED** | - |
| 46 | 41320 | `_debug_sprites` | 1 | extern-only | 2/0 | - | 2/3 | unsigned char; pooled | 81 render_sprite | **UNPLACED** | - |
| 47 | 41344 | `_build_sprite_globals` | 40 | extern-only | 1/0 | - | 1/15 | struct <unnamed-tag>; pooled | 81 render_sprite | **UNPLACED** | - |
| 48 | 41384 | `_debug_objects` | 1 | extern-only | 2/0 | - | 2/2 | unsigned char; pooled | 84 render_objects | **UNPLACED** | - |
| 49 | 41388 | `_cached_object_render_states` | 4 | COMMON render_objects | 0/1 | - | 1/11 | struct data_array *; pooled | 84 render_objects | **UNPLACED** | - |
| 50 | 41392 | `_debug_permanent_decals` | 1 | COMMON render_debug | 1/1 | - | 2/2 | absent | 87 render_debug | **UNPLACED** | - |
| 51 | 41393 | `_debug_input` | 1 | COMMON render_debug | 1/1 | - | 2/2 | absent | 87 render_debug | **UNPLACED** | - |
| 52 | 41394 | `_debug_bsp` | 1 | COMMON render_debug | 1/1 | - | 2/2 | absent | 87 render_debug | **UNPLACED** | - |
| 53 | 41395 | `_debug_structure` | 1 | COMMON render_debug | 1/1 | - | 2/2 | absent | 87 render_debug | **UNPLACED** | - |
| 54 | 41396 | `_debug_player` | 1 | COMMON render_debug | 1/1 | - | 2/2 | absent | 87 render_debug | **UNPLACED** | - |
| 55 | 41397 | `_debug_camera` | 1 | COMMON render_debug | 1/1 | - | 2/2 | absent | 87 render_debug | **UNPLACED** | - |
| 56 | 41408 | `_render` | 643732 | COMMON render | 1/1 | render.h | 43/679 | struct render_globals; pooled | 87 render_debug, 88 render_contrails, 90 render, 92 rasteri~ | **UNPLACED** | - |
| 57 | 685152 | `_rasterizer_lights` | 7172 | extern-only | 3/0 | - | 3/12 | struct rasterizer_lights; pooled | 95 rasterizer_lights | **UNPLACED** | - |
| 58 | 692352 | `_rasterizer_frame_statistics` | 368 | extern-only | 2/0 | rasterizer_frame_statistics.h | 12/281 | struct rasterizer_frame_statistic~; pooled | 95 rasterizer_lights, 97 rasterizer_frame_statistics, 98 ra~ | **UNPLACED** | - |
| 59 | 692720 | `_rasterizer_model_cortana_hack` | 1 | extern-only | 2/0 | - | 2/4 | unsigned char; pooled | 115 rasterizer_xbox_models | **UNPLACED** | - |
| 60 | 692736 | `_texture_table` | 16 | extern-only | 1/0 | - | 1/4 | absent | 128 rasterizer_xbox | **UNPLACED** | - |
| 61 | 692768 | `_texturestagestate_table` | 512 | extern-only | 1/0 | - | 1/4 | absent | 128 rasterizer_xbox | **UNPLACED** | - |
| 62 | 693280 | `_renderstate_table` | 576 | extern-only | 1/0 | - | 1/1 | absent | 128 rasterizer_xbox | **UNPLACED** | - |
| 63 | 693856 | `_global_d3d_caps` | 212 | extern-only | 1/0 | - | 1/2 | struct _D3DCAPS9; pooled | 128 rasterizer_xbox | **UNPLACED** | - |
| 64 | 694080 | `_pixel_shader` | 240 | extern-only | 10/0 | rasterizer_xbox.h | 18/1094 | absent | 128 rasterizer_xbox | **UNPLACED** | - |
| 65 | 694336 | `_global_window_parameters` | 600 | extern-only | 23/0 | - | 23/625 | struct rasterizer_window_begin_pa~; pooled | 128 rasterizer_xbox | **UNPLACED** | - |
| 66 | 694936 | `_global_frame_parameters` | 8 | extern-only | 3/0 | rasterizer.h | 11/38 | struct rasterizer_frame_begin_par~; pooled | 128 rasterizer_xbox, 129 rasterizer_common | **UNPLACED** | - |
| 67 | 694944 | `_debug_point_physics` | 1 | extern-only | 2/0 | point_physics.h | 2/2 | unsigned char; discarded | 131 point_physics | **UNPLACED** | - |
| 68 | 694976 | `_collision_usage_buffer` | 8856 | COMMON collision_usage | 0/1 | - | 1/17 | absent | 136 collision_usage | **UNPLACED** | - |
| 69 | 703840 | `_collision_usage_current` | 2952 | COMMON collision_usage | 0/1 | - | 1/9 | absent | 136 collision_usage | **UNPLACED** | - |
| 70 | 706816 | `_global_current_collision_users` | 64 | COMMON collision_usage | 1/1 | collision_usage.h | 23/35 | absent | 136 collision_usage, 140 collision_debug | **UNPLACED** | - |
| 71 | 706880 | `_collision_debug_phantom_bsp_point` | 12 | extern-only | 1/0 | - | 1/7 | absent | 140 collision_debug | **UNPLACED** | - |
| 72 | 706892 | `_collision_debug_flags` | 4 | extern-only | 1/0 | - | 1/1 | absent | 140 collision_debug | **UNPLACED** | - |
| 73 | 706896 | `_collision_debug_radius` | 4 | absent | 0/0 | - | 0/0 | absent | no referencer; interval [140, 140] | **UNPLACED** | - |
| 74 | 706912 | `_collision_debug_center` | 12 | absent | 0/0 | - | 0/0 | absent | no referencer; interval [140, 140] | **UNPLACED** | - |
| 75 | 706928 | `_collision_debug_vector` | 12 | extern-only | 1/0 | collision_debug.h | 3/21 | absent | 140 collision_debug | **UNPLACED** | - |
| 76 | 706944 | `_collision_debug_point` | 12 | extern-only | 1/0 | collision_debug.h | 3/15 | absent | 140 collision_debug | **UNPLACED** | - |
| 77 | 706956 | `_collision_debug_flag_use_vehicle_physics` | 1 | extern-only | 2/0 | - | 2/2 | absent | 140 collision_debug | **UNPLACED** | - |
| 78 | 706957 | `_collision_debug_flag_skip_passthrough_bipeds` | 1 | extern-only | 2/0 | - | 2/2 | absent | 140 collision_debug | **UNPLACED** | - |
| 79 | 706958 | `_collision_debug_flag_try_to_keep_location_va~` | 1 | extern-only | 2/0 | - | 2/2 | absent | 140 collision_debug | **UNPLACED** | - |
| 80 | 706959 | `_collision_debug_flag_objects_placeholders` | 1 | extern-only | 2/0 | - | 2/2 | absent | 140 collision_debug | **UNPLACED** | - |
| 81 | 706960 | `_collision_debug_flag_objects_light_fixtures` | 1 | extern-only | 2/0 | - | 2/2 | absent | 140 collision_debug | **UNPLACED** | - |
| 82 | 706961 | `_collision_debug_flag_objects_controls` | 1 | extern-only | 2/0 | - | 2/2 | absent | 140 collision_debug | **UNPLACED** | - |
| 83 | 706962 | `_collision_debug_flag_objects_machines` | 1 | extern-only | 2/0 | - | 2/2 | absent | 140 collision_debug | **UNPLACED** | - |
| 84 | 706963 | `_collision_debug_flag_objects_scenery` | 1 | extern-only | 2/0 | - | 2/2 | absent | 140 collision_debug | **UNPLACED** | - |
| 85 | 706964 | `_collision_debug_flag_objects_projectiles` | 1 | extern-only | 2/0 | - | 2/2 | absent | 140 collision_debug | **UNPLACED** | - |
| 86 | 706965 | `_collision_debug_flag_objects_equipment` | 1 | extern-only | 2/0 | - | 2/2 | absent | 140 collision_debug | **UNPLACED** | - |
| 87 | 706966 | `_collision_debug_flag_objects_weapons` | 1 | extern-only | 2/0 | - | 2/2 | absent | 140 collision_debug | **UNPLACED** | - |
| 88 | 706967 | `_collision_debug_flag_objects_vehicles` | 1 | extern-only | 2/0 | - | 2/2 | absent | 140 collision_debug | **UNPLACED** | - |
| 89 | 706968 | `_collision_debug_flag_objects_bipeds` | 1 | extern-only | 2/0 | - | 2/2 | absent | 140 collision_debug | **UNPLACED** | - |
| 90 | 706969 | `_collision_debug_flag_ignore_breakable_surfac~` | 1 | extern-only | 2/0 | - | 2/2 | absent | 140 collision_debug | **UNPLACED** | - |
| 91 | 706970 | `_collision_debug_flag_ignore_two_sided_surfac~` | 1 | extern-only | 2/0 | - | 2/2 | absent | 140 collision_debug | **UNPLACED** | - |
| 92 | 706971 | `_collision_debug_flag_back_facing_surfaces` | 1 | extern-only | 2/0 | - | 2/2 | absent | 140 collision_debug | **UNPLACED** | - |
| 93 | 706972 | `_collision_debug_repeat` | 1 | extern-only | 1/0 | collision_debug.h | 3/4 | absent | 140 collision_debug | **UNPLACED** | - |
| 94 | 706973 | `_collision_debug_features` | 1 | extern-only | 2/0 | - | 2/3 | absent | 140 collision_debug | **UNPLACED** | - |
| 95 | 706974 | `_collision_debug_spray` | 1 | extern-only | 2/0 | - | 2/3 | absent | 140 collision_debug | **UNPLACED** | - |
| 96 | 706975 | `_collision_debug` | 1 | extern-only | 2/0 | - | 2/3 | absent | 140 collision_debug | **UNPLACED** | - |
| 97 | 706976 | `_node_count` | 4 | extern-only | 1/0 | - | 1/3 | long; discarded | 143 bsp3d | **UNPLACED** | - |
| 98 | 706980 | `_debug_objects_names` | 1 | COMMON objects | 2/1 | objects.h | 2/2 | absent | 149 objects | **UNPLACED** | - |
| 99 | 706981 | `_debug_objects_pathfinding_spheres` | 1 | COMMON objects | 2/1 | objects.h | 2/2 | absent | 149 objects | **UNPLACED** | - |
| 100 | 706982 | `_debug_objects_physics` | 1 | COMMON objects | 2/1 | objects.h | 2/2 | absent | 149 objects | **UNPLACED** | - |
| 101 | 706983 | `_debug_objects_root_node` | 1 | COMMON objects | 2/1 | objects.h | 2/2 | absent | 149 objects | **UNPLACED** | - |
| 102 | 706984 | `_global_object_marker` | 4 | COMMON objects | 1/1 | objects.h | 1/10 | long; pooled | 149 objects | **UNPLACED** | - |
| 103 | 706992 | `_noncollideable_object_cluster_partition` | 12 | COMMON objects | 1/1 | objects.h | 1/12 | struct cluster_partition; pooled | 149 objects | **UNPLACED** | - |
| 104 | 707008 | `_collideable_object_cluster_partition` | 12 | COMMON objects | 1/1 | objects.h | 1/14 | struct cluster_partition; pooled | 149 objects | **UNPLACED** | - |
| 105 | 707020 | `_debug_object_garbage_collection` | 1 | COMMON objects | 2/1 | objects.h | 2/3 | absent | 149 objects | **UNPLACED** | - |
| 106 | 707024 | `_object_header_data` | 4 | COMMON objects | 1/1 | objects.h | 6/81 | struct data_array *; pooled | 149 objects | **UNPLACED** | - |
| 107 | 707028 | `_first_object_type_definition` | 4 | extern-only | 1/0 | - | 1/5 | struct object_type_definition *; pooled | 150 object_types | **UNPLACED** | - |
| 108 | 707032 | `_debug_lights` | 1 | extern-only | 2/0 | - | 2/2 | unsigned char; discarded | 152 object_lights | **UNPLACED** | - |
| 109 | 707033 | `_debug_object_lights` | 1 | extern-only | 2/0 | - | 2/2 | unsigned char; discarded | 152 object_lights | **UNPLACED** | - |
| 110 | 707034 | `_debug_rasterizer_light_count` | 2 | extern-only | 2/0 | - | 2/3 | short; pooled | 152 object_lights | **UNPLACED** | - |
| 111 | 707040 | `_lights_globals` | 848 | COMMON object_lights | 0/1 | - | 1/56 | struct <unnamed-tag>; pooled | 152 object_lights | **UNPLACED** | - |
| 112 | 707888 | `_light_cluster_partition` | 12 | extern-only | 1/0 | - | 1/15 | struct cluster_partition; pooled | 152 object_lights | **UNPLACED** | - |
| 113 | 707900 | `_light_data` | 4 | extern-only | 1/0 | - | 1/42 | struct data_array *; pooled | 152 object_lights | **UNPLACED** | - |
| 114 | 707904 | `_debug_damage` | 1 | extern-only | 2/0 | - | 2/3 | unsigned char; discarded | 157 damage | **UNPLACED** | - |
| 115 | 707908 | `_widget_data` | 4 | extern-only | 1/0 | widgets.h | 1/10 | struct data_array *; pooled | 158 widgets | **UNPLACED** | - |
| 116 | 707912 | `_glow_globals` | 8 | extern-only | 1/0 | glow.h | 1/24 | struct <unnamed-tag>; pooled | 164 glow | **UNPLACED** | - |
| 117 | 707920 | `_flag_data` | 4 | COMMON flags | 1/1 | flags.h | 1/12 | struct data_array *; pooled | 165 flags | **UNPLACED** | - |
| 118 | 707924 | `_antenna_data` | 4 | extern-only | 1/0 | antenna.h | 1/12 | struct data_array *; pooled | 170 antenna | **UNPLACED** | - |
| 119 | 707936 | `_network_game_server_memory_do_not_use_direct~` | 1212 | COMMON network_server_manager | 0/1 | - | 1/18 | absent | 174 network_server_manager | **UNPLACED** | - |
| 120 | 709152 | `_network_game_client_dont_use_directly` | 3248 | COMMON network_client_manager | 1/1 | network_client_manager.h | 1/5 | absent | 182 network_client_manager | **UNPLACED** | - |
| 121 | 712400 | `_render_model_no_geometry` | 1 | extern-only | 2/0 | - | 2/2 | absent | 183 models | **UNPLACED** | - |
| 122 | 712401 | `_render_model_markers` | 1 | extern-only | 2/0 | - | 2/2 | absent | 183 models | **UNPLACED** | - |
| 123 | 712402 | `_render_model_index_counts` | 1 | extern-only | 2/0 | - | 2/4 | absent | 183 models | **UNPLACED** | - |
| 124 | 712403 | `_render_model_vertex_counts` | 1 | extern-only | 2/0 | - | 2/4 | absent | 183 models | **UNPLACED** | - |
| 125 | 712404 | `_render_model_nodes` | 1 | extern-only | 2/0 | - | 2/2 | unsigned char; discarded | 183 models | **UNPLACED** | - |
| 126 | 712405 | `_debug_render_freeze` | 1 | extern-only | 2/0 | rasterizer_debug_options.h | 2/2 | unsigned char; pooled | 232 main | **UNPLACED** | - |
| 127 | 712416 | `_cached_variant_profile` | 324 | extern-only | 2/0 | - | 2/10 | struct _cached_variant_profile[0x~; pooled | 249 ui_widget_game_data_input_functions, 250 ui_widget_even~ | **UNPLACED** | - |
| 128 | 712768 | `_cached_player_profile` | 156 | extern-only | 2/0 | - | 2/13 | struct _cached_player_profile[0x3]; pooled | 249 ui_widget_game_data_input_functions, 250 ui_widget_even~ | **UNPLACED** | - |
| 129 | 712924 | `_local_player_index_for_draw_string_and_hack_~` | 2 | extern-only | 1/0 | - | 1/3 | short; pooled | 251 ui_widget | **UNPLACED** | - |
| 130 | 712928 | `_ui_plasma_effect_color` | 16 | extern-only | 1/0 | - | 1/16 | union real_argb_color; pooled | 251 ui_widget | **UNPLACED** | - |
| 131 | 712944 | `_light_z` | 4 | absent | 0/0 | - | 0/0 | absent | no referencer; interval [251, 253] | **UNPLACED** | - |
| 132 | 712948 | `_light_y` | 4 | absent | 0/0 | - | 0/0 | absent | no referencer; interval [251, 253] | **UNPLACED** | - |
| 133 | 712952 | `_light_x` | 4 | absent | 0/0 | - | 0/0 | absent | no referencer; interval [251, 253] | **UNPLACED** | - |
| 134 | 712960 | `_blur_shader` | 240 | extern-only | 1/0 | - | 1/14 | absent | 253 progress_bar | **UNPLACED** | - |
| 135 | 713216 | `_regular_shader` | 240 | extern-only | 1/0 | - | 1/10 | absent | 253 progress_bar | **UNPLACED** | - |
| 136 | 713456 | `_current_time` | 4 | extern-only | 1/0 | - | 1/1 | absent | 253 progress_bar | **UNPLACED** | - |
| 137 | 713460 | `_profile_graph` | 1 | extern-only | 2/0 | profile.h | 2/4 | absent | 261 interface | **UNPLACED** | - |
| 138 | 713461 | `_profile_display` | 1 | extern-only | 2/0 | profile.h | 2/2 | absent | 261 interface | **UNPLACED** | - |
| 139 | 713462 | `_blip_player_index` | 2 | extern-only | 1/0 | - | 1/1 | short; pooled | 267 motion_sensor | **UNPLACED** | - |
| 140 | 713464 | `_num_drawn` | 2 | absent | 0/0 | - | 0/0 | short; discarded | no referencer; interval [267, 267] | **UNPLACED** | - |
| 141 | 713466 | `_num_triangles` | 2 | absent | 0/0 | - | 0/0 | short; discarded | no referencer; interval [267, 267] | **UNPLACED** | - |
| 142 | 713472 | `_center_point` | 8 | extern-only | 1/0 | - | 1/3 | union real_point2d; pooled | 267 motion_sensor | **UNPLACED** | - |
| 143 | 713480 | `_color32` | 4 | absent | 0/0 | - | 0/0 | unsigned long; discarded | no referencer; interval [267, 273] | **UNPLACED** | - |
| 144 | 713484 | `_hud_msg_def` | 4 | extern-only | 1/0 | - | 1/10 | struct hud_messaging_parameters_d~; pooled | 273 hud_messaging | **UNPLACED** | - |
| 145 | 713488 | `_temporary_hud` | 1 | extern-only | 2/0 | - | 2/2 | unsigned char; discarded | 276 hud | **UNPLACED** | - |
| 146 | 713492 | `_object_list_data` | 4 | COMMON ai_script,hs,hs_runtime,object_lists,da~ | 0/1 | object_lists.h | 1/7 | struct data_array *; pooled | 281 object_lists | **UNPLACED** | - |
| 147 | 713496 | `_object_list_header_data` | 4 | COMMON ai_script,hs,hs_runtime,object_lists,da~ | 0/1 | object_lists.h | 1/15 | struct data_array *; pooled | 281 object_lists | **UNPLACED** | - |
| 148 | 713500 | `_debug_trigger_volumes` | 1 | extern-only | 2/0 | hs.h | 2/2 | unsigned char; discarded | 283 hs_runtime | **UNPLACED** | - |
| 149 | 713501 | `_debug_scripting` | 1 | extern-only | 2/0 | - | 2/2 | unsigned char; discarded | 283 hs_runtime | **UNPLACED** | - |
| 150 | 713504 | `_hs_debug_data` | 32 | extern-only | 2/0 | - | 2/8 | absent | 283 hs_runtime | **UNPLACED** | - |
| 151 | 713536 | `_hs_global_data` | 4 | extern-only | 1/0 | - | 1/15 | struct data_array *; pooled | 283 hs_runtime | **UNPLACED** | - |
| 152 | 713540 | `_hs_thread_data` | 4 | extern-only | 1/0 | - | 1/66 | struct data_array *; pooled | 283 hs_runtime | **UNPLACED** | - |
| 153 | 713544 | `_hs_syntax_data` | 4 | COMMON hs | 2/1 | - | 3/272 | struct data_array *; pooled | 283 hs_runtime, 286 hs_compile, 287 hs | **UNPLACED** | - |
| 154 | 713548 | `_players_globals` | 4 | COMMON players | 1/1 | players.h | 1/68 | struct players_global_data *; pooled | 288 players | **UNPLACED** | - |
| 155 | 713552 | `_team_data` | 4 | COMMON players | 0/1 | - | 1/5 | struct data_array *; pooled | 288 players | **UNPLACED** | - |
| 156 | 713556 | `_player_data` | 4 | COMMON players | 2/1 | players.h | 49/345 | struct data_array *; pooled | 288 players, 290 player_rumble, 291 player_queues_new, 292 ~ | **UNPLACED** | - |
| 157 | 713568 | `_global_default_animation_colors` | 48 | extern-only | 1/0 | - | 1/13 | union real_rgb_color[0x4]; pooled | 303 game_engine_king | **UNPLACED** | - |
| 158 | 713616 | `_global_default_animation_values` | 16 | extern-only | 1/0 | - | 1/5 | float[0x4]; pooled | 303 game_engine_king | **UNPLACED** | - |
| 159 | 713632 | `_game_engine_globals` | 36 | extern-only | 1/0 | - | 1/59 | struct _game_engine_globals; pooled | 307 game_engine | **UNPLACED** | - |
| 160 | 713668 | `_timeout_for_endgame_sound` | 4 | extern-only | 3/0 | - | 3/3 | long; pooled | 307 game_engine | **UNPLACED** | - |
| 161 | 713696 | `_global_stage` | 168 | extern-only | 1/0 | - | 1/13 | struct play_stage; pooled | 307 game_engine | **UNPLACED** | - |
| 162 | 713888 | `_game_variant_global` | 104 | extern-only | 1/0 | - | 1/7 | struct game_variant; pooled | 309 game | **UNPLACED** | - |
| 163 | 714000 | `_cheat` | 10 | extern-only | 2/0 | cheats.h | 7/28 | struct cheat_globals; pooled | 310 cheats | **UNPLACED** | - |
| 164 | 714012 | `_weather_particle_data` | 4 | extern-only | 1/0 | weather_particle_systems.h | 1/16 | struct data_array *; pooled | 312 weather_particle_systems | **UNPLACED** | - |
| 165 | 714016 | `_particle_data` | 4 | extern-only | 1/0 | particles.h | 3/35 | struct data_array *; pooled | 317 particles | **UNPLACED** | - |
| 166 | 714020 | `_system_particles` | 4 | extern-only | 1/0 | particle_systems.h | 1/13 | struct data_array *; pooled | 318 particle_systems | **UNPLACED** | - |
| 167 | 714024 | `_particle_systems` | 4 | extern-only | 1/0 | particle_systems.h | 1/27 | struct data_array *; pooled | 318 particle_systems | **UNPLACED** | - |
| 168 | 714028 | `_effect_location_data` | 4 | extern-only | 1/0 | - | 1/16 | struct data_array *; pooled | 324 effects | **UNPLACED** | - |
| 169 | 714032 | `_effect_data` | 4 | extern-only | 1/0 | - | 1/40 | struct data_array *; pooled | 324 effects | **UNPLACED** | - |
| 170 | 714036 | `_debug_decals` | 1 | extern-only | 2/0 | decals.h | 2/5 | unsigned char; discarded | 326 decals | **UNPLACED** | - |
| 171 | 714040 | `_global_decal_data` | 4 | extern-only | 2/0 | - | 2/48 | struct data_array *; pooled | 326 decals | **UNPLACED** | - |
| 172 | 714044 | `_contrail_point_data` | 4 | extern-only | 1/0 | contrails.h | 2/23 | struct data_array *; pooled | 328 contrails | **UNPLACED** | - |
| 173 | 714048 | `_contrail_data` | 4 | extern-only | 1/0 | contrails.h | 2/25 | struct data_array *; pooled | 328 contrails | **UNPLACED** | - |
| 174 | 714052 | `_debug_objects_devices` | 1 | extern-only | 2/0 | devices.h | 2/2 | absent | 341 devices | **UNPLACED** | - |
| 175 | 714056 | `_device_groups_data` | 4 | extern-only | 1/0 | devices.h | 3/30 | struct data_array *; pooled | 341 devices, 342 device_machines, 345 device_controls | **UNPLACED** | - |
| 176 | 714080 | `_error_globals` | 2056 | extern-only | 1/0 | errors.h | 1/31 | struct error_global_data; pooled | 356 errors | **UNPLACED** | - |
| 177 | 716160 | `_temporary` | 256 | COMMON cseries | 1/1 | cseries.h | 84/642 | char[0x100]; discarded | 357 debug_memory, 360 static_camera, 361 orbiting_camera, 3~ | **UNPLACED** | - |
| 178 | 716416 | `_director_camera_scripted` | 4 | extern-only | 1/0 | director.h | 1/5 | unsigned char *; pooled | 367 director | **UNPLACED** | - |
| 179 | 716420 | `_server_transport_globals` | 1 | COMMON transport_endpoint_set_winsock | 0/1 | - | 1/2 | absent | 373 transport_endpoint_set_winsock | **UNPLACED** | - |
| 180 | 716432 | `_global_key` | 16 | extern-only | 1/0 | - | 1/9 | absent | 373 transport_endpoint_set_winsock | **UNPLACED** | - |
| 181 | 716448 | `_global_key_id` | 8 | extern-only | 1/0 | - | 1/11 | absent | 373 transport_endpoint_set_winsock | **UNPLACED** | - |
| 182 | 716456 | `_global_nonce` | 8 | extern-only | 1/0 | - | 1/3 | absent | 373 transport_endpoint_set_winsock | **UNPLACED** | - |
| 183 | 716464 | `_global_address` | 12 | extern-only | 1/0 | - | 1/4 | absent | 373 transport_endpoint_set_winsock | **UNPLACED** | - |
| 184 | 716476 | `_prop_data` | 4 | extern-only | 1/0 | props.h | 24/186 | struct data_array *; pooled | 425 props | **UNPLACED** | - |
| 185 | 716480 | `_debug_obstacle_path_radius` | 4 | extern-only | 1/0 | path.h | 1/3 | absent | 429 path_obstacle_avoidance | **UNPLACED** | - |
| 186 | 716484 | `_debug_ignore_broken_surfaces` | 1 | extern-only | 1/0 | path.h | 1/2 | absent | 429 path_obstacle_avoidance | **UNPLACED** | - |
| 187 | 716485 | `_debug_obstacle_path_finishing` | 1 | extern-only | 1/0 | path.h | 1/2 | absent | 429 path_obstacle_avoidance | **UNPLACED** | - |
| 188 | 716488 | `_debug_obstacle_path_goal_surface_index` | 4 | extern-only | 2/0 | path.h | 2/3 | absent | 429 path_obstacle_avoidance | **UNPLACED** | - |
| 189 | 716496 | `_debug_obstacle_path_goal_point` | 12 | extern-only | 2/0 | path.h | 2/6 | absent | 429 path_obstacle_avoidance | **UNPLACED** | - |
| 190 | 716508 | `_debug_obstacle_path_start_surface_index` | 4 | extern-only | 2/0 | path.h | 2/3 | absent | 429 path_obstacle_avoidance | **UNPLACED** | - |
| 191 | 716512 | `_debug_obstacle_path_start_point` | 12 | extern-only | 2/0 | path.h | 2/6 | absent | 429 path_obstacle_avoidance | **UNPLACED** | - |
| 192 | 716524 | `_pursuit_data` | 4 | COMMON encounters | 1/1 | encounters.h | 1/11 | struct data_array *; pooled | 431 encounters | **UNPLACED** | - |
| 193 | 716528 | `_encounter_data` | 4 | COMMON encounters | 1/1 | encounters.h | 9/111 | struct data_array *; pooled | 431 encounters | **UNPLACED** | - |
| 194 | 716532 | `_platoon_array` | 4 | COMMON encounters | 1/1 | encounters.h | 3/5 | struct platoon_datum *; pooled | 431 encounters | **UNPLACED** | - |
| 195 | 716536 | `_squad_array` | 4 | COMMON encounters | 1/1 | encounters.h | 3/5 | struct squad_datum *; pooled | 431 encounters | **UNPLACED** | - |
| 196 | 716544 | `_profilestring` | 2048 | COMMON ai_profile | 0/1 | - | 1/12 | absent | 434 ai_profile | **UNPLACED** | - |
| 197 | 718592 | `_global_ai_profile_string_position` | 2 | COMMON ai_profile | 0/1 | - | 1/3 | absent | 434 ai_profile | **UNPLACED** | - |
| 198 | 718624 | `_ai_profile` | 3820 | COMMON ai_profile | 1/1 | ai_profile.h | 12/96 | absent | 434 ai_profile, 435 ai_debug | **UNPLACED** | - |
| 199 | 722444 | `_global_ai_debug_string_position` | 2 | extern-only | 1/0 | ai_debug.h | 1/1 | absent | 435 ai_debug | **UNPLACED** | - |
| 200 | 722448 | `_global_ai_debug_drawstack_height` | 4 | COMMON ai_debug | 1/1 | ai_debug.h | 1/74 | absent | 435 ai_debug | **UNPLACED** | - |
| 201 | 722464 | `_global_ai_debug_drawstack_last_position` | 12 | COMMON ai_debug | 1/1 | ai_debug.h | 1/148 | absent | 435 ai_debug | **UNPLACED** | - |
| 202 | 722480 | `_global_ai_debug_drawstack_next_position` | 12 | COMMON ai_debug | 1/1 | ai_debug.h | 1/170 | absent | 435 ai_debug | **UNPLACED** | - |
| 203 | 722496 | `_ai_debug` | 547628 | COMMON ai_debug | 2/1 | ai_debug.h | 21/830 | absent | 435 ai_debug, 436 ai_communication | **UNPLACED** | - |
| 204 | 1270124 | `_conversation_data` | 4 | extern-only | 1/0 | ai_communication.h | 3/26 | struct data_array *; pooled | 436 ai_communication | **UNPLACED** | - |
| 205 | 1270144 | `_global_communication_table_indices` | 114 | extern-only | 1/0 | - | 1/2 | short[0x39]; pooled | 436 ai_communication | **UNPLACED** | - |
| 206 | 1270260 | `_ai_globals` | 4 | extern-only | 7/0 | - | 7/173 | struct ai_globals *; pooled | 436 ai_communication, 437 ai, 438 actors | **UNPLACED** | - |
| 207 | 1270272 | `_global_continuous_damage_reference` | 12 | absent | 0/0 | - | 0/0 | struct tag_reference_definition; discarded | no referencer; interval [436, 453] | **UNPLACED** | - |
| 208 | 1270288 | `_global_damage_reference` | 12 | absent | 0/0 | - | 0/0 | struct tag_reference_definition; discarded | no referencer; interval [436, 453] | **UNPLACED** | - |
| 209 | 1270300 | `_swarm_component_data` | 4 | COMMON actors | 1/1 | actors.h | 6/21 | struct data_array *; pooled | 438 actors, 444 actor_type_infection, 453 actor_perception | **UNPLACED** | - |
| 210 | 1270304 | `_swarm_data` | 4 | COMMON actors | 1/1 | actors.h | 9/31 | struct data_array *; pooled | 438 actors, 444 actor_type_infection, 453 actor_perception | **UNPLACED** | - |
| 211 | 1270308 | `_actor_data` | 4 | COMMON actors | 1/1 | actors.h | 43/522 | struct data_array *; pooled | 438 actors, 439 actor_types, 440 actor_type_sentinel, 441 a~ | **UNPLACED** | - |
| 212 | 1270336 | `_avoidance_rays` | 448 | extern-only | 1/0 | - | 1/2 | struct vector_avoidance_ray[0x2][~; pooled | 454 actor_moving | **UNPLACED** | - |
| 213 | 1270784 | `_avoidance_directions` | 96 | extern-only | 1/0 | - | 1/12 | union real_vector3d[0x8]; pooled | 454 actor_moving | **UNPLACED** | - |
| 214 | 1270880 | `_sense_rays` | 252 | extern-only | 1/0 | - | 1/2 | struct vector_avoidance_ray[0x9]; pooled | 454 actor_moving | **UNPLACED** | - |
| 215 | 1271136 | `_global_temporary_render_color` | 16 | COMMON ai_debug | 1/1 | ai_debug.h | 1/10 | union real_argb_color; discarded | 459 actions | **UNPLACED** | - |
| 216 | 1271168 | `_OHCD_GlobalPool` | 48 | absent | 0/0 | - | 6/77 | absent | n/a (vendor/linker) | **PLACED** | xapilib.lib:obj\i386\pool.obj (vendor libra~ [EC-LIB+EC-MOD+EC-LIB-ID] |
| 217 | 1271232 | `_OHCD_InterruptObject` | 112 | absent | 0/0 | - | 1/2 | absent | n/a (vendor/linker) | **PLACED** | xapilib.lib:obj\i386\ohcd.obj (vendor libra~ [EC-LIB+EC-MOD+EC-LIB-ID] |
| 218 | 1271360 | `_XapiAutoPowerDownGlobals` | 80 | absent | 0/0 | - | 1/9 | absent | n/a (vendor/linker) | **PLACED** | xapilib.lib:obj\i386\powerdwn.obj (vendor l~ [EC-LIB+EC-MOD+EC-LIB-ID] |
| 219 | 1271440 | `_CheckHeapFillPattern` | 16 | absent | 0/0 | - | 1/1 | unsigned char[0x10]; discarded | n/a (vendor/linker) | **PLACED** | xapilib.lib:obj\i386\heap.obj (vendor libra~ [EC-LIB+EC-MOD+EC-LIB-ID] |
| 220 | 1271456 | `_NtGlobalFlag` | 4 | absent | 0/0 | - | 1/2 | unsigned long; pooled | n/a (vendor/linker) | **PLACED** | xapilib.lib:obj\i386\heap.obj (vendor libra~ [EC-LIB+EC-MOD+EC-LIB-ID] |
| 221 | 1271460 | `___@@_PchSym_@00@UcylcRurmzouivUkirezgvUmglhU~` | 4 | absent | 0/0 | - | 0/0 | - | n/a (vendor/linker) | **PLACED** | xapilib.lib:obj\i386\basedll.obj (vendor li~ [EC-LIB+EC-MOD+EC-PCH] |
| 222 | 1271464 | `_XapiProcessHeap` | 4 | absent | 0/0 | - | 3/13 | void *; pooled | n/a (vendor/linker) | **PLACED** | xapilib.lib:obj\i386\xapiheap.obj (vendor l~ [EC-LIB+EC-MOD+EC-LIB-ID] |
| 223 | 1271468 | `_XapiCurrentTopLevelFilter` | 4 | absent | 0/0 | - | 1/3 | function  *; pooled | n/a (vendor/linker) | **PLACED** | xapilib.lib:obj\i386\thread.obj (vendor lib~ [EC-LIB+EC-MOD+EC-LIB-ID] |
| 224 | 1271472 | `_XapiTlsSize` | 4 | absent | 0/0 | - | 2/2 | absent | n/a (vendor/linker) | **PLACED** | xapilib.lib:obj\i386\thread.obj (vendor lib~ [EC-LIB+EC-MOD+EC-LIB-ID] |
| 225 | 1271476 | `___@@_PchSym_@00@UcylcRurmzouivUkirezgvUdrmwl~` | 4 | absent | 0/0 | - | 0/0 | - | n/a (vendor/linker) | **PLACED** | dsound.lib:obj\i386\dsoundi.obj (vendor lib~ [EC-LIB+EC-MOD+EC-PCH] |
| 226 | 1271480 | `___mblcid` | 4 | absent | 0/0 | - | 1/7 | int; DEF mbctype.obj | n/a (vendor/linker) | **PLACED** | libcmt.lib:obj\i386\mbctype.obj (vendor lib~ [EC-LIB+EC-MOD+EC-LIB-ID] |
| 227 | 1271484 | `___ptmbcinfo` | 4 | absent | 0/0 | - | 2/7 | struct threadmbcinfostruct *; DEF mbctype.obj | n/a (vendor/linker) | **PLACED** | libcmt.lib:obj\i386\mbctype.obj (vendor lib~ [EC-LIB+EC-MOD+EC-LIB-ID] |
| 228 | 1271488 | `___ismbcodepage` | 4 | absent | 0/0 | - | 1/4 | int; DEF mbctype.obj | n/a (vendor/linker) | **PLACED** | libcmt.lib:obj\i386\mbctype.obj (vendor lib~ [EC-LIB+EC-MOD+EC-LIB-ID] |
| 229 | 1271520 | `__mbctype` | 257 | absent | 0/0 | - | 1/4 | unsigned char[0x101]; DEF mbctype.obj | n/a (vendor/linker) | **PLACED** | libcmt.lib:obj\i386\mbctype.obj (vendor lib~ [EC-LIB+EC-MOD+EC-LIB-ID] |
| 230 | 1271780 | `___mbcodepage` | 4 | absent | 0/0 | - | 1/6 | int; DEF mbctype.obj | n/a (vendor/linker) | **PLACED** | libcmt.lib:obj\i386\mbctype.obj (vendor lib~ [EC-LIB+EC-MOD+EC-LIB-ID] |
| 231 | 1271808 | `___mbulinfo` | 24 | absent | 0/0 | - | 1/3 | unsigned short[0x6]; DEF mbctype.obj | n/a (vendor/linker) | **PLACED** | libcmt.lib:obj\i386\mbctype.obj (vendor lib~ [EC-LIB+EC-MOD+EC-LIB-ID] |
| 232 | 1271840 | `__mbcasemap` | 256 | absent | 0/0 | - | 1/1 | unsigned char[0x100]; DEF mbctype.obj | n/a (vendor/linker) | **PLACED** | libcmt.lib:obj\i386\mbctype.obj (vendor lib~ [EC-LIB+EC-MOD+EC-LIB-ID] |
| 233 | 1272096 | `___active_heap` | 4 | extern-only | 2/0 | - | 3/3 | absent | n/a (vendor/linker) | **PLACED** | libcmt.lib:obj\i386\heapinit.obj (vendor li~ [EC-LIB+EC-MOD+EC-LIB-ID] |
| 234 | 1272100 | `___ptd_glob` | 4 | absent | 0/0 | - | 1/2 | absent | n/a (vendor/linker) | **PLACED** | libcmt.lib:obj\i386\tidtable.obj (vendor li~ [EC-LIB+EC-MOD+EC-LIB-ID] |
| 235 | 1272104 | `__nhandle` | 4 | extern-only | 2/0 | - | 14/17 | int; pooled | n/a (vendor/linker) | **PLACED** | libcmt.lib:obj\i386\ioinit.obj (vendor libr~ [EC-LIB+EC-MOD+EC-LIB-ID] |
| 236 | 1272128 | `___pioinfo` | 256 | extern-only | 6/0 | - | 26/43 | struct ioinfo *[0x0]; pooled | n/a (vendor/linker) | **PLACED** | libcmt.lib:obj\i386\ioinit.obj (vendor libr~ [EC-LIB+EC-MOD+EC-LIB-ID] |
| 237 | 1272384 | `___piob` | 4 | extern-only | 2/0 | - | 6/19 | void * *; pooled | n/a (vendor/linker) | **PLACED** | libcmt.lib:obj\i386\_file.obj (vendor libra~ [EC-LIB+EC-MOD+EC-LIB-ID] |
| 238 | 1272388 | `__nstream` | 4 | extern-only | 2/0 | - | 4/6 | int; pooled | n/a (vendor/linker) | **PLACED** | libcmt.lib:obj\i386\_file.obj (vendor libra~ [EC-LIB+EC-MOD+EC-LIB-ID] |
| 239 | 1272392 | `___env_initialized` | 4 | absent | 0/0 | - | 0/0 | int; discarded | n/a (vendor/linker) | **PLACED** | libcmt.lib:obj\i386\crt0dat.obj (vendor lib~ [EC-LIB+EC-MOD+EC-LIB-ID] |
| 240 | 1272396 | `___onexitend` | 4 | extern-only | 1/0 | - | 1/3 | function  * *; pooled | n/a (vendor/linker) | **PLACED** | libcmt.lib:obj\i386\crt0dat.obj (vendor lib~ [EC-LIB+EC-MOD+EC-LIB-ID] |
| 241 | 1272400 | `___onexitbegin` | 4 | extern-only | 1/0 | - | 1/2 | function  * *; pooled | n/a (vendor/linker) | **PLACED** | libcmt.lib:obj\i386\crt0dat.obj (vendor lib~ [EC-LIB+EC-MOD+EC-LIB-ID] |
| 242 | 1272404 | `___mbctype_initialized` | 4 | absent | 0/0 | - | 1/2 | int; pooled | n/a (vendor/linker) | **PLACED** | libcmt.lib:obj\i386\crt0dat.obj (vendor lib~ [EC-LIB+EC-MOD+EC-LIB-ID] |
