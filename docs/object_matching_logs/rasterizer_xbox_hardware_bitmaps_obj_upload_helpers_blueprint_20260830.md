# `rasterizer_xbox_hardware_bitmaps.obj` upload-helper blueprint (2026-08-30)

## Scope, authority, and disposition

This packet is limited to the three private upload helpers in
`source/rasterizer/xbox/rasterizer_xbox_hardware_bitmaps.c`:

- `_code_00157d50`, the two-dimensional texture uploader;
- `_code_00157f10`, the volume-texture uploader; and
- `_code_00158120`, the cube-texture uploader.

The packet starts from local commit
`8e91e7a66c9857e44ee71da7a3d6093a074b8674` on isolated branch
`jonas/ima-adpcm-fresh-packet-20260830`. The immutable January split object is
`build/split/source/rasterizer/xbox/rasterizer_xbox_hardware_bitmaps.obj`, raw
SHA-256 `ac2ed2f8928478c833b5f3431c3668fcde7da1f66080fc982f861532240d9968`.
Strict authority is the hardened COFF comparator, including relocation types,
destinations, addends, and owned data; raw object hashes are only local
sentinels. Production compilation remains the repository's pinned XDK 3911
VC7 edge and flags.

This is deliberately a research-only packet. The root campaign already had a
dirty, active hardware-bitmaps implementation lane while this census was in
progress. Editing the same translation unit here would have created an
avoidable merge conflict and would have made whole-TU ownership harder to
review. No source candidate was compiled, no production source was changed,
and no exact-match credit is claimed. The evidence below is sufficiently
specific for the active lane to compile one coherent three-helper packet
without repeating the disassembly, relocation, XDK, and HCEA census.

## Validated baseline

The isolated baseline is `7 / 19` strict-exact functions and `144 / 2448`
padded code bytes. The exact owners are the six already-admitted SDK callback
stubs plus `_rasterizer_bitmap_delete`. All three upload helpers are absent,
so their first baseline divergence is the missing section rather than an
instruction mismatch.

| Function | Meaningful / padded target | Target relocations | Target normalized SHA-256 | Baseline |
| --- | ---: | ---: | --- | --- |
| `_code_00157d50` | `437 / 448` | 25 | `dc4629f49cc5c2d5536f5ffe3e673716ed3852634bb779bb882ef3438ed7e10a` | unwritten |
| `_code_00157f10` | `514 / 528` | 26 | `fcec7c3ebfa9e48831f30974ede150d8bd7b7f676dc55fb5b3920ec016af49ab` | unwritten |
| `_code_00158120` | `517 / 528` | 26 | `d5c999a26a985f4f7728ebd084be49b4df6eb469e6bd98d45ab2b4454c64386e` | unwritten |

Together the helpers account for 1,504 padded bytes. Their meaningful ends
are the `ret` at target offsets `+0x1b4`, `+0x201`, and `+0x204`; the remaining
bytes are ordinary section padding.

The target also owns one combined 84-byte const-data section,
`_rdata_00290958`, with zero relocations and normalized SHA-256
`4303e8a36fe876148b5717d0229e95c72364de0d74bda7fb48cd44be57852f0b`.
That data is absent from this baseline. The active root lane had already begun
recovering its first 72 bytes; the final 12-byte face table described below
must remain part of the same contiguous owner.

## Provenance and artifact census

The following evidence was exhausted before considering source:

- reachable history for
  `source/rasterizer/xbox/rasterizer_xbox_hardware_bitmaps.c`, including the
  already-admitted callback/delete commits, contains no body for any upload
  helper;
- prior clean worktrees `work/lanes/hardware-bitmaps-upload-cluster` at
  `bcbd7f...` and `work/lanes/hardware-bitmaps-changed` at `d306...` contain no
  additional helper body or compiled helper section;
- surviving Claude/root scratch copies contain no full helper body;
- Stian's authenticated complete-history checkout at
  `research-cache/stian-halo-complete-history-20260828/src/halo/rasterizer/xbox/rasterizer_xbox_hardware_bitmaps.c`
  has the callback/delete work but no uploader donor;
- Stian's authenticated
  `research-cache/stian-halo-complete-history-20260828/src/halo/rasterizer/rasterizer_text.c`
  independently establishes the byte/word/long 2D and 3D swizzle signatures
  and pixel-width dispatch;
- HCEA's authenticated
  `research/halocea/src/rasterizer_bitmap_changed.c` establishes the exact
  2D/3D/cube dispatch and the device/base-address/hardware-format guard;
