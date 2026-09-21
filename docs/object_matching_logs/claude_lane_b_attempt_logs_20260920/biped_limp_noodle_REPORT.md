# Lane B worker report: source/units/biped_limp_noodle

## Baseline (python tools/campaign/gate.py source/units/biped_limp_noodle --all)

    EXACT       704  _biped_limp_noodle_adjust_orientations
    EXACT        16  _biped_limp_noodle_get_max_relaxation_iterations
    residual   1968  _biped_limp_noodle_move_relax_and_constrain_positions  [size 1888!=1968, sha]
    residual    256  _biped_limp_noodle_relax_nodes_onto_environment  [size 240!=256, sha]
    residual   1248  _biped_limp_noodle_valid_joint_rotation  [size 1216!=1248, sha]
    EXACT       496  _validate_real_vector3d_axes3
    == exact 3  residual 3  unwritten 0  (of 6 listed)

candidate.c started as a byte-exact copy of the real source (cmp SAME; all CRLF).

## Prior ledgers read

- docs/object_matching_logs/biped_limp_noodle_obj_opus5_150k_w1_20260914.md (j1-j5, m1-m14, rA-rH3)
- docs/object_matching_logs/biped_limp_noodle_obj_opus5_250k_w3c_20260915.md (vp1-3, rw1-2, vm6p)
- docs/object_matching_logs/biped_limp_noodle_obj_opus5_next150_n3a_20260915.md (census, no shapes)
- opus5-throughput-recovery w7_biped_limp_noodle.jsonl (15 + 11 + 2 more attempts)
- config/parked.json entries for all three functions
- prior scratch bodies: C:/halo-worktrees/opus5-150k-house-clean-20260914/scratch/workers/{biped_limp_noodle,w3c_biped_limp_noodle}/

Prior state in one line: valid_joint_rotation is strict EXACT only with inline point_from_line3d
(emits forbidden _point_from_line3d COMDAT); move_relax reaches January's size only with the same
helper; relax_nodes is an allocation swap (biped vs node_index) with 15 shapes spent.

## NEW evidence this lane: HCEX PDB locals (DIA2Dump; compiland path has NO units/ subdirectory:
   ../../../build/x360/SHIP/halo/biped_limp_noodle.obj). Dumps: hcex_ship.txt, hcex_release.txt

- valid_joint_rotation locals: new_vector, old_vector, rotation_axis, parent_inverse,
  parent_parent_inverse, local_space_rotation_axis, local_forward, rotate_to_position,
  new_local_hinge_rotation_vector, new_world_hinge_rotation_vector, world_space_hinge_rotation_axis.
  Params: biped_index, node_index (int), node_matrices, current_world_pos, new_world_pos, moved_nodes_flags (long *).
- move_relax locals: STATIC LOCAL `features` (struct collision_feature_list), moved_nodes_flags long[2],
  node_stack short[0x40], vel, collision (struct collision_plane[0xA]), to_node, new_pos_vec, new_pos,
  collision (struct collision_result), parent_sphere_edge (real_point3d), outside_geometry unsigned char[2],
  movement float[2].
- relax_nodes: STATIC LOCAL `last_positions` real_point3d[0x40].
- adjust_orientations: params biped_index, node_matrices, node_count (int), last_positions; local rotation_axis.

## Attempts

### A0 (probe) static-local globals form  [try_1.c]
HCEX lists `features` / `last_positions` as function-scope STATIC LOCALS. Replaced the
`biped_limp_noodle_globals` struct with two static locals. Gate: byte-identical rows to baseline
(all three residuals unchanged). Code-inert; would need a symbols.json split. NOT adopted.

### Prior bodies re-gated in THIS worktree (copies: prior_vj1.c prior_vj2.c prior_vp1.c prior_vm6p.c prior_vm6.c)
- prior_vj1: valid_joint_rotation 1248 [sha]; others baseline
- prior_vj2: valid_joint_rotation EXACT (== exact 4 residual 2) - but guard FAIL (_point_from_line3d COMDAT)
- prior_vp1: valid_joint_rotation 1248 [sha]
- prior_vm6p: move_relax [size 1952!=1968, sha], vjr 1248 [sha]
- prior_vm6: move_relax 1968 [sha], vjr EXACT (guard FAIL)

### M-a1 move_relax: position init at decl, parent_position assigned after velocity set (on prior_vm6) [try_a1.c]
Result: byte-identical to prior_vm6 (1968 [sha]). Pointer assignment ORDER alone is normalised.

### M-b1 move_relax: matrix-pointer locals + `&node_matrix->position` at uses [try_b1.c]
Evidence: accessor-binding law. Result: WORSE (633 insns; `add edi,esi; add edi,0x28` destructive form).
January's `lea edi,[edi+esi+0x28]` proves real_point3d* locals. Rejected.

