# `actor_moving.obj` — opus5 150K house-clean lane, wave w1 (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/ai/actor_moving.c`; target `build/split/source/ai/actor_moving.obj`
  (January 2002 Xbox debug build, VC7 13.00.9254).
- All measurements through `tools/campaign/gate.py --source ... --forbid-emitted-symbol _point_from_line3d`,
  `tools.campaign.alndiff`, `tools.campaign.tinfo`, `scratch/parkcheck.py`, `tools/fake_match_scan.py`.
- Worker report with every shape: `scratch/workers/actor_moving.md`; candidates in `scratch/workers/actor_moving/`.
- Prior ledgers read: all thirteen `actor_moving_obj_*.md`, both 100K-lane worker reports (waves B and C) and the
  100K consolidated handoff (do-not-repeat lists honoured; `_actor_move_update` excluded and untouched).

## 2. Baseline and result

| | exact | residual | unwritten |
|---|---:|---:|---:|
| baseline (HEAD 12f7375d4) | 24 | 12 | 0 |
| final real-file gate | **25** | 11 | 0 |

Newly strict EXACT: `_actor_move_calculate_movement` — 2,752 padded (~2,744 meaningful), 99 relocations,
normalized sha16 `970ee1ef079d1207` (= target). Listing diff vs baseline shows only this row; guard passes; park
`_actor_aim_jump` unchanged (256/4/`4087deef`); owner census 155/155 (only `$L` labels renumber); fake scan 0 leads;
CL /W3 warning count unchanged.

Park-reopen proposal: `_actor_aim_jump` (256 padded) — scratch candidate `scratch/workers/actor_moving/aim_reopen.c`
(current real file + only the aim body) is strict EXACT with zero sibling loss.

## 3. New measured VC7 facts (reusable)

1. **Emission order is bottom-up compile order.** VC7 emits a same-TU callee before every caller, including callers
   that auto-inline or force-inline it and for extern/static/`__inline` callees (synthetic probes
   `scratch/workers/actor_moving/syn/*`; real-TU probe `ro_a.c` defers get/test/VA to the end). January's own data:
   5,141 same-object REL32 calls in 468 split objects, only 18 caller-before-callee, all recursion cycles
   (`scratch/workers/actor_moving/orderscan.py`). Consequence: a function emitted *before* a same-TU helper whose body
   appears inline in it did not call that helper.
2. **Argument-temporary signature.** An inlined `set_real_vector3d`/`set_real_vector2d` call evaluates the argument
   expressions right-to-left onto the x87 stack before the first store and moves a plain `real` field argument through
   the x87 (not an integer copy); VC7 hoists a common argument (e.g. the `k` of four cases) to the join. Explicit
   component statements (any order) cannot produce this.
3. **Assert after if/else.** An invariant assert placed after an `if/else` whose else arm stores a constant is jumped
   over on the constant arm (the else block is laid out before the assert).
4. **Pinning an already-assigned value** (`x = a; ... x = PIN(x, lo, hi);`) leaves no store on the in-range arm;
   `x = PIN(a, lo, hi)` re-stores it.
5. **Field load reused across calls ⇒ a local.** VC7 does not CSE a structure field load across calls; January's
   register reuse across two calls proves a named local.
6. **Pointer local liveness sets the frame.** A pointer initialised at block top and reassigned later is live across
   intervening calls and spills; assigning it in both arms right before its use keeps it in a register.
7. **Accumulator initialisation placement.** Initialising loop accumulators/flags immediately before the loop that uses
   them (not at declaration) shortens live ranges and can free BL/EDI (vector_avoidance frame −8).
8. **`MAX(PIN(...), b)`** re-expands the PIN in the MAX arms (extra constant loads); January evaluates the PIN once.
9. **PAREN/FPU law extends to real assignments:** `component = (expr);` produces `fld src; fst home` instead of an
   integer copy (diagnostic only; not admissible without a genuine macro).

## 4. Per-function outcomes

