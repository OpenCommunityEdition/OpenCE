# Opus5 150K house-clean lane — HANDOFF (2026-09-14)

Branch `opus/150k-house-clean-20260914`, worktree `C:\halo-worktrees\opus5-150k-house-clean-20260914`.
Base `12f7375d4b845725c5ae3fda7ad715998ee54975` (7,149 functions / 1,244,970 meaningful). Final commit: `fe0ec76e8026746f3dde34b8a99a5ffae26f6908 (plus the handoff docs commit on top)`.
Nothing was pushed. The canonical checkout and the 100K donor worktree were not modified.

## 1. Result

| measure | value |
| --- | ---: |
| Strict new exact functions (stable diff vs 12f7375d4 snapshot) | 144 |
| **Strict new meaningful bytes (counts toward 150K)** | **89,085** |
| Strict new padded bytes (reported separately) | 90,112 |
| Target | 150,000 |
| Shortfall | 60,915 |
| Regressions vs baseline snapshot | 0 |
| Accepted ledger (build/semantic_report.json) | 7,297 / 1,336,250 |
| Newly accepted but NOT strict (objdiff-shape only; zero credit) | 4 fns / 2,195 B |
| Parks | 391 at base -> 280 active, 0 stale, 0 invalid |

**The 150K target was not reached.** The campaign was truncated at the owner's request, with about 15% of the weekly token
budget left, to bank completed and verified results. Wave 3b (50 groups of smaller units) and 24 of the 40 wave 3a
groups were never run; wave 3a ran truncated to its 16 largest groups, and wave 3b ran only its first 2 batches (16 of 46 groups). Section 7 lists the remaining reachable pool.

The inherited 100K donor work (+104,809) is excluded: the accounting subtracts the 100K worktree's accepted ledger and
counts only strict `section_infos_equal` gains in the stable diff against `scratch/claude150k-before.json`. That
snapshot is byte-identical to the 100K final snapshot, verified at startup.

Per-function evidence is in `opus5_150k_house_clean_admission_manifest_20260914.json`, which records each function's
unit, section, padded and meaningful bytes, relocations, sha and commits.

## 2. Waves

| wave | scope | landed strict | notes |
| --- | --- | ---: | --- |
| w1 | unwritten graphs (actor_perception, aim_assist, hud_draw, weapons, stubs) + families + parks C; 57 units | 54 fns / 29,021 B | 30 per-object commits, checkpoint ea9175577 |
| w2 | park re-investigation + residuals; 52 groups / 83 units | 54 fns / 33,824 padded (source-only) + 15 fns / 8,960 padded (config batch) | laws in scratch/w2/laws_w2.md |
| packets | P1 ai_debug.h, P3 actions.h, P4 bipeds.h, P5 path.h, P6 real_math.h (+ P2 zero credit) | 6 fns / 7,248 padded (P1 4,672; P3 160; P4 288; P5 816; P6 1,312) | P7 reverted, P8/P9/P10 rejected |
| w1 Batch B | config-held w1 landables | 2 fns / 864 padded | path, particle_systems; leaf_map zero credit |
| w3a (truncated) | second pass, 16 largest-residual units, run as two batches of 8 | 7 fns / 5,536 padded (actor_moving x3, actor_looking, projectiles, actor_perception, bipeds via units.h/weapons.h) | the first two launches were killed by session limits; the third completed |
| w3b (truncated) | second pass over 46 smaller groups (4 skipped as do-not-repeat-only), ordered by bytes; stopped after batches 1-2 (16 groups) for credits | 6 fns / 4,240 padded (rasterizer_text, particles x2, lightning, bitmap_extract, action_flee via units.h) | batch 3 was stopped within ~15 s of starting; nothing from it was used |

## 3. Header and config changes (each with its consumers)

Every change had its own commit, a full `ninja` build and a whole-board stable sweep with zero regressions.

- `source/ai/ai_debug.h` (b5b26e412): nested `burst_alignment` struct replaces `char __unknown1C[40]`, layout identical.
  26 consumers were shadow-compiled; the only change is `_actor_combat_update` (+4,672, 3611d423d). The flat spellings regress `units` by
  declaration count; the nested aggregate does not.