### M-c1..c5 move_relax: SCOPE of velocity / segment  [try_c1..c5.c]
Evidence: HCEX PDB lists every local of this function flat (no S_BLOCK scopes) => C89 function-scope
declarations. January's int stream is `mov esi; imul edi; mov0; mov0; movsx eax; imul eax; lea; lea; test`
while ours sinks the two zero stores of set_real_vector3d below the `test`.
- c1 velocity at function scope: zero stores move up before the leas (size 1952 due to pad cascade), still [sha]
- c2 segment at function scope: byte-identical to vm6 (segment scope inert)
- c3 both: same as c1
- c4/c5 velocity at do-body / for-body scope: same as c1
=> velocity's SCOPE is a live lever (previous waves never varied it).

### M-c6 move_relax: velocity at function scope AND pointers assigned after set_real_vector3d [try_c6.c]
Gate (verbatim):
    EXACT       704  _biped_limp_noodle_adjust_orientations
    EXACT        16  _biped_limp_noodle_get_max_relaxation_iterations
    EXACT      1968  _biped_limp_noodle_move_relax_and_constrain_positions
    residual    256  _biped_limp_noodle_relax_nodes_onto_environment  [size 240!=256, sha]
    EXACT      1248  _biped_limp_noodle_valid_joint_rotation
    EXACT       496  _validate_real_vector3d_axes3
    == exact 5  residual 1  unwritten 0  (of 6 listed)
BUT: guard FAIL (FORBIDDEN-EMITTED-SYMBOL _point_from_line3d) and newsyms OWNERSHIP FAIL
(+ _point_from_line3d, + _vector_intersect_plane3d). Saved as exact_needs_owner_ruling_v1.c.
This is NOT candidate.c. It is the NEEDS-OWNER-RULING body.

### M-c7/c8/c9/c7b/c7c: robustness + HCEX-order check of the scope lever (all on top of c6)
- c9  velocity declared before collision_planes: still == exact 5 residual 1
- c8  node_queue ALSO at function scope, order flags/node_queue/velocity/collision_planes (= HCEX PDB order
      moved_nodes_flags, node_stack, vel, collision[0xA]): still == exact 5 residual 1
- c7c c8 + moved_velocity/moved_position at function scope: still exact 5
- c7b c8 + segment at function scope: move_relax back to 1968 [sha] (third vector_from_points3d interleave
      with the model tag_get pushes changes) => segment is INNER scope, velocity is OUTER scope.
- c7  everything at function scope: [sha] (same defect as c7b)
CORRECTION of an earlier note: DIA2Dump prints no Block records for this PDB and the function has two
locals both named `collision`, so the flat listing is NOT proof of function scope. What the PDB order DOES
support: vel is listed BEFORE collision[0xA] (our function-scope collision_planes) and to_node AFTER it,
which is exactly c8's arrangement (function scope: flags,node_stack,vel,planes; inner: to_node,...).
Strip test: scope alone (c1) = not exact; late pointer assignment alone (a1 / prior m7) = not exact; both
together = exact. Neither is a decoration (no cast/paren/temp/dummy); both are plain C89 declaration style.
Saved: exact_needs_owner_ruling.c (= try_c8.c), 5/6 EXACT, guard FAIL + OWNERSHIP FAIL by policy only.

### ADM1/ADM2: ownership-clean admissible body (no point_from_line3d / vector_intersect_plane3d / set_real_point3d)
adm1 = c8 with the three point_from_line3d sites written as the baseline's component form -> newsyms FAIL
       (+_vector_intersect_plane3d). adm2 = adm1 with the push-distance written as the baseline's
       `-plane3d_distance_to_point(&collision.plane, endpoint) / dot_product3d(n, n)`.
adm2 gate (verbatim, with --forbid-emitted-symbol _point_from_line3d):
    == emitted-symbol guard passed (1 forbidden names checked)
    EXACT       704  _biped_limp_noodle_adjust_orientations
    EXACT        16  _biped_limp_noodle_get_max_relaxation_iterations
    residual   1968  _biped_limp_noodle_move_relax_and_constrain_positions  [size 1952!=1968, sha]
    residual    256  _biped_limp_noodle_relax_nodes_onto_environment  [size 240!=256, sha]
    residual   1248  _biped_limp_noodle_valid_joint_rotation  [sha]
    EXACT       496  _validate_real_vector3d_axes3
    == exact 3  residual 3  unwritten 0  (of 6 listed)
newsyms: candidate newly DEFINES 0 symbol(s), no longer defines 0 / OWNERSHIP PASS
unitcheck: gains=0 regressions=0 other=2 (vjr [size 1216] -> [sha]; move_relax [size 1888] -> [size 1952]).
ADOPTED as candidate.c (baseline copy kept as candidate_baseline_copy.c).

