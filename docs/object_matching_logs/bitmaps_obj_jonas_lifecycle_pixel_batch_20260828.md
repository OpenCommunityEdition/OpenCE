# `bitmaps.obj` lifecycle and pixel-count batch

## Result

Four public functions are strict semantic-COFF exact. `bitmaps.obj` advances
from 4/34 to 8/34 exact functions and remains honestly `NonMatching` with 26
functions still unwritten.

| Function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_bitmap_changed` | 55 / 64 | 5 | `f9540aa13efba679fe14f750fe1b8530769341bc276b6bf8450af7552f3e58ae` |
| `_bitmap_delete` | 75 / 80 | 5 | `10e91546b9d317d4fafb8afa37ecf9aeedc2192eb09518e7d80304ab1bd2f121` |
| `_bitmap_get_pixel_count` | 91 / 96 | 6 | `bb6686f820b7e9e55e6234484f9b27cba274e53999f14099d43de32a3ba2b560` |
| `_bitmap_get_pixel_data_size` | 97 / 112 | 7 | `f21a9814244522aaa0dea9c45ed5e74bca43e3036231a202ab01b36b150bee40` |

Direct `tools/coff_compare.py` comparison reports `all_equal: true` for all
four sections. The whole-unit gate reports eight exact, zero residual, and 26
unwritten functions, proving that all four inherited exact functions remain
exact. This wave adds 318 meaningful and 352 strict padded code bytes.

## Typed behavior and ABI

`_bitmap_changed` asserts its typed pointer and forwards it to
`_rasterizer_bitmap_changed`. `_bitmap_delete` accepts null, releases the
rasterizer resource, and frees the typed `base_address` and bitmap only when
the allocated flag is set. Both debug frees retain January's authenticated
source path and line values.

`_bitmap_get_pixel_count` verifies the bitmap and sums
`bitmap_mipmap_get_pixel_count` from mip zero through the stored count.
`bitmap_data.mipmap_count` remains the repository's established unsigned
field; the loop compares against an explicit `short` view because January's
instruction stream performs signed 16-bit bounds checks. That narrow cast
reproduces both loop comparisons without altering the shared structure.

`_bitmap_get_pixel_data_size` verifies the bitmap, obtains the total pixel
count, multiplies by the format's bits per pixel, and divides by eight. VC7
naturally emits January's signed division-by-power-of-two correction sequence.
The still-unwritten `_bitmap_format_get_bits_per_pixel` and
`_bitmap_mipmap_get_pixel_count` definitions are not claimed here; the exact
call relocations preserve their public ABI boundaries.

Stian Eklund's independently readable retail-Xbox reconstruction at commit
`41c10616b69b982700e0913f21a5137807a03d0e` corroborates the lifecycle
behavior and authenticated assertion/free line values. It was used as a
semantic pointer only; the retained implementation uses this repository's
typed `bitmap_data` fields and January's split-object instruction and
relocation evidence.

January object SHA-256 is
`6240548bf1ee72e09f15d1b48284092b17b6c096c11e8d294bbf71ae266acb57`;
the retained rebuilt raw object SHA-256 is
`8079df62033b03d6f9242b8db0b674c753114711bb3a7825f6b39151e3410546`.

## Validation and policy

- Complete `halobetacache_build`, `libcmt_build`, and `all_source`: pass.
- Progress: 574,237 / 2,198,102 overall and 561,323 / 1,770,166 Halo code
  bytes; 4,579 / 11,060 overall and 4,412 / 7,574 Halo functions.
- Semantic audit: 470 units, 4,728 functions evaluated, 4,616 accepted, zero
  unit errors.
- Admission: zero candidates, contradictions, and revocations.
- Parks: 13 active, zero stale, zero invalid.
- Strict board: 274/619 objects, 628,247 / 1,922,669 bytes, and 4,587 / 8,246
  functions after excluding parked and assembly-implemented credit.
- Tooling tests: 205/205 pass; `git diff --check` passes.

The retained implementations are readable typed C with every parameter on its
own line and explicit terminal returns. They introduce no assembly,
attribute, pragma, intrinsic, volatile/register forcing, raw address or byte
offset, undefined behavior, compiler option, object patch, semantic
exception, or comparator exception. `matrix_math.obj`, Vehicles, Units, and
all user-protected AI and game-engine sources are untouched.