- HCEA's `research/halocea/src/bitmap_mipmap_address.c`,
  `bitmap_mipmap_get_width.c`, `bitmap_mipmap_get_height.c`,
  `bitmap_mipmap_get_depth.c`, and `bitmap_mipmap_get_pixel_data_size.c`
  establish the helper semantics and narrow dimension types;
- HCEA's `research/halocea/src/headers/bitmap_flags.h` establishes bitmap type
  values and compressed flag bit 1;
- HCEA's `research/halocea/src/hcex/hcex_tex_2d_copytohw.cpp`,
  `hcex_tex_3d_copytohw.cpp`, and `hcex_tex_cube_copytohw.cpp` independently
  corroborate linear/compressed copying, volume slice-pitch stepping, and
  cube-face upload behavior; and
- HCEA's `research/halocea/src/headers/hcex/hcex_texture_upload.h` explicitly
  declares `short face_mapping_table[6]`.

The pinned XDK evidence is local and exact. `xbox/include/D3D8.h` declares the
three stock `D3DINLINE` wrappers at lines 2120, 2168, and 2215. Normal calls
emit the target external lock relocations. Their corresponding unlock wrappers
call empty force-inlined bodies at lines 2102, 2150, and 2199, so writing the
natural unlock calls adds no target relocation or machine operation. Do not
replace these APIs with hand-written imports or address-taking wrappers.

No authenticated source donor preserves the original C spelling. The target
disassembly plus relocation sequence, XDK inline bodies, and HCEA behavior are
the positive provenance for reconstruction.

## Exact shared data ownership

`_rdata_00290958` is exactly 84 bytes in this order:

1. 18 four-byte D3D format values (72 bytes):
   `25, 0, 1, 26, -1, -1, 5, -1, 2, 4, 7, 6, -1, -1, 12, 14, 15, 11`;
2. six two-byte cube-face values (12 bytes): `0, 2, 1, 3, 4, 5`.

The format values naturally name:

`D3DFMT_A8`, `D3DFMT_L8`, `D3DFMT_AL8`, `D3DFMT_A8L8`,
`D3DFMT_UNKNOWN`, `D3DFMT_UNKNOWN`, `D3DFMT_R5G6B5`,
`D3DFMT_UNKNOWN`, `D3DFMT_A1R5G5B5`, `D3DFMT_A4R4G4B4`,
`D3DFMT_X8R8G8B8`, `D3DFMT_A8R8G8B8`, `D3DFMT_UNKNOWN`,
`D3DFMT_UNKNOWN`, `D3DFMT_DXT1`, `D3DFMT_DXT3`, `D3DFMT_DXT5`, and
`D3DFMT_P8`.

The six face entries naturally name positive X, positive Y, negative X,
negative Y, positive Z, and negative Z, in that order. They must be declared
as a **short** array, not as `D3DCUBEMAP_FACES[]`. The XDK enum occupies four
bytes, which would incorrectly expand the tail to 24 bytes. The target cube
helper uses a signed 16-bit table load and its relocation points to
`_rdata_00290958 + 72`; HCEA independently declares the same table as
`short[6]`. The evidence-backed declaration surface is therefore:

```c
static short const face_mapping_table[6] =
{
	D3DCUBEMAP_FACE_POSITIVE_X,
	D3DCUBEMAP_FACE_POSITIVE_Y,
	D3DCUBEMAP_FACE_NEGATIVE_X,
	D3DCUBEMAP_FACE_NEGATIVE_Y,
	D3DCUBEMAP_FACE_POSITIVE_Z,
	D3DCUBEMAP_FACE_NEGATIVE_Z,
};
```

Keep it immediately after the 18-entry format table and verify that the
compiler emits one 84-byte const owner, rather than two reordered or
independently named sections.

## Exact strings and assertion sites

All three helpers share these target-owned strings:

```text
c:\halo\SOURCE\rasterizer\xbox\rasterizer_xbox_hardware_bitmaps.c
bitmap
### ERROR failed to lock surface
### ERROR uncompressed bitmap format does not have 1,2 or 4 bytes per pixel
### ERROR failed to change bitmap hardware format
```

The lock-report expressions are exact source strings and must not be
reformatted:

```text
IDirect3DTexture8_LockRect((IDirect3DTexture8*)bitmap->hardware_format, mipmap_index, &d3d_locked_rect, NULL, D3DLOCK_NOOVERWRITE)
IDirect3DVolumeTexture8_LockBox((IDirect3DVolumeTexture8*)bitmap->hardware_format, mipmap_index, &d3d_locked_box, NULL, D3DLOCK_NOOVERWRITE)
IDirect3DCubeTexture8_LockRect((IDirect3DCubeTexture8*)bitmap->hardware_format, face_mapping_table[face_index], mipmap_index, &d3d_locked_rect, NULL, D3DLOCK_NOOVERWRITE)
```

The entry `bitmap` assertions use source lines `0x8d`, `0xcb`, and `0x114` for
2D, 3D, and cube respectively. The impossible bytes-per-pixel assertions use
lines `0xb1`, `0xf9`, and `0x13f`. Target code calls `display_assert`, followed
by `system_exit(-1)`, for those assertions. Lock failures call
`error(_error_silent, "### ERROR failed to lock surface")`; final failure
calls `error(_error_silent, "### ERROR failed to change bitmap hardware format")`.
The target enum value for `_error_silent` is 2.

## Relocation proofs

Relocation type 6 below is `IMAGE_REL_I386_DIR32`; type 20 is
`IMAGE_REL_I386_REL32`. Offsets are decimal section-relative byte offsets.

### `_code_00157d50` (2D)

| Offset | Type | Destination |
| ---: | ---: | --- |
| 21 | 6 | source file string |
| 26 | 6 | `"bitmap"` |
| 31 | 20 | `_display_assert` |
| 38 | 20 | `_system_exit` |
| 46 | 6 | `_global_d3d_device` |
| 113 | 20 | `_D3DTexture_LockRect@20` |
| 122 | 6 | exact 2D lock-expression string |
| 129 | 20 | `_rasterizer_error` |
| 137 | 6 | failed-lock string |
| 144 | 20 | `_error` |
| 176 | 20 | `_bitmap_mipmap_address` |
| 191 | 20 | `_bitmap_mipmap_get_width` |
| 204 | 20 | `_bitmap_mipmap_get_height` |
| 226 | 20 | `_bitmap_mipmap_get_pixel_data_size` |
| 234 | 20 | `_csmemcpy` |
| 256 | 20 | `_bitmap_format_get_bits_per_pixel` |
| 294 | 6 | source file string |
| 299 | 6 | bad-bytes-per-pixel string |
| 304 | 20 | `_display_assert` |
| 311 | 20 | `_system_exit` |
| 339 | 20 | `_rasterizer_xbox_bitmap_swizzle2d_long` |
| 367 | 20 | `_rasterizer_xbox_bitmap_swizzle2d_word` |
| 395 | 20 | `_rasterizer_xbox_bitmap_swizzle2d_byte` |
| 417 | 6 | failed-change string |
| 424 | 20 | `_error` |

### `_code_00157f10` (3D)

| Offset | Type | Destination |
| ---: | ---: | --- |
| 21 | 6 | source file string |
| 26 | 6 | `"bitmap"` |
| 31 | 20 | `_display_assert` |
| 38 | 20 | `_system_exit` |
| 46 | 6 | `_global_d3d_device` |
| 113 | 20 | `_D3DVolumeTexture_LockBox@20` |
| 122 | 6 | exact 3D lock-expression string |
| 129 | 20 | `_rasterizer_error` |
| 137 | 6 | failed-lock string |
| 144 | 20 | `_error` |
| 176 | 20 | `_bitmap_mipmap_address` |
| 191 | 20 | `_bitmap_mipmap_get_width` |
| 204 | 20 | `_bitmap_mipmap_get_height` |
| 217 | 20 | `_bitmap_mipmap_get_depth` |
| 262 | 20 | `_bitmap_mipmap_get_pixel_data_size` |
| 277 | 20 | `_csmemcpy` |
| 321 | 20 | `_bitmap_format_get_bits_per_pixel` |
| 359 | 6 | source file string |
| 364 | 6 | bad-bytes-per-pixel string |
| 369 | 20 | `_display_assert` |
| 376 | 20 | `_system_exit` |
| 408 | 20 | `_rasterizer_xbox_bitmap_swizzle3d_long` |
| 440 | 20 | `_rasterizer_xbox_bitmap_swizzle3d_word` |
| 472 | 20 | `_rasterizer_xbox_bitmap_swizzle3d_byte` |
| 494 | 6 | failed-change string |
| 501 | 20 | `_error` |

### `_code_00158120` (cube)

