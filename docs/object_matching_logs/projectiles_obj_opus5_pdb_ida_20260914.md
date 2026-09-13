# `projectiles.obj` — Opus5 PDB/IDA lane, private collision graph (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/items/projectiles.c`, target `build/split/source/items/projectiles.obj`
  (SHA-256 `e4d5697dcce32e90b71e240b27d55363c85eca2bb0fb15e5074daa0c7b609c6c` in this worktree).
- Compiler: Microsoft 32-bit C/C++ 13.00.9254 (XDK 3911), repository `build.ninja` flags, measured with
  `tools/campaign/gate.py` (no ninja/configure).
- Lane base `c3e257e9ab9a5d7e1c9b39862f87ac2abde4359c`, branch `opus/pdb-ida-15k-20260914`. Only
  `source/items/projectiles.c` changed; no header, config, symbols.json or parked.json edit.
- Name/topology evidence:
  - January PDB publics (`scratch/pdb200-current/pdb_symbols.json`): only `_projectile_update` among the targets; no public in
    `.data 0x306b30..0x307140` (so the TU data are statics).
  - January debug-map atlas (`scratch/atlas/projectiles.obj.txt`): `000e8110 _projectile_detonate`, `000e88c0 _projectile_collision`.
  - No atlas record for `e7630`, `e7650`, `e7f10`: identified by caller graph + HCEA (`src/blam/items/projectile_set_action.c`,
    `projectile_effect_new.c`, `projectile_collision_test_line.c`). HCEA enum names used for TU-local enums: `projectile_action.h`,
    `detonation_timer.h`, `projectile_possible_response_flags.h`; `data/seconds_per_tick_1.c` and `effect_marker_names.c` for data names.
  - Ghidra decompilation of the January object used only as a hypothesis generator; every operand checked against disassembly.

## 2. Validated baseline and result

| Row | Baseline | Final real-file gate | Under pending-rename aliases |
|---|---|---|---|
| `_code_000e7630` (projectile_set_action, 32) | UNWRITTEN | UNWRITTEN (named `_projectile_set_action`) | **EXACT** |
| `_code_000e7650` (projectile_effect_new, 96) | UNWRITTEN | UNWRITTEN | reloc-identity only; **EXACT** with data anchor diagnostic |
| `_code_000e7f10` (projectile_collision_test_line, 512) | UNWRITTEN | UNWRITTEN | residual 496/10 (fuzzy) |
| `_code_000e8110` (projectile_detonate, 1040) | UNWRITTEN | UNWRITTEN | **EXACT** (+ `seconds_per_tick=rdata_00279068`) |
| `_code_000e88c0` (projectile_collision, 2928) | UNWRITTEN | UNWRITTEN | residual 2928/100 sha (fuzzy) |
| `_projectile_update` (4064) | UNWRITTEN | residual 4064/179 sha (fuzzy) | same |
| 21 baseline exact rows | EXACT | EXACT (unchanged) | EXACT |
| parked `_projectile_new/_accelerate/_aim_ballistic` | residual | residual, `measurements.base` identical | same |

Gate lines: baseline `== exact 21  residual 3  unwritten 6`; final real file `== exact 21  residual 4  unwritten 5`;
alias gate `== exact 23  residual 7  unwritten 0`; data-anchor diagnostic `== exact 24  residual 6  unwritten 0`.
Strict newly-exact credit in the real-file gate: **0** (all exact rows are pending symbols.json renames).

Data: the TU `.data` (1,548 B: `struct profile_section` "projectile_update" + 5 marker-name pointers) and the non-COMDAT `.rdata`
float `0x3d088889` are byte- and relocation-identical to January.

Residual fingerprints (target | candidate, normalized SHA-256):

| Function | Size T/C | Relocs T/C | Target hash | Candidate hash | First divergence |
|---|---:|---:|---|---|---|
| `_code_000e7f10` | 512/496 | 10/10 | `2e02f384…83511` | `ca8afada…60db8` | `+0x3` frame `sub esp,0x40` vs `0x4c` |
| `_code_000e88c0` | 2928/2928 | 100/100 | `16558424…5998f` | `c6a62ce8…e7c81` | `+0x2e` definition home slot |
| `_projectile_update` | 4064/4064 | 179/179 | `0cfda26e…71355` | `c721dcef…af517` | `+0x35` time_remaining home slot |

Exact under alias: `_code_000e7630` `f1f3a50d…302a8`; `_code_000e7650` `3733234e…1f1f7`; `_code_000e8110` `315992f2…cade8d`.

## 3. Accepted controls (retained in production)

