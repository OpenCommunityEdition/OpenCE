# `progress_bar.obj` Jonas SDK texture-state leaf ledger (2026-08-28)

## Scope and authority

- Integration base: `734dd02e1fdb650169171048cc5e00955cb509af`.
- Target owner object: `build/split/source/interface/progress_bar.obj` from the January x86 split, SHA-256
  `3926fd05ff81cae4ec93dd9a21328559b9edb309f7e61bc3e36a1f518a421911`.
- Candidate owner object: `build/base/source/interface/progress_bar.obj`, replay SHA-256
  `8af1622105121cd71376d76c66cf70cb8c78b6354d2e6bf365a972588e552345`.
- The target-owner COFF comparison is authoritative. Source similarity, same-size code, and
  executable-address comparisons are not acceptance gates.
- This was one bounded, natural typed-C batch. It did not touch Units, Vehicles, or any
  Claude-active lane.

## Donor and tooling audit

The following evidence was read before the batch:

- `docs/object_matching_logs/progress_bar_obj_jonas_d3dx_identity_first_shot_20260827.md`.
- Pinned XDK `xbox/include/D3D8-Xbox.h`, SHA-256
  `05488392a186c2ccad9b80946ff3f07378e27a9492fe68f682b506d9ae645a39`.
  Its `D3DDevice_SetTextureStageState` inline is the source-authentic donor for the retained
  body. It also identifies the small anonymous D3D compatibility/member thunks below.
- Pinned XDK `xbox/include/D3DX8Math.Inl`, SHA-256
  `6f118d38e87da13dc1998a2b7b0e560505e41a39ed8087b1e878e6f0d710785b`.
  It confirms the already-retained `_D3DXMatrixIdentity`; it does not supply another eligible
  leaf in this wave.
- `research-cache/halocea-cseries-20260820` at
  `570c83fd9c365dad6f2a3e7041705d5b84c7847c`, plus
  `research-cache/halocea-following-camera-20260813`. Their progress-bar files are the same
  later PPC/no-op reconstruction family (for example, both `progress_bar_eachframe.c` copies
  hash to `28d4ad9a78a8bf2c83881a2908d82ffae2178c864b484a9ef060d69a5d10ae23`).
  They are semantic context, not January x86 body donors. HCEA's texture-stage smart helper is
  empty because the Xbox 360 port does not use this fixed-function path.
- `research-cache/pastudan-halo-full-history-20260828/src/halo/interface/progress_bar.c`,
  SHA-256 `ec188a5879bfdd654ff41fa2b50c0d5d7f2b3c4cbe8b32d83822c42a4e138a2a`.
  It supplies useful semantic names for the broad render/texture lifecycle, but its bodies use
  raw addresses and reconstructed storage and therefore were not imported.
- `tools/progress_bar_batch_fragment.c` and `tools/progress_bar_batch_impl.c` in that Pastudan
  cache are identical, SHA-256
  `24079836bb1e50e2d842b3c269b0ecb78f9b2b8d558f78ca10d634fb9d354818`.
  Their wrappers are empty; they add mapping evidence only.

## One-shot candidate and frozen gate

The sole candidate was the XDK decision tree for:

```c
void SetTextureStageStateSmart(
    unsigned long stage,
    enum d3d_texture_stage_state_type type,
    unsigned long value);
```

The source keeps the SDK's signed enum comparisons and its five natural calls:

1. `type < 22` -> `D3DDevice_SetTextureState_Deferred` (SDK `__fastcall` ABI),
2. `type == 28` -> `D3DDevice_SetTextureState_TexCoordIndex`,
3. `type == 29` -> `D3DDevice_SetTextureState_BorderColor`,
4. `type == 30` -> `D3DDevice_SetTextureState_ColorKeyColor`, and
5. `22 <= type <= 27` -> `D3DDevice_SetTextureState_BumpEnv`.

The first strict owner gate was exact and was frozen immediately:

| Owner | Size | Relocs | Normalized SHA-256 | Result |
|---|---:|---:|---|---|
| `_SetTextureStageStateSmart` | 112 | 5 | `8fed83c112c7fb6ac31b8691825366412a83d0ed4925fc94213da5ade8bcf152` | exact |

Relocation offsets and targets are exact: `0x13` fastcall deferred, then `0x27`, `0x3b`,
`0x4f`, and `0x64` for TexCoordIndex, BorderColor, ColorKeyColor, and BumpEnv. A clean
retained-only rebuild reproduced the same size, relocation topology, normalized hash, and all
previously exact owner bodies.

No candidate body was rejected, so there was no rejected production code to retain or prune.

## Exact-state ledger after replay

The object advances from 16/54 to **17/54 exact owners**. Exact owner footprints total
`1264/7696` bytes. On the prior ledger's instruction-byte measure, the new body contributes
106 meaningful bytes (six terminal alignment bytes are excluded), advancing meaningful exact
coverage from `1011/7280` to **`1117/7280`**.

| Exact owner | Size | Relocs | Normalized SHA-256 |
|---|---:|---:|---|
| `_D3DXMatrixIdentity` | 64 | 0 | `4aa6805bb2566575ffd5d1210121322f1d9726c633ae21d0aa7546d4c0808d0b` |
| `_SetTextureStageStateSmart` | 112 | 5 | `8fed83c112c7fb6ac31b8691825366412a83d0ed4925fc94213da5ade8bcf152` |
| `_code_000d1150` | 16 | 1 | `908f13344e4829738bdc6459d5167bd7f19dd39e321ff34cc87567783e90c858` |
| `_code_000d16d0` | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_gen_cloud_coord` | 48 | 3 | `15e8604a4644ddffecfb85a4f29e370a62a51c2135b9e82ba5ad8c575dff5a7b` |
| `_gen_mask_coord` | 64 | 5 | `7dab11a8cc01d02e5983d769beaf518cd8673bbf99d3c98bea576bbd739fd7d1` |
| `_progress_bar_begin` | 48 | 3 | `13efcedf2bf2e45a6840c55ec542a4d1571349d8804ebcb39c98c90cf2223c56` |
| `_progress_bar_display` | 160 | 16 | `4f72be1168ff6a09bb6d3300ea5a3105333d2c599a9c2a5ddaf46ec137ff9dee` |
| `_progress_bar_dispose` | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_progress_bar_enable` | 16 | 1 | `de8c4439b030245988663f8f08b89f6a3ab735129d5467f31f6d3f0bde83c561` |
| `_progress_bar_end` | 32 | 2 | `8590b60019609c2fe07d9ba05ee7d93864878d570b9022f48850d5712ca3989a` |
| `_progress_bar_initialize` | 144 | 11 | `6290f19c9af4ed71701dcdb37f8cba1b4507c6227709d0c80a1a2b19b63449b4` |
| `_progress_bar_is_active` | 16 | 1 | `bc02314523356da167b512d65b2d9c87044e51ae794cfb44b87d8a5a18b05e61` |
| `_tgaLoad` | 80 | 5 | `f8b4b9349bd561b4a873c3a16eb7f6ff3e467536af42ba6a440d4358343f76ab` |
| `_tgaLoadHeader` | 208 | 12 | `d5e69514d04871026c0348b4a2e2f3854299a5c52cfb49a37da356fb4b2f630d` |
| `_tgaLoadImageData` | 144 | 1 | `80afa48e156b4bcb0a794ab17f3d9e1ced94769f6a3c185a67076e13221d07d6` |
| `_to_screen` | 80 | 3 | `33185d4b8ecaafa64b8b39dc62f9b4b544f59ff3d853784378b3db738d5bc855` |

## Complete unresolved owner map

