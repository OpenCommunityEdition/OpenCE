# `tiff_file.obj` opus5 150K house-clean wave w3 (2026-09-14)

## Outcome

No source change landed; `source/bitmaps/tiff_file.c` is unchanged (`git diff --stat` empty). Final real-file gate equals the
baseline: **1 exact / 2 residual / 0 unwritten**, `_point_from_line3d` guard passed, no parks, fake scan 0 leads.
Worker notes: `scratch/workers/w3_bitmap_extract_tiff_file.md`; probes `scratch/workers/w3_bitmap_extract_tiff_file/tf/`.

| Function | January padded / relocs | Result |
| --- | --- | --- |
| `_tiff_get_bounds` | 112 / 6 | EXACT (untouched; exact in every whole-TU probe) |
| `_tiff_export` | 1,056 / 37 | NOT-LANDED: two new structural recoveries (switch layout, a4r4g4b4 expressions); allocation cluster remains |
| `_tiff_import` | 1,344 / 51 | SKIPPED-EXHAUSTED: one allocation decision; no applicable law |

Ledgers read: `tiff_file_obj_jonas_recovered_envelopes_20260827`, `tiff_file_obj_jonas_fixed_point_closeout_20260830`,
`tiff_file_obj_opus5_150k_w1_20260914`, w1 notes `scratch/workers/tiff_dead_camera.md`, `opus_misc_small_admission_20260903`,
`safe_wave_residual_reconciliation_20260831`, `scratch/w2/laws_w2.md`.

## `_tiff_export`

### New January-side findings

1. **Single exit plus threaded layout (A3).** January emits the a8 case directly before the `TIFFOpen` block and floats the RGB case
   (0x15c) and the invalid-format return (0x174) after the second switch's jump table. The spelling
   `default: error_message = "invalid bitmap encoding for tiff export."; break;` followed by
   `if (!error_message) { ... TIFFOpen ... } return error_message;` (with `error_message = "failed to open tiff";` in the open-failure
   arm) makes VC7 thread the case-to-open edges (error_message is known NULL there) and float exactly those blocks: the RGB case
   lands at 0x15c and the invalid epilogue at 0x172/0x174, as in January. The w1 RGB-first case order (tf5) was the wrong lever.
2. **a4r4g4b4 channels are per-store inline expressions**, e.g.
   `row_buffer[x * 4 + 3] = ((byte)(pixel >> 12) & 0xF) | ((byte)(pixel >> 12) << 4);`, in store order +3, +2, +1, +0. The case
   body is then instruction-identical to January 0x25c-0x2bd, including the destructive `shr dx,8` for red. Without the `(byte)`
   casts VC7 folds the channels into an xor/and/xor form. w1's word temporaries spilled red to `[ebp-0x18]`. With the expression
   form the frame drops from 0x11c to 0x118.

### Shapes (base t0 = w1 tf4: `/ 8` division, word `middle`, word channel temporaries)

| Shape | Change | Result |
| --- | --- | --- |
| t1 | single-exit default + `if (!error_message)` wrapper | 1,056/37; first-switch layout identical to January |
| t2 | t1 + inline a4r4g4b4 without casts | 1,040; wrong xor folding |
| t3 | t1 + inline a4r4g4b4 with `(byte)` casts | case instruction-identical; frame 0x118; 1,040 padded |
| t4 | t3 + tiff/row_size/row_buffer/y scoped into the wrapper block | byte-identical to t3 (inert) |

### Remaining residual (class C)

- January keeps bitmap in EDI for the whole function.
- It keeps tiff in EBX, with a shrink-wrapped `push ebx` after the switch, homed in bitmap's parameter slot `[ebp+0xc]`.
- It memory-homes photometric and samples, sharing their slots with source_row and x.
- VC7 on t3 instead puts bitmap in EBX (spilled around the byte-register pixel loops), puts photometric and y in EDI, and gives
  tiff its own slot (+4 frame).
- In January's long-format cases x is homed at `[ebp-4]` (source_row's slot), while the word cases home x at `[ebp-8]`. This
  suggests per-case pointer or x webs. It was not tested within the budget.

Reopen criterion: start from `tf/t3.c`; reopen only with evidence that assigns bitmap to EDI and tiff to EBX (e.g. per-case
typed source pointers or x locals that let x share source_row's slot as in January's long cases). Do not repeat t2 or t4.

Policy note for the orchestrator (no action taken): the real file still spells the row size as the hand-expanded
`(row_bits + ((row_bits >> 31) & 7)) >> 3`. January's bytes prove plain `/ 8`, and w1 showed the expansion only pads the
envelope. It remains until an exact body replaces it, per the already-written-residual rule.

## `_tiff_import`

The alignment diff against the baseline object has 71 blocks. Control flow, the tail-duplicated goto-cleanup exits, the
`_snprintf` pushes of the cached bits (cx) and samples (ax), and all 51 relocation identities agree.

Every difference follows from one allocation decision at 0x1ce:

| Item | January | Ours |
| --- | --- | --- |
| scanline | EBX | EDI |
| bitmap | ESI, then homed at `[ebp-0x18]` | homed at `[ebp-0x20]` |
| y | ESI | EBX |

The TIFFGetField slot permutation recorded by w1 is derived from this decision. The frame ranking counts generated home
references, so the permutation is not independent source evidence. A3's early literal epilogues are already reproduced. No shape
was spent. Reopen criterion (unchanged): a live-range donor or authentic evidence for the scanline and y webs.

## Checks

- Final real-file gate is 1/2/0 and the guard passed.
- `_tiff_get_bounds` was EXACT in t1 (whole-TU gate).
- No parks exist for this unit.
- The fake scan reports 0 leads on the real file and on `tf/t3.c`.
- No header or config proposals.
