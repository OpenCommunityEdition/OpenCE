# Adversarial review: source/text/draw_string PACKET (hygiene_r2w2) - ledger

Reviewer label: review-r2w2-draw_string-adv (claimed). Tree HEAD 434f0151 (05255584 + ledger-only commit), clean.
- patch sha256 matches patches_sha256.txt; git apply --check PACKET rc 0 at HEAD.
- 01 == round-1 ownership.patch modulo index line (diff empty).
- HCEX -compiland draw_string.obj: text_pick_globals / draw_character_software_globals / text_bounds_globals = File Static
  `struct <unnamed-tag>`; font_drawing_globals = File Static `struct font_drawing_globals`. -type font_drawing_globals fields
  match the packet (string_list_index..paragraph_indent, multitexture_params @+0x4C). HCEX RVAs: pick 738, bounds 744, dcs 750, font 758.
- HCEX -t UDTs (hcex_t_draw_string_udts.txt): pick {point2d pick_point, short best_pick_string_index, short best_pick_distance,
  short last_string_index}; dcs {bitmap_data *bitmap, SHORT encoding_shift}; bounds {rectangle2d bounds, font_header *last_font_header}.
  Packet matches all names; it keeps production's `byte encoding_shift` (HCEX says short).
- Emulated csplit (rv_emu.py, own slug): ctl (unmodified config) == build/split 833/833; PACKET config -> only draw_string.obj differs.
- gate prod 27/1; cand vs emu split 27/1 (only _bitmap_draw_string 240!=304); EXACT rows (size/sha/relocs) identical to prod.
  cand vs OLD split 5/23 and prod vs emu 5/23 -> halves must land together.
- object_audit (copy w/ RV_SPLIT): cand vs emu FAIL(1) = _bitmap_draw_string only; 49 January symbols 0 differ; .bss 248/248 ok;
  4 statics storage 3 at 0/8/0x14/0x20 in both. prod vs build/split FAIL(1), 46 symbols.
- pdb_storage: prod 1 (_draw_string_globals ext, not public) -> cand/emu 0. No cachebeta public between 0x4c0cbc and 0x4c4000.
- No other object (base or split) defines or references _draw_string_globals or the 4 new names; no config entry but symbols.json.
- provider_link cand: 7 literals PASS both orders; --baseline: no new surplus. Surplus list == production.
- Park _bitmap_draw_string: target 304/9/4e8747.., base 240/9/ad3d0d.. unchanged; objdiff 3.3.1 fn 51.18261 unchanged -> park stays valid.
- objdiff 3.3.1 mini report prod == cand (92.155975, code 3253/7208, 22 fns, data 1056/1056, .bss/.rdata 100%).
- /W3: 13 == 13. fake_match_scan: 0 leads both.
- Strip/ablation of `= { 0 }`: none -> pick 0,bounds 0xc,dcs 0x18,font 0x20; dcs only -> pick,bounds,font,dcs; dcs+pick ->
  bounds,font,dcs,pick; three -> font,dcs,pick,bounds. ALL FOUR initialisers required (uninit hash-placed first) - same ablation
  shape as the models precedent a595bbc2 (owner-accepted; conditions: HCEX/Jan names+types, no aggregate/pad/steering name/
  section/directive, ablation proves the initialiser is the lever, zero regressions, parks valid, fake scan 0) -> all hold here.
- January relocation addends in the old aggregate partition exactly: dcs 0/4, pick 8..0x10, bounds 0x14..0x1c, font 0x20..0x68.
- encoding_shift width: January `mov cl, byte ptr [dcs+4]` (only read, never written in Jan). /Od x86 build 0x8b19e0
  (bitmap_draw_character, asserts name draw_character_software_globals) reads `movsx ecx, word ptr [0xf458cc]` = SHORT.
  HCEX UDT = short. LAB lab_short_shift.c: `short encoding_shift` -> 27/1 identical; all non-debug sections and symbol table of
  candA.obj == cand.obj. => byte is unattested; two first-party builds say short; January neutral.
- AMENDED packet PACKET_draw_string_AMENDED.patch (= worker packet with `short encoding_shift`, index line fixed to 984f239a):
  git apply --check rc 0 (combined, 01A alone, 02 alone); round trip == expected; emu split identical to original packet's;
  gate 27/1 same rows; audit FAIL(1) same fn, 49/0; pdb 0; link PASS / no new surplus; W3 13 same; fake 0; objdiff identical.
