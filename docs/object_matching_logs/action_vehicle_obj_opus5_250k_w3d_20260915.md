# action_vehicle.obj - opus5 250K house-clean lane, wave w3d (2026-09-15)

Unit: `source/ai/action_vehicle.c`. Worker notes: `scratch/workers/w3d_action_vehicle.md`.
Scratch: `scratch/workers/w3d_action_vehicle/`. Predecessors: the 11 earlier
`action_vehicle_obj_*` ledgers, in particular `_obj_opus5_100k_20260914.md` and `_obj_opus5_150k_w1_20260914.md`.

## Baseline and result

| | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| HEAD (baseline, guard on) | 12 | 5 | 0 |
| after w3d | 13 | 4 | 0 |

Newly strict EXACT: `_action_vehicle_evaluate_seat` - 976 padded bytes, 30 relocations, sha 9f67690555e799a6.
No parks for this unit; no baseline-exact row changed; `_point_from_line3d` emitted-symbol guard passed.

Remaining non-exact rows: `_action_vehicle_desirable` 272 [size 288, sha], `_action_vehicle_find_destination`
1232 [size 1248, sha], `_action_vehicle_setup_impromptu` 464 [sha], `_distance3d` 64 [sha].

## 1. What unblocked the object

`scratch/w3/laws_w3.md` section B lists, under "Newly ADMITTED by verifiers (do not avoid)":
"(real_point2d const *)&point3d casts into 2D helpers (32 source sites, HCEA); A15 role order for distance2d",
landed in wave w3a as `_actor_move_avoidance_setup` (see `actor_moving_obj_opus5_150k_w3_20260914.md` line 77).
Wave w1 had already produced a strict-EXACT `_action_vehicle_evaluate_seat` body and withheld it for exactly that
policy. The 2026-09-15 owner rulings (brief sections 9b/9c) do not touch the cast class, so no new ruling was needed.

## 2. `_action_vehicle_evaluate_seat` - EXACT, landed

Target 976 / 30 / 9f67690555e799a6; HEAD was 960 / 31 / sha.

Structural census before the work:
- One relocation too many (31 vs 30). January calls `_normalize2d` twice (@0x00c1 for the entry facing, @0x0283
  for the to-seat direction) and `_tag_get` twice; ours inlined the second normalize2d and emitted an extra
  `_tag_get` produced by the C4013 implicit declaration of `vehicle_specific_definition_get` at line 471
  (`scratch/gate4/c4013.txt`).
- 16 bytes short (960 vs 976) from the same two effects plus the frame.

The landed body combines five measured levers:
1. **A7 missing prototype.** `#include "units/vehicle_definitions.h"` - the genuine owner, which defines
   `vehicle_specific_definition_get` as a macro over `tag_get` at vehicle_definitions.h:30. No consumer-local
   prototype (R8). This also removes the paired C4047 "differs in levels of indirection" warning.
2. **Admitted 2D-helper casts.** `normalize2d((real_vector2d *)&facing)`,
   `distance2d((real_point2d const *)&actor->input.position.body_position, (real_point2d const *)&entrance_point)`,
   the MIN of that and the seat distance, `vector_from_points2d(... , &to_seat_direction)` and
   `dot_product2d(&to_seat_direction, (real_vector2d const *)&actor->input.facing_vector)`. `const` is used wherever
   the real_math.h parameter is `const`, matching the admitted w3a idiom.
3. **A23-class loop spelling.** `while (prop = prop_iterator_next(&iterator))`. A rotated
   `for (prop = next; prop; prop = next)` inflates the estimated frequency of the code after the loop and inlines
   the second `normalize2d`; the while form leaves it out of line exactly like January. The idiom is already in the
   tree at actions.c:4454, action_obey.c:1826 and actor_looking.c:734.
4. **Sibling scope.** The prop iterator and `prop` live in their own block around the loop, so VC7 shares their
   8-byte slot with `to_seat_direction` ([ebp-0x18]) and the frame drops 0x5c -> 0x54.
5. **A3/A29 single exit.** `result = TRUE` after the entrance-point success, `result = FALSE; break;` for the
   conflicting actor, the whole publish tail under `if (result)`, one `return result;`. This reproduces January's
   three epilogues: the break threads to `xor al,al`, the success path tail-duplicates `mov al,1`, the failures
   share `mov al,bl`.
6. **A17 discarded validation call.** The `if (!allow_any_seat)` block is the bare statement
   `vehicle_specific_definition_get(vehicle->definition_index);` followed by
   `unit_seat_is_driver(vehicle_index, seat_index);`. The earlier candidate used an unused local plus a `(void)`
   cast, which A17 rejects.

