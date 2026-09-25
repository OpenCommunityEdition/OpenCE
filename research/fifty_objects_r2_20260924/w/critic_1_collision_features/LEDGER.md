# critic_1_collision_features - LEDGER

Unit: source/physics/collision_features. Tree: 5d3ca708 (descendant of the 7b9de585 named in the task), clean
(only untracked research/fifty_objects_r2_20260924/ at start).

## 0. Claim
`claim.py claim source/physics/collision_features critic_1_collision_features ...` -> exit 3,
"HELD by codex (reserved): Codex packet: actor_combat cross_product2d provider repair + 5 dependents".
Per the task, a held claim means the result is BLOCKED. Everything below is read-only study (the brief
allows reading a reserved unit) plus a prepared config patch. No candidate source was written or compiled.

## 1. Premise check (read-only)
The stale entry (config/object_admission_rejections.json entries[5]) says the surplus
_projection_from_vector3d "reverses the x/y dominant-axis choice and differs from January's selected
decals owner". Its evidence doc (shared_inline_helper_admission_audit_20260904.md) gives the old wrong
hash 09cf2b51... and January's hash 70d35416....

Measured at HEAD:
- `real_math.h:972` body is now `k>=j && k>=i -> _z; else j>=i -> _y; else _x`. The fix landed in
  4f4c8502 (2026-09-22, "Reconcile Lane C with canonical house-rule audit"; its message reports a stable
  sweep of 23 gained, zero lost).
- helper_census.txt: ONE distinct 80-byte body across all 6 base definers (decals, real_math,
  breakable_surfaces, collision_features, leaf_map, structures), normalized sha 70d35416dc81990b...,
  equal to January's decals copy. The wrong 09cf2b51 body no longer exists anywhere on the board.
- surplus_identity: 29 candidate-only code COMDATs, 0 not identical (_projection_from_vector3d vs
  decals IDENTICAL).
- provider_link (build/base): 36 of 37 surplus rows PASS. The exception is `_cross_product2d` vs
  actor_combat (LNK2005 in both orders).
- comdat_sel.txt: January collision_features REFERENCES _projection_from_vector3d and
  _projection_sign_from_vector3d as undefined externals, so this is the out-of-line-reference class
  from owner ruling 20260921 #1 ("header visibility and folded COMDAT emission"). January's split
  lists every COMDAT as NODUP (a csplit artifact; decals' own split copies are NODUP too). In production,
  actor_combat's _cross_product2d is the only NODUP copy among 11 definers (a hand copy), which causes
  the one link failure.
- gate (production source, --out head.obj): 20/20 EXACT. object_audit on base and head: PASS, 30
  January symbols, 0 differ (outputs identical). pdb_storage: 0 disagreements. report.json: 20/20
  functions, 896/896 data.
- The functions the 20260904 header fix regressed in isolation: _decals_delete_permanent_from_cluster is
  100% at HEAD. _populate_statistic_buffer (95.3%) and _bitmap_group_add_bitmap (61%, parked) are not
  exact at HEAD, and 4f4c8502's stable sweep lost zero exact functions. Their current states are not
  losses caused by the header fix.

Verdict on the premise: CONFIRMED. The stated reason is factually false at HEAD. All three reopen
criteria are met: the helper was corrected from January evidence (it is byte-identical to January's
selected copy), the stable sweep shows inherited matches were kept, and ownership comes from ordinary
header topology with provider links PASS in both orders. The single real blocker left for this object
is _cross_product2d, and the Codex actor_combat repair owns that.

Prior art: round 1 already found this (owner queue item 1, line 86: "its rejection is stale") and prepared
research/fifty_objects_20260925/w/owner_queue/p1_cross_product2d/patches/03 (+03b). This critic
re-validates those findings at the current tree.

## 2. Probes (hypothesis -> evidence -> change -> measured)
P1. H: the stale entry is the only thing between the Codex 02 status flip and acceptance. E: I emulated
    tools.audit_object_admission.audit with a report copy that has collision_features metadata.complete=True.
    Change: none (sandbox only). M: HEAD rejections give completion-label-contradicted
    (_projection_from_vector3d); 01D gives accepted. CONFIRMED (emulate_admission.txt).
P2. H: 01D alone, before Codex, would hide a live link failure. M: with NonMatching status, 01D makes the
    object an "audit-coff-ownership-before-admission" candidate. The audit does not run provider links,
    and _cross_product2d still has LNK2005 against production actor_combat. Hence 01D must travel with
    Codex 01+02, and 01R is the honest standalone rewrite.
P3. H: _cross_product2d's NODUP is an actor_combat defect only. M: 1 NODUP (actor_combat) and 10 ANY
    among 11 base definers, all with identical bodies (cross_product2d_selection_census.txt, helper_census.txt).
P4. Patch mechanics: 01D, 01R, round-1 01+02+01D and 01+02+01R all pass `git apply --check` at 5d3ca708.
    01R->02R was checked with --directory on a sandbox copy. The round-1 03/03b still apply here (as
    long as 01R has not landed).
Side: decals' `_vector_from_points2d` rejection premise (NODUP Action Obey) is stale too, since the
    decals provider_link fails only on _cross_product2d. There is no patch because decals has function residuals.

## 3. Outcome
BLOCKED (reserved for Codex). Premise verified. Patches prepared: 01D (with Codex), or 01R (now) then 02R
(with Codex). No claim was acquired, so there is nothing to release.
