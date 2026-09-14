# `collisions.obj` Opus5 150K house-clean lane, wave w2 residual re-investigation (2026-09-14)

## Scope and provenance

- Translation unit `source/physics/collisions.c`. Target `build/split/source/physics/collisions.obj`
  (XDK 3911 VC7 13.00.9254, `/O2 /Oy- /DDEBUG /Dxbox`).
- Lane `opus/150k-house-clean-20260914`, worker `w2:collision_usage_collisions`. **No source, header or config change.**
  Experiments are in `scratch/workers/collision_usage_collisions/{x4.c,co/}`. Notes are in
  `scratch/workers/collision_usage_collisions.md`.
- Read first:
  - `collisions_obj_jonas_vector_exit_first_shot_20260829.md`
  - `collisions_obj_natural_feature_and_fix_pill_packet_20260912.md`
  - `collisions_obj_opus5_100k_20260914.md` and `collisions_obj_opus5_100k_waveC_20260914.md`, including their
    do-not-repeat lists
  - the 100K wave-C worker notes and candidates
  - `ai_obj_opus5_100k_waveD_20260914.md` (paren law)
  - the ai, bipeds and players 150K ledgers (same radius-argument blocker)
  - HCEA `object_test_vector.c` and `collision_test_vector_exit.c`
  - `git log --all -- source/physics/collisions.c`
- Excluded by the orchestrator: `_collision_move_point`. The inherited exact functions were not touched.

## Baseline and final (real file identical)

Gate with the `_point_from_line3d` guard (passed): **17 exact / 3 residual / 0 unwritten**, identical at the end. No parks
for this unit. The owner census against `build/base` and the target is clean, the fake scan has 0 leads, and
`git diff --stat` is empty.

| owner | target padded / relocs / sha16 | outcome |
| --- | --- | --- |
| `_object_test_vector` | 656 / 10 / `4fb51d1cde5c11c8` | owner-blocked; diagnostic strict EXACT in scratch |
| `_collision_test_vector_exit` | 400 / 6 / `26cf5d43237cdfad` | skipped, exhausted (preamble scheduling tie) |
| `_collision_move_point` | 4752 / 226 | excluded |

## `_object_test_vector`: owner-blocked (paren/FPU argument admissibility)

January stages `object->object.bounding_sphere_radius` through the x87 (`fld [ebx+0x5c]; … push ecx; … fstp [esp]` at
+0x56), as the ai, players and bipeds `fast_vector_intersects_sphere` callers do. No header or donor (HCEA, Stian,
punpckhdq) has a macro that parenthesises the argument.

| shape | change | result |
| --- | --- | --- |
| o1 | collision-model arm stores `object_index` right after `material_type` (declaration-order result copies) | residual (radius still integer-copied) |
| o2 (diagnostic, not admissible) | o1 + `(object->object.bounding_sphere_radius)` | **strict EXACT** 656/10. 18 exact / 2 residual, all baseline-exact rows unchanged, guard passed, census clean (`co/o2_diag_paren.c`) |

Not landed: brief section 5 forbids parentheses without a genuine macro. **Reopen:** an owner ruling or an attested
parenthesising spelling for `fast_vector_intersects_sphere` radius arguments. Then apply it here with o1's store order.

## `_collision_test_vector_exit`: skipped, exhausted

The orchestrator's lever (declaration-order result copies) was already applied by 100K wave C (`x4.c`). Re-measured at
this HEAD (`scratch/workers/collision_usage_collisions/x4.c`): 133/133 instructions, success block byte-exact, siblings
exact. Four blocks remain, all in the reverse-ray preamble:

- January loads `previous_collision->object_index` after `push edi; mov edi,vector; fadd [edi]`; ours loads it before
  `push edi`.
- January homes `&instance` in ECX, with `fstp origin.x` between the lea and the push; ours uses EAX.
- January's `origin.y` loads the vector operand first.

C1 normalises operand spelling. Wave C already measured a named local, block scope, statement order and the placement of
`found` (x5-x11). No new evidence exists for VC7's argument-push hoisting, so no shape was spent. **Reopen:** an exact
same-compiler function with call-argument pushes interleaved inside an x87 statement sequence. Start from `x4.c`.

## Proposals

- Owner ruling, shared with ai, players and bipeds: whether an x87-staged radius spelling for
  `fast_vector_intersects_sphere` is admissible. Evidence: `co/o2_diag_paren.c` is strict exact with the one
  parenthesised argument plus the declaration-order store.
