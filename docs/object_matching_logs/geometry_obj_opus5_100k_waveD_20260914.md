# Geometry: Opus5 100K wave D park re-investigation (2026-09-14)

## Scope and provenance

- Translation unit `source/math/geometry.c`, January target `build/split/source/math/geometry.obj`.
- Compiler: XDK 3911 VC7 13.00.9254, repository flags (unchanged), measured through
  `tools/campaign/gate.py --source` whole-TU compiles only. No header, config, ninja or configure use.
- Lane `C:\halo-worktrees\opus5-100k-consolidated-20260914`, wave D (park re-investigation, brief sections 9-10).
- Evidence used: January split object (authoritative bytes, relocations, assert line anchors);
  HCEA research cache `research-cache/halocea-full-blobs-20260830/src/blam/{math,networking,structures}/`
  (Xbox 360 build, parameter order and topology only); Stian `_research-stian-halo/src/halo/math/geometry.c`
  (PC build 2276 VC71 lift); Bungie `haloleak2024/halo1/random_math.c` (already cited by the lane's
  random_math reconciliation ledger); prior Geometry ledgers listed per function below.
- Worker notes with every shape: `scratch/workersD/geometry.md`; candidates and objects in `scratch/workersD/geometry/`.

## Validated baseline (lane HEAD, real file)

`== exact 18  residual 12  unwritten 0  (of 30 listed)`; all 12 residuals are PARKED/LOCKED.
The real file is unchanged by this wave (`git diff --stat -- source/math/geometry.c` empty); final real-file
gate identical to baseline, park drift PASS, owner census vs `build/base` clean, fake scan 0 leads,
`_point_from_line3d` guard passes. No non-parked residual exists, so nothing was landed.

## Park-reopen results (scratch only, strict EXACT under whole-TU gate)

| Function | Target size / relocs / normalized SHA-256 | Candidate | Evidence that closes it |
| --- | --- | --- | --- |
| `_convex_hull2d_perimeter` | 96 / 0 / `71292feb41a8e5d5...` | `perimeter_only.c` | closing edge through `distance2d` proves VC7 evaluates the right operand of an equal-weight sum first; January's loop squares x first, so the loop text is `edge.j*edge.j + edge.i*edge.i` (Stian's independent VC71 lift documents the same x-first-closing / y-first-loop asymmetry) |
| `_calculate_vertex` | 336 / 18 / `5bc4eb4df60182b5...` | `geosphere_family.c` | HCEA parameter order `(sphere, new_vertex, parent1, parent2, subdivision_count, subdivision_index)` (register-passed 360 ABI) fixes January's private ABI (new_vertex on the stack, parent1 in bx); body stores into `sphere->vertices[new_vertex]` then `normalize3d((real_vector3d *)&sphere->vertices[new_vertex])`, frame 8, the Bungie idiom attested by `random_math.c:183` |
| `_get_face_vertex` | 400 / 18 / `6b9b37ec87ba359a...` | `geosphere_family.c` | unchanged body; closes through the calculate_vertex ABI cascade |
| `_subdivide_triangle` | 944 / 52 / `f2e96f710a5b6398...` | `geosphere_family.c` | unchanged body; closes through the same cascade |
| `_build_torus` | 768 / 18 / `6460f3c33d8f750a...` | `torus_only.c` | January rotates the point in place through `(real_vector3d *)points`, copies a per-ring `real_point3d cylinder_point`, adds `radial` with a folded zero k (z self-copy), uses short counters (HCEA `int16_t`), zeroes triangle_strip_count first, and evaluates `cosine(ring_angle)`/`sine(ring_angle)` at their use sites |

`all_reopen.c` combines every change: 23 exact / 7 residual, all 18 inherited exact functions preserved.
Candidate SHA-256: all_reopen `50b7b498...b6fb`, perimeter_only `d13d1e90...cb22`,
geosphere_family `352b83c4...6132`, torus_only `e2ac30f0...a1eb`.

