# r2w2_draw_string ledger (admission rebuild of the .bss ownership packet)

Claim: r2w2_draw_string (claimed 2026-09-24). Tree HEAD 434f0151 (= 05255584 + ledger-only commit), clean except untracked research/.
Inputs read: round-1 research/fifty_objects_20260925/w/draw_string/{LEDGER.md,ownership.patch,symbols_json.patch};
prior r2 packet scratch/w/hygiene_r2w2/draw_string (MANIFEST) + reviewer scratch/w/review_r2w2_draw_string (REVIEW approve, amended short encoding_shift).
Precedent: a595bbc2 (models `= { 0 }` .bss order), 7979cf8f (first-party names + symbols.json static rows).

## Rebuild at HEAD 434f0151
- round-1 ownership.patch: git apply --check rc 0 (source unchanged since round 1). round-1 symbols_json.patch: FAILS at
  config/symbols.json:23268 (context moved; row is now 23279 and its neighbour 4987912 is `_features` static).
- 02_symbols_json.patch re-cut IN PLACE from the HEAD blob f9abb17d: line 23279 `_draw_string_globals` -> four `"static": true`
  rows (4987144 dcs, 4987152 pick, 4987164 bounds, 4987176 font). No re-serialisation; only that line changes. apply --check rc 0.
- 01_draw_string_ownership.patch = round-1 ownership.patch + `short encoding_shift` (see below), cut from HEAD blob 10f77809.
- PACKET_draw_string.patch = 01 + 02. apply --check rc 0 at HEAD; round trip in a non-repo copy == final/draw_string.c and the
  cut symbols.json. The resulting files are byte-identical to the reviewer's amended packet
  (scratch/w/review_r2w2_draw_string/PACKET_draw_string_AMENDED.patch) -> independent convergence.

## Evidence re-verified myself
- HCEX DIA2Dump -sym (hcex_sym.txt): text_pick_globals 0x16DB738, text_bounds_globals ..744, draw_character_software_globals ..750,
  font_drawing_globals ..758; all DataKind 0x5 "File Static"; three `struct <unnamed-tag>`, one `struct font_drawing_globals` (Length 0xD8).
- HCEX -type/-t (hcex_type_font.txt, hcex_t_draw_string_udts.txt): every member name/type used by the packet; dcs.encoding_shift is SHORT.
- /Od x86 build fn 0x8b19e0 (od_8b19e0.txt): `movsx ecx, word ptr [0xf458cc]` = short encoding_shift; asserts name draw_character_software_globals.
- January bytes: the only dcs+4 access is `mov cl, byte ptr [4]` (shift count, width-neutral). Addend census of all January
  relocations into _draw_string_globals: dcs {0,4}, pick {8..0x10}, bounds {0x14..0x1c}, font {0x20..0x68}: partition exact, no
  access crosses a static boundary. January assert literals name draw_character_software_globals.bitmap (x2) and
  font_drawing_globals.tab_stop_count.
- cachebeta publics (RVA == symbols.json file_offset): no public between 0x4c0cbc and 0x4c4000 -> all four were file-static.

## Emulated csplit (ds.py; build/tools/csplit.exe -i cachebeta.exe -p <copy> -o <slug dir>)
- ctl (HEAD config copy): 833/833 split files byte-identical to build/split (tool sanity).
- emu (HEAD config + 02): only source/text/draw_string.obj differs (emu_split_diff.txt). New split defines the 4 names
  storage 3 in .bss at 0/8/0x14/0x20, section 248 bytes, flags c0400080 (same as before).

## Gates
- gate.py production vs build/split: 27/1 (only _bitmap_draw_string 240!=304, sha).
- gate.py candidate vs build/split (old): 5/23 (expected: relocation targets renamed) -> halves must land together.
- fngate (coff_compare.section_infos_equal) cand vs emu split: 27/1, same lone residual; all 27 EXACT rows identical
  (size, normalized sha, relocs) to production vs build/split. prod vs emu: 5/23.
- byte vs short encoding_shift: cand.obj vs r1own.obj: 54/54 non-debug sections byte-identical -> short is byte-inert.

## Section-4 audit (vs emulated split)
- object_audit: FAIL(1) = _bitmap_draw_string only (same as production); 49 January symbols 0 differ (prod: 46/0);
  45 sections ok incl. .bss 248/248 and all 17 .rdata; surplus list identical to production (.drectve + 4 pooled strings + 3 __real).
- pdb_storage: production 1 disagreement (_draw_string_globals split 2 ours 2 not public) -> candidate 0.
- surplus_identity: 0 candidate-only code COMDATs (prod 0).
- provider_link: 7 surplus literals PASS both orders (same providers as production); --baseline=prod: no new surplus.
- data coff_compare: 18/18 data sections equal (data_coff_compare_cand_emu.txt).
- objdiff 3.3.1 mini report (odproj): prod(old split) == cand(emu split): 92.155975, code 3253/7208, 22/28 fns, data 1056/1056;
  _bitmap_draw_string 51.18261 -> park measurements (target 304/9/4e87.., base 240/9/ad3d.., objdiff 51.18261) unchanged.
- /W3: 13 == 13 diagnostics (identical list). fake_match_scan: 0 leads (cand and prod).
- No other tracked config/tool/source references _draw_string_globals or the new names; no admission-rejection entry for the unit.

## Ablation of `= { 0 }` (abl/, ablation_bss_order.txt; all 15 subsets compiled)
- none initialised -> pick 0, bounds 0xc, dcs 0x18, font 0x20 (VC7 name-hash order; also HCEX's address order) != January.
- January order dcs 0, pick 8, bounds 0x14, font 0x20 is produced by all four initialised AND by {pick,bounds,font}
  initialised with dcs uninitialised (the lone uninitialised static is placed first). Every other subset fails.
- => the initialisers ARE the lever (precedent condition), but dcs's own `= { 0 }` is individually byte-inert. Correction to the
  prior reviewer note "ALL FOUR initialisers required": three are required. Kept uniform on all four (models precedent spelling,
  production's existing `= { 0 }`, and a9e23674 initialises every recovered static); disclosed in MANIFEST.
- Declaration order dcs, pick, bounds, font: January (forward, initialised) and the /Od build (reverse: font, bounds, pick, dcs).

## Verdict
ADMISSION_FIX_PROPOSED (zero credit): the ownership packet is covered by a595bbc2 (+7979cf8f, a9e23674) and every precedent
condition holds. Whole-object admission stays BLOCKED by the owner-held _bitmap_draw_string NULL-bounds BUG body (brief sec.8).
- Lab (not landed): 01 + round-1 bug_body_after_ownership.patch applies at HEAD; vs emu split fngate 28/0, object_audit PASS
  (lab_full_bug_body.c / lab_full.obj). Confirms the ownership packet leaves the object one owner ruling from complete.
