# REVIEW source/text/draw_string - hygiene_r2w2 PACKET_draw_string (.bss ownership, zero credit)

Verdict: APPROVE, land the AMENDED packet `scratch/w/review_r2w2_draw_string/PACKET_draw_string_AMENDED.patch`
(worker packet + one member type: `short encoding_shift`). The worker's original is also regression-free.

## Independent checks (tree 434f0151 = 05255584 + ledger-only commit)
- apply: worker packet sha256 matches its checksum file; git apply --check rc 0; 01 == round-1 ownership.patch (modulo index).
- own csplit emulation (rv_emu.py): unmodified config reproduces build/split 833/833; patched config changes only draw_string.obj.
- gate: prod 27/1; candidate vs emulated split 27/1, identical EXACT rows (size, sha, relocs); lone residual _bitmap_draw_string
  (owner-held NULL-bounds BUG body, excluded). Either half alone: 5/23 -> land together + csplit-only regen.
- object_audit vs emulated split: FAIL(1) = that function only; 49 January symbols, 0 differ; .bss 248/248 ok.
- pdb_storage 1 -> 0. No cachebeta public in the region; no other object/config names any of the symbols.
- surplus identical to production; provider_link 7 literals PASS both orders; no new surplus.
- park _bitmap_draw_string measurements (target/base sha, relocs, objdiff 51.18261) unchanged -> park stays valid.
- objdiff 3.3.1 mini report identical; /W3 13 == 13; fake_match_scan 0 leads.

## Precedent (models a595bbc2, read in full)
Owner-accepted: explicit `= { 0 }` on correctly named/typed private statics to get January's .bss order; no aggregate facade,
padding, steering name, custom section or directive; ablation shows initialisers are the lever. Here: names from January asserts
(dcs, font) and HCEX (all four, File Static, three `<unnamed-tag>` structs + UDT font_drawing_globals); storage file-static by HCEX
DataKind 0x5 and cachebeta publics; the packet REMOVES an invented aggregate and pad members; my ablation reproduces the
precedent's shape (any uninitialised subset is hash-placed first). Not stretched. Production already spelled `= { 0 }`, and
>=25 canonical file-scope `_globals` statics use it.

## Amendment
`encoding_shift`: HCEX UDT says short; the later /Od x86 build reads it with `movsx ecx, word ptr` (short); January's single
read `mov cl, byte ptr [+4]` is width-neutral (lab: short is section-for-section identical). `byte` has no first-party support.

---

# SECOND ADVERSARIAL REVIEW (adv2, label review-r2w2-draw_string-adv2) - worker r2w2_draw_string final packet

Verdict: APPROVE `scratch/w/r2w2_draw_string/PACKET_draw_string.patch` (sha256 1b93d16a..; byte-identical to
`PACKET_draw_string_AMENDED.patch` above). Zero credit, unit stays NonMatching. No amendment needed.
Evidence and scripts: `adv2/` (LEDGER.md, emu2.py, fncmp.py, audit2.py, xref.py, w3.py, odproj/, abl_*.c/obj, od_bss_refs.txt, hcex_sym_*.txt).

## Independent re-measurement (tree 434f0151, clean)
- git apply --check rc 0 (combined, 01, 02). Round trip on CRLF copies of the working files: draw_string.c -> blob 984f239a,
  symbols.json -> blob 0f677d81, consistent CRLF; equal to the compiled candidate and the emulated config.
- Own csplit emulation (fresh config copy, 02 applied by exact one-line replacement): ctl 833/833 == build/split; new: only
  source/text/draw_string.obj differs; .bss 248 B c0400080 with dcs/pick/bounds/font storage 3 at 0/8/0x14/0x20.
- Strict gate (section_infos_equal): cand vs new split 27/1, prod vs old split 27/1, the EXACT rows identical on both sides
  (size/sha/relocs); cross pairs 5/23 (land both halves together). Every .bss relocation (name, addend) multiset in the new
  split equals the candidate's (dcs 0/4, pick 0..8, bounds 0..8, font 0..0x48: all on HCEX field offsets).
- object_audit vs new split: FAIL(1) = `_bitmap_draw_string` only (as production); 49 January symbols 0 differ; .bss + 17 .rdata ok.
- pdb storage: 1 -> 0. surplus: 0 code COMDATs, list identical to production; provider_link 7/7 PASS both orders, --baseline none new.
- objdiff 3.3.1 (sha1 3130e428) mini report: prod/old == cand/new (92.155975, 3253/7208, 22 fns, data 1056/1056), identical rows.
- Park `_bitmap_draw_string` (parked.json 304/9/4e8747.., 240/9/ad3d0d.., 51.18261) unchanged. /W3 13 == 13. fake_match_scan 0.
- No other object/source/config/tool references `_draw_string_globals` or the four names (split + base xref, repo grep).

## Evidence re-run
- HCEX DIA2Dump -sym: all four DataKind 0x5 File Static in the draw_string compiland; font_drawing_globals UDT Length 0xD8;
  UDT fields/types match the packet (short encoding_shift; short[0x10] tab_stops; multitexture_params @+0x4C).
- January .rdata literals: `draw_character_software_globals.bitmap->width/height`, `font_drawing_globals.tab_stop_count`.
- /Od build (data only, linear disasm of the TU): font 0xf457d8, bounds 0xf458b0, pick 0xf458bc, dcs 0xf458c8 = exact reverse
  of the packet's declaration order; the only encoding_shift access is `movsx ecx, word ptr [0xf458cc]` (short).

## Precedent audit (read the originals)
- a595bbc2 (models doc + commit): conditions = correct names/types, explicit zero initialisers as normal spelling, same .bss
  size/flags in January's order, no aggregate facade/padding/steering name/section/directive, ablation shows initialisers are
  the lever, zero regressions, parks valid, fake scan 0. All hold; name/type evidence is STRONGER here (all four HCEX, two
  January asserts, /Od declaration order) than in models (one HCEX name).
- models did not touch symbols.json; the symbols.json split is covered by a9e23674 (same construct class: invented aggregate
  + #define alias -> HCEA file statics, initialised in declaration order, "static": true rows) and 7979cf8f (in-place row
  edits to first-party names). 2026-09-15 owner hold: reason was invented names; reopen criterion (authenticated names) met.
- Not stretched: the only difference is that the object stays NonMatching (owner-held January-bug body), and the packet
  claims no credit or status change.

## Strip test / ablation (own)
- none initialised -> pick 0, bounds 0xc, dcs 0x18, font 0x20 (hash); dcs uninitialised -> January order (dcs `= { 0 }` is
  inert, as the worker disclosed; the prior "all four required" note above is corrected); font or pick uninitialised -> wrong.
- All ablations stay 27/1 on functions: the initialisers affect only symbol offsets, exactly the models situation. The inert
  dcs initialiser is not load-bearing, so it is not a strip-test fake; kept for uniform sibling spelling.

## Non-blocking notes
- Cosmetic: the packet deletes the (then empty) `/* ---------- macros */` section header; every sibling text/*.c keeps an empty
  one (231 of 328 macros sections board-wide are empty). Optional; comment-only, byte-inert; not amended.
- Integrator: apply both halves in one commit, csplit-only regen (only draw_string.obj changes), full ninja, pdb_storage 0,
  gate 27/1, parks, pytest (not run here: live config not patched).