### `_actor_move_calculate_movement` — EXACT, landed
Six value-identical, evidence-led source changes (each required; ablation of any one breaks exactness):
- override backward/left/right facing cases and the four pathfinding-test cases use `set_real_vector3d` (fact 2;
  same idiom as `actions.c`'s dive-direction switch); the separate `movement_direction.k = facing.k` store is folded
  into the calls;
- `free_facing_vector` assigned in both arms (fact 6; January loads the pointer into EDI per arm, frame 0x5c);
- `long pathfinding_surface_index` local reused as the `structure_test_line2d` argument (fact 5);
- the 1634 `maximum_throttle` assert after the if/else (fact 3);
- `steering_angle = PIN(steering_angle, minimum, maximum)` (fact 4).
Probes rejected: struct copy for the forward case, `set_real_vector2d` + separate k, dot argument swap, explicit dot
spellings (40 blocks), declaration swap / scoping of the free-path vectors (inert), `cross_product3d` (neutral).

### `_actor_aim_jump` — PARK-REOPEN-PROPOSED
January evaluates `k, j, i` onto the x87 before any store and stores `i, j, k`: the argument-temporary signature of
`set_real_vector3d(jump_velocity, align.i*hvel, align.j*hvel, vertical_vel)` (fact 2). That form is strict EXACT;
the control `aim2` (explicit stores written k, j, i) integer-copies `k` first and stays residual. The park's premise
(explicit component stores authentic; helper form = tuning) is refuted by the compiler law rather than by donor
history. Checks on `aim_reopen.c`: exact 25→26, only the aim row changes, guard passes, census 155/155, fake scan 0.

### `_actor_move_test_avoidance_vector` — NOT landed
Fact 1: January emits it (section 18) before `_actor_move_transform_avoidance_vector` (section 20), so its source did
not call the transform; the basis transform is written in the function, with both zero inits first (ta2: 272/272
instructions, frame slots aligned, 768/16). Remaining: ray origin/direction are the inline `point_from_line3d`/scale
bodies (ta4/ta5, guard-failing: the `_point_from_line3d` COMDAT is policy-blocked) and the component x87 defs, which
reproduce only with parenthesised assignments (fact 9, not admissible). Best scratch: `ta5.c` (18 blocks) /
`tparen.c` (15 blocks).

### `_actor_move_vector_avoidance` — NOT landed (large progress in scratch)
`va6.c` reaches 4128/135 (relocation count equal to January's 135; size 4144): `cross_product3d(&object->object.up,
&object->object.forward, &avoidance_data.left)` (January's inline cross body), site 2 written out with no
`perpendicular` local (a `__forceinline` diagnostic keeps the local and grows to 4304, so an inlined call cannot
explain it), avoid-ray weight loop clear-arm first, accumulator init before the weight loop (fact 7), the two
`MAX(PIN())` split (fact 8), and short adjacent-direction index locals computed before the adds. Remaining: frame
0x60e4 vs 0x60e0 (January's site-2 component shares emergency's home via an x87 def), loop register naming.

### `_actor_move_try_evasion_direction` — NOT landed
`set_real_vector2d` reproduces January's forward-case x87 copy (ed9/ed10), but the random arms still cross-jump
because `attempt_count` holds EBX (January spills it; EBX = hoisted `evasion_is_ledge`). Best: `ed10.c`.

### Others
- `_actor_path_refresh`: one nested-success shape (pr1) worse; exhausted (fresh-graphs v1–v6).
- `_actor_destination_update`, `_actor_get_stopping_distances`, `_actor_move_try_evasion_vector`: reviewed with the
  new facts; no new admissible lever (point2d casts blocked; operand canonicalisation; BL allocation).
- `_actor_move_avoidance_setup`: owner-blocked (distance2d casts); `_actor_move_initialize`: held (data/COMMON).
- `_actor_move_update`: excluded, untouched.

## 5. Proposals

- PARK-REOPEN `_actor_aim_jump` (above).
- Policy evidence: January `_actor_move_test_avoidance_vector` ray-origin bytes are the inline `point_from_line3d`
  body — a further consumer for the pending `_point_from_line3d` COMDAT ruling (with `_actor_move_update`).

## 6. Do-not-repeat

- calculate_movement: closed.
- test_avoidance_vector: do not look for a transform inline heuristic (emission order proves no call); zero-init
  order and component spellings measured (ta1–ta9); parenthesised assignment is the only component-def lever.
- vector_avoidance: `__forceinline`/call forms at site 2; site 2 without a component variable (reloads, 139 relocs).
- try_evasion_direction: `set_real_vector2d` alone does not move attempt_count out of EBX.

## 7. Reopen criteria

- vector_avoidance: start from `scratch/workers/actor_moving/va6.c`; needs the site-2 component slot sharing
  (x87-def home) and loop register naming.
- test_avoidance_vector: `_point_from_line3d` COMDAT ruling plus an admissible source for the component x87 defs
  (e.g. an authenticated macro); start from `ta5.c`/`tparen.c`.
- try_evasion_direction: from `ed10.c`, a cause for attempt_count's spill.

## 8. Disposition

Still active, not Matching: 25 / 36 strict exact (26 with the aim reopen). `_actor_move_calculate_movement` receives
new exact credit with no new owner; no fuzzy bodies landed.
