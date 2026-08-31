# `matrix_math.obj` policy fixed point (2026-08-26)

## Current state

At campaign commit `21b70d6a`, `source/math/matrix_math.obj` has:

- 34/35 strict functions;
- 4,928/5,264 strict padded code bytes;
- 4,635/4,958 ordinary meaningful code bytes;
- 114/114 owned data bytes;
- one absent residual, `_matrix4x3_multiply` (336 padded bytes, 323
  meaningful bytes, zero relocations).

The retained source and exact-function evidence are documented in
`docs/object_matching_logs/matrix_math_obj_codex_checkpoint.md`.

## Historical Claude evidence recovered

The complete older experiment ledger is available at
`f4982274:docs/object_matching_logs/matrix_math_obj.md`.  Its final provenance
result supersedes the earlier open-codegen hypotheses:

- Intel Application Note AP-930, *Streaming SIMD Extensions - Matrix
  Multiplication* (June 1999, order number 245045-001), contains the source
  kernel;
- its `PIII_Mult00_3x3_3x3` routine is a naked hand-written assembly block;
- the first 31 SSE instructions align with the January target in order, and 39
  of Intel's 47 instructions align across five contiguous runs;
- January's extra instructions adapt the 3x3 kernel for the 4x3 position row
  and scalar scale product;
- the distinctive asymmetric `up` load, `shufps 0x36`, final `shufps 0x8f`,
  zero `movaps`, and software-pipelined schedule all originate in Intel's
  hand-allocated kernel.

The same ledger records the exhausted legal-C/intrinsic search:

- 46,080 legal first-row forms and all 40,320 store orders were swept by an
  independent package;
- the best combined intrinsic/pointer-pressure shapes reach exact size but
  remain roughly 293 normalized bytes away;
- an external 35/35 package achieves equality only by transcribing the target
  into assembly and splicing the assembled function into the COFF object.

## Independent PC/Xbox/Digsite corroboration (2026-08-31)

User-supplied testimony from a discussion with Stian and Aerocatia adds an
independent binary-family cross-check. Aerocatia reports that Halo PC contains
three CPU-selected implementations (SSE, 3DNow, and an apparent C fallback),
that the PC SSE implementation and Xbox XBE implementation use the same
assembly, and that the Xbox difference at the end is an x87 multiplication of
the scalar scale. Aerocatia further reports the same SSE instruction stream in
the 2020 Digsite PC build despite its roughly twenty-years-newer compiler.

That is the expected fingerprint of a preserved hand-written kernel plus a C
wrapper/tail: the compiler can change without changing the SSE schedule, while
the target-specific scalar C tail can still be emitted as x87. This testimony
corroborates the stronger AP-930 primary-source attribution and the previously
recorded Oct-2001 debug-XBE prologue identity. It is recorded as testimony until
the compared executables, addresses, hashes, and build identifiers are attached
to the repository.

## Historical decision (superseded 2026-08-31)

No candidate was compiled and no production source was changed.  The only
known exact implementation requires mechanisms prohibited by the current house
rules: inline/standalone assembly, instruction-byte transcription, and object
post-processing.  Repeating intrinsic register-pressure sweeps would add no new
evidence.

Keep configuration index 226 `NonMatching`.  Reopen only if the project policy
explicitly changes to admit original vendor assembly, or if independent
original-source provenance proves that the January body came from a legal-C
source other than the identified AP-930 kernel.

## Current decision

The explicit 2026-08-31 house rule now permits sparse inline assembly in helper
and math functions, and the new PC/Xbox/Digsite evidence independently supports
the already authenticated AP-930 source topology. The historical exact hybrid
from commit `96e8e4ecb` is therefore restored: four ordinary C pointer locals,
one inline SSE block, and the ordinary C statement
`result->scale = a->scale * b->scale`. XDK VC7 emits the wrapper, x87 tail, and
epilogue itself; there is no standalone assembler, byte directive, COFF splice,
or object post-processing.

`matrix_math.obj` is consequently eligible for `Matching` at 35/35. The
function remains listed under `asm-implemented` solely so the exact-match
campaign transparently distinguishes an authenticated original-assembly
recovery from an ordinary-C reconstruction.

