# review source/cache/cache_files_windows
approve=True

## per_object
[
 {
  "unit": "source/cache/cache_files_windows",
  "approve": true,
  "issues": "APPROVE as OBJECT_COMPLETE. Apply C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/cache_files_windows/production.patch; it touches only source/cache/cache_files_windows.c, and `git apply --check` is clean against 931ed8dc. The CRLF full file scratch/w/cache_files_windows/cache_files_windows.c is an equivalent alternative. Then retire the parked.json entry for _cache_files_open_cache_files. Verified: 50/50 exact, object_audit PASS (0 differ), pdb_storage 0, surplus_identity 0 and provider_link PASS 6/6, data 100%. objdiff 3.3.1 gives open_cache_files 100% and matched_code 5968/6144; the remainder is the accepted semantic-coff precache_map_status. Strip and equivalent-spelling labs are clean. Reviewer notes: scratch/w/review2_cache_files_windows/REVIEW.md."
 }
]

## checks
Read all of WORKER_BRIEF.md (including section 7), the house rules, the campaign log, the worker LEDGER, the prior closeout ledgers for this unit and the parked.json entry. I did not run ninja or any git-mutating command, and I edited no tracked file. All reviewer work is in C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/review2_cache_files_windows/ (REVIEW.md, lab/, od/).
(1) Patch. `git apply --check scratch/w/cache_files_windows/production.patch` is clean against the current tree. HEAD is 931ed8dc, the blob is 4c8eadd2 and the worktree file is clean. HEAD's file with the patch applied is identical to cand.c. The CRLF full file differs from the worktree file only in the intended lines, and both files are all CRLF with no LF-only lines.
(2) gate.py --source (CRLF full file) --all --out review2/cand.obj: `== exact 50 residual 0 unwritten 0 (of 50)`. My obj differs from the worker's only in the gate temp-file names and the timestamp. tinfo shows the candidate's _cache_files_open_cache_files at 752 B / 47 relocs with sha f1c88310c585856c, which equals January and the parked target sha. In alndiff the counts are T 234 / O 234. The only row shown is the pre-existing literal attribution for 01.01.14.2342, which normalizes equal.
(3) object_audit on cand.obj: PASS, 87 January symbols, 0 differ. The surplus (.drectve, the buffer/header/01.01.14.2342 literals and the three D3D selectany tables) is identical to the surplus in production build/base. Production's audit fails only on open_cache_files. The D3D surplus is systemic: prior reviews counted 57-58 Matching objects that carry it.
(4) pdb_storage: 0 disagreements. surplus_identity: 0 code COMDATs. provider_link with cand.obj: PASS 6/6.
(5) Data: build/report.json shows .rdata 1192 and .bss 12412, both 100%. I re-ran objdiff 3.3.1 (build/tools sha1 3130e428) in review2/od. The candidate gets matched_code 5968/6144 and data 13604/13604, and open_cache_files is 100%. The only function below 100% is _cache_files_precache_map_status at 91.52542, which is strict-exact and already in the semantic_report accepted_ledger with semantic-coff proof. Production's matched_code is 5222.
(6) fake_match_scan on the candidate: 0 leads. The unit has no admission or semantic rejection entries. No file textually includes the .c. The function is static and absent from cachebeta publics, which is consistent.
(7) Strip and robustness labs:
- s1, the nested equivalent `if(!valid){if(file!=INVALID){CloseHandle;file=INVALID;}}`: EXACT.
- s2, the swapped `file!=INVALID && !valid`: EXACT.
- So the match does not depend on one particular spelling.
- s4, the PC /Od literal form: residual. This confirms h7a.
- s5, with no file guard: residual, and it would call CloseHandle(INVALID_HANDLE_VALUE). The guard is needed for correct behaviour, not added for the match.
- s3, which drops the pre-existing failure-arm `valid = FALSE`: residual at 237 insns, so that store is load-bearing.
(8) Evidence spot-checks:
- January bytes: 0x6b valid=0 at the loop top; 0x1b6 valid=0 in the failure arm; 0x1e2/0x1e8 CloseHandle and file=-1; J1 at 0x1ef; the 823 arm at 0x2b1 does `jmp 0x1f5` with no -1 store.
- /Od 0x857520: a trailing `if(!success){if(file!=-1)CloseHandle(file); file=-1;}` after the whole if/else.
- October DEBUG 0x1bd5f0: J1 at 0x1bd7df and the 773 arm after `ret`, the same layout as January.
- October release: `xor bl,bl` in the failure arm at 0x158b8e.

## issues
No blocking issues. Notes for the integrator and owner:
(a) The source-shape evidence for the trailing cleanup is an analog. It comes from a later first-party function in a different file (/Od 0x857520, saved_games/game_state_pc.c), built from the same CreateFile/resize idiom, not from this function itself. The October XBEs support the layout but cannot tell the source forms apart, because both base and candidate reproduce the October release. The acceptance therefore rests on four things: the strict-exact result, the plain, behaviour-preserving spelling that needs no decoration, the fact that equivalent spellings also match, and that analog. This meets the recorded 20260919 reopen criterion: the cold CreateFileA-failure placement is explained and the success store at 0x1a4 does not move.
(b) The failure-arm `valid = FALSE;` is redundant in meaning, because valid is already FALSE from the loop top. It was already in production and is not part of this patch. January's bytes attest it independently (0x1b6), and so does the October release build (xor bl,bl). It is load-bearing (lab s3).
(c) Assert-line bookkeeping: the 811 to 823 gap is 12 lines. Moving the two lines out of the failure arm makes our text span shorter still. This is text-only and weak, since comments and blank lines count, and it does not change the verdict.
(d) Bookkeeping: the integrator must retire the config/parked.json entry for source/cache/cache_files_windows / _cache_files_open_cache_files (target sha f1c88310...). Otherwise ninja PROGRESS fails on parked bytes moving. No semantic_matches or semantic_data_matches entry is needed, and no header, config or symbols.json change is involved.
