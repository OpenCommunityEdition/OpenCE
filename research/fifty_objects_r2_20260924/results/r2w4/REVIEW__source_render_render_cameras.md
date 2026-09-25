# REVIEW source/render/render_cameras

approve: True

amended_patch: None

## checks
All checks were run independently at HEAD 5d3ca708. No tracked files were edited. Evidence is in C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_critic3_render_cameras\ (LEDGER.md, MANIFEST.md).

1. Patch applies cleanly: `git apply --check` of patches/21_render_cameras.patch returns 0. It also applies with GNU patch to copies in the slug (applytest/).

2. First-party name evidence, read directly from the sources:
   - Sept-2001 cachebeta.map (Static symbols, line 19673): `_render_camera_warn_once` at 0001:00172bc0, size 0xa0 up to `_render_camera_new`.
   - Aug-2001 map: the same name at 0001:00158150, size 0xa0.
   - Neither map contains `check_warning_condition` anywhere.
   - January atlas 4cc87b45: 0x574fc0 is `_render_camera_warn_once` in render_cameras.obj, tier exact.

3. Body identity (bodycmp.py): the January split section (0xa0 bytes, 13 relocations) matches both the Sept xbe (.text+0x172bc0) and the Aug xbe (.text+0x158150) with 0 differing bytes. Only January's relocation fields were masked, so this is a raw-byte comparison, not just matching instruction shape.

4. Callers (callers.py, a scan of E8 calls): 22 calls, all inside render_camera_build_frustum, in Sept, in Aug and in the January PE (0x577464..0x5777bb). The source also has 22 call sites.

5. January strings: nothing contradicts the Sept name. The only related texts are the MAXIMUM_RENDER_CAMERA_WARNING_CONDITIONS assert and the "frustum-integrity" error string.
   - cachebeta publics: the function is not public under either name, so it was a file static and `"static": true` is correct.
   - HCEX.pdb (2011): the helper is absent under both names (streamed DIA2Dump dump). This is neutral.

6. References: I ran `git grep` over all tracked files. The patch updates the symbols.json row in place (CRLF-safe, order unchanged) and, in render_cameras.c:
   - the stale `_code_00174fc0` placeholder in the symbol-listing comment
   - the prototype, all 22 calls and the definition
   The result is 25 occurrences of the new name and 0 of the old name or the placeholder. No header, semantic_matches, semantic_data_matches, admission-rejection, splits, symbol_ownership or tools file references either name. The parked.json key is `_render_camera_build_frustum`, and the old name appears there only in prose about the PC corpus.

7. Gate:
   - production vs build/split: 20 EXACT / 1 residual (the parked build_frustum).
   - candidate vs the stale build/split: the helper shows UNWRITTEN, as expected, so the split must be regenerated.
   - candidate vs the emulated split: 20/1, with the same rows apart from the rename.

8. Emulated split (build/tools/csplit.exe, with config copies in the slug): split_ctl equals build/split for all 833 objects. split_cand differs only in render_cameras.obj, and objcmp finds it IDENTICAL under the rename map (every section's bytes and named relocation targets, plus the full symbol table).

9. Objects: objcmp of prod.obj vs cand.obj is IDENTICAL under the rename map; only .debug$S differs. build/base vs prod.obj is IDENTICAL.

10. Parked measurements for build_frustum re-measure equal to the recorded values: target 3376/113/3ce4fcfd…, base 3408/112/a0e3988a…. The normalized sha zeroes relocation fields, so names do not affect it.

11. Section-4 battery (read-only with_split redirect, tools unmodified), same result for production and candidate:
   - object_audit FAIL(1) = the parked build_frustum; the helper row is ok 160/160.
   - pdb_storage: 1 disagreement that already exists and is unrelated (`_previous_projection_coefficients`).
   - surplus_identity: 20 COMDATs, 0 not identical.
   - provider_link: PASS in both link orders.

12. objdiff 3.3.1 (sha1 3130e428): production, candidate and build/base are all 6721/10091 code, 20/21 functions, 1491/1491 data, 96.32176%.

13. Strip test: not applicable, since only identifiers and one comment change. The /Od declaration-order check is also not applicable: there are no new scopes or declarations, and the /Od build has no symbols. Held classes and house rules: nothing held is touched, and an authentic first-party name replaces a non-first-party (PC-corpus) name and a code_ placeholder (house rule 7).

Claim taken and released as review_critic3_render_cameras. `git status` is clean apart from the untracked research/ directory that was already there.

## issues
No blocking issues; I approve the patch unchanged. It earns zero credit because names cannot move bytes.

Notes for the integrator:
(a) Land the symbols.json edit and the .c edit together, then regenerate the split (ninja does this). Against the stale build/split the renamed helper shows as UNWRITTEN until the split is regenerated.
(b) The prose in the config/parked.json build_frustum evidence still names the helper `render_camera_check_warning_condition`. It is describing the PC reverse-engineering corpus, so it is still accurate and validation keys only on `_render_camera_build_frustum`. Adding the first-party name there is an optional follow-up, not a defect.
(c) Two round-1 FUZZY packets that were never landed spell the old helper name in both context and added lines: research/fifty_objects_20260925/w/render_cameras/production.patch and production_owner_gated.patch. If either is landed after patch 21, rename the helper in it first or it will not apply. No round-2 packet in scratch/w touches render_cameras.
(d) The docs/ and research/ ledgers keep the old name as history. That is correct.