- `source/game/player_control.h` (1839ef168): removes the extern for the file-private `player_control_globals`. 21
  consumers: player_control, ai_debug, bored_camera, director, editor_flying_camera, following_camera, player_effects,
  aim_assist, game, game_engine, players, hs_library_external, input_abstraction, first_person_weapons, hud, hud_draw,
  hud_weapon, interface, motion_sensor, ui_widget, main.
- `config/symbols.json` in-place `"static": true` (static-linkage law A1): `_animation_threads_debug`,
  `_game_allegiance_globals`, `_player_control_globals`, `_update_server_globals`, `_update_client_globals`,
  `_suppress_window_begin_end`, `_previous_window_index`, `_cinematic_screen_effect_globals`, `_sequential_counter`.
  Each affects only its own split object; game_allegiance also relabels player_queues_new.obj.
- `config/symbols.json` labels: insert `_geosphere_primitive_triangles` (geometry.obj); rename `_code_0004c7b0` ->
  `_path_heap_verify` (static; Bungie 2001-09-25 linker map); split `_rdata_0025a6b8` into the HCEX-PDB-named
  `_system_update_functions`, `_particle_creation_functions`, `_particle_update_functions`, and add `_seconds_per_tick`
  (particle_systems.obj).
- `source/units/bipeds.h` (276b0368f): `biped_approximate_surface_index` returns `long` (January caller `cmp eax,-1`).
  23 consumers; +288 (`_ai_scripting_teleport_starting_location_private`, park removed).
- `source/ai/actions.h` + `source/ai/ai_runtime.h` (ac4ff3d92): owner prototype for `actor_action_set_default_state`
  (ai_script C4013), plus removal of the verbatim duplicate `ai_reconnect_to_structure_bsp` prototype so that units.c's
  declaration count is unchanged. Whole board (446 units) shadow-compiled; +160.
- `source/ai/path.h` (2ac274833): `obstacle_from_disc` returns `short`. 55 consumers; +816 (`_path_add_steps`).
- `source/math/real_math.h` (21c41cb4b): `real_local_random_range` named result, justified by a scan of 9 January inline
  sites. All 572 cl edges shadow-compiled; +608 glow, plus +704 weather via a genuine `#include "bitmaps/bitmaps.h"`.
- `source/units/units.h` + `source/items/weapons.h` (082f9171b, wave w3a): owner prototypes for `unit_animation_start_action`, `unit_cause_player_melee_damage`, `unit_update_animation` (plus a forward declaration of `struct unit_animation_update_data`), `weapon_stop_reload` and `weapon_prevents_melee_attack`, which bipeds.c had been calling through C4013 implicit declarations. 76 consumers were blast-compiled, including units.c, vehicles.c and weapons.c, whose objects did not change; the only other change is the hash of the non-exact physics `_physics_update_old`. This is +1,120 (`_biped_update`, park removed). The header and bipeds.c are in one commit because the header alone moves the parked body's hash.
- `source/units/units.h` (b9030877e, wave w3b): owner prototypes for `unit_start_running_blindly` (units.c) and `unit_is_speaking` (unit_dialogue.c), which action_flee.c had been calling through C4013 implicit declarations. Both are required together: the first alone regresses units::_unit_preprocess_node_orientations by declaration count. This is +864 (`_action_flee_perform`, park removed). The already-fuzzy ai_communication park `_ai_communication_update_speech_timers` was re-measured.
- `source/scenario/scenario_definitions.h` (cc93a1fd1): scenario weapon datum owner types (HCEX PDB). 55 consumers, zero
  credit; `_weapon_place` written as a fuzzy body.
- `config/parked.json`: only in-place edits. It removed every park that became exact (unpark tool) and re-measured
  already-fuzzy parks with appended dated evidence: hud `_hud_show_action_response`, action_flee `_action_flee_perform`,
  encounters `_encounter_update_respawn` (toggles with declaration count under P3/P2), bitmap_group
  `_bitmap_group_add_bitmap`, rasterizer_xbox_models `__rasterizer_model_draw`, leaf_map
  `_leaf_map_build_portals_from_leaf` (reclassified `csplit-relocation-alias`), plus the w1 re-measures listed in the
  checkpoint ledger.

## 4. Rejected trials and held items (zero credit, not landed)

