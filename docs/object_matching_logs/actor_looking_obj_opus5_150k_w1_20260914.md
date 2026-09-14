# `actor_looking.obj` — Opus5 150K house-clean lane, wave w1 (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/ai/actor_looking.c`; January target `build/split/source/ai/actor_looking.obj`
  (Xbox debug build, VC7 13.00.9254, repository `build.ninja` flags through `tools/campaign/gate.py`).
- Lane base `12f7375d4` (completed Opus 100K donor lane). Only this `.c` file was modified; no header or config edits.
- Read first: the worker brief, house rules, methodology, throughput lessons, constants, assertions, the 100K
  handoff, and every actor_looking ledger (`_opus5_100k`, `_opus5_100k_waveC`, `_direction_decoder_natural_reconstruction`,
  `_private_leaf_reconciliation`, `_validity_owner_boundary`, `_jonas_two_body`, `_jonas_secondary_look_research`) plus the
  100K lane worker notes (`scratch/workers/actor_looking.md` and `scratch/workersC/actor_looking.md` in that worktree).
  `git log --all -- source/ai/actor_looking.c`: newest body `d6cbc58ea`; `branch_sweep.py`: no better blob.
- Worker notes: `scratch/workers/actor_looking.md`; scratch candidates and lab TUs in `scratch/workers/actor_looking/`.

## 2. Baseline and result

| | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| lane base | 10 | 6 | 0 |
| final real file | **11** | 5 | 0 |

Final gate: the `_point_from_line3d` guard passed. Every other row is identical to the baseline (row-by-row diff).
Park drift is OK (`_actor_look_decode_direction` 1616/99, unchanged). The owner census passes (no candidate-only
owner). `tools/fake_match_scan.py` reports 0 leads (0 at HEAD).

Newly strict EXACT: `_actor_look_find_random_vector` — 608 padded / 595 meaningful / 35 relocations,
normalized sha `424bed166d4ac6bb…` == target.

## 3. Accepted control (landed)

`actor_look_find_random_vector` — single-exit law.

- January's returns are constants. Loop exhaustion falls through to `xor al,al; ret`, and the success block ends
  with `mov al,1; ret`.
- `unobstructed` exists only in `bl` (neg/sbb/inc of the `collision_test_vector` result). The no-avoid path jumps
  straight into the success block.
- The retry loop has no top test (a counted `for` whose first test is folded).

The retained body therefore uses:

- a `result` flag returned once;
- a counted `for (attempt_count = 0; attempt_count < 10; attempt_count++)`;
- a per-attempt `boolean unobstructed = TRUE;` (HCEA `actor_look_find_random_vector.c` scopes the same flag inside
  the retry loop);
- the assert/normalize/copy success block ending in `result = TRUE; break;`.

The first compile was strict EXACT. The idle major/minor callers keep their exact register ABI (eax = base_vector).
Semantics are unchanged from the previous body: up to ten attempts, and the result vector is written only on success.

## 4. Experiment matrix (whole-TU gates; siblings and park re-checked)

| function | shape | result |
| --- | --- | --- |
| find_random_vector | result flag + counted for + scoped `unobstructed` + break (s2) | **EXACT, landed** |
| valid_aim_vector | a1 `real_vector2d v = *(real_vector2d const *)attempted` | January load registers; j*j still first [sha] |
| valid_aim_vector | a2 statement struct copy; a3 declaration order; b2 `set_real_vector2d` | identical / float loads [sha] |
| valid_look_vector | l1 struct-copy initialisers for both 2D locals | frame 0x14 -> January 0x10; only product order left [sha] |
| valid_look_vector | l2 = l1 + `boolean side` | January `movzx eax,al` reproduced; 2 order blocks left [sha] (best scratch) |
| valid_look_vector | l3 dot argument swap; l4 explicit dot; l5 unnamed index | identical; identical; loses movzx |
| idle_find_prop | p_fp1 LEARNING PROBE `(int)actor_index` (ai_debug.h field is `int`) | identical (type is not the cause) |
| idle_find_prop | s1 single-exit `found` flag | identical |
| update | v2 = wave-C u1+u2+u5 on the current file | 4704 (not exact) |
| update | v3 = v2 + stationary 2D struct copies via `(real_vector2d *)&actor->control…` | pointer reads like January; 4688; reverted |
| update | v4 = v2 + chained `change_facing = face_exactly = …` | 4688; reverted |
| decode (park) | LEARNING PROBE dd1: diagnostic switch removed | both validity checks still out of line (frequency, not budget) |
| lab TUs t1–t5 | 25 inline normalize2d spellings, TU-local normalize bodies, /Op, +1..+9 declarations, typedef removal | last-defined component always multiplied first; all inert |

## 5. Findings for future waves

- The header math inlines are correct. January `action_charge.obj`'s `_normalize2d`, `_magnitude2d`,
  `_magnitude_squared2d` and `_square_root` are strict EXACT from our `real_math.h`.
- VC7 13.00.9254 multiplies the most recently defined component first inside an inlined `normalize2d`. This holds
  for integer copies and x87-computed components alike; x87 values are forwarded with `fst`.
- January's three i-first inline sites (both actor_looking vector validators and `ai_communication_event`) show no
  such forwarding: January stores with `fstp` and reloads i first. No source spelling, declaration count, flag or
  inline-body variant reproduces this.
- `actor_look_valid_look_vector` had a structural frame gap (0x14 vs 0x10) that earlier ledgers missed. The
  struct-copy + `boolean side` form (`scratch/workers/actor_looking/p_look_l2.c`) closes the gap. Only the
  product-order tie remains.

## 6. Do-not-repeat

- **aim/look:** the lab matrix above — result placement, early return, nested if, `result = expr`, dot helper vs
  explicit expression, named magnitude, `set_real_vector2d`/`vector_from_points2d` nesting, TU-local normalize
  bodies, /Op, declaration count, typedef removal, declaration order and block scope.
- **find_prop:** the `(int)` type probe and the single-exit flag, in addition to wave A's five forms.
- **update:** v3/v4 above plus wave C's u3/u4. Do not sweep registers.
- **decode:** the result-gated spellings (wave C d1–d6) and the frequency/budget probe.

## 7. Residual classification and reopen criteria

| function | classification | reopen |
| --- | --- | --- |
| `_actor_look_update` 4688 vs 4720 | register/slot allocation tie after structural fixes | a source reason for `secondary_aim_valid` or the facing CSE to stay in memory; start from `v2.c` |
| `_actor_look_idle_find_prop` | register scheduling (one load/compare) | new January topology evidence |
| `_actor_look_valid_aim_vector` | x87 store-forwarding / operand-order tie | a January-side reason the copies are not live at the inline (intervening block or statement), or a measured VC7 control |
| `_actor_look_valid_look_vector` | same tie, after the frame fix in `p_look_l2.c` | same; land the l2 shape only together with an exact order fix |
| `_actor_look_decode_direction` (parked) | inline-frequency boundary (confirmed not budget) | a natural result-gated spelling weighted at function-top frequency |

## 8. Proposals

None: no header or config needs, no park reopen, no pending renames.
