# review_critic3_main LEDGER - adversarial review of critic_3 patches/15_main.patch

Tree 5d3ca708, clean (only the pre-existing untracked research/ dir). Claimed source/main/main as review_critic3_main.
No tracked edits, no ninja/configure/git mutation. All outputs in this directory.

## R0 patch shape
- `git apply --check scratch/w/critic_3/patches/15_main.patch` at repo root: rc 0.
- Applied to index-form (LF) copies in patchroot/: diff vs HEAD = exactly symbols.json line 4107 (name only, "static": true
  kept, in-place, order untouched) + main.c listing comment line 30 (`_code_000ef8a0` -> new name) + prototype (670),
  qsort argument (830) and definition (1552). Body untouched. No other tracked reference exists: git grep over the tree finds
  the old name only in main.c, config/symbols.json, generated build/report.json + build/semantic_report.json, and historical
  research/ notes. Not in parked.json, semantic_matches.json, semantic_data_matches.json, admission rejections, tools, docs.

## R1 name evidence (independent)
- Raw first-party linker maps, all three name `_sort_desired_local_player_controllers` in main.obj, in the Static symbols
  section, between main_get_difficulty and create_local_players, size 0x40:
  Aug-2001 cachebeta.map 0x4da130; Sept-2001 cachebeta.map 0x4edba0 (.text+0xed7c0); Sept-2001 retail cache.map 0x4b50d0.
  January: 0xef8a0 size 0x40 between main_get_difficulty (0xef890) and create_local_players (0xef8e0).
- Body identity (bodyid.py/bodyid.txt): January split 64 bytes == Sept cachebeta.xbe .text+0xed7c0 64 bytes, IDENTICAL.
  Sept retail cache.exe = same logic without a frame pointer; Aug xbe = an earlier branch arrangement.
- Caller (callers.py/callers.txt): January's only reference is create_local_players+0x157 (dir32 reloc); Sept xbe has the
  function's VA as an imm32 at .text+0xed957 = create_local_players+0x157. Same site.
- Current name provenance: atlas January rows (4cc87b45 exe, b4963176 xbe) carry `?sort_controllers_ascending@@YAHPBX0@Z`
  at tier "exact" (propagated). index.csv: the January builds have n_map=0; the ONLY Halo CE map-tier builds are the two
  Sept-2001 ones, and both give `_sort_desired_local_player_controllers`. The mangled name is C++ (`int __cdecl (void const *,
  void const *)`) from a later C++ lineage (Reach TU1 row is blamlib game.obj; map-tier origin in halo_2/3/reach/4 symbol
  files that are not in the local release). METHODOLOGY: a class with two map names is resolved by majority (~0.8% of
  classes, "a function renamed across versions"), which favours the many later-game builds. January main.obj is C, and
  every symbol is C-decorated, so the mangled spelling cannot be January's symbol. It entered the tree through owner commit
  9f74e993 ("Import 909 authenticated January and HCEA symbol names"), the bulk atlas import.
- HCEX.pdb: no `*sort*controller*`, `*controllers_ascending*`, `*ascending*` (Halo) symbol; create_local_players exists
  (static, 0xA4). Neutral.
- January cachebeta.exe strings: neither name occurs. The create_local_players assert text `(desired_controllers[i]>=0) && ...`
  (PDB public string) uses the `desired ... controllers` vocabulary. Consistent with the Sept name, but not proof.
- cachebeta publics: the function is not public -> static (the patch keeps "static": true; the source keeps `static`).
- Residual uncertainty: bytes cannot rule out a rename between Sept-2001 and Jan-2002. The Sept name is the nearest
  same-language first-party record with an identical body. The alternative is a cross-game majority-vote propagation
  whose signature (void const * / int) does not even match the C prototype.

## R2 measurements
- csplit (build/tools/csplit.exe -i cachebeta.exe -p <cfg> -o <slug dir>): split_ctl (unmodified config copy) == build/split
  833/833 byte-identical; split_cand vs split_ctl: only source/main/main.obj differs, and objcmp.py shows it identical
  modulo the rename (sections, relocations, symbols; storage 3 both sides; no old name left).
- gate: prod (gate_prod.txt) 92/2/1; cand via with_split.py redirect to split_cand (gate_cand.txt) 92/2/1. Rows identical
  modulo the rename (`_main_frame_rate_debug` [relocs 51!=52, sha], `_main_update_time` [sha], `_main_crash` UNWRITTEN).
  `_sort_desired_local_player_controllers` EXACT 64.
- objcmp compiled prod.obj vs cand.obj: IDENTICAL modulo rename (all non-.debug$S sections: bytes + relocations; symbol
  table). prod.obj vs build/base main.obj: only .debug$S differs (object path).
- tinfo --data: split_ctl == split_cand; prod vs cand differ only in the .debug$S gate object path.
- Section-4 battery (battery.txt), prod vs cand identical modulo rename: object_audit FAIL(6) both (chucky string + main_crash
  MISSING, 2 residual DIFF rows; the renamed row is `ok` 64/64); pdb_storage 1 disagreement both (_main_game_render,
  pre-existing); surplus_identity 2/0 not identical; provider_link SELECTED-PROVIDER LINK PASS (both orders) both.
- objdiff 3.3.1 (build/tools/objdiff-cli.exe sha1 3130e428) mini project objdiff/: prod and cand both 11128/13117 code,
  92/95 fns, 2659/4507 data, 99.74965%; the comparator is 100% under both names. Zero credit confirmed.

## R3 house rules / strip test
- Nothing added: no casts, parentheses, declarations, or scopes. The pre-existing qsort cast is unchanged. No new scopes
  (the /Od declaration-order check does not apply). The prototype stays in main.c (static, own file). One parameter per line.
- Follow-up (not in scope): main.c's symbol-listing comment still has 26 stale `_code_` placeholder rows, e.g.
  `_code_000ef8e0` for create_local_players, which symbols.json already names.

## Verdict
APPROVE as-is (zero credit, name-only). Integrator note: this reverses an owner-imported atlas name (9f74e993). Disclose the
atlas majority-vote provenance when landing.
