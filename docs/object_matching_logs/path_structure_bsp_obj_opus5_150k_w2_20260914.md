# `path_structure_bsp.obj` — Opus5 150K wave w2 (2026-09-14)

## Scope and baseline

- Translation unit `source/ai/path_structure_bsp.c`. Baseline gate (guard passes): `== exact 4 residual 1 unwritten 1`.
- `_structure_test_line2d` is parked (1104/29). `_structure_test_pill2d` is unwritten (624/11) and is on the brief's
  do-not-spend list.
- Ledgers read: `path_structure_bsp_obj.md`, `path_structure_bsp_line2d.md` (26 families), `path_structure_bsp_index_obj.md`,
  `path_structure_bsp_codex_salvage.md`, `path_structure_bsp_obj_jonas_defined_behavior_boundary_20260830.md`, and the
  donor-scan lead in `ai_obj_opus5_150k_w2_20260914.md`.

## Outcome

Nothing was landed or proposed, and the real file is unchanged.

### `_structure_test_line2d` — skipped as exhausted

The current alndiff shows:
- the documented collision-tail `t` spill (`fstp [ebp+8]` and reload versus our `fst`);
- a commutative operand order in the inner cross products (line2d ledger families 16 and 19 already spelled January's
  order, which was neutral);
- the final block order.

This wave found no lever for the same spill-versus-enregister family in the 128-byte `_circle_tangents` either (see
`path_obstacles_obj_opus5_150k_w2_20260914.md`).

There is also an admission blocker independent of the bytes. The body preserves January's read of
`pathfinding_surfaces[neighbor]` before any `NONE` test, which is an out-of-bounds read on boundary edges. Brief section 5
forbids reopening a body that preserves undefined behaviour.

Reopen criteria (both required):
- a same-compiler donor for the `t` spill;
- a defined-behaviour topology for the neighbour read.

### `_structure_test_pill2d` — skipped

The function is on the do-not-spend list. No genuinely new evidence was found: the ai w2 donor scan lists it only as a
research lead for the fst-home/hold/reload pattern, not as a solution.

Reopen criteria (either):
- a same-compiler exact donor for the perpendicular-component lifetime and tail hoist;
- orchestrator direction to land the historical 624/11 fuzzy body at zero credit.

## Final real-file gate

`== exact 4 residual 1 unwritten 1`. Rows are identical to baseline, the guard passes, park drift is 0 of 1, the owner
census is clean, the fake scan finds 0 leads, and `git diff --stat` is empty.
