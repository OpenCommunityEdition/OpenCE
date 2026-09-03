# `bitmap_extract.obj` private-helper dependency audit (2026-09-03)

## Decision

Do not implement or rename the five unnamed private owners preceding
`extract_add_bitmap` yet. January proves their behavior and private register
interfaces, but the available authenticated naming sources do not expose their
original names. Assigning attractive descriptive guesses would violate the
campaign requirement that private functions be named correctly rather than by
address or invention.

No production source or symbol configuration changes are retained by this
audit. The separately committed `extract_3d_textures` reconstruction remains
valid and independently measurable.

## January dependency map

| January owner | Padded bytes | Behavior established by calls/assertions | Dependency on the add path |
| --- | ---: | --- | --- |
| `0x00062640` | 576 | Verifies a bitmap, walks its first mip's pixels, and classifies pixel/alpha content. | Returns the format-like value consumed by `extract_add_bitmap`. |
| `0x00062880` | 944 | Validates source/destination bitmap dimensions and mip levels, calls `bitmap_compress_to_mipmap`, and contains the palettized closest-color path. | Called by the final processing owner below. |
| `0x00062C30` | 528 | Validates scaled dimensions/type/mipmap bounds, then either calls `bitmap_uncompress_from_mipmap` or converts every pixel through `bitmap_format_to_a8r8g8b8`. | Mipmap conversion/copy primitive used by the processing family. |
| `0x00062E40` | 608 | Builds and resizes texture pages through the authenticated texture-page API and reports page dimensions/utilization. | Page-allocation path associated with extracted bitmaps. |
| `0x00063620` | 976 | Applies shrink, sharpen, fade, alpha bleed, height-map/vector-map transforms, quantization/compression, and optional debug-plate generation. | Direct post-clone callee of `extract_add_bitmap`; also calls the `0x00062880` owner. |

The target makes the ABI dependency concrete. `extract_add_bitmap` receives its
bitmap in `EAX`; the `0x00062640` call consumes that same private argument
shape, while the post-clone `0x00063620` owner consumes two compiler-selected
register arguments. `extract_3d_textures` likewise calls the still-unwritten
private `extract_add_bitmap` through `EAX`. An unresolved ordinary declaration
necessarily emits stack arguments and cleanup, so a wrapper or calling-
convention annotation would be compiler steering rather than recovered source.

## Naming evidence searched

- January `bitmap_extract.obj`, its complete COFF symbol table, relocations,
  assertion strings, and all five function bodies: only address labels exist.
- Every reachable local Git ref and worktree: no source definition or semantic
  rename exists for these five owners.
- `punpckhdq/halo`: its `bitmap_extract.c` contains only the original address
  inventory and no implementations.
- Aerocatia's `halopc-restored` archive: restored game/tag assets, not the
  missing tool translation unit or symbols.
- Both local HCEX/HaloCEA PDBs queried through DIA2Dump: no
  `bitmap_extract.obj` compiland or `extract_add_bitmap`/helper symbols. This is
  consistent with the later runtime build excluding the editor extraction TU.
- The Stian public repository contains no `bitmap_extract.c` or helper symbol.

These sources corroborate bitmap layouts and utility behavior but cannot
authenticate the private names. Marathon may inform style, not Halo-private
symbol identity.

## Reopen condition

Resume implementation when one of the following becomes available:

1. an authenticated Halo tool/Sapien PDB or map containing this compiland;
2. original or independently recovered PC/Xbox `bitmap_extract.c` with private
   names;
3. a provenance-backed symbol list mapping the five January addresses.

At that point reconstruct smallest-first (`0x00062C30`, `0x00062640`,
`0x00062E40`, `0x00062880`, `0x00063620`), gate each body independently, and
only then define `extract_add_bitmap` so VC7 can recover the genuine private
register ABI naturally.
