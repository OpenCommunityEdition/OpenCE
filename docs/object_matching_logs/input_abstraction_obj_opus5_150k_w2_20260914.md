# `input_abstraction.obj` — Opus5 150K house-clean lane, wave w2 (2026-09-14)

## Result

The parked `_input_abstraction_update` was re-investigated in scratch only. A complete candidate is strict EXACT:
2,384 padded bytes, 103 relocations, normalized sha16 `7630e7c9cb2c1abf`, the same as January. Relocdiff reports
0 differing rows. All nine sibling functions stay exact, so the whole-TU gate is `exact 10 residual 0 unwritten 0`.

This is a **park-reopen proposal**, not a landing. The real file `source/input/input_abstraction.c` is unchanged, and
the real-file gate stays at baseline: `exact 9 residual 1 unwritten 0`, with the `_point_from_line3d` guard passing.
The orchestrator must verify the candidate, unpark it in `config/parked.json`, and apply the body in a dedicated commit.

- Candidate: `scratch/workers/input_abstraction/candidate_reopen.c`, CRLF, sha256 `94f2845cf52e7d24956d2fbe229dd6d1e2263c3f25d5bfb083136c3c5cb95206`.
- Content: the current real file plus two changes:
  - one new file-scope line, `static real const gamepad_axis_normalization_scale = 1.f / SHORT_MAX;`, placed before `stick_direction_angles`;
  - the replaced `input_abstraction_update` body.
- Candidate object: `scratch/workers/input_abstraction/candidate_reopen.obj`.

## Baseline and duplicate prevention

| row | January | parked body |
| --- | ---: | ---: |
| `_input_abstraction_update` | 2384 / 103 / `7630e7c9cb2c1abf` | 2416 / 114 / non-exact |

Before starting, the following were checked:
- **Ledgers read:** `input_abstraction_obj_jonas_lifecycle_preferences_20260821`, `_jonas_update_batch_rejection_20260828`, `_jonas_residual_freshness_audit_20260830`, `_small_family_wave_20260904`, and `hud_devices_input_next_integration_batch_20260904`.
- **Handoff:** the Opus5 100K handoff. This function is not on any do-not-repeat list, and it is not in the accepted ledger or the 100K manifest.
- **`branch_sweep`:** no blob is better than the current one. History blobs measure 6/0/4 or fail to compile.

## Evidence and measured levers (each moved the January instruction stream)

1. **Ownership: the normalization scale is a static `.rdata` scalar.** `config/symbols.json` already splits the old
   20-byte label into two symbols: `_gamepad_axis_normalization_scale` at 2552736 and `_stick_direction_angles` at 2552740.
   January references the scale by memory four times. The parked source never defined this owner: it emitted
   `__real@38000100` instead. The candidate's 20-byte `.rdata` has the same bytes and symbol offsets as January.
2. **`real_math.h` `sine()` / `cosine()` inline helpers.** These give a real-typed return, and VC7 CSEs `cosine(angle)` as a
   **float**: `fcos; fst dword; fabs; fld st(1); fcompp; ... fld dword; fabs`. Plain `sin`/`cos` with casts store
   `fabs(cos)` as a qword instead (labs l1/l2). `arctangent()` and `square_root()` are the matching house helpers.
   `(real)atan2` and `(real)sqrt` measure identically.
3. **No named x/y locals.** The source is `arctangent(stick.y, stick.x)` with `PIN(stick.x * gamepad_axis_normalization_scale * scale, -1.f, 1.f)`.
   With named `real x/y` locals, or with a struct copy, VC7 reassociates the product to
   `(scale_const*scale)*x`. That happens even for a single statement (lab l17 r1). The stick-field form keeps
   January's `fld x; fmul c; fmul st(1)` and its y-kept-on-x87 CSE (d3).
4. **Preferences are accessed directly** as `input_abstraction_globals.player_control_preferences[i].field`. January
   keeps `i*24` in `[ebp-0xc]`; it does not store a pointer.
5. **Legacy snapping:**
   - The quadrant is built with `|`.
   - The difference is computed first, then the magnitude.
   - `fabs(difference)` is taken where it is used, and CSE'd into the blend.
   - `real absolute_angle = fabs(angle)` makes the pi/4 and 3pi/4 compares float-typed. This is what produces January's
     `__real@3f490fdb` and `__real@4016cbe4`; the double compare emits `__real@3fe921fb60000000`.
   - The compare is `fabs(x) > fabs(y)`, and stores are ordered `x = sign*mag; y = 0`. The reverse store order is not exact.
6. **Switch:**
   - D-pad handling uses if/else statements, which give direct constant stores.
   - Southpaw pitch is `up ? (invert ? -1 : 1)`, `down ? (invert ? 1 : -1)`, `else (invert ? -1 : 1) * left_y`, written
     as if/else. January's arm-folded constants prove this form. The product forms `sign*(dpad ?:)` and `(dpad ?:)*sign`,
     a local, and `*=` all leave an x87 multiply at the join.
