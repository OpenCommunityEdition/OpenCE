# Opus5 150K house-clean lane: checkpoint-w2A

| total | value |
| --- | ---: |
| `head_commit` | d0c2aed7b0bead84df7ab113eb331fac355f7b72 |
| `accepted_functions` | 7259 |
| `accepted_meaningful` | 1308903 |
| `accepted_gain_functions` | 110 |
| `accepted_gain_meaningful` | 63933 |
| `strict_new_functions` | 108 |
| `strict_new_meaningful` | 62457 |
| `strict_new_padded` | 63264 |
| `nonstrict_new_accepted_functions` | 2 |
| `nonstrict_new_accepted_meaningful` | 1476 |
| `strict_remaining_to_150k` | 87543 |

Regressions vs the 12f7375d4 baseline snapshot: 0

## Strict new exact functions by object

| object | function | padded | meaningful |
| --- | --- | ---: | ---: |
| ai/actor_moving | _actor_move_calculate_movement | 2752 | 2744 |
| ai/actor_moving | _actor_aim_jump | 256 | 244 |
| bitmaps/bitmap_drawing | _bitmap_copy | 2784 | 2784 |
| ai/actor_firing_position | _pre_evaluator_global | 1616 | 1603 |
| ai/actor_firing_position | _pre_evaluator_combatmove | 576 | 567 |
| ai/actor_firing_position | _post_evaluator_global | 224 | 214 |
| input/input_abstraction | _input_abstraction_update | 2384 | 2384 |
| bitmaps/bitmap_utilities | _bitmap_2d_uncompress_from_mipmap | 880 | 866 |
| bitmaps/bitmap_utilities | _bitmap_3d_shrink | 800 | 799 |
| bitmaps/bitmap_utilities | _bitmap_2d_shrink | 656 | 649 |
| rasterizer/xbox/rasterizer_xbox_water | _rasterizer_water_draw | 2240 | 2240 |
| interface/ui_widget_game_data_input_functions | _network_pregame_status_screen_update | 1984 | 1980 |
| interface/hud_draw | _get_flash_color | 608 | 607 |
| interface/hud_draw | _hud_draw_bitmap_internal | 544 | 544 |
| interface/hud_draw | _hud_draw_bitmap_with_meter | 384 | 374 |
| interface/hud_draw | _hud_draw_bitmap_direct | 320 | 310 |
| rasterizer/xbox/rasterizer_xbox_widgets | __rasterizer_widget_begin | 1152 | 1139 |
| rasterizer/xbox/rasterizer_xbox_widgets | __rasterizer_widget_submit_occlusion_test | 640 | 626 |
| game/game_engine_king | _king_engine_post_rasterize | 928 | 916 |
| game/game_engine_king | _find_hill | 848 | 847 |
| ai/ai_communication | _ai_conversation_begin | 1744 | 1735 |
| physics/collision_usage | _collision_log_render | 1584 | 1582 |
| game/players | _players_reconnect_to_structure_bsp | 736 | 729 |
| game/players | _players_compute_combined_pvs | 368 | 354 |
| game/players | _player_teleport_on_bsp_switch | 320 | 320 |
| game/players | _unit_should_autopick_weapon | 144 | 134 |
| render/render_debug | _build_pill_points | 448 | 437 |
| render/render_debug | _render_debug_sphere | 400 | 389 |
| render/render_debug | _render_debug_circle | 304 | 293 |
| render/render_debug | _render_debug_point | 288 | 278 |
| render/render_debug | _build_circle_points | 112 | 102 |
| render/render_objects | _object_render_state_refresh | 800 | 786 |
| render/render_objects | _render_object_shadow_end | 720 | 711 |
| interface/first_person_weapons | _first_person_weapon_switch_weapons | 608 | 604 |
| interface/first_person_weapons | _first_person_weapon_message | 544 | 544 |
| interface/first_person_weapons | _first_person_weapon_render_update | 176 | 173 |
| interface/first_person_weapons | _first_person_weapon_set_visibility | 160 | 146 |
| rasterizer/xbox/rasterizer_xbox_shadows | __rasterizer_environment_shadow_draw | 1472 | 1458 |
| ai/encounters | _encounter_new | 656 | 652 |
| ai/encounters | _encounter_control_actors | 448 | 436 |
| ai/encounters | _encounter_place_actor | 368 | 368 |
| bitmaps/bitmap_extract | _extract_plateless_cube_map | 832 | 824 |
| bitmaps/bitmap_extract | _extract_get_bitmap_format | 576 | 568 |
| units/bipeds | _biped_fix_position | 1232 | 1224 |
| units/unit_dialogue | _unit_dialogue_update | 768 | 759 |
| units/unit_dialogue | _unit_notify_impulse_sound | 240 | 230 |
| units/unit_dialogue | _unit_dialogue_determine_variant | 176 | 174 |
| camera/editor_flying_camera | _editor_camera_orbiting_update | 1120 | 1117 |
| rasterizer/xbox/rasterizer_xbox_decals | __rasterizer_decals_draw | 848 | 840 |
| rasterizer/xbox/rasterizer_xbox_decals | __rasterizer_decals_initialize | 272 | 268 |
| ai/action_vehicle | _action_vehicle_perform | 768 | 767 |
| ai/action_vehicle | _action_vehicle_setup_specific | 336 | 328 |
| ai/path_obstacle_avoidance | _render_debug_path | 608 | 604 |
| ai/path_obstacle_avoidance | _path_new | 480 | 476 |
| cache/cache_files_decompress_windows | _cache_copy_update_write_buffers | 496 | 484 |
| cache/cache_files_decompress_windows | _cache_copy_get_status | 400 | 389 |
| cache/cache_files_decompress_windows | _acquire_read_request | 112 | 104 |
| sound/sound_manager | _sound_new_impulse | 976 | 970 |
| networking/network_server_message_handler | _network_game_server_handle_datagram | 784 | 778 |
| networking/network_server_message_handler | _network_game_server_handle_message_client_ping | 192 | 189 |
| game/game_engine | _find_closest_player_index | 448 | 444 |
| game/game_engine | _game_engine_get_place | 368 | 367 |
| game/game_engine | _multiplayer_message | 144 | 140 |
| camera/director | _director_update_controls | 960 | 949 |
| rasterizer/xbox/rasterizer_xbox_environment | __rasterizer_environment_diffuse_light_begin | 928 | 914 |
| structures/structures | _structure_clusters_in_cone | 480 | 476 |
| structures/structures | _structure_render_surface_from_point_and_leaf | 432 | 422 |
| cseries/profile | _profile_dump | 864 | 864 |
| rasterizer/rasterizer_swizzle | _rasterizer_xbox_bitmap_swizzle | 800 | 791 |
| bitmaps/bitmaps | _bitmap_format_to_a8r8g8b8 | 544 | 544 |
| bitmaps/bitmaps | _palette_find_closest_match | 256 | 245 |
| structures/structure_detail_objects | _render_debug_detail_objects | 752 | 740 |
| rasterizer/rasterizer_text | _rasterizer_draw_character_with_dropshadow | 368 | 354 |
| rasterizer/rasterizer_text | _rasterizer_draw_character | 240 | 228 |
| rasterizer/rasterizer_text | _rasterizer_text_cache_initialize | 144 | 139 |
| tag_files/files_windows | _file_open | 400 | 391 |
| tag_files/files_windows | _file_read | 176 | 168 |
| tag_files/files_windows | _file_write | 176 | 161 |
| game/game_statistics | _game_statistics_record_kill | 672 | 661 |
| interface/motion_sensor | _blip_type_get | 352 | 351 |
| interface/motion_sensor | _should_draw_object | 224 | 211 |
| interface/motion_sensor | _motion_sensor_blip_set_type_and_size | 96 | 87 |
| ai/action_flee | _action_flee_current_position_exposed | 352 | 347 |
| ai/action_flee | _action_flee_setup | 272 | 265 |
| cache/xbox_texture_cache | _texture_cache_build_hardware_format | 336 | 330 |
| cache/xbox_texture_cache | _texture_cache_start_loading_bitmap | 224 | 224 |
| cache/xbox_texture_cache | _texture_cache_locked_block_proc | 64 | 51 |
| ai/actor_looking | _actor_look_find_random_vector | 608 | 595 |
| ai/path_obstacles | _obstacles_disc_neighborhood | 560 | 551 |
| ai/actor_perception | _actor_perception_desire_prop | 560 | 549 |
| hs/hs_runtime | _hs_evaluate_inequality | 528 | 525 |
| interface/hud_weapon | _hud_render_weapon_interface | 496 | 494 |
| input/input_xbox | _input_get_raw_data_string | 240 | 237 |
| input/input_xbox | _input_flush_rumble | 176 | 173 |
| ai/props | _prop_get_base_by_unit_index | 400 | 395 |
| effects/decals | _decal_update | 400 | 387 |
| cache/xbox_sound_cache | __sound_cache_sound_request | 352 | 348 |
| interface/hud | _hud_draw_screen | 320 | 306 |
| game/aim_assist | _find_aim_assist_targets | 304 | 294 |
| effects/effects | _effect_evaluate_function_integral | 176 | 176 |
| effects/effects | _effect_get_node_matrix | 80 | 68 |
| interface/hud_messaging | _scripted_hud_set_objective | 160 | 147 |
| interface/hud_messaging | _scripted_hud_get_timer_ticks | 80 | 70 |
| interface/ui_widget | _widget_instance_render_column_list | 176 | 163 |
| bink/bink_playback | _bink_playback_idle | 160 | 160 |
| effects/contrails | _contrail_scale_random_value | 112 | 102 |
| ai/ai_script | _ai_scripting_allegiance_broken | 80 | 65 |
| items/weapons | _animation_update | 32 | 27 |

