# review source/objects/objects
approve=True

## checks
All checks were independent: my own compiles, a COFF parser and a Link.Exe harness written for this review. Evidence is in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review_objects\ (REVIEW.md, LEDGER.md).
(1) Patch integrity: all four Layer 1 diffs pass `git apply --check` on the clean tree at e9e62b78. Each patched file equals cand1.c or cand2.c, ignoring CRLF.
(2) objects (objects.c unchanged):
- gate --all gives 121/121 EXACT. cand.obj equals build/base in every section except .debug$S.
- object_audit PASS: 220 January-owned sections ok, including .data (1530 B) and .bss (13 B); 225 symbols, 0 differ.
- pdb_storage: 0 disagreements.
- surplus_identity: 26 code COMDATs, 0 not identical. My own check over all 71 candidate-only external definitions (code, literals, 3 D3D tables) found all identical to January's selected copies.
- provider_link on production fails exactly 4 symbols (_point_in_sphere, _real_random, _signed_angular_difference, _valid_real_vector3d_axes2). _valid_real_vector3d already PASSes.
(3) Providers, candidate vs production compile:
- Gate row text is identical: action_charge 21+1, player_control 49/49, bored_camera 9/9, path_obstacles 15+1 (residuals pre-existing).
- objcmp2.py (includes COMDAT selection): only NODUP->ANY on the 5 helpers, plus bored_camera's new ANY _valid_real_normal3d, _magnitude_squared3d and __real@3f800000. All three are identical to January's selected copies and pass provider_link.
- object_audit lines for January-owned sections are unchanged in all four providers.
(4) Board census: every other build/base definer of these helpers is ANY and identical to January's selected copy. The only NODUP definers are the 5 hand copies being removed.
(5) Link.Exe:
- objects x each candidate provider: 0 LNK2005/LNK1169 in both orders.
- Positive controls (objects x each production provider) report the expected LNK2005.
- objects + all 4 candidates combined: 0 in both orders.
(6) Genuine uses confirmed from January relocations: action_charge_setup @16b/@171 (real_random inlined), _interpolate_scalar @51f, _valid_real_vector3d_axes2 @1e8, _point_in_sphere @114. signed_angular_difference is used 3 times.
(7) Cross-lane composition: the action_charge diff applied to the action_charge lane's candidate.c (perform EXACT) gives 22/22, with only a selection change. The weapons lane's p4 is the identical edit.
(8) fake_match_scan: 0 leads on objects.c, the candidates and the production files.
(9) House rules: the diff only removes renames, #undefs, two consumer-local prototypes and the hand copies. Nothing new is introduced, so no strip test applies.
(10) Scorer/admission: I found the frozen 3.3.1 data gap and simulated tools.audit_object_admission with and without a semantic data entry (see issues).

## issues
REQUIRED AMENDMENT (the production-change list is incomplete as written):
- Under the frozen objdiff 3.3.1 build/report.json, objects' .data section ?section@?1??objects_update@@9@9 (1530 B, one relocation to a ??_C@...$AA@ literal) scores 99.74%, so strict matched_data is 3785/5315.
- The section is identical under coff_compare section_info_resolved (the known '$'-literal defect), but config/semantic_data_matches.json has no entry for objects.
- Simulation: setting objects.c to Matching without an entry leaves objects revoked ('1530 unmatched data bytes') and marked completion-label-contradicted.
- Fix: append the single-section entry in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review_objects\semantic_data_entry_objects.json (symbol ?section@?1??objects_update@@9@9, size 1530, relocation_count 1, normalized_sha256 e94bb79478c95dce11c4a1f720a07ccbaa3a8d18b84acbe5605472d6f8635b74). With it, the simulation shows 0 revoked and 0 contradicted.
- It must land in the same change as the Matching status, because apply_semantic_data_matches rejects an entry for a unit that is not marked complete.

Full apply order:
- git apply the four Layer 1 diffs.
- Rebuild with ninja.
- provider_link source/objects/objects should PASS.
- Remove the objects entry from object_admission_rejections.json. Its _valid_real_vector3d symbol was already stale.
- Set objects.c to Matching together with the data entry.
- Credit no helper bytes.

Integration notes:
- The action_charge edit is the same as the weapons lane's p4. Apply it once.
- It composes with the action_charge lane's patch: the hunks don't overlap, and the result was measured at 22/22.

Non-blocking, pre-existing:
- objects.c has 15 single-line parameter lists (static prototypes on lines 114-140 and void object_pvs_clear(void) at line 517), against house rules 1 and 2. 47 of 361 Matching objects have the same deviation. The fix is whitespace-only.
- path_obstacles still emits 3 invented-name COMDATs (*_inline). player_control keeps the limit2d rename.
- The bored_camera candidate ends with an extra trailing blank line.
