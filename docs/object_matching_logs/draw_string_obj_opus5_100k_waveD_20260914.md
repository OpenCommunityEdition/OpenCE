# `draw_string.obj` Opus5 100K wave D: park re-investigation (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/text/draw_string.c`, target `build/split/source/text/draw_string.obj`.
- Compiler: XDK VC7 13.00.9254 with the repository flags from `build.ninja` (via `tools/campaign/gate.py`).
- Lane `opus/100k-consolidated-20260914`, worktree `C:\halo-worktrees\opus5-100k-consolidated-20260914`.
- Evidence: January target COFF bytes and `.rdata` literals (primary); HCEA `src/blam/text/*.c` and
  `src/blam/bitmaps/bitmap_draw_character.c` for names and semantics only. No Ghidra or atlas output exists
  for this object. punpckhdq `source/text/draw_string.c` is only a symbol-list stub.
- Prior ledgers read in full: `draw_string_obj_jonas_hcea_one_shot_20260828.md`,
  `draw_string_obj_full_reconstruction_20260902.md`, `draw_string_obj_tab_stops_opus_reconciliation_20260903.md`.
- Worker notes: `scratch/workersD/draw_string.md`; scratch tooling in `scratch/workersD/draw_string/`
  (`swap.py` swaps one function definition, `run.sh` runs the gate, `parkcheck.py` checks park measurements,
  `dsdis.py` is a jump-table-aware disassembler/aligner (alndiff cannot walk inline jump tables), `census.py`).

## 2. Validated baseline

`gate.py source/text/draw_string --all --forbid-emitted-symbol _point_from_line3d`:
== exact 19 residual 9 unwritten 0. All nine residuals are parked and locked in `config/parked.json`:

| Function | January padded / relocs | Base padded / relocs | Base normalized sha16 |
| --- | ---: | ---: | --- |
| `_bitmap_draw_character` | 944 / 25 | 912 / 25 | a779c56eb5f9c0f5 |
| `_bitmap_draw_string` | 304 / 9 | 240 / 9 | ad3d0d63e2c82c1e |
| `_draw_string` | 912 / 46 | 976 / 45 | defd236dd149bb2f |
| `_draw_string_partial` | 528 / 9 | 544 / 9 | 55ba88e7380296d8 |
| `_draw_unicode_string` | 912 / 46 | 960 / 45 | a127308710f207cf |
| `_draw_unicode_string_partial` | 528 / 9 | 544 / 9 | f1c15fb2d0ccfe5b |
| `_parse_string` | 624 / 30 | 624 / 29 | ff1ae435b34e440a |
| `_styled_font_get` | 112 / 6 | 112 / 6 | bc72edd715836e32 |
| `_text_pick_draw_character` | 224 / 11 | 224 / 11 | 12157843c4789aaa |

## 3. Accepted controls (real file)

None. The real file is unchanged: every non-exact function is a locked park, so all work stayed in scratch and
is handed to the orchestrator as park-reopen proposals. Final real-file gate is identical to the baseline.

## 4. Experiment matrix and results (scratch only)

Strict EXACT in scratch, with natural house-rule source:

| Function | Candidate | Decisive evidence / law |
| --- | --- | --- |
| `_styled_font_get` | `reopen_styled_font_get.c` | style assert at function top, then `styled_font_index = style == _text_style_plain ? font_index : font_definition_get(font_index)->style_fonts[style].index;` The plain arm of the conditional expression is the block VC7 places after the ret. The direct accessor chain gives `[eax+ecx+0x48]` base/index order. |
| `_text_pick_draw_character` | `reopen_text_pick_draw_character.c` | Signed short distance locals, `distance_n = ABS(distance_n)` four times, then a sequential `MAX` chain. Base's `(short)` casts inside `ABS` blocked VC7 reassociating `y1 - pick.y` into `(y0 - pick.y) + dy`. |
| `_draw_unicode_string_partial` | `reopen_draw_unicode_string_partial.c` (base + this body) | `short` clip limits declared left, top, right, bottom and narrowed with plain `if` statements. One guard around the body. Glyph locals declared in the order x, y, width, source_x, source_y, height. This follows the declaration-order law: only this order is exact, and three other orders were measured. Base's extra `long` region locals caused the 0x3c vs 0x38 frame. |
| `_draw_string_partial` | `reopen_draw_string_partial_with_parse_string.c` | Same body as the unicode function. It also needs `_parse_string` to take its argument on the stack as in January. The base `_parse_string` gets a VC7 register ABI (ebx), so every caller changes. |
| `_draw_string` | `combined_v2.c` | New `.rdata` evidence: literal `??_C@_0EJ@CJKOFANJ@` contains `tab_stop_index>=0 && tab_stop_index<=font_drawing_globals.tab_stop_count`, so the original global is `font_drawing_globals`. Base spelled `draw_string_globals` and emitted a different 72-byte literal, which was the missing 46th relocation. Other fixes: no separate string_index/font/cursor_x_start/line_top locals; all counters are short; justification is a `switch` with center before right; the result switch has code order {2,6}, 3, 4, 1, default. Loop-top declaration order is justification, segment_start_index, tab_stop_count, line_width, break_string_index, break_line_width, previous_result, segment_end_index, done, line_bounds. |
| `_draw_unicode_string` | `reopen_draw_unicode_string.c` (base + body + name macro) | Same body as `_draw_string`, with assert lines 848/849/881 and halt line 1017. The base used 871, which is wrong. |
| `_bitmap_draw_string` | `evidence_bitmap_draw_string_null_bounds_bug.c` | Exact only by reproducing a **BUG (original)**: January clips `bounds` inside `if (!bounds)`, which compiles to absolute reads at addresses 0..6. It also needs `MAX(0, r->x0)` / `MIN(bitmap->width, r->x1)` operand order. This is an owner decision because it is not a safe expression. |

