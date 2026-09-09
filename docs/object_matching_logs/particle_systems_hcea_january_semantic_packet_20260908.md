# `particle_systems.obj` HCEA/January semantic packet (2026-09-08)

## Admission verdict

This packet is source-credible and consumer-safe, but it is not a whole-object
closure.  Only strict COFF equality receives exact-match credit.

- New strict closures: `particle_system_new_particle_explosion`, 262 meaningful
  / 272 padded bytes, and `particle_system_initialize`, 278 meaningful / 288
  padded bytes.  The latter changed from residual to strict exact when the
  authenticated private particle call identities were restored.
- Three bodies that were already strict now have authenticated semantic names:
  `particle_system_update_default` (97 / 112),
  `particle_system_new_particle_default` (60 / 64), and
  `particle_system_update_explosion` (37 / 48).  Counting semantic attribution,
  the packet exposes 456 meaningful / 496 padded exact bytes in those four
  semantically attributed bodies.  Including the newly strict initializer,
  the actual new strict delta is 540 meaningful / 560 padded bytes.
- `particle_system_new_particles` is retained as the best complete credible
  fuzzy body: 843 meaningful / 848 padded bytes and 35 relocations on both
  sides, but different normalized code hashes.  It receives zero exact credit.
- `particle_system_render` remains unwritten.  The later HCEA reconstruction
  contains target-specific rendering types and an unresolved raw `-184`
  sentinel; importing it would violate the no-nonsense/no-fake-source rule.

## Evidence and semantics

The named tag/runtime fields and enum domains come from the authenticated HCEA
fragments under
`research-cache/halocea-full-blobs-20260830/src/blam/effects`, the matching
HCEA headers, the Halo CE exact symbol atlas, and January COFF relocation and
instruction evidence.  The emitter body deliberately omits the later-build
first-person/render-throttling arms because January has no corresponding
player, first-person-weapon, or render-global relocations.

The creation selector uses the DB-authenticated
`particle_system_type_create` enum.  Typed tag access goes through
`particle_system_definition_get`/`TAG_BLOCK_GET_ELEMENT`; typed object and
particle access goes through `object_get`, `particle_system_get`, and
`ps_particle_get`.  There are no raw structure offsets in the reconstructed
source.

`struct object_marker` was already correctly owned by `objects.h` and required
no edit.  Its established layout is 0x6C bytes: `node_index` at 0x00,
`node_matrix` at 0x04, and `matrix` at 0x38.  January independently proves the
layout in `particle_system_new_particles`: marker selection strides by 0x6C,
the unattached forward vector is written at marker+0x3C, and the position is
written/read at marker+0x60.  The exact explosion closure independently reads
`marker.matrix.position` through that same owner layout.

## Strict measurements

Final isolated candidate:
`scratch/particle_packet_final_20260908.obj`

- target object SHA-256:
  `f1aa744aa00fafb545996305e2d555a5011b86c702d276f4ded9b24323b35771`
- candidate object SHA-256:
  `964ab6d74a5f6f10900f5e3e4e2a5c4fc824ec026304c49cdc0aac09ba595cec`
- `particle_system_new_particle_explosion`: 272 bytes, 9 relocations,
  target/candidate normalized SHA-256
  `98a73c35d6c50bf0e149e8578ad961a43819188de11233d4efb13ad7fe702f28`
- `particle_system_new_particles`: 848 bytes, 35 relocations; target normalized
  SHA-256
  `42b053f34b78452c8c6fa4b8b0c6aabbbcf6750abce7856556240f3637e01e27`,
  candidate normalized SHA-256
  `c29055f1631f8ebed715384f8f4f5282248bb985b13b347387a6f8488546d907`

The final particle gate reports 15 exact, 9 residual, and 1 unwritten owner.
All inherited exact owners remain exact.  The forbidden-emission guard confirms
that `_point_from_line3d` is not emitted.

## Header-consumer sweep

Direct/transitive particle-header consumers were enumerated before admission:

| unit | final isolated verdict | exact losses versus `build/base` |
|---|---:|---:|
| `source/game/game` | 24 exact, 3 residual | 0 |
| `source/effects/effects` | 31 exact, 10 residual | 0 |
| `source/scenario/scenario` | 46 exact, 0 residual | 0 |
| `source/objects/objects` | 121 exact, 0 residual | 0 |

Every consumer gate passed the `_point_from_line3d` forbidden-symbol guard.
The two apparent `game` gains versus the current `build/base` are outside this
packet and are not credited here.  `git diff --check` passes for all owned
source/header/config paths.

## Parked boundary

Do not claim exact credit for any residual body.  Reopen
`particle_system_new_particles` only with genuine source/compiler-owner
evidence for the remaining register allocation, x87 store scheduling, and
private dispatch-table identity.  Do not replace semantic source with
declaration-order steering.  Reopen `particle_system_render` only after the
January bitmap/shader/build-sprite owner types are authenticated; the later
HCEA raw sentinel is not acceptable source evidence by itself.
