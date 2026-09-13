# `rasterizer_xbox_active_camouflage.obj` draw reconstruction and real-alpha owner (2026-09-14)

Lane `opus/pdb-ida-15k-20260914`, worker for
`source/rasterizer/xbox/rasterizer_xbox_active_camouflage.c` only. Earlier
ledgers for this object (read in full before editing):
`..._jonas_visibility_20260828.md`, `..._jonas_credibility_recovery_20260830.md`,
`..._jonas_real_alpha_boundary_20260830.md`, `..._shared_owner_cache_20260906.md`,
plus `bitmaps_inlines` real-alpha reconciliation (commit `56cbe8470`).

## 1. Scope and provenance

- Target: `build/split/source/rasterizer/xbox/rasterizer_xbox_active_camouflage.obj`
  (January 2002 Xbox debug `cachebeta.exe`), VC7 13.00.9254 with the
  repository `/O2 /Oy- /DDEBUG /Dxbox` unit flags. No flag change; no `/QIfist`.
- Names/types: shared headers already in the tree
  (`rasterizer_model_types.h`, `rasterizer_xbox_pixel_shader.h`,
  `rasterizer_debug_options.h`, `game_globals.h`, `rasterizer_transparent_geometry.h`,
  `rasterizer_models.h`, `shaders.h`, `bitmaps_inlines.h`); HCEA
  `blam/rasterizer/rasterizer_active_camouflage_draw.c` for field names and
  semantics only; XDK 3911 `D3D8Types.h` for pixel-shader encodings; January
  assertion strings and line anchors 152-157, 164, 165. The January map atlas
  has no record for the draw; the PDB authenticates the public name only.
- Ghidra decompilation of the January object used as a hypothesis generator.

## 2. Validated baseline and result

| State | Exact / listed | Residual | Unwritten |
| --- | --- | --- | --- |
| Lane HEAD | 10 / 12 | 0 | 2 |
| Final real file | 11 / 12 | 1 | 0 |

Exact padded code: 2,192 of 4,208 bytes (was 2,080). Data: all eight January
`.rdata` assertion strings and the six-byte `.bss` owner remain equal.

| Function | Target size / relocs / sha16 | Final | Status |
| --- | --- | --- | --- |
| `_real_alpha_to_pixel32` | 112 / 6 / `28913a006347a05e` | 112 / 6 / `28913a006347a05e` | **newly strict EXACT** |
| `_rasterizer_active_camouflage_draw` | 2016 / 142 / `245046250b62273f` | 2016 / 142 / `19d5f73451e94d1c` | fuzzy, `[sha]` |

First meaningful divergence of the draw: target instruction 200 (0x2f2),
`fld [edi+0xa0]` of the texture-animation `v_scale` argument, where the
candidate instead emits the `vertex_constants[1]/[2]` constant stores first.

## 3. Accepted controls

- The draw is rebuilt on shared declarations. The historical donor body
  (blob `ed454827`, `74e015673`) was not copied because it depended on about
  ten shadow structures that shared headers now own.
- TU-local copies are limited to `transparent_geometry_group`,
  `shader_model_properties`/`shader_model_definition`, and the geometry-flag,
  shader-type, model-flag and model-effect enums. No shared header declares
  them yet. The copies are complete, layout-checked (`0xA0` group size,
  `animation` at `+0x6C`, `texture_animation` at `+0xFC`), and disclosure
  comments name the existing copies.
- `detail_map_v_scale` is a named local. January computes the product before
  storing `vertex_constants[0].i` (`fld [+0xec]; fmul [+0xd8]` precedes
  `mov eax,[+0xd8]`).
- The hyper-stealth lerp uses two statements (`x = normal*a; x += b*parameter;`).
  January reloads `group->effect.parameter` for every term and evaluates the
  normal-weighted product first. A cached parameter local produced an extra
  stack home (`fst [ebp+8]`) and a 16-byte size gap.
- Every pixel-shader constant is spelled with the XDK `PS_*` macros, as the
  cache function already is. The decoded values are exact: texture modes
  CUBEMAP/DOTPRODUCT/DOT_ST, dot mapping MINUS1_TO_1_D3D x2, 2 combiners, and
  outputs to R0. The final combiner is `T2,R0` with `C0.alpha` as alpha, and
  constant 0 is `real_alpha_to_pixel32(intensity)`.
