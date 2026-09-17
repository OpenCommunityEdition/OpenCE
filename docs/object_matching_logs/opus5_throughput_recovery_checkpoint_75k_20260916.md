# Throughput-recovery campaign: +75K checkpoint (2026-09-16)

Supersedes the running totals of the +60K record; earlier rejections and disclosures still apply. Frozen baseline
`9062b1e96`; branch `opus/throughput-recovery-150k-20260916`; nothing pushed. Manifest:
`opus5_throughput_recovery_checkpoint_75k_20260916.json`.

## Measured result (vs the frozen 9062b1e96 stable snapshot)

| measure | baseline | now | delta |
| --- | ---: | ---: | ---: |
| strict meaningful exact bytes (credited) | - | - | **+75,141** |
| strict padded exact bytes (board) | 1,397,165 | 1,473,069 | +75,904 |
| exact functions (board) | 7,279 | 7,378 | +99 |
| all-function-exact objects (board) | 309 | 327 | +18 |
| whole-object admission candidates | 8 | 22 | +14 |
| accepted ledger | 7,315 / 1,347,469 | 7,414 / 1,422,610 | +99 / +75,141 |
| active park entries (incl. 2 asm-implemented) | 268 | 209 | -59 |
| inherited regressions | - | - | **0** |

## Since the +60K record

| commit | content | functions | padded |
| --- | --- | ---: | ---: |
| `33985aa2a` | wave 11: _dsound_initialize, _extract_build_texture_pages_by_sequence | 2 | 1,328 |
| `336fecdbb` | wave 12: _connected_geometry_add_triangle, _render_inverse_transform_screen_point (texture and sound cache) | 3 | 928 |
| `fa96a1ee0` | symbols.json in-place rename `_bss_004d1198` -> `_texture_cache_debug_bitmaps` (static, provisional name); _texture_cache_debug_render | 1 | 784 |
| `dcede5fd1` | wave 13: _rasterizer_draw_string, _rasterizer_draw_unicode_string, _ai_scripting_renew, _ai_index_from_string | 4 | 1,696 |
| `5c3ea7ab2` | wave 16: six park reopens (render_debug, ctf, action_uncover, bitmaps_quantitize, lruv_cache, action_guard) | 6 | 3,152 |
| `06e5f0e5f` | wave 15: _hs_compile_and_evaluate, _inverse_kinematics_adjust_matrices, _actor_stimulus_prop_acknowledged, _lrar_allocate, _prop_add, _director_script_camera | 6 | 3,952 |

Wave 14 (second pass over actor_moving, physics, decals, projectiles, hud_weapon, ui_widget, encounters, weapons;
62,032 assigned bytes) closed nothing: the residuals there are register/x87 ties or held forms.

## Checkpoint verification

Full `ninja` after every packet; stable diff vs 9062: 99 gained, 0 regressions. `tools.parked_functions`: 209 active,
0 stale, 0 invalid. `audit_object_admission`: 22 candidates / 0 contradicted / 8 rejected / 0 revoked. pytest 1,151
passed, 5 skipped, 26 subtests. Fake-match scan 22 (= baseline, no new rule/path). `_point_from_line3d` emitters 7
(= baseline). Per-file audit of all 62 changed C files: 0 flagged. New owner deltas since +60K are select-any
real_math inline copies (`_add_vectors3d`, `_random_range`) of the admitted A4/A30 class. Caller parks re-measured
after static-helper fixes: `_texture_cache_debug_render` (then landed), `_sound_cache_debug_render` (non-exact before
and after).

## Disclosed judgement calls (landed)

- `_lruv_debug_to_file`: `MIN(9999, age)` integer-compare operand order changes the bytes; constant-first MIN is
  attested in Bungie source (network_server_manager.c); the steering hold covers x87 permutations.
- `_render_debug_add_cache_entry`: one case body per debug type (January emits separately scheduled identical bodies).
- `_texture_cache_debug_bitmaps`: descriptive provisional name replacing a forbidden bss+address placeholder.

## Rejected or held since +60K (never landed)

| function | reason |
| --- | --- |
| `_player_control_modify_desired_angles` | factor order chosen from a 12-order search (operand permutation steering) - owner-ruling queue |
| `_compute_sound_obstruction` | new `_vector_from_points3d` COMDAT pending owner ruling |
| `_update_potentially_audible_looping_sound`, `_render_debug_trigger_volumes` | duplicated arm stores |
| `_path_avoid_obstacles`, `_render_debug_obstacle_path` | replace a recorded .bss name with unauthenticated labels |
| `_light_volume_render`, `_actor_firing_position` (`_pre_evaluator_attack`) | point_from_line3d COMDAT |
| `_rasterizer_vertex_buffer_new`, `_rasterizer_triangle_buffer_new` | exact only with the January-proven redundant `locked_vertices = NULL` store (same class as `_encounter_test_rule`, owner-ruling queue); the XDK Unlock symbol renames themselves are authentic |

## Owner-ruling queue (measured unlocks)

point_from_line3d COMDAT (~13K B across ~9 functions), P7 projection_from_vector3d header body (1,328 B; no exact
loss board-wide), January-proven redundant stores (`_encounter_test_rule` 656, hardware geometry pair 720),
`_vector_from_points3d` COMDAT (512), `_player_control_modify_desired_angles` factor order (1,584), UB forms
(uninitialized/NULL/out-of-bounds reads).
