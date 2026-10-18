# NV2A vertex program to ARBvp1.0 translator

`port/mac/mac_nv2a_arb.{h,c}` turns an NV2A vertex program (the Xbox
instruction words) into an `ARBvp1.0` program, so that a shader can run in the
vertex unit of a Radeon 9550/9700-class GPU where Apple's GLSL compiler
sends it to a software path. It is the production form of the prototype
in `tools/mac_arb_retail_probe.c` (see `docs/ppc-mac-arb-vertex.md`). It is
pure text generation, C89, and calls nothing of GL.

## API

```c
int mac_nv2a_arb_translate(const unsigned long *instructions,
    unsigned long instruction_count, int bone_gather, char **arb_source_out,
    unsigned *env_parameter_count_out, struct mac_nv2a_arb_layout *layout_out,
    char *error, size_t error_size);
int mac_nv2a_arb_fragment_prelude(const struct mac_nv2a_arb_layout *layout,
    char *out, size_t out_size);
int mac_nv2a_arb_fragment_rewrite(const char *fragment_source,
    const struct mac_nv2a_arb_layout *layout, char **rewritten_out,
    char *error, size_t error_size);
```

`translate` returns 1 with a malloc'd program, 0 when the program uses
something unsupported (`error` says what; keep the GLSL path), -1 on
allocation failure or a bad argument. It stops at the instruction with the
final bit, as `nv2a_vertex_shader_to_glsl` does. The layout reports the
environment parameter count (195, always), whether the bone gather was used
(the caller then applies the `mac_bone_gather` plan to attributes 10..15 as
for GLSL, or, for `MAC_NV2A_ARB_RELATIVE`, that `A0` is used) and where the fog
travels.

`bone_gather` selects how relative constant reads (`c[a0 + k]`) are handled:

| value | meaning |
| --- | --- |
| 0 | any relative read is refused |
| 1 | bone gather: the program must match the pattern; the six reads are attributes 10..15 (`layout.bone_gather_used`) |
| 2 (`MAC_NV2A_ARB_RELATIVE`) | native relative addressing, any program; `layout.relative_used` is 1 when the program loads `A0` |

`fragment_rewrite` takes the fragment source from `mac_glsl120_pixel`,
drops the `varying` declarations of the eight colors/coordinates and the fog,
and puts the `#define` lines of `fragment_prelude` after `#version 120`. It
refuses (returns 0) a shader that declares any other varying.

## Environment parameters

| `program.env[n]` | Contents |
| --- | --- |
| 0..191 | the game's vertex constants c[0..191] |
| 192 | `viewport_scale` (the GLSL uniform) |
| 193 | `viewport_offset` |
| 194 | x `screen_offset`, y `point_size` |

195 parameters are used by every program. ARB_vertex_program guarantees only
96 (`GL_MAX_PROGRAM_ENV_PARAMETERS_ARB`); the caller checks the limit and keeps
GLSL if it is lower. Only the constants a program reads are declared.

## Inputs

NV2A `v0..v15` are `vertex.attrib[0..15]`. With `bone_gather` the six
relative constant reads `c[a0+60..62]` (twice) are `vertex.attrib[10..15]`,
numbered in the order of the printed operands (A, B, C of each instruction),
as `mac_bone_gather_rewrite` numbers them.

With `MAC_NV2A_ARB_RELATIVE` attributes 10..15 are not used at all and no
gather plan is applied: the caller binds the ordinary attributes and sets
`program.env[0..191]` to the whole of c[]. See "Native relative addressing".

## Native relative addressing

`ARBvp1.0` allows a relative address only into a declared parameter array, so
a program with relative reads declares `PARAM cr[192] = { program.env[0..191] };`
and `ADDRESS A0;`, sets `A0.x` to 0 at the start (the GLSL's `int a0 = 0`),
and a relative read `c[a0 + k]` of the shared GLSL translator
(`nv2a_vsh.c`: `c[clamp(a0 + k, 0, 191)]`, `k` the instruction's raw constant
field, no bias) is `cr[A0.x + k]`. Every constant operand of an instruction
with the relative bit is relative, as in the GLSL (they all use the one
constant field). The NV2A `ARL` (`a0 = int(floor(mac.x + 0.001))`) is

```
ADD t0.x, mac.x, ka.x;   # + 0.001
MAX t0.x, t0.x, ka.z;    # lower bound
MIN t0.x, t0.x, ka.w;    # upper bound
ARL A0.x, t0.x;          # floor
```

done after both units have read their inputs, so a read in the same
instruction sees the old address. `ka` is `{ 0.001, shift, low, high }`.

- Bounds. The GLSL clamps each read's index to 0..191. The program clamps the
  float before `ARL` (floor of a clamped float is the clamped floor, the
  bounds being integers) to `[-kmin, 191 - kmax]` over the offsets of the
  program's relative reads, so every read stays inside `program.env[0..191]`
  for any address. For every address that keeps all reads inside c[0..191]
  (the bone addresses of valid data, `v5 * c[7].w` plus the row offsets 60..62:
  bones 0..43) the result is exactly the GLSL's and the gather's. For an address
  outside that range the rows differ from the GLSL's per-read clamp (a read
  whose own index is still in range may be moved), but are bounded. A NaN
  address is not defined (the gather path refuses the draw on a non-finite
  address; the caller of mode 2 should too if it checks at all).
