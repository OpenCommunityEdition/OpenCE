# REVIEW source/hs/hs_runtime (storage: 12 converters)

approve: True

amended_patch: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\hs_runtime_storage\hs_runtime_converter_storage.patch

## per_object
[
 {
  "unit": "source/hs/hs_runtime",
  "approve": true,
  "issues": "Byte-inert: identical code, data and relocations; only the 12 storage classes change from 2 to 3. object_audit symbol rows go from 12 to 0, PDB-storage disagreements from 12 to 0, and provider_link from FAIL(12) to PASS. The object remains NonMatching at 64/66 (65/66 once the sibling's patch is applied). Advisory: add a superseding note to the 20260904 reconciliation ledger."
 }
]

## checks
All checks ran at HEAD b7c1b7d9 (the marker on f6d00a8c); the tree was clean at the start and the end. Evidence is in C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_r2_hs_runtime_storage_12_converters\ (REVIEW.md, LEDGER.md). The orchestrator's slug contains ':', which Windows does not allow, so the directory name was sanitised.
(1) Patch application. `git apply --check -v` and `--whitespace=error` are clean in the worktree. GNU patch on the HEAD blob reproduces the worker's cand.c exactly. A scripted check confirms each of the 24 changed lines is `'static ' + old line`.
(2) Gate. gate.py --all on base and cand gives identical rows: 64 exact, 2 residual (_hs_evaluate_begin_random, _render_debug_trigger_volumes), 0 unwritten. All 12 converters are EXACT.
(3) Raw object diff, base.obj vs cand.obj. The two objects differ in 20 bytes: 8 are the PID in the scratch/_gate_<pid> path (.debug$S and .file aux), and 12 are the storage-class bytes, which go from 2 to 3. The symbol table order, names, sections and values are identical (505/505).
(4) object_audit. Base fails with 14 findings (2 .text residuals + 12 storage rows); cand fails with 2, the two .text residuals only. All 129 January symbols match in cand, and all data sections are ok.
(5) PDB storage. Production has 12 disagreements; cand has 0. In cachebeta publics for RVA 0xB9800-0xBAA00, the only publics are hs_runtime_initialize, dispose_from_old_map, dispose, hs_can_cast and hs_cast. The converters at 0xBA220-0xBA38F are absent, so they were file-static.
(6) HCEX DIA2Dump (my own run). The two converters I dumped (hs_object_to_object_list, hs_real_to_long) show `static function: true`; the worker's dump (hcex_all.txt) shows the same for all 12. The control hs_cast is a SymTag 0xA public.
(7) provider_link. Base fails with 12 'NO JANUARY PROVIDER'. Cand passes: all 25 surplus rows pass in both link orders.
(8) surplus_identity, re-run on my objects. The 4 COMDATs are identical to January's providers.
(9) Consumers. Outside scratch, the names occur only in hs_runtime.c and symbols.json:3203-3214 (already "static": true). Across 1454 build/base+split objects there are zero undefined references to any of the 12 names, and no January object defines any of them as external.
(10) objdiff 3.3.1 (sha1 3130e428) report on a private mini project. The prod, base and cand units are identical: matched_code 10294, 60 functions, data 100%. All 66 per-function rows are equal, so there is no scorer gain or loss.
(11) CL /Zs /W3. Base and cand produce identical warning sets (138 lines).
(12) fake_match_scan finds 0 leads in both.
(13) Composition with the sibling render_debug_trigger_volumes production.patch. Both application orders give the same file, and sequential git apply works with offset -8. The composed file gates 65/1/0, and its object_audit fails only on begin_random. Against the sibling's applied object there are 0 section diffs, and only the 12 storage classes differ.
(14) House-rule review. STRIP TEST: removing `static` restores byte-identical code and data, so the keyword buys no byte match. It is a storage-truth fix supported by 3 independent sources: January split storage 3, absence from cachebeta publics, and the HCEX static flag. The patch adds no new declarations or scopes, no header, COMDAT or P1 change, and no held class. Parks and the config status are untouched.

## issues
No blocking issues. Advisory notes for the integrator:
(a) The whole unit source/hs/hs_runtime is claimed by the sibling r2w1 render_debug_trigger_volumes. Either application order works, but the integrator still has to apply both patches.
(b) Add a note to docs/object_matching_logs/hs_runtime_obj_opus_reconciliation_20260904.md that supersedes its 'Deliberate linkage exception' paragraph. That paragraph assumed the typecasting_procedures matrix was absent, which is no longer true: the matrix has been present since 4f4c8502 and its .data section audits ok.
(c) Credit is 0 B, which is correct. The object stays NonMatching.
(d) Out of scope and already present in base: the typedef `long (*hs_typecasting_procedure)(long)` (hs_runtime.c:383, matching the HCEX signature) does not match the union-spelled converters. The matrix initialiser raises C4028/C4133 warnings, and this patch leaves them unchanged.