- The duplicated `D3DTSS_ADDRESSV` on stage 0 (no ADDRESSW) is preserved.
  January's bytes show two ADDRESSV stores.
- `_real_alpha_to_pixel32` needs no new code. The admitted authenticated
  inline in `bitmaps_inlines.h` is emitted out of line by the genuine
  `final_combiner_constant_0` call (reloc at draw+0x795). No `/QIfist` is
  used, and no assembly is added to this TU.
- Every D3D call goes through the stock `IDirect3DDevice8_*` wrappers already
  exact in this object. No call exists that the reconstruction does not need.

## 4. Experiment matrix

| ID | Shape | Size / relocs | sha16 | Siblings | Kept |
| --- | --- | --- | --- | --- | --- |
| v1 | natural body on shared headers; v-scale product inline; parameter cached in a local | 2032 / 142 | - | 11 exact | no |
| v2 | named `detail_map_v_scale`; parameter read in place; single-expression lerps | 2016 / 142 | - | 11 exact | no |
| v3 | two-statement lerps | 2016 / 142 | `19d5f73451e94d1c` | 11 exact | basis |
| v4 | v3 with raw hex combiner ternary instead of `PS_COMBINERINPUTS` | 2016 / 142 | identical | 11 exact | no (inert) |
| v5 | locals at the if-block top instead of a bare inner block | 2016 / 142 | identical | 11 exact | yes |
| v6 | locals at function top (donor scope) | 2016 / 142 | identical | 11 exact | no (inert) |
| v7 | v5 + disclosure comments | 2016 / 142 | `19d5f73451e94d1c` | 11 exact | **landed** |

## 5. Do-not-repeat list

- Donor worktree `opus-30k-rasterizer-widgets-20260905/scratch` already holds
  about 60 variants that never closed these ties: `actcamo_pc_*` (24 in/out
  store permutations), `w13camo_d1..d3`, `w13camo_rest`, `w13camo_bat*`,
  `acamo_A*/B*/C*`. They cover bound locals for parameter/intensity/flags/
  distortion/model/map-scale pointers, tint-struct vs scalars vs
  `real_vector3d`, moved zero blocks, and out-param pointer spellings.
- This lane measured scope placement of locals, macro vs hex constants, and
  combined vs split lerps (only the split form is structural).
- Never: shadow copies of shared structs, `/QIfist`, new inline assembly,
  include-order or declaration-count steering, dummy D3D calls.

## 6. Residual classification

All measured, 542/542 instructions, identical frame (`sub esp,0x108`), and an
identical 142-relocation multiset by target. The residual is three linearization
ties of commutable stores (scheduling):

- A: `vertex_constants[1]/[2]` constant stores versus the
  `shader_texture_animation_evaluate` argument evaluation. Also present in the
  donor body.
- B: `faddp`/`fmul`/`fstp` of the distortion constants versus the
  `vertex_constants[1]` zero stores. Also present in the donor.
- C: the `pixel_shader.rgb_inputs[0]` `sbb` store versus the
  `rgb_outputs[0]`/`rgb_inputs[1]`/`rgb_outputs[1]` stores. C matched in the
  donor's shadow-struct TU and differs here with identical statement text.
  Inference: it follows TU declaration context (declaration-count oracle),
  not source logic.

## 7. Reopen criteria

- A shared-header consolidation that changes this TU's declaration set.
  Re-gate: tie C is known to flip with context.
- Authenticated January source or cross-build evidence for the
  texture-animation call site or the distortion constant block that implies
  different statements, not permutations.

## 8. Disposition

`_real_alpha_to_pixel32` is strict exact in the real-file gate and eligible for
credit. `_rasterizer_active_camouflage_draw` is landed as the best honest fuzzy
reconstruction with exact credit 0. The object stays `NonMatching`.

Checks at the final real file: `_point_from_line3d` guard passes. No parked
entries exist for this unit. The owner census finds no candidate-only code
owner; new data owners are only literal COMDATs that January references
undefined. `fake_match_scan` reports 0 leads, `/W3 /Zs` reports no warnings
from this TU, and `git diff --check` is clean. No header, config, tool, or
other source file was edited.
