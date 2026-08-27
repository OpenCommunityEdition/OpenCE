# `light_volumes.obj` distance-fade scheduling fixed point

## Scope and evidence

This bounded follow-up tested one new source position for the sole residual
`_light_volume_render`. It began from clean commit
`d35a4c2f7846259ec722b4225727e9fc27a515b3`, with the unit at 9/10 strict
functions and the renderer at 928 candidate versus 912 target padded bytes.

Before freezing the candidate, the following object-specific evidence was
read together:

- `docs/object_matching_logs/light_volumes_obj_jonas_partial.md`;
- `docs/object_matching_logs/light_volumes_obj_jonas_interpolator_recovery_20260826.md`;
- the retained source and its full local history, including donor foundation
  `dca60279`;
- `surreptitiousresearch/halocea` renderer/interpolator/frame sources at
  `570c83fd9c365dad6f2a3e7041705d5b84c7847c`;
- January and candidate disassembly for the complete renderer.

January assigns the `1.0f` distance-fade default with an immediate integer
store between the x-axis delta's `fsub` and `fstp`. The retained source
initializes the local before both helper calls. The prior ledger had tested an
assignment immediately after marker acquisition, but had not explicitly
tested the only C89 statement position that could expose the target latency
slot: declare the local at block entry, compute `delta.x`, then assign
`distance_fade = 1.0f` before computing `delta.y`.

The clean fail-closed baseline is
`build/audit/light_volumes_delta_fade_baseline_20260826.json`. Baseline source
SHA-256 was
`6f9c9172685d4d1645a6a08ded33b20c14b210f99e63855ef242f420a2f251a5`.

## Single candidate result

Exactly one code-producing candidate compile was performed. It emitted raw
object SHA-256
`8fe6afbaf9453795008dbf8853c7f40b2d2cfe8353416b65b4e6162bf7c2938c`.

The candidate did not improve the residual:

| Measurement | January | Candidate |
| --- | ---: | ---: |
| Padded bytes | 912 | 928 |
| Relocations | 47 | 48 |
| Normalized SHA-256 | `2e43dbc111ad125565ead94aff803117a92a3bb3215d18254e28281e204d0031` | `26bdbaa273cdbace8e5a76bd8640a0d1b67d664e54e78262113e30290af255f5` |

VC7 moved the assignment ahead of the x-axis result store, but materialized
`1.0f` through an additional read-only constant relocation rather than the
target immediate store. The result is the same 928-byte/48-relocation fixed
point documented for the earlier post-marker retry. This proves the optimizer
canonicalizes both neighboring legal-C source positions to the same machine
shape; another statement-position sweep is not justified without new source
or toolchain provenance.

## Regression and disposition

The whole-TU regression gate reported all nine accepted functions still exact
and only the already-nonexact renderer changed. The candidate was rejected.
The retained source was restored byte-for-byte to its baseline SHA-256, its
object was rebuilt with XDK 3911, and the baseline whole-TU check then passed
with no failures, warnings, changed nonexact functions, or lost exact
functions.

No source, header, configuration, semantic ledger, parked-function record,
target artifact, protected Units file, or compiler flag is changed by this
checkpoint. `light_volumes.obj` remains honestly partial at 9/10 strict. Reopen
the distance-fade lane only with original/line-record source evidence, a
same-lineage source donor that distinguishes the immediate-store topology, or
a compatible authentic compiler/QFE that changes this fixed point.
