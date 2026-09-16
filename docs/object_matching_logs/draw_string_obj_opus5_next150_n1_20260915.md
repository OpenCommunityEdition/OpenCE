# `draw_string.obj` — next-150K lane, wave n1 (2026-09-15)

## Result

`source/text/draw_string.c` is **unchanged in the real file** (all three residuals are locked parks).
`_bitmap_draw_character` is **strict EXACT in scratch** and is handed over as a **park-reopen proposal**.

| Function | Target padded / relocs | Real file (park base) | This wave | Disposition |
| --- | --- | --- | --- | --- |
| `_bitmap_draw_character` | 944 / 25 | 912 / 25 | **944 / 25, normalized sha equal, 299/299 insns** | PARK-REOPEN-PROPOSED (`scratch/workers/n1_draw_string/n4.c`) |
| `_bitmap_draw_string` | 304 / 9 | 240 / 9 | not attempted (out of scope) | OWNER-BLOCKED (UB ruling) |
| `_parse_string` | 624 / 30 | 624 / 30 | not attempted (out of scope) | SKIPPED-EXHAUSTED (laws_w2 §D) |

Real-file gate at baseline and at the end of the wave, with the `_point_from_line3d` guard:
`== exact 25 residual 3 unwritten 0`. Row listings identical; `git diff --stat` empty.
Candidate gate: `== exact 26 residual 2 unwritten 0`, one changed row.

## Duplicate prevention

- Ledgers read in full: `draw_string_obj_jonas_hcea_one_shot_20260828.md`,
  `draw_string_obj_full_reconstruction_20260902.md`,
  `draw_string_obj_tab_stops_opus_reconciliation_20260903.md`,
  `draw_string_obj_opus5_100k_waveD_20260914.md`, `draw_string_obj_opus5_150k_w2_20260914.md`;
  worker notes `scratch/workers/draw_string_bitmap_group.md`; `scratch/w2/laws_w2.md`, `scratch/w3/laws_w3.md`.
- `git log --all --oneline -- source/text/draw_string.c`: 13 commits, no exact body.
- `branch_sweep source/text/draw_string`: 10 unique blobs, best historical 19 exact vs the current 25.
- The 16 w2 bodies (C, C1-C4, C6, C8, C9, CA, CW, CY, L1, L2, S1, S2, V2, V3, O1) and waveD's bdc1-bdc10
  were all re-read before any new shape; none of them is repeated here.

## `_bitmap_draw_character` — census first

Starting body: w2's best, `scratch/workers/draw_string_bitmap_group/dsL1.c`, re-gated at this baseline
(944/25, `[sha]` only).

**Frame census.** January `sub esp,0x20`, ours `sub esp,0x20`; every `[ebp±N]` slot *and its reference
count* is identical (`+0xc`×5, `+0x10`×2, `+0x14`×2, `+0x15`×1, `+0x16`×1, `+0x18`×2, `+0x1c`×4,
`+0x20`×1, `+0x24`×3, `+0x28`×2, `+0x2c`×9, `-0x4`×4, `-0x8`×6, `-0xc`×7, `-0x10`×3, `-0x14`×2,
`-0x18`×2, `-0x1a`×1, `-0x1c`×3, `-0x20`×2). No missing or extra local; January reuses the dead `font`
home `[ebp+0xc]` for the R5G6B5 colour phi and the dead `dy` home `[ebp+0x2c]` for the source pointer,
and so does the candidate.

**Relocation census by target.** 25 target / 25 candidate / **0 differing rows**:
`_bitmap_format_get_bits_per_pixel`×1, `_display_assert`×3, `_system_exit`×3, `_draw_string_globals+0`×5,
`_draw_string_globals+4`×1, the three assert literals (×3/×1/×1) and six internal jump-table entries, all
equal by address, type, target and addend. The `"### ERROR unsupported bitmap format"` literal is an
undefined external in January's csplit object and a defined `.rdata` item in ours — resolved-equal and
identical at HEAD (linker string folding, a csplit artifact).

**Conclusion of the census.** The `[size 912!=944]` tag belongs to the *parked* body, not to w2's; at
dsL1 the object was byte-for-byte equal except ONE 4-instruction ordering block at `+0xae..+0xcc`:

