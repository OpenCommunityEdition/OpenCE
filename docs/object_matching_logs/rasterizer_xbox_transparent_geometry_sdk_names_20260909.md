# Transparent-geometry XDK private-owner naming, 2026-09-09

## Result

Eight existing, byte-identical Xbox SDK bodies in
`rasterizer_xbox_transparent_geometry.obj` are now bound to their real SDK
names and private linkage. No C or header source changed. The production
semantic report gains **eight accepted exact functions, 1,161 meaningful code
bytes, and 1,216 padded bytes**:

| January owner | Meaningful | Padded | Relocations |
| --- | ---: | ---: | ---: |
| `_D3DDevice_SetRenderState` | 422 | 432 | 32 |
| `_D3DDevice_SetTextureStageState` | 65 | 80 | 5 |
| `_IDirect3DDevice8_SetRenderState@12` | 543 | 544 | 32 |
| `_IDirect3DDevice8_SetTextureStageState@16` | 85 | 96 | 5 |
| `_IDirect3DDevice8_SetVertexShaderConstant@16` | 13 | 16 | 1 |
| `_IDirect3DDevice8_SetStreamSource@16` | 13 | 16 | 1 |
| `_IDirect3DDevice8_EndVisibilityTest@8` | 9 | 16 | 1 |
| `_IDirect3DDevice8_GetVisibilityTestResult@16` | 11 | 16 | 1 |

The focused gate advances from 13 to 21 strict functions while the sole
15,120-byte Halo residual remains a residual and receives zero credit.

## Identity and ownership evidence

`tools/campaign/namegap.py` found a unique strict pairing for every row: equal
padded bytes, normalized code, and complete ordered relocation identity. The
target records are at January file offsets `0x163B40`, `0x163CF0`, `0x163EE0`,
`0x164100`, `0x164160`, `0x164170`, `0x164190`, and `0x1641A0`.

The names and storage class come from the bundled authentic XDK header,
`xbox/include/D3D8-Xbox.h`. It defines `D3DINLINE` as
`static __forceinline` at line 20, defines the two device-state dispatchers at
lines 1497 and 1631, and defines the six `IDirect3DDevice8_*` wrappers at lines
1708, 1724, 1739, 1751, 1784, and 1785. Each candidate function is the natural
private owner emitted by real renderer calls in
`source/rasterizer/xbox/rasterizer_xbox_transparent_geometry.c`; no call or
retention anchor was added to materialize it.

The symbol-map records use `"static": true` because the source macro, not the
splitter's former default placeholder metadata, owns linkage. This is the same
SDK/private-owner rule already independently admitted for Xbox and screen-
effect copies in
`rasterizer_xbox_sdk_names_and_private_reciprocal_20260906.md`.

Two superficially adjacent 16-byte CubeTexture unlock proposals remain
unchanged. They have no relocations and their empty bodies are not uniquely
authenticated by a call graph; byte equality alone cannot name them.

## Verification

- full Ninja source build and semantic report: pass;
- focused gate: 21 exact / 1 residual / 0 unwritten;
- `_point_from_line3d` emitted-symbol guard: pass;
- frozen published-baseline comparison after this packet: 13 accepted gains,
  3,609 meaningful / 3,712 padded bytes cumulatively, zero losses;
- production source/header delta for this packet: none.

No whole-object, data-owner, or final-link completion claim is made.
