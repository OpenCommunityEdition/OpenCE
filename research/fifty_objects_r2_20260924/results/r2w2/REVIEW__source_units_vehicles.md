# REVIEW source/units/vehicles

approve: True

amended_patch: No amendment needed. Land as submitted: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\r2w2_vehicles\patches\01_vehicles_fallback.patch (sha1 7e45032c), then C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\r2w2_vehicles\patches\02_symbols_turret_static.patch (sha1 2a100d69). Optional and verified: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\r2w2_vehicles\patches\optional\03_OPTIONAL_pathfinding_scale_vector3d.patch (sha1 3639cd62; post-image blob a9e6fee5).

## per_object
null

## checks
All checks were run independently at HEAD 434f0151 (05255584 plus one ledger-only commit), with the tree clean. Artefacts are in C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_r2w2_vehicles\ (REVIEW.md, LEDGER.md).

1. **Patches apply.** `git apply --check` passes for 01 and 02 in the worktree. Applied outside the repo to the HEAD blobs, they give vehicles.c e6710a63 (same as the worker's cand.c) and symbols.json c602dcdc. The hunks are byte-identical to the round-1 files.
2. **Gate, every January function.** my_base.c is 37/2 (fighter [relocs 33!=32, sha], scout [sha]). my_cand.c is 37/2 (fighter [sha], scout [sha]). No row changes verdict.
   - my_base.obj matches build/base vehicles.obj with 0 differences, and my_cand.obj matches the worker's cand.obj with 0 differences.
   - Base vs candidate, keyed by owner symbol: the only changes are the new _point_from_line3d COMDAT and the fighter bytes.
3. **Relocation census.** January has one reference, `_vehicle_update+0x268 -> _point_from_line3d` (UNDEF). Production has that plus one at fighter+0x138. The candidate has only the `_vehicle_update` one, now pointing at a local ANY COMDAT.
   - Across build/split, exactly 17 objects hold an UNDEF reference. They are ruling #1's list word for word, vehicles included. action_charge is the only definer.
4. **Lab: is the new COMDAT caused by the non-exact fighter (P1)?** lab_nofighter.c deletes the fighter's use. The COMDAT is still emitted, section-identical, and referenced only by the exact `_vehicle_update`. So P1 does not apply.
5. **Emulated split.** I ran csplit with my own copy of config/ plus the patched symbols.json. Only source/units/vehicles.obj differs from build/split. Its 68/68 sections are equal and one symbol row changes: `_update_turret_physics` storage 2 -> 3.
6. **object_audit.** Candidate vs emulated split: 66 January-owned sections ok, 2 differ (the held fighter and scout), 68 symbols with 0 differing, so FAIL(2) is residuals only. Production vs build/split is FAIL(3). The surplus delta is exactly +_point_from_line3d (48 bytes).
7. **Storage.** pdb_storage gives production 1 disagreement and the candidate 0.
   - cachebeta publics have 0x1A4F00, 0x1A4F70 and 0x1A5040 but not 0x1A4F50.
   - No relocation in the whole split names `_update_turret_physics`.
8. **Surplus and provider link.**
   - surplus_identity: 28 candidate-only code COMDATs, 0 not identical. `_point_from_line3d` is identical to action_charge's copy.
   - provider_link on new surplus only: PASS against action_charge in both orders.
   - Full provider_link: 85 rows PASS (production has 84).
   - nodup_census: 26 definers, no NODUP.
9. **Data and scorer.** Every data section is identical to production. I generated an objdiff 3.3.1 report for this object alone.
   - The production pair reproduces build/report.json: 34 functions, 9436 of 16908 code bytes, 1744 of 1744 data bytes.
   - The candidate against the emulated split has the same counts.
   - The only per-function change is the fighter, 95.01 -> 99.91 fuzzy. The turret stays at 100.
10. **Warnings and fake-match scan.** `/Zs /W3` gives 27 diagnostics for both, the same multiset. fake_match_scan finds 0 leads on both.
11. **/Od check.** I dumped fn 0x8fd970 myself.
    - Both point_from_line3d calls (0x8fda42, 0x8fdad2) match the candidate's argument order. I checked the constants 0.4f and 2.0f and the pointers to (0,0,1) and (0,0,-1).
    - There is no `t` staging local in the /Od frame.
    - The surface_index store comes before the second call, as in the candidate.
12. **Asserts.** vehicles.c asserts use literal line numbers, so the -10 line shift changes no bytes.
13. **Optional 03.** I applied and compiled it: 37/2, whole object identical to the candidate, same /W3 set, and attested by the /Od call at 0x8fda65.
14. **Precedent.** I read ruling #1 (HANDOFF:65-68, :156-162), the rule-6 exception in docs/campaign_house_rules.md:32-40, the four precedent diffs, 05255584, the Lane B manifest l.1119-1120 (the ruling was requested for vehicles itself) and the round-1 vehicles review.

## issues
Nothing blocks this.

**Precedent check (special focus): not stretched.**
- Ruling #1 says, quoted, "a TU may emit it iff January's SAME object REFERENCES it out of line". vehicles is in the list, and Lane B asked for that ruling about vehicles.obj in the first place.
- Every condition of the rule-6 exception holds:
  - the COMDAT is byte-identical;
  - the caller that emits it (`_vehicle_update`) is strictly exact, and the lab shows the fighter does not anchor it;
  - base and split both change only vehicles.obj, and no row is lost;
  - the selected-provider link passes in both orders.
- The same paragraph forbids hand-expanding the helper to evade the check. Production's pathfinding hand expansion does exactly that, with a "without owning point_from_line3d" comment, and this packet removes it.
- The form matches all four precedents: plain removal of the define/undef pair, no comment. The one difference is that each precedent also gained strict functions, while this one gains nothing. The ruling does not require a gain.
- The owner's "keep the fighter held" (3ba2eb91) is about landing the fighter: the cross_product2d view casts and the `_cross_product2d` emission against actor_combat's NODUP copy, which is RESERVED. This packet changes no fighter source and emits no `_cross_product2d`. The fighter is not parked, and its bytes only move toward January.

**Non-blocking notes:**
- The manifest's "86/86 rows PASS" counts the summary line. It is really 85 provider rows (production 84). Nothing else changes.
- Optional 03, `scale_vector3d(global_down3d, 2.0f, &vector)`, is verified byte-inert and /Od-attested. I recommend it, but it is not required.
- Out of scope and already in production:
  - /Od names the pathfinding locals `collision_origin` and `collision_vector`, while production uses `origin` and `vector`.
  - vehicles.c has two C4013 implicit declarations: `matrix3x3_multiply` and `matrix3x3_rotation_to_quaternion`.

**Integrator:**
1. Apply 01, then 02 (and 03 if you choose).
2. Regenerate the split with csplit only.
3. Run a full ninja.

config.json stays NonMatching. There is no credit: the object stays at 37/39.
