# `libs/libcmt/tenpow.obj` exact-match ledger

This atomic lane starts from campaign commit `08f86f9b`. Before any production
edit, `tenpow.obj` was confirmed `MISSING` at config index 648, absent from the
parked ledger, all refs, worktrees, prior logs, and active peer ownership.

## Plain-English behavior

Decimal-to-floating-point conversion needs to scale a temporary value by a
power of ten without losing the two extra precision bytes that the CRT keeps
beyond the normal ten-byte x87 value. `__multtenpow12` decomposes that decimal
power into groups of three bits, selects the corresponding precomputed power,
and repeatedly multiplies it into the value. `__ld12mul` performs that
multiplication in integer pieces, normalizes the result, preserves sticky bits,
rounds to nearest-even, and handles zero, denormal, underflow, and infinity.

## Primary provenance and inventory

The exact XDK 3911 member `obj\i386\tenpow.obj` was extracted before writing
source. CodeView names ordinary Microsoft C source
`d:\xbox-finalfre\private\sdktools\crt\fpw32\conv\tenpow.c`. The same readable
source and `cv.h` representation macros were recovered from the pinned public
Microsoft CRT mirror at Easy-Build-2k commit
`36fce23389e43918faf4cd88688c64aa2668ff8f`. January bytes remain authoritative.

The archive member is independently strict-equal to January for the complete
runtime inventory and owns no data:

| Function | Size | Relocs | Normalized SHA-256 |
|---|---:|---:|---|
| `___ld12mul` | 546 | 3 | `23444201cf7f68b269849fa20c86271fdcf86c761984fc2346d296ad0aee58fb` |
| `___multtenpow12` | 118 | 3 | `098da188e1daed753c689758de1f82da97ae5f71a8fcd3b2d314a9f01439a0c8` |

The first function's relocations are `___addl + 0` at `+0xF8`, `___shl_12 + 0`
at `+0x13F`, and `___shr_12 + 0` at `+0x180`. The second uses `_pow10pos + 0`
at `+0x08`, `_pow10neg + 0` at `+0x1C`, and `___ld12mul + 0` at `+0x64`.

## Measured source experiment

| ID | Source factor | Strict result | Decision |
|---|---|---|---|
| E01 | Microsoft `tenpow.c` topology with a named 12-byte type and named layout accessors copied from `cv.h` | Both functions exact on the first compile: 546/546 and 118/118 bytes, 3/3 relocations each, identical hashes and relocation identities | Accepted |

No alternate shape was tested because E01 reproduced the entire object. The
apparently low-level accessors are the documented Microsoft representation of
a ten-byte extended value plus two extra precision bytes; they are named by
field purpose and are not guessed game-structure offsets.

## Strict gates and policy audit

XDK 3911 CL `13.00.9254.1` compiles the accepted source with the unchanged
libcmt `/O1 /Gy` command. A forced object rebuild and both full builds passed.
The refreshed semantic report evaluated 3,587 functions in 461 units, accepted
3,503 exact functions, and reported zero unit errors. The admission audit found
no new candidate, contradiction, or revocation for `libs/libcmt/tenpow`.

Progress reached 360/833 complete objects overall and 93/212 libcmt objects.
Libcmt now has 150/476 exact functions, 10,533/55,015 exact code bytes, and
5,052/8,637 exact data bytes. All 179 tooling tests pass. JSON validation, the
prohibited-construct scan, and `git diff --check` are clean. The clean-commit
per-unit regression snapshot/check classifies both functions as `still_exact`,
with no changed nonexact functions, failures, or warnings.

The source uses named arithmetic constants, descriptive locals, and the
authentic nested multiply/normalize/round control flow. It contains no assembly,
`volatile`, forced inlining, undefined behavior, byte patch, explicit alignment,
compiler pragma, flag change, comparator exception, or codegen-only cast. No
independently actionable original bug was established, so no speculative fix is
commented or applied.
