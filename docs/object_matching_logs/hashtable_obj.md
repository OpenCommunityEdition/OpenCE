# `source/memory/hashtable.obj` strategy ledger

This is a retroactive ledger built from preserved Git, source-comment,
progress-report, and parked-ledger evidence.

## Scope and baseline

- Compiler: XDK 3911 `CL.Exe` 13.00.9254.1,
  `/O2 /Oy- /DDEBUG /Dxbox`.
- January target: `build/split/source/memory/hashtable.obj`.
- Current inventory: 9/10 functions exact; 1,767/2,343 meaningful code bytes.
- Ordinary report: 270/270 data bytes at 100%. The historical park entry
  contains no data-section proof, so strict ownership must be revalidated
  before admission.
- `_hashtable_grow` is the sole residual.

## Parked residual

| Function | Size T/B | Relocs T/B | Hash T/B | Measured residual | Class |
|---|---:|---:|---|---|---|
| `_hashtable_grow` | `0x240/0x240` | `27/27`, including addresses | `0a7974681ab4f334bda59ada858d9aed8aa9a8c9e9b9518270dd31e1f3a7e606` / `6759ee7ece561fdbeda3bedaf66c86487d96f4ae0c7e76b179171c990cc445ac` | Two-byte EAX/ECX permutation while evaluating the commutative `capacity_bits + growth_bits` assertion; surrounding code realigns. | register allocation |

## Preserved experiment history

Historical source-shape work reconstructed the full grow/copy/restore
algorithm and reduced the difference to the two-byte permutation above.
Assertion-expression and local-order tightening were attempted; the original
lane did not retain a complete row-by-row matrix.

Do not repeat blind operand/declaration reordering of the commutative
assertion. Do not use `volatile`, assembly, byte patches, undefined behavior,
or compiler-flag changes.

## Reopen and disposition

Reopen with original local-expression provenance, a matching donor that
anchors EAX/ECX at an isomorphic assertion, or a new defined-C dependency
control that predicts the register choice before compilation.

Disposition: 9/10 code credited; object remains `NonMatching`, and its data
still requires a fresh hardened ownership gate at closeout.
