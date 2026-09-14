# collisions.obj - opus5 100K consolidated lane, wave C (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/physics/collisions.c`; target `build/split/source/physics/collisions.obj`.
- Compiler XDK 3911 VC7 13.00.9254 (`xbox/bin/vc7/CL.Exe`), repository flags (`/O2 /Oy- /DDEBUG /Dxbox` + include set).
- Authority: January COFF. Names/topology: January map atlas `scratch/atlas/collisions.obj.txt`, HCEA
  `halocea-review/src/blam/physics/{object_test_vector,collision_test_vector_exit,collision_move_point}.c`
  (semantics only), Ghidra `scratch/ghidra/out/collisions.obj.decomp.c`.
- Lane worktree `C:/halo-worktrees/opus5-100k-consolidated-20260914`, HEAD `7a3bbb9c8`; worker notes
  `scratch/workersC/collisions.md`; candidates and the float-argument lab in `scratch/workersC/collisions/`.
- Prior ledgers read: `collisions_obj_opus5_100k_20260914.md` (wave A), `collisions_obj_jonas_vector_exit_first_shot_20260829.md`,
  `collisions_obj_natural_feature_and_fix_pill_packet_20260912.md`.

## 2. Validated baseline and result

| state | exact | residual | unwritten | padded code |
| --- | ---: | ---: | ---: | ---: |
| lane HEAD (baseline) | 17 | 3 | 0 | 11,408 total |
| wave C final (real file unchanged) | 17 | 3 | 0 | 5,808 residual |

Data/code owner sets of the final object are identical to `build/base`. No parked entries exist for this unit.

| residual | padded | relocs | target nsha16 | ours nsha16 | first meaningful divergence |
| --- | ---: | ---: | --- | --- | --- |
| `_collision_move_point` | 4752 | 226 | 8b2f29007193d3aa | 9b2a27828ed3963f | +0x17b loop preheader: January preloads `cx=maximum_collision_count`, `edi=collision_count` |
| `_collision_test_vector_exit` | 400 | 6 | 26cf5d43237cdfad | fb0782bfc733c23e | +0x2e object_index argument load scheduled before `push edi` |
| `_object_test_vector` | 656 | 10 | 4fb51d1cde5c11c8 | bdb238c32ad17c39 | +0x56 radius argument `fld/push ecx/fstp [esp]` vs our `mov/push` |

## 3. Accepted controls

None new (no function became strict exact; `git diff -- source/physics/collisions.c` is empty).

## 4. Experiment matrix (whole-TU gated; all 17 siblings stayed exact in every compile; point guard passed)

| candidate | change | result |
| --- | --- | --- |
| a1 | object_test_vector: `real radius` local at loop-block start | eager slot store, frame 0x488 (reverted) |
| diag_fpu (diagnostic, NOT admissible) | radius as `(real)(x + 0.0)` | 2 real blocks left |
| diag_fpu2 (diagnostic, NOT admissible) | + object_index stored right after material_type (declaration order) | 1 instruction left (fstp placement), nsha d5386c3111d7031e |
| diag_fpu3 (diagnostic) | `-(-x)` | two fchs emitted, 672 bytes |
| x1 | exit: wave-A e4 re-measured | 9 real blocks |
| x2, x3 | exit: add_vectors3d (both argument orders) | identical to x1 |
| x4 | exit: x1 + result copies in collision_result declaration order | 4 real blocks, nsha 85df42a7ac93446c; success block byte-exact |
| x5, x6, x7, x9, x10 | exit x4 + point-first spelling / add_vectors3d / explicit negation / no bare block / COLLISION_POINT_FROM_LINE3D with 1.0f | inert (4 blocks) |
| x8, x11 | exit x4 + named object_index local / found assigned after the initial stores | 17 / 6 blocks (worse) |
| p1 | move_point: named `clip_collision` element pointer in the steepest loop (HCEA `cp`) | 42 blocks (from 43), nsha 5159f89df1e2c676; element walk matches January |
| p2 | p1 + HCEA declaration order in the synthetic-plane block | 4768 bytes (reverted) |
| lab l1-l14 | float-argument passing census (below) | law, no admissible form |

