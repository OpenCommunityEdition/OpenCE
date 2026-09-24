# draw_string worker ledger (lane claude/fifty-objects-20260925)

Unit: source/text/draw_string. Missing: _bitmap_draw_string (304 padded, park unclassified).
Slug dir did not exist at start of this (resumed) run -> fresh start.

## Prior evidence read
- config/parked.json entry: safe candidate 240/9 vs target 304/9, objdiff 51.18; NULL-bounds path reads absolute VA 0..6.
- draw_string_obj_opus5_100k_waveD_20260914.md: exact ONLY with BUG (original) body (evidence_bitmap_draw_string_null_bounds_bug.c):
  January clips `bounds` inside `if (!bounds)`; needs MAX(0, r->x0)/MIN(bitmap->width, r->x1) operand order. Owner decision.
- draw_string_obj_opus5_150k_w2 / next150_n1: OWNER-BLOCKED (UB ruling); not attempted.

## Baseline (production, this worktree)
- gate.py source/text/draw_string --all: exact 27 residual 1 (only _bitmap_draw_string [size 240!=304, sha]).
- January decode: after `test ebx,ebx; jne 0x94` (bounds != NULL skips), the arm reads [ebx],[2],[4],[6] (bounds==NULL const-propagated),
  clips them to bitmap via MAX(..,0)/MIN(bitmap,..) into set_rectangle2d(&local,-8), then ebx=&local. Clip arm identical shape on [ebp+0x10].
  Frame 0x10 (two rectangle2d). Nine relocs; no relocation on the [2]/[4]/[6] reads -> genuinely NULL-based, not a global.

## /Od readout (halo_cache_symbols.exe 0x8b2090, 546 B), RTC: adjusted_bounds [ebp-0x14] 8, adjusted_clip [ebp-0x24] 8
- byte local [ebp-5] = format-valid flag set by switch (TRUE for listed cases, FALSE default), then `if (valid)` wraps the body.
- `cmp [bounds],0; jne skip` -> body runs when bounds==NULL and dereferences bounds: SAME BUG in the later first-party build
  (independent build lineage corroborates the January original bug).
- set_rectangle2d(&adjusted_bounds, MAX(0,bounds[2]), MAX(0,bounds[0]), MIN(bitmap[4],bounds[6]), MIN(bitmap[6],bounds[4])); bounds=&adjusted_bounds;
- if (clip) same with adjusted_clip; clip=&adjusted_clip;  draw_string(bitmap_draw_character, bounds, NULL, clip, 0, string).
- HCEX.pdb: `DIA2Dump -sym bitmap_draw_string` returns nothing (no HCEX record) - no locals evidence from there.
- Prior exact body found: /c/halo-worktrees/opus5-100k-consolidated-20260914/scratch/workersD/draw_string/evidence_bitmap_draw_string_null_bounds_bug.body.txt
  (verifyD2 gate: EXACT 304). Uses switch-case body + `if (!bounds)` BUG + MAX(0,..)/MIN(bitmap->..,..).

## Probe 1 (cand1.c = production + b1.txt): BUG (original) body, /Od names adjusted_bounds/adjusted_clip, switch-case body
- Hypothesis: prior verifyD2 exact body still exact in this base; /Od RTC names are byte-inert.
- Result: gate == exact 28 residual 0 unwritten 0 (all rows EXACT, _bitmap_draw_string EXACT 304).

## Probe 2 (cand2.c = production + b2.txt): /Od-attested topology - boolean flag set by the format switch, then `if (valid_format)` body
- Hypothesis: /Od byte local [ebp-5] (1 in listed cases, 0 default) then `test; je` = flag + if; at -O2 jump threading gives January's switch->epilogue.
- Result: gate == exact 28 residual 0 (EXACT 304). Both cand1 and cand2 exact; cand2 follows /Od statement topology -> preferred.
- Layout proof: union rectangle2d {y0@0,x0@2,y1@4,x1@6}; set_rectangle2d(rect,x0,y0,x1,y1); January pushes
  y1=MIN(height[6],[4]) edi, x1=MIN(width[4],[6]) edx, y0=MAX(0,[ebx]=[0]) ecx, x0=MAX(0,[2]) eax -> exactly bounds->x0/y0/x1/y1 of a NULL bounds.

