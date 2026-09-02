# `bitmap_utilities.obj` smooth/sharpen completion

## Result

This lane completed the eight previously unwritten smooth/sharpen functions in
`source/bitmaps/bitmap_utilities.c`. The focused gate moved from
`37 exact / 3 residual / 8 unwritten` to
`41 exact / 7 residual / 0 unwritten` without reopening any inherited exact
function.

Four functions are ordinary strict exact:

| Function | Padded target bytes |
| --- | ---: |
| `bitmap_smooth` | 352 |
| `bitmap_cm_smooth` | 176 |
| `bitmap_3d_sharpen` | 208 |
| `bitmap_cm_sharpen` | 208 |

This is 944 new strict padded code bytes. `bitmap_sharpen` has identical code
bytes, size, and relocation count and is accepted through the fail-closed
`csplit-relocation-alias` class described below. The three substantive image
filters are complete, coherent C and are honestly fuzzy-parked. The object now
has no unwritten function.

## Sources and reconstruction boundary

January's split object is authoritative for control flow, assertions, source
line immediates, diagnostics, allocation behavior, x86 parameter order,
relocations, and table ownership. The later HaloCEA implementations under
`src/blam/bitmaps/bitmap_{2d,3d,cm}_{smooth,sharpen}.c` and the two public
dispatcher files independently establish the convolution and unsharp-mask
semantics. The existing campaign ledger records that open Marathon contains no
direct analogue for this family.

HaloCEA was used as semantic evidence rather than copied blindly. January adds
the validation assertions HaloCEA omits, calls
`error(_error_silent, "### ERROR failed to allocate temporary bitmap")` when a
scratch allocation fails, and passes `"\r\n"` as an unused `fprintf` vararg in
the warning paths. Those shipped behaviors are preserved. HaloCEA's PPC
`bitmap_2d_sharpen` float-last ABI note was rejected: January x86 proves the
same `(bitmap, real sharpen_amount, positive_table, negative_table)` source
order used by the other two sharpen helpers.

The retained algorithms are ordinary typed C:

- `bitmap_2d_smooth` performs wrapped horizontal and vertical convolution;
- `bitmap_3d_smooth` performs wrapped X, Y, and Z convolution and the
  target-proven final full-buffer copy;
- `bitmap_2d_sharpen` applies the positive center lookup and eight negative
  neighbor lookups to every ARGB byte, with `PIN` for the 0..255 clamp;
- the public functions build the ten-word binomial kernel or the two signed
  256-word sharpen tables and dispatch with semantic bitmap enum constants;
- 3D/cube sharpening and cube smoothing retain their target-proven warning
  behavior.

No helper was invented to alter code generation. No forced inline/noinline,
barrier, volatile or register forcing, pragma, assembly, fake dependency,
undefined behavior, raw-address overlay, or nonsensical exactness branch is
present. Project types and cseries macros are used throughout, every function
has an explicit return, and the public declarations are owned by the narrow
`bitmap_utilities.h` header.

## Corrected BSS ownership

Completing both construction and consumption exposed a semantic reversal in
the earlier provisional names. January proves this layout:

| Address | Name | Size |
| --- | --- | ---: |
| `0031BF00` | `bitmap_sharpen_negative_table[256]` | `0x200` |
| `0031C100` | `bitmap_sharpen_positive_table[256]` | `0x200` |
| `0031C300` | `bitmap_smooth_filter_coefficients[10]` | `0x14` |

The base table stores `value * strength / 8 / falloff` and supplies all eight
neighbor deductions. The `+0x200` table stores `100 * value / falloff` and
supplies the center term. `config/symbols.json`, the source definitions, the
source symbol map comment, and the earlier reconciliation ledger were corrected
together. The old provisional mapping is superseded.

`bitmap_sharpen` is machine-code exact but ordinary COFF comparison sees two
producer spelling differences in its strength-reduced table loop:

- target `bitmap_sharpen_negative_table+510` versus candidate
  `bitmap_sharpen_positive_table-2`, both resolving to `3260670`;
- target `_bss_0031bed4+42` versus candidate
  `bitmap_sharpen_negative_table-2`, both resolving to `3260158`.

The fail-closed resolved-address scan proves every differing relocation lands
at the same image address. Direct semantic indexing is retained instead of an
out-of-bounds alias designed to imitate csplit's nearest-preceding-symbol
spelling.

## Credible fuzzy boundary

| Function | Target | Candidate | Relocations | Objdiff |
| --- | ---: | ---: | ---: | ---: |
| `bitmap_2d_smooth` | 1,136 | 1,120 | 27 / 27 | 79.08777% |
| `bitmap_3d_smooth` | 1,824 | 1,792 | 28 / 28 | 78.04% |
| `bitmap_2d_sharpen` | 1,152 | 1,184 | 29 / 29 | 60.921833% |

All three retain the complete target/HaloCEA-backed algorithm and relocation
topology. The fail-closed classifier returns `UNKNOWN`; the broad residuals are
C2 frame, register, and address-lifetime scheduling rather than isolated easy
instruction choices. Under the current house rule they are parked after the
natural reconstruction instead of accepting code-generation carriers.

Two target quirks remain intentionally visible: `MAXIMUM_FILTER_SIZE` is 10
despite the ten-word coefficient storage, and the 3D smoothing and 2D sharpen
paths copy `bitmap_get_pixel_data_size()` bytes after processing the top-level
layout. This ledger records the shipped behavior rather than silently fixing
it.

## Verification

- focused gate: `41 exact / 7 residual / 0 unwritten`;
- all four new park entries: active, with no stale or invalid evidence;
- `bitmap_sharpen`: identical normalized hash and resolved relocation
  destinations;
- `hud_nav_points.obj`: unchanged at `30 exact / 2 residual / 0 unwritten`;
- protected `units.obj`: `189 exact / 0 residual / 0 unwritten`;
- full build/report generation completed successfully.

The final canonical integration reruns the repository-wide semantic,
admission, fake-match, COMDAT, and test suites after combining the other
independently reviewed lanes.
