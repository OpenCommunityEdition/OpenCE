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

## Decision

No candidate was compiled and no production source was changed.  The only
known exact implementation requires mechanisms prohibited by the current house
rules: inline/standalone assembly, instruction-byte transcription, and object
post-processing.  Repeating intrinsic register-pressure sweeps would add no new
evidence.

Keep configuration index 226 `NonMatching`.  Reopen only if the project policy
explicitly changes to admit original vendor assembly, or if independent
original-source provenance proves that the January body came from a legal-C
source other than the identified AP-930 kernel.