## Whole-object audit on cand2.obj
- strict coff_compare: build/base non-equal = [_bitmap_draw_string]; cand2.obj non-equal = [] (all January symbols).
- object_audit.py: PASS (46 January symbols, 0 differ). Candidate-only surplus = 7 literal/real COMDATs + .drectve,
  IDENTICAL to the production build/base surplus (pre-existing, not introduced).
- surplus_identity.py: 0 candidate-only code COMDATs.
- pdb_storage.py: 1 disagreement (PRE-EXISTING, same in production): _draw_string_globals split 2 ours 2, NOT a cachebeta public
  (no publics at all in 0x004C18xx-0x004C1Axx; no other split object references it) -> January storage was file-static.
  Assert literals name TWO different globals (draw_character_software_globals.bitmap, font_drawing_globals.tab_stop_count)
  -> the single 248-byte _draw_string_globals aggregate is invented (cf. hs_globals_external hold precedent).
- build/semantic_report.json: the 5 objdiff-3.3.1 under-credited functions (_bitmap_draw_character, _parse_string,
  _parse_unicode_string, _draw_string, _draw_unicode_string) are already hidden_exact/accepted_ledger - no semantic_matches entry needed.
- report.json data: .bss 248 100%, .rdata 808 100%. config.json status NonMatching.

## .bss ownership evidence (for the pdb_storage disagreement)
- January relocations into _draw_string_globals use offsets 0..0x68 only; +0x6C..+0xF8 (0x8C) is unreferenced in the object.
- /Od build globals (TU code 0x8b1900-0x8b5500): font_drawing fields 0xf457d8..0xf45822 (same relative layout as Jan +0x20..+0x6A),
  unreferenced 0x8C at 0xf45824..0xf458b0, text_bounds rect 0xf458b0 + dword 0xf458b8, pick (point 0xf458bc, shorts c0/c2/c4),
  draw_character_software bitmap 0xf458c8 + short 0xf458cc.
- /Od order is January's order reversed EXCEPT the 0x8C block stays directly after font_drawing in both builds -> it is the
  tail of font_drawing_globals (0x4C referenced + 0x8C = 0xD8). January .bss = draw_character_software_globals(8) @0,
  pick globals(12) @8, text-bounds globals(12) @0x14, font_drawing_globals(0xD8) @0x20 = 0xF8 total.

## HCEX.pdb globals (DIA2Dump -g / -sym / -type): FOUR file statics in draw_string
- text_pick_globals @0x16DB738 (File Static, struct <unnamed-tag>), text_bounds_globals @..744, draw_character_software_globals @..750,
  font_drawing_globals @..758 (UserDefinedType font_drawing_globals: string_list_index, current_font_index, current_flags,
  current_style, current_justification, current_color, tab_stop_count, tab_stops[0x10], highlight_start_index,
  highlight_stop_index, initial_indent, paragraph_indent, multitexture_params = struct rasterizer_dynamic_screen_geometry_parameters @+0x4C).
- repo rasterizer.h has that struct with size check 0x8C == January's unreferenced tail. Confirms the .bss is 4 statics, not one aggregate.

## Probe 3 (cand3a.c = gen3.py cand2.c: four UNINITIALISED file statics, production field names, byte tail)
- Hypothesis: splitting the aggregate into January's 4 statics leaves code bytes unchanged (loads are in-bounds in each object).
- simcmp.py (January relocs re-attributed to the 4 statics by offset): 28/28 functions strict-equal.
- .bss layout: cand pick 0, bounds 0x0C, dcs 0x18, font 0x20 (uninit = name-hash order; same relative order as HCEX)
  vs January dcs 0, pick 8, bounds 0x14, font 0x20 -> uninitialised form REJECTED by layout.

## Probe 4 (cand3b.c = gen3.py cand2.c --init: four statics `= { 0 }` in declaration order dcs, pick, bounds, font)
- Hypothesis (VC7 .bss law: zero-initialised statics go in declaration order; uninit = name hash, which probe 3 showed
  gives HCEX's pick,bounds,dcs,font order, not January's): January's neat order dcs,pick,bounds,font => initialisers.
