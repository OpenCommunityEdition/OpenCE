# `actor_moving.obj` — opus5 100K consolidated lane, wave C (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/ai/actor_moving.c`; target `build/split/source/ai/actor_moving.obj`
  (January 2002 Xbox debug build).
- Compiler: VC7 `CL.Exe` 13.00.9254, repository flags `/O2 /Oy- /DDEBUG /Dxbox` plus the `build.ninja`
  include set. All measurements through `tools/campaign/gate.py --source ... --forbid-emitted-symbol
  _point_from_line3d`. Per-call-site inline decisions: same command line plus `-W4 -w44710 -w44711`
  (`scratch/workersC/actor_moving/probe.py`; under Git Bash pass extra flags as `-Ob1`, not `/Ob1`, which
  gets rewritten into a path).
- Evidence: January relocations/bytes and section order, HCEA `halocea-review/src/blam/ai/actor_*.c`
  (semantics, and the 360 lift's operand orders as a second-compiler witness), read-only atlas/Ghidra copies
  under `C:/halo-worktrees/opus5-30k-fresh-graphs-20260914/scratch/`.
- Worker report with every shape: `scratch/workersC/actor_moving.md`; candidates in
  `scratch/workersC/actor_moving/`.
- Prior ledgers read: all twelve `actor_moving_obj_*.md` in this directory, the wave B worker report and the
  fresh-graphs worker report.

## 2. Validated baseline and result

| | exact | residual | unwritten | exact padded code bytes |
|---|---:|---:|---:|---:|
| lane HEAD (wave C baseline) | 23 | 13 | 0 | 4,272 / 20,352 |
| this wave (final real-file gate) | **24** | 12 | 0 | **4,512 / 20,352** |

Newly strict EXACT: `_actor_move_vector_avoidance_find_direction`: 240 padded, 2 relocations, normalized
sha16 `92f92b7c10eb4d14`.

Residuals in `scratch/workersC/actor_moving/final.obj` (target vs ours, size/relocs, normalized sha prefix):

| function | target | ours | class / first divergence |
|---|---|---|---|
| `_actor_move_vector_avoidance` | 4144/135 `a8a8010c` | 4192/140 `1d410a08` | transform inline decision (site 2 inlined in January) |
| `_actor_move_update` | 3136/72 `ed110da6` | 3088/72 `7231d70b` | owner-blocked blend helper (point_from_line3d), then register items |
| `_actor_move_calculate_movement` | 2752/99 `970ee1ef` | 2768/99 `5040dee2` | call-tail cross-jump + frame packing |
| `_actor_path_refresh` | 1440/52 `b6b01d01` | 1440/52 `89b79760` | 3 jcc pick the earlier identical epilogue |
| `_actor_destination_update` | 976/24 `4c6c5fda` | 976/24 `df9604dc` | step pointer registers, x87 offset schedule |
| `_actor_move_test_avoidance_vector` | 752/16 `271a1af1` | 544/17 `724a80ba` | transform not inlined (both sites) |
| `_actor_move_try_evasion_vector` | 592/24 `48da9be7` | 592/24 `74969347` | BL allocation |
| `_actor_move_avoidance_setup` | 576/18 `3060b44f` | 576/18 `a1eeed08` | owner-blocked (distance2d + prohibited casts) |
| `_actor_get_stopping_distances` | 464/11 `c22366f1` | 464/11 `2a722792` | biped dot operand canonicalisation |
| `_actor_move_try_evasion_direction` | 416/18 `a306a2cd` | 400/18 `6dc5c8e3` | attempt_count spill vs hoisted evasion_is_ledge |
| `_actor_move_initialize` | 336/15 `4cb84b73` | 336/15 `fcea0b01` | held (data/COMMON provenance) |
| `_actor_aim_jump` | 256/4 `141b04fc` | 256/4 `4087deef` | PARKED (unchanged) |

## 3. Accepted control (retained source)

`actor_move_vector_avoidance_find_direction`: the loop no longer binds a `direction` pointer local. The loop cross
product uses the same indexed spelling as the function's own `previous_cross` initializer
(`directions[direction_index].j*direction_vector->k - directions[direction_index].k*direction_vector->j`), and the
hemisphere test is `dot_product3d(direction_vector, &directions[direction_index]) > 0.f` (7-line diff).

Evidence: January's dot product emits terms k, j, i with operand orders `dir.k*v.k`, `dir.j*v.j`, `v.i*dir.i`.
Measured mechanism: VC7's commutative term and operand order for an inlined dot or magnitude depends on how the
operands are referenced elsewhere in the function (a named pointer local versus a CSE'd indexed expression, direct
field operations versus pointer helpers), and not on how the dot itself is written. With the named pointer, all
five earlier spellings stayed i, j, k. Without it, the order becomes k, j, i, and the argument order then fixes the
i-term's operands.

Checks: all 23 baseline-exact rows stay exact (full listing diff shows only this row); the `_point_from_line3d`
guard passes; the parked `_actor_aim_jump` stays 256/4/`4087deef…` (= `measurements.base`); owner census 155/155
with no new owner; the fake-match scan reports 0 leads.

## 4. Experiment matrix (summary; exact measurements in the worker report)

- find_direction: f1 (no pointer local, `dot(&directions[i], v)`) gave k,j,i order with 1 block left. f2
  (`dot(v, &directions[i])`) is **EXACT** and landed.
- move_update (scratch only, guard-blocked): January's blend adds go through `[ecx]`/`[ecx+4]`/`[ecx+8]`, which is
  exactly the inline body of `point_from_line3d` with p == result. mb2 (inline point_from_line3d, TU import removed)
  reproduces that addressing and flips the first local magnitude term order to January's i,k,j. mb3 (adds
  `scale_vector3d(&rotation, 1/angle, &rotation)`) fixes the second site. mb4 (store order: steering, throttle-if,
  oversteer lower/upper) aligns the vehicle block. All are 3120/72 sha with 49 blocks, and none can land because
  the TU would emit `_point_from_line3d`.
