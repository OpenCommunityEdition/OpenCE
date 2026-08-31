# `prime_numbers.obj` credible fuzzy park (2026-08-30)

## Accepted source state

The January symbol atlas recovers `_primegen` in this exact object. The
September 2001 linker map independently recovers the private
`compare_ulongs_descending` name in `prime_numbers.obj`; it is therefore
declared `static`. Public declarations now live in `prime_numbers.h`.

The production body remains the simplest donor-consistent C sieve. No
assembly, volatile/register forcing, pragma, barrier, undefined behavior,
inert expression, raw address, or byte-directed statement move is retained.

## Strict result

| Owner | Target padded bytes | Relocations | State |
| --- | ---: | ---: | --- |
| `_compare_ulongs_descending` | 32 | 0 | exact |
| `_primegen` | 352 | 12 | fuzzy parked |
| `_randomprime` | 80 | 4 | exact |
| `_probable_prime64` | 192 | 10 | exact |

`_primegen` retains all twelve relocation identities and order. Its target
normalized SHA-256 is
`2428a7a00be6ab6f96efe9dda273d824a0cc9829aeeb2dd5f4c9b35b2b7992b1`;
the retained candidate is
`66e1c7859e354a6bf7caccb70b260e99d516be6a94837571ecc60dd5b43edda6`.
Objdiff reports `81.614815%` for the function and `90.09357%` for the TU text.
All 72 target-owned data bytes are exact.

## Credibility disposition

The E1-E27 source matrix and the later Stian/Pastudan donor packet isolate the
residual to January's simultaneous register/stack lifetime choice. A later
pre-allocation `k` store reaches `91.14815%`, but its own ledger states that it
creates the target frame through a different route than January; it is not
accepted merely because it scores better. A separate statement relocation can
produce an exact byte stream but lacks credible original-source rationale and
is likewise rejected.

This object stays `NonMatching` at 3/4 strict-exact functions. The park records
queue disposition only and grants no object or byte-match completion credit.
