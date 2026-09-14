# `tiff_file.obj` - opus5 150K house-clean lane, wave w1 (2026-09-14)

## Outcome

No source change landed. `source/bitmaps/tiff_file.c` is unchanged (`git diff --stat` is empty).

The unit stays at **1 exact / 2 residual / 0 unwritten**. The `_point_from_line3d` guard passes, and the unit has no parks.

| Function | January padded / relocs | Baseline | Result |
| --- | --- | --- | --- |
| `_tiff_get_bounds` | 112 / 6 | EXACT | EXACT (untouched) |
| `_tiff_export` | 1,056 / 37 | residual (sha, frame 0x118 vs 0x114) | NOT-LANDED. Three structural source facts recovered in scratch; allocation and layout residual remains. |
| `_tiff_import` | 1,344 / 51 | residual (sha) | SKIPPED-EXHAUSTED. One byte-neutral house-rule macro shape; frame-slot ranking observation recorded. |

## Prior ledgers

- `tiff_file_obj_jonas_fixed_point_closeout_20260830.md`
- `tiff_file_obj_jonas_recovered_envelopes_20260827.md`
- `opus_misc_small_admission_20260903.md`: a 1,040-byte donor was rejected.
- `safe_wave_residual_reconciliation_20260831.md`

The git history of the file (17 commits) was also reviewed. `branch_sweep` finds no history blob better than the current source.

## `_tiff_export`: new January-side evidence

The 20260830 closeout describes its source families as exhausted. The frame-size law (a `sub esp` gap is structural) justified re-reading the bytes. Four findings follow.

### 1. Row-size division

January computes the row size with `cdq; and edx,7; add eax,edx; sar eax,3`. This is VC7's plain signed `/ 8`.

The real file instead spells it by hand as `(row_bits + ((row_bits >> 31) & 7)) >> 3`. That form emits different instructions (`mov edx,eax; sar edx,1fh`). It appears to have been kept only because it pads the function to 1,056 bytes; `/ 8` alone gives 1,040.

### 2. Failed-to-open return

- The failed-open exit at `@0x3d6` loads the literal straight into EAX. It never stores to `error_message` (`[ebp-0x10]`).
- The out-of-memory exit at `@0x3bc` does store to `error_message`, and reloads it after `TIFFClose`.
- Conclusion: the original is `return "failed to open tiff";`, not an `error_message` assignment.

### 3. 16-bit channel values

- **a4r4g4b4 case (`@0x25c`):** 16-bit `shr ax,0xc`, `shr ax,4` and a destructive `shr dx,8`. Each channel is expanded with `(c & 0xf) | (c << 4)`.
- **a1r5g5b5 case:** `middle` is a 16-bit `shr dx,5`.
- **Pixel loads:** `mov r16, word` with no zero-extension.
- Conclusion: the channel intermediates are word-typed. The real file uses byte `high`/`middle` and `(high >> 4)` forms.

### 4. Remaining residual

**Register and slot allocation:**

| Item | January | VC7 (ours) |
| --- | --- | --- |
| bitmap | EDI | EBX |
| tiff | EBX, homed in the bitmap parameter slot `[ebp+0xc]` | its own stack slot, with its later uses in EDI |
| photometric | on the stack; shares `[ebp-4]` with `source_row` | EDI |

**Block layout:** January chains the a8 case straight into the `TIFFOpen` block. It emits the RGB case and the invalid-format return after the inner jump table.

## `_tiff_export`: shapes

All shapes are in `scratch/workers/tiff_dead_camera/tfN.c`. Each builds on the previous one.

| Shape | Change | Result |
| --- | --- | --- |
| tf1 | `row_size = (short)(bits_per_pixel * width / 8)` | division now instruction-identical; 1,040 padded; 85 aligned diff blocks |
| tf2 | + one `TIFFClose` after the `row_buffer` if/else | no change |
| tf3 | + `else { return "failed to open tiff"; }` | failed-open block identical to January |
| **tf4** | + word channel temporaries (a4r4g4b4 alpha/green/red, a1r5g5b5 middle) | **1,056 / 37 relocs; 46 diff blocks**; frame 0x11c (red spilled) |
| tf5 | + RGB case before a8 | layout moves the wrong way; rejected |

`tf4.c` is the best scratch candidate, and its fake scan is clean. It is not strict exact, so it did not land.

**Classification:** class C allocation cluster plus switch layout.

**Reopen criterion:** start from `tf4.c`. Reopen only with evidence that moves bitmap into EDI and tiff into EBX. Examples would be a recovered local layout, or a per-case channel spelling that computes each word shift lazily as January does.

## `_tiff_import`

The alndiff confirms that control flow, the tail-duplicated error exits and all 51 relocation identities agree with January. Two differences remain.

### Stack-slot order

The stack slots of the TIFFGetField locals are permuted. Bracketed names are spilled register variables rather than address-taken locals.

- **January, top to bottom:** image_width, [y], [bitmap], samples, planar, orientation, [tiff], bits, image_height, photometric
- **Ours, top to bottom:** [y], image_width, samples, [tiff], [bitmap], bits, planar, orientation, image_height, photometric

One observation is new: January ranks `bits_per_sample` below `planar`/`orientation`, while ours ranks it above. That means January gives `bits_per_sample` the frame weight of a two-reference local. No natural spelling that removes a reference was found.

### Scanline register

The scanline register is the already-documented EBX-vs-EDI allocation.

### Shape tried

- **ti1:** `PIN(y, 0, image_height - 1)` and `PIN(x, 0, image_width - 1)`.
  - This is the house-rule cseries macro for the existing ternaries.
  - The result is byte-identical (sha `6f9acb1e`), so it was not applied.

**Reopen criterion:** authentic evidence for the reference count of `bits_per_sample` (for example, a recovered expression for the unsupported-bits diagnostic), or a live-range donor for the scanline's EBX home.

## Checks

- The final real-file gate equals the baseline (1/2/0) and the guard passes.
- `_tiff_get_bounds` stayed EXACT in every scratch shape.
- There are no parks. The fake scan finds 0 leads in the real file and in `tf4.c`.
- There are no header or config proposals.
