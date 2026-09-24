# Adversarial review: files_windows OBJECT_COMPLETE_CANDIDATE (reviewer, lane claude/fifty-objects-20260925)

## Reproduction
- production.patch applies (`git apply --check` OK); patched copy of source/tag_files/files_windows.c == worker files_windows.c == a1.c (sha256 aa3dac72...). Tracked tree untouched.
- gate --all on patched.c -> 27 exact / 0 residual / 0 unwritten (cand.gate.txt). Production source -> 26/1 (_file_get_size [sha]) (prod.gate.txt).
- object_audit cand.obj: PASS, 55 January symbols 0 differ; surplus identical to production audit (only diff line: _file_get_size DIFF->ok).
- secdiff.py (coff_compare.section_infos_equal per owner key) build/base vs cand.obj: 60 same / 1 diff (_file_get_size) -> file_error's own section and all
  4 other inlining callers (open/read/write/exists) byte+reloc identical to production; change is load-bearing ONLY in get_size.
## Strip tests (independent recompiles)
- strip_errcode.c (candidate minus error_code local, GetLastError() inline again): 26/1 residual _file_get_size.
- strip_singleexit.c (production two-return get_size + error_code local): 26/1 residual _file_get_size.
- => both edits load-bearing, jointly sufficient (confirms worker a4/a5).
## Name-count / tie discrimination (lab only, never landing material)
- La_unused_name.c (stripped + `unsigned long error_code;` declared but UNUSED, GetLastError() inline): 26/1 residual.
- Lb_1..4 (stripped + 1..4 extern names before file_error def), Lc_1..4 (before file_get_size def): all 26/1 residual.
  => the lever is NOT a declared-name count; it is the value-holding local (its IL web), consistent with the worker's C2 block-weight decode.
- Robustness: candidate + 1/3/7 extern names before get_size (Rc_*) or before file_error (Rb_*): all 27/27 EXACT.
  => the closure is not a fragile declaration-count lottery tie.
## Other audits (independent)
- pdb_storage: 55 split symbols, 0 disagreements with PDB publics.
- surplus_identity (build/base): 0 candidate-only code COMDATs; direct symbol census of cand.obj: only the 6 pre-existing .rdata surplus
  (3 literals, 3 D3D tables), no candidate-only .text.
- provider_link cand.obj: SELECTED-PROVIDER LINK PASS (6 rows PASS).
- data: build/report.json already 1348/1348 (100%); all non-.text sections of cand.obj identical to build/base (secdiff).
- fake_match_scan patched.c: 0 leads. No files_windows entry in object_admission_rejections.json / semantic_data_matches.json.
- /Od re-read independently (odbuild fn 0x8ae8f0 / 0x8aef00): identical to worker dumps. IAT resolved (pefile, data only):
  0xf56180=GetLastError, 0xf56184=SetLastError, 0xf562c0=FormatMessageA, 0xf56320=GetFileAttributesExA.
  file_error /Od: info=[ebp-8] <- get_info(file); GetLastError() -> [ebp-0xc] (named local; /Od never invents locals); used by FormatMessageA AND error().
  get_size /Od: info init -> full_path="" -> result(byte)[ebp-0x111]=0 -> assert(size) line 0x270 -> get_full_path -> if(GetFileAttributesExA){*size=nFileSizeLow; result=1}
  -> separate `movzx/test result; jne` = `if (!result) file_error(...)` (not else) -> return result. Candidate matches this shape and decl order exactly.
- objdiff-cli 3.3.1 (frozen scorer, build/tools) on review odproj/ (target=build/split obj, base=cand.obj): code 4062/4062, data 1348/1348, 27/27 functions.
- worker a1.obj vs review cand.obj: 61/61 sections identical (deterministic reproduction).

## House-rule review of the diff (source/tag_files/files_windows.c only)
- file_get_size: decl+init combined for info (rule 17); `boolean result = FALSE;` project type; braces on both ifs; single `return result;` (rule 3);
  match_assert untouched (line 524, predicate `size`); shape identical to the already-landed exact siblings file_read/file_open (`if (!result) { file_error(...); } return result;`).
- file_error: `unsigned long error_code = GetLastError();` (DWORD == unsigned long; descriptive name; no original name available - scalar, HCEX has no file_error record);
  used once as the %08x argument. Evaluation order vs get_info unchanged from production (info initializer still runs first) -> behaviour identical.
- No header, symbols.json, config.json, relocs, macro or cast change. file_error is static; nothing outside files_windows.obj can move (no LTCG).
- Semantics: identical return values and error path; no nonsensical logic.

## Authenticity assessment (the load-bearing question)
- Single-exit form: directly attested by /Od get_size (separate `if (!result)` test after the success block, byte `result` slot declared between full_path and data).
  Same law already landed for file_open/read/write in this TU. Not decoration.
- error_code local: /Od file_error has exactly three locals {info, GetLastError() result [ebp-0xc], tempstring[2048]}. January's file_error cannot hold tempstring
  (its 64-byte body is exact without FormatMessage; January format string lacks the trailing '%s'), production has only {info}; error_code is the ONE remaining
  first-party local. It is a used value-holding local, not a filler: an unused declaration of the same name (La) and extra names (Lb/Lc) do NOT close the row,
  so it is not the name-count oracle. Caveat for owner disclosure: in the PC revision the local is also consumed by FormatMessageA (a second use January lacks),
  so its presence in January is inferred from first-party shape + January's get_size allocation, not read directly. This is the campaign's standard
  /Od-shape + January-byte double attestation; precedent of an owner-rejected "steering pointer local" (rasterizer_sun_glow_draw) differs because that local had
  no first-party attestation.
- Strip test: plain spelling (inline GetLastError()) -> 26/1. By the brief's wording the local is therefore load-bearing and needs independent evidence; it has
  first-party /Od evidence in the same function, so it passes on the same basis as other /Od-attested named locals (not a cast/paren/macro decoration).

## Non-blocking observations
- Pre-existing (not introduced here): 6 sibling functions (create/delete/get_last_modification_date/close/set_position/...) hand-expand file_error's body with
  GetLastError() inline. Worker's deferral-model evidence (a3.c, 27/27, January emission order) says January calls file_error() there. Recommend a follow-up
  house-rule-13 packet using a3.c; not required for this admission.
- Park retirement: config/parked.json entry {unit source/tag_files/files_windows, function _file_get_size} is the only config reference; remove in the same
  change (or `python -B -m tools.campaign.unpark --write` after the full build).

## VERDICT: APPROVE (object-complete candidate verified: 27/27 strict, audit PASS, data 100%, surplus unchanged/pass, no blast radius).
