# Throughput-recovery campaign: +60K checkpoint (2026-09-16)

Supersedes the running totals of the +50K record; its rejections and disclosures still apply. Frozen baseline
`9062b1e96`; branch `opus/throughput-recovery-150k-20260916`; nothing pushed. Manifest:
`opus5_throughput_recovery_checkpoint_60k_20260916.json`.

## Measured result (vs the frozen 9062b1e96 stable snapshot)

| measure | baseline | now | delta |
| --- | ---: | ---: | ---: |
| strict meaningful exact bytes (credited) | - | - | **+63,500** |
| strict padded exact bytes (board) | 1,397,165 | 1,461,229 | +64,064 |
| exact functions (board) | 7,279 | 7,356 | +77 |
| all-function-exact objects (board) | 309 | 317 | +8 (new: tiff_file, input_xbox) |
| whole-object admission candidates | 8 | 14 | +6 |
| accepted ledger | 7,315 / 1,347,469 | 7,392 / 1,410,969 | +77 / +63,500 |
| active park entries (incl. 2 asm-implemented) | 268 | 223 | -45 |
| inherited regressions | - | - | **0** |

## Waves 9-10 (16 never-attempted TUs, 31,056 target bytes)

| commit | content | functions | padded |
| --- | --- | ---: | ---: |
| `84dce510b` | wave 10 + input.h packet: _input_get_device_states, _input_update_keyboard_devices, _structure_test_pill2d (was unwritten), _find_turning_point, _structure_visibility_build_surfaces_traverse_leaf | 5 | 4,176 |
| `eca9b6176` | wave 9: _tiff_import, _tiff_export, _hud_render_nav_points, _distance3d | 4 | 2,864 |

Yield: 9 functions / 7,040 padded from 31,056 assigned (22.7%). Header packet `input.h` (struct key_stroke
`ascii_code` is `char`; paired `edit_text.c` NONE test without the `(byte)` cast) measured over all 24 includers:
0 regressions. (Commit `84dce510b`'s message states parks 226 -> 223; the measured active entries after it were 224.)

## Checkpoint verification

Full `ninja` after every packet; stable diff vs 9062: 77 gained, 0 regressions. `tools.parked_functions`: 223 active,
0 stale, 0 invalid. `audit_object_admission`: 14 candidates / 0 contradicted / 8 rejected / 0 revoked. pytest 1,151
passed, 5 skipped, 26 subtests. Fake-match scan 22 (= baseline, no new rule/path). `_point_from_line3d` emitters 7
(= baseline). Per-file audit of all 44 changed C files: guard pass (real_math pre-existing), fake leads unchanged; new
owner deltas this round: select-any real_math inline copies `_point_from_line2d`, `_set_real_vector2d` (path_smoothing,
path_structure_bsp) and `_distance_squared2d`, `_normalize2d`, `_scale_vector2d` (path_structure_bsp) plus January's own
`__real@3f1a36e2e0000000` literal - the A30 select-any class.

## Rejected this round (never landed)

| function | reason |
| --- | --- |
| `_first_person_weapon_update` | exact only with the NULL guards removed (NULL-deref UB hold) |
| `_item_update` | hand-expanded point_from_line3d nudge to evade the COMDAT hold |

## Follow-up

`_path_test_pill_endpoint_near_wall_ok_bit` is a file-local enum in both path_smoothing.c and path_structure_bsp.c;
its owner is path_structure_bsp.h (header packet with a board-wide measurement).
