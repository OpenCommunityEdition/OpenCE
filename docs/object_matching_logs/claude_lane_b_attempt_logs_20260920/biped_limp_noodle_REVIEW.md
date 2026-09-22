# Adversarial review: source/units/biped_limp_noodle (Lane B)

Reviewer: independent, re-gated from scratch 2026-09-21. No source/header/config/tool edited; no ninja/configure/pytest/git-write run. All scratch output is under `scratch/lane_b/w/biped_limp_noodle/review/`.

## 1. Re-gate (own runs)

    unitcheck:  BASELINE  == exact 3  residual 3  unwritten 0  (of 6 listed)
                CANDIDATE == exact 4  residual 2  unwritten 0  (of 6 listed)
                GAINS: _biped_limp_noodle_relax_nodes_onto_environment residual 256 -> EXACT 256
                SUMMARY gains=1 regressions=0 other=0

gate.py --all on candidate (own run): adjust_orientations EXACT 704, get_max_relaxation_iterations EXACT 16,
validate_real_vector3d_axes3 EXACT 496 (all three baseline-exact functions retained), relax_nodes EXACT 256,
valid_joint_rotation residual [size 1216!=1248, sha] and move_relax residual [size 1888!=1968, sha] (unchanged rows).

Claimed-exact function, confirmed three ways:
- tinfo (January): size=256 relocs=9 sha=19128c623c557550. Candidate object: size=256 relocs=9 sha=19128c623c557550.
- relocdiff --allow-structural --count-by-target: "shape: size 256/256, relocations 9/9, sha equal", 0 differing rows;
  target multiset identical (object_get_and_verify_type x2, tag_get x2, object_header_block_get x1,
  _biped_limp_noodle_globals+44040 x2, move_relax x1, adjust_orientations x1), same offsets and types.
- alndiff: target 103 instructions, ours 103, no normalized instruction differences.

Park drift check (own, stronger than unitcheck's row compare): compiled baseline and candidate to review/base.obj and
review/cand.obj and diffed tinfo for all 24 code sections + data section metadata. The ONLY section that differs is
relax_nodes (240/d1b5db56... -> 256/19128c62...). The two remaining parked functions and every emitted helper COMDAT are
byte-identical to baseline. No park refresh needed for them.

## 2. Guard
`--forbid-emitted-symbol _point_from_line3d`: real source PASS, candidate PASS. No new emission.

## 3. Ownership
newsyms: "candidate newly DEFINES 0 symbol(s), no longer defines 0 / OWNERSHIP PASS".

## 4. fake_match_scan
Candidate: 0 review leads. Real source: 0 review leads. Normalised outputs identical. No new findings.

## 5. Complete diff (1 file, +2 -1, one hunk, inside relax_nodes only)

    +	struct object_datum *object = object_get(biped_index);
         real_matrix4x3 *node_matrices = (real_matrix4x3 *)object_header_block_get(
             biped_index,
    -		&object_get(biped_index)->object.node_matrices);
    +		&object->object.node_matrices);

Byte-level: real 754 CRLF / 0 bare LF, candidate 755 CRLF / 0 bare LF; every other line byte-identical. No header,
config, flag, pragma, volatile/register, cast, paren, macro, raw offset, or baseline-exact function touched. No
unrelated churn.

Hunk assessment:
- Not a dummy local: `object` is the datum whose field address is passed; one definition, one use; the reference count
  of the value is unchanged (it is not an "extra reference to shift priority"), and nothing is stored that was not
  already computed. Semantics identical to baseline on all paths.
- `struct object_datum *object = object_get(index);` is the single most conventional line in this codebase; it is at
  least as natural as nesting the accessor inside a call argument. House rules satisfied (declare-and-initialise, typed
  accessor macro, descriptive name, `real` n/a).
- C4013: the hunk adds no callee and no type that the baseline line did not already use (`object_get`,
  `struct object_datum`), so no implicit declaration can be introduced. (I did not run the worker's w3.py because it
  writes a temp file outside my permitted review/ directory.)
