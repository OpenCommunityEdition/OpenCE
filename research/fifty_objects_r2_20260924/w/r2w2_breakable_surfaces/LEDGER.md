# LEDGER r2w2_breakable_surfaces (tree 434f0151 = 05255584 + ledger-only commit; clean except untracked research/fifty_objects_r2_20260924/)

## Claim
- `claim.py claim source/physics/breakable_surfaces r2w2_breakable_surfaces ...` -> exit 3:
  HELD by review-r2w2-breakable_surfaces (claimed 09:55:54): "adversarial review of hygiene_r2w2 breakable_surfaces packet (lab only)".
- claims.log:37/49: r2w2-hygiene-breakable_surfaces already built THIS EXACT sub-packet and released it as
  ADMISSION_FIX_PROPOSED (zero credit) at 09:54:36 -> scratch/w/hygiene_r2w2/breakable_surfaces/PACKET_breakable_surfaces.patch.
- Per brief section 7 (exit 3 = do not probe for landing), no candidate compiled, nothing edited for this unit. Never claimed => nothing to release.

## Read-only cross-check of the existing packet (no compilation by me)
- `git apply --check scratch/w/hygiene_r2w2/breakable_surfaces/PACKET_breakable_surfaces.patch` -> rc 0 at 434f0151; git status unchanged.
- Packet contents == the task's suggested packet: (01) delete TU-local hand copy breakable_surface_get_plane_from_designator,
  call genuine bsp3d_get_plane_from_designator; (02) config.json:520 drop "options" /Ow /QIfist; (03) symbols.json rows 5362
  (_breakable_surface_effect) and 23116 (_globals, file_offset 4550316) gain "static": true in place (round-1 patch re-cut:
  its context row _collision_bsp_usage_times is now static).
- Hygiene evidence files (read): gate_cand_emu.txt 12/12 EXACT; emu_split_diff.txt 833 compared, only breakable_surfaces.obj differs;
  pdb_cand_emu.txt 0 disagreements; surplus_cand_emu.txt 26 COMDATs, 1 not identical (_real_local_random vs effects = B1);
  _bsp3d_get_plane_from_designator IDENTICAL to decals copy; provider_link FAIL(1) = _real_local_random only (pre-existing, same as prod);
  sweep_bs.txt 612 compared, 611 identical, only breakable_surfaces differs (flag-only sweep).
- Reviewer ledger (scratch/w/review_r2w2_breakable_surfaces/LEDGER.md) independently reproduces: A==F (QIfist dead), B vs D only the
  static hand copy -> _bsp3d_get_plane_from_designator; D vs emu split audit PASS 26/26; decals is the only January definer of
  _bsp3d_get_plane_from_designator; nodup_census 0 NODUP.

## Precedent conditions (docs/campaign_house_rules.md) vs existing evidence
- Rule 9 / rule 13 (:9, :13, genuine owners, no manual copies) + rule-6 exception (:33-40: COMDAT byte-identical to January's selected
  copy, caller strictly exact, zero-regression sweep, selected-provider link passes): _bsp3d_get_plane_from_designator IDENTICAL to decals,
  caller _breakable_surface_effect EXACT, sweep 611/612 identical (only this object), provider_link PASS for that row. HOLDS.
- Rule 22 (:28-31, /QIfist only where evidence requires; per-TU, not license to change unrelated flags): A==F proves /QIfist dead here;
  /Ow has no target evidence and dropping it makes 6 helper COMDATs IDENTICAL. HOLDS (drop is removal of an unevidenced flag).
- Storage follows cachebeta publics: neither symbol is public; pdb_storage 2 -> 0. HOLDS.
- Whole-object admission: NOT met (B1 _real_local_random provider FAIL, real_math RESERVED for Codex; B2 static
  _breakable_surface_plane_distance needs owner ruling). Sub-packet is zero credit, as the precedent text says.

## Verdict
BLOCKED (claim held; duplicate of an existing ADMISSION_FIX_PROPOSED packet under active review). No new packet produced.