## Newly accepted but NOT strict (excluded from the 150K count)

| object | function | meaningful | proof |
| --- | --- | ---: | --- |
| interface/ui_widget_game_data_input_functions | _player_profile_update_cache_for_nwide_list | 268 | objdiff,objdiff-coff-shape |
| interface/ui_widget_game_data_input_functions | _splitscreen_pregame_status_screen_update | 1208 | objdiff,objdiff-coff-shape |

## Checkpoint verification (w2 Batch A)

- Wave w2 (52 file groups, 83 units: park re-investigation and residuals) landed 31 source-only landables in per-object commits (1e936afab and predecessors) plus the ledger commits 97ed0eeee and the whitespace fix.
- Full `ninja` (all_source progress semantic_progress): pass.
- Stable diff from `scratch/claude150k-before.json` (sha d86c3063...): 108 gained / 63,264 padded / **0 regressions**. Batch-only diff from the w1 snapshot: 54 gained / 33,824 padded / 0 regressions.
- Parks: 306 active / 0 stale / 0 invalid. 49 parks were removed (all adversarially accepted reopens). Two already-fuzzy parks were re-measured with appended evidence (hud `_hud_show_action_response`, action_flee `_action_flee_perform`), both now target size and relocation count.
- Object admission audit: pass (unchanged rejections).
- pytest (`--basetemp scratch/pytest-ckB`): 1151 passed, 5 skipped.
- Fake-match scan of the 31 changed .c files: 7 leads, all the pre-existing `inline-assembly` rdtsc macro in cseries/profile.c (present at the base commit); no new leads. `git diff --check ea9175577 HEAD`: pass.
- Held (zero credit, not landed): collision_bsp `_collision_surface_test_sphere` (hand-written SSE `__asm` distance macro in a non-helper body; needs an owner asm ruling); geometry, player_control, recorded_animations, game_allegiance, player_queues_new, rasterizer_cinematics, rasterizer_xbox (need symbols.json static/label or header prerequisites; separate config batch).
- Accounting: `_action_charge_setup` landed as a zero-credit fuzzy body; two w1 objdiff-shape-only functions (1,476 meaningful) stay excluded from strict credit.

## Checkpoint verification (w1 Batch A)

- Full `ninja all_source progress semantic_progress`: pass (accepted 7,205 / 1,275,467).
- Stable diff from `scratch/claude150k-before.json` (sha d86c3063...): 54 gained / 29,440 padded / **0 regressions**.
- Parks: 355 active / 0 stale / 0 invalid. 36 parks were removed. Three already-fuzzy parks were re-measured with evidence (bitmap_3d_smooth, motion_sensor_update, update_motion_sensor).
- Object admission audit: 0 candidates / 0 contradictions / 7 rejections (unchanged from baseline).
- pytest tools: see the commit message.
- Fake-match scan: no new leads in any of the 30 changed files. `git diff --check`: pass.
- Accounting: two functions accepted only by objdiff-shape (1,476 meaningful) are excluded from strict credit.
