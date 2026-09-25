# MANIFEST: source/text/draw_string .bss ownership admission packet (zero credit)

- **Worker and claim:** r2w2_draw_string (claimed, then released at the end).
- **Base:** HEAD 434f0151, which is 05255584 plus a commit that only touches the ledger. No source, config or tools change between them.
- **Tip:** none. The integrator applies the packet.
- **Verdict:** ADMISSION_FIX_PROPOSED.
  - The ownership packet is covered by precedent, and every condition below holds.
  - The whole object still cannot be admitted. `_bitmap_draw_string` is the only function residual, and it needs the owner's ruling on a January bug (see Held items).

## Packet (apply together, then regenerate csplit)

| file | target | sha256 |
|---|---|---|
| `01_draw_string_ownership.patch` | source/text/draw_string.c (blob 10f77809 -> 984f239a) | 041960ed7f1c9fdc... |
| `02_symbols_json.patch` | config/symbols.json (blob f9abb17d -> 0f677d81); line 23279 edited in place | 22a4f97071ffc528... |
| `PACKET_draw_string.patch` | 01 + 02 in one file | 1b93d16a05bf04e4... |

- **Apply check:** `git apply --check` returns 0 at HEAD for the combined packet and for each half.
- **Round trip:** applying the packet in a copy outside the repo reproduces `final/draw_string.c` and the cut symbols.json exactly.
- **Agreement with the prior review:** my files are byte-identical to the amended packet from the r2 reviewer (`scratch/w/review_r2w2_draw_string/PACKET_draw_string_AMENDED.patch`). I rebuilt this packet independently.
- **Landing order:** 01 and 02 must land together.
  - The candidate against the current split scores 5/23.
  - Production against the new split also scores 5/23.
- **After landing:** regenerate csplit. Only `build/split/source/text/draw_string.obj` changes (emulated below).
- **Why 02 was re-cut:** the round-1 `symbols_json.patch` no longer applies (it fails at config/symbols.json:23268). The target row is now at 23279, and its trailing context row is now `_features` static. I cut 02 again in place from the HEAD blob.
- **Changed lines:** only that one line changes, and nothing is re-serialised.

## What changes (source)

1. **The invented aggregate becomes four file statics.**
   - Removed: the 248-byte `struct draw_string_globals draw_string_globals = { 0 };` aggregate. It was external, and the file had no header prototype for it.
   - Removed with it: the two TU-private alias macros `#define draw_character_software_globals draw_string_globals` and `#define font_drawing_globals draw_string_globals`, the pad members `__pad05`, `__pad12` and `__pad6A`, and the byte blob `multitexture_parameters[0x8C]`.
   - Added: four statics in declaration order.
     - `static struct { struct bitmap_data *bitmap; short encoding_shift; } draw_character_software_globals = { 0 };`
     - `static struct { point2d pick_point; short best_pick_string_index; short best_pick_distance; short last_string_index; } text_pick_globals = { 0 };`
     - `static struct { rectangle2d bounds; struct font_header *last_font_header; } text_bounds_globals = { 0 };`
     - `static struct font_drawing_globals font_drawing_globals = { 0 };`
2. **`struct font_drawing_globals` uses the HCEX field names:** string_list_index, current_font_index, current_flags, current_style, current_justification, current_color, tab_stop_count, tab_stops[MAXIMUM_NUMBER_OF_TAB_STOPS], highlight_start_index, highlight_stop_index, initial_indent, paragraph_indent and `struct rasterizer_dynamic_screen_geometry_parameters multitexture_params`.
   - Offset +0x4C is reached by natural alignment, with no pad member.
   - The typed tail is 0x8C bytes, and rasterizer.h has a size check for 0x8C.
3. **New include:** `#include "rasterizer/rasterizer.h"`, the genuine owner of that struct (rasterizer.h:170 has the size check).
4. **Use sites:** every use site is renamed to match. No logic changes.
5. **Header comment:** the "symbols in this file" comment lists the four statics with their sizes (8, 0xc, 0xc, 0xd8).
6. **Amendment over round 1:** `encoding_shift` is `short`, where production had `byte`. The evidence is under the next heading.

## Source evidence per changed construct

- **Names and file-static storage.** HCEX.pdb, from DIA2Dump `-sym` in `hcex_sym.txt`:
  - text_pick_globals @0x16DB738, text_bounds_globals @..744, draw_character_software_globals @..750 and font_drawing_globals @..758 all have DataKind 0x5, "File Static".
  - The first three are `struct <unnamed-tag>`. The last is `struct font_drawing_globals` with Length 0xD8, which matches January's 0x20..0xF8 extent.
