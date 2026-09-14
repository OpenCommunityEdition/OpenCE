# `action_vehicle.obj`: opus5 100K consolidated lane (2026-09-14)

## 1. Scope and provenance

- Source TU: `source/ai/action_vehicle.c`; target object: `build/split/source/ai/action_vehicle.obj`
  (January split, SHA-256 `49F00A01...F769` per earlier ledgers).
- Compiler: Microsoft XDK 3911 CL 13.00.9254 with the repository's unchanged flags
  (measured only through `tools/campaign/gate.py`; no ninja/configure run by this worker).
- Lane: `opus/100k-consolidated-20260914`, HEAD `b8cd50113`; worker claim `waveA:action_vehicle`.
- Evidence used: January COFF (code, relocations, literals); January map atlas
  `scratch/atlas/action_vehicle.obj.txt`; HCEA `action_vehicle_{desirable,find_destination,setup_specific,setup_impromptu}.c`
  for semantics only. Prior ledgers read: `action_vehicle_obj_evaluate_seat_symbol_reconciliation_20260913`,
  `..._fable_control_intake_20260906`, `..._fable_control_reconciliation_20260906`,
  `..._jonas_distance3d_fixed_point_20260827`, `..._jonas_one_leaf_20260821` and the grep hits in the
  units/path_obstacles/fable intake ledgers. `branch_sweep.py source/ai/action_vehicle`: no blob with more exact rows.
- Per-function worker report (full measurements): `scratch/workers/action_vehicle.md`.

## 2. Validated baseline

Gate at lane HEAD: `== exact 10  residual 7  unwritten 0  (of 17 listed)`; no parked entries.

| Residual | Target padded / relocs / nsha16 | Base padded / relocs | First divergence |
| --- | --- | --- | --- |
| `_action_vehicle_desirable` | 272 / 7 / e3b1ecf64be4fb82 | 288 / 7 | callee-saved push `edi` vs `esi` (private ABI), dead-test layout |
| `_action_vehicle_evaluate_seat` | 976 / 30 / 9f67690555e799a6 | 960 / 31 | `mov eax,[ebp+8]` before actor load; `_vehicle_specific_definition_get` call vs `_tag_get` |
| `_action_vehicle_find_destination` | 1232 / 34 / 698d66d07e83af5b | 1264 / 34 | frame `0x464` vs `0x468`; register params `ecx`/`eax` |
| `_action_vehicle_perform` | 768 / 18 / e61cabd696765a6f | 768 / 18 | actor/vehicle register swap `edi`/`esi` |
| `_action_vehicle_setup_impromptu` | 464 / 18 / 0347ef4f8993dc07 | 464 / 18 | inlined desirable square order; find_destination call ABI |
| `_action_vehicle_setup_specific` | 336 / 15 / aa10ba89a138d06b | 336 / 15 | inlined desirable dead test (branchy vs branchless) |
| `_distance3d` | 64 / 0 / 4cdb6852c1705b5f | 64 / 0 | x87 square order (exhausted, 2026-08-27) |

## 3. Accepted controls (retained in production)

`_action_vehicle_desirable` is strict EXACT (272 padded, 272 meaningful, 7 relocations) in the whole-TU
real-file gate `== exact 11  residual 6  unwritten 0`. Two source changes, both in `action_vehicle.c`:

1. The TU-local `#define REAL_MATH_EXTERNAL_MAGNITUDE_SQUARED3D` / `#undef` pair was removed, which restores
   the header's default inline linkage. January inlines `magnitude_squared3d` inside the distance test and calls it
   out of line for the velocity test. With the header default, VC7 makes the same per-call choice by itself.
2. The body now reads `boolean desirable = FALSE;` followed by `if (dead) desirable = FALSE; else if (scripted)
   desirable = TRUE; else { ...; if (already_inside || distance_squared3d(&body, &origin) < max*max) {...} }`.

Binary support for each part:
- January computes the three coordinate differences *after* the `already_inside` test, with no `fstp` cleanup.
  So the distance is an inline helper in the `||` right operand, not a set of pre-computed locals.
- `xor bl,bl` comes before the dead test, which is the initializer.
- The not-dead body is placed after the return, which is the if/else-if chain.
- January's `setup_specific` inlines desirable with scripted=TRUE as branchless `shr cl,2; not cl; and cl,1`.
  VC7 only if-converts an if/else whose arms both store constants.
- January's `setup_impromptu` inlines desirable, and its dead path jumps to the `xor bl,bl` block that it
  cross-jumps with the velocity FALSE store. Without an explicit dead-arm store, the dead path jumps straight to the
  upright test (measured variant s4). So the dead-arm assignment is January's, even though it restates the initializer.

This match makes no header, config or prototype change. The owner census shows one extra header-inline select-any
COMDAT, `_magnitude_squared3d`: January references it undefined, and the same COMDAT is already emitted by many
build/base AI objects. There is no data, BSS or COMMON change. The fake scan reports 0 leads.

## 4. Experiment matrix (all via `gate.py --source`; siblings stayed at 10 exact unless noted)

