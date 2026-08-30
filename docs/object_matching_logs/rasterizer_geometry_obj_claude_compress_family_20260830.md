# `rasterizer_geometry.obj` compress family (2026-08-30)

## Result

Four previously unwritten functions are now strict exact, first compile:

| Function | Bytes | Relocations |
| --- | ---: | ---: |
| `_compress_real_to_int8` | 96 | 7 |
| `_compress_real_to_int8_clamp` | 80 | 5 |
| `_compress_real_to_int16` | 112 | 8 |
| `_compress_real_to_int16_clamp` | 96 | 6 |

Object: **9/19 -> 13/19** strict exact. Board: 4,822 -> 4,826 functions,
667,591 -> 667,975 strict code bytes. No previously exact function changed.

## What unblocked them

These are the "pure oracles" the `fast_ftol` attribution predicted: every one
ends in `fld <param>; fistp <local>`, which no ordinary C conversion emits
under this compiler. They were unreachable until the owner admitted the
verbatim helper on 2026-08-30. With it they wrote first try.

The recovered shape, read directly from January and confirmed byte-exact:

```c
byte compress_real_to_int8(
	real z)
{
	match_assert("...rasterizer_geometry.c", 42, z>=0.0f && z<=1.0f);

	z = z * 255.0f;

	return (byte)fast_ftol(z);
}
```

Two details January's bytes pin down:

- the scaled value is **reassigned into the parameter** (`fstp [ebp+8]`),
  not held in a fresh local;
- the `_clamp` variants are `PIN(z, lo, hi)` folded into the same
  multiply-and-store statement, with no separate clamp temporary;
- the int16 pair round through `(real)floor(z * 32767.5f)` before the
  conversion, which is why they carry a `_floor` relocation.

`fast_ftol` is kept unit-local rather than in its historical `cseries.h`
home, so a shared-header `__inline` cannot perturb unrelated units.

## Blocked: `_rasterizer_geometry_get_vertex_size`

Written trivially, but it cannot match while the repository header disagrees
with January. The target asserts `type < 12` (`cmp si, 0xc`) and indexes a
12-entry `short` table; the repository's
`NUMBER_OF_RASTERIZER_VERTEX_TYPES` is **18**, having imported six later
types (`..._ff`, `model_processed`, `unlit_zsprite`, `widget`). The assert
string proves the bound is the macro, not a literal, so January's enum had
twelve members.

Correcting the enum is a shared-header change that would need a whole-board
rebuild and diff before it could be trusted; it was not attempted here. The
table itself is recovered and recorded for whoever does:

```
56, 32, 20, 8, 68, 32, 24, 36, 20, 16, 16, 8
```

## Attempted, not landed: `_uncompress_int32_to_real_vector3d`

The arithmetic is fully decoded -- three bit fields (11/11/10) sign-extended
by shifting to the top of a dword, `fild`-ed, then
`(2v+1)/(2^n-1)`-scaled with `1/2047` for i and j and `1/1023` for k, using
`2^-20` and `2^-21` pre-scales. Two spellings were measured: independent
field extraction gives 128 bytes, destructive `compressed >>= 11` between
components gives 112, against January's 144. January stages the three
results through float locals (`sub esp, 0xc`, `fstp [ebp-N]`, integer copy
to `result->`), which neither spelling reproduces. Left unwritten rather
than landed non-exact.

## Remaining

`_rasterizer_geometry_get_vertex_size` (enum, above),
`_uncompress_int32_to_real_vector3d` (144),
`_compress_real_vector3d_to_int32` (544),
`_compress_real_vector3d_to_int32_clamp` (512),
`_rasterizer_geometry_compress_vertices` (736),
`_rasterizer_geometry_uncompress_vertices` (1088).

The two `compress_real_vector3d_to_int32` functions are the natural next
targets: they are the vector form of the four landed here and will use the
same helper and the recovered `511.5` / `1023.5` / `32767.5` constants.
