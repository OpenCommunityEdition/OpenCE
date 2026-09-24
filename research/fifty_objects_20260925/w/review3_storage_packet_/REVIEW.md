# REVIEW (wave 3): source/hs/hs storage/ownership packet (worker scratch/w/hs, verdict ADMISSION_FIX_PROPOSED)

Reviewer slug: scratch/w/review3_storage_packet_/. Base 954eebd2. Notes appended as I go.

## R1. Patch application + gates
- git apply --check: production.patch OK, storage.patch OK (tree clean for hs.c / symbols.json).
- Applied production.patch (my own apply; content == worker cand.c modulo EOL) -> gate --all: exact 447 residual 1
  unwritten 0 (residual _real_random_range_evaluate [sha]) == HEAD gate (head.obj).
- storage.patch alone (cand_storage.c) -> gate --all: exact 447 residual 1 unwritten 0.

## R2. DEFECT in production.patch: the cleanup hunk adds a surplus header-inline COMDAT
- Section multiset cand.obj (production.patch) vs head.obj: +1 `.text` COMDAT `_real_random_range` (32 B,
  2 relocs). storage.patch alone: section multiset IDENTICAL to head.obj (only .debug$S path differs).
- January's hs.obj does NOT define `_real_random_range` (selected provider: action_alert).
- surplus_identity (re-pointed at cand.obj): "1 candidate-only code COMDATs" (_real_random_range IDENTICAL to
  action_alert's). provider_link(cand.obj): PASS. object_audit(sim split, cand.obj): `+.text _real_random_range 32`.
- The worker's claims "every section byte-identical to build/base; only storage differs" and "surplus_identity:
  0 code COMDATs" are FALSE for production.patch (the worker ran surplus_identity on build/base, not the candidate).
- Rule 6 / brief s2: a shared-header __inline may emit a COMDAT ONLY if its caller is strictly exact. The caller
  `_real_random_range_evaluate` is the residual -> the cleanup hunk is NOT admissible now (owner-gated class,
  same shape as the held actor_combat P1 item). Drop it from the production packet.

## R3. csplit simulation (independent)
- Control csplit (unedited config) into split_ctrl: 833/833 byte-identical to build/split.
- Edited symbols.json into split_edit: only source/hs/hs.obj differs; section diffs 0; exactly 838 symbols
  storage 2->3; name set == static_symbols.txt.
- symbols.json edit: 838 lines, each exactly `, "static": true` inserted before ` }`, CRLF preserved, line count equal.
- object_audit(split_edit vs cand_storage.obj): 1830 January symbols, 0 differ; only DIFF = _real_random_range_evaluate
  .text; surplus = .drectve + 20 literal COMDATs (no code COMDAT).
- Storage vs cachebeta publics on cand_storage.obj and split_edit hs.obj: 922 symbols, 1 disagreement
  (_hs_enumerate_scenario_data external, PDB says static) -- the disclosed anomaly.

## R4. The `_hs_enumerate_scenario_data` "anomaly" is mis-explained by the worker
- Worker: "an unreferenced function is only emitted with external linkage -> must stay external".
- Board census (pubcensus.py over all 833 split objects): 1,709 UNREFERENCED external functions ARE in the
  cachebeta publics (so the linker lists unreferenced externals), and 148 Halo functions are unreferenced AND
  non-public (e.g. main.c `main_*_private`, object_lights `light_attenuation`). That class is exactly the VC7
  static-emission law (memory: vc7-static-comdat-law): a static function is emitted iff the source USES it; when
  every use is inlined the body is still emitted, with zero references.
- Image scan of cachebeta.exe (data only): no dword 0x4B34D0 and no E8/E9 rel32 to it anywhere -> truly unreferenced.
- January hs.obj: the 11 scenario enumerators (script, ai, ai_command_list, starting_profile, conversation,
  object, trigger_volume, cutscene_flag, cutscene_camera_point, cutscene_title, cutscene_recording names) are each
  48 B with relocs exactly [global_scenario_index, global_scenario_get, hs_enumerate_block_data] == the helper's
  own body relocs (48 B).
- /Od 0x5f52c0 = hs_enumerate_scenario_data(block_offset movsx, name_offset, element_size) with local block
  [ebp-4]; its callers (odbuild callers 0x5f52c0) are 12 one-line enumerators, e.g. /Od 0x5e7da0
  `push 0x5c; push 0; push 0x49c; call 0x5f52c0`.
- => January: `static` hs_enumerate_scenario_data CALLED by the enumerators and inlined at every site. Our source
  hand-expands the helper in 11 callers (section-8 (g) blocker class) and keeps the helper external and unused.
  Hypothesis H-R4: static helper + 11 one-line calls is byte-exact and closes the last storage disagreement.

## R5. H-R4 probe (r4a.c = storage.patch + static helper + 11 one-line calls)
- Evidence before compile: PDB (absent from publics), image (unreferenced), VC7 static-emission law, January relocs
  of the 11 callers == helper body relocs, /Od 0x5f52c0 + its 11 one-line callers (od_scenario_callers.txt:
  e.g. (0x49c,0,0x5c) scripts, (0x42c,0,0xb0) encounters, (0x360,4,0x60) trigger volumes, (0x4e4,4,0x5c) cutscene
  flags ...; name_offset 4 exactly where ours has 4). Sept-2001 atlas maps name all 11 enumerators + block_data.
- gate --all: exact 447 residual 1 unwritten 0; `_hs_enumerate_scenario_data` EXACT 48 (still emitted: static+used);
  all 11 callers EXACT 48.
- r4a.obj vs cand_storage.obj: sections POSITIONALLY identical (bytes + raw reloc records), symbol table identical
  except storage of _hs_enumerate_scenario_data 2->3.
- symbols.json amended (839 lines: the worker's 838 + `_hs_enumerate_scenario_data`), csplit into split_edit2:
  only hs.obj differs from control; 0 section diffs; exactly 839 storage flips.
- object_audit(split_edit2, r4a.obj): 1830 January symbols 0 differ; only DIFF = _real_random_range_evaluate .text;
  surplus = .drectve + 20 literal COMDATs; NO code COMDAT.
- cachebeta publics vs r4a.obj AND vs split_edit2 hs.obj: 922 symbols, 0 disagreements (worker packet: 1).

## R6. Final amended packet (production_amended.patch) checks
- git apply --check vs current tree: OK; round-trip (patch -> file) == r4a.c; gate final.c: 447/1/0; final.obj ==
  r4a.obj (sections + symbols).
- /W3 census (w3.py, CL /Zs /W3): head 58, storage 58, worker cand 58, r4a 58; zero added/removed warning lines.
- fake_match_scan: 0 leads (head, worker cand, r4a).
- surplus_identity re-pointed at final.obj: 0 candidate-only code COMDATs. provider_link(final.obj): 20 literal
  COMDATs PASS, SELECTED-PROVIDER LINK PASS.
- objdiff 3.3.1 mini report (od/): hs_cur, hs_final (split_edit2 vs final.obj), hs_worker all = 25248/25329 code,
  447/448 fns, data 18/54798 -> score-neutral (ownership-only packet).
- Consumers: no header touched. grep over source/**/*.c,*.h (excluding hs.c): 0 hits for any of the 418 functions or
  421 definitions. No other January split object names any of the 839 symbols. January hs_runtime.obj imports all
  39 `__hs_type_*_default` as UNDEF (section 0) -> external definitions are link-required.
- Static consistency: every prototype+definition line of the 418 static functions carries `static`; all 345
  macro-generated evaluators are in the static set (no public evaluator is generated by an HS_EVALUATE macro);
  duplicate macro #defines (VOID_UNSIGNED_SHORT, VOID_BOOLEAN) both got `static` (no C4005 added).
- HCEX spot checks: static in HCEX: hs_compile_source, alphabetize, hs_allocate, game_safe_to_save_evaluate,
  hs_enumerate_ai_names, hs_enumerate_block_data, hs_enumerate_navpoints; DataIsGlobal+public: _hs_type_boolean_default,
  hs_function_table_count. hs_rebuild_source is external in HCEX (2011) but absent from January publics -> January
  truth (static) kept; disclosed. hs_enumerate_scenario_data absent from HCEX by exact name.
- House rules on the diff: only `static` add/remove + the 11 callers collapsed to one /Od-attested call each (removes
  hand-expanded helper bodies with the odd double-assignment pointer arithmetic). No new block scope, no new locals,
  no casts, no decoration -> strip test N/A (static is PDB-attested storage; the call form is the only way VC7 emits a
  static helper -- a static unused helper is dropped, measured by the worker as UNWRITTEN).

## VERDICT
- Worker packet AS PROPOSED (production.patch + 838-line symbols.json): REJECT. (1) cleanup hunk emits a surplus
  `_real_random_range` COMDAT in hs.obj whose caller is not strictly exact (rule-6 exception not met; owner-gated,
  like actor_combat P1) and the worker's "every section byte-identical / 0 code COMDATs" claims are false for it;
  (2) `_hs_enumerate_scenario_data` left external on a wrong mechanism ("only external unreferenced functions are
  emitted") -- refuted by the VC7 static-emission law + board census + /Od callers.
- AMENDED packet: APPROVE -> scratch/w/review3_storage_packet_/production_amended.patch (applies to 954eebd2) +
  scratch/w/review3_storage_packet_/config/symbols.json (839 in-place `, "static": true` edits; list in
  static_symbols_amended.txt). Integrator: derive line ops by diff vs config/symbols.json, csplit-only regen.
  Fallback if the scenario_data hunk is not wanted: scratch/w/hs/storage.patch + scratch/w/hs/config/symbols.json
  (838) -- also verified exact/storage-only, leaves 1 PDB disagreement.
- Owner item (not landed): the /Od-attested `real_random_range(arguments[0].real_value, upper_bound)` cleanup of the
  residual real_random_range_evaluate (byte-identical caller; emitted `_real_random_range` COMDAT IDENTICAL to
  action_alert's selected copy; provider link PASS) -- needs the caller exact or an owner ruling.
