# Opus5 100K consolidated lane: checkpoint-60k

| total | value |
| --- | ---: |
| `head_commit` | 94f7bbb80cf05ae68bb9a6ab3965927f38c4f0f8 |
| `accepted_functions` | 7092 |
| `accepted_meaningful` | 1206879 |
| `gain_functions_vs_base` | 120 |
| `gain_meaningful_vs_base` | 66718 |
| `strict_gained_functions` | 125 |
| `strict_gained_padded` | 69136 |
| `strict_gained_meaningful` | 68131 |
| `remaining_to_target` | 33282 |

Regressions vs base snapshot: 0

## Gained functions by object

| object | function | padded | meaningful |
| --- | --- | ---: | ---: |
| bitmaps/bitmap_extract | _extract_build_debug_plate | 1408 | 1398 |
| bitmaps/bitmap_extract | _extract_bitmap | 1168 | 1154 |
| bitmaps/bitmap_extract | _extract_mipmaps_to_bitmap | 976 | 968 |
| bitmaps/bitmap_extract | _extract_pixels_to_mipmap | 944 | 934 |
| bitmaps/bitmap_extract | _bitmaps_extract | 736 | 729 |
| bitmaps/bitmap_extract | _extract_add_bitmap | 688 | 676 |
| bitmaps/bitmap_extract | _extract_cube_maps | 544 | 541 |
| bitmaps/bitmap_extract | _extract_pixels_from_mipmap | 528 | 518 |
| bitmaps/bitmap_extract | _extract_3d_textures | 464 | 457 |
| bitmaps/bitmap_extract | _extract_warn_about_horizontal_border | 128 | 114 |
| ai/actor_firing_position | _actor_select_firing_position | 5248 | 5248 |
| ai/actor_firing_position | _actor_active_select_firing_position | 576 | 569 |
| ai/actor_firing_position | _firing_position_compute_line_of_sight | 464 | 455 |
| ai/actor_firing_position | _firing_position_compare | 272 | 264 |
| interface/hud_draw | _hud_draw_multitexture_overlay | 2272 | 2264 |
| interface/hud_draw | _hud_draw_numbers | 1856 | 1845 |
| interface/hud_draw | _hud_draw_meter | 1312 | 1298 |
| interface/hud_draw | _hud_draw_static_element | 640 | 627 |
| interface/hud_draw | _hud_draw_multitexture_overlay_get_current_weapon_definition | 480 | 468 |
| items/weapons | _weapon_trigger_fire | 1984 | 1970 |
| items/weapons | _weapon_trigger_begin_firing | 624 | 620 |
| items/weapons | _weapon_aim | 272 | 257 |
| items/weapons | _weapon_trigger_release_charge | 256 | 245 |
| items/weapons | _weapon_trigger_can_fire_again | 240 | 234 |
| items/weapons | _weapon_trigger_overload | 208 | 193 |
| items/weapons | _weapon_magazine_start_chamber | 192 | 187 |
| items/weapons | _weapon_trigger_fully_charged | 192 | 189 |
| items/weapons | _projectile_distribute | 160 | 149 |
| items/weapons | _weapon_trigger_change_state | 144 | 132 |
| items/weapons | _weapon_trigger_idle | 144 | 137 |
| items/weapons | _weapon_trigger_start_ejection_port | 128 | 125 |
| items/weapons | _weapon_magazine_finish_chamber | 128 | 122 |
| items/weapons | _weapon_trigger_overcharged | 128 | 127 |
| items/weapons | _weapon_effect_looping_new | 112 | 104 |
| items/weapons | _weapon_trigger_locked | 96 | 84 |
| items/weapons | _weapon_trigger_recover | 96 | 83 |
| items/weapons | _weapon_get_projectile_owner_object_index | 80 | 71 |
| items/weapons | _weapon_magazine_idle | 80 | 70 |
| items/weapons | _weapon_state_key_frame | 80 | 74 |
| items/weapons | _weapon_trigger_finish_tracking | 80 | 79 |
| items/weapons | _weapon_detonate | 64 | 49 |
| items/weapons | _weapon_magazine_state_interruptable | 32 | 18 |
| items/weapons | _random | 16 | 15 |
| models/model_animations | _overlay_animation_apply_continuous_scaled | 1024 | 1012 |
| models/model_animations | _overlay_animation_apply_continuous | 960 | 953 |
| models/model_animations | _animation_get_keyframe_rotation | 784 | 779 |
| models/model_animations | _animation_get_keyframe_translation | 768 | 759 |
| models/model_animations | _animation_get_keyframe_scale | 752 | 742 |
| models/model_animations | _animation_update_internal | 304 | 300 |
| ai/encounters | _encounter_post_combat | 1984 | 1971 |
| ai/encounters | _encounter_update_follow | 1920 | 1909 |
| ai/encounters | _encounter_spawn_actor | 288 | 274 |
| ai/encounters | _encounter_post_combat_select_random_behavior | 272 | 259 |
| ai/ai_script | _ai_scripting_migrate_internal | 1360 | 1352 |
| ai/ai_script | _ai_scripting_migrate_find_target_squad | 1072 | 1067 |
| ai/ai_script | _ai_scripting_command_list_status | 592 | 590 |
| ai/ai_script | _ai_scripting_vehicle_encounter | 496 | 491 |
| ai/ai_script | _ai_scripting_migrate_by_unit_internal | 240 | 232 |
| ai/ai_script | _ai_index_actor_iterator_new | 208 | 204 |
| ai/ai_script | _ai_scripting_migrate | 160 | 145 |
| ai/ai_script | _ai_scripting_command_list_status_internal | 80 | 78 |
| ai/action_obey | _action_obey_describe_command | 1936 | 1928 |
| ai/action_obey | _action_obey_command_perform | 1408 | 1404 |
| ai/action_obey | _action_obey_individual_perform | 288 | 275 |
| ai/action_obey | _action_obey_perform | 192 | 189 |
| ai/action_obey | _vehicle_possibility_qsort | 64 | 49 |
| physics/physics | _physics_update_new | 1984 | 1976 |
| physics/physics | _physics_compute_biped_collision | 1136 | 1133 |
| physics/physics | _rotate_vectors3d_by_angular_velocity | 432 | 417 |
| physics/collisions | _collision_test_vector | 1744 | 1730 |
| physics/collisions | _collision_fix_pill | 672 | 657 |
| physics/collisions | _collision_get_features_in_sphere | 512 | 507 |
| physics/collisions | _collision_fix_pill_nudge_collision | 160 | 147 |
| physics/collisions | _clip_position_to_line | 128 | 117 |
| physics/collisions | _clip_velocity_to_line | 80 | 73 |
| physics/collisions | _clip_position_to_plane | 64 | 54 |
| physics/collisions | _clip_velocity_to_plane | 64 | 51 |
| ai/actor_looking | _actor_look_secondary | 880 | 869 |
| ai/actor_looking | _actor_look_idle_new_major_direction | 528 | 523 |
| ai/actor_looking | _actor_look_idle_new_minor_direction | 368 | 362 |
| ai/actor_looking | _actor_look_idle_timer | 352 | 351 |
| ai/actor_looking | _actor_looking_test_validity | 176 | 161 |
| ai/actor_looking | _actor_look_affect_movement | 128 | 122 |
| ai/actor_looking | _actor_look_get_looking_definition | 80 | 71 |
| rasterizer/xbox/rasterizer_xbox_screen_effect | _rasterizer_screen_effect_set_texture_transforms | 2128 | 2115 |
| ai/actor_moving | _actor_move_calculate_free | 720 | 717 |
| ai/actor_moving | _actor_move_calculate_controlled_by_aiming | 640 | 626 |
| ai/actor_moving | _actor_move_vector_avoidance_find_direction | 240 | 230 |
| ai/actor_moving | _midpoint3d | 64 | 56 |
| game/players | _players_update_after_game | 816 | 810 |
| game/players | _player_examine_nearby_item | 800 | 793 |
| sound/sound_dsound_xbox | _dsound_set_listener_properties | 816 | 816 |
| sound/sound_dsound_xbox | _channel_set_location | 752 | 738 |
| items/projectiles | _projectile_detonate | 1040 | 1031 |
| items/projectiles | _projectile_effect_new | 96 | 92 |
| items/projectiles | _projectile_set_action | 32 | 28 |
| render/render_objects | _render_object_list | 1152 | 1152 |
| game/game_engine | _game_engine_verify_current_map | 640 | 638 |
| game/game_engine | _select_players_to_display | 496 | 494 |
| sound/sound_manager | _refresh_sounds | 544 | 543 |
| sound/sound_manager | _looping_sound_new | 304 | 292 |
| sound/sound_manager | _limit_pitch | 96 | 92 |
| sound/sound_manager | _sound_travel_milliseconds | 32 | 18 |
| ai/actor_combat | _actor_start_first_burst_delay | 208 | 205 |
| ai/actor_combat | _actor_start_pause | 176 | 173 |
| ai/actor_combat | _actor_combat_allow_special_fire_situation | 176 | 173 |
| ai/actor_combat | _actor_combat_get_firing_parameters | 160 | 151 |
| ai/actor_combat | _actor_combat_enable_special_fire_situation | 144 | 139 |
| ai/actor_combat | _fast_ftol | 32 | 17 |
| ai/actor_combat | _random_boolean | 32 | 26 |
| effects/decals | _decal_insert | 704 | 692 |
| effects/decals | _bsp3d_get_plane_from_designator | 112 | 101 |
| units/bipeds | _biped_update_jumping | 512 | 509 |
| units/bipeds | _biped_start_landing | 272 | 257 |
| interface/ui_widget | _ui_widget_load_children_recursive | 672 | 672 |
| game/aim_assist | _object_compute_autoaim_target | 416 | 413 |
| game/aim_assist | _limit3d | 96 | 88 |
| game/aim_assist | _reciprocal_square_root | 16 | 16 |
| rasterizer/rasterizer_frame_statistics | _rasterizer_frame_statistics_get_fps | 384 | 375 |
| render/render_sprite | _build_sprite_get_group | 384 | 381 |
| interface/motion_sensor | _motion_sensor_tick | 112 | 99 |
| interface/motion_sensor | _get_motion_sensor_data | 64 | 61 |
| rasterizer/xbox/rasterizer_xbox_active_camouflage | _real_alpha_to_pixel32 | 112 | 99 |
| cseries/stack_walk_windows | _symbol_sort_proc | 48 | 47 |
| rasterizer/xbox/rasterizer_xbox_environment | _IDirect3DDevice8_SetTexture@12 | 16 | 12 |

## Checkpoint verification (+60K, at the bitmap packet)

- Full build: pass.
- Stable diff from base snapshot: 0 regressions.
- Parks: 446 active / 0 stale / 0 invalid. 19 parks were reopened with adversarial verification, and 3 were re-measured with appended evidence (speech timers, screen_effect, bitmap_group_add_bitmap).
- Admission audit, pytest and fake-scan: unchanged from checkpoint 1 at every packet.
- Treemap input: `scratch/report-ck60k.json`.
