# Throughput-recovery campaign: +10K checkpoint (2026-09-16)

Worktree `C:\halo-worktrees\opus5-throughput-recovery-20260916`, branch `opus/throughput-recovery-150k-20260916`,
frozen baseline `9062b1e969cb3c7626d92381f6d10beaa7b19864`. Nothing pushed; canonical checkout, the old lane and
`jonas/exact-pilots` untouched. Machine-readable manifest: `opus5_throughput_recovery_checkpoint_10k_20260916.json`.

## Measured result (vs the frozen 9062b1e96 stable snapshot)

| measure | baseline | now | delta |
| --- | ---: | ---: | ---: |
| strict meaningful exact bytes (credited) | - | - | **+10,688** |
| strict padded exact bytes | 1,397,165 | 1,408,045 | +10,880 |
| exact functions (board) | 7,279 | 7,308 | +29 |
| all-function-exact objects (board) | 309 | 312 | +3 (devices, rasterizer_xbox, rasterizer_xbox_widgets) |
| whole-object admission candidates | 8 | 10 | +2 (devices, rasterizer_xbox_widgets) |
| accepted ledger | 7,315 / 1,347,469 | 7,344 / 1,358,157 | +29 / +10,688 |
| active parks | 268 | 255 | -13 reopened as strict exact |
| inherited regressions | - | - | **0** |
| coherent fuzzy bytes | - | - | 0 credited (kept in scratch only) |

## Commits

| commit | content | functions | meaningful |
| --- | --- | ---: | ---: |
| `51c94f13e` | per-function donor harvest (history + worktree bodies) | 11 | 3,884 |
| `e30b1f358` | wave 2 (players, game_engine, aim_assist) | 9 | 2,978 |
| `d6df15102` | wave 1 (actor_moving, decals, ui_widget, projectiles) | 4 | 1,643 |
| `42f573014` | owner-unblocked reserved units (networking) | 6 | 2,183 |

## Checkpoint verification

Full `ninja` pass; stable diff vs 9062: 29 gained, 0 regressions; `tools.parked_functions`: 255 active / 0 stale / 0
invalid; `audit_object_admission`: 10 candidates / 0 contradicted / 7 rejected / 0 revoked; pytest 1,151 passed, 5
skipped, 26 subtests; `git diff --check` clean for every commit; fake-match scan 22 findings (= baseline 22); per-file
audit of all 18 changed files: guard pass (real_math pre-existing), fake leads unchanged, owner census changes only
`$L` labels plus one disclosed select-any `_dot_product3d` COMDAT in devices.obj (laws_w3 A30 class; 70 other objects
already carry it) and the removal of a non-January `_cosine` COMDAT from players.obj; `_point_from_line3d` emitters 7
(= baseline 7).

## What worked

- **Per-function transplant** (`scratch/tr/transplant.py`): earlier lanes mined history only as whole files and
  recorded it exhausted. Splicing a single historical/worktree body into the current file found 20 zero-loss exact
  bodies; 11 passed review (3,884 B), 6 more came from the owner-unblocked reserved units (2,183 B).
- **Waves 1-2** (16 disjoint-TU workers, 108,768 target bytes, 6.9M subagent tokens): 13 functions / 4,621 B
  (4.2% of assigned bytes). Most residuals are register/x87 ties; workers stopped each after the shape budget.

## Rejected (never landed)

| function | reason |
| --- | --- |
| `_render_ui_widgets`, `_render_ui_widgets_postgame` | exact only with duplicated identical `should_render = TRUE` arms |
| `_eat_my_shorts` | exact only with an identical-outcome `if (first < second) return FALSE;` branch |
| `_effect_allowed_by_environment` | donor returns an uninitialized result if the fatal assert returns (UB) |
| `_saved_game_files_enumerate_available_to_local_player_index` | donor reads an uninitialized count when the mutex fails (UB) |
| `_rasterizer_lights_reset_for_new_map` | donor memset overruns the array by one record (UB) |
| `_build_sprite_transform_origin_and_direction` | donor writes a 12-byte vector over a 4-byte parameter slot (UB) |
| `_ai_communication_event`, `_actor_emotion_update` | exact forms need an uninitialized read / out-of-bounds read (UB) |
| `_projectile_collision_test_line`, `_actor_move_test_avoidance_vector`, `_actor_move_update`, `_actor_perception_refresh_danger_zone` | need an emitted `_point_from_line3d` COMDAT |
| `_ai_communication_find_actor_to_reply_to_player` | needs `short` impulse prototypes in units.h/units.c, which costs 2 exact units.obj functions |
| `_player_examine_nearby_device`, `_convex_hull2d_perimeter`, `_player_handle_powerup_minor` (donor), `_player_profile_1wide_list_update` (donor) | volatile / hand-expanded helper / changed assert strings |
| `_transport_network_available`, `_transport_nonce_is_equal` donor forms | boolean truncation cast and `byte *` parameters; natural minimum forms landed instead |

## Queued, not landed

- Phase 5 header packet: `_render_debug_profile_stall_tick` (271 B strict) with `_render_debug_profile` (2,272 B,
  csplit-relocation-alias, zero credit) needs profile.h / cseries_windows.h / effects.h ownership moves, a 6-parameter
  `profile_frame_get_messages`, and a ruling on the colour-inferred stall enum (`scratch/workers/w1_interface/`).
- `_player_aim_projectile` (832 B): complete body at size/reloc parity needs an authentic `pin_normal_to_cone3d`
  owner prototype.
- Waves 3-4 staged (`scratch/tr/w3-groups.json`, `w4-groups.json`, 73,904 B); reserved units and formerly
  do-not-spend functions are now schedulable per the owner (rules unchanged). Not launched: awaiting the owner's go.
