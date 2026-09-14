# `actors.obj` Opus5 100K lane, wave D: park re-investigation (2026-09-14)

## Scope and provenance

- Translation unit `source/ai/actors.c`, target `build/split/source/ai/actors.obj`.
- Lane `C:\halo-worktrees\opus5-100k-consolidated-20260914`, HEAD `548451f6d`.
- Compiler: XDK VC7 13.00.9254 `CL.Exe /nologo /c /O2 /Oy- /DDEBUG /Dxbox` (build.ninja, unchanged).
- Evidence: January split COFF (bytes, relocations, section order); January map atlas records for
  `actors.obj` (names only, extracted to `scratch/workersD/actors/atlas_actors.txt`; no Ghidra export
  exists for this object); HCEA `src/blam/ai/*.c` (behavior and statement order only); punpckhdq
  `source/units/units.h` (prototypes); memory laws named per row.
- Worker notes: `scratch/workersD/actors.md`; scratch candidates `scratch/workersD/actors/`.

## Validated baseline

`gate.py source/ai/actors --all --forbid-emitted-symbol _point_from_line3d`:
68 exact / 8 residual / 0 unwritten of 76 (12,944 exact padded code bytes of 19,680).

| Function | Target size/relocs | Base size/relocs | Park |
| --- | ---: | ---: | --- |
| `_actor_attacking_target` | 176 / 7 | 160 / 7 | no |
| `_actor_unit_control` | 784 / 48 | 784 / 48 | no |
| `_actor_swarm_attach_unit` | 656 / 43 | 656 / 43 | register-allocation |
| `_actor_customize_unit` | 640 / 26 | 672 / 26 | unclassified |
| `_actor_create_for_unit` | 624 / 29 | 624 / 29 | register-allocation |
| `_actors_spawn_from_unit` | 672 / 29 | 672 / 29 | instruction-scheduling |
| `_actor_place` | 800 / 41 | 816 / 41 | unclassified |
| `_actor_input_update` | 2384 / 89 | 2400 / 89 | unclassified |

## Final state (real file)

69 exact / 7 residual / 0 unwritten (13,120 exact padded bytes, +176). Point guard passes, all six parks
keep their `measurements.base` hashes, owner census shows no candidate-only owner, fake-match scan 0
leads. Only `actor_attacking_target` changed in `source/ai/actors.c`.

## Accepted control

### `_actor_attacking_target` (strict EXACT, landed)

January emits two separate success epilogues (`mov al,1`) whose copy temporaries use different
registers; our early-return form let VC7 merge both vector copies behind one block (160 bytes). With the
campaign's single-exit law the function carries `boolean result = FALSE`, and each arm assigns
`result = TRUE` *before* its struct copy, returning `result` once. That ordering keeps the arms distinct
and return-block specialisation restores both epilogues: 176/7, normalized sha `fe4a0a53ab6cdd1b`.
The later `else if` is required semantically by the single-exit form.

## Experiment matrix

| Function | Shape | Result | Kept |
| --- | --- | --- | --- |
| attacking_target | single exit, copy then `result = TRUE` | 160, base sha | no |
| attacking_target | `else if` with returns | 160, base sha | no |
| attacking_target | single exit, `result = TRUE` then copy | 176/7 EXACT | **yes** |
| attacking_target | second copy componentwise | 192 | no |
| unit_control | probe prototype `short animation_impulse` (scratch rename) | code identical to January | proposal |
| unit_control | same with `word` | identical (short chosen, see below) | - |
| unit_control | overlay `units.h` (two prototypes short) | EXACT, 70/6, no park drift | proposal |
| swarm_attach_unit | NONE initialiser in the attach block + single exit | 656/43 EXACT, 0 hunks | park-reopen proposal |
| customize_unit | equipment test inverted (HCEA order) | base sha | no |
| customize_unit | block locals at function scope | base sha | no |
| customize_unit | HCEA `flags` local for camouflage | base sha | no |
| place | definition-only `short noncombat_sequence_id` | inert | no |
| place | overlay actors.h + definition short | 816, new sha, January push form reproduced | no (drift) |
| place | flattened inner block | same as previous | no |
| place | single exit | 816, pins -1 in ebx (January does not) | no |
| create_for_unit | single exit with `valid` flag | NONE pinned, but in edi (January esi) | no |
| create_for_unit | HCEA early return + `can_create` flag | same | no |
| create_for_unit | flag declared first | same | no |
| input_update | migrate flag via `&&` + `migrate = FALSE` | 2384 (size equal) | scratch |
| input_update | + store order gunner/bombardment/driver type/flying, flags read after driver store, facing call if/else | 69 hunks | scratch |
| input_update | + no `actor_encounter_index` local | 27 hunks (from 53) | scratch best `viu3.c` |
| input_update | + `short component_index` | 29 hunks | no |
| spawn_from_unit | single exit | `xor edi,edi` pin and single `mov ax,di` epilogue reproduced | scratch |
| spawn_from_unit | + actor-first branch, `position.z += 0.3f` before the push-out, launch via `scale_vector3d` | 207/207 insns, 18 hunks | scratch |
| spawn_from_unit | + `for` loop (HCEA) | 11 hunks, 688 (alignment only) | scratch best `vsfu_c.c` |
| spawn_from_unit | function-scope declarations / scale forward / vertical first | inert | no |

