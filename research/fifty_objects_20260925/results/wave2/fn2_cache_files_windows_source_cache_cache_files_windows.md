# source/cache/cache_files_windows

## verdict
OBJECT_COMPLETE_CANDIDATE

## newly_exact_functions
[
 "_cache_files_open_cache_files"
]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/cache_files_windows/production.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/cache_files_windows/cache_files_windows.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/cache_files_windows/cand.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/cache_files_windows/cand.obj",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/cache_files_windows/LEDGER.md"
]

## production_changes
Apply C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/cache_files_windows/production.patch (git apply --check is clean against the 931ed8dc worktree; index blob 4c8eadd2). The full replacement file is scratch/w/cache_files_windows/cache_files_windows.c (CRLF, like the worktree). The patch touches source/cache/cache_files_windows.c only. It changes no headers, config or symbols.json.

The edit is inside cache_files_open_cache_files:
(1) In the failure arm of the stale-file setup, delete the two lines `CloseHandle(file);` and `file = INVALID_HANDLE_VALUE;`. The arm keeps `valid = FALSE;` and the 811 match_vassert.
(2) Insert this block after the `else { match_vassert(... 823 ...) }` arm and before `map_file->file = file;`:
		if (!valid && file != INVALID_HANDLE_VALUE)
		{
			CloseHandle(file);
			file = INVALID_HANDLE_VALUE;
		}
		(followed by a blank line)

Behaviour is the same on all four paths.

Integrator bookkeeping:
- Retire the config/parked.json entry for unit source/cache/cache_files_windows, function _cache_files_open_cache_files (class unclassified). Its target normalized sha f1c88310c585856cc88840f5dfa06dfa579231bca2e1f57c06cf7d00803c7a3f equals the candidate's.
- No semantic_matches entry is needed. The only function still below 100% in objdiff is _cache_files_precache_map_status (91.52542, jump table). It is strictly exact and already in build/semantic_report.json accepted_ledger.

## evidence
Target: 234 instructions. Base: 232. Only two differences: (a) January has a shared reload block J1 at 0x1ef, reached by SUCC's jmp and FAIL's fall-through; ours reloads ebx separately in each arm. (b) January places the 823 CreateFileA-failure arm after the epilogue.

New primary evidence:
(1) The October 2001 Xbox XBEs under research/ were parsed as data (scratch xbe.py).
- oct-betaP.xbe (DEBUG) at 0x1bd5f0 has January's exact layout, with the same 12-line assert gap (761/773).
- oct-default.xbe (RELEASE, no asserts) at 0x158a40 still has J1.
- A release-config lab of our source (rel.py) reproduces the October release, so the setup body itself is right.
(2) Labs e2/L_ld/L_le/L_la-c/L_noexists decoded the mechanism. RA builds a shared landing block only when FAIL can fall into the join, so J1 is a consequence of the 823 arm moving. Per the arm-sinking law, that happens when FAIL's continuation lies after the else arm. Identical reload sets alone do not create J1.
(3) /Od first-party build 0x857520 (saved_games\game_state_pc.c 'persistent storage') is clearly derived from this idiom: OPEN_ALWAYS, size check, blank write, SetFilePointer+SetEndOfFile, 'couldn't resize'/'couldn't open or create' vasserts. It closes the handle AFTER the whole if/else: `if (!success) {...CloseHandle(file); file=INVALID_HANDLE_VALUE;}`.

Probes:
- h7b (single-test trailing cleanup) is EXACT.
- h7a (the PC nested form) leaves an extra -1 store on the 823 path; January has none, so the single-test form is byte-proven.
- h7a_nf/h7b_nf show that the pre-existing `valid = FALSE` in the failure arm is load-bearing. It is attested by January 0x1b6 and by the October release (`xor bl,bl`).
- h6 (failure arm first, else valid=TRUE) is inert.
- h1 (inlined set-size helper) is inert.
- Flag lab: /Ob0 /Ob1 /Os /Ot /Og /Oa /Ow /Oy /Gs /Oi- /G5 /G6 never move the 823 arm.

Strip test: nothing decorative was added. The new `file != INVALID_HANDLE_VALUE` term is semantically required and byte-attested.

Section-4 audit on cand.obj:
- gate --all: exact 50, residual 0, unwritten 0. The _point_from_line3d guard passed.
- object_audit: PASS; 87 January symbols, 0 differ. The surplus (.drectve, three literals, three XDK selectany tables) is all pre-existing.
- pdb_storage: 0 disagreements.
- surplus_identity: 0 code COMDATs. provider_link: PASS 6/6.
- Data in report.json: .rdata 1192 and .bss 12412, both 100%.
- objdiff 3.3.1 (pinned, sha1 3130e428), run in scratch/w/.../od: open_cache_files at 100%, matched_code 5968/6144 (up from 5222).
- No rejection entries exist for this unit.

## blockers
None found for this object. The only item below 100% in objdiff is _cache_files_precache_map_status (91.52542%, a jump-table false negative). It is strictly exact and already credited by the generated semantic_report accepted_ledger. The candidate still needs the integrator's full-build chain: ninja, progress, stable diff, parks, admission audit, fake scan and pytest.

## reopen_criteria
n/a (exact). Revert only if the full build or stable diff shows a regression in this object. This is a .c-local, single-function change, so no other translation unit can be affected.

## task notes
Key reusable finding: the October 2001 Xbox builds oct-betaP.xbe (DEBUG) and oct-default.xbe (RELEASE, asserts compiled out) in research/ are same-family first-party evidence. Parsers are in scratch/w/cache_files_windows/xbe*.py. The release build separates assert-arm layout effects from body shape. Mechanism decoded: VC7 builds a shared reload landing block in front of a join only when a predecessor that needs a reload can fall into it. The arm-sinking law decides whether it can, and a statement after an if/else that VC7 threads per path puts FAIL's continuation after the else arm. The derived /Od function 0x857520 is a good example of Bungie's trailing handle-cleanup idiom. Other file-open/setup functions in the parks may carry the same idiom.
ledger: C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/cache_files_windows/LEDGER.md
