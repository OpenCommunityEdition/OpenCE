# collisions.obj - opus5 100K consolidated lane (2026-09-14)

## 1. Scope and provenance

- Translation unit: source/physics/collisions.c; target object build/split/source/physics/collisions.obj.
- Compiler: XDK 3911 VC7 13.00.9254 (xbox/bin/vc7/CL.Exe), repository flags (/O2 /Oy- /DDEBUG /Dxbox ...).
- Authority: January COFF (code, relocations, literals). Names/topology: January map atlas
  scratch/atlas/collisions.obj.txt (including mangled clip_*_to_line signatures), HCEA
  halocea-review/src/blam/physics/*.c (semantics only), Ghidra decomp scratch/ghidra/out/collisions.obj.decomp.c.
- Lane worktree C:/halo-worktrees/opus5-100k-consolidated-20260914, HEAD b8cd50113; worker scratch report
  scratch/workers/collisions.md; candidates scratch/workers/collisions/*.c.

## 2. Validated baseline and result

| state | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| lane HEAD | 9 | 4 | 7 |
| this lane, real file | 12 | 1 | 7 |

Newly strict exact (whole-TU gate, --forbid-emitted-symbol _point_from_line3d passes):

| function | padded | relocs | normalized sha16 |
| --- | ---: | ---: | --- |
| _collision_fix_pill_nudge_collision | 160 | 5 | c3b6224c13285c47 |
| _collision_fix_pill | 672 | 28 | 9fe807e097dac017 |
| _collision_get_features_in_sphere | 512 | 25 | fa6cdc09bc777620 |

Remaining non-exact rows:

| function | padded | state | first divergence / blocker |
| --- | ---: | --- | --- |
| _collision_test_vector_exit | 400 | residual, 6 relocs, sha 26cf5d43 target | object_index load scheduled before origin.x; success-field store permutation (tie) |
| _collision_test_vector | 1744 | unwritten | header naming: structure_fog_plane +0x00 region / +0x02 material (P2) |
| _object_test_vector | 656 | unwritten | emitted only by collision_test_vector; FPU-routed radius argument lever |
| _collision_move_point | 4752 | unwritten | no line_from_planes3d prototype in any header (P1) |
| _clip_position_to_plane / _clip_velocity_to_plane / _clip_position_to_line / _clip_velocity_to_line | 64/64/128/80 | unwritten | statics emitted only by collision_move_point (P1) |

Data: owner set identical to build/base. Code owners gain the header-inline COMDAT _set_real_point3d
(already emitted by 15 build/base objects).

## 3. Accepted controls (retained in production)

- collision_fix_pill_nudge_collision: the contact pointer (&collision->point) is declared inside the
  collision->t > 0 block. Evidence: January saves ESI only inside that block (push at +0x15, pop at +0x6d) and
  receives collision in EAX; the final point copy uses EDI+4. HCEA scopes the pointer identically.
- collision_fix_pill: offset-search locals (collision, fallback, have_fallback, offset_index) live in the else arm,
  candidate in the loop body, the ground vector in the two blocks that use it. Evidence: January frame 0xac88 (ours
  was 0xac94) with the sphere center sharing the ground-vector slot (-0x10); VC7 shares storage only across
  lexically disjoint blocks. The sphere center is built with set_real_point3d(&center, x, y, z): January evaluates
  the z sum before the x/y copies and after the first call pushes (argument evaluation order).
- collision_get_features_in_sphere: the leaf reference is bound to a named long leaf_index before masking with
  LONG_MAX (same idiom as exact collision_test_pill); loop counter renamed leaf_reference_index.

## 4. Experiment matrix (all whole-TU gated; siblings checked every compile)

| candidate | change | result |
| --- | --- | --- |
| f1 | nudge pointer scoped to if-block | nudge EXACT (retained) |
| f2 | fix_pill else-block scoping | frame exact, 25 real diff blocks |
| f3 | declaration order permutation | inert |
| f4 | candidate in loop body, ground vector per using block | 5 blocks (center.z schedule) |
| f6 | center z,x,y statement order | inert |
| f5 | set_real_point3d(&center, ...) | fix_pill EXACT (retained) |
| g1/g2 | named long leaf_index | get_features EXACT (retained) |
| e1-e4 | exit: LONG_MIN, direct object_index arg, negate_vector3d, point macro, store order, add spelling | 24 -> 9 real blocks, not exact (reverted) |
| v1-v12, n1, n2 | collision_test_vector reconstruction | n2 strict EXACT with (short)fog_plane->pad shim (not landed) |
| m1-m15 | collision_move_point + clip helpers | clip helpers EXACT; move_point 4752/226, 43 real blocks (not landed) |
| d1/d2 | object_test_vector radius cast / double local (diagnostic) | double local fixes all call-site registers (not admissible) |
| fpulab, fpulab2 | standalone VC7 lab of float struct-field arguments | only double negation routes through FPU |

## 5. Do-not-repeat

- collision_test_vector_exit: vector-first vs point-first add spellings; success-field store permutations;
  block-start vs direct object_index argument (all measured, tie remains).
- collision_move_point loop: while with negated conjunction, do/while with break, while(TRUE), count in the loop
  condition (either order), steepest index assigned late, collision pointer before the fabs test.
- object_test_vector radius argument: x*1.0f, x+0.0f, same-type or double casts, const parameter, unnamed prototype,
  union field, inline wrapper - all integer-copy.
- REAL_MATH_EXTERNAL_DOT_PRODUCT3D / PLANE3D_DISTANCE_TO_POINT defines for collision_test_vector: they force the fog
  dot product out of line; the natural inlines reproduce January exactly.
- Natural point_from_line3d in collision_test_vector is byte exact but emits _point_from_line3d (guard fail).

## 6. Residual classification

- collision_test_vector_exit: scheduling (measured one load and a store permutation; register allocation otherwise equal).
- collision_test_vector: ownership/naming blocker only (candidate bytes equal January).
- object_test_vector: ABI/argument-passing lever (FPU vs integer copy of a real struct-field argument), shared with
  players.c, ai.c and bipeds.c residuals; plus emission dependency on collision_test_vector.
- collision_move_point: header prototype blocker; residual loop register preload / alignment tie (inference: VC7
  loop-header allocation, not source topology - all calls, asserts, inline choices and relocations match).
- clip helpers: dependency on collision_move_point only.

## 7. Reopen criteria

- P1 (line_from_planes3d prototype in real_math.h) applied: land the move_point packet from
  scratch/workers/collisions/full2.c (clip helpers exact, move_point fuzzy).
- P2 (structure_fog_plane region_index / runtime_material_type) applied: land collision_test_vector (exact) and
  object_test_vector (fuzzy) from full2.c.
- An admissible FPU route for real struct-field arguments (would also reopen players/ai/bipeds residuals).
- New evidence on VC7 loop preheader register preloads for collision_move_point.

## 8. Disposition

Three functions strict exact and retained (1,344 padded bytes). collision_test_vector_exit stays active residual.
Seven unwritten functions are header-blocked with measured candidates; no parked entries were added or touched.

## Header packets P1/P2 (orchestrator, checkpoint 4)

`line_from_planes3d` is declared in the focused owner header `math/real_math_planes.h`. The same declaration in real_math.h regressed two distant exact functions and drifted four parks at any position. The fog plane fields are named `region_index` / `runtime_material_type`. The full2.c body then landed with its shim removed. Full build: collision_test_vector and the four clip helpers are exact (2,080 padded); collision_move_point and object_test_vector stay fuzzy at zero credit; 0 regressions; parks clean.
