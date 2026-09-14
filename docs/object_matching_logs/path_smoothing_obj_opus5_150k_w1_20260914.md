# `path_smoothing.obj` Opus5 150K house-clean lane, wave w1 (2026-09-14)

## Scope and baseline

- Translation unit `source/ai/path_smoothing.c`; January target `build/split/source/ai/path_smoothing.obj`.
- Lane `opus/150k-house-clean-20260914`, base `12f7375d4`.
- Baseline real-file gate (guard passes): **4 exact / 2 residual / 0 unwritten**.
  - `_find_tangent_point`: 336 B, sha.
  - `_find_turning_point`: 896 B, sha.
- There are no parks.
- Prior ledgers honoured: `path_smoothing_obj_jonas_private_surface_predicate_20260826.md`, `path_smoothing_obj_jonas_natural_frontier_audit_20260830.md` (no broad permutations), `path_smoothing_obj_jonas_helper_restoration_exact_20260831.md` (helper-restoration precedent).
- `branch_sweep` found no better blob.

## Result

**Nothing landed.** The real file is unchanged, the final gate is still 4 / 2 / 0, and the guard passes.

## `_find_turning_point` (896 B, 30 relocs, 326/326 instructions)

The only delta is at +0x190, in the x component of the negative side point. January loads `[ebp-0xc]` (the hoisted `-radius`) first and multiplies it by `edge_direction.i`; ours loads in the opposite order.

Four shapes were tried, following the helper-restoration precedent:

| shape | source change | result |
| --- | --- | --- |
| ps_v1 | `normalize2d` + `point_from_line2d(point, &dir, radius, ...)` + `point_from_line2d(point, &dir, -radius, ...)` | x order now matches January; y becomes `fld nr; fmul st(2)` (January `fxch; fmul nr`) plus an extra `fstp`, growing to 912 |
| ps_v2 | the same helpers with the `negative_radius` local | byte-identical to baseline (helper vs direct spelling is inert) |
| ps_v3 | direct expressions with `-radius`, no local | not hoisted; frame 0x40 vs January 0x44 |
| ps_v4 | helper pair only | same as ps_v1 |

**Classification:** an x87 operand scheduling tie. A hoisted temporary versus a named local flips the operand order of both components together, while January mixes the two orders.

**Reopen:** authentic side-point source, or a VC7 law that separates the two components.

## `_find_tangent_point` (336 B, 5 relocs, 129/129 instructions)

January computes `i*tl` as `fld i; fmul st(3)`, keeping `tl` on the x87 stack until the end. Ours brings `tl` to the top with `fxch` and consumes it.

- **pt_v1:** `rotate_vector2d` + `point_from_line2d`, since the tangent formulas equal a rotation by ±tl. Result: a `fchs` appears, the frame becomes 0x2c (January 0x24), and the function drops to 124 instructions. Worse.
- Operand permutations were not tried; they are forbidden steering, and the prior matrix found them inert.

**Classification:** an x87 stack-lifetime tie. The reopen criteria are unchanged from the 20260830 ledger.

## Checks

- No real-file change: `git diff --stat` is empty.
- Fake scan: 0 leads.