- Offsets. ARB allows an immediate offset of -64..63. When the highest `k`
  exceeds 63 (not in the retail programs, whose offsets are 17, 21, 60, 61 and
  62) the lowest `k` is added to the `ARL` source (`shift`) and subtracted from
  every immediate (`floor(v) + s == floor(v + s)` for an integer `s`);
  offsets more than 63 apart, or above 191, are refused.
- Cost: four extra instructions per `ARL` (two ARLs in a skinned program),
  no vertex attribute traffic, no client-side array per skinned draw.
- All 67 retail vertex shaders translate in this mode; 23 have relative reads
  (5, 9, 10, 11, 13, 14, 15, 17, 18, 30, 31, 32, 33, 36, 39, 45, 47, 50, 57,
  60, 62, 63, 64). Shaders 11 and 33 do not match the gather pattern (three
  ARLs, reads at 60, 21 and 17 from `v8` and `c[15]`); they only run
  natively in this mode.

## Outputs

| ARB result | NV2A output | GLSL varying |
| --- | --- | --- |
| `result.texcoord[0]` | oB0, clamped to [0,1] | `xB0` |
| `result.texcoord[1]` | oB1, clamped | `xB1` |
| `result.texcoord[2..5]` | oT0..oT3 | `xT0..xT3` |
| `result.texcoord[6]` | oD0, clamped | `xD0` |
| `result.texcoord[7]` | oD1, clamped | `xD1` |
| one unwritten component of a texcoord slot | oFog.x | `xFog` |
| `result.pointsize.x` | oPts.x (default `point_size`) | n/a |
| `result.position` | the epilogue below | n/a |

Fog is not clamped by any free ARB result (`result.color` clamps), so it
travels in a component that the program never writes (the search takes
B1, B0, D1, D0, T3..T0, component w first). That component is a constant
(0,0,0,1)[c] in the GLSL path, and the prelude restores it:
`#define xB1 (vec4(0.0, 0.0, 0.0, gl_TexCoord[1].w))`,
`#define xFog (gl_TexCoord[1].w)`. A program that never writes oFog has the
constant `xFog` of 1.0 and uses no slot (retail entries 31 and 50 are like
this). If no component is free the program is refused.

Per-pixel lighting (`xWorldNormal`, `xWorldPosition`) cannot be carried: all
eight texcoord slots are used. The caller must not offer a program with a
lighting argument, and `fragment_rewrite` refuses such a fragment shader.

`result.pointsize` needs `GL_VERTEX_PROGRAM_POINT_SIZE_ARB` to take effect.

## Position

The epilogue of `nv2a_vsh.c`, then the Mac depth remap of
`mac_glsl120.c` (`z = 2z - w` after the guard): a zero scale component
stands for 1; with a captured clip position (`rcc` of r12.w) the position is
`(clip.xyz * c[58].xyz + (c[59].xyz + (0.5 + screen_offset, 0.5, 0) -
viewport_offset.xyz) * clip.w) / scale`, otherwise from oPos; a w of zero or
NaN gives (0,0,0,-1) before the remap, so (0,0,1,-1) after it.

## Operations

All MAC operations (including `ARL`'s `mac = A`, which an output write can
use) and ILU `MOV RCP RCC RSQ EXP LOG LIT`:

