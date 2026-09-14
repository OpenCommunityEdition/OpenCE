# `bipeds.obj` opus5 100K consolidated lane — wave D park re-investigation (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/units/bipeds.c`, target `build/split/source/units/bipeds.obj`.
- Compiler: VC7 13.00.9254 (XDK 3911), repository flags from `build.ninja` (no per-TU override).
- Lane `opus/100k-consolidated-20260914` at `548451f6d`, worker `waveD:bipeds` (claimed in `scratch/WORK_CLAIMS.json`).
- Evidence: January target COFF (disassembly, relocations, literals); `CL /Zs /W3` implicit-declaration scan;
  HCEA `src/blam/units/biped_{adjust_placement,check_discard,find_nearby_support_surface,fix_position,get_autoaim_pill,
  accelerate,update}.c` (later build, semantics only); Stian build-2276 `units/bipeds.c` (check_discard lift);
  wave A worker notes `scratch/workers/bipeds.md` and ledger `bipeds_obj_opus5_100k_20260914.md`.
- Duplicate check: ledgers `bipeds_obj_batch7_exact_and_fuzzy_20260912.md`, `bipeds_obj_opus5_100k_20260914.md`,
  `bipeds_obj_adjust_placement_fuzzy_20260904.md`; all nine `config/parked.json` bipeds entries;
  `git log --all -- source/units/bipeds.c` (22 refs, newest `d154664b2` = lane body). Worker notes:
  `scratch/workersD/bipeds.md` (per-function blocks).

## 2. Validated baseline

Gate at lane HEAD: `== exact 40 residual 11 unwritten 0` (`_point_from_line3d` guard passed).

| Function | Target size / relocs / sha16 | Base size / relocs / sha16 | Parked |
|---|---|---|---|
| `_biped_accelerate` | 480 / 18 / 654a83fa0ea28d30 | 480 / 18 / 8c1b58e20b0bdd7a | yes |
| `_biped_adjust_placement` | 96 / 2 / 41db59000f9eb782 | 96 / 2 / 5fb478d541413800 | yes |
| `_biped_check_discard` | 176 / 10 / 5f7c9efc91592d80 | 176 / 10 / 096f8aa672e716e4 | yes |
| `_biped_find_nearby_support_surface` | 544 / 23 / d2fe0224f5db1253 | 544 / 23 / a0ef780b38e8c2aa | yes |
| `_biped_fix_position` | 1232 / 47 / 8932705148ab7aa1 | 1232 / 47 / 6298e459f43db497 | yes |
| `_biped_get_autoaim_pill` | 336 / 9 / c232d22b9b72f535 | 336 / 9 / 6b5e592092e374ee | yes |
| `_biped_get_sight_position` | 416 / 19 / 59e27e5e5b6f7a7c | 416 / 19 / 46b593827ea2de53 | yes |
| `_biped_update` | 1120 / 61 / fe3b81e079c33a13 | 1104 / 61 / 95e78b0eb93d7d95 | yes |
| `_biped_update_turning` | 1712 / 49 / 9e0e4d2d24a79a17 | 1760 / 49 / 5148f334e4cd36fb | yes |
| `_biped_update_moving` | 4080 / 97 / 6349add44dc8f867 | 4000 / 97 / c457c69052f0b282 | no |
| `_biped_update_physics` | 5376 / 124 / f202fb20dd4d8fba | 5248 / 112 / fccc72d132c21317 | no (fuzzy) |

## 3. Result

- Real file **unchanged**; final gate `== exact 40 residual 11 unwritten 0`, identical listing to baseline, guard passed,
  park drift 9/9, owner census clean, fake scan 0 leads (`scratch/workersD/bipeds/final.obj`).
- **Two park-reopen proposals, both strict EXACT in scratch** (orchestrator unparks and lands):
  1. `_biped_check_discard` — standalone: `scratch/workersD/bipeds/park_reopen_check_discard.c` (41E/10R whole-TU gate).
  2. `_biped_update` — needs a header packet (units.h + weapons.h prototypes, §8):
     `scratch/workersD/bipeds/park_reopen_update_needs_headers.c`, measured with shadow headers
     (`scratch/workersD/bipeds/shadow/`, `shadowgate.py`) -> only `_biped_update` changes; combined candidate
     `park_reopen_combined_needs_headers.c` -> 42/51, both functions exact, every other row unchanged.

