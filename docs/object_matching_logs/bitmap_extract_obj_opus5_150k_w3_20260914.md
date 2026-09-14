# `bitmap_extract.obj` opus5 150K house-clean wave w3 (2026-09-14)

## Scope and baseline

- Unit `source/bitmaps/bitmap_extract.c`, target `build/split/source/bitmaps/bitmap_extract.obj` (January 2002 Xbox debug,
  VC7 13.00.9254). Lane HEAD `403108e28`. Worker notes: `scratch/workers/w3_bitmap_extract_tiff_file.md`; scratch dir
  `scratch/workers/w3_bitmap_extract_tiff_file/`.
- Baseline real-file gate (`--all --forbid-emitted-symbol _point_from_line3d`): **exact 18 / residual 3 / unwritten 0**, guard
  passed. The three residuals are parked and locked: `_extract_build_texture_pages_by_sequence` (608), `_extract_sequence` (720),
  `_extract_sprites` (1104). Park base measurements verified with `scratch/parkcheck.py` (0 drift).
- The real source file was **not modified** (`git diff --stat` empty). Final real-file gate identical to baseline (18/3/0, guard
  passed), park drift 0/3, owner census clean, fake scan 0 leads.
- Previous ledgers read: `_fable_backlog_reconciliation_20260902`, `_3d_texture_reconstruction_20260903`,
  `_private_helper_dependency_audit_20260903`, `_remaining_private_packet_20260912`, `_opus5_100k_waveC_20260914`,
  `_opus5_150k_w1_20260914`, plus the w1 worker notes and `scratch/w2/laws_w2.md`. No w2 ledger exists for this object.

## Outcomes

| Function | Padded | Outcome | Candidate |
| --- | ---: | --- | --- |
| `_extract_sprites` | 1104 | PARK-REOPEN proposed (scratch strict EXACT) + coupled builder park re-measure | `scratch/workers/w3_bitmap_extract_tiff_file/be_reopen_sprites.c` |
| `_extract_build_texture_pages_by_sequence` | 608 | not landed (one floated arm); re-measure 672 -> 608 proposed with the sprites reopen | same candidate |
| `_extract_sequence` | 720 | skipped: January-proven redundant branch, owner ruling | prev-lane `seqe` |

## `_extract_sprites` (1104 padded, 49 relocations, sha b853b0998fdec2cd)

Candidate `be_reopen_sprites.c` (sha256 `1dfeb829ed488400646344ffb046de2ba2cca1bf75a4590a8ccd96e115ba2542`) = current real file +
the w1 stack-ABI builder body (`pg1`) + the new sprites body. Whole-TU gate: **19 exact / 2 residual**; the row-by-row diff against
the baseline listing changes only the builder tag (`[size 672!=608, sha]` -> `[sha]`) and sprites (residual -> EXACT).

January evidence (target disassembly):

- `0xfa: xor ebx,ebx; test dl,dl; mov [ebp-0xc],ebx; jne 0x110; <epilogue mov al,dl>`, reached from the first loop's
  result-false exit by `jmp 0xfa`: the page loop's `page_index = 0` precedes its `result &&` entry test, so there is no early
  `if (!result) return result;` (single-exit law A3).
- The first-loop index lives in EBX across `error()` and is never homed, while the page-loop index is homed at `[ebp-0xc]` and
  `page_bitmap` shares `[ebp-0x10]` with the spilled maximum dimension: two index variables. An element pointer
  `struct bitmap_extract_entry *entry` in the first loop (the same spelling as the inner loop) is what moves the index into EBX and
  produces January's early zero pin (`xor ebx,ebx` serving total = 0, `cmp ax,bx`, background[0], index = 0).
- The three `background_colors` stores precede the maximum-dimension spill store: `background_colors` is declared before
  `maximum_bitmap_dimension` (A20).
- The budget-exceeded epilogue loads `xor al,al` next to `mov al,[ebp-1]`/`mov al,dl` epilogues and result is cached in DL from
  `mov dl,1`: `result = FALSE;` with the single `return result;` instead of a second `return FALSE;` (A3).
