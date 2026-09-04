# `rasterizer_swizzle.obj` Opus reconciliation (2026-09-03)

## Result

The current canonical skeleton emitted none of January's 13 functions. Selective reconciliation of the committed and interrupted Opus work now emits every function and reaches 12/13 strict exact, gaining 12 functions and 3,328 strict padded bytes with zero whole-tree regressions. The sole residual, `_rasterizer_xbox_bitmap_swizzle`, is an honest 97.91958% scheduler fixed point at the exact 800-byte and 47-relocation envelope.

The January target object is derived from `cachebeta.exe` (SHA-256 `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`) with XDK 3911 VC7. The independent HaloCEA bitmap-swizzle sources corroborate every retained algorithm and data table; January disassembly, ordered relocations, strings, line records, and exact compiler output remain authoritative.

## Semantic ownership

The former address labels were resolved rather than retained:

- `_code_00171e60` is the private `compute_swizzle_masks` helper.
- `_rdata_0029ed10` is split into the private 64-word `swizzle_table` at +0 and six-short `face_mapping_inverse_table` at +128.
- `_bss_004b82b0` is split into the private `global_swizzle_z_mask`, `global_swizzle_y_mask`, and `global_swizzle_x_mask` scalars at +0/+4/+8.

Three separate mask scalars are required by January's emitted relocations and instruction shapes; an array spelling causes VC7 to common the base, dropping relocations and code bytes. The table contents, every consumer relocation, and the cube-face mapping independently authenticate both read-only owners.

Public declarations now live in `rasterizer_swizzle.h`. BITMAPS.C-owned mipmap declarations live in the narrow `bitmaps_mipmap.h`, which is included only by the two rasterizer consumers. The first attempt added missing declarations to the broadly included `bitmaps_internal.h`; a full-tree sweep caught a one-function compiler-allocation regression in `bitmap_utilities.obj`, so that broad edit was removed. The narrow owner header restores the existing exact function while preserving correct declaration ownership. Consumer-local duplicate prototypes were removed from `rasterizer_xbox_hardware_bitmaps.c`.

## Exact functions

- `compute_swizzle_masks`: 128 padded bytes
- `bitmap_swizzle_vector2d`: 304
- `bitmap_swizzle_vector3d`: 128
- `rasterizer_xbox_bitmap_swizzle2d_byte`: 208
- `rasterizer_xbox_bitmap_swizzle2d_word`: 224
- `rasterizer_xbox_bitmap_swizzle2d_long`: 208
- `rasterizer_xbox_bitmap_swizzle3d_byte`: 272
- `rasterizer_xbox_bitmap_swizzle3d_word`: 288
- `rasterizer_xbox_bitmap_swizzle3d_long`: 272
- `rasterizer_xbox_bitmap_get_max_mipmap_count`: 368
- `rasterizer_xbox_bitmap_get_pixel_data_size`: 256
- `rasterizer_xbox_bitmap_rebuild_hardware_format`: 672

The late exact lever for `bitmap_swizzle_vector2d` is the natural shared store site after the mutually exclusive x/y high-bit arms. The late exact lever for `rasterizer_xbox_bitmap_get_pixel_data_size` is a block-scoped `padding` local assigned after the two assertions; it naturally restores January's stack home and register allocation without fake dependencies.

## Parked residual

`rasterizer_xbox_bitmap_swizzle` has identical size and all 47 relocation identities. The remaining eight instruction slots are confined to the cube-map prologue: January homes the widened `bytes_per_pixel` value before division by six, while VC7 keeps it live in another register and homes it after the division. Declaration-order, initialization, constant-spelling, and call-order probes either returned to this fixed point or degraded it. The complete mipmap traversal, allocation/error flow, 2D/3D/cube dispatch, copies, free, flag update, and assertions are retained. No steering construct is present, so the function is parked as `instruction-scheduling` until authoritative source or compatible-compiler evidence appears.

## Validation

- Unit gate: 12 exact, 1 residual, 0 unwritten.
- Rename-stable whole-tree comparison against `scratch/after-combined-owner-fix-20260903.json`: +12 exact / +3,328 padded bytes, zero regressions.
- Full `ninja all_source progress semantic_progress`: pass, 473 semantic units, zero unit errors.
- Fake-match scanner on the reconstructed source: zero review leads.
- The protected user bitmap work was overlaid in the isolated validation tree; `bitmap_utilities.obj` remains 41 exact / 7 residual after the narrow-header repair.
- No `point_from_line3d` COMDAT is emitted by this packet.
