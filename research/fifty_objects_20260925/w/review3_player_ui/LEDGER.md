# review3_player_ui adversarial review ledger (base 954eebd2)
Target: worker result scratch/w/player_ui (OBJECT_COMPLETE_CANDIDATE), patches 01..06 + optB.
## Reading done
- WORKER_BRIEF 0-8, house rules, lane log (held items), player_ui reconciliation ledger + rejection entry, worker LEDGER.md.
## Patch application (reviewer)
- git apply --check 01..06 against current tree: all OK; optB --check OK on top of 01-06 applied copy.
- Applied 01..06 in sequence to a copy (applied/): all 11 candidate .c equal cand_A2 (EOL-normalised); symbols.json equals worker copy.
## Independent sweep method (tcomp.py / fullsweep.py)
- Full copy of source/ at tree/ with 01..04 applied; each unit compiled IN PLACE in the tree with build.ninja cflags,
  /I source* rewritten to the tree (no shadowing tricks); compared to build/base per section index (name/flags/size/
  bytes/relocs by symbol name, .debug$S path record skipped) + full symbol table; plus January exact count.
- Controls (tree=. i.e. production source): player_ui, game_engine, game_engine_race IDENTICAL to build/base.
## R1 full-board sweep (tree = production source + patches 01..04), 454 source units
- 453/454 IDENTICAL to build/base (sections by index incl. bytes/relocs + symtab; compiler-local $L/$name$N label numbering
  normalised: 27 units shift label ordinals only). Only player_ui differs: _player1_profile_path symbol + 2 fns' reloc
  target/addend (expected). January exact counts unchanged in all 454. Production control (tree=.) 454/454 IDENTICAL.
- CONSUMER CENSUS (reviewer /showIncludes on all 454): game_engine.h 38, player_ui.h 14, playlist_profile.h 5 -> 47 units.
  Worker list (45) MISSES source/interface/ui_widget_game_data_input_functions (direct #include of BOTH player_ui.h and
  game_engine.h; absent from worker includes.json, which lacks 16 build units) and saved games/saved_game_files
  (playlist_profile.h only; unchanged header -> harmless).
- NEW DIAGNOSTICS: ui_widget_game_data_input_functions gains 2 x C4133 (lines 2119, 2757: '=' incompatible types from
  'game_variant *' to 'playlist_profile *') - its TU-local `struct playlist_profile` view receives the retyped getter.
  Bytes identical, but patch 03 introduces a type-incompatible implicit pointer assignment in a consumer = defect.
## R2 evidence checks
- HCEX -type: game_variant/universal_variant (explicit pad0..2, pad4..6 CONFIRMED)/ctf/slayer/king/oddball/race variants
  confirmed field names+order; HCEX union also has `terminator_variant terminator {long ignored}` (omitted by worker; no
  January user); HCEX oddball/race members are enum-typed (worker: long); flags long (worker keeps production unsigned long).
- HCEX player1_profile_path File Static char[0x100]; _player_ui_globals has no pad members (pads removal consistent).
- relocs.json: +0x230 referenced 6x only by _player_ui_remember_player1_profile(3)/_get_player1_last_used_profile_index(3);
  max other offset +0x22C (initialized) -> struct 0x22D -> 0x230 natural size. CONFIRMED.
- January strings present: 'failed to retrieve editable game variant' (uwehf split), race assert literal (race split).
## R3 split regeneration (real csplit, not simulation)
- build/tools/csplit.exe -i cachebeta.exe with config copy: control reproduces build/split 833/833 byte-identical;
  with patch-05 symbols.json ONLY player_ui.obj changes.
- object_audit vs regenerated split: candidate PASS (68 January symbols, 0 differ); production FAIL(1) MISSING
  _player1_profile_path -> 04+05 must land together (confirmed).
- objdiff 3.3.1 mini-project: candidate vs new split == production vs old split (41/42 raw + known semantic entry
  _set_local_player_controls_from_player_profile 95.89041; data 2004/2004).
- surplus_identity 0; provider_link PASS (3 strings); string_audit: each of the 3 is UNDEFINED in January player_ui,
  section_infos_equal to the selected provider copy, selection 2 both sides. pdb_storage 0 disagreements. fake scan 0.
## R4 strip tests
- path uninitialised: path@.bss0, globals@0x100, 40/42 (2 fns lose) -> `= { 0 }` load-bearing.
- both globals+path uninitialised: globals becomes COMMON (no .bss) -> fails. The chosen spelling is the unique plain
  working form; matches the file's existing `player_ui_globals = { 0 }` and 86 file-scope `= {0}` precedents (VC7 .bss law).
## R5 oracles (NOT landing choices)
- +HCEX terminator_variant{long ignored} in the union: units _unit_preprocess_node_orientations LOST (188/189), uwgdif
  gains 1, main parked fn moves -> omission is count-consistent with January (corroborates worker's layout).
- optB (3-constant enum) on top: 45/47 identical; only main _main_update_time (parked) moves; units 189/189 -> worker claim verified.
## R6 amendments produced (reviewer)
- 07_ui_widget_game_data_input_functions_game_variant.patch: the two getter consumers use `struct game_variant *profile`
  (human_readable_game_description / game_engine_index). IDENTICAL to production (181 sections), 44/46 unchanged,
  the 2 new C4133 gone. Depends on 01+03.
- 04_player_ui_c_amended.patch = worker 04 + refresh of 5 stale `_code_000d05xx` names in the file's symbol comment to
  the symbols.json names (comment only; object identical to A2 except .debug$S path record; audit PASS vs new split).
## R7 final (tree = 01,02,03,04-amended,07)
- Full board 453/454 IDENTICAL (player_ui differs only by the path static, as designed); /W3 net new warnings = 0 on 47.
- VERDICT: approve WITH mandatory reviewer patch 07 (+ preferred 04-amended). REVIEW.md written.
- cleanup: removed tree/, applied2/, cfg_*/, split_*/ (reproducible: apply patches; csplit -p <config+05> ); kept regenerated split_new_keep/player_ui.obj