- Private ABI settles from the callers; all five helpers are ordinary `static` functions defined after the public code.
- `static struct profile_section projectile_update_section = {"projectile_update", NONE, TRUE};` then
  `static char const *effect_marker_names[NUMBER_OF_EFFECT_MARKERS]` — order and contents proven by `.data` relocations
  (`_data_00306b30+0` profile name, `+8` active byte read by `profile_enter/exit`, `+0x5F8` table used by effect_new/collision).
- `static real const seconds_per_tick = 1.0f / TICKS_PER_SECOND;` — January's detonate/update contrail dt reads a TU `.rdata`
  object (not a `__real` COMDAT), while guidance uses the literal; HCEA DB names that object `seconds_per_tick`.
- Header switches (no header edits): `REAL_MATH_EXTERNAL_POINT_FROM_LINE3D` (update's flyby calls `_point_from_line3d`
  out of line; the switch avoids emitting a forbidden COMDAT) and `COLLISIONS_EXTERNAL_COLLISION_TEST_LINE` around
  `physics/collisions.h` (collision_test_line's left test is a REL32 `_collision_test_line`).
- `random_vector_in_cone3d` is now the real_math.h inline (the TU remap and explicit definition were removed): January collision's dead
  parameter-homing store before `seed_random_vector_in_cone3d` proves the inline, and its used non-static COMDAT stays exactly
  January's 48-byte `_random_vector_in_cone3d`.
- detonate: `if (count <= 6) {flag + random timers} else {zero timers}` (January then-block); material type read directly from the datum.
- collision: if/else damage_scale PIN, marker arrays in their own block (frame `0xe4`), `-dot_product3d(new_velocity, &plane) + noise`,
  marker store order incident/negative/gravity/normal/reflect, default response `display_assert(NULL, …, 1487, TRUE)`.
- update: separate `detonation_timer_starts` case labels (January `dec/je/dec/je`), `set_real_euler_angles2d(pitch, yaw)` (yaw temp copied
  after pitch store), `& UNSIGNED_SHORT_MAX` wander phase, if/else gravity scale, fraction branch as then-arm, fall-through
  detonate→delete, field paths for translational_velocity, explicit listener vector.
- TU-local enum copies with disclosure comment: effect vectors (objects/damage.c), AI spatial effects (ai/actors.c, ai/ai.c),
  `_collision_surface_breakable_bit` (ai/path.c, path_smoothing.c), periodic functions (math/periodic_functions.c); plus
  projectile action / detonation-timer-starts / potential-response enums (HCEA DB names, no repo header declares them).

## 4. Experiment matrix (bounded; all measured with the full TU)

| ID | Function | Shape | Size | Relocs | Result | Decision |
|---|---|---|---:|---:|---|---|
| S1 | set_action | natural static | 32 | 1 | exact (alias) | kept |
| E1 | effect_new | natural static | 96 | 4 | bytes equal, data identity only | kept |
| D1 | detonate | zero-branch first + short local | 1040 | 37 | branch order + `lea` slot | reverted |
| D2 | detonate | random branch first | 1040 | 37 | `lea ecx,[ebx+0x240]` position | reverted |
| D3 | detonate | ternary material select | 1040 | 37 | inert | reverted |
| D4 | detonate | direct field read, no local | 1040 | 37 | exact (alias incl. rdata) | kept |
| T1 | collision_test_line | result flag, inline point/line helpers | 480 | 14 | extern point_from_line3d relocs | reverted |
| T2 | collision_test_line | block returns + inline point_from_line3d | 528 | 10 | emits `_point_from_line3d` (guard FAIL) | reverted |
| T3 | collision_test_line | HCEA explicit offsets/points | 496 | 10 | CSE of offset*radius | reverted |
| T4 | collision_test_line | nested `if (!ctv)` | 496 | 10 | 108 structural lines | reverted |
| T5 | collision_test_line | nested `normalize3d(cross_product3d(up, vector_from_points3d(...), &offset))` | 496 | 10 | 115 structural lines | kept (fuzzy) |
| C1 | collision | init+PIN damage scale, function-scope markers | 2944 | 100 | frame 0x120 | reverted |
| C2 | collision | if/else PIN + marker block | 2928 | 100 | frame 0xe4 | kept |
| C3 | collision | impact velocity forms (4 spellings) | 2912–2928 | 100 | `-dot(vel,plane)+noise` best | kept |
| C4 | collision | marker store order (2 orders) | 2928 | 100 | incident-first best | kept |
| C5 | collision | header-inline random_vector_in_cone3d | 2928 | 100 | homing store reproduced | kept |
| C6 | collision | speed_squared block scope / compare reversal / `!(>=)` / velocity-noise locals | 2912–2928 | 100 | inert | reverted |
| U1 | update | first full body | 4032 | 181 | — | superseded |
| U2 | update | ten structural fixes (see §3) | 4064 | 179 | 15 structural lines | kept |
| U3 | update | `(word)` cast / range-switch / if / ternary timer starts | — | — | worse | reverted |
| U4 | update | explicit scale_vector3d / target vector | 4064 | 180 | listener vector still out of line | reverted |
| U5 | update | declaration order, loop-scope promotion/demotion, loop-top order | 4064 | 179 | inert or worse | reverted |

## 5. Do-not-repeat

- Any use of the inline `point_from_line3d` in this TU (emits `_point_from_line3d`; lane guard).
- Declaration-order permutations for update/collision frame slots (inert, as the frame-density memory predicts).
- `speed_squared` scoping/compare spellings in collision; velocity-noise local spellings.
- Explicit `scale_vector3d` rewrites in update (do not change the listener inline decision).
- Remapping `random_vector_in_cone3d` back to an explicit TU definition (loses collision's homing store).

## 6. Residual classification

- `_code_000e7f10`: **guard/ownership-blocked + scheduling**. Measured: January's four side points are the inline `point_from_line3d`
  expansion (radius reloaded per call, `-radius` shared); explicit math CSEs across points (−16 B). January also computes the up×path
  product without storing the path vector (not reproduced).
- `_code_000e88c0`: **allocator tie** (measured): `speed_squared` kept on x87 stack with `fst` in January vs homed/reloaded here; block
  placement and a 4-byte slot shift follow.
- `_projectile_update`: **allocator tie** (measured): identical per-slot reference census for time_remaining (15) and new_velocity (48) but
  different slot order; esi/edi exchange between `definition` and the velocity-address CSE; immediate-vs-register store in one inlined
  set_action.
- Owner census (inference about systemic cause): header-inline code COMDATs (`_cross_product3d`, `_distance3d`, `_dot_product3d`,
  `_set_real_euler_angles2d`, `_valid_real_normal3d`, `_valid_real_point3d`, `_valid_real_vector3d_axes2`, `_valid_realcmp`,
  `_vector_from_points3d`) and xtl.h D3D selectany data via `collision_usage.h` also appear in accepted base objects (action_charge, units,
  ai). January references most of those functions as undefined externals from this very object.

## 7. Reopen criteria

- collision_test_line: the `_point_from_line3d` emission guard is relaxed for this unit, or an admitted per-call-site inline mechanism.
- collision/update: new evidence on VC7 x87 home/slot priority (e.g. another January TU with the same idiom exact) — not more spellings.

## 8. Disposition and orchestrator proposals

Still active; no strict credit. After the renames below, re-verify: expected strict EXACT `_projectile_set_action`, `_projectile_detonate`,
`_projectile_effect_new` (+1,168 padded bytes), with collision_test_line/collision/update fuzzy.

```
symbols.json (edit lines in place, "static": true):
  _code_000e7630 (file_offset 947760) -> _projectile_set_action
  _code_000e7650 (file_offset 947792) -> _projectile_effect_new
  _code_000e7f10 (file_offset 950032) -> _projectile_collision_test_line
  _code_000e8110 (file_offset 950544) -> _projectile_detonate
  _code_000e88c0 (file_offset 952512) -> _projectile_collision
  _rdata_00279068 (file_offset 2592872) -> _seconds_per_tick
  _data_00306b30 (file_offset 3173168) -> _projectile_update_section
  add _effect_marker_names at file_offset 3174696 (0x306b30 + 0x5f8)
```

## Lead admission notes (2026-09-14)

- The rename proposal was applied in the separate configuration prerequisite
  `2364150f9` (five static function names and the three static data names,
  following the particles.obj precedent). After the csplit regeneration the real
  file gates **24 exact / 6 residual / 0 unwritten** with no aliases.
  `_projectile_set_action` (32), `_projectile_effect_new` (96) and
  `_projectile_detonate` (1,040) are strict exact.
- The newly added unreachable default arm in `projectile_collision` was changed
  from a raw `display_assert(NULL, ...)` / `system_exit(NONE)` pair to the
  documented `match_vassert(file, 1487, FALSE, NULL)` form, per the assertion
  house rule. The emitted section is byte-identical (2928/100, sha16
  `c6a62ce8b166ed4c`). The inherited export-function default is unchanged.
- The full build and rename-stable census against the lane base show 0
  regressions, and all three projectiles parks are unchanged.
- Owner census disclosure: the header-inline SELECT_ANY COMDAT copies listed in
  the worker proposal above are the systemic surplus class. No new non-inline code,
  data, BSS or COMMON owner was added.