Policy flags for the owner: the geosphere and torus closures need a point-to-vector pointer cast. The brief lists
representation casts as exhausted; the new fact is Bungie's own `random_math.c` using exactly this cast on the
same geosphere vertex storage (the lane already admitted that cast in `source/math/random_math.c:176`), and HCEA
shows the same in-place rotate. The torus form calls `cosine`/`sine` twice per ring (VC7 CSEs the intrinsics).
The torus form also emits a natural `_add_vectors3d` COMDAT (selection ANY), the same systemic header-inline
helper class as the 29 target-absent helper COMDATs Geometry already emits; `actor_combat.obj` owns a
NODUPLICATES copy, the same whole-object veto class already recorded for the ActionCharge helpers.
`_get_edge_vertex` drifts in every geosphere candidate (its call site must change) and needs a new park base.

## Experiment matrix (not retained in production)

| Function | Shape | Result |
| --- | --- | --- |
| perimeter | named edge + `magnitude2d` | residual, same as base |
| perimeter | `square_root(edge.j*edge.j + edge.i*edge.i)` | EXACT |
| calculate_vertex | donor cast body, old parameter order | only parent1/new_vertex stack-vs-register swap |
| calculate_vertex | cast body + HCEA order | EXACT (+ get_face_vertex, subdivide_triangle) |
| calculate_vertex | typed local vector + HCEA order | not exact (get_face_vertex grows to 416) |
| get_edge_vertex | `MIN`/`MAX` va/vb, reversed computed after | 400/17 `b9dc8bcc...`, 24 differing target insns (base 96) |
| get_edge_vertex | single ternary return / `long offset` local | 416 size but eax/ecx swap throughout |
| geosphere_new | separate fill index; hoist block locals; hoist vertex_subdivision_indices | all inert |
| build_torus | cylinder_point + in-place cast rotate + add_vectors3d | 752, closer |
| build_torus | + short counters | 768, only zero-pin order and fcos/fsin schedule |
| build_torus | + strip count declared first | only fcos/fsin schedule |
| build_torus | sine first; scalar radius adds | worse (schedule / frame 0x48) |
| build_torus | cosine/sine at use sites | EXACT |
| convex_hull3d_begin | hoist offset/projection/t/distance; hoist offset; function-scope swap | all inert (frame 0x40 vs 0x4c) |
| polygon3d_clip | `result_count != NONE` arm first (assert 1584 precedes 1591) | block layout identical |
| polygon3d_clip | + single-exit `result_count = 0` dispatch | tail identical, 1120/26 `a37afef4...` |
| polygon3d_clip | `scale*vector.i`; explicit clamp chain | inert / worse |
| polygon2d_clip | `clip_flags ? TEST_FLAG(...) : FALSE`; PIN passed as `t` | worse / no gain |

## Do-not-repeat

- Typed local-vector repairs of calculate_vertex and build_torus: measured non-exact; the cast is what January compiled.
- geosphere_new scope hoists and fill-index splits; convex_hull3d_begin scope hoists and swap scope.
- polygon3d scale operand order and explicit clamp; polygon2d ternary SET_FLAG value and PIN-as-argument.

## Residual classification and reopen criteria

- `_get_edge_vertex` (scheduling/width): the two returns use movsx plus a 16-bit `add ax,cx; dec eax`.
  Reopen with a source form that makes the sum short-typed while the offset stays int, backed by evidence.
- `_geosphere_new` (loop induction): January keeps vertex_index as a live short IV with a separate source
  pointer and a 0x10 frame. Reopen with evidence of that copy-loop form; the triangles relocation targets
  `_geosphere_primitive_vertices+74` because config has no interior label at `.rdata 0x27A300`.
- `_convex_hull3d_begin` (frame): 12-byte unreferenced hole. Reopen only with evidence of the owning local.
- `_convex_polygon3d_clip_to_plane` (scheduling): the `q2` tail is structurally proven; the in-loop distance term
  order, the fsubr load in the duplicate test and the PIN join duplicate remain.
- `_convex_polygon2d_clip_to_plane` (scheduling): memory-homed in_front, early previous_point home, PIN join duplicate.
- `_convex_hull2d` (register allocation) and `_convex_hull3d_expand` (schedule; prior UB-closer form rejected):
  no applicable new law. Not re-spent.

## Disposition

Real file unchanged; zero exact credit this wave. Five park-reopen proposals (perimeter; calculate_vertex +
get_face_vertex + subdivide_triangle; build_torus) are handed to the orchestrator with the candidates above.
All other Geometry parks stay as they are.
