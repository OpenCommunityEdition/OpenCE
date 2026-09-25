# MANIFEST - source/physics/collision_features (critic_1_collision_features)

Status: BLOCKED. The unit is RESERVED for the Codex packet (claim.py returned exit 3, holder `codex`).
No candidate source was written. The work is a read-only re-validation of the standing admission
rejection, plus config patches prepared to apply together with the Codex packet.

- Base/tip: 5d3ca708 (claude/fifty-objects-r2-20260924), which contains the 7b9de585 named in the task.
  The tree is clean. No tracked file was touched.
- Production changes proposed: config/object_admission_rejections.json only. There is no source,
  header, symbols.json, parked.json, semantic_*.json or config.json change of my own.

## Source evidence (no construct changed)
- Helper body: real_math.h:972 `projection_from_vector3d` (z if |k|>=|j| and |k|>=|i|, else y if
  |j|>=|i|, else x). It landed in 4f4c8502 (2026-09-22) with a stable sweep of +23 exact / 0 lost. Its
  80-byte COMDAT normalized sha 70d35416dc81990b... equals January's selected decals copy, which is the
  hash the 20260904 audit doc itself gives as January's. The old wrong body 09cf2b51... appears in none
  of the 6 base definers (helper_census.txt).
- January topology: the split collision_features.obj holds UNDEF externals for
  _projection_from_vector3d and _projection_sign_from_vector3d. That is the out-of-line-reference class
  (owner ruling 20260921 #1). _cross_product2d is inlined-only in January (0 January referencers board-wide).
  That is the all-inlined class (owner ruling #5, admitted when the caller is strict exact, the copy is
  byte-identical and the full sweep is clean).

## Target vs production per function (fn_table.md; gate_head.txt)
All 20 EXACT under coff_compare.section_infos_equal (padded size / relocs / normalized sha identical):
new 32/1, render_debug_collision_sphere 32/1, render_debug_collision_cylinder 48/1, from_point 304/1,
from_line 832/10, from_polygon 432/15, from_vertex 160/5, from_edge 432/14, from_surface 208/6,
collision_bsp_get_features_in_sphere 416/15, sphere_test_point 208/2, cylinder_test_point 384/4,
prism_test_point 304/3, features_test_point 416/3, sphere_test_vector 384/5, cylinder_test_vector 752/17,
prism_test_vector 656/11, features_test_vector 544/4, render_debug_collision_prism 224/6,
render_debug_collision_features 304/18. gate: `== exact 20 residual 0 unwritten 0`.

## Whole-object audit (section 4)
1. gate --all: 20/20 EXACT (head.obj = production source compiled PID-isolated).
2. object_audit: PASS on build/base and on head.obj (identical output). 30 January-owned sections ok
   (20 .text, 10 .rdata), 30 January symbols with 0 differing.
3. pdb_storage: 30 split symbols, 0 disagreements with cachebeta publics.
4. surplus_identity: 29 candidate-only code COMDATs, 0 not identical. provider_link: 36 of 37 surplus
   rows PASS in both orders. FAIL (1): `_cross_product2d` vs actor_combat (LNK2005 both orders), because
   production actor_combat.c defines a hand-written NODUP copy. It is the only NODUP among the 11 base
   definers (cross_product2d_selection_census.txt). January's split marks every COMDAT NODUP, which is a csplit artifact.
5. Data: report.json 896/896 matched, and all 10 .rdata owners are ok in object_audit. No '$'-defect entry is needed.
6. Rejections: entries[5] (`_projection_from_vector3d`, candidate-only-comdat-owner) is STALE. Its
   stated premise (a reversed x/y choice that differs from the decals copy) is false at HEAD, and all three reopen
   criteria are met. No park exists for the unit.

## Emulated admission audit (emulate_admission.txt; tools.audit_object_admission.audit, tracked inputs read-only)
| status | rejections | collision_features result |
|---|---|---|
| NonMatching | HEAD | rejected (_projection_from_vector3d) |
| NonMatching | 01D retire | candidate (audit-coff-ownership-before-admission) |
| NonMatching | 01R rewrite | rejected (_cross_product2d) |
| Matching (Codex 02 flip) | HEAD | **completion-label-contradicted** (the gap the task describes) |
| Matching (Codex 02 flip) | 01D retire | accepted (not listed; contradicted 0) |
| Matching (Codex 02 flip) | 01R rewrite | contradicted (_cross_product2d), so the veto holds until 02R |

## Patches (patches/; each `git apply --check` clean at 5d3ca708)
- `01D_retire_collision_features_projection_rejection.patch`: deletes the stale entry. Its content is
  the same as round-1 `research/fifty_objects_20260925/w/owner_queue/p1_cross_product2d/patches/03`
  (which also still applies here). Apply ONLY in the same batch as, or after, the Codex actor_combat repair
  (round-1 01 + 02). The admission audit does not run provider links, so 01D alone would list the
  object as a clean candidate while the _cross_product2d LNK2005 still stands.
  Checked: round-1 01 + 02 + 01D together apply cleanly.
- `01R_rewrite_collision_features_rejection_to_cross_product2d.patch`: standalone and applicable now,
  independent of Codex. It replaces the stale entry with the one true blocker (`_cross_product2d`, same
  class, evidence = owner-queue item 1, reopen = the actor_combat NODUP hand copy removed and provider_link
  PASS in both orders). Validated with tools.object_admission_policy.rejection_index.
- `02R_retire_rewritten_collision_features_rejection_with_codex.patch`: the follow-up to 01R that goes
  with the Codex packet (checked on a sandbox copy after 01R). If 01R has landed, round-1 03/03b no longer
  apply to the collision_features hunk. Use 02R, and for 03b's decals half apply it separately.

## Credit
- Meaningful/padded code: 0 new (all 20 functions were already exact and credited). Data: 0 new.
- Admission: +1 object (collision_features), realised ONLY when the Codex packet lands with 01D (or 01R
  then 02R) and the integrator's full batch gate passes. No credit accrues from this critic alone.
- Surplus helper bytes (the decals-owned 80 bytes, the actor_combat-owned 32 bytes): never credited.

## House-rule review
There is no source change. The rewritten entry names no invented symbols and cites tracked evidence.
Nothing here suppresses emission, alters COMDAT metadata or hand-expands a helper.

## Side observation (not my unit; no patch)
In decals' `_vector_from_points2d` rejection, the premise "January's selected Action Obey copy is
no-duplicates" no longer describes production. action_obey's base copy is now select-any (Layer 2), and
decals' provider_link fails only on `_cross_product2d` (provider_link_decals_nonpass.txt). Decals stays
NonMatching on its function residuals either way.

## Tests run
gate.py (production source), object_audit (base and head), pdb_storage, surplus_identity, provider_link
(collision_features and decals), comdat_sel.py/helper_census.py (own read-only COFF readers), policy
loader validation, emulated admission audit, and git apply --check for every patch and combination. ninja,
configure, pytest and stable_verdicts were not run (integrator-owned).

## Reopen / hand-off
Critic done: the rejection is stale and the patches are ready. The object's completion depends only on the
reserved Codex actor_combat `_cross_product2d` repair. When Codex reports, the integrator applies
Codex 01 + 02 + 01D (or 02R after 01R), then runs the full batch gate.
