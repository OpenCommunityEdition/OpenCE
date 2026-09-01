# `rasterizer_geometry.obj` compress family (2026-08-30)

> Historical experiment ledger. Its intermediate `fast_ftol` ownership and
> `14/19` conclusion are superseded by
> `rasterizer_geometry_obj_jonas_reconciled_closeout_20260901.md`. The measured
> negative controls and three parked-function fingerprints remain applicable.

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

## The vector3d pair: reconstructed, size/relocation exact, 2 instructions short

`_compress_real_vector3d_to_int32` (544) and `..._clamp` (512) are now
written. Both measure **size exact, relocations exact (38/38, address for
address), 181/181 and 164/164 instructions**, sha differing at a single
transposition. Not exact; no credit.

Recovered structure (both, from January's bytes):

```c
unsigned long compress_real_vector3d_to_int32(
	real_vector3d const *v)
{
	long i, j, k;
	real_vector3d v2;

	match_assert(FILE, 69, v);
	match_vassert(FILE, 78, <all three components in [-1,1]>,
		csprintf(temporary, "invalid vector= [%f %f %f] 0x%x%x%x",
			v->i, v->j, v->k,
			*((long *)&v->i), *((long *)&v->j), *((long *)&v->k)));

	i = fast_ftol((real)floor(v->i * 1023.5f)) & 0x7ff;
	j = fast_ftol((real)floor(v->j * 1023.5f)) & 0x7ff;
	k = fast_ftol((real)floor(v->k * 511.5f)) & 0x3ff;

	v2 = *uncompress_int32_to_real_vector3d(&v2, ((k << 11) | j) << 11 | i);

	match_assert(FILE, 92, fabs(v2.i - v->i)<0.01f);   /* 93, 94 for j, k */

	return ((k << 11) | j) << 11 | i;
}
```

Pinned by the target: 11/11/10-bit fields with `1023.5f`/`511.5f` scales and
`0x7ff`/`0x3ff` masks; the pack is `((k << 11) | j) << 11 | i`; the debug
round trip is a **self-referencing** aggregate copy through the same local
(`&v2` in, `*result` back into `v2`); the message buffer is the global
`temporary` and the format is exactly 36 bytes including NUL, which is what
fixes `0x%x%x%x` rather than a spaced form. The clamp twin folds
`PIN(x, -1.0f, 1.0f)` into the same multiply and asserts at lines 104 and
118-120.

### The one unsolved instruction

Two spellings bracket the target and neither reaches it:

| form | frame | copy block | differing |
| --- | --- | --- | ---: |
| one local, self-copy (**landed**) | `sub esp,0x10` correct | loads/stores interleaved | 10 insns |
| separate `decompressed` local | `sub esp,0x1c` (+12) | grouped: all three loads, then stores — **correct** | 2 bytes |

January needs *both*: a single 12-byte slot **and** the aliasing-safe
grouped copy. The landed form is the faithful one — by the campaign's
frame-layout law a frame-size mismatch is structural, so the byte-closer
two-local variant is the wrong reconstruction despite measuring nearer.

Measured inert on the copy schedule, all at 10 differing instructions:
copying through a named pointer, that pointer scoped to an inner block,
declaring `v2` before the integer locals, and both combined. Naming the
packed value in a local regresses to 528 bytes.

## `_uncompress_int32_to_real_vector3d`: 2 instructions, parked

144/144 padded bytes, 9/9 relocations with identical destinations, 49/49
instructions. One dependency-free transposition remains: January emits
`fadd __real@3f800000` before `shl eax, 0x16`; ours emits them swapped,
which also shifts that relocation's address by three bytes.

The shape that fixed the frame was **a local `real_vector3d` copied out
through `*result`**, not three scalar `real`s — `sub esp, 0xc` is the
vector, and naming three scalars lets VC7 fold them away entirely (112
bytes). Fields are 11/11/10 bits with `2^-20`/`2^-21` pre-scales and
`1/2047`/`1/1023` scales, reached by *destructive* shifts (`compressed >>=
11`) so the `shr` sequence matches rather than collapsing into masks.

Measured inert: folding the second shift into the k expression, long-hand
assignment, hoisting the shift ahead of j, a division form of the k scale.

## Parked

Three functions are parked as `instruction-scheduling` ties, each meeting
the parking standard — exact padded size, exact relocation count and
destinations, full instruction-count parity, one precisely classified
divergence, and an exhausted control set recorded in the entry:

| function | bytes | insns | divergence | fuzzy |
| --- | ---: | ---: | --- | ---: |
| `_uncompress_int32_to_real_vector3d` | 144/144 | 49/49 | one `fadd`/`shl` transposition | 95.0 |
| `_compress_real_vector3d_to_int32` | 544/544 | 181/181 | round-trip copy grouped vs interleaved | 96.35 |
| `_compress_real_vector3d_to_int32_clamp` | 512/512 | 164/164 | same as its twin | 94.8 |

The build gates on parked evidence, so these fail closed if they drift.

## Remaining

Two functions are still unwritten:
`_rasterizer_geometry_compress_vertices` (736) and
`_rasterizer_geometry_uncompress_vertices` (1088). Both are the bulk
converters that drive the compress/uncompress helpers now in place, so they
should be attempted with those as fixed points.

Object state: **14/19 strict exact, 3 parked, 2 unwritten** (from 9/19 at
the start of the session).

The two `compress_real_vector3d_to_int32` functions are the natural next
targets: they are the vector form of the four landed here and will use the
same helper and the recovered `511.5` / `1023.5` / `32767.5` constants.
