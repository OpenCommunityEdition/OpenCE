# cache_files_windows wave-2 worker ledger (fifty-objects lane, tree 931ed8dc)

## Baseline
- gate --all on production: `== exact 49 residual 1 unwritten 0 (of 50)`; only `_cache_files_open_cache_files` 752 [sha].
- alndiff base (alndiff_base_full.txt): T 234 / O 232 insns. Two differences only:
  (1) January keeps an inner join J1 @0x1ef (`mov ebx,[ebp-0x14]; mov esi,[ebp-0xc]`) with preds SUCC(jmp) + FAIL(fallthrough);
      ours reloads ebx in each of SUCC/FAIL and jumps straight to the outer join J2.
  (2) January sinks the 823 CreateFileA-failure vassert arm after the epilogue (je 0x2b1 ... jmp 0x1f5); ours inline.
- Read prior evidence: reaudit 20260912, opus5 w2/w3, Lane D REPORT (claude-lane-d-refresh-20260922/.../cache__cache_files_windows),
  structural lane REPORT (claude-structural-20260923/.../cache__cache_files_windows, ~100 lab probes).
  Key prior finding: a conditional at the END of the else-of-exists block (p2 `if (deleted) Sleep(0);`) reproduces
  January's layout exactly except its own 5 insns -> January's IL has a conditional/extra content in J1 that leaves 0 bytes.
  Recorded negatives (do NOT repeat): X1-X5 success-flag copies, y1 valid=valid, y2 if(!deleted) deleted=TRUE, p4/p5/p6
  dead/empty ifs, t1-t5 valid threading, v_a/v_b, c1-c4, nc_1..64 decl-count, d1/d2, e1-e3, h2a/h2b split map_file->file
  stores, q1-q5, u1/u2/w1, Lane D shapes A/B, scope sweep, vassert(valid) family; opus5 rh1-4/hb1/cv1/ce1-5.
- /Od build + HCEX: no open_cache_files analog (checked again: /Od 0x5183a0 / 0x518cf0 are PC map-load / read_header;
  HCEX has cached_map_file_read_header + cache_files_initialize only, no locals).

## Session findings (this worker)
- NEW EVIDENCE SOURCE: the October 2001 Xbox builds in research/ (DATA only, parsed with scratch xbe.py).
  * oct-betaP.xbe (DEBUG, asserts on): open_cache_files at VA 0x1bd5f0 (oct_open_cache_files.txt). Layout is IDENTICAL
    to January: J1 reload block @0x1bd7df (`mov ebx,[ebp-0x14]; mov esi,[ebp-0xc]`, preds SUCC jmp + FAIL fallthrough),
    823-equivalent arm sunk after ret. Assert lines 0x2f9/0x305 (761/773): the same 12-line gap as January 811/823;
    cached_map_file_get/get_size asserts 1157/1181 (January 1208/1232) -> same text, file shifted by 51 lines.
  * oct-default.xbe (RELEASE, asserts compiled out, /Oy, intrinsics): open_cache_files at VA 0x158a40
    (oct_default_open.txt). Even with NO 823 arm, J1 exists: SUCC `mov bl,1; jmp J1`, FAIL `push ebp; xor bl,bl;
    call CloseHandle; or ebp,-1`, J1 `mov esi,[map_file_index]`, exists-valid jumps past J1 to J2.
