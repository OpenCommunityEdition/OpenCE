# `object_lights.obj` small-family recovery (2026-09-02)

## Result

Starting from canonical `783b70ce6`, this packet restores two small, real
Object Lights functions. It adds one strict function and 224 padded bytes with
no strict loss anywhere in the tree:

| Function | Meaningful | Padded | Relocations | Result |
|---|---:|---:|---:|---|
| `_lights_queue_lens_flare` | 221 | 224 | 13 | strict exact |
| `_object_get_self_illumination` | 198 | 208 | 7 | 96.35135%, parked |

The strict board moves from `721,011 / 1,922,413` bytes and `5,041 / 8,245`
functions to `721,235 / 1,922,413` bytes and `5,042 / 8,245` functions.
`object_lights.obj` moves from `18 exact / 5 residual / 20 unwritten` to
`19 exact / 6 residual / 18 unwritten`.

Restoring `object_get_self_illumination` also repairs genuine translation-unit
context for the later private `find_point_lights_for_object_in_cluster`.
Without an artificial source nudge, that helper improves from 576 padded bytes
and 97.430855% to January's 560-byte envelope, all 31 relocation identities,
and 99.3617%. Its parked fingerprint is updated rather than claiming strict
credit.

## Source and ownership

The January split object remains authoritative. The later HCEA implementations
corroborate both routines' semantics: lens-flare submissions pack color, scale,
direction, up, window, and identifiers; self illumination sums the brightness
of attached light datums and recursively visits the first child and next
sibling. The advanced Claude object-lights donor was replayed selectively, not
cherry-picked wholesale.

The retained source uses `real`, semantic attachment constants, the existing
typed `object_get` and `object_definition_get` accessors, and local typed
`light_get`/`lens_flare_definition_get` accessors. The two cross-TU compression
functions are declared in their owner,
`source/rasterizer/rasterizer_geometry.h`; no use-site prototype is added.

The rasterizer-geometry header includer census is five translation units:

- `source/objects/object_lights.c`
- `source/rasterizer/rasterizer_geometry.c`
- `source/rasterizer/rasterizer_transparent_geometry.c`
- `source/rasterizer/rasterizer.c`
- `source/rasterizer/xbox/rasterizer_xbox_hardware_geometry.c`

A full rebuild covered those consumers and their transitive dependents.
Protected `units.obj` remains `189/189`.

## Rejected donor shapes

- The donor's duplicated 240-byte bounding-sphere body inside
  `light_reconnect_to_map` was rejected. HCEA and the current semantic topology
  call the real private helper; retaining a manual duplicate would violate the
  no-fake and avoid-inlining rules.
- `render_debug_light`, the marker helpers, sampling helpers, and the three
  recovered shade helpers were not exposed merely to force section emission.
  HCEA CodeView records identify those as private routines; their real callers
  must be restored first.
- The known if/return respelling of `light_unmarked` was not admitted.
- Three natural declaration/lifetime arrangements for
  `object_get_self_illumination` reduced its residual to two independent setup
  schedules but did not reach strict identity. It is parked instead of using a
  volatile, barrier, assembly, fake dependency, or nonsensical carrier.

## Exact and fuzzy fingerprints

- `_lights_queue_lens_flare`: normalized SHA-256
  `88581c42d53ec754677cd3981598187808cefbd6f7c6cadc606bcffa4a1f35cd`.
- `_object_get_self_illumination` target:
  `f104fb248006b06b69810ac8eb1b9460fba97be1e28353017b9867084897a681`;
  candidate:
  `cfef1bd48ce36ee1ba93ff9b23618e5ea288f0810e834eb9b0e27c8e161bd302`.
- `_find_point_lights_for_object_in_cluster` candidate after restored TU
  context:
  `3c814b365210b7757eb6ba5dee596c15ec8fefe476a335ad299af138e89876a2`.

## Verification

- Focus gate: `19 exact / 6 residual / 18 unwritten`; queue is strict exact.
- Address-keyed whole-tree verdict: exactly one gain, zero regressions.
- Full `ninja`, including report, semantic report, parked verification, and
  progress: pass.
- Combined canonical park validator: `91 active / 0 stale / 0 invalid`.
- Changed-file fake-match scan: zero review leads.
- Test suite: `255 passed` with a workspace-local pytest basetemp.
- `units.obj`: `189 exact / 0 residual / 0 unwritten`.
- COFF owner check: `object_lights.obj` emits no `point_from_line3d`; its
  pre-existing `normalize3d` COMDAT ownership is unchanged. No new forced
  inline directive or math-helper assembly was added.
