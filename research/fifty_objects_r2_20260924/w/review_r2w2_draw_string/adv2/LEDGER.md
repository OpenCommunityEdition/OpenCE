# Adversarial review 2 (adv2) ledger - Thu Sep 24 10:31:34 PDT 2026
Label review-r2w2-draw_string-adv2 (claimed). Tree HEAD 434f0151 clean (only untracked research/fifty_objects_r2_20260924/).
- Packet sha256 1b93d16a.. == prior reviewer's PACKET_draw_string_AMENDED.patch (cmp identical). git apply --check rc 0: combined, 01, 02.
- 01 applied (non-repo, GIT_CEILING) to HEAD:draw_string.c -> blob 984f239a == worker final/draw_string.c byte-identical.
- 02 applied by exact single-line replacement in own copy of config (emu2.py) -> LF blob 0f677d81 (== patch index).
- Own csplit emulation: ctl 833/833 identical to build/split; new: only source/text/draw_string.obj differs.
  new split .bss: 248 B flags c0400080; dcs st3@0, pick st3@8, bounds st3@0x14, font st3@0x20 (old: _draw_string_globals st2@0).
- fncmp (coff_compare.section_infos_equal) cand vs new split 27/1; prod vs old split 27/1; EXACT rows identical (target+ours size/sha/relocs).
  cand vs old 5/23, prod vs new 5/23 -> halves must land together. Residual _bitmap_draw_string 304/240 r9/9 4e8747/ad3d0d (== park).
- object_audit (copy audit2.py, ADV2_SPLIT) cand vs new: FAIL(1) .text _bitmap_draw_string only; 49 Jan symbols 0 differ; 1 .bss + 17 .rdata ok.
  prod vs old: FAIL(1) same fn; 46/0. Surplus list identical (.drectve, 4 literals, 3 __real).
- pdb storage (own re-impl on new split/cand): prod 1 disagreement (_draw_string_globals st2 not public) -> cand 0. None of 5 names public;
  publics seg 0014 jump 0x4c0cbc -> 0x4ccb10 (region sparse; HCEX File Static is the primary storage proof).
- xref build/split + build/base: _draw_string_globals defined only by draw_string; the 4 new names referenced nowhere else.
- provider_link cand: 7/7 PASS both orders; --baseline=prod: no new surplus. surplus code COMDATs: 0 (prod 0).
- objdiff 3.3.1 (sha1 3130e428) own mini report: prod/old == cand/new: 92.155975, 3253/7208, 22 fns, data 1056/1056; function+section lists identical.
- park _bitmap_draw_string: parked.json target 304/9/4e8747.., base 240/9/ad3d0d.., 51.18261 == measured on new split/cand.
- HCEX DIA2Dump -sym (re-run): all four DataKind 0x5 File Static; font_drawing_globals UDT Length 0xD8; RVAs pick 738, bounds 744, dcs 750, font 758.
- /Od build (halo_cache_symbols.exe, linear disasm 0x8b1900-0x8b5600, DATA ONLY): font 0xf457d8 (fields +0..+0x48), bounds 0xf458b0/+8,
  pick 0xf458bc/+4/+6/+8, dcs 0xf458c8 (+4 = movsx word, the only encoding_shift ref). => font,bounds,pick,dcs = exact reverse of dcs,pick,bounds,font.
- Ablation (own): none -> pick 0, bounds 0xc, dcs 0x18, font 0x20 (hash); no-dcs-init -> January order (dcs init inert, as worker disclosed);
  no-font-init -> font 0 first; no-pick-init -> pick 0 first. ALL variants 27/1 fn-exact: order is visible only in symbol offsets (object_audit),
  functions compare by symbol+addend.
