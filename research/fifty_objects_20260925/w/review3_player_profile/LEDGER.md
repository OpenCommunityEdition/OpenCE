# review3_player_profile ledger (adversarial review of scratch/w/player_profile, base 954eebd2)

- [start] read WORKER_BRIEF 0-8, house rules, lane log, saved_game_family 20260909 rejection, 3 parks, worker LEDGER.
- [patches] git apply --check OK on 954eebd2 for production / production_defined_full / production_owner_gated; each applied
  to HEAD blobs reproduces final_min.c / final_def.c / final_og.c byte-for-byte; symbols.json copy = 1-line in-place diff (line 22790).
- [baseline gate] production 19/3 (create_default [size 336!=288, sha], new [relocs 18!=19, sha], wt [size 384!=352, sha]).
- [vs CURRENT split] final_min 18/4, final_def 18/4, final_og 21/1: the extra residual is get_rgb_color [reloc-identity]
  (expected: target still names _player_profile_primary_colors).
- [own csplit sim] splitcfg = config/{config,contribs,relocs,splits,symbol_ownership}.json + worker symbols.json.
  CONTROL (unedited symbols.json) regen == build/split byte-identical. FULL regen with edit: 833 objects, ONLY
  player_profile.obj differs; its symbol diff = _player_profile_primary_colors(st 2) -> _profile_color_table(st 3), .text sec 6 relocs.
- [vs SIM split, own simgate.py (gate.py logic)] base 18/4 (get_rgb reloc-identity); final_min 19/3 (create_default 336/14
  sha 3ce0cc6f = base, new 464/18 sha 4df7452a = base, wt 384/27 sha bbb652e0...); final_def 19/3 (cd 320/14 c4f4ba33, new
  464/18 4f42ddba, wt 384/27 2d428375); final_og 22/22 EXACT. All worker numbers REPRODUCED.
- [HCEX] DIA2Dump confirms union player_profile_block {unsigned char data[0x200]; struct player_profile_internal profile_block;},
  player_profile_new locals profile_file + block(union); _player_profile_globals {default_profile, thread_input, thread, initialized}
  (no pad); compiland File Static long[0x12] profile_color_table. cachebeta publics: nothing between 0x31620C and 0x316AA8
  (0x316880 absent => static); _player_profile_globals public.
- [audit vs SIM] own audit_sim.py (= scratch/tools/object_audit.py + AUDIT_TARGET): final_og PASS (51 syms 0 differ, every
  January section ok); final_min / final_def FAIL(3) = only the 3 residual .text sections; 51 syms 0 differ in all.
  Surplus identical in base/min/def/og: .drectve + 4 pooled literals + 3 D3D tables + __real@3b808081 (NO new surplus).
- [provider_link] final_og / final_min / final_def: SELECTED-PROVIDER LINK PASS (8 surplus). surplus_identity (unit): 0 code COMDATs.
- [pdb_storage production] 1 disagreement: _player_profile_primary_colors split 2 ours 2 PDB-public False -> packet fixes it.
- [objdiff 3.3.1, own odproj vs SIM] base 95.341644 (wt 88.40708 / new 76.566666 / cd 83.795456); final_min 95.34539
  (wt 88.451324, others unchanged); final_def 95.71412 (wt 87.56637 / new 81.26 / cd 82.65909); final_og 100.0,
  code 4119/4119, data 1644/1644, 22/22. All worker figures REPRODUCED.
- [/W3 /Zs] base/min/def/og: identical 12 header-only warnings (cseries.h C4146, real_math.h C4244); 0 TU-local.
- [fake_match_scan] final_min/def/og: 0 leads.
- [/Od] odbuild fn 0x8596d0 / 0x85d0c0 / 0x85acf0 reproduce the worker's od_*.txt exactly (non-comment lines).
  "begin player profile write" absent from /Od; 0x8596d0 is the later synchronous write (params index, profile;
  RTC profile_file/block), `if (failed) delete` at 0x8597f2 after the open if/else.
- [lab, January-byte attestation of the `if (failed)` move] uninit block + production in-arm placement
  (lab_inarm_uninit) = [sha], ONE raw byte +0x10b (target 09, ours 21) = the 20260909 donor byte; uninit block + /Od
  placement (lab_moved_uninit = worker lab_wt2) = EXACT 352. => the placement is attested by January bytes AND /Od.
- [January bytes, R3] target _player_profile_new failure path: error; delete(index) @0x151; `or esi,-1` @0x159;
  close(&file, esi) @0x164; `mov eax,esi` return. Two close copies (0x164 / 0x180). R3 genuine in January.
- [section-8 scan final_og] no consumer-local prototypes (prototype block = 5 TU statics), no float, no hand-expanded
  helpers, no new block scopes. Advisory: globals type tag/members are descriptive, not HCEX
  (_player_profile_globals / player_profile_write_internal_input{index,profile} / thread_input).
- [lab min_hcexnames = final_min + HCEX type/member names] vs SIM 19/3 with IDENTICAL shas (3ce0cc6f/4df7452a/bbb652e0);
  whole object identical except .debug$S. Byte-inert. Optional patch hcex_names_over_production.patch (applies over
  production.patch; NOTE it changes context lines used by owner_gated_over_production.patch, so B's incremental would need
  regenerating if both land).
- [verdict] packet A APPROVE (zero code credit; storage/name blocker fixed; 1 park re-baseline); production_defined_full
  acceptable optional alternative (3 park re-baselines, mixed fuzzy); packet B NOT landable (owner-gated R1-R3), evidence
  verified and correctly disclosed.
