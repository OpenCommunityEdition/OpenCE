# `rasterizer_xbox_transparent_geometry_group_draw` handoff (2026-09-08)

## Admission status

The Effect-arm owner blocker is resolved. The current source reconstructs the
complete semantic arm from HCEA's DB-derived shader layout, January relocation
evidence, and the authenticated Opus scratch reconstruction. It is the strongest
credible local candidate, but the main function is **fuzzy and receives zero
exact-match credit**:

| image | padded size | relocations | normalized SHA-256 |
| --- | ---: | ---: | --- |
| January target | 15,120 | 872 | `e9d4cea8f7e21b93a338e3c69573f7d35f98ccd28821142030f8030fc075063f` |
| current candidate | 15,072 | 888 | `dd79df2b28b091ec2b9dfb45a12221379548c189587a9edf5e361b7a69bd16e8` |

The isolated gate reports 13 exact, one residual, and eight unwritten functions.
Relative to canonical `build/base`, two ordinary functions are newly exact:

- `rasterizer_transparent_geometry_group_draw__internal`: 384 padded bytes;
- `rasterizer_transparent_geometry_groups_end`: 432 padded bytes.

That is 809 meaningful / 816 padded exact bytes with no exact losses: 384 / 384
for the internal draw helper and 425 / 432 for groups-end. It is below the
campaign's 10,000-byte publication threshold and is admitted only as part of a
larger verified batch.

## Verification

- PID-isolated target gate: passed.
- Forbidden emitted-symbol guard for `_point_from_line3d`: passed.
- Direct-consumer census: 62 unique C translation units include
  `rasterizer.h`, `shader_definitions.h`, or `shaders.h`; seven more consume
  those headers transitively through `render_sprite.h`, `particles.h`, or
  `contrail_definitions.h`.
- All 69 direct and transitive consumers compiled through PID-isolated gates.
- Exact-set comparison against `build/base`: zero lost exact functions in all
  69 consumers.
- The subsequent prototype-owner pass gated its ten direct consumers and again
  found zero exact losses.
- The sweep also observed an unrelated new exact function in the already-dirty
  `effects/decals.c`; it is not credited to this packet.

## Evidence-backed owner changes

`source/shaders/shader_definitions.h` now exposes the fields January reads from
`struct shader_effect_definition` while preserving its `0xB4` size:

| offset | field | type / size |
| ---: | --- | --- |
| `0x28` | `flags` | `unsigned short` |
| `0x2A` | `framebuffer_blend_function` | `short` |
| `0x2C` | `framebuffer_fade_mode` | `short` |
| `0x2E` | `primary_map_flags` | `unsigned short` |
| `0x30` | reserved | 28 bytes |
| `0x4C` | `secondary_map` | `struct tag_reference` |
| `0x5C` | `secondary_map_anchor` | `short` |
| `0x5E` | `secondary_map_flags` | `unsigned short` |
| `0x60` | `secondary_map_animation` | `struct shader_texture_animation` (`0x38`) |
| `0x98` | `secondary_map_radius` | `real` |
| `0x9C` | `secondary_map_zsprite_radius_scale` | `real` |
| `0xA0` | reserved | 20 bytes |

The shared `shader_texture_animation` layout moved without alteration to the
focused owner header `source/shaders/shader_texture_animation.h`. Both
`shaders.h` and `shader_definitions.h` include that owner. Effect enums remain
translation-unit-private because publishing them caused genuine name collisions
with existing private enums in `render_sprite.c` and
`rasterizer_xbox_dynavobgeom.c`.

January and the Halo symbols build independently identify
`rasterizer_globals + 0x3C` as the floating-point z-buffer mode. The accessor
`RASTERIZER_GLOBALS_FLOATING_POINT_ZBUFFER` gives that byte a semantic name,
and a compile-time offset assertion proves `0x3C`. The underlying 13-byte
aggregate remains intact so the original positional initializer and the C2
definition schedule are preserved. Splitting it into fields or introducing a
new nested type was tested and rejected because it either failed the initializer
or regressed an exact function in `rasterizer_xbox_draw_primitives.obj`.

## Effect-arm relocation evidence

The complete candidate now has every Effect-path operation whose absence exposed
the earlier reduced arm:

| relocation target | January | candidate |
| --- | ---: | ---: |
| `IDirect3DDevice8_SetStreamSource` | 1 | 1 |
| `IDirect3DDevice8_SetVertexShaderConstant` | 12 | 12 |
| `SetTextureStageStateSmart` | 81 | 81 |
| `rasterizer_set_texture` | 11 | 11 |
| `shader_texture_animation_evaluate` | 3 | 3 |
| `rasterizer_error` | 12 | 12 |
| `rasterizer_globals + 0x3C` | 1 | 1 |

The DB-authenticated constants used by the arm are nonlinear-tint bit 1,
don't-overdraw-first-person-weapon bit 2, point-sampled bit 0, U-clamp bit 1,
V-clamp bit 2, z-sprite anchor 2, and vertex-constant range `-0x3F` / 2.

## Remaining exact-match boundary

The 48-byte/16-relocation excess is not presented as allocator proof or exact
credit. January has an out-of-line `dot_product3d` call and two
`plane3d_distance_to_point` calls where the current January header schedule
inlines the equivalent helpers. Do not tune this with fake source, and do not
emit a `point_from_line3d` COMDAT. Park the complete source at fuzzy status.

The donor-added cross-TU prototypes no longer live in the caller. Existing
transparent-geometry and active-camouflage declarations were added to their
owner headers. Water, plasma, Chicago preprocessing, and bitmap color conversion
use narrow subsystem headers because adding them to the broad `rasterizer_xbox.h`
or `bitmaps.h` headers measurably regressed exact consumers through C2 definition-
position sensitivity. The narrow-header solution preserves all exact owners.

The January-exact plasma body uses a private `0x70` prefix view of the full
transparent group. Its owner prototype therefore retains that authenticated
prefix type, and the caller makes the representation conversion explicitly.
Changing the exact body to a generic group parameter plus a local alias was
tested and rejected because it changed the function hash.

## Donor census

- Best clean donor: Opus commit `1e50493e305c5ad6587958d40f6759322960c3eb`
  on `opus/30k-rasterizer-widgets-20260905`.
- Authenticated full Effect arm:
  `C:/halo-worktrees/opus-30k-rasterizer-widgets-20260905/scratch/w8-tg-b-effect-arm.c`.
- Older commit `608c66380` and `wave3-rejected` sources remain evidence only;
  they use shadow layouts, raw padding, or stale interfaces and were not admitted.