- **P7** `projection_from_vector3d` three-return fix plus leaf_map (+960): clean in isolation, but stacked on P6 it
  REGRESSED `_bitmap_copy` (2,784). Reverted, and the revert was verified identical to the P6 state. Note that the current
  header body is semantically wrong (January's decals.obj COMDAT proves the three-return logic). An owner decision is
  needed on how to land the fix without the bitmap_drawing loss.
- **P8** `_pi` as a float literal: 6 strict regressions, and January folds `DEGREES_TO_RADIANS` with double pi. Do not retry.
- **P9** aim_assist cone owner / microheader: zero strict credit on every path; `_player_aim_projectile` residual is
  a normalize3d push placement.
- **P10** symbols packets: rejected by the csplit evaluation (scratch/packets/P10_symbols_packets).
- **Held for owner rulings:**
  - collision_bsp `_collision_surface_test_sphere` (880): exact only with a hand-written SSE `__asm` distance macro in a
    non-helper body (house rule 10).
  - rasterizer_xbox_environment_fog `screen_active` + `model_submit` (800): a 21-static .bss split in which 15 names are
    invented descriptive names.
  - rasterizer_xbox_models `__rasterizer_model_transparent_geometry_submit` (1,296): a 13-static split in which 5 names
    have no record.
  - editor_flying_camera `_editor_camera_update` (304): the split keeps `is_scripted` external as a layout anchor and
    defines an unreferenced static.
  - The `_point_from_line3d`-COMDAT ruling family (about 6,080 B; laws_w2 C3) is out under the lane rule.
- **Verifier rejection patterns** R1-R12 (scratch/w2/laws_w2.md section B), e.g. a result temp in a one-return function,
  the same statement in both arms, commutative x87 operand swaps, hand-written `= {0}` expansion, `(long)NUMBEROF`
  casts, consumer-local prototypes, varargs/UB forms.
- **w3a:** `_actor_move_initialize` was REJECTED despite a strict-exact candidate, because a standing hold (actor_moving_obj_fable_independent_intake_20260906.md) requires independent provenance for its 15 constants, the linker-common arrays and the ray types. Nine of the 16 w3a groups landed nothing; their blockers are in their w3 ledgers.
- **w3b:**
  - Five cache_files_decompress_windows reopens (`_cache_copy_set_flag`, `_cache_copy_FileIOCompletionRoutine@12`, `_cache_copy_issue_read`, `_cache_copy_issue_write`, `_simple_cache_copy_thread@4`) are exact ONLY with the prohibited `volatile unsigned long flags`. REJECTED; do not retry without an owner policy change.
  - `_item_update` and `_pre_evaluator_attack` need a surplus `_point_from_line3d` COMDAT (lane rule: out).
  - `_ai_scripting_follow_target_ai` is exact only by reproducing a suspicious original bug.
  - `_compare_profile_sections` is exact, but rejected on policy (see the profile w3 ledger).
  - `_actor_action_try_to_dive` needs the unlanded short `animation_impulse` prerequisite.
  - `_rasterizer_sun_glow_draw`: two strict-exact candidates were rejected for authenticity (steering pointer local / representation), per the rasterizer_xbox_lights w3 ledger.

## 5. Do-not-repeat findings

- scratch/w2/laws_w2.md section D: 25 tie verdicts with no more than 5 shapes spent (e.g. `_hud_show_action_response`
  EBX/EDI mirror, `_king_calculate_hill_state` store merge, `_file_get_size` callee-saved swap, `_hs_add_script`
  esi/edi), plus 10 skipped as exhausted.
- The original do-not-spend list still stands: `_rasterizer_transparent_geometry_group_draw`, `_actor_move_update`,
  `_weapon_update`, `_collision_move_point`, `_action_obey_control`, the ui_widget IV pair, `_structure_test_pill2d`,
  `_ai_debug_render_actor`, and the profiler switch without authentic enum names.
- Header packets that are each clean can regress together (P6 + P7). Always rebuild and sweep each packet on top of the
  previous ones.
- The w3a and w3b ledgers record, per unit, the shapes spent in the second pass with the w2 laws. Of the 32 groups run, 20 landed nothing (e.g. ai_communication, physics, rasterizer_xbox_models, hud_weapon, ui_widget, render_cameras, hud_messaging/virtual_keyboard, hud_nav_points, observer/editor_flying_camera, stack_walk_windows/profile, players/aim_assist). Read those ledgers before spending on these objects again.

## 6. Verification (final)

Run at `fe0ec76e8` (the handoff commit only adds these docs):

- `ninja`: pass, and the parked-function verification inside it passes.
- `stable_verdicts snapshot scratch/claude150k-final.json`, diffed against `scratch/claude150k-before.json`: **144 gained / 90,112 padded / 0 regressions** (8,245 functions, 7,263 exact).
- `python -B -m tools.parked_functions`: 280 active / 0 stale / 0 invalid.
- `python -B -m tools.audit_object_admission`: pass. It reports 0 contradicted and 7 rejected (unchanged from baseline), plus **7 new whole-object candidates**, where every function and data owner is exact: action_flee, game_statistics, input_abstraction, network_server_message_handler, rasterizer_cinematics, rasterizer_swizzle, rasterizer_xbox_decals (decision `audit-coff-ownership-before-admission`). These are not admitted by this lane and carry no credit.
- `tools/fake_match_scan.py` over all 77 changed .c files: 10 leads at HEAD, identical to the same files at the base commit (profile.c rdtsc macro x7, decals.c and hud_draw.c inline-asm/codegen leads). No new leads.
- `pytest --basetemp scratch/pytest-final`: 1151 passed, 5 skipped.
- `git diff --check 12f7375d4 HEAD`: pass. `git status --short`: no tracked changes. Untracked files are local `scratch/` evidence only and were not committed (objects, IDA/Ghidra exports, shadow trees).
- Zero-credit fuzzy or close bodies landed (reported separately, not counted):
  - w1: action_vehicle `_action_vehicle_find_destination` and `_action_vehicle_desirable`, hud_weapon `_crosshairs_draw`, ui_widget_game_data_input_functions `_player_profile_update_cache_for_nwide_list` and `_splitscreen_pregame_status_screen_update`.
  - w2: action_charge `_action_charge_setup`.
  - P2: weapons `_weapon_place`.
  - Resolved-address identical csplit aliases: leaf_map `_leaf_map_build_portals_from_leaf` (w1 Batch B), collision_usage `_collision_log_get_current_user` (w3b).
  - w3a: actor_perception `_prop_status_refresh` and `_actor_situation_update`, decals `_decal_new_from_collision` (no `point_from_line3d` form, guard passes).
  - The accepted ledger credits 4 of them through objdiff shape only (ui_widget_game_data_input_functions `_player_profile_update_cache_for_nwide_list`, ui_widget_game_data_input_functions `_splitscreen_pregame_status_screen_update`, collision_usage `_collision_log_get_current_user`, leaf_map `_leaf_map_build_portals_from_leaf`; 2,195 B), and they are excluded from the 150K count.

## 7. Remaining blockers and reachable pool

- At the w3 launch HEAD (before the truncated w3a/w3b landings), 150 units had about 391,700 B of residual/unwritten padded bytes, excluding the protected
  files and `rasterizer_xbox_transparent_geometry`; 280 parks remain active after w3a/w3b. Wave 3 groups for the un-run units are
  prepared in `scratch/wave3b-groups.json` and entries 17-40 of `scratch/wave3a-groups.json` (1-16 ran as the truncated w3a) and groups 17-46 of `scratch/wave3b-args-keep.json` (1-16 ran as the truncated w3b) (with notes, C4013
  census `scratch/w3/c4013.txt`, laws `scratch/w2/laws_w2.md`, brief section 9b).
- Highest-yield next steps, in order:
  1. Park re-investigation with laws A1-A21 over the un-run units (w2 closed 69 functions this way).
  2. The static-linkage census for header-extern globals (king_globals first).
  3. Owner rulings on the held items in section 4.
  4. The P7 + `_bitmap_copy` interaction.

## 8. Changed files

89 source/config files:

- `config/parked.json`
- `config/symbols.json`
- `source/ai/action_charge.c`
- `source/ai/action_flee.c`
- `source/ai/action_vehicle.c`
- `source/ai/actions.c`
- `source/ai/actions.h`
- `source/ai/actor_combat.c`
- `source/ai/actor_firing_position.c`
- `source/ai/actor_looking.c`
- `source/ai/actor_moving.c`
- `source/ai/actor_perception.c`
- `source/ai/ai_communication.c`
- `source/ai/ai_debug.h`
- `source/ai/ai_runtime.h`
- `source/ai/ai_script.c`
- `source/ai/encounters.c`
- `source/ai/path.c`
- `source/ai/path.h`
- `source/ai/path_obstacle_avoidance.c`
- `source/ai/path_obstacles.c`
- `source/ai/props.c`
- `source/bink/bink_playback.c`
- `source/bitmaps/bitmap_drawing.c`
- `source/bitmaps/bitmap_extract.c`
- `source/bitmaps/bitmap_utilities.c`
- `source/bitmaps/bitmaps.c`
- `source/cache/cache_files_decompress_windows.c`
- `source/cache/xbox_sound_cache.c`
- `source/cache/xbox_texture_cache.c`
- `source/camera/director.c`
- `source/camera/editor_flying_camera.c`
- `source/cseries/profile.c`
- `source/cutscene/recorded_animations.c`
- `source/effects/contrails.c`
- `source/effects/decals.c`
- `source/effects/effects.c`
- `source/effects/particle_systems.c`
- `source/effects/particles.c`
- `source/effects/weather_particle_systems.c`
- `source/game/aim_assist.c`
- `source/game/game_allegiance.c`
- `source/game/game_engine.c`
- `source/game/game_engine_king.c`
- `source/game/game_statistics.c`
- `source/game/player_control.c`
- `source/game/player_control.h`
- `source/game/player_queues_new.c`
- `source/game/players.c`
- `source/hs/hs_runtime.c`
- `source/input/input_abstraction.c`
- `source/input/input_xbox.c`
- `source/interface/first_person_weapons.c`
- `source/interface/hud.c`
- `source/interface/hud_draw.c`
- `source/interface/hud_messaging.c`
- `source/interface/hud_weapon.c`
- `source/interface/motion_sensor.c`
- `source/interface/ui_widget.c`
- `source/interface/ui_widget_game_data_input_functions.c`
- `source/items/projectiles.c`
- `source/items/weapons.c`
- `source/items/weapons.h`
- `source/math/geometry.c`
- `source/math/real_math.h`
- `source/networking/network_server_message_handler.c`
- `source/objects/widgets/lightning.c`
- `source/physics/collision_usage.c`
- `source/rasterizer/rasterizer_cinematics.c`
- `source/rasterizer/rasterizer_swizzle.c`
- `source/rasterizer/rasterizer_text.c`
- `source/rasterizer/xbox/rasterizer_xbox.c`
- `source/rasterizer/xbox/rasterizer_xbox_decals.c`
- `source/rasterizer/xbox/rasterizer_xbox_environment.c`
- `source/rasterizer/xbox/rasterizer_xbox_shadows.c`
- `source/rasterizer/xbox/rasterizer_xbox_water.c`
- `source/rasterizer/xbox/rasterizer_xbox_widgets.c`
- `source/render/render_debug.c`
- `source/render/render_objects.c`
- `source/scenario/scenario_definitions.h`
- `source/sound/sound_manager.c`
- `source/structures/leaf_map.c`
- `source/structures/structure_detail_objects.c`
- `source/structures/structures.c`
- `source/tag_files/files_windows.c`
- `source/units/bipeds.c`
- `source/units/bipeds.h`
- `source/units/unit_dialogue.c`
- `source/units/units.h`

Plus 204 files under `docs/object_matching_logs/` (per-object w1/w2/w3 ledgers, checkpoint, manifest, this handoff).

## 9. Dependency-aware integration order

Apply the commits in branch order (`git log --reverse 12f7375d4..HEAD`). The history is linear, and
each header or config prerequisite commit precedes the source commit that depends on it:
ai_debug.h -> actor_combat; player_control.h -> player_control; symbols.json static/label lines ride in the same
commit as their unit; bipeds.h, actions.h/ai_runtime.h, path.h, real_math.h -> weather include; scenario_definitions.h
-> weapons. `config/parked.json` is edited in place in every per-object commit, so cherry-picking individual objects
out of order will conflict on parked.json (and fail the ninja park check). Integrate as one ordered range.

## 10. Overlap note

Codex's separate verified networking packet `faae809ab` owns
`source/bungie_net/network/transport_endpoint_set_winsock.c`, `source/bungie_net/network/transport_endpoint_winsock.c`
and `source/networking/network_connection.c`. This lane did not touch them, nor `source/math/matrix_math.c`,
`source/units/units.c` or `source/units/vehicles.c`
(`git diff --name-only 12f7375d4 HEAD` contains none of the six). The header packets were blast-tested
over their consumers, including units.c and vehicles.c where transitive, with zero changes to those protected objects.
`source/networking/network_server_message_handler.c` (w2, 2 park reopens) is a different file from the Codex packet.