- **Types and member names.** From HCEX `-type` and `-t` (`hcex_type_font.txt`, `hcex_t_draw_string_udts.txt`):
  - pick: `{point2d pick_point; short best_pick_string_index; short best_pick_distance; short last_string_index}`
  - dcs: `{bitmap_data *bitmap; short encoding_shift}`
  - bounds: `{rectangle2d bounds; font_header *last_font_header}`
  - font_drawing_globals: the members listed above.
- **January's own bytes.**
  - Assert literals in the target .rdata name `draw_character_software_globals.bitmap` (twice) and `font_drawing_globals.tab_stop_count`.
  - Relocation addends into the old aggregate split cleanly: dcs {0,4}, pick {8..0x10}, bounds {0x14..0x1c}, font {0x20..0x68}. No access crosses a static boundary.
- **Storage.** The cachebeta publics (where RVA equals the symbols.json file_offset) have no public between 0x4c0cbc and 0x4c4000, so all four were file-static in January.
- **`encoding_shift` width.**
  - HCEX says short.
  - The later first-party /Od build, at fn 0x8b19e0 (`od_8b19e0.txt`), reads `movsx ecx, word ptr [0xf458cc]`, which is a short load.
  - January's only access is `mov cl, byte ptr [dcs+4]`, a shift count, so January does not reveal the width.
  - Lab result: short versus byte gives 54/54 non-debug sections byte-identical. `byte` has no first-party support.
- **Declaration order dcs, pick, bounds, font.** January's .bss follows declaration order because the statics are initialised. The /Od build places the same four in exactly reverse order (font, bounds, pick, dcs).

## Target vs candidate

The target is the emulated split with 02 applied. The full table is in `fntable.md`, and the gate rows are in `fngate_*.txt` and `gate_*.txt`.

- **Candidate vs emulated split:** 27 EXACT, 1 residual. The residual is `_bitmap_draw_string`: target 304 bytes / 9 relocations / sha 4e8747..; candidate 240 / 9 / ad3d0d...
- **Production vs build/split:** 27/1, with the same residual.
- **The 27 EXACT rows** are identical in size, normalized sha and relocation count to production's rows.
- **Emulated csplit:** with HEAD config, 833/833 files are identical to build/split. With 02 applied, only `source/text/draw_string.obj` differs (`emu_split_diff.txt`). The new split defines the 4 names with storage 3, in .bss at offsets 0, 8, 0x14 and 0x20. The section is 248 bytes with flags c0400080.

## Data, symbols and surplus (section 4, against the emulated split)

- **object_audit** (`audit_cand_emu.txt`):
  - FAIL(1) for `_bitmap_draw_string` only. Production also fails on that one function (`audit_prod_oldsplit.txt`).
  - 49 January symbols, 0 differ (production: 46, 0 differ).
  - 45 sections ok, including .bss 248/248 with equal flags and alignment.
- **Data coff_compare:** 18 of 18 data sections are equal: .bss plus 17 .rdata (`data_coff_compare_cand_emu.txt`).
- **pdb_storage:** production has 1 disagreement (`_draw_string_globals`: split 2, ours 2, not public). The candidate has 0 (`pdb_*.txt`).
- **surplus_identity:** 0 candidate-only code COMDATs, in production and in the candidate.
- **Candidate-only surplus:** the list is identical to production's: .drectve, 4 pooled strings and 3 `__real`.
- **provider_link:** all 7 pass in both link orders (`provider_link_cand.txt`). With `--baseline=prod.obj`, no new surplus appears.
- **objdiff 3.3.1** (frozen, sha1 3130e428) mini report in `odproj/`:
  - Production against the old split and candidate against the emulated split give identical results: 92.155975, code 3253/7208, 22 of 28 functions, data 1056/1056, .bss and .rdata at 100%.
  - The park for `_bitmap_draw_string` stays valid: target and base measurements are unchanged, and objdiff is still 51.18261.
- **Configuration:** the unit has no entry in object_admission_rejections.json. No other tracked config, tool or source file references `_draw_string_globals` or the new names.

## Precedent section

### Governing ruling: a595bbc2 (models)

Quoted from `docs/object_matching_logs/models_obj_explicit_zero_bss_owner_20260924.md`:

> "Declaring each with an explicit zero initializer is a normal source spelling for an intentional fallback default; VC7 still emits a 136-byte `.bss` section, but now places the records in January's order"
> "No aggregate, padding, renamed steering symbol, custom section, or compiler directive is involved. An ablation leaving only the first two explicit zero initializers reverted to the old order, so the result is genuinely tied to source initialization rather than declaration order. The later first-party HCEX PDB confirms `default_function_values` as a private `float[4]`; January's own COFF establishes all four offsets and private storage."

**Supporting precedents:**
- **7979cf8f (R2 batch 1a).** First-party names replace invented static names, and symbols.json rows get `"static": true` where no cachebeta public exists. The rows are edited in place.
- **a9e23674.** This is the same construct class: "Replace invented detail-object globals with real file statics".
  - An invented aggregate and its `#define` alias were replaced by HCEA-attested file statics.
  - They were declared "in January's .bss order (initialised, so VC7 keeps declaration order)".
  - symbols.json got `"static": true` row insertions.

