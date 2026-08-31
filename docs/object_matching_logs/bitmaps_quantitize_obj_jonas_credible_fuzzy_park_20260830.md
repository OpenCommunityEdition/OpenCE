# `bitmaps_quantitize.obj` credible fuzzy park (2026-08-30)

## Result

The object remains `NonMatching` at two of three strict-exact functions, with
all 164 target-owned data bytes exact. The remaining `row_dither` function is
fuzzy-parked as a three-instruction scheduler tie; the park grants no object
or byte-match completion credit.

| Owner | Target/base padded bytes | Relocations | State |
| --- | ---: | ---: | --- |
| `bitmap_quantitize_read_row` | 32/32 | 0/0 | exact |
| `row_dither` | 464/464 | 0/0 | fuzzy parked |
| `bitmap_quantitize` | 592/592 | 28/28 | exact |

The January atlas and multiple independent build maps recover `row_dither`.
No authenticated private name survives for the 32-byte row reader or the
eight-byte reversed-channel table, so `bitmap_quantitize_read_row` and
`bits_per_channel_reversed` are explicitly descriptive rather than claimed
original. Both are private to the translation unit. Public bitmap prototypes
now live under their owner sections in `bitmaps.h`.

## Credibility boundary

The natural Floyd-Steinberg body matches the target's size, frame, control
flow, and every instruction except the order of three independent inner-loop
maintenance operations:

```text
January: inc eax; add ebx,2; add ecx,2
current: add ebx,2; inc eax; add ecx,2
```

Reversing the two obvious C statements emits
`inc eax; add ecx,2; add ebx,2`, not January's order. The prior experiment
packet exhausted declaration order, loop-tail topology, pointer topology,
artificial dependencies, barriers, and pragmas; the later freshness audit
censused 1,571 worktrees and ten source hashes without finding a new donor or
candidate. Diagnostic assembly was never admissible and is not retained.

Target normalized SHA-256 is
`a5feef0c98bc8565991a9cf9722e0258d2bd556e23fc0326891aa0ce0ebca5db`;
the retained candidate is
`7afe02e297bf06dd682a9c44bfd00aacf39c2f7ea0c0f45acce838dc9b696da4`.
Objdiff reports `98.809525%` for `row_dither` and `99.49139%` for TU text.

No assembly, volatile/register forcing, pragma, compiler barrier, undefined
behavior, raw address, byte patch, nonsensical branch, or inert codegen anchor
is present. Reopen only with original local/statement provenance, a natural C
donor built by the same compiler and flags, or a new lawful scheduling lever.
