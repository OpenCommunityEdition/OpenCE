# `bitmap_extract.obj` opus5 100k wave C park re-investigation (2026-09-14)

## Scope and provenance

- Translation unit `source/bitmaps/bitmap_extract.c`, target `build/split/source/bitmaps/bitmap_extract.obj`
  (January 2002 Xbox debug build, VC7 13.00.9254, repository `build.ninja` flags).
- Evidence: January target COFF (instructions, relocations, `.rdata` literals), January PDB public symbols
  (`scratch/pdb200-current/pdb_symbols.json`), HCEA headers `bitmap_data.h`, `bitmap_group.h` and
  `bitmap_compress_to_mipmap.c` (names/types only). No atlas or Ghidra output exists for this object.
- All 15 residual functions are PARKED; every experiment below was made in scratch copies. The real source file is
  unchanged (`git diff --stat` empty). Worker notes: `scratch/workersC/bitmap_extract.md`.

## Validated baseline (lane HEAD)

`gate.py --all --forbid-emitted-symbol _point_from_line3d`: exact 6, residual 15, unwritten 0. Exact:
`_bitmaps_extract_from_plate`, `_extract_find_bitmap_bounds`, `_extract_find_sequence_bounds`, `_extract_initialize`,
`_extract_sequences`, `_extract_without_sequences`. Residual sizes/relocations/hashes are the `measurements.base`
records in `config/parked.json`.

## Results (scratch, strict `section_infos_equal`)

### A. Park-reopen set on HEAD headers - `scratch/workersC/bitmap_extract/park_reopen_head.c`