Shapes: E1 (the w1 body, re-gated at this HEAD) EXACT; v1 = E1 plus the const casts and the A17 bare statement,
EXACT - landed.

Checks: all 12 baseline-EXACT rows preserved (row-by-row `--all` diff); guard passed; `scratch/parkcheck.py` reports
no parks for the unit; owner census versus `build/base` adds only `_dot_product2d`, a real_math.h select-any
header-inline COMDAT of the class laws_w2 A4 accepts as systemic (`_vector_from_points3d`, `_magnitude_squared3d`,
`_normalize2d`, `_square_root`, `_scale_vector2d`, `_vector_from_points2d`, `_magnitude2d`, `_magnitude_squared2d`,
`_distance_squared3d` are already candidate-only owners at HEAD, so R14 is not newly triggered); no data, BSS or
COMMON change; `tools/fake_match_scan.py` 0 leads (HEAD 0); `CL /Zs /W3` removes C4013 and C4047 and adds nothing.

Authenticity: the routine computes the seat entry facing from entrance to seat in the XY plane, falls back to the
actor own facing vector when that degenerates, takes the nearer of the entrance and seat 2D distances, rejects the
seat when a friendly actor is already closer to the same seat, and publishes weight / range / facing / fake flags
only for an accepted seat. Every helper used is the real_math.h routine the arithmetic is named after.

## 3. `_action_vehicle_find_destination` - structural gap closed in scratch, not landed

Target 1232 / 34 / 698d66d07e83af5b; HEAD 1248 / 34, frame 0x468 against the January 0x464.

Census (recorded before any shape):
- **Relocation census by target**: 34 versus 34, every symbol count equal, with ONE real difference -
  January references `__real@3dfae147` where we emit `__real@3dfae148`. `0x3dfae147` is `(float)(0.35f*0.35f)`,
  the square of the float 0.35f; `0x3dfae148` is the decimal literal `0.1225f`. The HEAD source writes the folded
  literal; January wrote the product. This is an emitted-constant fidelity defect, not a tie.
- **Relocation ORDER**: January calls `_global_collision_bsp_get` at @0x40f, BEFORE the `_global_up3d` read at
  @0x418; ours calls it at @0x47e just before `_collision_bsp_test_vector`. By the store/call-order law the BSP
  pointer is obtained before the test_point and down_vector statements, i.e. it is a block-top declaration
  `struct collision_bsp *collision_bsp = global_collision_bsp_get();`, not an inline argument.
- **Frame census** (`slots.py` over `alndiff --include-equal`): January `sub esp,0x464`, ours `sub esp,0x468`.
  Ours owns one extra dword `[ebp-0x50]` (3 references, all in the trailing collision block) which pushes the
  collision result buffer from `[ebp-0x464]` to `[ebp-0x468]`. January packs BOTH trailing aggregates into slots
  the earlier block already owns (test_point -> [ebp-0x10..-8], down_vector -> [ebp-0x24..-0x1c]); ours packs only
  down_vector. laws_w2 A6 side law: nested-block locals may pack.
- **Statement order from the aligned stream**: January computes `marker.x - actor_position->x` and
  `marker.y - actor_position->y` at @0x116/@0x130, i.e. in the OUTER block before `anchor_point` is assigned, and
  reuses those two cells in the `t` numerator and the perpendicular sign test - a named outer-block 2D vector is
  missing from HEAD. The tail is single-exit (`test al,al; je <fail>` with the success block inline, `mov al,1`
  and a separate `xor al,al` epilogue). `*surface_index_reference` is stored FIRST, its load interleaved into the
  x destination product.

All five defects are implemented by the w1 diagnostic body `x5.c`, which the cast admission now makes usable. On
top of it this wave spent five evidence-led shapes:

| shape | change | result |
| --- | --- | --- |
| s1 | the two hand-written 2D sign dots become `dot_product2d(...)` calls (A35(1)+(2)) | 411 vs 412 insns; also closed the point_from_line2d x87 CSE pop and the magnitude_squared3d k,j,i sum order. KEPT |
| s2 | `collision.t*down_vector.i + test_point.x` (t-first) | byte identical, inert. REVERTED |
| s3 | `scale_vector3d(global_down3d, 4.0f, &down_vector);` for the three component multiplies (A35(1) leaf age) | 412 vs 412 instructions at IDENTICAL addresses; closed the stray `fxch st(1)` and the Y/Z destination products. KEPT |
| s4 | surface_index store moved after the destination stores | worse. REVERTED |
| s5 | dot-2 argument swap | byte identical - confirms laws_w3 A44. REVERTED |