### R1 relax_nodes: bind the second object_get to a named local  [try_r1.c / try_r1_min.c / try_r1_full.c]
Evidence: January performs `object_get_and_verify_type(biped_index, -1)` then `add eax,0x1a0` then
object_header_block_get - i.e. a genuine second object_get. All 15 prior shapes kept it as the nested
expression `&object_get(biped_index)->object.node_matrices`; NONE varied that dimension. Accessor-binding
law (January uses both forms per site) + same-codebase EXACT donor: source/objects/objects.c
_object_get_marker_by_name (EXACT 272) spells exactly
`struct object_datum *matrix_object = object_get(object_index); ... object_header_block_get(object_index, &matrix_object->object.node_matrices);`.
Change: `struct object_datum *object = object_get(biped_index);` + `&object->object.node_matrices`.
Result on the BASELINE file (try_r1_min.c), verbatim:
    == emitted-symbol guard passed (1 forbidden names checked)
    EXACT       704  _biped_limp_noodle_adjust_orientations
    EXACT        16  _biped_limp_noodle_get_max_relaxation_iterations
    residual   1968  _biped_limp_noodle_move_relax_and_constrain_positions  [size 1888!=1968, sha]
    EXACT       256  _biped_limp_noodle_relax_nodes_onto_environment
    residual   1248  _biped_limp_noodle_valid_joint_rotation  [size 1216!=1248, sha]
    EXACT       496  _validate_real_vector3d_axes3
    == exact 4  residual 2  unwritten 0  (of 6 listed)
alndiff: target 103 instructions, ours 103 instructions, no normalized instruction differences.
unitcheck: gains=1 regressions=0 other=0. newsyms: 0 new / OWNERSHIP PASS.
Strip test: the change is not a decoration (no cast/paren/macro/temp-for-bytes); the local is the used
object datum. The un-bound spelling (baseline) measures 240/256 [size, sha].
Same change on the owner-ruling body (try_r1_full.c): == exact 6  residual 0  unwritten 0  (whole object code exact).

### FINAL FILE LAYOUT
- candidate.c                        = baseline + R1 only (minimal; 4/6 exact; guard PASS; ownership PASS; no park drift)
- admissible_improved_residuals.c    = adm2 + R1 (4/6 exact; guard/ownership PASS; vjr [sha] 1248, move_relax [size 1952]; drifts 2 parks)
- exact_all6_needs_owner_ruling.c    = c8 + R1 (6/6 EXACT; guard FAIL + OWNERSHIP FAIL: _point_from_line3d, _vector_intersect_plane3d)

## FINAL CHECKS on candidate.c (= baseline + R1; diff is 2 lines in relax_nodes)
gate --all --forbid-emitted-symbol _point_from_line3d (verbatim):
    == emitted-symbol guard passed (1 forbidden names checked)
    EXACT       704  _biped_limp_noodle_adjust_orientations
    EXACT        16  _biped_limp_noodle_get_max_relaxation_iterations
    residual   1968  _biped_limp_noodle_move_relax_and_constrain_positions  [size 1888!=1968, sha]
    EXACT       256  _biped_limp_noodle_relax_nodes_onto_environment
    residual   1248  _biped_limp_noodle_valid_joint_rotation  [size 1216!=1248, sha]
    EXACT       496  _validate_real_vector3d_axes3
    == exact 4  residual 2  unwritten 0  (of 6 listed)
newsyms: candidate newly DEFINES 0 symbol(s), no longer defines 0 / OWNERSHIP PASS
unitcheck: SUMMARY gains=1 regressions=0 other=0
fake_match_scan: 0 review lead(s)
CL /Zs /W3 (w3.py): no C4013; only the header warnings the baseline file also produces.

## NEEDS OWNER RULING: exact_all6_needs_owner_ruling.c  (== exact 6  residual 0  unwritten 0)
valid_joint_rotation (1248) and move_relax (1968) are byte-EXACT once the source calls the real_math.h
inlines January's bytes prove: point_from_line3d x3 (rotate_to_position; ray origin; endpoint push) and
vector_intersect_plane3d x2 (push distance). Both inline fully (no call relocation), but VC7 emits their
COMDAT bodies => guard FAIL (_point_from_line3d) and newsyms FAIL (+_point_from_line3d,
+_vector_intersect_plane3d).
Evidence for the ruling: the BASELINE object already emits 18 helper COMDATs January's split object
lacks (_arccosine _cross_product3d _distance3d _dot_product3d _magnitude3d _magnitude_squared3d
_normalize3d _plane3d_distance_to_point _plane3d_from_point_and_normal _scale_vector3d
_set_real_vector3d _sine _square_root _valid_real _valid_real_normal3d _valid_real_vector3d_axes3
_valid_realcmp _vector_from_points3d). In config/symbols.json every one of them is attributed to an
object EARLIER in link order (e.g. _dot_product3d/_normalize3d/_scale_vector3d/_point_from_line3d in
action_charge @file_offset 5504-5744, _vector_intersect_plane3d in math/geometry @1003536;
biped_limp_noodle is @1633680) - i.e. the linker kept the first COMDAT copy. The two new symbols are the
same class as the 18 tolerated ones.
New lever found this lane for move_relax (closes the "first-node-block schedule" residual that
m7-m14 could not): `velocity` declared at FUNCTION scope (HCEX PDB order: moved_nodes_flags,
node_stack, vel, collision[0xA] precede to_node) + position/parent_position assigned AFTER
set_real_vector3d; `segment` must stay in the inner block (c7b).
