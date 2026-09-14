# `collisions.obj` Opus5 150K house-clean lane, wave w3 second pass (2026-09-14)

## Scope and provenance

- Translation unit `source/physics/collisions.c`. Target `build/split/source/physics/collisions.obj`
  (XDK 3911 VC7 13.00.9254, `/O2 /Oy- /DDEBUG /Dxbox`).
- Lane `opus/150k-house-clean-20260914`, worker `w3:collisions_collision_usage`. **No source, header or config change.**
  Notes are in `scratch/workers/w3_collisions_collision_usage.md`. The call-site census script is
  `scratch/workers/w3_collisions_collision_usage/callscan.py`.
- Read first: all five earlier collisions ledgers (`jonas_vector_exit_first_shot_20260829`, `natural_feature_and_fix_pill_packet_20260912`,
  `opus5_100k_20260914`, `opus5_100k_waveC_20260914`, `opus5_150k_w2_20260914`), the w2 worker notes, `scratch/w2/laws_w2.md` (A1-A22,
  R1-R12, table D), the w3a results (bipeds paren ruling) and `scratch/w3/warn/source_physics_collisions.txt` (no C4013/C4133).
- Orchestrator exclusions: `_collision_move_point` (do not spend) and `_collision_test_vector_exit` (table D, only with a genuinely new law).
  Inherited exact functions were not touched.

## Baseline and final (real file identical)

Gate with the `_point_from_line3d` guard (passed): **17 exact / 3 residual / 0 unwritten**, identical at the end. No parks for this unit.
Owner census against `build/base` and the target is clean, the fake scan has 0 leads, and `git diff --stat` is empty.

| owner | target padded / relocs / nsha16 | outcome |
| --- | --- | --- |
| `_object_test_vector` | 656 / 10 / `4fb51d1cde5c11c8` | owner-blocked (paren/FPU radius argument); 0 shapes |
| `_collision_test_vector_exit` | 400 / 6 / `26cf5d43237cdfad` | skipped, exhausted (table D; no new law); 0 shapes |
| `_collision_move_point` | 4752 / 226 / `8b2f29007193d3aa` | excluded (do not spend) |

## `_object_test_vector`: owner-blocked, with new census evidence

Law detectors run: A7 found no missing prototype in the warning file. A4, A11, A18 and A22 have no construct at the radius argument. The
PAREN law is the only form that reproduces January, and no genuine macro exists. So no shape was spent.

New fact, a scan of every `_fast_vector_intersects_sphere` call site in `build/split`:

| caller object | radius argument in January | route |
| --- | --- | --- |
| ai, players, bipeds, collisions | struct field (`pills[i].width`, `X->object.bounding_sphere_radius`) | x87 (`fld; ...; fstp [esp]`) |
| collision_bsp `collision_surface_test_sphere` | `data->radius` (field through a pointer) | integer copy (`mov edx,[esi+0x10]; push edx`) |
| real_math pill tests (5 sites) | a parameter | integer copy |
| decals (2 sites) | product expression | x87 (natural) |

January itself integer-copies a plain field argument to this callee, so the x87 route does not come from the callee's declaration. A
parenthesising wrapper macro for `fast_vector_intersects_sphere` is refuted. The route belongs to the spelling at each of the four residual
call sites. This narrows the pending owner ruling (laws_w2 C3) but does not unblock it.

**Reopen:** an owner ruling on the parenthesised radius spelling, or an attested per-site spelling. Then apply w2
`scratch/workers/collision_usage_collisions/co/o2_diag_paren.c` (the declaration-order `object_index` store plus the radius).

## `_collision_test_vector_exit`: skipped, exhausted

The w2 x4 candidate (100K wave C) was re-gated at this HEAD: 133/133 instructions, with 4 blocks left, all in the reverse-ray preamble.
Detector pass:

- A4 helper routing: `add_vectors3d`, `negate_vector3d` and the point macro were already measured inert.
- A15: no subtraction role.
- A20: already applied in x4.
- A22 younger-operand: an operand swap is inadmissible (R3), and x5/x6 were inert.
- A6: the frame is equal at 0x448.
- A10, A14, A16: not applicable.

No new law, so 0 shapes. **Reopen:** unchanged. It needs evidence for VC7 argument-push hoisting inside an x87 statement sequence; start
from x4.c.

## Proposals

None for this unit (no header, config or park change).
