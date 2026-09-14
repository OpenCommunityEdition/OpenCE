# `action_vehicle.obj`: opus5 150K house-clean lane, wave w1 (2026-09-14)

## 1. Scope and provenance

- Source TU: `source/ai/action_vehicle.c`. Target: `build/split/source/ai/action_vehicle.obj` (January split).
- Lane `opus/150k-house-clean-20260914`, base `12f7375d4`. Only `source/ai/action_vehicle.c` was modified.
  No header, config, build, or tool file was touched. No ninja, configure, pytest, or git write was run.
- Measurement: `tools/campaign/gate.py`, `alndiff`, `relocdiff`, `tinfo`, `scratch/parkcheck.py`, and `tools/fake_match_scan.py`.
  A `/Zs /W3` warning census was also run with the unit's own cflags.
- Ledgers read: all 11 `action_vehicle_obj_*` logs, including `opus5_100k_20260914` and its admission note, plus the
  `opus5_100k_consolidated_HANDOFF_20260914` rejections. The 100K worker report is at
  `C:\halo-worktrees\opus5-100k-consolidated-20260914\scratch\workers\action_vehicle.md`.
- `git log --all` shows 14 commits for the file. `branch_sweep`: the best historical blob has 9 exact functions.
- Evidence sources:
  - January COFF: code, relocations, and COMDAT set.
  - HCEA `action_vehicle_{desirable,evaluate_seat,find_destination}.c`: semantics and topology only.
  - January `actors.obj` `.rdata` assert literals (section 5).
- Per-function worker report: `scratch/workers/action_vehicle.md`. Candidates are in `scratch/workers/action_vehicle/`.

## 2. Baseline (real file at lane base, guard on)

`== exact 10  residual 7  unwritten 0  (of 17 listed)`. The unit has no parks.

| Residual | Target padded / relocs | Base tags |
| --- | --- | --- |
| `_action_vehicle_desirable` | 272 / 7 | size 288 |
| `_action_vehicle_evaluate_seat` | 976 / 30 | size 960, relocs 31 |
| `_action_vehicle_find_destination` | 1232 / 34 | size 1264 |
| `_action_vehicle_perform` | 768 / 18 | sha |
| `_action_vehicle_setup_impromptu` | 464 / 18 | sha |
| `_action_vehicle_setup_specific` | 336 / 15 | sha |
| `_distance3d` | 64 / 0 | sha |

## 3. Landed (final real-file gate `== exact 12  residual 5`, guard passed)

| Function | Padded | Meaningful | Relocs | Normalized sha16 |
| --- | ---: | ---: | ---: | --- |
| `_action_vehicle_perform` | 768 | 767 | 18 | e61cabd696765a6f |
| `_action_vehicle_setup_specific` | 336 | 328 | 15 | aa10ba89a138d06b |

The gain is **1,104 padded / 1,095 meaningful** code bytes. All 10 baseline-exact rows are unchanged. The row diff is:
- perform and setup_specific: residual -> EXACT;
- find_destination: tag 1264 -> 1248.

### 3.1 New lever: callee register pressure through a late-bound position pointer (find_destination)

January `action_vehicle_find_destination` is a private register-ABI function. Every caller loads
`ecx = actor_index` and `eax = vehicle_index`, then pops 0x18.

**How January's body uses the actor pointer.** The actor datum pointer is stored once at entry, to `[ebp-0x14]`.
It is loaded exactly once, at +0x113, immediately after the anchor-point selection, into `edi`:

    mov edi,[ebp-0x14]; fsub [edi+0x12c]; add edi,0x12c; ...

All later body-position reads go through that one pointer, including the `debug->field_C8` struct copy
(`mov edx,edi`). In the base source the actor pointer was used at every site, so it took `ebx`, and
`vehicle_index` lost its register parameter.

**Source change.** `real_point3d const *actor_position;` is declared with the block locals. It is assigned
`&actor->input.position.body_position` immediately before its first use, which is the entry-direction computation after
the anchor selection. Every body-position read goes through it.

**Why the assignment point matters (measured):**

| Placement of the pointer | Result |
| --- | --- |
| Initialized at the top of the if-block (f2) | ABI unchanged |
| Initialized at the top of the function (f5) | ABI unchanged |
| Assigned just before the anchor selection (f8) | ABI unchanged |
| Assigned just after the anchor selection (f3 / landed m1) | January ABI |
| Declared and initialized in a nested block after the anchor selection (f4) | January ABI (not used, because it needs a bare block) |

