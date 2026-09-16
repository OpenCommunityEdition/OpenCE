# Throughput-recovery campaign: +50K checkpoint (2026-09-16)

Supersedes the running totals of the +40K record (`opus5_throughput_recovery_checkpoint_40k_20260916.md`, whose
rejected list and disclosures still apply). Frozen baseline `9062b1e96`; branch `opus/throughput-recovery-150k-20260916`;
nothing pushed. Machine-readable manifest: `opus5_throughput_recovery_checkpoint_50k_20260916.json`.

## Measured result (vs the frozen 9062b1e96 stable snapshot)

| measure | baseline | now | delta |
| --- | ---: | ---: | ---: |
| strict meaningful exact bytes (credited) | - | - | **+56,818** |
| strict padded exact bytes (board) | 1,397,165 | 1,454,493 | +57,328 |
| exact functions (board) | 7,279 | 7,347 | +68 |
| all-function-exact objects (board) | 309 | 315 | +6 |
| whole-object admission candidates | 8 | 12 | +4 (hud_unit all-exact but object-rejected: pre-recorded `_fast_ftol` select-any owner) |
| accepted ledger | 7,315 / 1,347,469 | 7,383 / 1,404,287 | +68 / +56,818 |
| active parks | 268 | 226 | -42 |
| inherited regressions | - | - | **0** |
| coherent fuzzy bytes | - | - | 0 credited |

## Waves 6-8 (24 never-attempted TUs, 111,840 target bytes)

| commit | content | functions | padded |
| --- | --- | ---: | ---: |
| `0e65f0f8b` | wave 8: _observer_update_command, _main_load_last_solo_map, _main_get_window_count | 3 | 1,296 |
| `2ba3ea7ce` | wave 6: _actor_start_burst, _actor_combat_reaim_grenade, _actor_combat_find_nearby_target, _bitmap_2d_sharpen | 4 | 3,920 |
| `223ab6f08` | header packet: bitmap_utilities.h owns color structs + six public prototypes; _bitmap_2d_smooth | 1 | 1,136 |
| `cf097d7f7` | wave 7: _hud_render_unit_interface, _flag_render_proper, _path_state_begin, _blip_begin | 4 | 6,320 |

Yield: 12 functions / 12,672 padded from 111,840 assigned (11.3%), 8.4M subagent tokens.

## Checkpoint verification

Full `ninja` after every packet; stable diff vs 9062: 68 gained, 0 regressions. `tools.parked_functions`: 226 active,
0 stale, 0 invalid. `audit_object_admission`: 12 candidates / 0 contradicted / 8 rejected (new: hud_unit, the recorded
`_fast_ftol` select-any rejection) / 0 revoked. pytest 1,151 passed, 5 skipped, 26 subtests. Fake-match scan 22 (=
baseline, no new rule/path). `_point_from_line3d` emitters 7 (= baseline). Per-file audit of all 36 changed C files:
guard pass (real_math pre-existing), fake leads unchanged; new owner deltas this round: `_negate_vector3d` select-any
COMDAT in actor_combat (A30 class, disclosed). Header packet measured board-wide; untouched fuzzy parks re-measured:
`_bitmap_3d_smooth` (1792/28, objdiff 78.03 -> 78.04) and `_render_motion_sensor` (now 784 vs January 768, objdiff
92.60 -> 94.25; caller of the `_blip_begin` signature fix).

## Disclosed judgement calls (landed)

- `_blip_begin`: January's caller pushes a zero second argument the callee never reads; the parameter is added with a
  provisional name and comment (n3a had held this body; the owner lifted scheduling holds on 2026-09-16).
- `_main_get_window_count`: a named single-window predicate local.
- `_actor_start_burst`: `negate_vector3d` replaces hand-written negation and emits the A30-class COMDAT.

## Rejected this round (never landed)

| function | reason |
| --- | --- |
| `_render_debug_trigger_volumes` | exact only with the same nine stores duplicated in both arms (R2) |
| `_leaf_face_get_vertex3d`, `_leaf_map_get_leaf_bounds`, `_intersect_planes3d` | need the owner-declined P7 `projection_from_vector3d` header body (evidence: no exact row changes board-wide) |
| `_closest_point_to_attractor` | point_from_line3d COMDAT |
| `_breakable_surface_effect` | renamed local copies of point_from_line3d / projection_from_vector3d |
| `_observer_update_positions` | same-value carrier copy of remaining_time |
| `_main_crash` | January writes through a null pointer (UB) |
| `_effect_generate_part` | mismatched csprintf varargs (UB) |

## Near misses left as coherent fuzzy (zero credit, scratch only)

`_ai_debug_render_actor` (24,976 B: size and 1,905 relocations exact, 382 -> 93 differing regions),
`__rasterizer_model_draw` (size/relocs/frame exact), `_hud_messaging_update` (January sha; csplit relocation alias),
`_effect_generate_particles`, `_bitmap_3d_smooth`, `_main_update_time`.
