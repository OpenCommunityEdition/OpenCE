# `collision_debug.obj` public-donor audit (2026-08-26)

## Scope

This note records the evidence used to decide whether the parked
`_collision_debug_render` reconstruction should receive another normal
code-producing candidate compile.  It supplements the full experiment history in
`docs/object_matching_logs/collision_debug_obj.md` (commit `cd3798e0`).

The production source remains `source/physics/collision_debug.c`.  At
`9a453f23737698f8cb4d13441a30c4c581223b8b`, the object has one absent function:

- target `_collision_debug_render`: 4,182 meaningful bytes, 4,192 padded bytes,
  278 relocations;
- recovered data: 28,978/29,278 bytes;
- recovered BSS: 28,962/28,962 bytes;
- recovered DATA: 16/16 bytes.

## Prior bounded search

The historical E2-E10 source families already span the plausible legal-C loop,
cache-layout, induction-variable, vector-initialization, and ignore-object
lifetime forms.  The closest legal candidates remain split:

- exact padded size (`0x1060`) with 280 relocations; or
- exact relocation count (278) with a body 16 bytes short.

The divergence begins near function offset `+0x8B` and persists through the
spray loops.  The prior ledger therefore requires either a genuine cross-build
body donor or new local-variable provenance before reopening the compile wave.

## Supplied-repository search

GitHub code search was run against the repositories supplied by the user:

- `surreptitiousresearch/halocea`;
- `stianeklund/halo`;
- `pastudan/halo`;
- `punpckhdq/halo`;
- `Aerocatia/halopc-restored`;
- `Aerocatia/demon`;
- `Zoephie/Baboon`;
- `chadski/nimbus`.

Queries covered `collision_debug_render`, `collision_debug`,
`COLLISION_DEBUG_SPRAY`, `collision_debug_spray_cache`, `debug spray`,
`phantom bsp`, `front_facing_surfaces`, and distinctive target format-string
fragments.

Two relevant source families were returned:

1. `punpckhdq/halo/source/physics/collision_debug.c` at
   `fc47f9a15a0aea7f0081394c4d66db5c68ab5323` contains the known symbol/section
   inventory and an empty public/private-code shell.  It does not contain the
   render body or local-variable topology.
2. `surreptitiousresearch/halocea` at
   `570c83fd9c365dad6f2a3e7041705d5b84c7847c` contains the known
   `collision_debug_*` script-global definitions.  In particular,
   `src/data/collision_debug_spray_definition.c` reconstructs only the 16-byte
   external-global record for `collision_debug_spray`; it does not implement the
   render or spray loops.

No supplied repository returned a render-body donor, spray-cache implementation,
distinctive target format string, or local-lifetime evidence that is new relative
to E2-E10.

## Admission decision

No candidate was compiled and no production source was changed.  Replaying a
previously exhausted legal-C family would violate the frozen-wave rule without
adding evidence.  `collision_debug.obj` remains parked until one of these appears:

- an original or cross-build implementation of the spray body;
- debug symbols/local-variable records for `_collision_debug_render`; or
- another provenance source that resolves the competing loop-counter and cache
  lifetimes responsible for the 16-byte/two-relocation split.