- The redundancy (a second fetch of the same datum already held in `biped`) is January's, not the candidate's:
  January at T 0x33-0x3e does `push -1; push ebx; call _object_get_and_verify_type; add eax,0x1a0` and only then
  `call _object_header_block_get`.

## 6. Strip tests / alternatives (own measurements, files in review/)

| variant | spelling | relax_nodes result |
|---|---|---|
| strip (v0) | nested `&object_get(biped_index)->object.node_matrices` (byte-identical to real source, asserted) | residual [size 240!=256, sha] |
| v1_reuse_biped | `&biped->object.node_matrices` (no second fetch) | residual [relocs 8!=9, sha] - January's second object_get is real |
| v2_helper_call | `object_get_node_matrices(biped_index)` (the real out-of-line helper) | residual [size 224!=256, relocs 8!=9, sha] - January does not call the helper here, so the expansion is forced by the bytes, not a manual-inlining choice |
| v4_rename | local named `matrix_object` | EXACT 256 - result is name-independent (idempotent) |
| candidate | local named `object` | EXACT 256 |

Is the named local a "decoration" that fails the strip test? My ruling: no. It is not a cast, parenthesis, macro or
steering temporary; it is one of the two ordinary spellings of the same statement, and the campaign's own
accessor-binding law (WORKER_BRIEF section 4; "January uses both - read the bytes per site") treats the choice between
them as a per-site source fact to be read from January. Independent corroboration I verified myself:
`source/objects/objects.c` `_object_get_marker_by_name` (gate: EXACT 272; objects board 121/121) has exactly this
shape - a first datum pointer, then `struct object_datum *matrix_object = object_get(object_index);` bound and passed as
`&matrix_object->object.node_matrices` - while `_object_get_node_matrices` (EXACT 48) and `_object_get_node_matrix`
(EXACT 96) use the nested form. Both forms are therefore attested as exact in January's objects.obj neighbourhood. This
also satisfies the park entry's own recorded reopen criterion ("a natural same-compiler donor explaining the pointer
lifetime").

HCEX PDB: lists only `biped_index` and static local `last_positions` for this function (no `biped`, `definition`,
`node_matrices` either), so it is neutral - neither supports nor contradicts a local `object`.

## 7. Semantics vs January
Whole function 103/103 instructions equal, bytes and relocations identical: biped_get (mask 1), two tag_get
('bipd', 'antr'), object_get (mask -1) + 0x1a0, object_header_block_get, `current >= max` via cmp/sbb/inc,
snapshot loop into globals+44040, move_relax, adjust_orientations, CHAR_MAX saturation. No path diverges.

## 8. Residual functions
valid_joint_rotation and move_relax_and_constrain_positions are byte-identical to baseline in candidate.c: NO_CHANGE.
The worker's `exact_all6_needs_owner_ruling.c` is NOT part of this landing. Spot check confirms the worker's own
disclosure: guard FAILS (`_point_from_line3d` emitted, .text 48 bytes) and newsyms reports 2 VIOLATIONS
(`_point_from_line3d`, `_vector_intersect_plane3d`). It must stay out of source until the owner rules; I did not
otherwise review that body and give it no verdict.

## Verdict
- `_biped_limp_noodle_relax_nodes_onto_environment`: ACCEPT_EXACT (+256 padded bytes).
- `_biped_limp_noodle_valid_joint_rotation`: NO_CHANGE.
- `_biped_limp_noodle_move_relax_and_constrain_positions`: NO_CHANGE.
- Overall: LAND candidate.c as is.

Orchestrator actions on landing: remove the `config/parked.json` entry for
`source/units/biped_limp_noodle :: _biped_limp_noodle_relax_nodes_onto_environment` (it becomes EXACT; leaving it would
fail the parked-bytes check). The other two park entries need no refresh (section bytes unchanged). No symbols.json or
header change. Re-verify with the full build + per-function sweep as usual (this review used gate.py only).
