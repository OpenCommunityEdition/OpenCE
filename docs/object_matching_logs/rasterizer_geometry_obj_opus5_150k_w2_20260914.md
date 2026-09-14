# `rasterizer_geometry.obj`: opus5 150K wave 2 park re-investigation (2026-09-14)

## Baseline and outcome

The real-file gate, run with the guard, is `exact 16 / residual 3 / unwritten 0`. All three residuals are parked:

| Function | Padded bytes / relocations |
| --- | --- |
| `_uncompress_int32_to_real_vector3d` | 144 / 9 |
| `_compress_real_vector3d_to_int32` | 544 / 38 |
| `_compress_real_vector3d_to_int32_clamp` | 512 / 38 |

Nothing landed, and the real file is unchanged. Final-state checks:

- The gate rows are identical to the baseline.
- None of the three parks drifted.
- The owner census matches base. The only candidate-only owner against the target is the `_fast_ftol` COMDAT, which was already there.
- The fake scan reports 0 leads.

## `_uncompress_int32_to_real_vector3d`

**Residual:** one transposition, unchanged. January emits `fadd __real@3f800000` at +0x28, before `shl eax,0x16`.

**New evidence:** the real file defines this function last. January's section order puts it third, directly after
`uncompress_int16_to_real`. Moving the definition to January's position (`rg_ORD1.c`) keeps all 16 exact functions
exact, but the transposition does not move.

**Inert shapes:** each of these leaves the same transposition:

- member-wise result stores (these also shrink the function to 112 bytes);
- `1.0f + x*s`;
- scale-first product;
- fraction-first product;
- `>>=` folded into the j expression, or into both j and k.

## `_compress_real_vector3d_to_int32` and `_clamp`

**Residual:** 10 and 11 differing instructions (raw listing), all in the debug round-trip copy.

**Evidence:** every other January call to `uncompress_int32_to_real_vector3d` uses a separate buffer slot:

- rasterizer_lights: corona rotation, occlusion tests, lens flares draw;
- rasterizer_xbox_lights: sun glow;
- rasterizer: debug model vertices.

Only this pair shares the argument buffer with the copy destination and loads all three components before storing any.

**Inert shapes:**

- an inner block declaring `real_vector3d v2= *uncompress(&v2, ...)`;
- an inner-block declaration followed by assignment;
- moving the callee to January's definition position.

**Diagnostic only, not admissible (prototype cast):** a by-value return produces the grouped copy, but through a hidden
temporary at `[ebp-0x1c]`. That reproduces the park's known two-slot bracket.

## Reopen criteria

The recorded park criteria are unchanged. The probe scripts are in
`scratch/workers/connected_geometry_rasterizer_geometry_etc/rg_probe*.py`.