## 4. Accepted controls (scratch candidates)

### `biped_check_discard` — argument position, not evaluation order

January: `call _ai_debug_describe_actor; fld z; add esp,14h; push eax; mov eax,[esi]; sub esp,18h; fstp qword [esp+10h] ...;
push name; push fmt; push 2; call _error`. The describe result is pushed *before* the 24-byte double block, so it sits in
the stack slot above `z`. cdecl argument positions are ABI-fixed, so January's argument list is
`(name, x, y, z, describe)` against the format `"... %s (%s) ... (%.1f %.1f %.1f) ..."` — a **BUG (original)** in a silent
diagnostic. The park premise ("argument evaluation order") is refuted by the byte layout. Candidate moves the
`ai_debug_describe_actor(...)` argument after `position.z` and documents the bug in a comment; strict EXACT.

### `biped_update` — four independent January facts

1. `/W3` C4013: `weapon_prevents_melee_attack`, `unit_animation_start_action`, `weapon_stop_reload`,
   `unit_cause_player_melee_damage`, `unit_update_animation` are undeclared in this TU. January `test al,al` (boolean) and
   `cmp ax,1` (short) prove prototyped return types (definitions: weapons.c `boolean`, units.c `short`).
2. Literal `__real@3c23d70b` is `float(0.1f*0.1f)`, not `0.01f` (`3c23d70a`): the throttle dead zone is a squared 0.1.
3. The jump-table case blocks are emitted moving-first, then idle/turn, then default: case source order.
4. Melee: the frame count is stored directly into `player_melee_ticks` and re-read (loaded into `bl` before the second call)
   as the left operand of the attack tick; one short-typed quarter (`movsx ax,al`) is subtracted from both fields with
   compound assignments (the first attack-tick store survives). Semantics equal HCEA's `t - t/4`, `t - k - t/4`.

## 5. Experiment matrix

| ID | Function | Shape | Size T/C | Result |
|---|---|---|---|---|
| p1 | adjust_placement | inline point_from_line3d (external define removed; probe) | 96/96 | ECX chosen but load before fld |
| p2 | adjust_placement | no flags local | 96/96 | identical to base |
| p3 | adjust_placement | inline helper + height_offset local | 96/96 | identical to base |
| n1-n3 | find_nearby_support_surface | plane3d_distance_to_point / dot_product3d swapped / fully expanded | 544/544 | z,y,x order; worse |
| h1,h3 | find_nearby_support_surface | per-field plane copy | 544/544 | frame 0x1050, worse (aggregate copy proven) |
| cd1 | check_discard | describe argument last | 176/176 | **EXACT** |
| ap1 | get_autoaim_pill | head_node_index local | 336/336 | worse (January rereads +0x4e6) |
| gs1 | get_sight_position | inline point_from_line3d x2 (probe) | 416/416 | identical to base |
| gs2 | get_sight_position | no distance locals | 416/400 | worse (aliasing) |
| gs3 | get_sight_position | both locals initialised together | 416/416 | eager copy, worse |
| gs4 | get_sight_position | one reused distance local | 416/416 | identical to base |
| ac1 | accelerate | HCEA if/else halve/stop-limp | 480/464 | worse |
| ac2 | accelerate | scale_vector3d halving | 480/480 | identical |
| ac3 | accelerate | `velocity.i += acceleration->i` | 480/480 | add operands flip; cross-product flips other way |
| fx1/fx2 | fix_position | `?:` cluster merge (long/short) | 1232/1264 | movsx fixed, collision_test_line inlines |
| fx3/fx5/fx7 | fix_position | short local / leaf pointer / function-scope long | 1232/1232 | identical to base |
| fx6 | fix_position | `cluster_index = NONE; if (leaf) cluster_index = ...` | 1232/1264 | same as fx1 |
| u0 | update | + prototypes | 1120/1104 | return widths fixed |
| u1 | update | + `0.1f*0.1f` | 1120/1104 | literal fixed |
| u6 | update | + moving-first case order | 1120/1104 | switch fixed |
| u2-u5 | update | compound / short-local variants with total_time locals | 1120/1104 | quarter hoisted, worse |
| u7 | update | attack-then-ticks compound, no locals | 1120/1104 | spill |
| u8 | update | ticks-first compound, no locals | 1120/1120 | only movsx site |
| u10/u11 | update | char / long quarter local | 1120/1120 | one site each |
| u9/u12 | update | short quarter local | 1120/1120 | **EXACT** |
| um0/um1 | update_moving | wave A n2 body; n2 + January fabs guard | 4080/4096 | 80 / 76 real diff blocks |