Fixed point `scratch/workers/w3d_action_vehicle/fd_cand.c` (the current real file with only find_destination
replaced): 1232 / 34, `[sha]` only - the `[size]` tag is gone - 412/412 instructions at identical addresses,
whole-TU `== exact 13 residual 4`, guard passed, fakescan 0 leads, no new warnings, owner census +`_dot_product2d`
and +`_scale_vector3d` (both A30/A4 accepted class) and the `.rdata` literal corrected to `__real@3dfae147`.

Eleven instructions remain, in four x87 leaf-age / scheduling families: the i-term of the first inlined
`dot_product2d` (@0x284), both terms of the second (@0x36e, @0x377), the interleave of the
`debug->field_F8 = chosen_point` loads against the `debug->field_F4 = TRUE` byte store (@0x3dd), and the X
destination product (@0x483/@0x48c; Y and Z now match).

NOT LANDED under brief section 6: an already-written residual may be landed only when it becomes strict EXACT, and
this candidate is not below the January size or relocation count, so the fuzzy allowance does not apply.

Reopen criterion: a source lever that flips x87 leaf age inside an inlined `dot_product2d` or a named-local product
without an R3 operand swap (A35 is diagnostic only today); or an orchestrator decision to take the strictly-closer
zero-credit body for its proven fidelity content - notably the emitted constant, where HEAD writes `0.1225f`
(`__real@3dfae148`) and January writes `0.35f*0.35f` (`__real@3dfae147`).

## 4. `_action_vehicle_desirable` - initializer proven, blocked by the pending A42 ruling

Target 272 / 7 / e3b1ecf64be4fb82; HEAD 288 / 7 (100 versus 112 instructions).

Census: relocations 7/7 with identical targets; frame slots identical apart from one extra `[ebp-8]` read. The
16-byte gap is therefore not a frame gap: it is our two extra reloads of the datum homed at `[ebp-8]` plus the
position of `xor bl,bl`.

New evidence this wave: laws_w3 A38/A42 fire. January zeroes BL at 0x2b, before the `test al,4` dead-bit compare at
0x2d, which A42 reads as proof of a declaration initializer. Shape `d4` - `boolean desirable = FALSE;` at the
declaration plus `if (!TEST_FLAG(vehicle->object.damage_flags, _object_dead_bit)) { if (scripted_request) ... else
... }`, with no redundant store anywhere - reproduces that hoisted `xor bl,bl` exactly and brings the row to
**272 bytes, the January size**, at 101 versus 100 instructions. So the initializer, not the w1 `vd` redundant
dead-arm store, is what the January entry proves.

Two families still differ in `d4`, and they are coupled:
- January keeps `vehicle_index` in its incoming ESI and the vehicle datum in EDI (pushing only ebx and edi);
  `d4` moves `vehicle_index` into EDI and the vehicle datum into ESI, which flips the private ABI of the static.
  `_action_vehicle_perform` (768) and `_action_vehicle_setup_specific` (336), both exact at HEAD, then fall back to
  `[sha]` (`== exact 11 residual 6`). Losing an exact row is a regression, so `d4` is not landable.
- January lays the not-dead block out of line (`je 0x58`, `jmp 0x31` back); the `d4` `if (!dead) { ... }` body stays
  inline. HEAD's `if (dead) X; else if ... else ...` chain reproduces the January layout, but its first arm cannot
  be empty, so the only spelling carrying BOTH the initializer and the chain is the w1 `vd` redundant store.

Reopen criterion: the pending A42 owner ruling on byte-proven redundant stores (laws_w3 C3 groups
`_encounter_test_rule`, `_extract_sequence`, `_action_alert_next_position` and `_action_obey_command_begin` here).
If granted, `vd` plus this wave's initializer evidence closes desirable at 272, and the w1 ledger records that
`_action_vehicle_setup_impromptu` then has only the inlined x,z,y square order left.

## 5. Not targets this wave

`_action_vehicle_setup_impromptu` (464, [sha]) and `_distance3d` (64, [sha]) were not in the w3d TARGET list and no
shapes were spent. w1 classified setup_impromptu as desirable-dependent (it inlines desirable) plus one x87
square-order tie, and `_distance3d` as exhausted - a caller-independent header COMDAT body whose two frozen waves
and the w1 `g1` rename-removal probe all produce z,x,y or z,y,x against the January x,y,z.
