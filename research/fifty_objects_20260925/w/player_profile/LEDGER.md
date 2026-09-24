# player_profile worker ledger (lane claude/fifty-objects-20260925, wave 3, base 954eebd2)

Targets: _player_profile_create_default_profiles_on_disk (288), _player_profile_write_thread_proc@4 (352),
_player_profile_new (464). Prior ledgers read: player_profile_obj_* (wrapper_pair, public_quintet,
highest_level, opus5 w2/n1/n4/n5), player_profile_s3tc_batch_reconciliation_20260907,
saved_game_family_reconciliation_20260909 (owner REJECTED exact bodies: uninit padding, close(&file,NONE),
default-profile seek-fail leak), player_profile_color_return_20260924, parked.json entries (3 parks).
Recorded negatives NOT to repeat: union file-block (n1 pp_v4, 336->320 for default; new 19 relocs [sha]),
both branch polarities (inert), two-variable result (refuted by push esi == returned reg), static buffer /
csmemset (27->28 relocs), shorter file_write (format change).

## Log
- [baseline] gate --all at 954eebd2: exact 19 / residual 3 / unwritten 0. Residuals:
  create_default [size 336!=288, sha], new [relocs 18!=19, sha], write_thread_proc@4 [size 384!=352, sha].
  (argb_color is now EXACT via the 20260924 return-by-value packet.)
