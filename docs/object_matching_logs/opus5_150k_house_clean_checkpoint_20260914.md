# Opus5 150K house-clean lane: checkpoint-w1

| total | value |
| --- | ---: |
| `head_commit` | 8e9a6de05d55c3aac3aab52f651f9a10dcedf317 |
| `accepted_functions` | 7205 |
| `accepted_meaningful` | 1275467 |
| `accepted_gain_functions` | 56 |
| `accepted_gain_meaningful` | 30497 |
| `strict_new_functions` | 54 |
| `strict_new_meaningful` | 29021 |
| `strict_new_padded` | 29440 |
| `nonstrict_new_accepted_functions` | 2 |
| `nonstrict_new_accepted_meaningful` | 1476 |
| `strict_remaining_to_150k` | 120979 |

Regressions vs the 12f7375d4 baseline snapshot: 0

## Strict new exact functions by object

| object | function | padded | meaningful |
| --- | --- | ---: | ---: |
| ai/actor_moving | _actor_move_calculate_movement | 2752 | 2744 |
| ai/actor_moving | _actor_aim_jump | 256 | 244 |
| bitmaps/bitmap_utilities | _bitmap_2d_uncompress_from_mipmap | 880 | 866 |
| bitmaps/bitmap_utilities | _bitmap_3d_shrink | 800 | 799 |
| bitmaps/bitmap_utilities | _bitmap_2d_shrink | 656 | 649 |
| interface/ui_widget_game_data_input_functions | _network_pregame_status_screen_update | 1984 | 1980 |
| interface/hud_draw | _get_flash_color | 608 | 607 |
| interface/hud_draw | _hud_draw_bitmap_internal | 544 | 544 |
| interface/hud_draw | _hud_draw_bitmap_with_meter | 384 | 374 |
| interface/hud_draw | _hud_draw_bitmap_direct | 320 | 310 |
| ai/ai_communication | _ai_conversation_begin | 1744 | 1735 |
| game/players | _players_reconnect_to_structure_bsp | 736 | 729 |
| game/players | _players_compute_combined_pvs | 368 | 354 |
| game/players | _player_teleport_on_bsp_switch | 320 | 320 |
| game/players | _unit_should_autopick_weapon | 144 | 134 |
| render/render_objects | _object_render_state_refresh | 800 | 786 |
| render/render_objects | _render_object_shadow_end | 720 | 711 |
| ai/encounters | _encounter_new | 656 | 652 |
| ai/encounters | _encounter_control_actors | 448 | 436 |
| ai/encounters | _encounter_place_actor | 368 | 368 |
| bitmaps/bitmap_extract | _extract_plateless_cube_map | 832 | 824 |
| bitmaps/bitmap_extract | _extract_get_bitmap_format | 576 | 568 |
| camera/editor_flying_camera | _editor_camera_orbiting_update | 1120 | 1117 |
| ai/action_vehicle | _action_vehicle_perform | 768 | 767 |
| ai/action_vehicle | _action_vehicle_setup_specific | 336 | 328 |
| ai/path_obstacle_avoidance | _render_debug_path | 608 | 604 |
| ai/path_obstacle_avoidance | _path_new | 480 | 476 |
| cache/cache_files_decompress_windows | _cache_copy_update_write_buffers | 496 | 484 |
| cache/cache_files_decompress_windows | _cache_copy_get_status | 400 | 389 |
| cache/cache_files_decompress_windows | _acquire_read_request | 112 | 104 |
| sound/sound_manager | _sound_new_impulse | 976 | 970 |
| game/game_engine | _find_closest_player_index | 448 | 444 |
| game/game_engine | _game_engine_get_place | 368 | 367 |
| game/game_engine | _multiplayer_message | 144 | 140 |
| rasterizer/xbox/rasterizer_xbox_environment | __rasterizer_environment_diffuse_light_begin | 928 | 914 |
| rasterizer/rasterizer_text | _rasterizer_draw_character_with_dropshadow | 368 | 354 |
| rasterizer/rasterizer_text | _rasterizer_draw_character | 240 | 228 |
| rasterizer/rasterizer_text | _rasterizer_text_cache_initialize | 144 | 139 |
| interface/motion_sensor | _blip_type_get | 352 | 351 |
| interface/motion_sensor | _should_draw_object | 224 | 211 |
| interface/motion_sensor | _motion_sensor_blip_set_type_and_size | 96 | 87 |
| ai/actor_looking | _actor_look_find_random_vector | 608 | 595 |
| ai/actor_perception | _actor_perception_desire_prop | 560 | 549 |
| hs/hs_runtime | _hs_evaluate_inequality | 528 | 525 |
| interface/hud_weapon | _hud_render_weapon_interface | 496 | 494 |
| ai/props | _prop_get_base_by_unit_index | 400 | 395 |
| effects/decals | _decal_update | 400 | 387 |
| game/aim_assist | _find_aim_assist_targets | 304 | 294 |
| interface/hud_messaging | _scripted_hud_set_objective | 160 | 147 |
| interface/hud_messaging | _scripted_hud_get_timer_ticks | 80 | 70 |
| interface/ui_widget | _widget_instance_render_column_list | 176 | 163 |
| effects/contrails | _contrail_scale_random_value | 112 | 102 |
| ai/ai_script | _ai_scripting_allegiance_broken | 80 | 65 |
| items/weapons | _animation_update | 32 | 27 |

## Newly accepted but NOT strict (excluded from the 150K count)

| object | function | meaningful | proof |
| --- | --- | ---: | --- |
| interface/ui_widget_game_data_input_functions | _player_profile_update_cache_for_nwide_list | 268 | objdiff,objdiff-coff-shape |
| interface/ui_widget_game_data_input_functions | _splitscreen_pregame_status_screen_update | 1208 | objdiff,objdiff-coff-shape |

## Checkpoint verification (w1 Batch A)

- Full `ninja all_source progress semantic_progress`: pass (accepted 7,205 / 1,275,467).
- Stable diff from `scratch/claude150k-before.json` (sha d86c3063...): 54 gained / 29,440 padded / **0 regressions**.
- Parks: 355 active / 0 stale / 0 invalid. 36 parks were removed. Three already-fuzzy parks were re-measured with evidence (bitmap_3d_smooth, motion_sensor_update, update_motion_sensor).
- Object admission audit: 0 candidates / 0 contradictions / 7 rejections (unchanged from baseline).
- pytest tools: see the commit message.
- Fake-match scan: no new leads in any of the 30 changed files. `git diff --check`: pass.
- Accounting: two functions accepted only by objdiff-shape (1,476 meaningful) are excluded from strict credit.