- rel.py (release-config lab: asserts #defined away, /Oy /Oi): our production body reproduces the October release
  SUCC/FAIL/J1/J2 structure exactly (base_rel.lst). So the setup region's source is right; the DEBUG-only difference is
  which edges share compensation. In release SUCC and FAIL need the SAME reload set ({map_file_index}) -> one J1.
  In our DEBUG build SUCC needs {ebx} but FAIL needs {ebx, esi(file)} (esi still holds `file` from the 0x182 reload
  on the SUCC path) -> reloads go into each pred, no J1, FAIL must jmp over the 823 arm -> no sink.
  January DEBUG reloads esi on the SUCC edge too, i.e. at SUCC's end RA did not consider esi == `file`.
LAB h1 (SFP/SEOF via inlined static set_size helper): byte-identical to base (param coalesced) -> hypothesis 'distinct IL variable for SFP/SEOF' refuted in this form
- LAB e2 (drop `file = INVALID` in FAIL): FAIL still reloads esi at its end; SUCC does not -> SUCC/FAIL compensation
  asymmetry is not caused by FAIL's def. (lab only)
- HYPOTHESIS H6 (line-count + layout evidence): January wrote the setup test NEGATIVELY with the failure arm FIRST and
  `else { valid = TRUE; }` LAST. Bungie brace style line count from the 811 vassert: 812 CloseHandle, 813 file=,
  814 }, 815 else, 816 {, 817 valid= TRUE;, 818 }, 819 }, 820 }, 821 else, 822 {, 823 vassert -> EXACTLY 823
  (current SUCC-first order counts to 819; the October build has the same 12-line gap). Not in the negative list
  (L1-L3/X5/lab5 had NO else arm and valid=TRUE hoisted; t3 inverted the EXISTS test, not the setup test).
- LAB h6 (fail-first + else valid=TRUE): output byte-identical to base -> VC7 normalizes the arm order. INERT (line-count fit is text-only).
- LAB L_la/L_lb/L_lc (a call / a use of `file` / call-before-store in SUCC): SUCC still does NOT reload esi.
- LAB L_ld/L_le (FAIL reduced to CloseHandle(+file=INVALID), no assert, no valid store): FAIL STILL reloads esi at its
  end. So FAIL's esi reload is structural (FAIL = multi-pred else arm), not caused by its def/asserts/calls.
- LAB L_noexists (exists arm removed): SUCC and FAIL now need IDENTICAL compensation ({esi,edi}) yet NO shared J1 and
  NO 823 sink -> identical compensation alone does not create J1; the shared landing block needs FAIL to fall into
  J2 (823 already out of the way) or a CONDITIONAL pred into the join at RA time. Refined model (from structural-lane
  sink table: p2/q1/q2/L1-L3 sink; p3/r3/q4/p1 do not): the 823 arm is displaced when RA must build a LANDING block
  in front of J2 for an edge that cannot take appended compensation (a conditional pred) inside the stale arm; the
  landing block is laid out immediately before J2 and FAIL (fallthrough) shares it with SUCC (union -> SUCC's redundant
  esi reload). January's final bytes show no conditional pred into J1, so the conditional edge must exist at RA time
  and vanish afterwards (or J1 must be created by something else).
- FLAG LAB (diagnostic): /Ot /Og /Oa /Ow /Oy /Gs /Oi- /G6 /G5 /Ob0 /Ob1 /Os: none sinks the 823 arm. Not a flag.
- NEW EVIDENCE (/Od PC build 0x857520, saved_games\game_state_pc.c "persistent storage"): a later first-party function
  clearly DERIVED from this idiom (CreateFile OPEN_ALWAYS; size-ok -> success=TRUE; else write blank buffer,
  SetFilePointer+SetEndOfFile -> success=TRUE else vassert("couldn't resize...", line 1051); else-arm
  vassert("couldn't open or create...", line 1059)); the handle cleanup is NOT in the failure arm but AFTER the whole
  if/else: `if (!success) { if (file!=INVALID_HANDLE_VALUE) CloseHandle(file); file= INVALID_HANDLE_VALUE; }`.
  HYPOTHESIS H7: January had the same trailing cleanup. VC7 threads the cleanup test per path, so FAIL's continuation
  (CloseHandle; file=-1) sits AFTER the 823 else arm in IL -> the else arm's exit (J2) differs from the then arm's
  fall-through (cleanup) -> 823 is sunk (sink law) and the cleanup/compensation becomes the shared J1.
- PROBE h7b (FAIL arm = `valid = FALSE; vassert(811)`; trailing `if (!valid && file != INVALID_HANDLE_VALUE)
  { CloseHandle(file); file = INVALID_HANDLE_VALUE; }` before `map_file->file = file;`): **EXACT 752** (gate row EXACT).
  h7a (PC-literal nested form `if (!valid) { if (file!=INVALID) CloseHandle(file); file = INVALID; }`): residual,
  823 COLD, 7 norm-diff (extra -1 store on the 823 path). h7a_nf/h7b_nf (no valid=FALSE in FAIL): 18 diffs (valid=0
  store missing) -> the explicit `valid = FALSE;` in the failure arm is load-bearing and byte-attested (0x1b6).
- CANDIDATE cand.c (= lab/h7b.c): gate --all == exact 50 residual 0 unwritten 0. Release-config lab (rel.py) of cand.c reproduces the October release FAIL/J1 shape exactly (push ebp; xor bl,bl; call CloseHandle; or ebp,-1; J1 reload) -> no contradiction from the second first-party binary.

## RESULT: _cache_files_open_cache_files EXACT; whole object 50/50 (OBJECT_COMPLETE_CANDIDATE)
Candidate: scratch/w/cache_files_windows/cache_files_windows.c (full file, CRLF like the worktree) == cand.c (LF lab copy).
Patch: scratch/w/cache_files_windows/production.patch (git apply --check: OK against 931ed8dc worktree; index blob 4c8eadd2).
Source change (only this function; no header/config/symbols change):
  - failure arm of the stale-file setup keeps `valid = FALSE; match_vassert(811 ...)` but no longer closes the handle;
  - new trailing cleanup after the `if (file != INVALID_HANDLE_VALUE) ... else { vassert 823 }`:
        if (!valid && file != INVALID_HANDLE_VALUE) { CloseHandle(file); file = INVALID_HANDLE_VALUE; }
    immediately before `map_file->file = file;`. Behaviour identical on all four paths.
Evidence chain: (1) January layout needs FAIL's continuation to lie AFTER the 823 else arm (sink law: else arm exit !=
  then-arm fall-through) -> the handle cleanup is a separate statement after the if/else, threaded per path by VC7;
  (2) first-party /Od build 0x857520 (game_state_pc.c persistent storage, derived from this idiom) has exactly this
  trailing `if (!success) ... CloseHandle(file); file= INVALID_HANDLE_VALUE;` cleanup; (3) January's single-test form
  is proven by bytes: the PC nested form (h7a) stores -1 on the 823 path, January does not; (4) `valid = FALSE` in the
  failure arm is attested by January 0x1b6 and by the October release build (`xor bl,bl` in the failure arm);
  removing it costs 18 insns (h7b_nf) - it was already in production. (5) October DEBUG build (oct-betaP 0x1bd5f0)
  has the identical layout; release-config lab of the candidate matches October release (oct-default 0x158a40).
