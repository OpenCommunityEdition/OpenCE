# `decals.obj` typed-rasterizer-header reconciliation

## Result

The complete ordinary-C reconstruction remains canonical, but
`decals_delete_permanent_from_cluster` is now recorded as fuzzy and receives
zero exact credit. The other 20 reconstructed functions in the object remain
strict exact.

January and the retained function are both 400 padded / 392 meaningful bytes
with all 29 relocation identities. The production report measures 99.81035
percent similarity:

| owner | size | relocations | normalized SHA-256 |
| --- | ---: | ---: | --- |
| January | 400 | 29 | `b3eeb427eeef1e02f793286b1011a5b89822e041f0b442759259255e1117220a` |
| canonical | 400 | 29 | `6fb830e33513eb040f0ff7cb46cc9167d30a96893dedb145370b416426025ffd` |

## Why prior exact credit was retired

The prior strict result depended on unrelated model declarations remaining in
the broad `rasterizer.h` interface. The publication audit moved those APIs to
`rasterizer_models.h`, their typed subsystem owner, and updated their actual
callers to include that header.

That declaration-context correction changes only the order of two adjacent,
independent stack reloads at the decal function's outer-loop latch. It does not
change the function's control flow, calls, constants, size, or relocation
graph. Reintroducing the previous hash would require duplicate or incorrectly
owned declarations, so the function is honestly parked instead of receiving
inappropriate matching credit.

## Verification

`tools/campaign/gate.py source/effects/decals --all` reports 20 exact, five
residual, and eight unwritten functions. The retained body uses the established
`DECAL_GET` accessor, named flags and semantic globals. It contains no compiler
steering, fake dependency, raw offset, undefined behavior, or nonsensical
logic. The residual is registered in `config/parked.json` as zero-credit
`tu-context-optimization` evidence.
