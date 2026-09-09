# `rasterizer_xbox_environment.obj`: Opus reflection reconciliation (2026-09-09)

## Result

This packet admits the two environment-reflection draw owners reconstructed in
the completed Opus rasterizer lane, plus the stock XDK render-state wrapper that
their calls naturally instantiate.  Against published canonical
`b74f7a2d7dcf7dec0e3f6ba5b7771bc92832a53f`, the focused PID-safe gate advances
`source/rasterizer/xbox/rasterizer_xbox_environment` from **37/44 to 40/44
strict-exact function owners** with no inherited exact loss.

| Newly exact owner | Meaningful | Padded | Relocations | Normalized SHA-256 prefix |
| --- | ---: | ---: | ---: | --- |
| `_IDirect3DDevice8_SetRenderState@12` | 543 | 544 | 32 | `d391ce622a8d1be40209833f735c268b440f24a07bc06da8e1b306fc82ad8a0e` |
| `__rasterizer_environment_reflection_mirror_draw` | 1,631 | 1,632 | 122 | `60ef77b39b5ed8a9` |
| `__rasterizer_environment_reflection_draw` | 1,836 | 1,840 | 133 | `505d2e810cd86f80` |
| **total** | **4,010** | **4,016** | | |

Meaningful sizes are the January report sizes.  Padded sizes, relocation counts
and hash prefixes come from the January COFF owners.  The abbreviated hashes are
the bounded `tools/campaign/tinfo.py` display values.  This packet claims no
data/BSS credit and does not mark the object complete.

## Donor and selective admission

- donor branch: `opus/30k-rasterizer-widgets-20260905`;
- donor tip: `bd8c008e85a1543877d0843bab3f1c2dffab9a13`;
- donor worktree at intake:
  `C:/halo-worktrees/opus-30k-rasterizer-widgets-20260905`;
- donor campaign ledger:
  `docs/object_matching_logs/opus_30k_rasterizer_widgets_wave3_4_20260905.md`.

Only the two coherent reflection bodies, their required semantic enum/record
members, and the naturally emitted wrapper were replayed.  None of the donor's
rejected lightmap inline-suppression experiment or hand-bitwise source was
transferred.

The initial selective compile exposed a useful ownership dependency: without
`effects/decals.h`, VC7 sees `real_a_rgb_color_to_pixel32` only through an
implicit declaration and promotes each `real` alpha argument to `double`.
That made both draw owners 16 bytes too long.  Including the genuine declaration
owner removes those conversions and makes both complete functions strict-exact;
this is declaration correctness, not translation-unit steering.

## Source and ownership review

- Reflection flags/types and the primary target use semantic enum constants.
- The implementation uses project `real` types, `TEST_FLAG`, and `PIN`; it adds
  no raw object/tag access, assembly, forced inline, `/QIfist`, address-derived
  name, or artificial compiler-state source.
- The renderer uses the shared `struct rasterizer_window_begin_parameters`
  definition and its named `camera.position`/`camera.forward` fields.  No new
  local window-layout facade is retained.
- The TU-private reflection block carries the semantic mirror-index,
  mirror-depth, cube-map, and unused fields recorded by the local HCEA
  reconstruction reference instead of a raw `byte[0x28]` offset carrier.  HCEA
  is used here for field semantics only; January COFF remains the byte oracle.
- The previously unnamed `0x0C` frame-statistics span is assigned the coherent
  reflection vertex, triangle, and draw counters in the statistics owner header.
- The mirror/reflection backend declarations were removed from the consumer
  `rasterizer.c` and placed, with the adjacent begin/end family, in the narrow
  Xbox backend owner header `rasterizer_xbox_internal.h`.  Each parameter is on
  its own line and every no-argument declaration uses the project `(void)` form.
- Both implemented `void` functions terminate with explicit `return;`.  Their
  control flow is ordinary render-state setup, texture binding, shader constant
  construction, draw submission, and optional statistics accounting; no
  semantically implausible exact-match construct was admitted.

No fuzzy body is added by this packet, so its zero-credit fuzzy contribution is
**zero**.  The two pre-existing residual owners and two pre-existing unwritten
owners remain uncredited:

- residual: `__rasterizer_environment_diffuse_light_begin` and
  `_rasterizer_environment_specular_spot_light_begin`;
- unwritten: `_IDirect3DDevice8_SetTexture@12` and
  `__rasterizer_environment_lightmap_draw`.

## Focused verification

The worker used only PID-named `gate.py` scratch objects; it did not configure,
run Ninja, or write the shared build tree.  The owner gate reported 40 exact,
two residual, and two unwritten owners, and explicitly passed the forbidden
`_point_from_line3d` emitted-symbol guard.  The same guard and focused compile
also passed for every direct consumer of the changed narrow backend header:

- `source/rasterizer/rasterizer`: 131/131 exact;
- `source/rasterizer/xbox/rasterizer_xbox`: inherited 72 exact preserved;
- `source/rasterizer/xbox/rasterizer_xbox_active_camouflage`: inherited 10
  exact preserved;
- `source/rasterizer/xbox/rasterizer_xbox_transparent_geometry`: inherited 21
  exact preserved.

All twelve direct C consumers of `rasterizer_frame_statistics.h` were compiled
through the same PID-safe guard as well: `rasterizer_debug`,
`rasterizer_frame_statistics`, `rasterizer_lights`, `rasterizer_xbox`, decals,
draw primitives, dynavobgeom, environment, environment fog, models, shadows,
and vertex-shader runtime.  Each compiled successfully and retained its current
January-exact owners; the orchestrator's stable whole-tree sweep remains the
publication authority.

Commands:

```text
python tools/campaign/gate.py source/rasterizer/xbox/rasterizer_xbox_environment --all --forbid-emitted-symbol _point_from_line3d --out scratch/environment-shared-window.obj
python tools/campaign/gate.py source/rasterizer/rasterizer --all --forbid-emitted-symbol _point_from_line3d --out scratch/rasterizer-consumer.obj
python tools/campaign/gate.py source/rasterizer/xbox/rasterizer_xbox --all --forbid-emitted-symbol _point_from_line3d --out scratch/rasterizer-xbox-consumer.obj
python tools/campaign/gate.py source/rasterizer/xbox/rasterizer_xbox_active_camouflage --all --forbid-emitted-symbol _point_from_line3d --out scratch/active-camo-consumer.obj
python tools/campaign/gate.py source/rasterizer/xbox/rasterizer_xbox_transparent_geometry --all --forbid-emitted-symbol _point_from_line3d --out scratch/transparent-consumer.obj
git diff --check
```

The orchestrator still owes the campaign-wide Ninja build, semantic report,
stable-owner sweep, park validation, and test suite before publication.
