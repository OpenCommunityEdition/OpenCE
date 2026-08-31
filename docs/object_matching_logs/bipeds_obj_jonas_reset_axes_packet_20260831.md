# `bipeds.obj` reset and flying-axes packet (2026-08-31)

## Exact gains

| Owner | Meaningful / padded bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| `_biped_reset` | 88 / 96 | 6 | `3c490b1543002fe3297897886ce356b6e8a1c8083a86e4ba55a9f9bd80c82fb5` |
| `_biped_build_flying_axes` | 428 / 432 | 20 | `16f2f09927b7c10e87e2dcdd50ed848f46ca637136d43296223464e41bf6d6d7` |

Both typed bodies are strict exact and HCEA-correspondence `exact_unique`.
`bipeds.obj` advances from 11 to 13 exact owners without a residual or an
inherited loss.  The reset uses `sizeof(biped->biped)`, not HCEA's later raw
size, and the axes helper uses the real vector operations and validation API.

The `biped_reset` declaration moved from `object_types.c` to the owning
`bipeds.h`; `object_types.obj` remains 31/31 exact.  Every direct header
consumer rebuilt, including `units.obj` at 189/189.

## Correct `_depths_of_hell` owner

The candidate exposed an older canonical type error.  January places
`_depths_of_hell` at offset 16 of the Physics data owner and gives it 16 bytes:
the plane `{ {0, 0, 1}, -256 }`.  Canonical incorrectly defined it as a
four-byte zero `long`, while Claude hid the conflict behind a TU-local
`extern real_plane3d`.

The real plane type, initializer, and declaration now live in
`physics.c/.h`, and `bipeds.c` consumes the owner header.  The rebuilt Physics
data begins with the exact first 32 target bytes (four real globals followed
by the plane).  The target section has another 12 currently unreconstructed
bytes, so no data-owner match is claimed.  All six existing exact Physics
functions remain exact.

No incompatible extern, raw byte access, guessed collision field, or
code-generation directive is retained.
