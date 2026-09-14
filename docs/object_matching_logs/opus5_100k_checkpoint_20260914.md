# Opus5 100K consolidated lane: checkpoint-40k

| total | value |
| --- | ---: |
| `head_commit` | f94f6d7db8c8375dd1b64e48cf1bfd426eb218c5 |
| `accepted_functions` | 7053 |
| `accepted_meaningful` | 1180466 |
| `gain_functions_vs_base` | 81 |
| `gain_meaningful_vs_base` | 40305 |
| `strict_gained_functions` | 86 |
| `strict_gained_padded` | 42368 |
| `strict_gained_meaningful` | 41718 |
| `remaining_to_target` | 59695 |

Regressions vs base snapshot: 0

## Gained functions by object

| object | function | padded | meaningful |
| --- | --- | ---: | ---: |
| ai/actor_firing_position | _actor_select_firing_position | 5248 | 5248 |
| ai/actor_firing_position | _actor_active_select_firing_position | 576 | 569 |
| ai/actor_firing_position | _firing_position_compute_line_of_sight | 464 | 455 |
| ai/actor_firing_position | _firing_position_compare | 272 | 264 |
| interface/hud_draw | _hud_draw_multitexture_overlay | 2272 | 2264 |
| interface/hud_draw | _hud_draw_numbers | 1856 | 1845 |
| interface/hud_draw | _hud_draw_meter | 1312 | 1298 |
| interface/hud_draw | _hud_draw_static_element | 640 | 627 |
| interface/hud_draw | _hud_draw_multitexture_overlay_get_current_weapon_definition | 480 | 468 |
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
| ai/actor_moving | _actor_move_calculate_free | 720 | 717 |
| ai/actor_moving | _actor_move_calculate_controlled_by_aiming | 640 | 626 |
| ai/actor_moving | _midpoint3d | 64 | 56 |
| physics/collisions | _collision_fix_pill | 672 | 657 |
| physics/collisions | _collision_get_features_in_sphere | 512 | 507 |
| physics/collisions | _collision_fix_pill_nudge_collision | 160 | 147 |
| items/projectiles | _projectile_detonate | 1040 | 1031 |
| items/projectiles | _projectile_effect_new | 96 | 92 |
| items/projectiles | _projectile_set_action | 32 | 28 |
| render/render_objects | _render_object_list | 1152 | 1152 |
| physics/physics | _physics_compute_biped_collision | 1136 | 1133 |
| ai/actor_combat | _actor_start_first_burst_delay | 208 | 205 |
| ai/actor_combat | _actor_start_pause | 176 | 173 |
| ai/actor_combat | _actor_combat_allow_special_fire_situation | 176 | 173 |
| ai/actor_combat | _actor_combat_get_firing_parameters | 160 | 151 |
| ai/actor_combat | _actor_combat_enable_special_fire_situation | 144 | 139 |
| ai/actor_combat | _fast_ftol | 32 | 17 |
| ai/actor_combat | _random_boolean | 32 | 26 |
| game/players | _player_examine_nearby_item | 800 | 793 |
| units/bipeds | _biped_update_jumping | 512 | 509 |
| units/bipeds | _biped_start_landing | 272 | 257 |
| effects/decals | _decal_insert | 704 | 692 |
| interface/ui_widget | _ui_widget_load_children_recursive | 672 | 672 |
| sound/sound_manager | _refresh_sounds | 544 | 543 |
| sound/sound_manager | _sound_travel_milliseconds | 32 | 18 |
| game/aim_assist | _object_compute_autoaim_target | 416 | 413 |
| game/aim_assist | _limit3d | 96 | 88 |
| game/aim_assist | _reciprocal_square_root | 16 | 16 |
| game/game_engine | _select_players_to_display | 496 | 494 |
| rasterizer/rasterizer_frame_statistics | _rasterizer_frame_statistics_get_fps | 384 | 375 |
| render/render_sprite | _build_sprite_get_group | 384 | 381 |
| models/model_animations | _animation_update_internal | 304 | 300 |
| interface/motion_sensor | _motion_sensor_tick | 112 | 99 |
| interface/motion_sensor | _get_motion_sensor_data | 64 | 61 |
| rasterizer/xbox/rasterizer_xbox_active_camouflage | _real_alpha_to_pixel32 | 112 | 99 |
| cseries/stack_walk_windows | _symbol_sort_proc | 48 | 47 |
| rasterizer/xbox/rasterizer_xbox_environment | _IDirect3DDevice8_SetTexture@12 | 16 | 12 |

## Checkpoint verification (+40K)

- Full `ninja all_source progress semantic_progress`: pass.
- Stable diff from base snapshot `scratch/before.json` (sha 39557d49...): 86 gained, 0 regressions.
- Parks: 465 active / 0 stale / 0 invalid. The speech-timer park was re-measured once, under the ai_debug.h packet.
- Object admission audit: 0 candidates / 0 contradictions / 7 rejections.
- pytest tools at checkpoint 1: 1,151 passed / 5 skipped.
- `git diff --check`: pass.
- Fake-match scan: no new leads in any changed file.
- `_point_from_line3d` guard: pass on every worker gate.
- Treemap input: `scratch/report-ck40k.json`, viewable with `python decomp_treemap.py scratch/report-ck40k.json`.
