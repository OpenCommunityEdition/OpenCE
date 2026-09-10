# `bitmaps.obj` natural reconstruction packet (2026-09-09)

## Result

This packet raises `source/bitmaps/bitmaps.obj` from 18 to 30 exact
functions after the semantic target resplit. Ten closures come from newly
reconstructed bodies:

- `_bitmap_3d_slice_extract` (640 padded bytes)
- `_bitmap_3d_slice_insert` (640)
- `_bitmap_cube_map_face_extract` (640)
- `_bitmap_cube_map_face_insert` (640)
- `_bitmap_mipmap_get_width` (160)
- `_bitmap_mipmap_get_height` (160)
- `_bitmap_mipmap_get_depth` (144)
- `_bitmap_mipmap_get_pixel_count` (176)
- `_bitmap_mipmap_get_pixel_data_size` (144)
- `_bitmap_verify` (352)

The gross padded closure is 3,696 bytes. Meaningful-byte credit must come
from the canonical stable sweep rather than this padded total.

Two additional 32-byte private predicates were already implemented naturally
but hidden behind address-derived target labels. January call sites and the
three-function width/height/depth family identify them as
`bitmap_format_type_valid_width` and `bitmap_format_type_valid_height`; the
symbol manifest now records those semantic static names. This is symbol-owner
repair, not a semantic allowlist.

## Evidence and reconstruction policy

The implementations are ordinary source reconstructions corroborated by the
January target disassembly, the locally cached HCEA reconstruction, and the
historical branch-sweep candidate at `f9dd42218`. The public names and
signatures already have project-header owners. The bodies use typed bitmap
fields, enum constants, project integer/boolean types, the established
assertion helpers, and the existing address/copy routines. No opaque storage,
address-named source identifiers, volatile ordering, or fake-match padding was
introduced.

`bitmap_verify` intentionally uses
`(bitmap->flags&~(FLAG(NUMBER_OF_BITMAP_FLAGS)-1))==0`: the more general
`VALID_FLAGS` helper was tested first, but VC7 emitted a byte-sized test while
the January function performs the word load and full invalid-bit mask. This is
the direct flag invariant, not a code-generation decoy.

## Ownership decision

An initial 128-byte `_bitmap_rebuild` closure required declarations for the
Xbox rasterizer bitmap lifecycle. It was removed from this packet. The prior
rasterizer-owner audit rejected an invented narrow header and documented a
dependent-object regression from moving those declarations into the genuine
shared owner header. The function therefore remains unwritten and receives no
credit until that ownership packet can be admitted without collateral loss.

Three complete, ordinary reconstructions are retained as credible residuals
at zero exact credit:

- `_bitmap_format_to_a8r8g8b8`: 560 candidate padded bytes versus 544 target;
- `_palette_find_closest_match`: 272 candidate versus 256 target; and
- `_bitmap_2d_get_pixel`: 1,200 candidate versus 1,296 target.

They remain useful semantic source, but their instruction selection and/or
layout differ and no matching credit is assigned. `_bitmap_rebuild` is the
sole remaining unwritten target owner and likewise receives no credit.

## Verification

- The pre-resplit focused gate reports `28 exact, 3 residual, 3 unwritten`;
  the canonical semantic resplit is expected to convert the two named private
  predicates from name gaps to strict exact owners, for `30 exact, 3 residual,
  1 unwritten`.
- `python tools/campaign/gate.py
  source/rasterizer/xbox/rasterizer_xbox_hardware_bitmaps` remains
  `18 exact, 1 residual, 0 unwritten`; the temporary header experiment left
  no change in that translation unit.
- `python tools/fake_match_scan.py source/bitmaps/bitmaps.c
  source/rasterizer/xbox/rasterizer_xbox_hardware_bitmaps.c` reports zero
  review leads.
- `python -m tools.test_bitmaps_runtime_differential` passes.
- The final canonical full build, stable sweep, dependent sweep, and complete
  test suite remain the admission authority for this packet.
