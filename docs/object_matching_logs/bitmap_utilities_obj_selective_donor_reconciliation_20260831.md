# `bitmap_utilities.obj` selective donor reconciliation

## Scope and result

This lane started from canonical commit `40a912a2f` and selectively reviewed
`source/bitmaps/bitmap_utilities.c` from
`C:\halo-worktrees\claude-breakable-surfaces-20260830` at donor commit
`e02971dfd`.  The donor file was not transplanted wholesale.  Each retained
body was compared independently against the January target with the PID-safe
`tools/campaign/gate.py` harness and then reconciled to the current house
rules.

The canonical baseline was `17 exact / 0 residual / 31 unwritten`.  The
retained source gates at `36 exact / 4 residual / 8 unwritten`: all seventeen
canonical exact functions survive, nineteen donor gains are strict exact, and
one otherwise exact donor gain is retained as a typed 99.922264% scheduling
residual.  The nineteen new exact functions cover 8,640 padded target bytes.
Including the honest 880-byte typed residual, this lane reconstructs the full
9,520-byte useful donor scope without accepting the donor's DXT3 type error.

No compiler option, object permutation, comparator exception, inline assembly,
volatile access, barrier, register hint, fake dependency, undefined behavior,
manual byte patch, or nonsensical exact-only branch was used.

## Evidence and naming

