# review_critic_3_king LEDGER (adversarial review of critic_3 patch 11: hill_points_to_2d -> point3d_to_point2d)

Worktree HEAD 5d3ca708 (task text names 7b9de585). Claimed source/game/game_engine_king as review_critic_3_king.

## R1 patch integrity
- `git apply --check` of critic_3 patches/11 at HEAD 5d3ca708: rc 0 (7b9de585..5d3ca708 = one docs ledger only; 7b9de585 is an ancestor).
- Candidate built by exact textual replacement (4 .c occurrences, 1 symbols.json row) == LF patch application in applytest/ (diff clean).
  Change is identifier-only: symbol-listing comment, static prototype, definition, the one call in find_hill, symbols.json 2303 (static kept).

## R2 name evidence (independent)
- Raw maps: Aug-2001 cachebeta.map L19997, Sept-2001 cachebeta.map L21371 (both in the "Static symbols" section, between
  _king_engine_dispose and _find_hill, size 0x20, game_engine_king.obj) = `_point3d_to_point2d`; Sept-2001 RETAIL cache.map L15620
  `@point3d_to_point2d@12` (3 dword args = the 3 params). No other object in any map defines this name (no collision).
- Raw BYTES (sept_body.py/.txt): January split `_hill_points_to_2d` (32 B, 0 relocs) is BYTE-IDENTICAL to Sept cachebeta.xbe
  (.text+0x9ebb0), Aug 1749betaP.xbe (.text+0x94d20) and Sept retail cache.exe (0x475e70). critic_3's "0.929" is a
  capstone op_str artifact (absolute branch target rendered with the load address, 13/14 insns); the true relation is identity.
- The helper is inlined at its only site in find_hill in January AND in Sept (no call/reloc to it in either find_hill) and still
  emitted out-of-line: consistent with round-1 law "unreferenced static still emitted if source uses it".
- Current name origin: invented at the Fable King wave (9d2372ea/f68c6e25); HCEX.pdb has neither name (find_hill only);
  Jan atlas has none at the address; cachebeta.pdb publics: neither name (static); cachebeta.exe strings: no token
  "point3d_to_point2d"/"hill_points"/"points_to_2d". Nothing contradicts the Sept name.
- /Od later build (evidence only): the same loop is a real call 0x5bdce0 from 0x5bcc80 whose RTC locals are hill_points_3d /
  hill_points_2d (likely inspiration for the invented name); no symbol name. It pushes (count, points3d, points2d) - a
  PRE-EXISTING parameter-order observation, not in this patch's scope (January exact 29/29 with current order; out-of-line
  body uses ecx=points3d, edx=points2d, esi=count in Jan, Sept and retail alike).

## R3 measurements
- csplit (build/tools/csplit.exe, config copies in slug): split_ctl == build/split 833/833; split_cand differs ONLY in
  game_engine_king.obj; objcmp (all non-.debug sections bytes+relocs+symbols, name-mapped) IDENTICAL; unmapped = 1 symbol name.
- gate: prod vs build/split 29/29 EXACT; cand vs emulated split 29/29 EXACT; rows identical modulo the rename.
  (cand vs stale build/split: 28 exact + `_hill_points_to_2d` UNWRITTEN => csplit regen is REQUIRED at integration.)
- compiled prod.obj vs cand.obj: objcmp IDENTICAL name-mapped (74 sections, 197 symbols; only .debug$S differs).
- object_audit: prod vs build/split PASS; cand vs emulated split PASS; (cand vs stale split FAIL(2) = the rename, control).
- pdb_storage: 23 disagreements prod == cand (pre-existing held king storage packet; helper static both sides, not public).
- surplus_identity: identical output (10 candidate-only code COMDATs, 0 not identical). provider_link both orders: PASS, identical.
- objdiff 3.3.1 (sha1 3130e428): prod 4865/4865 code, 29/29 fns, 740/876 data == cand; helper 100% under both names.
- No strip-test construct, no new scope/declaration, no cast: nothing for /Od declaration-order review.

## R4 integration hazards
- HELD round-1 owner packet research/fifty_objects_20260925/w/owner_queue/ub_king_glow/king/patches/02_king_symbols.patch
  carries line 2303 `_hill_points_to_2d` as CONTEXT: after patch 11 it FAILS (hunk 1). Rebased copy (one context line):
  held_king_02_symbols_rebased_after_p11.patch (dry-run OK after 11). 01, 01A, if_declined, 03-05 still apply after 11.
- No tracked tool/test/config other than symbols.json references the old name (docs logs are history).

## R5 verdict: APPROVE (unchanged patch). Zero credit, byte-inert, authentic first-party name.
