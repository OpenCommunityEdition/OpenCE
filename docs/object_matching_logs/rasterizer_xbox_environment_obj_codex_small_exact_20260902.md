# `rasterizer_xbox_environment.obj` small-function recovery (2026-09-02)

## Frozen baseline and evidence

The isolated lane is based on canonical commit
`88d0bd31eb25f74c9f7064c4e5fcb941ee8f52c1`. The baseline source blob was
`35596c607b774e1a5abd9dfe49408b5dbe611b0f`; the retained source blob is
`6b39ceedb8221adc876d12cbd019f28f31a9c19e`.

January's split object, hardened COFF comparison, and aligned disassembly are
the byte authority. Semantic reconstruction was cross-checked against the
local HaloCEA function/header corpus, the Xbox D3D8 SDK headers, the campaign
matching methodology, and the throughput playbook. The XDK bodies were not
hand-written: only the one naturally emitted `D3DDevice_SetRenderState`
COMDAT is credited.

The frozen baseline was 19 exact, 0 residual, and 25 unwritten functions:
784 padded exact code bytes and 53 exact relocations. Its 25 unwritten
functions represented 20,480 padded bytes.

## Retained exact recovery

The isolated final gate was 32 exact, 3 residual, and 9 unwritten functions.
Canonical admission corrected the 0xB5 environment block from an unresolved
extern to its target-proven TU-private definition; that naturally closed
`_rasterizer_environment_lightmap_begin`. The admitted result is **33 exact,
2 residual, and 9 unwritten**, adding **14 strict functions and 8,704 padded
code bytes**. Every inherited exact function remains exact.

| January address | Function | Padded | Meaningful | Relocations |
| --- | --- | ---: | ---: | ---: |
| `0x14FCC0` | `D3DDevice_SetRenderState` | 432 | 422 | 32 |
| `0x150200` | `_rasterizer_environment_reflection_lightmap_mask_draw` | 512 | 504 | 34 |
| `0x150480` | `_rasterizer_environment_lightmaps_begin` | 400 | 388 | 29 |
| `0x150610` | `_rasterizer_environment_lightmap_begin` | 400 | — | 30 |
| `0x151750` | `_rasterizer_environment_diffuse_lights_begin` | 704 | 704 | 63 |
| `0x151DB0` | `_rasterizer_environment_diffuse_light_draw` | 560 | 545 | 39 |
| `0x151FE0` | `_rasterizer_environment_diffuse_textures_begin` | 400 | 394 | 32 |
| `0x1527E0` | `_rasterizer_environment_specular_lights_begin` | 576 | 569 | 45 |
| `0x152DE0` | `_rasterizer_environment_specular_light_begin` | 896 | 887 | 70 |
| `0x153160` | `_rasterizer_environment_specular_light_draw` | 816 | 813 | 66 |
| `0x153490` | `_rasterizer_environment_specular_lightmaps_begin` | 928 | 915 | 81 |
| `0x153920` | `_rasterizer_environment_specular_lightmap_draw` | 784 | 777 | 59 |
| `0x153C30` | `_rasterizer_environment_reflection_lightmap_masks_begin` | 432 | 423 | 42 |
| `0x154C70` | `_rasterizer_environment_transparent_geometry_submit` | 864 | 852 | 57 |

The target object after evidence-backed symbol naming has SHA-256
`486EF4D6629757F498CBE5B3C7DFCBB75B76632AD4D8383ECDE09077619EA416`;
the retained scratch candidate has SHA-256
`54A504FD9986CB81302816396D3475220CA2102156C7ADD049C32AADA3D028C2`.
The object contributes no newly exact data section; the data work is semantic
naming of the 2-byte specular permutation index and the 0xB5-byte environment
global block. Canonical admission confirmed that the latter is owned privately
by this translation unit, so both its symbol metadata and C definition are
`static` rather than an unresolved cross-TU declaration.

## Honest fuzzy parks and remaining work

Two coherent, evidence-backed bodies are retained as fuzzy residuals and are
not credited as exact:

- `0x151A10`, `_rasterizer_environment_diffuse_light_begin`: both are 928
  padded / 914 meaningful bytes with 58 relocations and equivalent control
  flow; C2 assigns three local basis vectors to different stack slots.
- `0x152A20`, `rasterizer_environment_specular_spot_light_begin`: the HCEA-
  backed vector/cross-product implementation is 944 padded / 940 meaningful
  bytes versus January's 960 / 949, with the same 64 relocations. No dummy
  locals, scheduling barriers, or fake arithmetic were admitted.

The remaining 9 unwritten functions total 9,888 padded bytes: the four large
environment draw bodies (9,136 bytes) and five stock XDK inline wrappers (752
bytes). These are parked for later evidence-led recovery rather than fabricated
for symbol emission.

## House-rule and regression gates

- Private/global/data identities are semantic. The XDK owners are named from
  `D3D8.h`; no `code_<address>`, `data_<address>`, or `bss_<address>` name is
  introduced by this object work.
- Cross-TU declarations added by this recovery live in narrow owner headers
  for transparent geometry, frame statistics, and the memory pool, or in the
  closest Xbox rasterizer/HUD header. The public transparent-submit wrapper
  now forwards all 12 January/HCEA parameters and remains a strict 16-byte
  exact thunk.
- Project `real`, `boolean`, color/vector/plane types, flag macros, semantic
  enums, multiline signatures, and explicit returns are used. There is no
  inline assembly, volatile scheduling device, raw-byte emission, UB cast, or
  admission exception.
- Object inventory confirms no `point_from_line3d` COMDAT. The plausible spot
  helper uses the existing math helpers and retains the January inline policy.
- Targeted Ninja compiled the seven directly changed translation units and the
  escaped-space `saved games/game_state.obj`. Gate compilation covered all 7
  HUD-header includers and 24 of 25 rasterizer-header includers; the remaining
  escaped-space includer was covered by that targeted Ninja build. The
  environment, transparent geometry, frame statistics, memory pool, HUD draw,
  Xbox rasterizer, and rasterizer-wrapper exact sets retain their prior exact
  functions.
- `fake_match_scan.py --fail-on-findings` reports 0 findings across all 12
  touched C/header files. The focused comparator/regression/fake/parked/
  admission test selection passes 103/103. `git diff --check` passes.

A shared/full Ninja build was intentionally not run from this isolated lane.
