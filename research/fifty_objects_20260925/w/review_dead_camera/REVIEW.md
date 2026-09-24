# Review of dead_camera (FUNCTIONS_EXACT_OBJECT_BLOCKED)
started Wed Sep 23 18:46:42 PDT 2026

## R1 patch integrity
- production.patch applied to worktree HEAD (de77a780) -> 708198eb == final_prod.c. owner_update_bug.patch on top -> 7186c901 == final_full.c; full_object.patch -> 7186c901. Patches apply cleanly.

## R2 independent gate
- applied_full.c: EXACT 4/4 (new 288, update 1248, next_player 176, has_allies 112).
- applied_prod.c: exact 3, residual 1 (_dead_camera_update sha).
- HEAD: exact 2 residual 2.

## R3 whole-object audit (independent)
- object_audit cand_full.obj: 6/6 January sections ok; 1 symbol diff = _dead_camera_constants storage split 2 / ours 3 (the static). Surplus list identical to base object audit.
- secs.py (section_infos_equal per January section, current split): all 6 True (reloc target compare is name-based; storage only visible to object_audit).
- surplus_cand.py (my candidate-object variant of surplus_identity): all 8 code COMDATs + 11 rdata surplus byte-identical to base AND to January provider. 0 not identical.
- provider_link cand_full: FAIL(1) _valid_real_vector3d_axes2 vs bored_camera NODUP; base FAILs identically; precedent verified on static_camera, orbiting_camera, first_person_camera base objects (all Matching, same FAIL). Not a new emission.
- data: report.json .rdata 48/48 100%; tinfo --data bytes unchanged.
- pdb_storage: _dead_camera_constants PDB-public False. cachebeta publics DO list external const .rdata data (_rasterizer_global_defaults, _MOUSE_YAW_SCALE, _step_size_table, ...), 0x256AE8 absent -> file-static evidence sound. Only build/split + build/base dead_camera.obj reference the symbol; no other source file names dead_camera_constants -> static cannot break another object.
- symbols.json precedent: orbiting_camera constants (lines 14090-14092) flags 0 + "static": true, same form.

## R4 DEFECT: symbols_static.patch line endings
- config/symbols.json is LF in the index and in the worktree (0 CR); symbols_static.patch carries CRLF content lines -> `patch --dry-run` FAILS ("different line endings"). Mechanically inapplicable as delivered; would inject a CR if forced.
- Fixed copy: scratch/w/review_dead_camera/symbols_static_lf.patch (dry-run applies clean). Integrator should use it (or hand-edit line 14063).
- production.patch / full_object.patch dry-run clean on working tree.

## R5 strip tests (my own compiles, gate.py --source ... --all)
| variant | change vs candidate | _dead_camera_new |
| --- | --- | --- |
| s3_prod_nostatic.c | final_prod without `static` | residual -> static is load-bearing; independently evidenced by cachebeta publics (not fake) |
| s4_prod_perarm.c | ternary replaced by HEAD per-arm `camera->unit_index = ...` stores | residual; alndiff = tail only: January `mov eax,edi; mov [esi+0x28],eax` (a phi copy) vs ours `mov [esi+0x28],edi`. The single-store phi is attested by January's own bytes AND by /Od (one store through temp [ebp-0x14] after the join). Not decoration. |
| s1_head_static_swap.c | HEAD + static + pitch store moved before timer only (per-arm tail) | residual (same tail) |
| s5_head_static_swap_ternary.c | HEAD locals kept (observer separate, distance/yaw/player_index locals) + static + swap + ternary tail | **EXACT** -> the /Od-only rewrites (observer initializer, direct distance/yaw stores, no player_index local) are byte-inert; they rest on /Od only, which is acceptable for byte-inert shape (no contradiction with January). |
Worker's stfac/ factorial (forms a/b exact in 16 contexts each; c/d/e residual) read and consistent with my s5.

## R6 evidence checks
- /Od od_new.txt re-read: observer call before line-22/23 asserts; distance/yaw fstp straight to [camera+0x14]/[+0xc] (no locals); pitch fstp [ebp-8] -> xorps -> [camera+0x10] BEFORE timer store [camera+0x1c]; switch_timer [-0xc] with inner ternary temp [-0x10]; player_index stored to camera then reloaded for player_get; unit via temp [-0x14], one store. Claims hold.
- CAVEAT (doc only): /Od reads the timers from shared literal-pool entries (0x93dd68 used for both dead_timer and singleplayer switch = 3.0f; 0x940190 = 15.0f), i.e. the later build no longer has the constants struct. /Od attests statement ORDER only, not the constants reads; January relocs (+0/+4/+8 of _dead_camera_constants) attest the struct and cachebeta publics attest static. Independent second source for pitch-before-timer: HCEA donor (2026-08-26 ledger).
- n1/w1 "pitch store must follow timer" conclusion was measured with the EXTERNAL constant; the static changes alias analysis (L hoist), so this is new evidence, not a repeat. HCEA frozen combined topology (current_player before unit, 272 B) is NOT repeated.
- od_update.txt 0x522eb5 `mov [ebp-8],0xffffffff` at switch-block top, no else arm: confirmed. Owner patch (3) correctly held for ruling (brief: report, don't land). u_bug form reproduces January 0x176 `mov eax,[ebp+0x10]`; n1 v1/v2 already proved defined initialisers cost a store.
- objdiff 3.3.1 (scratch odproj/report.json, harness validated: head_vs_split reproduces build/report.json 280 / 93.09 / 99.83): full 1792/1792 code + 48/48 data vs BOTH current split and storage-3 sim split; prod alone 557 (+277 over HEAD), data 48/48.
- prod vs HEAD: _dead_camera_update, both helpers section-identical to HEAD compile (== build/base); only _dead_camera_new changes. No header/config touched by (1); symbols change touches one symbol referenced only by dead_camera split.
- fake_match_scan: 0 leads (applied_prod, applied_full). No _point_from_line3d emission. /W3 /WX compile clean.
- House rules on the diff: params one per line, `real`, explicit `return;`, decl+init (rule 17), no new prototypes, no helper expansion, assert unchanged (match_assert), logic semantically identical (independent stores reordered; direct stores; ternary == if/else). OK.

## Verdict
APPROVE the result (FUNCTIONS_EXACT_OBJECT_BLOCKED) with conditions:
1. Use scratch/w/review_dead_camera/symbols_static_lf.patch (LF) instead of the CRLF symbols_static.patch.
2. Land (1) production.patch and (2) symbols edit together, then csplit-only regen + a full ninja before trusting stable_verdicts (stale-base hazard); re-run object_audit to confirm storage 3/3.
3. (3) stays owner-gated; (4) status flip only after ruling + regen, and the landing ledger must record the pre-existing provider-link FAIL (_valid_real_vector3d_axes2 vs bored_camera NODUP; identical in base, same as Matching static/orbiting/first_person_camera).