All 37 remaining owners are mapped below from target call and relocation topology. `SDK member`
means an XDK C++ compatibility/member inline whose target entry consumes values already live in
`EAX`/`ECX`/`EDX` and often cleans only the hidden `this` slot. Those bodies cannot be expressed
as ordinary public C owners without forcing a private ABI.

| Target owner | Size/relocs | Normalized SHA-256 | Topology mapping and policy |
|---|---:|---|---|
| `_code_000d0d10` | 432/32 | `0c9239db142a50a3fe1585fe634701a6d96f7cf60c6424feeb317433eae9e4d0` | XDK inline `D3DDevice_SetRenderState`; 30 state callees plus `_D3DSIMPLERENDERSTATEENCODE` and `_D3D__RenderState`; private register entry and over batch bound. |
| `_code_000d0ec0` | 80/5 | `b6cf386298aad881ad444eb1d4f3f732cfd75e40bc5626748da27fe3eac79469` | XDK inline texture-stage dispatcher, void/private-register variant; same five callees as the retained public owner. |
| `_code_000d0f10` | 16/1 | `da3274ee297466e512d4f578a729fd2a12e86cc08fec77580ea60253bc65e8a8` | SDK member `GetBackBuffer`; raw callee `_D3DDevice_GetBackBuffer@12`; private register entry. |
| `_code_000d0f20` | 32/1 | `b66fe9aee1054d937c03d48868f80a8234d7fd347165bd3619636cd5d3a1dac0` | SDK member `CreateTexture`; raw callee `_D3DDevice_CreateTexture@28`; private register/stack split. |
| `_code_000d0f40` | 16/1 | `52e1fc33ef345b2e01eb4dfeb90dbea92beabe621157eda9ca9fe0c3ab98b107` | SDK member `SetRenderTarget`; raw callee `_D3DDevice_SetRenderTarget@8`; private register entry. |
| `_code_000d1020` | 16/1 | `86bba478f3ae679ef47c51029274a929eeef320b774995619ffc8bf0b082af00` | SDK member `GetDepthStencilSurface`; raw callee `_D3DDevice_GetDepthStencilSurface@4`; private register entry. |
| `_code_000d1030` | 32/1 | `7c0f2bf860046338a42bfbc700952ddde1cc4fd22dcd64cf11d1512eda94be76` | SDK member `Clear`; raw callee `_D3DDevice_Clear@24`; private register/stack split. |
| `_code_000d10e0` | 16/1 | `52e1fc33ef345b2e01eb4dfeb90dbea92beabe621157eda9ca9fe0c3ab98b107` | SDK member `SetTransform`; raw callee `_D3DDevice_SetTransform@8`; private register entry. |
| `_code_000d10f0` | 16/1 | `52e1fc33ef345b2e01eb4dfeb90dbea92beabe621157eda9ca9fe0c3ab98b107` | SDK member `GetTransform`; raw callee `_D3DDevice_GetTransform@8`; private register entry. |
| `_code_000d1160` | 544/32 | `d391ce622a8d1be40209833f735c268b440f24a07bc06da8e1b306fc82ad8a0e` | SDK member render-state dispatcher; same 30 state edges as `0d10`, HRESULT return/private `this`, over batch bound. |
| `_code_000d1380` | 16/1 | `52e1fc33ef345b2e01eb4dfeb90dbea92beabe621157eda9ca9fe0c3ab98b107` | SDK member `SetTexture`; raw callee `_D3DDevice_SetTexture@8`; private register entry. |
| `_code_000d1390` | 96/5 | `89d1c696147a0aae6d137f03b3bf6d284aa9ef5c083a33905dd28caeb08775c3` | SDK member texture-stage dispatcher, HRESULT/private-`this` variant; five texture-state edges. |
| `_code_000d14c0` | 16/1 | `db1d1b9bf1eb9c529b3d3187b7a89d8d35944820bb41637558be9520fed5cfa4` | SDK member `SetVertexShader`; raw callee `_D3DDevice_SetVertexShader@4`; private register entry. |
| `_code_000d1660` | 16/1 | `db1d1b9bf1eb9c529b3d3187b7a89d8d35944820bb41637558be9520fed5cfa4` | SDK member `SetPixelShaderProgram`; raw callee `_D3DDevice_SetPixelShaderProgram@4`; private register entry. |
| `_code_000d1670` | 16/1 | `6d46436cb945dbb1b016f222c634a74f3596ac3dee003ade6338c25801aa8edb` | SDK member `BlockUntilVerticalBlank`; raw callee `_D3DDevice_BlockUntilVerticalBlank@0`; hidden-slot cleanup. |
| `_code_000d1680` | 32/1 | `1968e8e622540279eb52c0ed6857a463992923b18f0aa79ae8d5a455d65fa39f` | SDK member `SetVertexData2f`; raw callee `_D3DDevice_SetVertexData2f@12`; private register/stack split. |
| `_code_000d16a0` | 48/1 | `5c16c28fe05bb30302d29f8835bb9f81b6de3d0eb8436ec3e4bb957926b21459` | SDK member `SetVertexData4f`; raw callee `_D3DDevice_SetVertexData4f@20`; private register/stack split. |
| `_code_000d16e0` | 16/1 | `db1d1b9bf1eb9c529b3d3187b7a89d8d35944820bb41637558be9520fed5cfa4` | SDK member `Begin`; raw callee `_D3DDevice_Begin@4`; private register entry. |
| `_code_000d16f0` | 16/1 | `c9740afccf5e7e01a66dfdb669d22a8462ac214622a1e8eff90b803ddb2d68c5` | SDK member `End`; raw callee `_D3DDevice_End@0`; hidden-slot cleanup. |
| `_code_000d1770` | 352/22 | `a9509aab99edb2bd3c1fcb370494c434e66d22e7739ca3823806071353b2dba6` | Quad drawing helper: `_generate_gravy_rect`, `Begin`, five `SetVertexData4f`, ten `SetVertexData2f`, `End`; no authenticated typed signature. |
| `_code_000d18d0` | 96/3 | `1a9005ce878930a6650b8fa1051e810c7203f084dbc6f7baef123b55af967504` | Wrapper around `0d1770` with 320.0/240.0 double constants; donor identity/signature uncertain. |
| `_code_000d1930` | 96/3 | `b97e9b8de80651484aec3b85c442890712ab8a10727916fdee74edff460d8d68` | Second wrapper around `0d1770` with the same 320.0/240.0 constants; donor identity/signature uncertain. |
| `_code_000d1990` | 48/0 | `850ed5c64644ea9fbc25bc5646e9e317d4b05f3fe936cc0af8774d97007bfa75` | Initializes a 20-byte D3D descriptor from pointers already in `EAX`/`ECX`; private register entry and representation ownership uncertain. |
| `_code_000d19c0` | 160/12 | `11a3091abfda5f0177775ee201cbbef3ed04dc681fde3313e770784f066a9277` | Four `SetVertexData2f` edges and eight relocations to `_num_layers+4`; typed storage/signature ownership uncertain. |
| `_code_000d1a60` | 304/23 | `6cf2621ba7ff5f4bf2784ace65df4338521695310598cdf216d11c782785b489` | Textured quad chain: `Begin`, five `SetVertexData4f`, four calls to `0d19c0`, `End`, plus float constants; dependent signatures unresolved. |
| `_code_000d1ba0` | 16/0 | `2a2f950faf0d7851fcb17366fb6bc179054ac4270b9c00d37aa03373db6cc20a` | SDK texture/resource member no-op (`ret 8`); private member ABI. |
| `_code_000d1bb0` | 16/1 | `43d508dca4211f96527b769632b4c2daf03f76d0dd594c76496b6c2c32c5c27a` | SDK resource `Release` member; `_D3DResource_Release@4`; private register entry. |
| `_code_000d1bc0` | 16/1 | `b81e2c81923547d777c095e4eb56dd8d95c5fa3e9f93f27fd9a81c7c476ad329` | SDK texture `GetLevelDesc` member; `_D3DTexture_GetLevelDesc@12`; private register entry. |
| `_code_000d1bd0` | 32/1 | `6db80a204a2444a6d70a961bbe723fc8fc6e2ee29afba744bb924f5979bef60f` | SDK texture `LockRect` member; `_D3DTexture_LockRect@20`; private register/stack split. |
| `_code_000d1bf0` | 16/0 | `da8ba144e75a66d0069c294448c6de9ed6a0ad3e5d2dbea20b96b413d6cb6f97` | SDK texture/resource member returns `S_OK` and `ret 8`; private member ABI. |
| `_SetRenderStateSmart` | 592/32 | `b290dcd2becddad5cdf6469ba3b0bc0c1be4b7fd33c64f0a6681741951c75801` | Named public XDK render-state decision tree; 30 state edges plus state-table data; omitted as a large reconstruction. |
| `_code_000d1f00` | 1200/95 | `d46ad31ad2bfc55deebf3158be3f38b9a89d51bde569f05b4f27a417d323db62` | Main progress render: pow/ftol, sound, transforms, render/texture state, quad helpers, shaders, timing, and extensive BSS/data; large and storage/math-sensitive. |
| `_code_000d23b0` | 176/12 | `5707b7a36b2cf90dcb514df9b1f8a4678b7c124af5d1d228bed7ea8cde8d971d` | Texture decode/load wrapper: CreateTexture/LockRect/GetLevelDesc, cache path/string/stdio/TGA calls; D3D object/storage ownership uncertain. |
| `_code_000d2460` | 208/14 | `b2a6e3a0d21c1564c3e2dd46b4d37b290023240ae021f5a15411594a5f508b37` | Gradient-texture builder: CreateTexture/LockRect plus `__CIpow`/`__ftol2` and texture storage; forbidden math-lowering and ownership risks. |
| `_progress_bar_eachframe` | 192/21 | `610ebeb977414e4afa3afdb90df3caf97ca622f2137cead30634d25eb6573c1c` | Lifecycle/cleanup: GetTickCount, three resource releases, sound stop/release, `_num_layers`, progress-mode, `_last_t`, and broad BSS; HCEA is later PPC and Pastudan uses raw storage addresses. |
| `_code_000d25f0` | 1168/105 | `af7bdc9fb76354055664e52862b524a8cb18b04974936d8bad5274935f40075a` | Screen/resource initialization: D3DX matrices, texture helpers, render state, sound, D3D surfaces, shaders, and extensive BSS; large reconstruction. |
| `_generate_gravy_rect` | 272/16 | `d6a98b3d0e5206030f585883ac0f137fbd6bb1644799835a8960a5de77110b12` | Two `D3DXVec4Transform` calls and two assertion/exit branches rooted at screen-transform BSS; existing natural body remains nonexact and was outside this batch. |

## No-retry and next-evidence policy

- Do not retry the anonymous SDK member/compatibility owners as ordinary C functions. Their
  register entry and hidden-slot cleanup are target facts, and matching them would require a
  forbidden private ABI, register forcing, assembly, or byte forcing.
- Do not use HCEA's PPC/no-op progress-bar files as x86 body donors.
- Do not import Pastudan's raw addresses, reconstructed D3D layouts, or storage casts.
- Do not retry `0d1770` through `0d1a60`, `0d23b0`, or `progress_bar_eachframe` until an
  authenticated January-era typed source/signature donor resolves their ownership.
- Do not attempt `0d2460` or the large render/setup owners without a separately authorized
  math/storage lane.
- The retained public texture-stage body is closed. Reopen it only if the target object or
  compiler contract changes; its first gate and clean replay are already exact.