Gate: 12 exact / 9 residual; point guard passed; the other 9 parks equal their base measurements; owner census clean
(only literal COMDAT swaps to January's own strings); fake scan 0.

| Function | Padded | Key evidence |
| --- | ---: | --- |
| `_bitmaps_extract` | 736 | assert text `decompressed_plate_size==sizeof(pixel32)*...`; single success flag: alloc failure reports and clears it, the resize block runs when `!result` (January's double error), switch gated on `result` after the if/else. |
| `_extract_bitmap` | 1168 | single exit (success paths load `[ebp-1]`, failures return FALSE, 2d-NONE path cross-jumps into the plateless tail); alpha as `(color >> 24)` expression (EDI across the warning call, un-threaded retest); `warned_about_dxt1_alpha` declared first; strength-reduced `source_y - adjusted_bounds.y0` / `destination[source_x - adjusted_bounds.x0]`. |
| `_extract_sequence` | 720 | missing reloc = `found_bottom_reference` flag; switch state 0 falls into 1; target-proven redundant `(found_bottom_reference || single_sequence) && !found_bitmap` state test; working rectangle copied before trimming; field stores x0,y0,x1,y1; `x = 0` after asserts; `while (x < width && state != 2)`; asserts `top>=0`, `bottom>=top` (base had `>`), `bottom<=...`. |
| `_extract_cube_maps` | 544 | literal "failed to create temporary bitmap"; short index; face==6 block outside the temporary block; skip block frees without clearing the pointer (BUG (original), commented); nested equality form. |
| `_extract_3d_textures` | 464 | `short first_bitmap_index` (16-bit compare); `bitmap_count` declared before `incompatible_dimensions`. |
| `_extract_warn_about_horizontal_border` | 128 | compare the masked pixel directly (a named `color` local reserves EAX at the break edge). |

### B. Header packet set - `scratch/workersC/bitmap_extract/best.c` (packet copy `packet/source/...`)

Gate (headers via the packet copies): 17 exact / 4 residual; point guard passed; remaining 4 parks equal base; fake scan 0.

| Function | Padded | Key evidence |
| --- | ---: | --- |
| `_extract_add_bitmap` | 688 | prototyped short pushes; one 32-bit registration-point copy (`point2d registration_point`, HCEA); signed group `mipmap_count`; `MIN(group->mipmap_count - 1, mipmap_count)`; 3D `movsx` depth. |
| `_extract_build_debug_plate` | 1408 | PDB public symbol => non-static stack ABI; `_error_silent` messages mapped to the right failures; second half on `converted_bitmap`; per-case `csmemcpy`; `(dim >> mip) + (border ? 3/4 : 0)` increments; alpha test after the outer if/else; needs the `bitmap_cube_map_address` prototype (C4013 fix). |
| `_extract_mipmaps_to_bitmap` | 976 | literal texts; switch default NULL; named `real mipmap_count` and `real detail_fade` in the detail-map block. |
| `_extract_pixels_from_mipmap` | 528 | literal texts; verifies destination fully first, source loosely; signed fields remove the casts. |
| `_extract_pixels_to_mipmap` | 944 | missing reloc = `extract_sequences ? &extract_data.adjusted_bounds_reference : NULL` (HCEA `transparent_color`); case order from the jump table; `(channel >> bits) << position` packed formats. |

Packet (orchestrator): `bitmap_group.h` (`short depth`, `union point2d registration_point`, `short mipmap_count` in
`bitmap_data`; `short mipmap_count` in `bitmap_group`), `bitmaps_mipmap.h` (+`bitmap_get_max_mipmap_count`),
`bitmap_group_internal.h` (+`struct bitmap_group;`, +`bitmap_group_add_bitmap`), `bitmap_utilities.h/.c`
(`pixel32 const *transparent_color`), registration-point respelling in `bitmap_group.c`, `hud_draw.c`, `interface.c`,
`overhead_map.c`. Blast radius over all 45+4+5+1 consumers (shadow-header compile, `blast.py`): lost 0, gained 0; one
parked body changes (`bitmap_group::_bitmap_group_add_bitmap` 992/59 sha 0bd88e42 -> 2e0bbd84, point2d) and needs a
park re-baseline. Putting the add-bitmap prototype in `bitmap_group.h` instead loses
`decals::_decals_delete_permanent_from_cluster` (declaration-count law); putting `bitmap_get_max_mipmap_count` in
`bitmaps_internal.h` drifts `_extract_plateless_cube_map`.

## Not landed (best scratch bodies)

| Function | Best | Residual classification |
| --- | --- | --- |
| `_extract_get_bitmap_format` | `gbf_a.c` 576/20 sha | opposite-extreme 1-bit test and 32-bit channels proven; remaining join of the two flag tests + callee-saved colouring. |
| `_extract_plateless_cube_map` | `pl_a.c` 832/19 sha | real defect found: base transposes x/y block/edge members; remaining face IV base (`&faces[i].source_y_block`) and ESI/EDI colouring. |
| `_extract_build_texture_pages_by_sequence` | `pages_b.c` 608/22 sha | stack ABI, short spanned count, `>> 1`, MAX/MIN locals proven; one block-layout difference (new-page branch). |
| `_extract_sprites` | `sp_e.c` 1104/49 sha | budget `== 0.0f`, declaration order, no early return proven; register allocation residual. |

## Do-not-repeat

- `_bitmaps_extract`: pointer re-test after the alloc error (global reload), switch inside the plate nest, e2/e3 result forms.
- `_extract_bitmap`: named `alpha` local, `alpha == 0`, pointer-increment destination spellings, compare operand swap.
- `_extract_build_debug_plate`: static linkage (EAX ABI), alpha test inside the converted block on HEAD headers.
- `_extract_mipmaps_to_bitmap` fade: lerp/reordered sums, explicit cast, single named fade.
- `_extract_pixels_to_mipmap` a4r4g4b4: `(pixel >> 28) << 12`, `& 0xF0` channel forms.
- `_extract_get_bitmap_format`: inline channel expressions, `(first & M) == (alpha ? 0 : M)`, `alpha ? !... : ...`.
- `_extract_sequence`: MIN(bounds.y0, y); inner condition swap with `x = 0` at the declaration.
- `_extract_warn_about_horizontal_border`: warning after the loop.

## Reopen criteria / disposition

- Sets A and B are proposed to the orchestrator as park-reopens (A needs no header change; B needs the packet and two
  park re-baselines). No exact credit is claimed in this wave: the real file was not modified.
- The four remaining parks: reopen only with a spelling that explains the recorded layout/colouring residual.
