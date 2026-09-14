# `path_smoothing.obj` - opus5 150K house-clean lane, wave w3 (2026-09-14)

## Scope and baseline

- Translation unit `source/ai/path_smoothing.c`; target `build/split/source/ai/path_smoothing.obj`.
- Worker notes: `scratch/workers/w3_path_smoothing_actor_firing_position_etc.md`.
- Baseline real-file gate (guard passes): `== exact 4 residual 2 unwritten 0`: `_find_tangent_point` 336 [sha],
  `_find_turning_point` 896 [sha]. No parks.
- Ledgers read: `path_smoothing_obj_jonas_private_surface_predicate_20260826.md`, `..._jonas_natural_frontier_audit_20260830.md`,
  `..._jonas_helper_restoration_exact_20260831.md`, `..._opus5_150k_w1_20260914.md`. w1 shapes ps_v1..v4 and pt_v1 not repeated.
- `branch_sweep.py`: best historical blob 4/2/0 (equal to the current file).

## Result

**Real file unchanged** (final 4/2/0, guard passes, fake scan 0 leads). Law detectors run; four new shapes, none exact.

### `_find_turning_point` (896/30, only +0x190 operand order)

- A12 confirms the `negative_radius` local: January's preheader emits `-radius` -> `[ebp-0xc]` before `&bsp->vertices` and the
  zero-extended `clockwise`.
- January negative x `fld nr; fmul dir.i`, negative y `fxch st(1); fmul nr`: A22 ranks nr younger at x but reuses the x87 dir.j at y.
- t1: negative side through `point_from_line2d(point, &edge_direction, -radius, &negative_point)` (no local): x matches, y
  `fld nr; fmul st(2)` + extra `fstp st(0)`, 912 B. t2: positive side through the helper, negative by name: byte-identical.
- Classification: x87 operand scheduling tie.

### `_find_tangent_point` (336/5)

- January keeps `tangent_length` on the x87 until after the tangent_points[1].x store (two trailing `fstp st(0)`), the A11
  signature of an implicit-conversion CSE. Detector hit: `tangent_length = (real)sqrt(tangent_length);`.
- tp1: cast removed (`sqrt`); tp2: `square_root(tangent_length)`. Both byte-identical to baseline, so A11 is refuted at this site.
- Classification: x87 stack-lifetime tie.

## Reopen criteria

Unchanged from the 20260830 ledger: authentic source, local-variable evidence, or a VC7 law separating the x/y ranking
(turning point) or the tangent-length lifetime (tangent point).
