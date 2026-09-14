# `bipeds.obj` Opus5 150K house-clean lane, wave w2 (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/units/bipeds.c`; target `build/split/source/units/bipeds.obj`.
- Lane worktree `opus5-150k-house-clean-20260914`, branch `opus/150k-house-clean-20260914`, HEAD `ea9175577`.
- Compiler: repository XDK 3911 `CL.Exe` (VC7 13.00.9254), build flags unchanged, no `--cflag`.
- **The real `bipeds.c` is unchanged** (`git diff --stat -- source/units/bipeds.c` is empty). No header, config, park entry or
  tool was edited. All exact results below are scratch-only park-reopen proposals.
- Evidence, in authority order: January target COFF (bytes, relocations, frames); Ghidra/atlas (hypotheses); HCEA
  `units/biped_fix_position.c`, `biped_update_moving.c` (semantics/topology only); same-compiler donor: the players w1
  ledger's `players_reconnect_to_structure_bsp` long cluster phi; standalone VC7 harness probes (`scratch/workers/bipeds/w2/cc/`).
- Ledgers read: `bipeds_obj_opus5_100k_20260914.md`, `bipeds_obj_opus5_100k_waveD_20260914.md`,
  `bipeds_obj_batch7_exact_and_fuzzy_20260912.md`, `bipeds_obj_adjust_placement_fuzzy_20260904.md`, the 100K handoff, the
  players w1 ledger; donor notes `opus5-100k-consolidated-20260914/scratch/workersD/bipeds.md`.
- Worker notes: `scratch/workers/bipeds.md`. Candidates, probes and objects: `scratch/workers/bipeds/w2/`.

## 2. Gates

| Gate (real file, `--forbid-emitted-symbol _point_from_line3d`) | Result |
|---|---|
| Baseline | `== exact 40  residual 11  unwritten 0`, guard passed |
| Final (real file unchanged) | identical listing, row by row |
| Park drift (real file) | 9 parks OK, 0 drift |
| Owner census vs `build/base` and target | no new / no missing owners |
| `tools/fake_match_scan.py bipeds.c` | 0 leads |

Newly strict exact in the real file: **none**.

## 3. Park-reopen proposals

| Park | Padded / relocs | Target nSHA16 | Candidate | Needs |
|---|---:|---|---|---|
| `_biped_fix_position` | 1232 / 47 | `8932705148ab7aa1` | `scratch/workers/bipeds/w2/park_reopen_fix_position.c` | nothing |
| `_biped_update` (+ fix_position) | 1120 / 61 | `fe3b81e079c33a13` | `scratch/workers/bipeds/w2/park_reopen_update_and_fix_position_needs_headers.c` | header packet vfd (§5) |

### 3.1 `biped_fix_position` — loop condition, long cluster phi, search-state scope

January facts:
1. `movsx eax,word [eax+8]; add esp,0Ch; cmp eax,-1` after the leaf element fetch is a long phi:
   `cluster_index = scenario_leaf_index_from_point(p) == NONE ? NONE : TAG_BLOCK_GET_ELEMENT(...leaf(p) & LONG_MAX...)->cluster_index`
   (the idiom that closed players `reconnect_to_structure_bsp`; HCEA calls `scenario_cluster_index_from_point`).
2. Both `_collision_test_line` calls stay out of line. The merge form removes the old `if (leaf)` level, so by the inline
   frequency law one more enclosing condition must exist: the search loop is
   `for (fudge_vector_index = 0; !fixed && fudge_vector_index < maximum_fudge_vector_count; ++fudge_vector_index)`.
   The `do { if (i >= n) break; ... } while (!fixed)` and `while` spellings jump-thread to the same loop bytes but inline the first call.
3. Frame: `unused_pill_base` shares `pill_vector`'s slot `[ebp-48h]` and the count sits at `[ebp-4Ch]`: the search locals'
   scope opens after the `new_position` if/else (VC7 coalesces the block-scoped pill base only with a local whose scope starts after it).
4. Entry `xor bl,bl` after the bounding-sphere block is the `line_of_sight_only = FALSE;` statement at that point.

The chosen body also replaces the park's `goto collision_user_end` with an if-block, groups `left`/`pill_vector`/index in a
search block and declares the per-iteration collision results in the loop body (as HCEA does).

Shapes (whole-TU gates): condition-embedded ?: 1264; ?: + nested LOS if 1264; TU-local `__inline` cluster helper probe 1264;
[learning probes with an inadmissible `COLLISIONS_EXTERNAL_COLLISION_TEST_LINE` define: 1232 slots-only; pill_vector block -> EXACT];
for-loop without the define -> EXACT; while-loop 1264; goto removed with declaration-initialised flag 1216; flag statement
restored -> EXACT (three scoping layouts); final k1 -> **EXACT**.

Checks: 40E -> 41E with only this row changing; guard passed; only this park drifts (to the target hash); census clean; fake scan 0.
Criterion ("authoritative January source/local records or a natural same-compiler donor explaining the remaining schedule"):
the premise is refuted — the residual was loop spelling, a value merge with a same-compiler donor, and local scope, not a schedule tie.

### 3.2 `biped_update` — count-neutral header packet

The wave D body (C4013 prototype widths, `0.1f*0.1f`, moving-first case order, short melee quarter) is still exact at HEAD
with shadow headers (42/51 together with fix_position). New in this wave is the blast radius:

| Packet | TUs compiled | Changed functions | Exact losses |
|---|---:|---:|---|
| wave D proposal (units.h +4, weapons.h +2) | 76 | 5 | `units::_unit_preprocess_node_orientations` |
| **vfd** = wave D + remove the verbatim duplicate `ai_reconnect_to_structure_bsp` from `ai/ai_runtime.h` | 77 | 4 | **none** |

Bisect: the units.h part alone is neutral; `weapon_stop_reload` in weapons.h (already re-declared locally in the protected
units.c) is what flips units. vfd's changed rows are residual -> residual: `_encounter_update_respawn` (parked; base
`647cc2e7` -> `32d0a247`, needs a park re-baseline), `_render_weapon_hud`, `_physics_update_old`, and the HEAD `_biped_update` park body.

## 4. Not landed / blocked

| Function | Classification | Evidence (new this wave in bold) | Reopen criterion |
|---|---|---|---|
| `_biped_update_moving` 4080 (real 4000, frame 0x68c vs 0x67c) | owner-level argument site + ~75 scheduling sites | **January stages the `fast_vector_intersects_sphere` radius through the x87 (`push ecx` hoisted, `fld [edi+0Ch]; fstp [esp]`) exactly as at the ai, players and collisions callers; real_math (radius parameter) and collision_bsp push integers. A VC7 harness shows only a parenthesised whole argument through a pointer stages (`(d->radius)` yes, `(r)` parameter no, a local copy no), so an argument-parenthesising wrapper macro would explain every January site, but no such macro is authenticated.** Donor um1 re-measured at HEAD: 4096/96, frame 0x67c, 76 real hunks. | authenticated rvalue spelling for the sphere radius (shared with players/collisions/ai), then um1 + store-in-arm player speed blends |
| `_biped_update_physics` 5376 (real 5248/112) | header + policy | debug block needs the collision_debug.h externs (wave A proposal). **January's stick test reads `collisions[best_collision_index]` with the index still -1 (`[ebp-380h]`, inside the stand-up test's uninitialised `collision` local at `[ebp-3A4h]`): an out-of-bounds read, inadmissible under the original-bug policy.** | owner ruling on the `collisions[-1]` read and the collision_debug.h packet |
| `_biped_adjust_placement` 96 | register tie | shapes: no biped local (identical), field re-reads (worse) | donor for the freed-register choice |
| `_biped_get_autoaim_pill` 336 | tail-duplication store/load order | spherical-arm tail only; single exit already | donor |
| `_biped_get_sight_position` 416 | x87 local creation (fld/fst vs mov copy) | inline helper parameters were already ruled out (wave D gs1) | donor |
| `_biped_accelerate`, `_biped_find_nearby_support_surface` | x87 operand age | no new evidence | donor |
| `_biped_update_turning` | policy (`(real_vector2d *)` storage reuse) | unchanged | owner ruling |
| `_biped_check_discard` | policy (mismatched error() varargs) | unchanged | owner ruling |

## 5. Orchestrator proposals

1. **park-reopen `_biped_fix_position`**. Candidate `scratch/workers/bipeds/w2/park_reopen_fix_position.c`; no header needs.
2. **Header packet vfd plus park-reopen `_biped_update`**. Candidate `scratch/workers/bipeds/w2/park_reopen_update_and_fix_position_needs_headers.c`.
   - `source/units/units.h`, before `void unit_dialogue_update(`, add:
     - `struct unit_animation_update_data;`
     - `void unit_animation_start_action(long unit_index, short action);`
     - `void unit_cause_player_melee_damage(long unit_index);`
     - `short unit_update_animation(long unit_index, struct unit_animation_update_data *data);`
     - Use the house multi-line parameter form.
   - `source/items/weapons.h`, after `weapon_get_first_person_animation_time`, add:
     - `void weapon_stop_reload(long weapon_index);`
     - `boolean weapon_prevents_melee_attack(long weapon_index);`
   - `source/ai/ai_runtime.h`: delete the duplicate `void ai_reconnect_to_structure_bsp(void);`. The owner declaration stays in `ai/ai.h`, which the only caller, `scenario.c`, includes.
   - Consumers: units.h 66 direct, weapons.h 19 direct, ai_runtime.h 8 direct; 77 transitive TUs measured.
   - Shadow tree: `scratch/workers/bipeds/w2/vs_vfd/`. Blast results: `scratch/workers/bipeds/w2/blast/result_vfd.txt`.
   - Re-baseline the `_encounter_update_respawn` park in the same commit.
3. **Evidence only, not a proposal to land:** the paren-staging harness result for `fast_vector_intersects_sphere`, recorded in §4 for the owner decision shared with players.

## 6. Reusable VC7 facts measured this wave

- **Loop-condition frequency:** in a `for (i = 0; !done && i < n; ++i)` loop, the loop condition counts as an enclosing condition for the inline frequency law. The equivalent do/break and while spellings emit identical loop bytes but one fewer level, so a header `__inline` call deeper in the body inlines.
- **Scope-start slot coalescing:** a block-scoped aggregate shares a frame slot only with a same-size local whose scope opens after that block. Moving the partner's declaration into a block that starts later changed which slot `unused_pill_base` joined, and with it the frame rank order.
- **Paren argument staging (refined):** a parenthesised float argument is staged through the x87 when it is a load through a pointer or array. A parenthesised parameter or a local copy is still integer-pushed. The stack slot (`push ecx`) is reserved early and filled with `fstp [esp]` just before the call.
