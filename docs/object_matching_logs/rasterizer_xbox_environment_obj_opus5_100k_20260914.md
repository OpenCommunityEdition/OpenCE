# `rasterizer_xbox_environment.obj`: opus5 100K consolidated lane (2026-09-14)

## Scope and provenance

- Translation unit `source/rasterizer/xbox/rasterizer_xbox_environment.c`,
  target `build/split/source/rasterizer/xbox/rasterizer_xbox_environment.obj`.
- VC7 13.00.9254, repository flags `/O2 /Oy- /DDEBUG /Dxbox` with the checked-in
  XDK headers; the January split COFF is the byte authority.
- Lane `opus/100k-consolidated-20260914` (worker waveB). No Ghidra or map-atlas
  extract was prepared for this object. The evidence used:
  - the January COFF: relocations, jump tables, and instruction order from
    `tools.campaign.alndiff`;
  - the XDK `D3D8Types.h` pixel-shader macros;
  - the earlier environment ledgers (`_codex_small_exact_20260902`,
    `_diffuse_texture_reconciliation_20260909`,
    `_opus_reflections_reconciliation_20260909`,
    `_lightmap_draw_fuzzy_20260912`).

## Validated baseline

Lane HEAD gate (point guard passed): **40 exact, 3 residual, 1 unwritten of 44**,
15,344 of 21,264 padded code bytes exact. No data work in scope.

| Row | Target size/relocs/sha16 | Base size/relocs/sha16 | First divergence |
| --- | --- | --- | --- |
| `_IDirect3DDevice8_SetTexture@12` | 16 / 1 / 52e1fc33ef345b2e | absent | unwritten |
| `__rasterizer_environment_lightmap_draw` | 4016 / 248 / 7818e559325c1524 | 3984 / 246 / 02f716db92e8099f | 0x3e1, stage-0 filter call setup order |
| `__rasterizer_environment_diffuse_light_begin` (parked) | 928 / 58 / 96f42a578f493250 | 928 / 58 / 6b3b2693e14dabc8 | stack-slot assignment (park) |
| `_rasterizer_environment_specular_spot_light_begin` (parked) | 960 / 64 / e0331f6b94a74f9c | 944 / 64 / d103843ee5f01bad | park |

## Accepted controls (retained in production)

In the no-lightmap branch of `_rasterizer_environment_lightmap_begin`,
`D3DDevice_SetTexture(2, NULL);` became
`IDirect3DDevice8_SetTexture(global_d3d_device, 2, NULL);`.

Evidence:

- The only `_D3DDevice_SetTexture@8` call relocation in the whole January
  object is at `lightmap_begin`+0x110.
- January also carries the out-of-line `_IDirect3DDevice8_SetTexture@12` COMDAT.
- The XDK wrapper is `static __forceinline`. This TU already emits the
  SetTextureStageState@16, SetVertexShaderConstant@16 and SetRenderState@12
  COMDATs through the same mechanism, from genuine calls.
- The rest of this file and `rasterizer_xbox.c` both use the spelling
  `IDirect3DDevice8_SetTexture(global_d3d_device, stage, texture)`.

Result:

- `_IDirect3DDevice8_SetTexture@12` is strict EXACT (16 B, 1 relocation).
- `lightmap_begin` stays exact (400/30).
- The final gate is **41 exact, 3 residual, 0 unwritten**. The 40 inherited
  exact owners are unchanged.
- Both parks still equal their `measurements.base`.
- The only new owner is the COMDAT, which the target also owns.
- `fake_match_scan.py` reports 0 findings.

## Experiment matrix: `__rasterizer_environment_lightmap_draw` (not landed)

| Shape | Size / relocs / sha16 | Siblings | Kept |
| --- | --- | --- | --- |
| base | 3984 / 246 / 02f716db92e8099f | 40 E | - |
| v1: see note 1 below the table | 4000 / 248 / e569fe1831834046 | 40 E | scratch |
| v2a: v1 + stage-0 MAG/MIN/MIP through `D3DDevice_SetTextureStageState` | 4000 / 248 / e569fe1831834046 (inert) | 40 E | reverted |
| v2b: v1 + stage-0 MAG/MIN/MIP written in both arms of the test-pattern if/else | 4000 / 248 / 43ac7853c0bb4168 | 40 E | scratch |
| probe (not admissible): v2b with `double` primary/secondary values | 4016 / 248 / a1748596cb70feb5, frame 0x220 | 40 E | diagnostic only |

