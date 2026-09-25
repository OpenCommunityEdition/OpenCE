# LEDGER - review_critic3_hs_runtime (adversarial review of critic_3 patch 14: hs_runtime_recompile_error -> script_error)

- Start: HEAD 5d3ca708, git status clean except untracked research/. Claimed source/hs/hs_runtime (review only).

## R1 first-party name evidence (independent)
- Sept-2001 cachebeta.map: `_script_error` 0001:000b9320 hs_runtime.obj, in the STATIC symbols section (map line 20741 > "Static symbols" 19268).
  Neighbours by address: _hs_global_reconcile_write (0xb9190) -> _script_error (0xb9320) -> _hs_runtime_get_executing_thread_name (0xb9360):
  same anchors as January (0xbab50 -> 0xbace0 -> 0xbad20), gap 0x40 = January padded size.
- Sept retail cache.map: `@script_error@12` (fastcall, 3 args = 12 bytes -> 3 params, matches (thread_index, reason, expression)).
- Aug-2001 cachebeta.map: `_script_error` 0001:000ac5a0 hs_runtime.obj, static section.
- Own XBE byte compare (xbecmp.py, VA = 0x11000 + .text offset): Sept cachebeta.xbe 0xca320 and Aug 1749betaP.xbe 0xbd5a0 vs January split
  `_hs_runtime_recompile_error`: 64 bytes, 6 relocations, ZERO non-relocation byte differences (xbecmp_sept.txt / xbecmp_aug.txt).
  Sept relocation operands resolve via the Sept map to the same targets: _hs_thread_data (0008:002e7304), _datum_get (0001:00106a40),
  "no reason given." (0007:0002d734), _hs_thread_format (0001:000b8460), "script %s needs to be recompiled" (0007:0002d708), _error (0001:0007cf40).
- January: not in cachebeta.pdb publics (static; consistent). cachebeta.exe strings: no literal names either `script_error` or
  `hs_runtime_recompile_error`; body string "script %s needs to be recompiled. (%s: %s)" is compatible with the Sept name.
- HCEX.pdb 2011: neither `script_error` nor `hs_runtime_recompile_error` (nor hs_script_error / hs_runtime_script_error) exists; no contradiction.
- Current name `hs_runtime_recompile_error` has no first-party attestation (invented descriptive).

## R2 references (git grep of all tracked files)
- Only source/hs/hs_runtime.c (symbol-listing comment L80, prototype L530, call L3094, definition L3113) and config/symbols.json L3220.
  docs/object_matching_logs/hs_runtime_obj_opus_reconciliation_20260904.md:34 mentions the old name (historical log; not a reference).
  No parked.json / semantic_matches.json / semantic_data_matches.json / object_admission_rejections.json / symbol_ownership.json / tools / tests reference.
- Patch touches all 4 source occurrences + the symbols.json line in place (static:true kept, order untouched). git apply --check OK at 5d3ca708.

## R3 emulated split (own csplit runs; build/tools/csplit.exe -i cachebeta.exe -p cfg -o slug)
- split_ctl (config copy) == build/split 833/833 byte-identical.
- split_R (patched symbols.json) vs split_ctl: ONLY source/hs/hs_runtime.obj differs; objcmp.py (all sections incl. .debug$S, relocs, symbols):
  IDENTICAL under rename map; storage stays 3 (static) at the same symbol index.

## R4 gate + object compare
- gate prod (source vs build/split): 65 exact / 1 residual (_hs_evaluate_begin_random [sha]).
- gate cand (patched .c vs split_R via with_split redirect copy): 65 / 1; row SET identical modulo rename (only row ORDER moves: gate sorts by name).
- prod.obj == build/base hs_runtime.obj (all non-.debug$S sections, relocs, symbols). cand.obj vs prod.obj: IDENTICAL under rename map
  (non-debug sections, relocations, symbol table order/storage). .debug$S differs (source path/name) as expected.

## R5 section-4 battery (battery_prod.txt vs battery_cand.txt; candidate via with_split.py copy -> split_R, --base-obj cand.obj)
- object_audit: FAIL(1) both (the pre-existing parked `_hs_evaluate_begin_random` [sha] residual); 129 January symbols, 0 differ, both.
- pdb_storage: 129 split symbols, 0 disagreements, both (`_script_error` absent from cachebeta publics -> static 3; split 3; ours 3).
- surplus_identity: 5 candidate-only code COMDATs, 0 not identical, both. provider_link: PASS both (both link orders inside the tool).
- Whole battery output set-identical modulo the rename (only sort position / column width of the renamed row moves).
- Data: all non-.debug$S sections byte+reloc identical (objcmp), so data sections unchanged; no relocation anywhere (split or ours)
  references the renamed symbol (January inlines its body into hs_arguments_evaluate; the out-of-line copy has no caller).
- objdiff 3.3.1 (sha1 3130e428, own objdiff/ project): prod, prod_base and cand all 11374/14604 code, 61/66 fns, 11940/11940 data,
  97.997246%; the renamed function 100.0 under either name. Zero credit, as the packet claims.

## R6 callers / composition / coupling
- E8 + absolute-reference scans: no caller of the function in January cachebeta.exe (VA 0x4bace0), Sept cachebeta.xbe (0xca320) or Aug
  1749betaP.xbe (0xbd5a0): all three builds inline it at the only use and keep the unreferenced static copy. Consistent pairing.
- /Od later build (odbuild str/fn/callers): 0x607750 is a real standalone function with the same shape (thread from hs_thread_get,
  reason ? reason : "no reason given.", error(2, ...), return FALSE), one caller 0x600760. No name there (no symbols) - shape only.
- January atlas (4cc87b45): no record at 0x4bace0 (neighbour _hs_global_reconcile_write at 0x4bab50 exact) -> no contradicting name.
  Other halo_ce atlas builds: `@script_error@12` hs_runtime.obj (tiers exact/exact/map). No atlas build carries hs_runtime_recompile_error.
- git apply --check of patch 14 alone at 5d3ca708: OK. Sequential GNU patch of all 21 critic_3 patches on HEAD blobs (compose/): all apply;
  composed hs_runtime.c == patch-14-only hs_runtime.c. Sibling hs_runtime packets (converter storage, render_debug_trigger_volumes,
  review_r2 amended) are already in HEAD (git apply -R --check OK) -> no pending conflict.
- Coupling: patched source vs UNregenerated build/split -> 64 exact + `_hs_runtime_recompile_error` UNWRITTEN. Source and symbols.json
  edits MUST land in one commit followed by the csplit regen (patch 14 carries both).
- Process slip disclosed: ran tools/campaign/relocdiff.py once with no arguments before reading it; argparse printed usage and exited
  (no side effects; git status clean afterwards). Also one broken shell chain (path with a space) aborted before any write outside the
  slug; git status verified clean.

## R7 verdict
- APPROVE (no amendment). First-party name proven by two independent pre-January maps with byte-identical bodies, identical relocation
  targets, identical anchor neighbours, static storage everywhere; every tracked reference updated; byte-inert (gate/objcmp/battery/objdiff);
  zero credit. Only remaining old-name mention: historical prose in docs/object_matching_logs/hs_runtime_obj_opus_reconciliation_20260904.md:34
  (a dated ledger; leave as history).
