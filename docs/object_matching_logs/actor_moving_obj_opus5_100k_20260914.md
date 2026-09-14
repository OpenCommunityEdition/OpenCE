# `actor_moving.obj` — opus5 100K consolidated lane, wave B (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/ai/actor_moving.c`; target `build/split/source/ai/actor_moving.obj`
  (January 2002 Xbox debug build).
- Compiler: VC7 `CL.Exe` 13.00.9254 (`xbox/bin/vc7`), repository flags `/O2 /Oy- /DDEBUG /Dxbox` plus the
  include set from `build.ninja`; every measurement through `tools/campaign/gate.py --source ...
  --forbid-emitted-symbol _point_from_line3d`. Inline-decision diagnostics used the same command line plus
  `-W4 -w44710 -w44711` (`scratch/workers/actor_moving/clw.py`, scratch objects only).
- Evidence: January relocations/bytes (authoritative), January section order, HCEA
  `halocea-review/src/blam/ai/actor_move_*.c` (semantics only), the fresh-graphs Ghidra/atlas outputs
  (read-only copies in `C:/halo-worktrees/opus5-30k-fresh-graphs-20260914/scratch/`; this lane has no
  actor_moving atlas/Ghidra files).
- Worker report with every shape: `scratch/workers/actor_moving.md`; candidates `scratch/workers/actor_moving/*.c|obj`.
- Prior ledgers read: all eleven `actor_moving_obj_*.md` in this directory.

## 2. Validated baseline and result

| | exact | residual | unwritten | exact padded code bytes | data owners |
|---|---:|---:|---:|---:|---:|
| lane HEAD | 22 | 14 | 0 | 3,632 / 20,352 | 70 / 70 |
| this wave (final real-file gate) | **23** | 13 | 0 | **4,272 / 20,352** | 70 / 70 |

Newly strict EXACT: `_actor_move_calculate_controlled_by_aiming` — 640 padded, 16 relocations, normalized
sha16 `1832fd5e56f794b9`.

Residuals in the final object (target vs ours, size/relocs, normalized sha16):

| function | target | ours | class / first divergence |
|---|---|---|---|
| `_actor_move_vector_avoidance` | 4144/135 `a8a8010c` | 4192/140 `1d410a08` | transform inline-cost boundary (2nd transform site inlined in January), weight reloads, frame 0x60e0 vs 0x60ec |
| `_actor_move_update` | 3136/72 `ed110da6` | 3088/72 `7231d70b` | see §4; best scratch candidate reaches 3136/72 |
| `_actor_move_calculate_movement` | 2752/99 `970ee1ef` | 2768/99 `5040dee2` | free-facing pointer spill (frame 0x60 vs 0x5c); 3-way call cross-jump |
| `_actor_path_refresh` | 1440/52 `b6b01d01` | 1440/52 `89b79760` | epilogue-copy selection (3 jcc) |
| `_actor_destination_update` | 976/24 `4c6c5fda` | 976/24 `df9604dc` | x87 lifetime / register |
| `_actor_move_test_avoidance_vector` | 752/16 `271a1af1` | 544/17 `724a80ba` | transform inline-cost boundary |
| `_actor_move_try_evasion_vector` | 592/24 `48da9be7` | 592/24 `74969347` | EBX allocation: `found` in BL (January) vs pointer ranges |
| `_actor_move_avoidance_setup` | 576/18 `3060b44f` | 576/18 `a1eeed08` | owner-blocked (distance2d COMDATs) |
| `_actor_get_stopping_distances` | 464/11 `c22366f1` | 464/11 `2a722792` | dot operand canonicalisation |
| `_actor_move_try_evasion_direction` | 416/18 `a306a2cd` | 400/18 `6dc5c8e3` | attempt_count register vs January `[ebp-4]` |
| `_actor_move_initialize` | 336/15 `4cb84b73` | 336/15 `fcea0b01` | held (data/COMMON provenance) |
| `_actor_aim_jump` | 256/4 `141b04fc` | 256/4 `4087deef` | PARKED (unchanged) |
| `_actor_move_vector_avoidance_find_direction` | 240/2 `92f92b7c` | 240/2 `452690e2` | dot term canonicalisation |

## 3. Accepted control (retained source)

Removed the per-TU `#define normalize3d normalize3d_inline`, its `#undef`, and the local external
`real normalize3d(real_vector3d *)` prototype (5 lines). Evidence: January's relocations prove
normalize3d is inlined at five `_actor_move_vector_avoidance` sites (5 `_real_epsilon` double compares, no
calls), while calculate_free (2), controlled_by_aiming (2), path_3d_available (1), calculate_movement (6) and
move_update (2) call it out of line (13 calls = the 13 undefined `_normalize3d` references; calculate_movement's
single epsilon compare is its explicit `fabs(angle_adjustment) > _real_epsilon`). The rename forced every site
out of line AND hid the callee from the translation unit. With the plain `real_math.h` inline, VC7 13.00.9254
makes exactly January's per-site decisions (18 of 18, confirmed by C4710 at each out-of-line site), and because
the out-of-line callee is now a same-TU COMDAT whose register usage the compiler knows, the callers' register
allocation changes: `_actor_move_calculate_controlled_by_aiming` closes (its former "scheduling tie" was this),
and calculate_movement's frame moves (0x58 -> 0x60; January 0x5c). All 22 baseline-exact
functions stay exact, the `_point_from_line3d` guard passes, the parked `_actor_aim_jump` measurements are
unchanged, fake-match scan reports 0 leads, and all 70 data owners are section-equal.