Strip test: nothing decorative was added; the only new condition term (`file != INVALID_HANDLE_VALUE`) is semantically
  required (never CloseHandle the invalid handle on the CreateFile-failure path) and byte-attested (h7a vs h7b).
Section-4 audit on cand.obj:
  1 gate --all: == exact 50 residual 0 unwritten 0 (emitted-symbol guard _point_from_line3d passed).
  2 object_audit: PASS (all January sections/symbols ok; 87 January symbols, 0 differ). Surplus = .drectve,
    "buffer"/"header"/"01.01.14.2342" literals and 3 XDK selectany tables - all pre-existing in production.
  3 pdb_storage: 87 split symbols, 0 disagreements (no storage change in the candidate).
  4 surplus_identity: 0 candidate-only code COMDATs; provider_link: SELECTED-PROVIDER LINK PASS (6/6).
  5 data: report.json .rdata 1192 100%, .bss 12412 100% (matched_data 13604/13604).
    objdiff 3.3.1 (pinned, sha1 3130e428) on cand.obj in scratch/w/cache_files_windows/od/: open_cache_files credited
    100% (matched_code 5222 -> 5968 of 6144). The only objdiff<100 function left is _cache_files_precache_map_status
    (91.52542, jump-table representation; strict EXACT and already in build/semantic_report.json accepted_ledger).
  6 no object_admission_rejections / semantic_credit_rejections entries for the unit.
Park to retire: config/parked.json `source/cache/cache_files_windows` / `_cache_files_open_cache_files` (unclassified).
Target normalized sha f1c88310c585856cc88840f5dfa06dfa579231bca2e1f57c06cf7d00803c7a3f, 752 B, 47 relocs = candidate.
