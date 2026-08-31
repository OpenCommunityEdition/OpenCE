# `matrix_math.obj` authenticated vendored-kernel recovery

## Result

`source/math/matrix_math.obj` is byte-complete at **35/35 functions**. The last
function, `_matrix4x3_multiply`, compiles to January's exact 336-byte padded
section with zero relocations and normalized SHA-256
`0436d82fe1070f30523ce8798bcce3b7625099794f5d0ab3fbf9a9c33802314e`.
All previously exact functions and all 114 owned data bytes remain exact.

This is a compiler-built C/assembly hybrid, not a copied object: XDK VC7 compiles
the source translation unit normally. There is no standalone assembler, emitted
byte sequence, alternate compiler, COFF append, object splice, or post-build
patch.

## Original-source provenance

The historical fixed-point research identified the primary source as Intel
Application Note AP-930, *Streaming SIMD Extensions - Matrix Multiplication*
(June 1999, order 245045-001). Its `PIII_Mult00_3x3_3x3` routine is published as
a naked hand-written `__asm` kernel. January preserves its distinctive schedule:

- the first 31 SSE instructions align in order;
- 39 of Intel's 47 instructions align across five contiguous runs;
- the asymmetric up-row load and `shufps 0x36` are retained;
- `shufps 0x8f` reorders lanes immediately before the third-row store;
- all eight XMM registers are hand allocated with no `movaps` copies;
- row calculations are software-pipelined.

The additional instructions adapt Intel's 3x3 multiply for Halo's fourth
position row and uniform scalar scale.

The user supplied a 2026-08-31 discussion with Stian and Aerocatia as independent
corroboration. Aerocatia reports three PC implementations selected by CPU
detection or command line (SSE, 3DNow, and an apparent C fallback), identical
SSE instruction streams in the PC executable and Xbox XBE, and the same SSE
stream in the 2020 Digsite PC build despite its much newer compiler. The Xbox
version differs at the end by using x87 for the scalar scale multiplication.
That observation independently predicts exactly the recovered source boundary:
a preserved assembly kernel plus an ordinary C scale tail. The testimony is
recorded as corroboration pending executable hashes, function addresses, and
precise build identifiers.

## Recovered source topology

Historical exact commit `96e8e4ecb` supplied the decisive source-level witness
and was reverted by `861c679f3` only because the former house rule prohibited all
assembly. The 2026-08-31 owner rule now permits sparse inline assembly in helper
and math functions when the original assembly is authenticated.

The restored function has four ordinary pointer locals, in declaration order:

1. `a_elements = a->n[0]`;
2. `b_elements = b->n[0]`;
3. `result_elements = result->n[0]`;
4. `a_scale = &a->scale`.

VC7 homes the first two at `[ebp-8]` and `[ebp-4]`, reuses the dead incoming
parameter slots for the latter two, and loads those pointers into Intel's
`ecx`/`edx`/`eax` convention. The one inline assembly block implements the
authenticated SSE schedule. The final statement remains ordinary C:

```c
result->scale = a->scale * b->scale;
```

VC7 emits that statement as the target x87 `fld`/`fmul`/`fstp` tail interleaved
with the callee-saved register pops. The public prototype remains in its correct
owner, `source/math/real_math.h`; no header or caller changes are required.

## Negative controls retained

The former C/intrinsic search remains useful negative evidence: 46,080 legal
first-row forms and all 40,320 store orders were swept, and combined
register-pressure experiments still stopped roughly 293 normalized bytes away.
Named `__m128` locals force the wrong aligned-stack prologue; loops, helper
functions, pointer layouts, intrinsic reorderings, and store schedules do not
reproduce Intel's hand allocation. No codegen trick from those experiments is
retained.

## Verification

- `tools/campaign/gate.py source/math/matrix_math --all`: 35 exact, zero
  residual, zero unwritten.
- Full `all_source progress build/report.json`: pass.
- Semantic audit: 4,970 accepted exact, zero ordinary rejected, zero unit
  errors.
- Aggregate progress: 633,780 / 2,198,102 code bytes, 4,927 / 11,060 functions,
  384 / 833 complete objects.
- Halo progress: 620,866 / 1,770,166 code bytes, 4,760 / 7,574 functions,
  282 / 468 complete objects.
- Tool tests: 227 passed.
- `config/config.json` index 226 is `Matching`; the function remains classified
  `asm-implemented` in `config/parked.json` for transparent provenance
accounting.
