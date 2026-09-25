# REVIEW source/rasterizer/xbox/rasterizer_xbox_profile

approve: True

amended_patch: C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/review_r2w2_rasterizer_xbox_profile/PACKET_rasterizer_xbox_profile_V1R_reviewed.patch (sha256 0a5885fdde06bb8c167956d9e0a15e743cf7e1abaceb1c74a3bfc15f8b3eecc9). It touches 3 files: rasterizer_xbox_profile.c (04 + 08 + listing sync), rasterizer_xbox_detail_objects.c (06) and config/object_admission_rejections.json (07R_V1R). The full post-image rejections file is rejections.V1R.json in the same folder.

## per_object
[
 {
  "unit": "PACKET_rasterizer_xbox_profile_full.patch (FULL)",
  "approve": false,
  "issues": "It makes the parked rasterizer_xbox_models:__rasterizer_model_draw stale (sha 89b0d7ea to cbfa8585; objdiff 95.08453 to 95.07629), so the ninja progress park check fails. There is no parked.json hunk and no disclosure. It also moves a parked residual away from January and leaves rasterizer_xbox.c with a duplicate extern. It carries the F2 rejection-text stretch."
 },
 {
  "unit": "PACKET_rasterizer_xbox_profile_V1_byte_inert_subset.patch (V1 as submitted)",
  "approve": false,
  "issues": "The code hunks 04, 06 and 08 are verified byte-inert everywhere (10/10) and are sound. The 07R_V1 text declares the stock SDK tables resolved, contradicting canonical's open policy (641e1466 reconciliation), and removes them from reopen. This is fixed in V1R."
 },
 {
  "unit": "PACKET_rasterizer_xbox_profile_V1R_reviewed.patch (reviewer amendment)",
  "approve": true,
  "issues": "None blocking. Contents: V1 (04+06+08), plus a byte-inert listing-comment sync to the symbols.json static rows, plus 07R_V1R, which keeps the SDK-table owner-policy item open and adds the park re-baseline to the window reopen clause. Results: 10/10 IDENT; profile 16/16, detail_objects 20/20; audit PASS; pdb 0; link PASS in both orders; /W3 same; fake scan 0; rejection_index OK. Zero credit, and the unit stays NonMatching."
 },
 {
  "unit": "source/rasterizer/xbox/rasterizer_xbox_detail_objects (06 consumer)",
  "approve": true,
  "issues": "20/20 EXACT and canonically identical. The audit FAIL(14) is pre-existing and unchanged. Remaining debt, not introduced here: the consumer-local main_get_window_count prototype and the detail_objects_debug_options view."
 }
]

## checks
Base: HEAD 434f0151, which is 05255584 plus a ledger document. Claim review-r2w2-profile-adv, now released. The tree is clean: the only untracked path is research/, which is not mine.

- **git apply --check at HEAD:** FULL rc 0, V1 rc 0, amended V1R rc 0. The V1R round trip is byte-equal to the tested tree.
- **Harness.** I wrote my own sweep.py and canon.py, compiled from shadow copies of source/ with the production flags, and compared every non-debug section (bytes, plus relocations by target name) and the symbol table against build/base.
- **Sweep of all 10 includers of the changed files:**
  - control 10/10 IDENT, so the harness is valid;
  - FULL 9/10: only rasterizer_xbox_models __rasterizer_model_draw changes, from sha 89b0d7ea to cbfa8585 at 5168 bytes and 348 relocations;
  - V1 10/10 IDENT;
  - V1R 10/10 IDENT.
- **Lab.** Production models.c with its own extern moved after either the internal.h include or the models.h include is canonically identical to the FULL object. The worker's position-tie diagnosis is correct.
- **Gate:** profile 16/16 EXACT; detail_objects 20/20 EXACT.
- **object_audit** (no symbols.json change, so build/split is the emulated split):
  - profile PASS: 67 symbols, 0 differ; .data 132/132 and .bss 1122/1122 ok. Surplus is the same as production: .drectve, 4 literals, 3 SDK tables, 3 __real.
  - detail_objects FAIL(14), identical to the production audit (pre-existing).
