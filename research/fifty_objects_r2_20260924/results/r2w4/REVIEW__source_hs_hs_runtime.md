# REVIEW source/hs/hs_runtime

approve: True

amended_patch: None

## checks
All work is under C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_critic3_hs_runtime\ (LEDGER.md R1-R7, MANIFEST.md). HEAD 5d3ca708; git status clean at start and end. I claimed source/hs/hs_runtime for the review and released it afterwards.

(1) Name evidence, checked independently:
- Sept-2001 cachebeta.map lists `_script_error` at 0001:000b9320 in hs_runtime.obj, in the Static symbols section. Its neighbours, _hs_global_reconcile_write and _hs_runtime_get_executing_thread_name, are the same functions that surround the January slot, and the 0x40 gap equals January's padded size.
- Sept retail cache.map lists `@script_error@12`, which means 3 parameters. That fits (thread_index, reason, expression).
- Aug-2001 map: `_script_error` in hs_runtime.obj, also static.
- Byte check with my own XBE reader (xbecmp.py): Sept xbe at 0xca320 and Aug xbe at 0xbd5a0 against January's split function. 64 bytes and 6 relocations each, with no byte differences outside relocations. The 6 Sept relocation operands resolve through the Sept map to the same targets as January's (_hs_thread_data, _datum_get, "no reason given.", _hs_thread_format, "script %s needs to be recompiled", _error).
- Nothing contradicts the name. It is not a cachebeta.pdb public, so static fits. No January literal contains either name. The January atlas has no record at 0x4bace0. HCEX.pdb has neither name.
- Callers: E8 and absolute-reference scans find no caller in January, Sept or Aug. All three builds inline the body into hs_arguments_evaluate and keep an unreferenced out-of-line copy. The later /Od build has it as a real standalone function with the same shape (0x607750).

(2) References: `git grep` of all tracked files finds the old name only in the 4 source occurrences (symbol-listing comment, prototype, call, definition) and symbols.json line 3220. The patch updates all of them in place, keeps `"static": true`, and leaves the line order alone. Nothing in parked.json, the semantic or rejection files, symbol_ownership, tools or tests refers to it. The one remaining mention is prose in a dated docs ledger.

(3) Emulated split: I ran csplit myself into my slug. The control split matches build/split on all 833 objects. With the patched symbols.json, only hs_runtime.obj changes, and it is identical under the rename map (all sections including .debug$S, relocations, symbols; storage stays 3).

(4) Gate: production gives 65 exact / 1 residual (_hs_evaluate_begin_random). The candidate, compiled from the patched .c and gated against my split, gives 65 / 1 with the same set of rows apart from the rename. My production object equals build/base, and the candidate object equals production under the rename map (non-debug sections, relocations, symbol table order and storage).

(5) Section-4 battery, production vs candidate:
- object_audit: FAIL(1) in both, from the existing parked residual; 129 January symbols, 0 differ.
- pdb_storage: 0 disagreements in both.
- surplus_identity: 5 COMDATs, 0 not identical, in both.
- provider_link: PASS in both, with both link orders.
- Data sections are unchanged. No relocation in either object points at the renamed symbol.

(6) objdiff 3.3.1 (sha1 3130e428): production, build/base and candidate all score 11374/14604 code, 61/66 functions, 11940/11940 data. The renamed function scores 100 under either name.

(7) `git apply --check` passes for patch 14 alone. All 21 critic_3 patches apply in sequence on HEAD copies, and the composed hs_runtime.c is byte-identical to the patch-14-only result. The sibling hs_runtime packets (converter storage, render_debug_trigger_volumes, review_r2 amended) are already in HEAD (`git apply -R --check` passes), so nothing pending conflicts.

(8) House rules: only identifiers and a comment change, and an authentic first-party name replaces an invented one. There are no new constructs, so the strip test and /Od declaration-order checks do not apply.

## issues
I found nothing that blocks this patch and no amendment is needed. Two notes:

(a) Landing coupling: the source rename and the symbols.json rename must land in one commit, followed by a csplit regen. Gating the patched source against the current, un-regenerated build/split shows 64 exact plus `_hs_runtime_recompile_error` UNWRITTEN. Patch 14 already carries both edits.

(b) This earns zero credit, as the packet says: it fixes a house-rule name only, and objdiff and gate are unchanged. The object is still not complete, because of the existing parked `_hs_evaluate_begin_random` [sha] residual (object_audit FAIL(1)).

Process disclosure: I ran tools/campaign/relocdiff.py once with no arguments before reading it. argparse printed its usage and exited without side effects. Separately, one shell chain broke on the "saved games" path, which contains a space, and aborted before writing anything outside my slug. After both, `git status` showed only the pre-existing untracked research/.
