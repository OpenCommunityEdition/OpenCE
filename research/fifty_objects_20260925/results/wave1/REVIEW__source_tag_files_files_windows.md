# review source/tag_files/files_windows
approve=True

## checks
I re-ran every check myself; all artifacts are in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review_files_windows\ and the notes are in REVIEW.md there.

1) Patch application. production.patch passes `git apply --check`. A copy of the production file with the patch applied (patched.c) has sha256 aa3dac72..., the same as the worker's files_windows.c and a1.c. All lines are CRLF in both files. The tracked tree is untouched.

2) Gate. gate.py --all on patched.c gives 27 exact / 0 residual / 0 unwritten (cand.gate.txt). The production source gives 26/1, with _file_get_size [sha] as the residual (prod.gate.txt).

3) object_audit on cand.obj: PASS. 55 January symbols, 0 differ. The surplus list is identical to the production audit; the only changed row is _file_get_size, DIFF -> ok.

4) Section comparison (secdiff.py, coff_compare.section_infos_equal per owner key). build/base vs cand.obj: 60 sections the same, 1 different (_file_get_size). So _file_error's own out-of-line body and the four callers that inline it (open/read/write/exists) are byte- and relocation-identical to production. The worker's a1.obj matches cand.obj on 61/61 sections. Against January's split obj, _file_get_size and _file_error are both equal.

5) pdb_storage: 0 disagreements. surplus_identity: 0 candidate-only code COMDATs. A direct symbol census of cand.obj finds only the 6 surplus .rdata entries production already has (3 literals, 3 D3D tables). provider_link on cand.obj: SELECTED-PROVIDER LINK PASS.

6) Data. build/report.json already shows 1348/1348, and every non-.text section is identical to build/base. objdiff-cli 3.3.1 (the frozen scorer in build/tools) on a fresh scratch project: code 4062/4062, data 100%, 27/27 functions.

7) Other gates. fake_match_scan: 0 leads. The unit has no entry in object_admission_rejections.json or semantic_data_matches.json. The only config reference to _file_get_size is its parked.json entry.

8) Strip tests (my own recompiles):
   - strip_errcode.c (the candidate with GetLastError() passed inline again): 26/1.
   - strip_singleexit.c (production's two-return get_size plus the error_code local): 26/1.
   Both edits are load-bearing and together sufficient.

9) Name-count / tie check, lab only:
   - La: an UNUSED `unsigned long error_code;` declaration: 26/1.
   - Lb_1..4 / Lc_1..4: 1-4 extern names before file_error or before file_get_size, without the local: all 26/1.
   - So the lever is the local that holds a value, not the name count.
   - Robustness: the candidate plus 1, 3 or 7 extern names at either position stays 27/27, so this is not a fragile declaration-count tie.

10) /Od build. I re-read fn 0x8ae8f0 and 0x8aef00 with odbuild; both match the worker's dumps. I resolved the IAT with pefile, reading the file as data only: 0xf56180=GetLastError, 0xf56184=SetLastError, 0xf562c0=FormatMessageA, 0xf56320=GetFileAttributesExA.
   - file_error keeps GetLastError() in the named slot [ebp-0xc], after info at [ebp-8].
   - get_size shows the separate `if (!result)` test and declaration order info, full_path, result (byte), data. The candidate matches both exactly.

11) HCEX.pdb has no file_error or file_get_size record, so it gives no scalar-local evidence either way. The Stian files.c is an LLM lift and counts as corroboration only. The optional a3.c variant (sibling functions call file_error) re-gates at 27/27.

12) House-rule review of the diff:
   - info is declared with its initializer (rule 17).
   - `boolean result = FALSE;`, braced ifs, a single `return result;`.
   - The match_assert is unchanged.
   - `unsigned long error_code` is DWORD with a descriptive name.
   - Behaviour is identical, because GetLastError() still runs after get_info.
   - No header, symbols.json, config, relocs, cast, paren or macro change, and file_error is static, so nothing outside files_windows.obj can move.

## issues
No blocking issues. Disclosures for the integrator and owner:

1) Evidence for the error_code local (owner disclosure).
   - In the later PC /Od build, file_error has exactly three locals: info, the GetLastError() result at [ebp-0xc], and a 2048-byte tempstring.
   - January cannot have tempstring. Its 64-byte file_error is exact without FormatMessage, and its format string lacks the trailing '%s'.
   - That leaves error_code as the only first-party local production lacks, and adding it closes the row.
   - Caveat: in the PC revision the value is used twice (FormatMessageA and error()), while January uses it once. Its presence in January is therefore inferred from first-party shape plus January's get_size register allocation, not read directly.
   - This is the campaign's standard /Od plus January-bytes double attestation. It differs from the owner-rejected "steering pointer local" (rasterizer_sun_glow_draw), which had no first-party attestation.
   - Name-count and declaration-count explanations are refuted by lab tests La, Lb and Lc.
   - The ledger text should state this caveat.

2) Park retirement.
   - The config/parked.json entry {unit source/tag_files/files_windows, function _file_get_size} must be removed in the same change, because the parked bytes move and the park would otherwise fail PROGRESS.
   - It can be removed by hand or with `python -B -m tools.campaign.unpark --write` after the full ninja build.
   - The integrator must still run the full ninja build and the stable-verdict sweep; I only gated the single TU.

3) Pre-existing house-rule-13 debt (not introduced here, byte-inert, does not block this admission).
   - Production hand-expands file_error's body in 6 places (the `error_info` blocks: create, delete, get_last_modification_date, close, set_position, and one more).
   - Those copies still pass GetLastError() inline, so they no longer match the helper's text.
   - The worker's a3.c recovers `file_error()` calls there plus January's definition order. It re-gates at 27/27, and the worker reports its emission order matches January's.
   - It is a 735-line reorder, so I recommend it as a separate follow-up packet rather than part of this change.
