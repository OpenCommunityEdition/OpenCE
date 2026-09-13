# `particle_systems.obj`: owner-safe render reconstruction (2026-09-12)

## Scope

This packet reconstructs the formerly unwritten
`particle_system_render` body from the January COFF control flow and uses the
HCEA reconstruction only as semantic cross-build evidence.  It also completes
the existing `particle_system_type_particle_state` owner with the embedded
shader-effect record at offset `0xB8`.  Bitmap sequences, shader effects, and
sprite batching continue to use their shared owning headers; no translation-
unit-local duplicate layout was introduced.

The reconstruction uses typed particle-system and bitmap tag access, named
particle/render flags and modes, project `real`/color/vector types, and the
shared sprite-builder API.  It retains the January behavior that interpolates
current and transition state values, rejects incompatible shader transitions,
chooses a stable sprite frame, renders simple or rotational sprites, applies
system lighting for the zero blend mode, and updates the batch shader's
secondary-map radius before submission.

## Gate result

The focused gate first changed the unit from
`15 exact / 9 residual / 1 unwritten` to
`15 exact / 10 residual / 0 unwritten`.  Independent integration review then
restored the function's TU-private linkage.  That gives VC7 the same natural
interprocedural call boundary as January, closes the sole caller
`particle_systems_render` exactly, and leaves the final unit at
`16 exact / 9 residual / 0 unwritten`.  All inherited exact functions remain
exact, and the `point_from_line3d` emitted-symbol guard passes.

`particle_system_render` is a coherent fuzzy candidate and receives **zero
exact credit**:

| measurement | January target | retained candidate |
| --- | ---: | ---: |
| meaningful code bytes | 1,346 | 1,335 |
| padded section bytes | 1,360 | 1,344 |
| relocations | 33 | 35 |
| normalized SHA-256 | `c4d841f9803c7f3e816b29a2a35274f12a6bf63eaec2fec2b0eef97ccc17d946` | `eb89022c439f93d240734c9bad1c0f5dc26f9c52c55ed8bd8695615ad7b32f1a` |

The January sole caller moves `system_index` into `EAX` and calls without a
stack argument.  Declaring the one-caller body `static` reproduces that boundary
naturally: the caller is now strict exact and the callee consumes `EAX` at
entry.  The remaining callee differences begin with a four-byte frame-size
delta (`0x11C` versus `0x118`), register allocation, and two extra floating-
constant relocations.  Those are expression-scheduling differences, not
missing renderer behavior.  This packet therefore parks the natural source
instead of adding a calling-convention shim, compiler-control annotation, or
expression-order steering.

## Collateral checks

- `effects.obj` direct owner-header consumer: `32 exact / 9 residual / 0 unwritten`;
- `objects.obj` direct owner-header consumer: `121 exact / 0 residual / 0 unwritten`;
- changed-file fake-match scan: zero review leads;
- `git diff --check`: pass;
- no inline assembly, raw offset access, address-derived identifiers,
  volatile/register steering, forced inline/noinline control, or candidate-only
  helper was introduced.