7. **Disconnect block:**
   - The available-controller count is a **do-while** loop. This follows the VC7 unroller law: the loop survives in
     January's bytes, while `for` and `while` unroll (lab l14).
   - Both main-menu arms test `c0 != i && ... && !wants(i)`. `show_error` is initialized TRUE and set FALSE in the
     `>= 2` arm, which reproduces January's `jne SHOW; jmp SKIP` layout.
   - Non-menu code is a three-way `else if (global_network_game_client_get()) {...} else {...}`. Each mode sets
     pause, code and `show_error = local_player_exists(controller_index)`, followed by `if (show_error == TRUE)`.
     VC7 hoists the identical arm-head `push ebx` above the `je` and cross-jumps the tails. Lab l21 q4 reproduced this;
     q1, with a single call after the if/else, did not.
   - `local_player_exists` takes `long` (`players.h:205`). The old `(short)` cast was a type defect and produced a `movsx`.
8. **Frame layout.** The four stick outputs are `real_point2d left_stick, right_stick`. VC7 scalar-replaces the aggregate
   and allocates its fields after the other frame objects: January has `[ebp-0x20..-0x2c]` as ly, lx, ry, rx. Scalar
   locals are allocated first in every declaration order and name tested (L1/L2/N1/N2 inert). `real_vector2d`, with or
   without `magnitude2d(&stick)`, is also exact. `real_point2d` was chosen because it parallels `point2d sticks[]`.

## Rejected or non-exact shapes

The candidate chain is kept in `scratch/workers/input_abstraction/parts/`. Measurements are size / relocations.

| shape | result |
| --- | --- |
| v1: static scale, direct preferences, `double` scale | 2416 / 126 |
| v2b: named y/x | 2416 / 116 (reassociation) |
| v4: helper CSE | 2416 / 116 |
| v6: d-pad if/else | 2400 / 104 |
| v7: do-while | 2384 / 101 |
| d1: explicit parentheses | 2384 / 103 (still c-first, and no genuine macro, so inadmissible) |
| int sign constants | `sete`/`lea` |
| T4a: `show_error = local_player_exists(...) == TRUE` | `sbb` materialization |
| h1–h8: `?:`, defaults and cast alternatives for the client branch | not exact |
| E2: `y = 0` stored first | not exact |

## Checks on the candidate

- **Gate:** the whole-TU gate with `--forbid-emitted-symbol _point_from_line3d` passes: exact 10, residual 0.
- **Parks:** `scratch/parkcheck.py` reports drift only for the reopened park itself, which is now exact. The object has no other parks.
- **Fake scan:** `tools/fake_match_scan.py` finds 0 review leads, the same as HEAD.
- **Owner census vs `build/base`:**
  - Added: `_gamepad_axis_normalization_scale`, which January owns with identical bytes.
  - Added: literal COMDATs `__real@3f490fdb` and `__real@4016cbe4`, which January references as undefined literal symbols.
  - Added: non-static `real_math.h` `__inline` COMDATs `_sine`, `_cosine`, `_arctangent`, `_square_root`. This is the known
    systemic surplus: 89 base objects define such helper COMDATs, while only 3 split objects do.
  - Dropped: `__real@38000100`, `__real@3fe921fb60000000`, `__real@4002d97c80000000`. The `$L` label is an artifact.
  - No BSS or COMMON change.
- **Semantics:** the candidate is equivalent to the parked body. The only differences are January-proven: float-typed
  compares and the polarity of the snap-window test for NaN inputs.

## Authenticity

The body is ordinary typed game code written in house style:
- real_math helpers and typed aggregates;
- cseries `MAX`/`PIN`;
- named enum constants;
- a three-way mode chain;
- per-mode assignments.

It uses no casts, parentheses, volatile, pragmas, assembly, or raw offsets to steer codegen. One point needs reviewer
attention: `show_error = local_player_exists(controller_index)` appears once in each client arm. The lab proves this is
what January's hoisted push requires, and it reads naturally as "each mode fully specifies pause, error code, and
whether to show".

## Orchestrator actions

- Verify and unpark `_input_abstraction_update` from `config/parked.json` in a dedicated commit. The park criterion was
  "authoritative source or new precision/ownership evidence". It is met by the ownership (1) and precision (2, 5)
  evidence above, plus the measured VC7 laws (3, 6, 7, 8).
- No header or config text change is needed. `symbols.json` already has the correct static split.

## Reopen criterion (if rejected)

A reviewer may rule the duplicated per-arm `local_player_exists` call or the do-while count loop inadmissible. In that
case, the next-best candidate is sL/hX1: 744 instructions, with only one hoisted push plus the slot permutation. Reopen
it with a different natural spelling that reproduces the arm-head push hoist.