## 5. FPU argument lever - measured law (lab `scratch/workersC/collisions/lab/l*.c`, compiled by `lab.py` with collisions.obj flags)

January census (`fpuscan_jan.txt` vs `fpuscan_base.txt`): plain-memory float arguments routed `fld; push ecx; fstp [esp]`
occur in hs evaluators and D3D `__forceinline` wrappers (both reproduced by our tree), and otherwise only in
fast_vector_intersects_sphere radius arguments (collisions, players, ai, bipeds - every call result tested), dsound
SetPosition (result tested) and lightmap_draw point_from_line3d. `ai_test_line_of_fire` passes the same field
`pills[i].width` via x87 to fast_vector_intersects_sphere and via mov/push to vector_intersects_pill3d.

- **L1**: a named `real` local or inline-parameter temporary defined in the same basic block as the consuming call is
  x87-homed (`fld; push ecx; ...; fstp [esp]` placed after the last argument address computation) only when the call is an
  expression statement. When the call value is used (if-test, assignment, return, global store, switch; boolean or long
  return) the temporary is demoted to a memory home (dead store to a free param slot or a new slot) and integer-copied.
  Argument count/pressure is not the variable.
- **L2**: a direct field, nested member or array-element argument is always integer-copied (`&&`, `?:`, if/else, statement,
  nested struct, stack array, `register` prototype parameter, non-inlined non-static `__inline` callee). `/Op` is inert on this TU.
- **L3**: a double-typed argument expression (named double local, `(real)(x + 0.0)`) is x87 in every context, but its fstp is
  scheduled before the final argument address computation; `x * 1.0` folds in C1 and stays integer.
- **Inference**: January's object_test_vector schedule (fstp after `lea edx,[ebx+0x50]`) is the L1 float-temporary pattern
  with a used call result - a combination not produced by any real-typed spelling measured here. No admissible source form found.

## 6. Do-not-repeat

- Everything in wave A section 5.
- object_test_vector radius: real local at block start or in an inner block, assignment instead of initialisation, const local,
  inline accessor with or without side statements, inline forwarding wrapper (cdecl/stdcall), switch on the result, ternary
  arms, nested member, `register` parameter, long-returning callee, `/Op`, `-(-x)`, `+ 0.0` (diagnostic only).
- collision_test_vector_exit preamble: point/vector operand spelling, add_vectors3d, explicit negation, COLLISION_POINT_FROM_LINE3D
  with 1.0f, bare-block removal, named object_index local, found-after-stores.
- collision_move_point: HCEA declaration order in the synthetic-plane block (size regression).

## 7. Residual classification

- `_object_test_vector`: ABI/argument passing (measured): the only remaining blocker is the x87 route of the radius argument;
  with a diagnostic x87 route plus declaration-order result copies it is one instruction from exact (inference: the x87 value is a
  float temporary, not a double).
- `_collision_test_vector_exit`: scheduling (measured) in the reverse-ray preamble only; the success block is solved by
  declaration-order copies (x4).
- `_collision_move_point`: register allocation (measured): collision_count/maximum live range across the fabs loop header;
  NONE-constant register pinning for the synthetic plane stores; tail element walk solved by p1 (inference: allocator ties).

## 8. Reopen criteria

- object_test_vector (and players/ai/bipeds/dsound residuals): an admissible source form that keeps a float temporary on the x87
  while the call result is used (e.g. a same-compiler exact function showing it), or cross-build evidence of a
  double/arithmetic radius expression. Then apply `diag_fpu2.c`'s store order.
- collision_test_vector_exit: evidence for the preamble argument/store schedule; start from `x4.c`.
- collision_move_point: evidence on the loop live range or constant pinning; start from `p1.c`.

## 9. Disposition

All 17 baseline-exact functions remain exact; the three residuals stay active (no parks). No production source, header or config
changed. New transferable levers recorded: declaration-order result copies (exit, object_test_vector) and the L1/L2/L3
float-argument law.
