# `lrar_cache.obj` source-credibility cleanup (2026-08-31)

## Outcome

`source/memory/lrar_cache.obj` is now honestly classified `NonMatching`: 11 of
12 functions are strict exact and `_lrar_allocate` is fuzzy-parked at
67.61811%. The earlier 12/12 result is revoked because it depended on source
that was written to steer VC7 rather than on a plausible original program.

The retained C is coherent and uniform. Every block lookup calls
`lrar_get_block`; no helper body is selectively copied into callers, and no
`__declspec(noinline)` remains.

## Rejected exact construction

Commit `14e4e8adf` reached exact bytes by combining two independent controls:

- `__declspec(noinline)` on the private block getter; and
- hand-expansion of that getter's cache verification, index assertion, block
  address, and block verification in exactly two call sites.

Those expansions duplicated an existing helper without a semantic reason.
They are compiler-output transcription, not credible source, and are rejected
even though they reproduced January bytes.

The same audit restored natural helper calls in `lrar_block_address` and
`lrar_deallocate`. VC7 inlines them there by itself and both functions remain
strict exact, which is positive evidence that the natural source shape is
appropriate.

## Private names

No surviving January public symbol names these six private functions. Their
descriptive names follow the neighboring first-party `lra_cache.c` family and
their observed behavior:

| Address-era label | Retained name |
|---|---|
| `code_0010bfa0` | `lrar_default_lock_proc` |
| `code_0010bfb0` | `lrar_default_unlock_proc` |
| `code_0010bfc0` | `lrar_unlock_block` |
| `code_0010bfe0` | `lrar_verify_block` |
| `code_0010c040` | `lrar_verify_cache` |
| `code_0010c280` | `lrar_get_block` |

All are private in source and marked static in `config/symbols.json`.
`lrar_default_unlock_proc` also now uses the owner typedef's `void *`
parameter and performs an explicit `short *` conversion instead of relying on
an incompatible function-pointer assignment.

## Measurement

The natural source retains 11 strict-exact functions and one residual:

| Function | Target | Base | Relocations T/B | objdiff |
|---|---:|---:|---:|---:|
| `_lrar_allocate` | 784 | 1056 | 27/47 | 67.61811% |

- target normalized SHA-256:
  `a34c14dad0afc2a3462c738ca6153c3b46d2bdd0164b176dd208295594f86663`
- base normalized SHA-256:
  `8a2fe453345af3d00eaf476f17eedbfc67629d8af6d6a036cc097d669dcc3652`

January calls `_lrar_get_block` at `+131` and `+207`. The current incomplete
translation unit inlines the validation body at those sites and adds direct
`_lrar_verify_block` relocations. The fail-closed classifier reports UNKNOWN;
the park's `tu-context-optimization` label records only this directly observed
selective-inlining boundary, not a recipe.

## Source analogues

The published Bungie Marathon 2 source was checked at
`research-cache/m2-infinity-source-code-full-20260831` as a first-party naming
and style analogue. Its released CSeries sources contain no LRAR
implementation, so it supplies no missing helper or call-boundary authority.
The neighboring Halo `lra_cache.c` implementation is the closer local naming
analogue.

Reopen `_lrar_allocate` only with original LRAR source, recovery of missing
whole-TU call-graph context, or another ordinary source form. Do not restore
`noinline`, copied helper bodies, pragmas, volatile controls, barriers, dead
code, or assembly for match credit.
