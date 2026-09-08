# `random_math.obj` Bungie-source name reconciliation (2026-09-08)

## Result

The supplied Bungie source at
`C:\Users\isabe\Downloads\haloleak2024\halo1\random_math.c` identifies
canonical's private `_code_000fab20` owner as `direction3d_from_table` and
authenticates its parameter order as `(real_vector3d *direction, short index)`.
This packet adopts that name and signature, types the direction table as
`real_vector3d *`, and records the source constants `RANDOM_A`, `RANDOM_C`, and
`RANDOM_DIRECTION_TABLE_GEOSPHERE_SEGMENT_COUNT`.

The C file now follows the supplied include map by including the genuine
`geometry.h` owner. Its previous TU-local `struct geosphere`, `geosphere_new`,
`geosphere_dispose`, and `system_seconds` declarations are removed; the
source-attested vector view is expressed at the table copy. This proper-owner
cleanup remains 17/17 exact.

Both the name-only and authentic reordered-signature candidates gate at the
existing **17 exact / 0 residual / 0 unwritten**. All 17 inherited exact owners
are preserved; this source-quality correction claims **zero new exact bytes**.
The semantic target mapping is:

```text
1026848  _code_000fab20 -> _direction3d_from_table  (static)
```

No generic `float`, raw-address identifier, fake dependency, inline directive,
or compiler-steering construct was introduced. Literal diagnostics use the
appropriate assertion macro, no-argument definitions use `void` on its own
line, and every void definition ends in an explicit `return;`.

`docs/common_constants.md` already carries these constants for future agents,
so this packet does not duplicate that index.

## Verification before target regeneration

- Alias gate with the semantic owner: 17 exact / 0 residual / 0 unwritten.
- Authentic parameter-order trial: 17 exact / 0 residual / 0 unwritten.
- Fake-match scan: zero findings.
- Exact-byte credit: zero; naming and source authenticity only.
