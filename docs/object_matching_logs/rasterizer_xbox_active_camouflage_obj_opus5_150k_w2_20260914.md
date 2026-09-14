# `rasterizer_xbox_active_camouflage.obj` draw residual (opus5 150K lane, wave w2, 2026-09-14)

Worker for `source/rasterizer/xbox/rasterizer_xbox_active_camouflage.c` only. The real file is
**unchanged**: nothing became strict exact.

Ledgers read first: `_jonas_visibility_20260828`, `_jonas_real_alpha_boundary_20260830`,
`_jonas_credibility_recovery_20260830`, `_shared_owner_cache_20260906`, `_opus5_pdb_ida_20260914`
(including its do-not-repeat list), and the 100K consolidated handoff. `_real_alpha_to_pixel32` is
inherited and was not touched.

## Baseline

`== exact 11 residual 1 unwritten 0`; the guard passes; there are no parks for this unit.

`_rasterizer_active_camouflage_draw` (target 2016 / 142 / `245046250b62273f`; real file
`19d5f73451e94d1c`):
- 542/542 instructions and frame `0x108` on both sides;
- three linearisation ties, A, B and C (see the previous ledger).

## New evidence used

- **January `set_real_vector4d` owner.** The punpckhdq and repository symbol listing for
  `rasterizer_xbox_environment_fog.obj` proves one exists (`rasterizer_xbox_environment_fog.c:843`,
  exact).
- **Aggregate-initializer / scope law.** Measured this wave in `structure_detail_objects`: the
  constant stores of a block-scoped initializer are scheduled with the following statement's
  evaluation, and block scope moves where pending constant stores are emitted.

## Shapes (scratch only; a4–a6/a9 are new families, not in the donor `actcamo_*`/`w13camo_*`/`acamo_*` sets)

| ID | Shape | Result |
| --- | --- | --- |
| a1 | `set_real_vector4d` statements (TU-local inline probe) | identical to base (inert) |
| a2 | `set_real_vector4d` as `shader_texture_animation_evaluate` arguments | tie A unchanged |
| a3 | named `base_map_v_scale` local | worse (20 blocks) |
| a4 | arm 1: block-scoped `real_vector4d model_vertex_constants[3] = {{scale, v_scale*scale, 1, 1}, {1,0,0,0}, {0,1,0,0}}` | **tie A order resolved**, but frame `0x138` |
| a5/a6 | arm-2 array block-scoped only around the constant block | arm-2 constants scheduled early (24 blocks) |
| a9 | a4 + arm-2 block from the lerps through `SetVertexShaderConstant` | **frame `0x108`, tie A gone; only B and C remain (14 blocks)** |
| b1 | `real normal = …` initializer in the a9 block | inert |
| b2 | declaration order in the a9 block | inert |

Best structure: `scratch/workers/rasterizer_xbox_active_camouflage_structure_detail_objects/a9.c`.
It stays non-exact, so under the residual rule it is not landed.

## Remaining residual classification

- **B:** one-position interleave of the arm-2 zero-row constant stores with
  `faddp` / `fmul [intensity]` / `fstp`. It is a scheduling tie.
- **C:** the `pixel_shader.rgb_inputs[0]` `sbb` store is emitted after the
  `rgb_outputs[0]` / `rgb_inputs[1]` / `rgb_outputs[1]` stores. The prior ledger shows it matched
  in the donor shadow-struct TU with identical statement text, so it follows TU declaration
  context.

## Checks

The real file is unchanged. The real-file gate is `exact 11 residual 1`; the guard passes. The
owner census matches base; `fake_match_scan` reports 0 leads.

## Reopen criterion

Start from the a9 structure (arm-1 initializer block, arm-2 scoped block), then either:
- a shared-header consolidation that flips tie C; or
- authenticated evidence for the arm-2 constant-row spelling (for example a January
  vertex-constant helper) that moves tie B.

Do not repeat a1–a3, b1 or b2, or the donor permutation families.