- [NEW /Od evidence, player_profile_new] odbuild str -> fn 0x85d0c0 (od_new.txt). RTC: profile_file 280,
  block 8192. Block init = `mov byte [block],0; memset(block+1,0,0x1fff)` => byte-array `block = {0}` (1-byte
  head store, same class as January's `mov byte [ebp-0x204],bl; lea edi,[ebp-0x203]`). Failure path at
  0x85d1df..0x85d20c: error("failed to initialize..."); delete_enumerated_saved_game_file(index);
  index = NONE (0x85d1fa); THEN saved_game_file_close(&profile_file, index) at 0x85d20c, shared by both arms
  (success arm jumps 0x85d1dd -> 0x85d201). => the later first-party source ALSO closes with NONE on write
  failure: the owner-rejected ordering is first-party attested in two builds (Jan bytes + /Od).
  Condition shape: `if (!(file_set_position(&f,0) && file_write(&f,size,block)))`.
- [NEW /Od evidence, create_default] fn 0x85acf0 (od_default.txt). RTC: file 280, full_path 256, block 8192.
  NO initialiser on block (uninitialised, only build_default_profile(block,i,0) + checksum fill it).
  Control: if (file_reference_create_from_path(&file,path,FALSE) && <bool=1>) { checksum;
  if (file_create && file_open(&file,2) && file_set_position(&file,0)) { success = file_write(...);
  if (!file_close(&file)) error("failed to close..."); } } if (!success) error("failed to create/update...").
  => seek folded into the && chain (open-then-seek-fail leaves the file open) and uninitialised block are
  BOTH first-party attested. January bytes (dis_range) have exactly this topology, minus the extra boolean.
- [NEW /Od evidence, write thread] "begin player profile write" absent from /Od; the body lives in /Od fn 0x8596d0
  (od_write_thread.txt; later refactor without mutex/begin/end). RTC profile_file 280, block 8192 (no
  initialiser; later format copies 0x1ffc profile bytes + 4-byte checksum at block+0x1ffc, so the later block
  is fully written; January's 0x30+0x14 of 0x200 is not). `failed` ([ebp-5]) = FALSE at top; failure arm
  sets failed=TRUE; close && !synchronize -> error; else "failed to open"; and `if (failed) delete(index)` sits
  AFTER the open if/else (0x8597f2), not inside the open-success arm as in production.
- [probe wt1, cand_wt1.c] hypothesis: the rejected donor's single +0x10b byte (je disp 09 vs 21) is the
  branch binding of `if (failed)`: production binds `je` to the open-failure arm's release_mutex copy (0x146)
  while January binds to its own arm's copy; /Od places `if (failed)` after the open if/else, which gives
  the open-failure arm a jump-threaded path of its own. Change: move `if (failed) delete` after the else
  (defined, /Od-attested; fill kept). RESULT: write_thread still [size 384!=352, sha] (fill), but the tail
  binding now matches January (`test bl,bl; je <own release copy>`; base binds je 0x146 = other arm).
  All 19 EXACT rows unchanged.
- [lab wt2, lab_wt2.c = wt1 + uninitialised block, OWNER-GATED lab only] RESULT: _player_profile_write_thread_proc@4
  EXACT (gate exact 20 / residual 2). => the +0x10b byte (documented since 20260909 as unexplained) is SOLVED by
  the /Od statement placement; the ONLY remaining blocker for write_thread is the uninitialised-padding ruling.
- [HCEX types] DIA2Dump: player_profile_new locals `struct file_reference profile_file`,
  `union player_profile_block block`; `union player_profile_block { unsigned char data[0x200];
  struct player_profile_internal profile_block; }`; `struct player_profile_internal { struct player_profile
  profile; unsigned char checksum[4] @+0x7BC }` (HCEX format differs; January checksum = XCALCSIG_SIGNATURE @+0x30).
  create_default HCEX locals: char full_path[0x100], union player_profile_block block, struct file_reference file.
  player_profile_read: profile_file, block (union), checksum, sanitized_profile. => the n1 union spelling is
  FIRST-PARTY typed (name + member order: raw byte data FIRST, which is exactly what January's 1-byte head
  store of `= {0}` in player_profile_new requires). /Od RTC names agree (block / profile_file / full_path).
- [storage] pdb_storage: `_player_profile_primary_colors` split=2 ours=2 but ABSENT from cachebeta publics;
  only player_profile.obj references it in build/split (grep) => January file-static. Object blocker #4
  (symbols.json "static": true + `static` in source). Data sections otherwise 100% (report.json .bss 108,
  .data 72, .rdata 1464).
- [cand_def.c] (mk_cands.py) defined/house-clean: HCEX `union player_profile_block {byte data[512];
  struct player_profile_internal profile_block;}` + `struct player_profile_internal {profile; checksum;}`
  replaces the invented padded struct; /Od RTC local names profile_file (new, write thread) and full_path
  (create_default); write thread `if (failed)` after the open if/else (/Od 0x8597f2); new closes the valid index
  in both arms (n1 pp_v1 structure: success close / error; delete; close; index = NONE). RESULT: 19 EXACT
  unchanged; create_default [size 320!=288, sha] (was 336); new [sha] only (was [relocs 18!=19, sha]; frame
  0x310 = January); write thread [size 384!=352, sha] with January's tail binding.
- [cand_og.c] OWNER-GATED exact form = cand_def + (a) no initialiser on the write-thread and create_default
  blocks (BUG comments), (b) create_default seek folded into `file_create && file_open && file_set_position`
  (BUG comment, /Od 0x85ae02), (c) new: `if (!set_position || !write) {error; delete; index = NONE;}` then
  `saved_game_file_close(&profile_file, player_profile_index)` (BUG comment, /Od 0x85d1fa/0x85d20c).
  RESULT: gate `== exact 22 residual 0 unwritten 0`. WHOLE OBJECT FUNCTION-EXACT under the owner-gated forms.
- [HCEX compiland ..\build\x360\SHIP\halo\player_profile.obj] (hcex_player_profile_compiland.txt):
  `static long profile_color_table[0x12]` = File Static, long[18] = our 72-byte `_player_profile_primary_colors`
  (.data 0x316880). => authentic name + storage: rename to `profile_color_table`, `static`, symbols.json
  "static": true (cachebeta: absent from publics; split: referenced only by player_profile.obj).
  player_profile_globals is a HCEX global and a cachebeta public (external, correct). HCEX private fns
  create_default/read/write are static (ours static already).
- [symbols.json] scratch/w/player_profile/config/symbols.json: line 22790 edited in place
  `_player_profile_primary_colors` -> `_profile_color_table`, + "static": true (single-line diff).
  csplit-only regen into scratch (splitcfg/ = config copy + edited symbols.json; `csplit.exe -i cachebeta.exe
  -p splitcfg -o splitsim "source\saved games\player_profile.obj"`): simulated split differs from the current
  split only in that symbol (name + storage 2->3).
- [cand_og2.c] = cand_og + `static long profile_color_table[...]` (+ get_rgb_color use, header comment).
  vs CURRENT split: 21/22 (get_rgb_color [reloc-identity], expected until regen). vs SIMULATED split
  (simgate.py): 22/22 EXACT. audit_sim.py (object_audit with AUDIT_TARGET=simulated split):
  OBJECT AUDIT: PASS, 51 January symbols 0 differ (audit_og2_sim.txt).
  cand_store.c (storage/name packet ONLY on production) vs simulated split: 19/3 = production rows unchanged.
- [labs] lab_nopad (remove invented `byte pad[3]` from the globals type; HCEX _player_profile_globals has no pad
  member; size 0x6C preserved by alignment) and lab_hcexnames (HCEX type/member names
  player_profile_write_internal_input{index,profile}, _player_profile_globals{...thread_input...}) are both
  byte-inert on cand_og2 (22/22 vs simulated split). pad removal adopted; HCEX type names left optional.
- [final_og.c] = cand_og2 + nopad: vs sim split 22/22; audit_sim PASS (51 symbols 0 differ, all sections ok);
  provider_link SELECTED-PROVIDER LINK PASS (8 surplus: 4 pooled literals, 3 D3D tables, __real@3b808081);
  surplus_identity 0 code COMDATs; /W3 no TU-local warnings; fake_match_scan 0 leads.
  objdiff 3.3.1 (odproj/, target = simulated split): code 4119/4119, 22/22 fn, data 1644/1644, fuzzy 100.
- [final_def.c] = cand_def2 + color table + nopad (DEFINED, landable at zero credit): vs sim split 19/3
  (production rows). objdiff fuzzy: write_thread 88.407->87.566, new 76.567->81.26, create_default
  83.795->82.659; object fuzzy 95.3416->95.7141. (wt1 alone: write_thread 88.451.)
- [/Od context] later build_default_profile (0x85a040) memsets 0x1ffc bytes; later block = 0x1ffc profile +
  4-byte checksum, so the later build's equally-UNINITIALISED block declaration is fully overwritten. In
  January the same declaration leaves 0x200-0x30-0x14 = 444 bytes of stack in the file: the defect is a
  format-size artefact of an authentic uninitialised declaration (not UB: indeterminate bytes read as byte).
- [final_min.c] minimal defined packet = production + profile_color_table static/rename + pad removal +
  /Od `if (failed)` placement. vs sim split 19/3; new + create_default bytes/sha UNCHANGED (parks stay valid);
  write_thread 384/27 sha bbb652e0... objdiff 88.451324 (was 88.40708) -> re-baseline that one park.
- [patches] (all `git apply --check` clean on 954eebd2; stacking verified in a temp repo, EOL-normalised):
  production.patch = final_min (.c + symbols.json line 22790);
  production_defined_full.patch = final_def (+ HCEX union + /Od local names; .c + symbols.json);
  production_owner_gated.patch = final_og (whole object exact; .c + symbols.json);
  owner_gated_over_production.patch / owner_gated_over_defined_full.patch = incremental .c-only diffs.
  No header change (player_profile.h untouched; profile_color_table has no other referencing TU in source or
  in build/split).
- [not adopted] cand_def.c's new() (n1 pp_v1 two-arm close: error; delete; close; NONE) reaches objdiff 90.13 but
  deletes the enumerated file while it is still open, contradicting the park's close-before-delete criterion;
  kept production's close -> delete -> NONE order in every defined candidate.

## Summary / stop (methodology stop rule: remaining gap is policy, not machine code)
- Landable now (zero code credit): production.patch = HCEX-proven `static long profile_color_table` (rename +
  storage; needs symbols.json line 22790 + csplit regen) + removal of invented `byte pad[3]` + /Od-attested
  `if (failed)` placement in the write thread. 19 EXACT rows unchanged (vs simulated split); re-baseline only
  the write-thread park (384/27, sha bbb652e083aadc92..., objdiff 88.451324).
- Owner-gated (report only): production_owner_gated.patch makes the WHOLE OBJECT exact: 22/22 functions,
  object_audit PASS (51 symbols 0 differ), data 1644/1644, provider link PASS, 0 code COMDATs. It needs
  three rulings, each now FIRST-PARTY ATTESTED by the later /Od build (new evidence since the 20260909
  rejection): (R1) uninitialised 512-byte block in write thread + create_default (/Od 0x8596d0, 0x85acf0:
  no initialiser; HCEX union player_profile_block); (R2) create_default folds file_set_position into the
  open condition so a failed seek skips file_close (/Od 0x85ae02); (R3) player_profile_new sets the index to
  NONE before the shared saved_game_file_close (/Od 0x85d1fa -> 0x85d20c).
- Reopen: owner admits R1-R3 (then apply owner_gated_over_production.patch, retire the 3 parks, promote
  config/config.json line 400 to Matching after full ninja + sweep). Without the rulings no defined source
  can reach January's bytes: R1 needs no fill instructions (any defined initialisation adds code or
  relocations - measured by earlier waves: static buffer/csmemset 27->28 relocs), R2 needs the close skipped,
  R3 needs NONE pushed as the close argument (push esi == returned register).
