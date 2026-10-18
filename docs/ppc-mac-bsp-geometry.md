# Native retail BSP geometry and first G4 terrain render

Follow-up: [collision, spawn and real texture results](ppc-mac-playfield.md)
now supersede this earlier height-color-only diagnostic milestone.

Verified 2026-10-15, using the user's installed version 7 Blood Gulch.
This is a standalone offscreen diagnostic, **not a playable engine build**.

## Native decoding

`port/mac/retail_geometry.c` adds a bounded material visitor over the immutable
retail map view. It validates all BSP geometry before the first callback, then
decodes each material into reusable native scratch arrays:

- 56-byte environment vertices: position, normal, binormal, tangent, UV.
- 20-byte lightmap vertices: incident radiosity vector and UV.
- Three native uint16 indices per triangle, local to that material's vertices.
- Native shader handles, lightmap bitmap indices and counts as metadata.

IEEE float bits are copied from LE words with memcpy; no arithmetic occurs in
conversion. Validation rejects NaNs/infinities and non-unit vector components
outside the existing converter's 1.005 tolerance. Limits match the engine:
131072 BSP surfaces,128 lightmaps,2048 materials/lightmap,20000 surfaces/material,
64000 vertices/material. Cumulative work is also bounded against repeated aliases.
Counts, byte spans, alignment and every triangle index are checked before output.
Lit/unlit vertex counts must agree; lightmap vertex type0 or2 is accepted as in
existing PC validation. Shader handles and bitmap indices are not yet checked
against asset definitions; this is not complete material semantic validation.

Callbacks receive data valid only until return. Raw source and view must remain
immutable, including during callbacks. Allocation/callback failures free scratch
storage. A callback may fail after previous uploads; the consumer must then
release partial uploads. Malformed input itself never triggers partial callbacks.

Layout evidence: `source/structures/structure_bsp_definitions.h`,
`source/structures/structures.h`, `source/render/render.h`,
`source/rasterizer/rasterizer_geometry.c`, and the existing bytewise PC validator
in `port/linux/game/cache_file_formats.c`. Independent research checked offsets,
limits and actual map values, and reviewed the new decoder without finding a
blocking defect.

## Observed results

Apple GCC4.0 on the MacBook, Tiger10.4u SDK, target10.4, optimized PPC/i386 builds:

| Machine | Materials | Environment vertices | Lightmap vertices | Triangles | Decoded digest |
| --- | ---: | ---: | ---: | ---: | --- |
| MacBook Intel control | 79 | 5762 | 5506 | 5503 | 89aa9249 |
| PowerBook G4 | 79 | 5762 | 5506 | 5503 | 89aa9249 |
| iBook G4 | 79 | 5762 | 5506 | 5503 | 89aa9249 |

The report digest operates on native decoded float bits, integer indices and
material metadata as numeric words, so byte order does not change it. It is a
regression fingerprint, not a cryptographic integrity check. A modern-host
ASan/UBSan run matches. The15 existing asset-free retail tests and7 new geometry
tests pass; geometry cases include exact native values/indices, invalid spans,
counts, late NaN/index rejection with zero callbacks, and callback failure.

The staged MacBook map SHA-256 is
`3e75dd4c505f509d794e7fc2f150bfc02fb0703c117001249f854355e3ab73aa`.
The PowerBook reads its installed file directly. The iBook uses a staged copy
and source-machine resource-size metadata, as documented in the retail report.
No Halo installation files changed.

## Offscreen drawing

`tools/mac_bsp_render.c` feeds those decoded vertices and indices to legacy
OpenGL in an accelerated CGL context, with a512x512 color/depth FBO. No drawable
or window is attached and the desktop is untouched. It uses an orthographic
view from above, fits each horizontal axis to the output, and assigns synthetic
height colors. It does not render textures, models, lightmaps or engine shaders.
PPC compilation disables fused multiply-add. This diagnostic is not the planned
production renderer and does not establish performance or minimum-spec support.

Observed renderer strings and nonzero RGB channel counts:

| Machine | Renderer | Nonzero channels | GL errors |
| --- | --- | ---: | ---: |
| MacBook | NVIDIA GeForce9400M OpenGL Engine | 410895 | 0 |
| PowerBook | ATI Radeon9600XT OpenGL Engine | 410868 | 0 |
| iBook | ATI Radeon9550 OpenGL Engine | 410868 | 0 |

The PowerBook renderer string differs from its reported Mobility Radeon9700
hardware identity; this is recorded in the hardware inventory. Different GPU
pixel counts are not a geometry mismatch: native geometry digests agree. We do
not assert pixel-identical rendering across GPU vendors.

The retrieved PowerBook image was visually inspected: canyon outline, terrain
and both bases are visible. Local output:
`build/mac-audit/bloodgulch-powerbook.png` (lossless PPM-to-PNG conversion).
Original PPM SHA-256:
`481c99cc70fe214d1303dafed110b116cf4a4f90ddbdf0f95595f9435e60278d`.

Reproduce using `sh tools/mac_cache_checks.sh` on the MacBook, then run
`bsp_render_ppc /path/bloodgulch.map output.ppm` on a G4. The report executable
also now invokes the geometry decoder. Logs:
`build/mac-audit/geometry-build.txt`, `powerbook-geometry.txt`, `ibook-geometry.txt`.
Remote probes remain under `$PROBES/20261015`; the MacBook build
stage is its `cache-endian` subdirectory.

## Next work

Proceed to collision BSP and scenario placement decoding, then bitmap/lightmap
resource uploads and native engine cache integration. Model geometry, remaining
tag classes, retail script mapping, platform/linkage/TLS and input/audio remain.
The complete first-map criterion still requires real movement, collision,
weapons, HUD, textures and audio in an engine session, not this drawing test.
No existing engine or Android/web build units changed in this iteration.