## 6. Do-not-repeat list

- adjust_placement: flags-local removal, inline point_from_line3d (either local form).
- find_nearby_support_surface: helper vs expanded vs swapped dot product; per-field plane copies.
- get_sight_position: removing locals, joint initialisation, reused local, inline point_from_line3d.
- accelerate: if/else halving, scale_vector3d halving, direct `+=` velocity update.
- fix_position: short/long/leaf-pointer/function-scope cluster forms; every value-merge form lowers the collision chain depth.
- update_moving: wave A `update_moving_best_n2.c` calls an undeclared `realcmp` (implicit double push) — do not start from it.
- update_turning: `(real_vector2d *)` casts remain prohibited; typed copies are the 48-byte gap.

## 7. Residual classification

| Function | Class | Fact vs inference |
|---|---|---|
| adjust_placement | register tie (ECX vs EAX) | measured |
| find_nearby_support_surface | x87 commutative order, offset-0 field | measured; mechanism unresolved |
| get_autoaim_pill | register tie in duplicated return block | measured |
| get_sight_position | x87 local definition (mov/mov/fld vs fld/fst) | measured |
| accelerate | x87 operand age inside inline cross_product3d | measured (ac3 proves age-driven) |
| fix_position | frame rank tie + widened compare needing a condition level | measured; source form unresolved |
| update_turning | representation-cast policy | policy |
| update_moving | ~20 scheduling/operand-order sites | measured |
| update_physics | missing debug block (header pending) + ties | measured in wave A |

## 7a. Reopen criteria

- adjust_placement / autoaim_pill / sight_position: a same-compiler donor for the specific register or spill choice.
- find_nearby_support_surface / accelerate: a donor explaining offset-0 operand ages.
- fix_position: evidence of an extra genuine condition level around the collision chain.
- update_turning: owner ruling admitting `(real_vector2d *)&vector3d` (30 landed precedents).
- update_moving: start from `scratch/workersD/bipeds/um1.c`.
- update_physics: collision_debug.h extern packet (already proposed in wave A).

## 8. Orchestrator proposals

1. **park-reopen `_biped_check_discard`** — candidate `scratch/workersD/bipeds/park_reopen_check_discard.c`; no header need.
2. **header packet + park-reopen `_biped_update`**:
   - `source/units/units.h` (before `unit_dialogue_update`): `struct unit_animation_update_data;`,
     `void unit_animation_start_action(long unit_index, short action);`, `void unit_cause_player_melee_damage(long unit_index);`,
     `short unit_update_animation(long unit_index, struct unit_animation_update_data *data);`
   - `source/items/weapons.h` (after `weapon_get_first_person_animation_time`): `void weapon_stop_reload(long weapon_index);`,
     `boolean weapon_prevents_melee_attack(long weapon_index);`
   - Consumers: units.h 66 direct (listed in `scratch/workersD/bipeds.md`), weapons.h 19 direct. units.c and vehicles.c
     already declare some of these locally. Declaration-count law: gate the full board before landing.

## 9. Disposition

No new strict-exact credit in this wave (real file unchanged). Two parks are proven closeable pending orchestrator action;
the remaining seven parks and two non-parked residuals stay rigorously classified above.