```
JAN   movsx esi,dx ; shl esi,cl ; movsx ecx,[ebp+0x1c] ; sar eax,3 ; imul ecx,eax
      mov eax,[ebx+0x2c] ; add esi,ecx ; ... ; add esi,eax
OURS  movsx edx,[ebp+0x1c] ; mov esi,eax ; sar esi,3 ; imul esi,edx
      mov dx,[ebp+0x18] ; movsx eax,dx ; shl eax,cl ; ... ; add esi,eax ; add esi,[ebx+0x2c]
```

January evaluates `x0 << encoding_shift` into the accumulator first and materialises
`bitmap->base_address` into a register; the candidate evaluated `y0 * (bits_per_pixel * width / 8)`
first and folded the base as a memory operand.

## The lever

w2 had already proved that *every* source permutation and association of the three-term sum is inert
(C2 product-first, C4 reversed multiply, C6/C8 pointer-last, CA parenthesised, C9 `&p[a+b]`): VC7
canonicalises the sum and orders the addends by rank. Ranks at the divergence point:

- `x0 << encoding_shift` — one fresh leaf (the shift is already in `CL` in both builds);
- `y0 * (bits_per_pixel * bitmap->width / 8)` — one fresh leaf **plus a live pending temp in EAX**
  (the unfinished signed divide: `cdq; and edx,7; add eax,edx` is emitted before the divergence in both
  builds, and only the closing `sar eax,3` sits inside the multiply).

The multiply therefore outranks the shift and wins the accumulator. Removing the pending temp — by
giving the divisor sub-expression its own local — levels the ranks and restores source order.

w2's C3 (`short bits_per_pixel = bitmap_format_get_bits_per_pixel(...);`) had been read as evidence
against any named intermediate, because it flushed the deferred `add esp,4`. That reading is too wide:
January's pop sits at `+0xe2`, two statements past the call, which only proves the call's result is
**still being consumed by arithmetic in the same expression**. A local initialised with an expression
that *contains* the call keeps the deferral; only a local initialised with the *bare* call result flushes
it.

## Shapes (4)

| Shape | Base | Change | Result |
| --- | --- | --- | --- |
| N1 | dsL1 | `long stride = bitmap_format_get_bits_per_pixel(...) * width / 8;` then `y0 * stride` | **EXACT 944/25** on the first probe |
| N2 | HEAD real-file body | the same named local + `destination_pixel` before `source_pixel` | 928 `[size]` — HEAD's inner-loop shapes are also wrong |
| N3 | N1 | re-spelled in house style: real-file names, `if (dy <= 0) return;`, multi-line `match_assert`, `&glyph_pixels[...]` | **EXACT 944/25** |
| N4 | N3 | `stride` → `row_pitch` | **EXACT 944/25**, gate byte-identical to N3 |

`row_pitch` follows the project's own vocabulary: January names the function
`bitmap_mipmap_get_row_pitch` (`source/bitmaps/bitmaps.c:52`, `source/bitmaps/bitmaps_mipmap.h:41`) and
`source/rasterizer/rasterizer_swizzle.c:550` already declares `long row_pitch;` for exactly this
quantity. `long` is also the correct type: `bits_per_pixel * width` overflows a `short` at 32 bpp.

## Source-level defects the reopen fixes

1. **The per-row byte pitch is its own local** (the decisive new hunk):
   `long row_pitch = bitmap_format_get_bits_per_pixel(draw_character_software_globals.bitmap->format) *
   draw_character_software_globals.bitmap->width / 8;` with the destination spelled `... + y0 * row_pitch`.