With the pointer assigned after the anchor selection, the pointer's live range begins where January's `edi` derivation begins.
Only the minimal routing change was landed. The residual rule does not allow more changes to this function.
find_destination stays residual at 1248/34 with frame 0x468 (January 0x464). Its first 0x55 bytes are now identical to January.

### 3.2 Non-redundant desirable chain (required prerequisite)

The landed desirable has no initializer. It reads:

    if (dead) desirable = FALSE;
    else if (scripted) desirable = TRUE;
    else { ...; desirable = FALSE; if (already_inside || distance_squared3d(body, origin) < max*max) { desirable = TRUE; if (moving) desirable = FALSE; } }

The upright veto follows unchanged. The TU-local `REAL_MATH_EXTERNAL_MAGNITUDE_SQUARED3D` define/undef pair is removed, as the 100K lane
proved necessary. Semantics are identical to the base on all inputs, and no store restates an earlier value. This avoids the
redundant dead-arm re-store that got the 100K `vd` form rejected.

This body gives desirable January's private ABI (`eax = actor_index`, `esi = vehicle_index`) and January's branch layout.
- **perform:** the new ABI removes perform's actor/vehicle register swap.
- **setup_specific:** its inline copy gets January's branchless dead/scripted if-conversion.

Measured prerequisites:

| Variant | Result |
| --- | --- |
| Base desirable + m1 | perform, setup_specific residual |
| d1 keeping the magnitude define | perform grows to 880 bytes |
| d3 (d1 with named dx/dy/dz) | setup_specific exact, perform not |
| d2 (boolean-expression form) | desirable no longer inlined into setup_impromptu |

desirable itself stays residual at 288/7. The only remaining difference is that the not-scripted `desirable = FALSE` store is not hoisted
into January's entry `xor bl,bl`.

### 3.3 Checks

- Point guard passed.
- parkcheck: no parks.
- Owner census against `build/base`: one added select-any COMDAT, `_magnitude_squared3d`. January references it undefined from desirable
  and setup_impromptu; it is the systemic header-inline class. There is no data, BSS, or COMMON change, and nothing is dropped.
- Fake scan: 0 leads (base 0).
- `/W3` warnings: identical to the base.

## 4. Not landed

| Function | Final | Classification / reason |
| --- | --- | --- |
| `_action_vehicle_evaluate_seat` | 960/31 in the real file; **strict EXACT scratch candidate** `E1.c` | Blocked only by the earlier orchestrator decline of 2D-helper casts; see section 5 |
| `_action_vehicle_find_destination` | 1248/34 | Offset-vector representation cast, plus x87 operand-order ties (below) |
| `_action_vehicle_desirable` | 288/7 | Owner ruling on the vd dead-arm restatement |
| `_action_vehicle_setup_impromptu` | 464/18 sha | Depends on vd. With vd + m1 only the inlined x,z,y square order differs |
| `_distance3d` | 64 sha | Exhausted per the 2026-08-27 ledger |

**find_destination cast diagnostic (x2/x5).** Size 1232, frame 0x464, and January's success/failure epilogue layout are reached with a
single-exit `found` flag. The remaining x87 operand-order differences were measured inert:
- destination `fld down.i; fmul t`: operand swap, `test.x + down.i*t`, and the inline point_from_line3d body (diagnostic, guard failure)
  all still load `t` first;
- the refine dot operand order;
- the offset_distance temp lifetime;
- the chosen-to-actor square order.

**distance3d via a genuine caller (g1).** Using the header-inline copy through `distance3d(&marker, hint_point)` emits the header body with
sha 01d7809ae796fc6e, which is the wave-one z,x,y schedule. That is a measured negative: the COMDAT body does not depend on its callers.

## 5. evaluate_seat: new levers and the proposal

The 100K lane stopped at 992/31: its cast form still inlined the second `normalize2d`. Three new measured VC7 levers close
that gap. All three are cast-independent mechanisms.

1. **Loop spelling controls post-loop inline frequency.**
   - `for (prop = next; prop; prop = next)` keeps `normalize2d(&to_seat_direction)` inline.
   - `while (prop = prop_iterator_next(&iterator))`, or `prop = next; while (prop) {...; prop = next;}`, makes it an out-of-line call
     as in January.
   - A diagnostic with the loop deleted also puts the call out of line. The rotated for-loop inflates the estimated frequency of the code
     after it.