- simcmp.py: 28/28 functions strict-equal after re-attribution; layout dcs 0x00, pick 0x08, bounds 0x14, font 0x20, all storage 3 (.bss) == January.

## Simulated csplit (build/tools/csplit.exe -i cachebeta.exe -p <scratch cfg copy> -o <scratch dir>; no tracked file touched)
- simcfg0 (unmodified config copy) -> simsplit0 draw_string.obj byte-IDENTICAL to build/split (tool sanity).
- simcfg1 = symbols.json line 23271 `_draw_string_globals` replaced IN PLACE by 4 static lines (symbols_json.diff):
  0x4C1908 _draw_character_software_globals, 0x4C1910 _text_pick_globals, 0x4C191C _text_bounds_globals, 0x4C1928 _font_drawing_globals.
  Only draw_string.obj changes across the whole split (diff -rq). New split defines the 4 as storage 3 at 0/8/0x14/0x20.
- cand3b.obj vs simsplit1: 49 symbols strict-equal, object_audit_sim PASS (49 January symbols, 0 differ), surplus = production's 7 literals,
  provider_link PASS (all 7), simulated pdb_storage: 0 disagreements (was 1).

## Probe 5 (cand3c.c = gen3 --init --hcex-names --typed on cand2): HCEX field names + typed multitexture_params (adds #include "rasterizer/rasterizer.h")
- simcmp 28/28 equal; object_audit_sim PASS (49/0); surplus unchanged; CL /Zs /W3: same 13 warnings as production; fake_match_scan 0 leads.
- relocs.json rows 82245..82255, 82475 target file_offset 4987144 addend 0 (bitmap_draw_character/bitmap_draw_string refs to offset 0):
  they resolve to whatever symbol sits at 0x4C1908 -> _draw_character_software_globals; already exercised by the simulated csplit.
## Probe 6 (candA.c = gen3 --init --hcex-names --typed on PRODUCTION, i.e. ownership fix WITHOUT the bug body)
- simcmp: 27/28 equal, only _bitmap_draw_string differs (the safe 240-byte body, as today). object_audit_sim FAIL(1) = that function only.
  => the .bss ownership packet is independently landable at zero code credit with no regression.

## objdiff 3.3.1 (build/tools/objdiff-cli.exe report generate on scratch odproj/, simulated split as target)
- prod vs old split reproduces build/report.json exactly (sanity). ownership-only vs new split: identical scores (.bss 248 100%, .rdata 100%).
- full vs new split: _bitmap_draw_string objdiff 87.83% while coff_compare strict-EXACT -> same hidden_exact class as the 5
  already-accepted draw_string functions (build/semantic_report.json accepts strict equality automatically).

## Final files (all verified; patches re-applied outside the repo reproduce them byte-for-byte, CRLF kept)
- final_ownership.c / ownership.patch: 4 HCEX file statics + HCEX field names + typed multitexture_params, safe body kept.
  Against simulated split: 27/28 strict (only _bitmap_draw_string, unchanged residual), pdb storage 0 disagreements.
- final_full.c / production.patch: ownership + BUG (original) bitmap_draw_string (/Od topology, house /* BUG (original) */ comment).
  Against simulated split: 49/49 January symbols strict-equal, object_audit PASS, pdb storage 0, provider_link PASS (7 pre-existing literals),
  /W3 same 13 warnings, fake_match_scan 0 leads.
- bug_body_after_ownership.patch (stack on ownership) ; bug_body_only_vs_production.patch (cand2.c: function only, current split:
  gate exact 28 residual 0, but leaves the pdb_storage disagreement).
- symbols_json.patch: in-place replacement of line 23271 by 4 static lines (no re-serialisation).

## Verdict
- _bitmap_draw_string: exact ONLY with January's proven NULL-bounds dereference (BUG (original)); brief: owner approval required -> BLOCKED (owner ruling).
  NEW evidence vs prior ledgers: the later /Od first-party build (0x8b2090) has the SAME inverted test (independent lineage), and
  RTC names adjusted_bounds/adjusted_clip; /Od-attested valid-format flag form is also exact.
- .bss ownership packet: independently landable now at zero credit (ADMISSION_FIX_PROPOSED), clears the object's only pdb_storage disagreement.
