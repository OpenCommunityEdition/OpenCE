# `rasterizer_xbox_debug.obj` completion (2026-09-02)

## Result

The Xbox debug-rasterizer compiland is now completely represented and marked
`Matching`:

- 18 / 18 strict exact code owners;
- 3,264 / 3,264 padded code bytes;
- 3,108 / 3,108 meaningful code bytes;
- 292 / 292 data bytes;
- 100% object-level objdiff code and data.

The inherited source emitted only the three empty end functions (48 padded
bytes). This wave therefore adds 15 strict owners and 3,216 padded bytes with
no fuzzy or semantic exception.

## Evidence and source identity

January's split COFF is the byte, ABI, assertion, relocation, and D3D call
authority. HCEA independently authenticates the underscore-prefixed immediate
debug family and its parameter roles. The generic rasterizer callers establish
the cross-compiland interface. Xbox D3D8 declarations establish the remaining
eight owners as compiler-retained XDK inline wrappers, not anonymous Bungie
helpers:

- `D3DDevice_SetRenderState`;
- `IDirect3DDevice8_SetRenderState`;
- `IDirect3DDevice8_SetVertexShaderConstant`;
- `IDirect3DDevice8_SetVertexData4f`;
- `IDirect3DDevice8_SetVertexData2s`;
- `IDirect3DDevice8_SetVertexDataColor`;
- `IDirect3DDevice8_Begin`;
- `IDirect3DDevice8_End`.

`config/symbols.json` records the exact decorated XDK names at the eight
January section boundaries. No `code_<address>` function identity remains in
this compiland's target map or source inventory.

The XDK's stock `D3DINLINE` declarations naturally retain out-of-line copies
in this January compiland. The source does not add an inline annotation,
`__forceinline`, address-taking trick, wrapper body, or compiler override.
Changing the vendor header policy loses the target sections, so the admitted
form preserves the authentic SDK inline schedule while still avoiding
project-authored inlining.

## Behavior

The source reconstructs the opaque and translucent debug setup paths, world
and screen-space state setup, line and triangle submission, optional secondary
colors, packed screen coordinates, color conversion, and line-strip loop.
Named D3D render-state and primitive constants are used throughout. The
semantic `rasterizer_debug_options`, `global_window_parameters`, `pixel_shader`,
and `global_d3d_device` owners replace anonymous storage views.

Public rasterizer calls are declared in their closest existing owner headers:

- the immediate-debug interface is owned by `rasterizer/rasterizer.h` and the
  duplicate declarations were removed from `rasterizer.c`;
- Xbox device, pixel-shader, state, and error interfaces are owned by
  `rasterizer/xbox/rasterizer_xbox.h`;
- color conversion is obtained from `interface/hud_draw.h`.

The two narrow `rasterizer_xbox_debug_*_prefix` layouts used to read the shared
rasterizer debug and window aggregates remain translation-unit-private. Their
named fields and offsets are proven by January accesses; no raw byte-offset
expression is used in executable code.

## House-rule audit

- Cseries real, point, color, pixel, boolean, and integer aliases are used.
- Parameters are one per line; no-argument lists name `void`.
- Every void function ends in an explicit `return;`.
- No handwritten inline assembly, volatile/register forcing, barrier, pragma,
  raw byte emission, fake dependency, false linkage anchor, undefined access,
  or nonsensical exact-match construct is present.
- No address-derived private function or global name remains.
- The compiland emits no `point_from_line3d` symbol or COMDAT.
- Focused fake-match, resolved-address, and volatile scans report no finding.

## Verification

- Focused strict gate: 18 exact, 0 residual, 0 unwritten.
- One-unit objdiff report: 100% code and 100% data.
- `rasterizer` remains 131 / 131 strict exact after declaration ownership was
  corrected.
- Direct consumers of the two edited owner headers and the protected 189-owner
  Units sentinel are rechecked on current canonical HEAD at admission.
- Full stable-tree regression comparison, linked Ninja build, semantic report,
  parked validation, and 261-test suite are rerun after cherry-pick.