| Offset | Type | Destination |
| ---: | ---: | --- |
| 21 | 6 | source file string |
| 26 | 6 | `"bitmap"` |
| 31 | 20 | `_display_assert` |
| 38 | 20 | `_system_exit` |
| 46 | 6 | `_global_d3d_device` |
| 144 | 6 | `_rdata_00290958 + 72` (16-bit face table) |
| 155 | 20 | `_D3DCubeTexture_LockRect@24` |
| 164 | 6 | exact cube lock-expression string |
| 171 | 20 | `_rasterizer_error` |
| 179 | 6 | failed-lock string |
| 186 | 20 | `_error` |
| 237 | 20 | `_bitmap_cube_map_address` |
| 252 | 20 | `_bitmap_mipmap_get_width` |
| 265 | 20 | `_bitmap_mipmap_get_height` |
| 287 | 20 | `_bitmap_mipmap_get_pixel_data_size` |
| 311 | 20 | `_csmemcpy` |
| 336 | 20 | `_bitmap_format_get_bits_per_pixel` |
| 374 | 6 | source file string |
| 379 | 6 | bad-bytes-per-pixel string |
| 384 | 20 | `_display_assert` |
| 391 | 20 | `_system_exit` |
| 419 | 20 | `_rasterizer_xbox_bitmap_swizzle2d_long` |
| 447 | 20 | `_rasterizer_xbox_bitmap_swizzle2d_word` |
| 475 | 20 | `_rasterizer_xbox_bitmap_swizzle2d_byte` |
| 497 | 6 | failed-change string |
| 504 | 20 | `_error` |

This sequence is also a statement-order proof. In particular, each lock and
its report precede address/dimension queries; dimension queries precede the
compressed/uncompressed branch; and the byte-width query precedes the
long/word/byte dispatch.

## Common natural-C shape

The three functions are private, non-address-taken helpers. Preserve internal
linkage and let their already-proven caller determine the private register
ABI. The target caller holds the bitmap in `ESI` for 2D and cube. Immediately
before the 3D call it copies `ESI` to `EDI`; the target 3D helper consumes the
bitmap from `EDI`. Adding external linkage, a public prototype, or
address-taking would destroy that evidence-backed convention.

Each helper has this common operation graph:

1. assert `bitmap` at its exact line and initialize a `boolean success` to
   `TRUE`;
2. perform no work unless `global_d3d_device`, `bitmap->base_address`, and
   `bitmap->hardware_format` are all non-null, in that order;
3. iterate mip levels from 0 through `bitmap->mipmap_count` inclusive while
   `success` remains true;
4. call the stock XDK lock wrapper with `D3DLOCK_NOOVERWRITE`;
5. combine the prior success with `SUCCEEDED(lock_result)`, and if false call
   `rasterizer_error(0, exact_lock_expression)`;
6. combine success with the locked `pBits` pointer, and if false call
   `error(_error_silent, failed_lock_string)`;
7. derive the source pointer and dimensions only after the lock succeeded;
8. copy compressed data linearly, or dispatch uncompressed data by
   `bitmap_format_get_bits_per_pixel(bitmap->format) / 8` to case 4, 2, or 1;
9. assert false on any other bytes-per-pixel value at the helper's exact line;
10. invoke the natural XDK unlock wrapper after the upload. Its force-inlined
    body is empty and must add no relocation; and
11. after the loop, report the shared failed-change string if success is
    false.

The target's lock path is the same XDK HRESULT idiom authenticated elsewhere:
the wrapper call occurs first, then the loop-carried success byte in `BL` is
tested. The XDK wrapper returns the constant `S_OK`, so the HRESULT folds to
literal zero for `rasterizer_error` while the success dependency remains
visible. Do not simplify away the readable HRESULT/success expression merely
because the pinned inline wrapper cannot fail.

The target stack frames prove the natural lock structures:

- 2D: 20-byte frame containing an 8-byte `D3DLOCKED_RECT` and mip/width/height
  locals;
- 3D: 28-byte frame containing a 12-byte `D3DLOCKED_BOX` and
  mip/width/height/depth locals; and
- cube: 24-byte frame containing an 8-byte `D3DLOCKED_RECT` and
  mip/face/width/height locals.

## Helper-specific reconstruction

### Two-dimensional uploader

Use `bitmap_mipmap_address(bitmap, mipmap_index)`, then obtain width and height
with the two narrow getters. If compressed flag bit 1 is set, copy exactly
`bitmap_mipmap_get_pixel_data_size(bitmap, mipmap_index)` bytes from the mip
source to `d3d_locked_rect.pBits`. Otherwise compute signed bytes per pixel
from the short-returning format helper and dispatch:

