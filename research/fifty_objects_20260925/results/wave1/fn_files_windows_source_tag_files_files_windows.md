# source/tag_files/files_windows

## verdict
OBJECT_COMPLETE_CANDIDATE

## newly_exact_functions
[
 "_file_get_size"
]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\files_windows\\production.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\files_windows\\files_windows.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\files_windows\\a1.obj",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\files_windows\\a1.gate.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\files_windows\\a1.audit.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\files_windows\\LEDGER.md",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\files_windows\\od_get_size.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\files_windows\\od_file_error.txt"
]

## production_changes
1) Apply C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\files_windows\production.patch to source/tag_files/files_windows.c. It is a unified diff with a/ b/ paths and CRLF preserved, and `git apply --check` passes. The result is byte-identical to scratch/w/files_windows/files_windows.c (sha256 aa3dac72...). It makes 2 TU-private edits:
  (a) file_get_size gets the single-exit shape from the /Od build: `struct file_reference_info const *info = file_reference_get_const_info(file);` as the declaration initializer; `boolean result = FALSE;` declared between full_path and attribute_data; `if (GetFileAttributesExA(...)) { *size = attribute_data.nFileSizeLow; result = TRUE; }`; `if (!result) { file_error(file, "file_get_size"); }`; then `return result;`.
  (b) Static file_error gains `unsigned long error_code = GetLastError();` after the info declaration and passes error_code to error() in place of the inline GetLastError() call.
  No header, symbols.json, config.json or relocs change.
2) Retire the config/parked.json park {unit: source/tag_files/files_windows, function: _file_get_size}. The parked bytes change, so the park must go in the same change.
3) Run the usual full ninja build and stable-verdict sweep. Only this TU changes and file_error is static, so nothing outside files_windows.obj is affected.
4) Optional ledger text for docs/object_matching_logs is in scratch/w/files_windows/LEDGER.md.
Optional recoveries (byte-inert, not needed for completion, not in the patch):
  - p5.c/a3.c: January definition order plus close/get_position/set_position/get_eof/set_eof/create/delete/get_last_modification_date calling file_error(). With this, the emission order matches January 27/27.
  - p6.c: file_error(function_name, file) parameter order as in the /Od build.

## evidence
Baseline: 26 exact, 1 residual (_file_get_size), 0 unwritten. The only difference was a callee-saved permutation: January has file in ESI and info in EBX; ours had them swapped.

/Od build: halo_cache_symbols.exe fn 0x8aef00 is file_get_size. Its frame order is info, full_path, result (char, initialised FALSE after the full_path memset), then data. The body is `if (GetFileAttributesExA) { *size = ...; result = TRUE; } if (!result) file_error(...); return result;`.

/Od build: fn 0x8ae8f0 is file_error. It stores GetLastError() into a named local slot [ebp-0xc] after info [ebp-8]. The Stian lift also holds XGetLastError() in a local, which corroborates this but is not byte proof.

The cause was found with Lane A's dbg32c tracer, copied to lab/dbg with a new breakpoint pair at 0x1072a527/0x1072a54c. That C2.dll code shows how priority is computed:
- The weight of a block is the number of webs referenced in it.
- A web that stays live through a block without referencing it loses that block's weight.
- m0 weights: entry 5, assert-fail 1, post-assert 2, error block 3. That gives info 17 against file 13, so info is coloured first and takes ESI (ESI wins ties over EBX).
- With the error_code local, the inlined file_error adds a web to the error block. Combined with the /Od single-exit result web, the error block weight reaches 5 and file is coloured first. That reproduces January's allocation.

Ablations, full-TU gate:
- Both changes: 27/27 EXACT.
- Single-exit only (a4): 26/1.
- error_code local only (a5): 26/1.
Both changes are required together.

Audit of a1.obj (brief section 4):
- gate --all --forbid-emitted-symbol _point_from_line3d: 27 exact / 0 residual / 0 unwritten.
- object_audit: PASS. 55 January symbols, 0 differ. The production base audit was FAIL(1), for get_size only.
- pdb_storage: 0 disagreements.
- surplus_identity: 0 candidate-only code COMDATs.
- provider_link: SELECTED-PROVIDER LINK PASS. The 6 surplus literals and D3D tables are the same as production.
- All 34 non-.text sections are byte- and relocation-identical to build/base. report.json data is already 1348/1348.
- objdiff-cli 3.3.1 on scratch project odproj/: code 4062/4062 (100%), data 100%, 27/27 functions.
- fake_match_scan: 0 leads.
- No entry for this unit in config/object_admission_rejections.json.

Negatives recorded (do not repeat):
- Name-count oracle, N=0..40: negative.
- Explicit sibling error block: no change.
- Const casts and non-const parameter: no change.
- Assert macro forms (?:, do-while, if/else, halt): no change.
- Parameter order, TU reorder and sibling file_error calls on their own: no change.
- Seven result-shape variants without error_code: residual.
- Call-valued result forms (v03, v06, mF) flip the allocation but leave result in BL.

## blockers
None blocking object completion. Owner disclosure: the error_code local in file_error is required for the match (strip test: without it the object stays at 26/1). It is a real, used local, not a dummy or decoration, and the first-party /Od build has it as a named slot. In that later PC build it is also used by an extra FormatMessage call. The effect is through the inlined helper's IL web, not the name-count oracle: file_error is defined after file_get_size, and the N=0..40 name sweep was negative. If the owner rejects the local, the fallback is to keep the park, with this ledger as the reopen evidence.

## reopen_criteria
Not applicable if landed. If the owner rejects the error_code local, reopen only with other first-party evidence of how January's file_error or its error block was shaped. The tracer (lab/dbg/prio2.py) needs the error-block weight to reach 5 while the post-assert block stays at weight 2.

## task notes
The previous run left no slug directory, so this run started fresh.\n\nFindings reusable elsewhere:\n(1) VC7 C2 compiles functions in definition order, except that a function calling a not-yet-defined TU function is deferred to the end of the TU. This was verified on our own object, and it recovers January's definition order from its emission order.\n(2) The allocator's block weight is the number of webs referenced in the block (C2 0x1072aac5..0x1072ab3a). A web that stays live through a block without referencing it loses that block's weight. A helper's locals, once inlined, therefore shift caller register allocation.\n(3) lab/dbg/prio2.py is a retargetable per-block priority tracer: gencfg.py WT/unit edited for this worktree. It runs the stock CL under Lane A's debugger and nothing on disk is patched.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\files_windows\LEDGER.md