Owner census: one new owner, `_normalize3d` (SELECT_ANY header-inline COMDAT). January's actor_moving.obj
references `_normalize3d` undefined 13 times; 55 other base objects already emit the identical SELECT_ANY copy
(ActionCharge holds the NODUP copy — the existing math-owner family documented in
`actor_moving_obj_fable_math_owner_audit_20260906.md`). Raised as an orchestrator proposal.

## 4. Experiment matrix (summary; exact measurements in the worker report)

- controlled_by_aiming: nr1 (rename removal) EXACT, landed. Other renames: scale_vector3d removal byte-neutral
  (swaps `_scale_vector3d_inline` owner for `_scale_vector3d`), object_get_bounding_sphere adds unreferenced
  assert literals, distance3d adds `_distance3d` — none applied.
- try_evasion_direction: ed1 `for (;!success && i<n;)` 432; ed2 for+`success = TRUE; break;` + ternary tail
  `*ref = success ? evade_direction : NONE;` 400 (both January exit tails, `movsx ecx,di`/`or eax,-1`, now
  reproduced); ed3 component forward copy; ed4-6 declaration order inert; ed7 j-then-i; ed8 swapped condition 432.
- try_evasion_vector: declaration order, collision_bsp scope, height_delta scope, boolean-first declarations — all
  byte-identical or slot-swap only.
- move_update (on the landed state): mu1 inline `actor_path_has_path` + no actor reload (January relocation
  order reproduced), mu2 declaration order from January's initializer stores, mu5 `timer > 0` arm first + flag
  test, mu6 hover nesting (`airborne_ticks > 0`; `allow_jump` before the up.k test; struct copy + k=0), mu7 grenade
  `crouch` after the vector call and `!= 0.f`, mu8 store order, mu9 `(crouch && TEST_FLAG(..)) || (!crouch &&
  TEST_FLAG(..))`, mu10 `scale_vector3d(&rotation, 1-blend, &rotation)` + three `+=` → **3136/72, sha only**;
  mu11 named accumulator pointer worse. mu3/mu4 probes: flying-block declaration order inert; explicit term order
  is emitted reversed.
- calculate_movement: cm1 two calculate_free calls → frame 0x5c restored, 2784/100; cm2 declaration order and cm3
  local rename inert.
- vector_avoidance / test_avoidance_vector: C4710/C4711 probes (p2-p7): the EXACT transform body is never inlined
  (even `__inline`, even into a truncated caller); a body without the global_zero copy inlines at
  get_avoidance_direction and vector_avoidance but not test_avoidance_vector; a 4-statement body inlines
  everywhere. Function order in January section order (ro1) and removing static forward prototypes (ro2) are
  byte-neutral.

## 5. Do-not-repeat list

- Do not re-add a normalize3d rename; do not look for normalize3d per-site controls (the compiler already matches).
- try_evasion_direction: declaration order, for/while loop spellings with a materialised success flag.
- try_evasion_vector: local scoping/declaration order of `found`, `evasion_is_ledge`, `collision_bsp`, `height_delta`.
- move_update: explicit sum-term reorderings for the local magnitude sites (VC7 emits written order reversed; no
  natural spelling gives January's i,k,j); a named accumulator pointer (mu11).
- calculate_movement: declaration order / renames of the free-path vectors.
- transform: `__inline`, caller truncation, function reordering, prototype removal.

## 6. Residual classification

- Structural, evidence-located but unresolved: transform inline cost (vector_avoidance, test_avoidance_vector);
  move_update pointer-based blend adds, EDI zero pin / EBX NONE pin, in_midair arm cross-jump; calculate_movement
  3-way call cross-jump and free-path slot order.
- Allocation/scheduling ties: try_evasion_vector (BL), try_evasion_direction (attempt_count), destination_update,
  path_refresh epilogue choice.
- Canonicalisation family: find_direction, get_stopping_distances, move_update local magnitude term order.
- Ownership: avoidance_setup (distance2d COMDATs), initialize (data/COMMON).

## 7. Reopen criteria

- A transform spelling that stays EXACT out of line and is inlined exactly at test_avoidance_vector ×2 and the
  second vector_avoidance site (check with the C4710/C4711 probe before gating).
- move_update: start from `scratch/workers/actor_moving/mu10.c` (size/relocs equal) with a natural cause for the
  term order and the `[ecx+4]` add addressing.
- calculate_movement: start from `cm1.c`.
- try_evasion_direction: start from `ed7.c` (single exit + ternary tail already proven by January's exit tails).

## 8. Disposition

Still active, not Matching: 23 / 36 strict exact. Only `_actor_move_calculate_controlled_by_aiming` receives new
exact credit (subject to acceptance of the `_normalize3d` inline COMDAT owner). No fuzzy bodies landed.