| Function | Shape | Size / relocs | Result |
| --- | --- | --- | --- |
| desirable | v0 named dx/dy/dz (base) | 288 / 7 | residual |
| desirable | s3 `distance_squared3d` with external magnitude define | 240 / 8 | out-of-line distance_squared3d |
| desirable | s4 s3 without the define | 272 / 7 sha | layout/ABI still off |
| desirable | va chain, no init, else-FALSE on distance fail | 272 / 7 sha | reverted |
| desirable | **vd** init FALSE + dead-arm FALSE + else-if chain | 272 / 7 | **EXACT, landed** |
| desirable | vh va + init | 272 / 7 sha | reverted |
| desirable | d1 `magnitude_squared3d(vector_from_points3d())` | sha | desirable lost exact, reverted |
| find_destination | f1 MAX / early marker vector / 3D offset cast / chosen.z | 1248 / 34, frame 0x464 | not landed |
| find_destination | f2 f1 + `0.35f*0.35f` + bsp local first | 1248 / 34 | not landed |
| find_destination | f3 helper args without POINT_FROM_LINE3D define (diagnostic) | guard FAIL (`_point_from_line3d` emitted) | discarded |
| find_destination | f4 f2 + scalarised 3D helper vectors | 1232 / 34 sha, frame 0x460, ABI unchanged | not landed |
| find_destination | f5 destination operand order | no change | discarded |
| evaluate_seat | e0 `#include "units/vehicle_definitions.h"` only | 960 / 31 (reloc name fixed) | not landed alone (no row change) |
| evaluate_seat | e1 2D helpers on 3D storage (casts, diagnostic) | 976 / 32 | discarded |
| evaluate_seat | e2 e1 + single-exit nesting | 976 / 32 | discarded |
| evaluate_seat | e3 e2 + explicit prop-loop distance | 992 / 31 | only site-2 `normalize2d` inlined vs January call |
| evaluate_seat | e4/e5/e6/e8/e3ord variants of e3 | 992 / 31 | site-2 still inline |

## 5. Do-not-repeat list

- **Measurement trap:** a `sed 's/...\r$//'` on this LF file silently kept the magnitude define, and about 25
  "inline budget" probes were built against the wrong base. The real per-call inline boundary in desirable was the
  TU-local define alone. Always `grep -c` the define after generating a variant base.
- desirable: named-local distance forms, `magnitude_squared3d(vector_from_points3d())`, the explicit else-FALSE
  chain, and chain + init + else-FALSE are all measured non-exact.
- find_destination: the `_point_from_line3d` inline/out-of-line split cannot be reproduced without emitting the
  forbidden COMDAT. Destination operand order and bsp-in-arguments are measured inert.
- evaluate_seat: none of these changes the site-2 `normalize2d` inline choice: explicit to_seat vector,
  vector_from_points3d facing, assigned result, conditional context, January function order.
- `_distance3d`: see the 2026-08-27 ledger (two waves exhausted).

## 6. Residual classification

- **find_destination (policy + ABI).** Measured facts:
  - January passes `ecx=actor_index, eax=vehicle_index`.
  - The actor pointer is homed and all body accesses go through one derived pointer.
  - The destination uses the inline `point_from_line3d` body order, while the refine offset calls `_point_from_line3d`.
  - The offset vector is a real_vector3d used as perpendicular2d/normalize2d storage.
  - The approach update is `MAX`, the literal is `0.35f*0.35f`, and the bsp fetch comes first.

  Inferred: the register ABI follows from the body shape, which needs the forbidden COMDAT and a representation cast.
- **evaluate_seat (policy + one inline decision).** Measured facts:
  - The implicit `vehicle_specific_definition_get` call is a missing include.
  - The body is single-exit.
  - The facing is a 3D vector normalized as 2D.
  - The distance is `MIN(distance2d, distance2d)`, and January owns the 2D helper COMDATs.

  Unresolved: VC7 inlines the second `normalize2d` in our build, while January calls it.
- **setup_specific, setup_impromptu, perform (ABI/register).** They are blocked by find_destination's private ABI.
  setup_impromptu also differs in one x87 square order in its inlined desirable copy.
- **_distance3d.** x87 scheduling, exhausted.

## 7. Reopen criteria

- find_destination: an owner ruling that allows a select-any `_point_from_line3d` COMDAT here, or a C spelling that
  produces the out-of-line offset call without it; plus admission of the offset-vector storage cast.
  Start from `scratch/workers/action_vehicle/f/f4.c`.
- evaluate_seat: owner admission of the 2D-helper-on-3D-storage casts, plus evidence for the site-2 `normalize2d`
  inline choice. Start from `scratch/workers/action_vehicle/e/e3.c`, with the `vehicle_definitions.h` include.
- setup_specific, setup_impromptu, perform: re-gate as soon as find_destination matches January's ABI.
- `_distance3d`: the 2026-08-27 criteria still apply.

## 8. Disposition

`action_vehicle.obj` is now 11/17 strict exact (+272 padded bytes: `_action_vehicle_desirable`). It is still active
and not Matching. No function is parked or fuzzy-landed by this lane.

## Orchestrator admission note (checkpoint 1)

`_action_vehicle_desirable` was **not admitted**. The adversarial verifier rejected the worker's strict-exact form because its dead-vehicle arm re-stores `desirable = FALSE` after the identical initializer, which is a redundant store. The orchestrator also measured the single-assignment form (no initializer, explicit `else desirable = FALSE`): 10 exact / 7 residual, so `desirable` is not exact. `action_vehicle.c` was reverted to HEAD. The `_point_from_line3d` emission and 2D representation-cast proposals are declined under the lane rules.