## Proposals (orchestrator)

1. **park-reopen `_actor_swarm_attach_unit`**: candidate
   `scratch/workersD/actors/park_reopen_actor_swarm_attach_unit.c`. January stores
   `component_index = NONE` after the swarm-actor test (store/call-order law) and its datum_new failure
   exit returns the live `succeeded` slot (`mov al,[ebp-1]`), which is the single-exit law. Both laws
   postdate the park. Gate 70/6 on top of the landed attacking_target; other parks unchanged.
2. **header `units.h` + `units.c` (protected)**: `unit_test_animation_impulse` and
   `unit_start_animation_impulse` take `short animation_impulse`; static `code_00198fd0`
   (atlas `unit_can_play_animation_impulse`) takes `short animation_impulse`, and its two
   `(short)animation_impulse` casts go. Evidence: every January caller (actors, actions dive,
   ai_communication event) pushes a zero-extended 16-bit load; punpckhdq units.h:528-529; HCEA int16_t.
   Scratch units.c with all three changed keeps both definitions instruction-identical; actors
   `_actor_unit_control` becomes EXACT; actions `_actor_action_try_to_dive` goes 704 to 720 (size-equal).
   66 direct consumers (`scratch/workersD/actors/units_h_consumers.txt`); type-only change.

## Do-not-repeat

- attacking_target: early-return spellings, componentwise copies, pointer or memcpy copies.
- customize_unit: condition inversion, declaration scope, flags local (all hash-identical).
- create_for_unit: flag placement and declaration order under single exit.
- place: flattened scope; single exit (introduces a pin).
- input_update: short loop index.
- spawn_from_unit: declaration scope, scale_vector3d of the forward vector, vertical-first launch.

## Residual classification

- `_actor_unit_control`: ABI/prototype (header type), proven above; not a tie.
- `_actor_customize_unit`: callee-saved priority among variant, unit_index and unit (inference: allocator
  order). The frame is identical.
- `_actor_place`: allocation of starting_location, actor_definition and unit_index (frame +4 from the
  extra unit_index home). Measured fact: January's 12th `actor_create_for_unit` argument is converted
  char to 16-bit, so its prototype parameter is `short`.
- `_actor_create_for_unit`: register choice for the pinned NONE web (esi vs edi) plus ebx shrink wrap.
- `_actor_input_update`: unit pointer slot/register around the threat and aiming blocks, one swarm-center
  store scheduled after the loop guard, and x87 operand order in two inlined cross products. Structural
  statement-order defects are fixed in `viu3.c`.
- `_actors_spawn_from_unit`: encounter/squad held in registers across the join, plus launch x87 operand
  order. Structural defects are fixed in `vsfu_c.c`.

## Reopen criteria

- Parks stay as recorded, except for proposal 1. Future waves on input_update or spawn_from_unit should
  start from the listed scratch bodies. Every change there is a January byte-proven statement-order or
  control-flow fact.
- place: re-measure if `actors.h` changes `actor_create_for_unit`'s last parameter to `short`.

## Disposition

Object still active (69/76 strict). No fuzzy credit taken.
