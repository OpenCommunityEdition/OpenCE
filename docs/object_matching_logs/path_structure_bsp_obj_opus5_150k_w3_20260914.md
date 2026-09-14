# `path_structure_bsp.obj` - opus5 150K house-clean lane, wave w3 (2026-09-14)

## Scope and baseline

- Translation unit `source/ai/path_structure_bsp.c`; target `build/split/source/ai/path_structure_bsp.obj`.
- Worker notes: `scratch/workers/w3_path_smoothing_actor_firing_position_etc.md`.
- Baseline real-file gate (guard passes): `== exact 4 residual 1 unwritten 1`: `_structure_test_line2d` 1104 [sha] (parked),
  `_structure_test_pill2d` 624 UNWRITTEN (brief and orchestrator DO-NOT-SPEND).
- Ledgers read: `path_structure_bsp_obj.md`, `path_structure_bsp_obj_jonas_defined_behavior_boundary_20260830.md`,
  `path_structure_bsp_obj_opus5_150k_w2_20260914.md`; laws_w2 R12.
- `branch_sweep.py`: best historical blob 4/0/2 (no line2d or pill2d body exact anywhere).

## Result

**Nothing spent; real file unchanged** (final 4/1/1, rows identical, guard passes, park drift 0/1, fake scan 0 leads).

- `_structure_test_line2d`: laws_w2 R12 lists it as policy-excluded (it preserves January's `pathfinding_surfaces[neighbor]` read
  before the NONE test, an out-of-bounds read). No A-law addresses the collision-tail `t` spill and no new donor exists.
- `_structure_test_pill2d`: on the do-not-spend list; no new evidence.

## Reopen criteria

- line2d: a defined-behaviour neighbour-read topology **and** a same-compiler donor for the `t` spill.
- pill2d: a same-compiler donor for the perpendicular-component lifetime and tail hoist, or orchestrator direction to land the
  historical 624/11 fuzzy body at zero credit.
