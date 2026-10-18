# Blood Gulch collision, starts and textures

Verified 2026-10-15 on the PowerBook G4 and iBook G4, using MacBook-built native
PPC executables. Intel MacBook results agree. This is still standalone port
infrastructure and offscreen rendering, **not a playable engine session**.

## Collision and player starts

`port/mac/retail_collision.c` decodes all eight collision arrays into owned
native structures. It checks file spans, counts, finite floats, references,
edge/vertex indices and directed surface-ring closure before exposing data.
Point-to-leaf classification follows the engine's `bsp3d_test_point` arithmetic,
with bounded traversal. PPC probes disable fused multiply-add. The static
vertical grounding query is explicitly diagnostic: upward-facing convex surface
polygons, excluding breakable faces. It does not implement biped/capsule physics,
step-up, wall response, moving objects or damage/broken-surface state.

`port/mac/retail_spawns.c` decodes all starting locations atomically, preserving
position, facing, team, BSP and four game-type selectors. Facing is radians
around +Z. Mode matching follows the engine's wildcard semantics; it does not
invent team restrictions or replace engine spawn selection/rating. The actual
scenario header layout witness now checks the players block at 0x354; its
included source header already asserts the 0x34 starting-location record size.

Observed on **all three Macs**:

| Check | Result |
| --- | --- |
| Collision array counts, in on-disk order | 12159 / 4666 / 5971 / 13221 / 516 / 5098 / 8068 / 2830 |
| Native collision digest | `4cf01d47` |
| 32x32 diagnostic ground grid | 611 hits; all 611 above-ground samples in open leaves |
| Starting locations | 72, all BSP 0, all classify as open leaves |
| Ground beneath starts within z−5 to z+0.5 | 72 hits; all 72 above-ground samples open |
| Start-to-ground vertical gap | 0.000495695 to 0.050686568; largest at start 38 |
| Mode matching | CTF 38; Slayer/Oddball/King 17 each; Race 16; engine inactive 1 |
| Regression checks | Spawn 913/0 failures; collision 39/0 failures |

No ground result establishes capsule clearance, safe standing height or a valid
engine-selected spawn. The gap test deliberately reports misses/solid results
rather than assuming every editor start must satisfy a new policy. Starting
locations have not been used to instantiate a real engine player yet.

## Texture decoding and rendering

`port/mac/retail_texture.c` provides bounded 2D base-level bitmap lookup and
explicit byte-output RGBA8 decoding for RGB565, XRGB/ARGB8888 and DXT1/3/5. It
never uploads native packed integers as pixels. Unaligned resource offsets are
supported. Swizzled, paletted, volume and cube data are rejected; dimensions are
currently limited to 2048. Source/output buffers must not overlap. This stage
only decodes mip level zero; it does not validate or upload the rest of a mip
chain. Bitmap lookup checks the declared base level fits its stored payload;
callers must also validate/read that range against the actual source file.

The diagnostic shader selector handles environment base maps and single-map
Chicago shaders with identity UV transforms. Actual terrain uses DXT1/3/5;
its 15 lightmap images use RGB565. Existing structure UVs are decoded natively.

`tools/mac_bsp_render.c` optionally accepts `bitmaps.map` and uploads RGBA bytes
through legacy OpenGL. It combines base color and lightmap with two texture
units. It still creates no window/drawable and touches no desktop state.
It is not a faithful engine material renderer: no full detail/bump/specular,
shader blending/animation, overbright policy or mipmapping. Two teleporter
materials require extended multi-stage shaders; they are counted and marked
magenta instead of being presented as complete. In the top-down image the
small unsupported surfaces may be occluded.

Actual results:

| Machine | Loaded texture images | Decoded RGBA digest | Unsupported materials | GL errors |
| --- | ---: | --- | ---: | ---: |
| MacBook Intel | 30 | `ad223d21` | 2 | 0 |
| PowerBook G4 | 30 | `ad223d21` | 2 | 0 |
| iBook G4 | 30 | `ad223d21` | 2 | 0 |

The texture digest covers decoded RGBA bytes in load order, not driver pixels.
It matches across byte orders. Renderer output differs slightly between ATI
and NVIDIA, as expected; this is not a pixel-exact comparison or benchmark.
The PowerBook image was retrieved and visually inspected: terrain texture,
paths, cliffs and base roofs are present, with baked lighting visible.

Image: `build/mac-audit/bloodgulch-textured-powerbook.png` (lossless conversion of
its offscreen PPM). The MacBook/PowerBook used their installed game resources.
The iBook now uses a **full 317199907-byte copy of bitmaps.map** staged at
`$PROBES/20261015/bitmaps-v7.map`; unlike the earlier metadata-only
tests, this run reads real bitmap payload bytes. Its map remains the staged
MacBook copy. No files in the original installations were changed.

## Verification and reproduction

Three subagents independently implemented/researched collision, spawn semantics
and texture layouts/tests. Root integrated, reviewed and ran hardware checks.
The 276 texture checks include independently specified RGB565/BGRA values,
DXT1 transparency/interpolation, DXT3 alpha, both DXT5 alpha modes, cross-byte
selectors, edge blocks, unaligned input, output guards and malformed lengths.
Spawn/collision synthetic tests cover bounds and invalid data; real-map results
above are additional evidence, not synthetic fixtures.

Host sanitizer runs for collision/spawns and texture decoding are clean.
The existing 15 retail-reader and 7 geometry regression cases still pass.
All new standalone probes compile on MacBook GCC 4.0 with strict warnings and
Tiger 10.4u SDK, target 10.4. Actual G4 OS remains the modified Leopard build
recorded in `ppc-mac-hardware.md`; no new Tiger runtime claim is made.

Build from staged repository root on MacBook:

```
sh tools/mac_playfield_checks.sh /path/bloodgulch.map
sh tools/mac_cache_checks.sh
```

Run `spawn_ppc map`, `collision_ppc map`, `texture_ppc`, and optionally
`bsp_render_ppc map output.ppm bitmaps.map` on the G4s. Executables are in
`build/mac-audit` on the MacBook stage and `$PROBES/20261015` on G4s.
The MacBook stage is `$PROBES/20261015/cache-endian`.

Logs: `playfield-build.txt`, `textures-build.txt`, `powerbook-playfield.txt`,
`ibook-playfield.txt`, `ibook-textures.txt`, and host sanitizer logs, all under
ignored `build/mac-audit`. No commercial data is tracked.

## Next priority toward a session

Connect these native decoders to an explicit native-cache/accessor boundary and
bring up the actual engine player/object update path. Audit remaining reachable
tag classes and retail script numbering before native engine casts. Preserve
engine collision/physics and maths rather than substituting the diagnostic
vertical-ground helper. Then supply platform input/audio and the engine renderer
path; complete teleporter/material effects separately. Full game build/linkage,
TLS, object/model/HUD/audio support remain blockers. Neither the collision probe
nor the textured map image constitutes a playable session.

This iteration changes standalone Mac modules/probes only, not existing engine
units, protocol, or Android/web build graphs.
