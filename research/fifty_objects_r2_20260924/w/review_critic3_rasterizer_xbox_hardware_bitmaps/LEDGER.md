# review_critic3_rasterizer_xbox_hardware_bitmaps LEDGER (adversarial review of critic_3 patch 20)

Base: worktree 5d3ca708. Claimed unit (verification only) as review_critic3_rasterizer_xbox_hardware_bitmaps.
Patch under review: scratch/w/critic_3/patches/20_rasterizer_xbox_hardware_bitmaps.patch
  - symbols.json line 5810: _rasterizer_bitmap_cube_map_changed -> _rasterizer_bitmap_cm_changed (static kept)
  - .c: symbol-listing comment (_code_00158120 -> _rasterizer_bitmap_cm_changed), prototype, definition, one call site.

## R0 start
- claim.py claim -> CLAIMED (previous state: released by critic_3). git status at start: clean except untracked research/.
- git apply --check patch 20 at 5d3ca708: rc 0 (blob indexes 885b3b3d symbols.json / a62f75f5 .c match HEAD).

## R1 first-party name evidence (independent)
- Sept-2001 beta cachebeta.map (ts 3bb0dabe): 0001:00155dd0 `_rasterizer_bitmap_cm_changed` f rasterizer_xbox_hardware_bitmaps.obj;
  next symbol _rasterizer_bitmap_delete 0x155fe0 -> size 0x210 = January 0x210. Neighbours 2d_changed (0x1c0) / 3d_changed (0x210)
  line up with January 0x157d50 (0x1c0) / 0x157f10 (0x210) = our already-landed 2d/3d names.
- Aug-2001 cachebeta.map (ts 3b7b3218): 0001:0013b980 `_rasterizer_bitmap_cm_changed`, size 0x210.
- Sept-2001 RETAIL cache.map (ts 3bb0db06): `@rasterizer_bitmap_cm_changed@4` (fastcall in that config) - third first-party map.
- January atlas 4cc87b45 (2002-01-14 cachebeta.exe): off 0x558120 = `_rasterizer_bitmap_cm_changed` tier exact, between
  _rasterizer_bitmap_3d_changed (0x557f10) and _rasterizer_bitmap_delete (0x558330). Current name appears in NO atlas build.
- cachebeta.pdb publics: neither name public (file static; symbols.json keeps "static": true). HCEX.pdb: neither name.
- Body identity (bodyid.py/bodyid.txt, reloc fields of the January split masked): Sept beta xbe 528/528 bytes identical,
  195/195 instructions; Aug xbe differs only in 3 bytes = the assert line immediate (0x110 vs January 0x114).
- Callers (E8 scan): Sept xbe sole caller 0x16707d inside _rasterizer_bitmap_changed; January exe sole caller 0x5583cd
  inside _rasterizer_bitmap_changed (0x558360..). Same in-object dispatcher.
- January strings: no literal names either spelling; `cm` = Bungie's cube-map abbreviation in January text
  (rasterizer_xbox `default_cm_hardware_format` assert strings x3) and Sept map bitmap_utilities `_bitmap_cm_*`. Nothing contradicts.
- Independent second proposer: research/.../triage_rasterizer_a LEDGER P7 + r2w1 TRIAGE (same rename, delta 0xECB0).

## R2 references
- grep (excluding scratch/): only symbols.json:5810, the .c (prototype 111, definition 542, call 715) and the historical
  docs ledger docs/object_matching_logs/rasterizer_xbox_hardware_bitmaps_obj.md:65 (table row; historical record, not updated
  by the patch - integrator may add a rename note). No parked.json / semantic / ownership / rejection / tools / tests reference.
  parked.json unit entry is _rasterizer_bitmap_new (unaffected). Patch updates all 3 code refs + listing comment line.
- Sibling overlap: patch 19 symbols hunk 5783-5789 vs 20 at 5807-5813: disjoint. hwgeom 02_symbols (context 5810) ALREADY
  landed at 5d3ca708 (lines 5813.. carry "static": true), so no textual conflict.

## R3 emulated split + gate (tools unmodified; csplit -p slug cfg -o slug split)
- split_ctl (unmodified config copy) == build/split 833/833. split_cand vs split_ctl: only hardware_bitmaps.obj differs;
  objcmp with rename map: IDENTICAL (32 sections, 96 symbols; storage 3 kept); without map only the symbol + 1 reloc name.
- gate prod (vs build/split): 18 exact / 1 residual (_rasterizer_bitmap_new [sha]). gate cand (cand.c vs split_cand via
  with_split.py): 18/1, rows identical modulo the name (gate_prod.txt / gate_cand.txt).
- compiled objects: build/base == prod.obj (IDENTICAL); prod.obj vs cand.obj with rename map IDENTICAL (46 sections, 114 symbols).
- parked _rasterizer_bitmap_new measurements (target 041cef84/400/26, base 71946946/400/26) unchanged under split_cand/cand.obj.

## R4 battery (battery.txt)
- object_audit prod FAIL(1) == cand FAIL(1) (only _rasterizer_bitmap_new DIFF; 32 January symbols, 0 differ both).
- pdb_storage 0 disagreements both. surplus_identity 0/0 both. provider_link PASS both orders, both (5 pre-existing surplus).
- objdiff 3.3.1 (sha1 3130e428) mini report objdiff/: prod and cand both 1875/2263 code, 18/19 fns, 1584/1584 data, 99.99379%.

## R5 house rules / strip / Od
- Identifier-only change; no new construct, cast, scope or declaration -> strip test and /Od order not applicable.
- Rule 7: authentic first-party name replaces an invented descriptive one (both compliant; authentic preferred, and brief 8
  allows first-party names to bypass the descriptive-name owner gate).
- Pre-existing, NOT introduced: consumer-local prototypes texture_cache_bitmap_delete / rasterizer_error / extern
  global_d3d_device (rule 9), and symbol-listing placeholders _code_00157d50/_code_00157f10 + D3D thunk _code_ rows.

## R6 verdict
APPROVE patch 20 unchanged (no amendment needed). Zero credit. Integrator notes: stable_verdicts per-function diff will
show a name swap (cube_map_changed -> cm_changed, both EXACT 528), not a regression; csplit regen required with the .c edit.