Note 1: v1 makes three changes together:

- switch cases reordered to bump_color, bump_edge, specular_mask, matching
  January's jump table (3690/3730/3780/3820);
- both illumination `texture_modes` stores written as
  `PS_TEXTUREMODES(..., lightmap_missing ? PS_TEXTUREMODES_NONE : PS_TEXTUREMODES_PROJECT2D, PS_TEXTUREMODES_CUBEMAP)`;
- in the illumination branch, the constant stores placed before the five
  `real_*_to_pixel32` calls.

Measured facts:

- **`texture_modes` form.** The two-constant ternary compiles to
  neg/sbb/and/add. January has `sete; shl 0xa; or 0x18001/0x18021`, which is the
  `PS_TEXTUREMODES` shift form. The ternary also let VC7 tail-merge the NONE
  branch's `final_combiner_inputs_abcd/efg` stores. That merge accounts exactly
  for the two missing `_pixel_shader+32/+36` relocations in the 20260912 ledger.
- **Store order.** January places all five colour-constant calls after every
  constant `pixel_shader` store. A store cannot move across a call, so this is
  the source statement order.
- **Stage-0 filters.** One filter block after the join emits
  `push 2; mov edx; xor ecx`. January emits `xor ecx; mov edx; push 2`. Only a
  copy of the block in each arm reproduces this, because VC7 cross-jumps the
  identical tails.
- **Remaining difference.** After v2b, the only divergence is the `t` argument
  of both `point_from_line3d` calls:
  - January loads the stack `real` with `fld [ebp-4]` and pushes it with
    `push ecx; fstp [esp]`.
  - Ours emits `mov ecx,[ebp-4]; push ecx`.
  - This moves where pending pops are flushed (-16 B). It also changes which of
    eax/ecx/edx the following store block uses.
- **Double probe.** With `double` locals, VC7 produces the fld form and the
  complete downstream schedule matches byte for byte, but the frame changes. So
  the residual is exactly this one x87 argument form.
- **Rarity.** A scan of every January Halo object finds a stack float pushed
  to `[esp]` through fld only at these two sites.

## Do-not-repeat list

These probes left the `t` argument form unchanged:

- `#pragma inline_depth(0)` with the header `__inline point_from_line3d`. The
  call is not inlined, and it still uses mov/push.
- A TU-local `__inline` interpolate helper. It also reorders the calls.
- `(real)(double)x` and `(double)x` casts. Both are folded away.
- Moving the value locals into the else block.
- Declare+init in a nested block.
- `register`. Inert, and prohibited anyway.
- A `real values[3]` array.
- `(void)&x`. Optimized away.

Also inert: calling `D3DDevice_SetTextureStageState` instead of the
`IDirect3DDevice8_` wrapper for the stage-0 filters.

## Residual classification

- `lightmap_draw`:
  - **Measured:** the four corrections above (case order, `PS_TEXTUREMODES`,
    store/call order, per-arm stage-0 filters). They are kept in scratch.
  - **Remaining:** the x87 argument form at two call sites.
  - **Inference:** no natural `real`-typed spelling that produces it is known.
    This is an unresolved codegen form, not a register tie.
- The two parks are unchanged and were not edited (locked).

## Reopen criteria

- `lightmap_draw`: reopen when a natural `real`-typed source form is found that
  makes VC7 push a stack float argument with fld/fstp, either in another exact
  function or in a compatible-compiler donor. Then combine that form with
  `scratch/workers/rasterizer_xbox_environment/lightmap_draw_candidate.c`. That
  file is the landed file plus v1 and v2b: gate 41/3/0, lightmap_draw 4000/248,
  park drift pass.
- Parks: as documented in `config/parked.json`.

## Disposition

- `_IDirect3DDevice8_SetTexture@12`: strict exact, retained.
- `__rasterizer_environment_lightmap_draw`: still residual, zero credit. The
  improved candidate stays in scratch only, because brief section 6 lets a
  written residual land only when it is strict exact.
- Parked rows: unchanged.