- **pdb_storage:** 0 disagreements.
- **surplus_identity:** 0 candidate-only code COMDATs.
- **provider_link:** 10/10 PASS in both orders.
- **SDK tables** against their January split definers (headers, progress_bar, decals): section_infos_equal and flags equal, 3/3.
- **January split UNDEFs** confirm _D3DDevice_InsertCallback@12, _QueryPerformance* and _global_window_parameters.
- **Data:** coff-equal. The report's .data 68.69% is the known '$' defect; no entry is added.
- **CL /Zs /W3:** 12 and 12 warnings, identical in ctl, V1, FULL and V1R.
- **fake_match_scan:** 0 leads.
- **Admission audit** with the prod, FULL and V1 rejection files: 9/0/2/0. The amended entry validates through object_admission_policy.rejection_index.
- **/Od evidence (odbuild fn 0x8004c0)** confirms the 04 parameter order (condition, profile, message). It also confirms that the NONE-branch push order matches 08's BUG comment.
- **Park check:** tools/parked_functions.py and project_x86.py:717, for FULL's effect on the parked __rasterizer_model_draw.
- **Canonical precedent read in the original:** claude_fifty_objects_canonical_reconciliation.md (641e1466), claude_fifty_objects_20260925.md:518-541, profile 20260906 #remaining-whole-object-boundaries, and the collision_debug b9a8d587 review.

## issues
I approve only the amended V1R patch. Both packets as submitted are rejected.

**F1 (FULL, blocking and not disclosed).**
- config/parked.json:1166 parks rasterizer_xbox_models:__rasterizer_model_draw with pinned base sha 89b0d7ea and objdiff 95.08453.
- FULL changes that body to sha cbfa85852c836af123fe93a3f15f22f2f43ba574fbff9add3c28f0e0275ed4d4, and the worker's own 3.3.1 mini report gives 95.07629.
- The park therefore goes STALE, and require_valid_parked_functions exits the ninja progress step (project_x86.py:717).
- The packet has no parked.json hunk and never mentions the park.
- FULL could land only with all of:
  - a re-baseline of that park;
  - owner acceptance that a parked residual moves one instruction away from January;
  - cleanup of rasterizer_xbox.c, which would keep a duplicate local extern next to the new header declaration.
- Six full-type consumer externs and nine partial views remain, so the dynavobgeom window reopen criterion is still unmet.

**F2 (07R and 07R_V1, precedent stretch).**
- Both rewrites say the three stock SDK tables "are now accounted for as authentic topology" and drop them from reopen.
- Canonical's 641e1466 reconciliation says this criterion "conflicts with canonical's existing ruling" and is "pending a uniform policy/owner audit". Brief section 8 says the dynavobgeom hold reasons still apply.
- The 20260906 evidence also records LNK2005 against January's split owners.
- So the collision_debug standard does not settle this item.
- My 07R_V1R keeps every verified fact (identical tables, pair links in both orders, D3D8.h imports), states that the owner policy decision is still open, and adds it plus the park re-baseline to reopen.

**Otherwise.** V1's 04, 06 and 08 are sound, attested and byte-inert:
- 06 also fixes a wrong void return type in the removed rasterizer_set_texture prototype.
- I added FULL's byte-inert listing-comment hunk. It syncs the stale "symbols in this file" block (which still shows _bss_00465e28) to the existing symbols.json static rows; I checked every offset.

**Held and untouched:** B1 (invented aggregates and the +0x45c object), B2 (varargs bug), B3 (volatile).

**Credit:** zero. The unit stays NonMatching with no data entry.

**Evidence:**
- C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/review_r2w2_rasterizer_xbox_profile/REVIEW.md
- C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/review_r2w2_rasterizer_xbox_profile/LEDGER.md