The semantic private names were authenticated against HaloCEA commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`, principally the implementations
under `src/blam/bitmaps/bitmap_2d_*.c`, `bitmap_3d_*.c`, and
`bitmap_cm_*.c`.  They replace every `_code_<address>` identifier in this
compiland with the `bitmap_2d_*`, `bitmap_3d_*`, or `bitmap_cm_*` family name
shown by the recovered topology.  The previously established
`bitmap_2d_vector_map`, `bitmap_3d_vector_map`, and `bitmap_cm_vector_map`
names and their exact bodies were preserved.

The 0x414-byte target BSS owner was also split semantically instead of leaving
`_bss_0031bf00`. January relocations prove two 0x200-byte lookup tables at
offsets `+0x000` and `+0x200`, used by `bitmap_sharpen`, followed by a
ten-word (`0x14` byte) filter at `+0x400`, used by `bitmap_smooth`. Subsequent
completion of the sharpen family established the two table roles more strongly
than this lane could: January builds the neighbor-subtraction values at the
base address and the center-positive values at `+0x200`, then uses them in
exactly those roles in `bitmap_2d_sharpen`. The corrected restrained names are:

- `bitmap_sharpen_negative_table[256]` at `0031BF00`;
- `bitmap_sharpen_positive_table[256]` at `0031C100`;
- `bitmap_smooth_filter_coefficients[10]` at `0031C300`.

The arrays are named in the target manifest but are not emitted speculatively
while the two public smooth/sharpen functions and their six private helpers
remain unwritten.  The name `oo_unsigned_short_max` for the exact 1/65535
rdata constant is convention-derived, not claimed as PDB-authenticated; it
follows the existing Bungie reciprocal-limit naming family and the constant's
two color-conversion uses.  The January bytes remain authoritative.

The open Marathon source was searched for this bitmap utility, S3TC decode,
and sharpen-table family.  It contains no directly corresponding API or
implementation, so no Marathon code was imported.

## Strict exact set

The seventeen inherited exact functions are:

- `argb_color_to_real_argb_color`;
- `bitmap_fill` and `bitmap_alpha_to_rgb`;
- `real_rgb_color_brightness`;
- `rgb_color_to_hsv_color` and `hsv_color_to_rgb_color`;
- `real_rgb_color_to_real_hsv_color` and
  `real_hsv_color_to_real_rgb_color`;
- `rgb_color_to_real_rgb_color`;
- `pixel32_to_real_argb_color` and `pixel32_to_real_rgb_color`;
- `valid_real_rgb_color` and `rgb_colors_interpolate_and_scale`;
- `bitmap_vector_map` and its exact 2D, 3D, and cube-map helpers.

The nineteen new strict-exact functions are:

| Function | Padded bytes |
| --- | ---: |
| `bitmap_alpha_bleed` | 160 |
| `bitmap_clone` | 400 |
| `bitmap_compress_to_mipmap` | 528 |
| `bitmap_fade` | 400 |
| `bitmap_height_map` | 208 |
| `bitmap_shrink` | 208 |
| `bitmap_uncompress_from_mipmap` | 512 |
| `rgb_colors_interpolate` | 544 |
| `bitmap_cm_shrink` | 400 |
| `bitmap_3d_alpha_bleed` | 272 |
| `bitmap_cm_alpha_bleed` | 160 |
| `bitmap_cm_height_map` | 176 |
| `bitmap_2d_compress_to_mipmap` | 512 |
| `bitmap_3d_compress_to_mipmap` | 720 |
| `bitmap_cm_compress_to_mipmap` | 704 |
| `bitmap_3d_uncompress_from_mipmap` | 704 |
| `bitmap_cm_uncompress_from_mipmap` | 688 |
| `bitmap_2d_height_map` | 1,072 |
| `bitmap_3d_height_map` | 272 |

## Honest residual boundary

Four ordinary-C bodies are retained and recorded in `config/parked.json`:

| Function | Target envelope | Relocations | Objdiff | Boundary |
| --- | ---: | ---: | ---: | --- |
| `bitmap_2d_shrink` | 656 | 18 | 95.15126% | 244 candidate instructions versus 245 target instructions; the nested sampling loop chooses a different accumulator/address-call register schedule. |
| `bitmap_3d_shrink` | 800 | 18 | 77.116196% | The complete nested voxel shrink topology is present, but scale/dimension/pointer lifetimes cascade across callee-saved registers and five additional candidate instructions. |
| `bitmap_2d_alpha_bleed` | 560 | 24 | 99.934784% | The only difference is two independent stack reloads reversed at `+0x191/+0x194`. |
| `bitmap_2d_uncompress_from_mipmap` | 880 | 49 | 99.922264% | The only difference is two independent final-loop reloads reversed at `+0x33A/+0x33D`. |

For the decompressor, target and candidate both contain 297 aligned
instructions.  The retained DXT3 branch uses the canonical
`struct s3tc_block_alpha4`, `DecodeBlockAlpha4` prototype, and alpha4 size.
A bounded diagnostic using the donor's incorrect alpha3 cast and size did not
alter the final reload tie, so that spelling was rejected.  DXT5 continues to
use `struct s3tc_block_alpha3`.

## Header and COMDAT boundary

The donor carried foreign BITMAPS.C and S3TC prototypes inside
`bitmap_utilities.c`, plus a second local `fast_ftol`.  The retained source
instead uses:

- `source/bitmaps/bitmaps_internal.h` for the narrow BITMAPS.C
  allocation/address/slice/face interface;
- `source/bitmaps/bitmap_utilities.h` for this compiland's recovered public
  interface;
- `source/bitmaps/s3tc/s3tc.h` for the canonical RGB, alpha4, and alpha3
  block types and decoders;
- the shared authenticated `fast_ftol` from `cseries.h`.

No second `fast_ftol` definition or new point/math COMDAT owner is introduced.
This preserves the established inline ownership schedule.

An intermediate broad expansion of `source/bitmaps/bitmaps.h` was rejected by
measurement.  Its eleven direct C consumers were censused:

1. `source/bitmaps/bitmap_utilities.c`
2. `source/bitmaps/bitmaps_quantitize.c`
3. `source/bitmaps/bitmaps.c`
4. `source/bitmaps/tiff_file.c`
5. `source/cutscene/cinematics.c`
6. `source/effects/particles.c`
7. `source/objects/objects.c`
8. `source/objects/widgets/light_volumes.c`
9. `source/physics/breakable_surfaces.c`
10. `source/rasterizer/xbox/rasterizer_xbox_hardware_bitmaps.c`
11. `source/units/units.c`

The broad header preserved the complete non-exact set of the other nine
non-utility consumers and kept `objects.obj` at `121/121`, but it reopened
`unit_preprocess_node_orientations`, changing `units.obj` from `189/189` to
`188 exact / 1 residual`.  That definition-position sensitivity is not a
semantic dependency.  The broad edit was removed; `bitmaps.h` now has the
same Git blob hash as canonical (`4aa8adde9951566290b1809f787e41ca9366d71f`).
The two dedicated headers are consumed only by `bitmap_utilities.c`, so every
shared-header consumer returns to its unchanged canonical preprocessor input.

The measured unchanged consumer baselines were:

| Unit | Gate |
| --- | --- |
| `bitmaps_quantitize` | 2E / 1R / 0U |
| `bitmaps` | 9E / 0R / 25U |
| `tiff_file` | 1E / 2R / 0U |
| `cinematics` | 16E / 1R / 0U |
| `particles` | 13E / 0R / 7U |
| `objects` | 121E / 0R / 0U |
| `light_volumes` | 9E / 1R / 0U |
| `breakable_surfaces` | 11E / 1R / 0U |
| `rasterizer_xbox_hardware_bitmaps` | 18E / 1R / 0U |
| `units` | 189E / 0R / 0U |

## Remaining unwritten scope

The intentionally unclaimed functions are the two public dispatchers
`bitmap_smooth` and `bitmap_sharpen` plus the six private 2D/3D/cube-map
smooth and sharpen helpers.  HaloCEA is useful semantic evidence for a later
lane, but those bodies were not mixed into this selective exact salvage.

The gating replacements used to compare newly semantic target names against
the pre-regeneration canonical split are recorded only in the untracked
scratch file `bitmap_reconcile_gate_edits.json`; it is not source, evidence,
or a commit candidate.