```c
case 4:
	rasterizer_xbox_bitmap_swizzle2d_long(destination, source, width, height);
	break;
case 2:
	rasterizer_xbox_bitmap_swizzle2d_word(destination, source, width, height);
	break;
case 1:
	rasterizer_xbox_bitmap_swizzle2d_byte(destination, source, width, height);
	break;
```

The target keeps the bitmap in `ESI`, success in `BL`, source in `EDI`, and
locked destination in `EBX`. This is a measurement for comparison, not a
request for `register` or other source forcing.

### Volume uploader

After lock, query source, width, height, and depth, in that exact order. For a
compressed texture, copy one source slice at a time when signed depth is
positive:

```c
slice_size = bitmap_mipmap_get_pixel_data_size(bitmap, mipmap_index) / depth;
for (slice_index = 0; slice_index < depth; slice_index++)
{
	csmemcpy(destination, source, slice_size);
	source += slice_size;
	destination += d3d_locked_box.SlicePitch;
}
```

The target divides the total size by signed depth once per mip, advances the
linear source by that quotient, and advances destination by `SlicePitch`.
`RowPitch` is not used. If depth is non-positive it performs no slice copy and
continues successfully. For uncompressed data, dispatch case 4/2/1 to the
corresponding 3D swizzler with `(destination, source, width, height, depth)`.

The target keeps the bitmap in `EDI`, success in `BL`, source in `EBX`, and
locked destination in `ESI`. The compiler reuses dead dimension slots as the
signed divisor and unsigned slice countdown; express this as normal typed C,
not stack aliasing.

### Cube uploader

Nest six face uploads inside each mip level, with both loops stopping when
success becomes false. Lock
`face_mapping_table[face_index]`, not the raw face index. Obtain the source
with this exact argument shape:

```c
bitmap_cube_map_address(bitmap, 0, 0, face_index, mipmap_index)
```

Then query width and height. Compressed cube data copies
`bitmap_mipmap_get_pixel_data_size(bitmap, mipmap_index) / 6` bytes per face.
The target's signed divide-by-six strength reduction is visible as multiply by
`0x2aaaaaab`, which ordinary `/ NUMBER_OF_CUBE_MAP_FACES` produces under VC7.
Uncompressed faces use the same 2D case 4/2/1 swizzle dispatch as the 2D
helper.

The target keeps the bitmap in `ESI`, success in `BL`, face/source in `EDI`,
and locked destination in `EBX`. The target face lookup is a signed 16-bit
load from the combined const owner at offset 72.

## Required types and declarations

The active lane should source these from existing repository headers wherever
possible, rather than duplicating declarations. The target evidence requires:

- `<xtl.h>` or the repository's established XDK include surface;
- `cseries/errors.h` for `_error_silent`;
- `_bitmap_type_2d = 0`, `_bitmap_type_3d = 1`,
  `_bitmap_type_cube_map = 2` where the surrounding changed/new functions need
  them;
- `_bitmap_compressed_bit = 1`;
- `NUMBER_OF_CUBE_MAP_FACES = 6`;
- `extern D3DDevice *global_d3d_device`;
- `short` return types for width, height, depth, and bits per pixel;
- `long` return type for mip pixel-data size;
- a pointer return for both address helpers; and
- the authenticated swizzle parameter shapes shown below.

```c
void *bitmap_mipmap_address(
	struct bitmap_data *bitmap,
	short mipmap_index);
void *bitmap_cube_map_address(
	struct bitmap_data *bitmap,
	short x,
	short y,
	short face_index,
	short mipmap_index);
short bitmap_mipmap_get_width(
	struct bitmap_data *bitmap,
	short mipmap_index);
short bitmap_mipmap_get_height(
	struct bitmap_data *bitmap,
	short mipmap_index);
short bitmap_mipmap_get_depth(
	struct bitmap_data *bitmap,
	short mipmap_index);
long bitmap_mipmap_get_pixel_data_size(
	struct bitmap_data *bitmap,
	short mipmap_index);
short bitmap_format_get_bits_per_pixel(
	short format);
void rasterizer_xbox_bitmap_swizzle2d_byte(
	void *destination,
	void const *source,
	short width,
	short height);
void rasterizer_xbox_bitmap_swizzle2d_word(
	void *destination,
	void const *source,
	short width,
	short height);
void rasterizer_xbox_bitmap_swizzle2d_long(
	void *destination,
	void const *source,
	short width,
	short height);
void rasterizer_xbox_bitmap_swizzle3d_byte(
	void *destination,
	void const *source,
	short width,
	short height,
	short depth);
void rasterizer_xbox_bitmap_swizzle3d_word(
	void *destination,
	void const *source,
	short width,
	short height,
	short depth);
void rasterizer_xbox_bitmap_swizzle3d_long(
	void *destination,
	void const *source,
	short width,
	short height,
	short depth);
```

