# source/bink/bink_playback (storage classes)

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\bink_playback\\storage.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\bink_playback\\S0_static_bss.c"
]

## production_changes
Apply C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\bink_playback\storage.patch. It is two unified diffs:
(a) source/bink/bink_playback.c (CRLF): add `static` to the four .bss definitions bink_pointer_blocks, bink_bitmap, bink_texture and bink_pointer_block_count (about lines 404-412).
(b) config/symbols.json: edit lines 23299, 23300, 23301 and 23304 in place to append `, "static": true` for _bink_pointer_blocks, _bink_bitmap, _bink_texture and _bink_pointer_block_count. Do not re-serialize or sort the file.

Then regenerate the bink_playback split (csplit) and run the full build and report. Verify that object_audit shows 0 differing symbols and that data stays at 100%.

## evidence
pdb_storage.py flags these four symbols (plus _code_001b5850) as absent from cachebeta publics. A scan of every build/split and build/base object finds zero UNDEF references to the four, and no other source TU names them. By the storage-truth rule they were file-static in January.

S0_static_bss.c (production plus the four statics) is byte-inert: gate 23/1 unchanged, and object_audit differs only in storage class 2 vs 3 against the split still generated from the unedited symbols.json. S1_q3_static_bss.c gives 24/0 with the same four storage-only differences, which the symbols.json edit plus regen removes.

_code_001b5850 is left unchanged, because a static unreferenced function is not emitted (tested in S2: UNWRITTEN).

## blockers
Needs integrator symbols.json edits plus a csplit regen (worker may not edit tracked files or run ninja). Not verified under a regenerated split or an objdiff 3.3.1 report.

## reopen_criteria
n/a (mechanical admission fix; verify after regen).

## task notes
No slug directory existed, so this run started fresh; prior shapes were taken from the committed ledgers (w2, n3a, n4 and n5 in the opus5-150k worktree) and none were repeated. No tracked file was edited and ninja, configure and git were not run. The Oct 2001 prototype XBEs and halo_cache_symbols.exe were read as data only.

Tools written in the slug directory:
- mkvar.py builds a variant by swapping in a .fn body and gating it.
- namesweep.py is the lab-only name-count oracle.
- disobj.py disassembles one function from an object, with relocations.
- xbe.py, scan_xbe.py and scan_xbe2.py parse XBE sections and sweep for the byte compares.

The October 2276 prototype function locations are recorded in the ledger. They are reusable as a second compilation context: 2276P uses a register-argument call convention and 2276defaultP inlines the getter across modules. Both could discriminate other held allocation rows.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\bink_playback\LEDGER.md
