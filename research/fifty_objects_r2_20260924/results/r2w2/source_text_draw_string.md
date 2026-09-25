# source/text/draw_string (admit source/text/draw_string)

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
[]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/r2w2_draw_string/PACKET_draw_string.patch",
 "C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/r2w2_draw_string/01_draw_string_ownership.patch",
 "C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/r2w2_draw_string/02_symbols_json.patch",
 "C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/r2w2_draw_string/final/draw_string.c",
 "C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/r2w2_draw_string/MANIFEST.md",
 "C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/r2w2_draw_string/LEDGER.md",
 "C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/r2w2_draw_string/lab_full_bug_body.c"
]

## production_changes
Zero-credit ownership packet. The integrator applies C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/r2w2_draw_string/PACKET_draw_string.patch, which is 01_draw_string_ownership.patch + 02_symbols_json.patch in the same folder, then regenerates csplit. Both halves must land together: either half alone gives 5 exact / 23 residual against the other split.

(01) source/text/draw_string.c (blob 10f77809 -> 984f239a):
- Replaces the invented external 248-byte `draw_string_globals` aggregate.
- Removes its two TU-private #define aliases, the pad members __pad05/__pad12/__pad6A and the byte[0x8C] blob.
- Adds four HCEX-attested file statics in declaration order dcs, pick, bounds, font, each `= { 0 }`:
  - `static struct { struct bitmap_data *bitmap; short encoding_shift; } draw_character_software_globals`
  - `text_pick_globals`
  - `text_bounds_globals`
  - `static struct font_drawing_globals font_drawing_globals`
- `font_drawing_globals` uses the HCEX field names and a typed `struct rasterizer_dynamic_screen_geometry_parameters multitexture_params`, with `#include "rasterizer/rasterizer.h"` (its genuine owner).
- Every use site is renamed; the header symbol comment is updated.
- Amendment over round 1: `short encoding_shift` (HCEX + /Od movsx word; byte-inert, 54/54 sections identical).

(02) config/symbols.json (blob f9abb17d -> 0f677d81):
- Line 23279 `_draw_string_globals` is edited IN PLACE into four `"static": true` rows at 4987144/4987152/4987164/4987176: _draw_character_software_globals, _text_pick_globals, _text_bounds_globals, _font_drawing_globals.
- The round-1 symbols_json.patch fails at :23268 because its context moved, so this row was re-cut from the HEAD blob.

`git apply --check` returns 0 for the combined packet and for each half. The round trip reproduces the tested files exactly, and they are byte-identical to the reviewer's amended packet (scratch/w/review_r2w2_draw_string/PACKET_draw_string_AMENDED.patch).

No status change: the unit stays NonMatching. No parked.json, semantic or rejection edits are needed.

## evidence
Emulated csplit (build/tools/csplit.exe on a scratch config copy):
- Unmodified config: 833/833 split files are byte-identical to build/split.
- With 02: only source/text/draw_string.obj differs. It defines the 4 names with storage 3 in .bss at 0/8/0x14/0x20; the section is still 248 bytes with flags c0400080.

Strict gate (coff_compare.section_infos_equal):
- Candidate vs emulated split: 27 EXACT / 1 residual.
- Production vs build/split: 27/1.
- The 27 EXACT rows are identical in size, normalized sha and relocation count. The lone residual in both is _bitmap_draw_string (304 vs 240 bytes).

object_audit vs emulated split:
- FAIL(1), that function only, the same as production.
- 49 January symbols, 0 differ (production 46/0).
- .bss 248/248 ok; 18/18 data sections are coff_compare-equal.

pdb_storage: 1 disagreement -> 0.

Surplus:
- surplus_identity: 0 code COMDATs.
- Candidate-only surplus is identical to production's (.drectve, 4 pooled strings, 3 __real).
- provider_link: 7/7 PASS in both orders; --baseline finds no new surplus.

objdiff 3.3.1 mini report: production (old split) == candidate (new split), 92.155975, data 1056/1056. The _bitmap_draw_string park measurements (304/9/4e8747.., 240/9/ad3d0d.., 51.18261) are unchanged. /W3: 13 == 13 diagnostics. fake_match_scan: 0 leads.

First-party evidence, re-run by me:
- HCEX DIA2Dump: all four names are DataKind 5 File Static (three `<unnamed-tag>`, plus font_drawing_globals of length 0xD8). The UDT member names and types match the packet, and encoding_shift is short.
- January assert literals name draw_character_software_globals.bitmap and font_drawing_globals.tab_stop_count.
- January relocation addends split cleanly: dcs {0,4}, pick {8..0x10}, bounds {0x14..0x1c}, font {0x20..0x68}.
- cachebeta has no public between 0x4c0cbc and 0x4c4000.
- The /Od build places the four in exactly reverse declaration order.

Precedent conditions (a595bbc2 quoted in MANIFEST): all hold.
- Authentic names and types.
- First-party private storage.
- Only explicit zero initialisers; same .bss size, January's order.
- The packet REMOVES the aggregate facade and padding; no steering names, sections or directives.
- A 15-subset ablation proves the initialisers are the lever: with none, the order is the hash order pick, bounds, dcs, font.
- No function regressions; the parks stay valid; the fake scan is clean.
- Also supported by a9e23674 (the same construct class: an invented aggregate replaced by initialised HCEA file statics plus static symbols.json rows) and by 7979cf8f.
- The 2026-09-15 owner .bss-split hold is satisfied because every name is HCEX-authenticated.

Disclosure: dcs's own `= { 0 }` is individually byte-inert ({pick,bounds,font} initialised also gives January's order). This corrects the reviewer's "all four required". I kept all four for uniform spelling, matching models, the production spelling and a9e23674.

Lab only, not landed: 01 + the round-1 bug_body_after_ownership.patch gives 28/0 and object_audit PASS against the emulated split.

Files: C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/r2w2_draw_string/{MANIFEST.md, LEDGER.md, fntable.md, ablation_bss_order.txt, audit_cand_emu.txt, emu_split_diff.txt}.

## blockers
This is not a precedent-condition failure; the ownership packet itself is fully covered. The whole-object admission and every-function-EXACT goal are blocked by _bitmap_draw_string (304 bytes). January tests `if (!bounds)` and then dereferences bounds, a NULL-pointer read. That is the held January-bug class (brief section 8) and needs an owner ruling. The safe body stays at 240 bytes, fuzzy and parked. The packet therefore earns zero credit and no status flip.

## reopen_criteria
Owner approves the January NULL-bounds BUG (original) body for _bitmap_draw_string. Stack round-1 research/fifty_objects_20260925/w/draw_string/bug_body_after_ownership.patch on this packet; it was re-verified at 434f0151 at 28/28 with object_audit PASS against the emulated split. The object is then OBJECT_COMPLETE (status Matching).

## manifest
C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/r2w2_draw_string/MANIFEST.md

ledger: C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/r2w2_draw_string/LEDGER.md