The existing `struct bitmap_data` layout proves offsets used by the target:
format at `+0x0c`, flags at `+0x0e`, mipmap count at `+0x14`, hardware format
at `+0x28`, and base address at `+0x2c`. Keep the structure typed; do not spell
those offsets in production C.

## Experiment matrix and admission plan

| ID | Evidence-backed source shape | Production compile | Result | Decision |
| --- | --- | --- | --- | --- |
| R01 | coherent 2D/3D/cube packet described above, stock XDK inline APIs, combined format/short-face data owner | not run | active root lane already edits the same TU; a second implementation would conflict | hand off as research-only |

The receiving lane should compile at most one fresh coherent packet first,
because the three helpers share strings, inline-wrapper instantiations, and
the 84-byte const-data owner. Compare all 19 target functions, every ordinary
code/data/COMDAT section, relocation destination/addend, and whole-TU symbol
ownership. Retain only individually strict-exact helpers and only if every
inherited exact owner remains exact. If any helper is rejected, restore its
source exactly; do not keep a semantic approximation in production.

The focused acceptance gates are:

1. pre-edit whole-TU regression snapshot;
2. XDK 3911 production build;
3. hardened comparator for the three helpers and all inherited exact owners;
4. exact `_rdata_00290958` size/hash/zero-relocation ownership check;
5. focused regression check with no changed previously exact owner;
6. `halobetacache_build`, `libcmt_build`, `progress`, and
   `semantic_progress`;
7. object admission and parked-function audits;
8. the complete pytest suite and fixed Units sentinel; and
9. `git diff --check`.

## Do-not-repeat boundary and reopen criteria

Do not:

- redeclare the face table as `D3DCUBEMAP_FACES[6]`; it is provably 16-bit;
- split, reorder, or pad the 84-byte shared const owner;
- call raw addresses or hand-declare alternative D3D imports;
- omit the natural unlock calls merely because XDK compiles them to no code;
- replace short dimension/format returns with `long` placeholders;
- flatten the 3D compressed upload into one contiguous copy; destination must
  advance by `SlicePitch` per slice;
- use raw cube face index instead of the mapping table;
- repeat a source candidate from this lane, because none was compiled here;
  or
- use assembly, volatility, `register`, pragmas, intrinsics, attributes,
  barriers, raw offsets/addresses, pointer or union puns, undefined behavior,
  byte forcing, alternate flags/tools, object patching, fuzzy credit, or
  comparator exceptions.

This research packet is complete, not parked production code. Reopen it in a
single source-owning lane with the exact coherent candidate above. If that
ordinary spelling does not make a helper exact, the next ledger must record
its size, relocations, normalized hash, first true instruction divergence, and
sibling/data regression state before any second source topology is considered.

## Verification of the research-only packet

The production source has no tracked diff and retains blob
`b6fb90dffeb174f5d4ca8dacc58cd918309b42c2`. The rebuilt local object raw
SHA-256 is
`31c5246c278eaee11dd43fd2a3e941b683e58bc77181c8733199cf9f1ca49800`.
The strict board therefore remains `7 / 19`, `144 / 2448` padded code bytes;
there is intentionally no implementation delta.

The focused whole-TU regression gate passes with `changed_nonexact=[]`,
`newly_exact=[]`, and all seven inherited owners in `still_exact`.

The complete verification set passes:

- `halobetacache_build`, `libcmt_build`, `progress`, and
  `semantic_progress`; the semantic scan reports `unit_errors=0` and Halo
  progress reports `600230 / 1770166` code bytes at this tested baseline;
- object admission reports zero candidates, contradictions, or revocations;
- parked-function audit reports 12 active, zero stale, and zero invalid;
- `python -m pytest -q -p no:cacheprovider`: `212 passed`;
- fixed Units sentinel `_unit_preprocess_node_orientations`: strict exact at
  1,920 padded bytes, 87 relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`;
  and
- `git diff --check`: clean.