- transform inline decisions (diagnosis): graded transform bodies on the real callers give per-site thresholds
  in the order vector_avoidance > get_avoidance_direction > test_avoidance_vector. January's pattern (test ×2 and
  VA site 2 inlined; get and VA site 1 not) needs thr(test) > thr(get), so no transform spelling alone can
  reproduce it. Synthetic callers show that the per-site budget shrinks with statements before the call (stores,
  distinct asserts), grows with the number of sequential loops before the call (loops after the call do not help,
  nesting does not help), and is not a per-TU or per-callee budget. `-Os` disables auto-inlining. VA sites 1 and 2
  always received the same decision here (h1–h3). Rewriting test's origin/direction with inline
  point_from_line3d/scale_vector3d (January's bytes are those bodies) did not make the transform inline.
- get_stopping_distances: g2 biped `dot(fwd, vel)` fixed the i-term but reversed k/j. g3 also swapped the
  vehicle branch (worse). g5, a shared function-scope `unit` pointer, gave the opposite asymmetry (the i-term
  reuses the vehicle value number). g7/g8 cast bases were identical to g2.
- calculate_movement: c1 (two free-path calls) gives 2784/100: the face tail merges but the else call keeps its
  own arg-load order (al, esi, ebx). c2 (actor_facing_vector scoped into its if-block) is identical to c1.
- destination_update: du1 (initialise `next_step_point` before `step_point`) cuts 25 blocks to 12. du2 (vector
  declaration swap) and du3 (operand order in the flying point stores) are inert. Not exact.

## 5. Do-not-repeat list

- find_direction: closed.
- Transform auto-inlining: `__inline`, component-wise zero copies, HCEA no-component body, per-body grading,
  caller arg kinds, static/extern caller, big locals, asserts added to get/VA, caller point_from_line3d rewrite.
- get_stopping_distances: argument swaps in either branch, shared unit pointer, cast bases.
- move_update: explicit sum-term spellings (the real cause is the blend/scale helpers; see mb2/mb3).
- calculate_movement: two-call form with or without scoped actor_facing_vector.
- destination_update: vector declaration order and point-store operand order.

## 6. Residual classification

- Owner/policy-blocked with located source: move_update (point_from_line3d header inline; remaining register
  items unresolved).
- Structural and unresolved: transform per-site inline decisions (vector_avoidance, test_avoidance_vector). The
  measured caller-side law is a fact; which caller difference January had is unknown.
- Scheduling/allocation ties: calculate_movement (call-tail merge), try_evasion_direction (EBX hoist),
  try_evasion_vector (BL), destination_update (register naming and x87), path_refresh (epilogue copy).
- Canonicalisation: get_stopping_distances (biped dot operands).
- Ownership/provenance holds: avoidance_setup, initialize.

## 7. Reopen criteria

- move_update: the orchestrator permits actor_moving.obj to emit the header-inline `_point_from_line3d`
  COMDAT. January references it undefined, which is the same family as the accepted `_normalize3d`. Then start from
  `scratch/workersC/actor_moving/mb4.c`.
- test_avoidance_vector / vector_avoidance: a January-authentic caller-side reason for test's call sites to
  out-budget get_avoidance_direction's (by the measured law, e.g. a preceding loop), or a same-compiler donor
  with a split site-1/site-2 decision.
- get_stopping_distances: January evidence of an earlier j/k use of the biped velocity/forward leaves.
- calculate_movement: from c1.c, a cause for the else-arm arg-load order edi, esi, al, ebx.
- destination_update: from du1.c.

## 8. Disposition

Still active, not Matching: 24 / 36 strict exact. `_actor_move_vector_avoidance_find_direction` is newly exact
with no new owner. No fuzzy bodies landed; the park is unchanged.