2. **`destination_pixel` is declared before `source_pixel`** (w2's V2 measured the reverse at 928).
3. **The three column loops are up-counters** `for (column = 0; column < dx; column++, ...)`; the outer
   row loop stays the guarded down-counter `for (row = dy; row > 0; row--, y++, y0++)` behind
   `if (dy <= 0) return;`. (HCEA's PPC-2011 reconstruction uses down-counters throughout; that is a later
   source revision, not January.)
4. **`word destination_color;` has no initializer.** January's else arm is
   `+0x5f mov edi, dword ptr [ebp+0xc]` — it reads back the *dead `font` parameter home* instead of
   materialising `color` from EDX, which proves the original local was not initialised. HEAD's
   `pixel32 destination_color = color;` is a reconstruction defect. There is **no source-level
   uninitialised read**: the variable is read only inside `case _bitmap_format_r5g6b5`, the arm that
   assigns it. HCEA annotates the same slot as "was uninitialized stack".
   The channel pack is written as intent —
   `(word)(((((color >> 16) & 0xFF) >> 3) << 11) | ((((color >> 8) & 0xFF) >> 2) << 5) | ((color & 0xFF) >> 3))` —
   which VC7 folds to January's `and edi,0xfff8 / shl 5 / shl 3` because the `(word)` truncation makes the
   alpha bits dead.

## New measured laws (for the lane law file)

**NAMED-INTERMEDIATE OPERAND RANK [new].** In a canonicalised commutative integer or pointer sum, VC7
evaluates the highest-rank addend into the accumulator first and folds the cheapest addend as the memory
operand of the final `add`. Source permutations and associations of the sum are inert. Binding one
addend's sub-expression to a named local lowers that addend's rank, restores source order, and forces the
remaining memory addend into its own register (`add esi,[ebx+0x2c]` → `mov eax,[ebx+0x2c]; add esi,eax`).
*Detect:* an identical frame, identical relocations and identical instruction counts, with January and
the candidate differing only in which addend initialises the accumulator — then look for a sub-expression
that should be a named local. *Closed:* `_bitmap_draw_character` 944.

**CALL-RESULT POP DEFERRAL [refines laws_w2 A14 and laws_w3 A31(a)].** VC7 defers a cdecl `add esp,N`
past following statements only while the call's result is still consumed by arithmetic in the same
expression. A named local initialised with the **bare** call result flushes the pop immediately (w2 C3);
a named local initialised with an expression that *contains* the call keeps the deferral (this wave, pop
still at `+0xe2`). "January defers the pop" therefore refutes a bare-call local but does **not** refute a
named intermediate for the surrounding arithmetic.

## Verification of the candidate `scratch/workers/n1_draw_string/n4.c`

1. Whole-TU gate: `_bitmap_draw_character` strict **EXACT**, 944/944 padded, 25/25 relocations,
   normalized sha equal, `relocdiff` 0 differing rows, `alndiff` 299/299 with no code block.
2. Row-by-row diff of the full `--all` listings vs baseline: exactly one line changes
   (`residual 944 _bitmap_draw_character [size 912!=944, sha]` → `EXACT 944`). No baseline-exact row lost.
3. `_point_from_line3d` emitted-symbol guard passes.
4. `scratch/parkcheck.py`: only `_bitmap_draw_character` drifts, and it drifts *to the target
   measurement* 944 / 25 / `1fbab7db0314ed56`. `_bitmap_draw_string` and `_parse_string` unchanged.
5. Owner census vs `build/base/source/text/draw_string.obj`: identical defined-owner set
   (code/data/BSS/COMMON), no storage-class or section change, no new or removed COMDAT; only the
   compiler-internal `$L####` jump-table label numbers move (10 vs 10). vs `build/split`: the same seven
   linker-folded `.rdata` items that HEAD already owns (systemic select-any class, laws_w3 A30).
6. `tools/fake_match_scan.py`: 0 review leads, same as HEAD.
7. `CL /Zs /W3`: 13 warnings, byte-for-byte the same set as HEAD; no new warning.
8. The diff vs the real file is confined to lines 498-586, i.e. entirely inside `bitmap_draw_character`.

## Disposition

- **Park-reopen (independent).** Land the `_bitmap_draw_character` body from
  `scratch/workers/n1_draw_string/n4.c` and delete the `source/text/draw_string::_bitmap_draw_character`
  entry from `config/parked.json` in the same commit. No sibling park needs a re-baseline; no header,
  config or `symbols.json` change is required.
- The park's criterion ("Reopen only for authoritative January source/local records or a natural
  same-compiler donor that explains the remaining allocation and schedule gap") has its **premise
  refuted**: the gap was never allocation or schedule. The frame and relocation censuses are identical
  and the whole residual was four source-shape defects; the function is now strict EXACT in ordinary
  house-rule C.
- `_bitmap_draw_string` and `_parse_string` keep their parks and their existing reopen criteria
  unchanged; neither was touched this wave.
