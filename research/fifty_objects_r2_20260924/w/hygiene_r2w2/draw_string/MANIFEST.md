# MANIFEST - source/text/draw_string: .bss ownership packet (zero credit)

Base: 05255584 (worktree `claude-fifty-objects-r2-20260924`, clean). Tip: none. The integrator applies this packet.
Claim: `r2w2-hygiene-draw_string`. Verdict: **ADMISSION_FIX_PROPOSED** (zero credit, zero regressions).

## Packet (apply in order, or apply the combined file)
- `PACKET_draw_string.patch` = 01 + 02 in a single file. `git apply --check` rc 0 at 05255584. Round trip to the tested files: PASS.
- `01_draw_string_ownership.patch`: source/text/draw_string.c. Identical to round-1 `ownership.patch` apart from the index line.
- `02_symbols_json.patch`: config/symbols.json, row 23279 edited in place. `_draw_string_globals` becomes four `"static": true` rows:
  0x4C1908 `_draw_character_software_globals`, 0x4C1910 `_text_pick_globals`, 0x4C191C `_text_bounds_globals`, 0x4C1928 `_font_drawing_globals`.
  - Round-1 `symbols_json.patch` no longer applies: its context row 4987912 is now `_features` static. This file regenerates it.
  - Run a csplit-only regeneration after landing. 01 and 02 must land together: either half alone gives 5 exact out of 28 against the other split.

## Source evidence for each changed construct
- **Four file statics instead of one 248-byte aggregate.**
  - HCEX.pdb (`DIA2Dump -g` and `-sym`, re-run today): all four names exist with DataKind 0x5 (DataIsFileStatic).
  - HCEX address deltas give sizes 8, 12, 12 and 0xD8+, which match January's layout 0/8/0x14/0x20.
  - cachebeta publics list none of these names; the pdb_storage truth is file-static.
- **Field names.**
  - January's own assert literals name `draw_character_software_globals.bitmap` and `font_drawing_globals.tab_stop_count`.
  - The font_drawing_globals member names and `multitexture_params` (`struct rasterizer_dynamic_screen_geometry_parameters`, 0x8C, which is January's unreferenced tail) come from the HCEX UDT, per the round-1 ledger.
  - The anonymous pick and bounds members keep their production names.
- **`= { 0 }` on each static.** The VC7 .bss law says an initialised static goes in declaration order, while an uninitialised one goes in name-hash order. Round-1 probe 3 showed that the uninitialised form reorders the objects, so the initialisers are required for January's order: dcs, pick, bounds, font.
- **Removed constructs.** The `#define draw_character_software_globals draw_string_globals` / `font_drawing_globals` alias macros are gone, and so are the invented pad members `__pad05`, `__pad12`, `__pad6A` and `multitexture_parameters[0x8C]`.
- **Added include.** `#include "rasterizer/rasterizer.h"` is the genuine owner of the multitexture struct.

## Target vs candidate
The target is the emulated split with 02 applied (`ds_emu/split`). Full table: `fntable.md`.
- The gate is 27/1 in production (build/split) and 27/1 for the candidate against the emulated split. The same row stays residual: `_bitmap_draw_string` 240 vs 304.
- That row is the owner-held NULL-bounds BUG body. It is excluded here; see the round-1 `production.patch`.
- All 27 exact rows have identical size, normalized sha and relocation count.

## Data, symbols and surplus
- **object_audit.**
  - Production: FAIL(1), which is `_bitmap_draw_string`, with 46 symbols and 0 differing.
  - Candidate against the emulated split: FAIL(1), the same function only. It has 49 January symbols and 0 differ, and .bss is 248/248 ok.
- **Emulated split.** 833 objects compared; only `draw_string.obj` differs (`emu_split_diff.txt`).
- **pdb_storage.** Production has 1 disagreement (`_draw_string_globals`); the candidate against the emulated split has 0.
- **surplus.** It is the same list as production: .drectve, 4 pooled strings and 3 `__real`. surplus_identity finds 0 code COMDATs. provider_link shows the 7 literals PASS in both orders, and `--baseline` finds no new surplus.
- **objdiff 3.3.1 mini report (`../odproj`).** Production and candidate are identical: code 3253/7208, fn 22/28, data 1056/1056, fuzzy 92.1560.

## Tests run
- Full shadow sweep (612 TUs) against the control: 611 identical, with only draw_string differing.
- `CL /Zs /W3`: 13 warnings in both (SAME).
- fake_match_scan: 0 leads in both.
- The candidate object from the sweep and the gate object are canonically identical.

## Credit
Zero: 0 meaningful and 0 padded bytes, no data credit, no admission. The object is still blocked by `_bitmap_draw_string`, the owner's January-bug ruling.
Reopen when the owner approves the NULL-bounds BUG body. `final_full.c` then passes the whole audit on this split (round 1).