**Owner hold of 2026-09-15:** the owner held .bss splits (env_fog, models) because the names were invented. The reopen criterion was authenticated names, and that criterion is met here: every name is HCEX-attested, and two are also January-assert-attested.

### Conditions and how each holds

| # | precedent condition | holds? | evidence |
|---|---|---|---|
| 1 | Statics have correct, authentic names and types | yes | HCEX DataKind 5 File Static names; HCEX UDT member names and types; January assert literals |
| 2 | Private storage is established by first-party evidence and January's COFF | yes | HCEX "File Static"; no cachebeta public in the region; January reloc partition; emulated split storage 3 at 0/8/0x14/0x20 |
| 3 | Only explicit zero initializers: same .bss size, January's order | yes | .bss stays 248 bytes; order dcs 0, pick 8, bounds 0x14, font 0x20 equals January; flags c0400080 on both sides |
| 4 | No aggregate facade, padding, renamed steering symbol, custom section or compiler directive | yes | the packet REMOVES the invented aggregate, its alias macros and 3 pad members plus the byte blob. Each anonymous struct is one HCEX `<unnamed-tag>` static, not a facade over several statics. No pragma, section or directive; names are HCEX, not hash-steering choices |
| 5 | Ablation shows the result is tied to the initializers | yes | with no initializers, the order is pick, bounds, dcs, font (hash order), not January's. All 15 subsets are recorded in `ablation_bss_order.txt`. Disclosure: dcs's own `= { 0 }` is individually byte-inert ({pick,bounds,font} alone also gives January's order). I keep all four for uniform spelling, as models, production's `= { 0 }` and a9e23674 do. |
| 6 | Strict gate and no function regressions | yes | 27/27 of the previously exact functions stay EXACT with identical rows. Only draw_string.c changes, with no header edits, and only draw_string.obj changes in the split, so no other object can move |
| 7 | Parks remain valid | yes | `_bitmap_draw_string` park: target and base size, relocations, sha and objdiff are unchanged |
| 8 | Fake scan zero leads | yes | `fake_match_scan` finds 0 leads (candidate and production) |
| 9 | Zero credit claimed | yes | ownership closure only (see Credit) |

The precedent's object also had every function exact (models 12/12). That is NOT true here: 27 of 28 are exact, so this packet does not flip the status to Matching and claims no admission. The orchestrator's precedent audit classified the construct as a "Zero-credit ownership packet", and that is exactly what this is.

## House-rule review (changed constructs)

- **Names:** authentic names only; no `bss_` or `code_` names.
- **Headers and declarations:** no consumer-local prototype. The include goes to the genuine owner header.
- **Types:** `real` is untouched and `short` is HCEX-attested.
- **Fake matching:** no pad members or invented aggregate. No new macro, pragma, volatile or cast.
- **Declarations:** declaration and initialisation are combined.
- **Warnings:** `CL /Zs /W3` gives 13 diagnostics in both, and the lists are identical (`w3_*.txt`).
- **Strip test:** stripping the initializers breaks the .bss order. That is the precedent's accepted mechanism, not decoration.

## Tests run

- gate.py for production and candidate against the current split
- fngate (coff_compare) against the emulated split, for production, the round-1 variant and the candidate
- control and patched csplit emulations
- object_audit
- pdb_storage
- surplus_identity
- provider_link, both orders and `--baseline`
- data coff_compare
- objdiff 3.3.1 mini report
- /W3 comparison
- fake_match_scan
- 15-way initializer ablation
- byte-vs-short lab comparison
- round-trip apply

pytest was not run. The tests use fixtures rather than the live config, so the integrator runs it after landing.

## Credit

Zero:
- 0 meaningful code bytes
- 0 padded code bytes
- no data credit, since the data was already at 100%
- no admission; the status stays NonMatching

What the packet does:
- clears the object's only pdb_storage disagreement
- replaces an invented aggregate with January's four HCEX-attested file statics

## Held items and reopen criteria

- **`_bitmap_draw_string`** (304 bytes) is owner-held in the January-bug class (brief section 8): January derefs `bounds` inside `if (!bounds)`, a NULL-pointer read.
  - The round-1 `production.patch` / `final_full.c` stacks the /Od-attested BUG (original) body on this ownership packet.
  - Re-verified at 434f0151, lab only and NOT landed: 01 plus round-1 `bug_body_after_ownership.patch` applies cleanly. Against the emulated split it gives fngate 28/0 and object_audit PASS (`lab_full_bug_body.c`, `lab_full.obj`).
  - Reopen when the owner approves that January-bug body. It must carry a `/* BUG (original) */` comment.