Not exact:

| Function | Best | Residual |
| --- | --- | --- |
| `_parse_string` | `parse_string_best.body.txt` (ps5): 624/30, all 189 instructions and relocation identities equal | Pure esi/edi coloring swap between `state` and the `&state->string_index` / `can_end_words` web. The recovered structure is a `switch (result)`; escape case order p, i, b, k, u, l, r, c, n, t, read from the jump-table code order; and the kinsoku predicate with DB enum names `_parsed_end_of_word` (2) and `_parsed_character` (6). Note that HCEA inverts the ASCII can-end test relative to January. |
| `_bitmap_draw_character` | `bitmap_draw_character_best.body.txt` (bdc6): 944/25, the January size and relocation count | Evaluation order of `x0 << shift` vs `y0 * stride`, which of x0/y0 stays cached for the asserts, and a dx spill. Structure recovered: word R5G6B5 color set only in its branch; destination read before coverage; one expression per ARGB channel; bits-per-pixel call inline in the stride. |

Combined proposal `combined_v2.c` gates == exact 25 residual 3 (combined_v3.c with the BUG (original) wrapper:
26 / 2). All 19 baseline exact functions stay exact. The two bitmap parks stay BASE and `_parse_string` drifts
(it needs re-measurement). The owner census shows only January literal COMDATs. Fakescan finds 0 leads and the
point guard passes.

## 5. Do-not-repeat list

- styled_font_get: if/else, init+if, `!=`-first `?:`, hoisted assert with named font local (all measured, only
  the `==`-plain `?:` with accessor chain is exact).
- parse_string register swap: pointer local present/absent, declaration order, function-scope pattern locals,
  case order of `switch (result)`, if/else-if (worse, changes ABI), exact styled_font_get callee — all inert.
- draw_string/draw_unicode_string loop-top declaration orders other than the recorded one.
- bitmap_draw_character: operand order of the destination sum/product (b4, b5, b7, b8, b9, b10) inert or worse.
- MIN/MAX macros for the partial renderers' clip narrowing (moves `clip` into a register ABI).

## 6. Residual classification

- `_parse_string`: register allocation (esi/edi coloring). This is a measured fact: the instruction and relocation streams are identical.
- `_bitmap_draw_character`: operand-evaluation / spill scheduling in the per-row geometry and assert block
  (measured: size and relocations equal, 8 diff blocks).
- `_bitmap_draw_string`: source topology is proven, but the matching source contains January's own NULL dereference (policy).

## 7. Reopen criteria

- `_parse_string`: a same-compiler donor showing what decides callee-saved coloring between a pointer
  parameter and a block-scope pointer web of equal instruction streams.
- `_bitmap_draw_character`: new evidence for the order of the stride/shift terms or the assert temporaries.
- `_bitmap_draw_string`: owner ruling on reproducing a January-proven original bug (README "BUG (original)" rule
  vs the no-undefined-behaviour house rule).

## 8. Disposition and orchestrator proposals

- Real file: unchanged; object remains 19/28 exact in production.
- Park-reopen (clean): `_styled_font_get`, `_text_pick_draw_character`, `_draw_unicode_string_partial`
  (independent, base file + body only).
- Park-reopen (coupled): `_draw_string_partial` and `_draw_string` need the ps5 `_parse_string` body. Land
  that body in the same commit and re-measure `_parse_string`'s park (624/30, sha b68bfa11d9b3797e).
  `_draw_string` and `_draw_unicode_string` also need the assert text `font_drawing_globals.tab_stop_count`.
- Naming: the scratch candidates use `#define font_drawing_globals draw_string_globals`, the same mechanism the file
  already uses for `draw_character_software_globals`. The cleaner alternative is a `config/symbols.json`
  rename or split of `_draw_string_globals` (file_offset 4987144) into its authentic statics
  (`font_drawing_globals`, `draw_character_software_globals`, ...). The only consumers are
  `source/text/draw_string.c` and `config/parked.json`.
- Cleanup with the reopen: remove the unused `_parsed_ascii_character` / `_parsed_ascii_word_break` aliases
  (combined_v2 keeps 25 exact without them).
- Owner decision: `_bitmap_draw_string` BUG (original) body.