2. **Sibling scope shares a frame slot.** Putting the 8-byte `prop_iterator` in its own block around the loop lets it share `[ebp-0x18]`
   with `to_seat_direction`. The frame goes from 0x5c to 0x54.
3. **Single exit (HCEA topology).** The body sets `result = TRUE` after the entrance point is found. A conflicting actor does
   `result = FALSE; break;`. The tail sits in `if (result)` with one `return result;`. This reproduces January's three epilogues:
   the threaded `xor al,al`, the tail-duplicated `mov al,1`, and `mov al,bl`.

The candidate `scratch/workers/action_vehicle/E1.c` (the current real file plus `E1.txt`) makes evaluate_seat **strict EXACT at 976/30/9f67690555e799a6**.
- Whole-TU gate: exact 13, with all 12 real-file exact rows kept.
- Point guard passed; fake scan 0 leads.
- Census: adds a header-inline `_dot_product2d` COMDAT (systemic class).
- It also adds the legal `#include "units/vehicle_definitions.h"`. This removes the base's C4013 implicit
  `vehicle_specific_definition_get` call to a nonexistent `_vehicle_specific_definition_get`.

**Casts in the candidate:**
- `normalize2d((real_vector2d *)&facing)`;
- `MIN(distance2d((real_point2d *)&body, (real_point2d *)&entrance_point), distance2d(... &seat_point))`;
- `vector_from_points2d((real_point2d *)&body, (real_point2d *)&seat_point, &to_seat_direction)`;
- `dot_product2d(&to_seat_direction, (real_vector2d *)&actor->input.facing_vector)`.

**New January-side evidence for these casts:**
- January `actors.obj` `.rdata` contains the assert literals `(real_vector2d *) &actor->input.facing_vector` and
  `(real_vector2d *) &control_data.facing_vector`. These are stringified by `assert_valid_real_normal2d`, and canonical
  `source/ai/actors.c:1946-1950` and `4167-4171` already reproduce them. So Bungie's AI code applied 2D helpers to 3D vectors through
  exactly this cast.
- The canonical tree already carries 31 such casts in admitted exact code. Examples:
  - `action_obey.c:2449` `normalize2d((real_vector2d *)&facing) == 0.0f`;
  - `action_obey.c:2615-2622`;
  - `action_charge.c:769/834`;
  - `actor_combat.c:2310`;
  - `actor_firing_position.c:1548`.
- January's own action_vehicle COMDAT set is `point_from_line2d`, `distance_squared2d`, `distance2d`, `negate_vector2d`, `distance3d`,
  in `real_math.h` order. It shows that this TU instantiated the 2D helpers.

**ORCHESTRATOR-PROPOSAL:** admit these January-attested casts for `action_vehicle_evaluate_seat`, and apply `E1.txt` (the function body plus the
include) to `source/ai/action_vehicle.c`. Expected gain: 976 padded bytes. No header or config change; the change has no external consumers.

## 6. Do-not-repeat (this wave)

- desirable:
  - d1/d2/va/d3 and d1 with the magnitude define (all measured above);
  - the dead-arm restatement (vd) is the only exact form.
- find_destination pointer placement: f2, f5 and f8 do not change the ABI. Accessor binding on the first use only (f1) does not change it either.
- The destination x87 load order is inert to operand order, sum order, and the inline point_from_line3d body.
- setup_impromptu: d2 stops the desirable inline.
- `_distance3d`: the header copy via a genuine caller gives the z,x,y body.
- evaluate_seat, still inline: nested `normalize2d(vector_from_points2d(...))` (e4), splitting the first condition into nested ifs (e5),
  and a no-op cast on the site-2 argument (ed1).

## 7. Reopen criteria

- **evaluate_seat:** the cast ruling in section 5 (candidate ready).
- **find_destination:** the cast ruling, plus a source spelling that controls the destination/refine x87 operand order. Start from
  `scratch/workers/action_vehicle/x5.c`. The closer cast-free body is `f7.c`.
- **desirable, setup_impromptu:** an owner ruling on vd, or a non-redundant source whose FALSE store VC7 hoists to the entry.
  After that, the inlined x,z,y square order in setup_impromptu remains.
- **`_distance3d`:** the 2026-08-27 criteria.
