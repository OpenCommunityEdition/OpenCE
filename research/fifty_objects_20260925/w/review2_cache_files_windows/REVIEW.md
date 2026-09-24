# Review2 cache_files_windows (adversarial review of OBJECT_COMPLETE_CANDIDATE)
- started; tree 931ed8dc

## Independent re-verification (reviewer)
- Patch: `git apply --check` CLEAN against current tree 931ed8dc. patch(HEAD file) == worker cand.c (LF) == worker
  cache_files_windows.c (CRLF, after CR strip). Only source/cache/cache_files_windows.c touched; no header/config/symbols.
- gate.py --source cand_crlf.c --all --out review cand.obj: `== exact 50 residual 0 unwritten 0 (of 50)`.
  Reviewer obj differs from worker obj only in the gate temp filename strings (_gate_<pid>) + timestamp.
- alndiff open_cache_files: T 234 / O 234; the only listed row is the `01.01.14.2342` literal reloc attribution
  (symbol vs defined-noncode), which is the pre-existing /GF literal surplus (normalizes equal; gate EXACT).
- object_audit: PASS, 87 January symbols 0 differ; surplus (.drectve, buffer/header/01.01.14.2342 literals,
  3 D3D selectany tables) IDENTICAL to production build/base obj surplus (production audit FAILs only on open_cache_files).
- pdb_storage: 87 split symbols, 0 disagreements. surplus_identity: 0 code COMDATs. provider_link(cand.obj): PASS 6/6.
- objdiff 3.3.1 (build/tools sha1 3130e428, reran in review2/od): cand matched_code 5968/6144, data 13604/13604,
  only <100 fn = _cache_files_precache_map_status 91.52542 (semantic-coff accepted_ledger entry exists). prod 5222.
- fake_match_scan on candidate: 0 leads.
- January bytes: 0x6b valid=0 at loop top, 0x1b6 valid=0 in failure arm (pre-existing `valid = FALSE;` attested),
  0x1e2 CloseHandle(esi) + 0x1e8 file=-1 on failure path, J1 0x1ef, 823 arm 0x2b1 jmp 0x1f5 (skips J1, no -1 store).
- /Od 0x857520 (worker od_857520.txt) read independently: trailing `if(!success){ if(file!=-1) CloseHandle(file);
  file=-1; }` after the whole open/resize if-else; the PC failure arm has NO success=FALSE store.

## Reviewer labs (strip / robustness; lab only, under review2/lab/)
- s1_nested  `if (!valid) { if (file != INVALID_HANDLE_VALUE) { CloseHandle; file = INVALID; } }` -> EXACT 752.
- s2_swapped `if (file != INVALID_HANDLE_VALUE && !valid)` -> EXACT 752.
  => the match does not hinge on one decorated spelling; any semantically equivalent single-guard form matches.
- s4_pc_nested (PC /Od literal form: `file = INVALID` unconditional inside `if (!valid)`) -> residual
  (extra ebx reload / J1 not formed), confirming worker's h7a claim; January has no -1 store on the 823 path.
- s5_nofileguard (`if (!valid) { CloseHandle(file); file = INVALID; }`) -> residual AND semantically wrong
  (would CloseHandle(INVALID_HANDLE_VALUE) on the CreateFile-failure path) => the guard is required, not decoration.
- s3_novalidfalse (drop the PRE-EXISTING failure-arm `valid = FALSE;`) -> residual, 237 insns, frame slots move
  => load-bearing; independently attested by January 0x1b6 `mov byte [ebp-1],0` and October release 0x158b8e `xor bl,bl`.
- October DEBUG (oct-betaP 0x1bd5f0) spot-checked: J1 0x1bd7df reload pair, 773(0x305) arm after `ret` jmp 0x1bd7e5,
  failure arm stores valid=0 then CloseHandle + file=-1 - same layout as January. Corroborative only (same bytes shape).

## House-rule / owner-gate review of the diff
- Adds no declarations (no name/decl-count oracle risk), no casts, no parens, no volatile, no goto, no pragma, no asm.
- Behaviour on all 4 paths identical to production (exists-ok, stale-setup-ok, stale-setup-fail, CreateFile-fail).
- Style: tabs, Bungie braces, INVALID_HANDLE_VALUE constant, boolean `!valid`; function keeps explicit `return;`.
- Owner-gated classes (sec 7): none touched (no uninit reads, no view casts, no header edits, no dead locals, no temps).
- Prior reopen criterion (three_leaf_object_closeout 20260919): "source/donor explaining the cold CreateFileA-failure
  placement without moving the success store" - met by first-party /Od analog + strict exact (success store 0x1a4 in place).
- cache_files_open_cache_files is static; absent from cachebeta publics (consistent). No symbols.json change.

## Verdict: APPROVE (OBJECT_COMPLETE_CANDIDATE confirmed)
Integrator: apply production.patch (or copy the CRLF full file); retire config/parked.json entry
source/cache/cache_files_windows::_cache_files_open_cache_files (target sha f1c88310... == candidate sha, verified by tinfo).
No semantic entries needed (precache_map_status already semantic-coff accepted). D3D selectany surplus = systemic precedent
(57-58 Matching objects), provider_link PASS.
