# Interrupted Opus `leaf_map` / `bipeds` re-audit, 2026-09-04

## Scope and provenance

- Canonical audit base: `fc057d673a3f0e3a8a782bb4ec8ae8ea8824f5ea`.
- Read-only donor worktree:
  `C:\halo-worktrees\opus-small-families-30k-20260902` at
  `eeedd72e5ba0fc1761519f68cad241fe606320e7`.
- The donor had interrupted, uncommitted edits in only the two files audited
  here: `source/structures/leaf_map.c` and `source/units/bipeds.c`.
- This packet was reconstructed in an isolated worktree. Neither canonical nor
  the donor was modified.
- January PC COFF is the match authority. HCEA, Marathon, local history, and the
  donor session notes were used only as corroborating source evidence.

The audit read the campaign throughput and Opus reconciliation ledgers, every
tracked `leaf_map.obj` and `bipeds.obj` object ledger, the limp-body checkpoint,
the prior Opus small-unit admission, and both interrupted donor reports before
adjudicating any source. No applicable `CLAUDE.md` or `AGENTS.md` exists in the
canonical or donor exact-match tree.

## Admitted exact packet

Only two independently separable functions survived the audit. Both are
ordinary, naturally typed source and select no forbidden helper COMDAT.

| Object / function | Base | Admitted | Padded gain | Meaningful gain | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `leaf_map.obj::_leaf_map_family_mark` | residual | exact | 176 B | 169 B | 4 | `db9f2dd651c445ab3a717b88b1a37b0a9415a31bdba7015c01460c4efeb27b30` |
| `bipeds.obj::_biped_snap_facing` | unwritten | exact | 1,360 B | 1,360 B | 58 | `842546aeaaf322b8614b3cafef0d1a048102dcb2bd2b217b6cff514ff2fc1500` |
| **Total** |  | **2 exact** | **1,536 B** | **1,529 B** |  |  |

`leaf_map_family_mark` now chooses the neighboring leaf with the natural
conditional expression authenticated by the target branch and register
topology. This closes the pre-existing residual without changing interfaces or
data ownership.

`biped_snap_facing` is a complete semantic reconstruction using the owned
`biped_get` and `biped_definition_get` accessors, project `real` and vector
types, named flags, and the established math helpers. Its public declaration is
in `bipeds.h`, not a consumer-local prototype. The climbing snap angle retains
the source-level `real` cast needed to reproduce January's double widening of
the single-precision angle. No inline assembly, raw offset, address-derived
name, dummy dependency, aliasing trick, volatile/register coercion, or pragma
was introduced.

## Rejected donor material

- The donor restored a `point_from_line3d` call in
  `leaf_map_leaf_spans_polygon`. It causes the candidate to define a
  `point_from_line3d` COMDAT while January records that symbol as undefined.
  The already-canonical scalar form is therefore retained.
- The donor's sign-bit rewrites in `leaf_map_build_portals_from_leaf` improve
  an already adjudicated 432-byte candidate for the 480-byte target but do not
  close it. They belong to the evidence-exhausted inline-schedule gap and were
  not reopened without new evidence.
- The full donor `leaf_map.c` also conflicts with current type ownership through
  a duplicate `leaf_portal` definition and obsolete local prototypes. No broad
  transplant was attempted.
- `biped_get_sight_position` reaches the target's 416-byte padded size and 19
  relocations only in the form that emits the forbidden `point_from_line3d`
  COMDAT. Scalar expansion avoids the symbol but shrinks the function to 400
  bytes and is less target-close. It was rejected.
- `biped_accelerate` is coherent at 480 bytes and 18 relocations, but remains
  an evidence-exhausted two-instruction x87/cross-product scheduling tie. It
  supplies no new strict gain, so this audit did not reopen or distort it.
- The donor `biped_get_autoaim_pill` uses offsets derived from the current
  over-wide `collision_unused[12]` field. Cross-build evidence suggests a
  ten-byte field; correcting that owner structure yields a 336-byte,
  9-relocation candidate, but it still retains a three-instruction scheduler
  difference. A speculative shared-layout change was not admitted in this
  narrow packet.
- The donor's private `biped_make_footstep`, `biped_update_slipping`, and
  `biped_update_landing` bodies are not selected because their static caller
  graph remains unwritten. Giving them external linkage merely to force
  emission would be fake matching and was refused.
- The remaining donor changes did not produce a newly exact function. Existing
  exact canonical functions were left untouched.

## Verification

- Full configure and `ninja all_source progress semantic_progress`: passed.
- Progress: 882,677 to 884,206 meaningful exact code bytes
  (`+1,529`); 6,069 to 6,071 exact functions; data unchanged at 2,018,310
  bytes; linked objects unchanged at 391/833.
- Stable whole-tree comparison: 8,245 functions on each side; exactly the two
  gains above; `+1,536` strict padded bytes; zero regressions.
- Focused gates: `leaf_map.obj` moved from 7 exact / 4 residual / 14 unwritten
  to 8 / 3 / 14; `bipeds.obj` moved from 25 / 1 / 25 to 26 / 1 / 24.
- Semantic progress: 6,497 functions evaluated, 6,127 semantic exact, 6,144
  accepted, zero unit errors.
- Rebuilt candidate `leaf_map.obj` and `bipeds.obj` contain no
  `point_from_line3d` symbol or definition.
- The Units sentinel remains 189/189. In particular,
  `unit_preprocess_node_orientations` remains exact at 1,920 padded bytes, 87
  relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- `tools/fake_match_scan.py --fail-on-findings`: zero findings in all admitted
  source/header files.
- Park registry: 224 active, zero stale, zero invalid.
- Admission audit: zero new candidates or contradictions; two inherited
  rejections; zero revocations.
- Tooling tests: 261/261 passed.
- `git diff --check`: clean.