- `RSQ` is ARB's own (it takes the absolute value as `inversesqrt(abs(x))`).
- `RCC` clamps by sign to +-[5.42101e-20, 1.884467e+19] with exact 0/1 masks.
- `EXP`, `LOG` use `EX2`, `LG2`, `FLR` (not ARB's approximate `EXP`/`LOG`);
  `LOG` of zero gives (-1e30, 1, -1e30, 1) by masks that never multiply an
  infinity. `LIT` is `exp2(w * log2(y))` held below the largest float and masked
  by `x > 0`, with log2(0) held at -1e30, so `pow(0, 0)` is 1.
- Both units read the state before either writes; the ILU writes r1 when the
  MAC runs, r12 is oPos, and a program's unconnected operand reads as zero.
- Registers read before they are fully written start at zero (`MOV r, kc.x`),
  as in GLSL; the rest are not initialised. A single-unit instruction that writes
  only a temporary is emitted as one ARB instruction.

## What is refused

Returns 0 for: a MAC code of 14 or 15; a temporary above r12 or a constant above
c[191] in a used operand; a relative constant read when `bone_gather` is 0;
with `bone_gather`, any relative read pattern that is not the one
`mac_bone_gather_rewrite` recognises, checked on the instruction words: one
`v5 * c[7].w` product to r.xy, exactly two ARLs (the first from r.x, with
`v4.zzzz` through the ILU or the legacy form, the second from r.y), no write
to r.x or r.y between the product and the second ARL, exactly six relative reads
`xyzw` at offsets 60, 61, 62, 60, 61, 62, the first three between the ARLs, the
last three after, and no ordinary read of attributes 10..15; more than 136
instructions; no free fog component.

## Differences from the prototype

- Live viewport and offsets from the environment; zero-scale fallback; no-capture
  path; depth remap; the w guard (the prototype had fixture constants).
- Relative reads are validated on the words rather than numbered by encounter
  order (the prototype's documented weakness).
- Constants from `program.env`, not `program.local`; the outputs, fog and
  point-size defaults as in GLSL; only used constants, attributes and
  temporaries are declared; fewer instructions (entries 31 and 50 are 164 and
  141 ARB instructions).
- `EXP`, `LOG`, `LIT` and `ARL`-output writes are supported; the prototype
  refused them.
- Fog is carried in an unwritten texcoord component, not in `result.color`.
- The fragment prelude gave a slot that carries the fog the wrong components
  (zeros for the slot's own values, the fog in the place of the default); it
  now reads the slot's own components and the default for the one that carried
  the fog (found by the GPU comparison of shaders 5, 9, 14 and 18, whose fog
  travels in xB1.w).

## Verification and open risks

`tools/mac_nv2a_arb_checks.sh` (no GPU, no game data) runs a checker that
interprets the generated text (declarations, opcodes, reads of undefined
components, every result written) beside a direct interpreter of the words that
follows `nv2a_vsh.c`: on both retail shaders with gathered rows from
`mac_bone_gather_apply`, and on random programs for every MAC and ILU
operation. Values agree to 2e-4 relative; non-finite reference values are not
compared. All 67 retail shaders are also translated in `MAC_NV2A_ARB_RELATIVE`
mode (the checker understands `ADDRESS`, `ARL` and `cr[A0.x + n]`) and the 23
with relative reads are compared on 40 vertices each, with bone addresses
in range; wild addresses, offsets above 63 and the refusals are tested.

`tools/mac_nv2a_arb_gpu_probe.c` (built by `mac_nv2a_arb_gpu_checks.sh`, run
by hand on each Mac) compares the programs with the GLSL path on a GPU. Results
of 2026-10-17 (`build/mac-audit/arb-relative/PROVENANCE.md`): every
output of the 23 relative shaders in mode 2, and of 31 and 50 in mode 1 and
three plain shaders in mode 0, is identical to the pixel on a GeForce 9400M,
a Radeon 9600 XT and a Radeon 9550, depth within 2 LSB of 24 bits, mode 2
identical to mode 1 on the same GPU, `kCGLCPGPUVertexProcessing` 1 and
`GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB` 1 for every program on the Radeons (their
GLSL runs 31, 50 and eight more in software). The one exception is the fog of
shaders 5, 9, 14 and 18, where Apple's R300 GLSL path differs from the ARB
path (and from the GeForce's GLSL); the ARB result is the one the GeForce gives.

- The w guard selects arithmetically. A zero w with finite position is exact;
  a NaN w gives a NaN position when the hardware computes `0 * NaN` as NaN
  (the ARB spec has no conditional select).
- The division by `viewport_scale` is by reciprocal (exact for powers of two).
- Two distinct program parameters appear in some instructions (`sp` and `kc`);
  the ARB spec does not forbid this, and the prototype did it, but a driver
  that does will refuse the program, which is the signal to keep GLSL.
- Native resource limits (ALU instructions, temporaries, parameters)
  are the caller's to read with `GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB`.