- The budget test `fcom 0.0; test ah,44h; jp` is equality (`== 0.0f`); the parked `<= 0.0f` was a reconstruction defect.
- January pushes four builder arguments: the builder must have the stack ABI, which the parked 672-byte body does not give (A8).

Shapes (base `sp0` = real file + `pg1` builder + w1 `s2` sprites): s1 no early return (69 blocks, loop exit now as January);
s2 separate `short page_index`; s3 triple `extract_data.bitmaps[i].bitmap` (no change); s4 `entry` pointer (zero pin, index EBX,
dimension spilled: 54 blocks); s5 background before dimension (store order identical); **s6 `result = FALSE` single exit: strict
EXACT**. Control: the s6 body on the real parked builder is not exact.

Checks: point guard passed; `parkcheck`: `_extract_sequence` OK, `_extract_sprites` drifts to the target, the builder drifts
672/22 `a0a34a20` -> **608/22 `da8246c856fc6d86`** (strictly closer: target size, relocations equal); owner census vs
`build/base` and `build/split`: no candidate-only or base-only owner; fake scan 0 leads.

Authenticity: validate that every sprite fits a page less spacing; pack pages per sequence; fill each page bitmap with the usage
background, copy its sprites (normalised registration point and bounds, sequence first index/count), add it to the group and
accumulate its area; report the budget. `result` is the single success flag returned once. Behaviour equals the parked body apart
from the January-proven `== 0.0f` budget test.

Park criterion (`config/parked.json`): fail-closed unclassified with no stronger diagnosis. The premise is refuted: once the callee
has January's stack ABI, the single-exit, entry-pointer and separate-page-index shapes are exact.

## `_extract_build_texture_pages_by_sequence` (608 padded, 22 relocations, sha 70092abc226e6d94)

The `pg1` body (short spanned count proven by `movsx` for `fprintf`, MAX then `MIN(512, ceiling_power2())` per axis, `>> 1` shrink
proven by `sar`, `new_page` assigned in each arm) differs from January only in block placement: January floats the
`page_count < 32` test and new-page arm (`cmp bx,20h; jge; xor esi,esi; mov [ebp-2],1; jmp 5ah`) after the first MIN arm's `jmp`
barrier, and the existing-page arm falls into the bitmap-loop join.

Rejected this wave (do not repeat): b1 `new_page = page_index >= page_count` (setge, worse); b2-b4 default initialisers with arm
overrides (defaults emitted before the compare); b5 `while (!page_complete)` (byte-identical); c1 page tests in the do-while
condition (624); c3 `while (B || A)` with new-arm-first body (624); c6 if/else followed by `if (new_page && page_count >= 32)
break;` (test threaded away, arm still inline). Previous do-not-repeat: w1 p2-p8.

Proposal: re-baseline the park to the `pg1` body in the same commit as the sprites reopen (A8 callee-ABI cascade). Reopen
criterion: a natural spelling in which only the new-page arm is reached through a relocated edge while the existing-page arm is the
fall-through of `page_index < page_count` (w1's p6 mirror image).

## `_extract_sequence` (720 padded, 28 relocations)

Re-read of 0x181-0x1b0: `if (found_bottom_reference || single_sequence)` then `!found_bitmap` and `state == 1` cross-jumped into the
else arm's `state == 1 && !found_bitmap` tail; the found_bitmap-true edge re-enters the else test and reloads `found_bitmap`. Every
spelling that reproduces this is behaviourally redundant (w1 proof stands); no w2 law gives the branch a distinct meaning. No shape
spent. Reopen: owner ruling on January-proven behaviour-neutral branches (prev-lane `seqe`, exact on the current tree), or
authentic source giving the arm a distinct meaning.

## Proposals

1. **park-reopen** `_extract_sprites` from `be_reopen_sprites.c`, with the coupled park re-measure of
   `_extract_build_texture_pages_by_sequence` to base size 608, relocation_count 22, normalized_sha256
   `da8246c856fc6d86814908319b6600b6ff89bd4564dcff9222904c77bb51f359`. No header or config text is required.
2. The w1 names-only atlas batch (`process_sprites`, `build_texture_pages_by_sequence`, `extract_bitmaps_in_row`, ...) remains open.
