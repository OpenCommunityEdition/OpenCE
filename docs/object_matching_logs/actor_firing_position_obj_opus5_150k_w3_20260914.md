# `actor_firing_position.obj` - opus5 150K house-clean lane, wave w3 (2026-09-14)

## Scope and baseline

- Translation unit `source/ai/actor_firing_position.c`; target `build/split/source/ai/actor_firing_position.obj`.
- Worker notes: `scratch/workers/w3_path_smoothing_actor_firing_position_etc.md`; scratch dir `scratch/workers/w3_path_smoothing_actor_firing_position_etc/`.
- Baseline real-file gate (guard passes): `== exact 26 residual 1 unwritten 0`. Residual `_pre_evaluator_attack` 1120 [relocs 38!=39] is parked.
- Ledgers read: `actor_firing_position_obj_jonas_prefix_20260821.md`, `..._opus5_100k_20260914.md`, `..._opus5_100k_waveD_20260914.md`,
  `..._opus5_150k_w2_20260914.md`; prior scratch bodies `scratch/workers/actor_firing_position/at_{s1,s2,s3,d4}.c`.
- `branch_sweep.py`: best historical blob 23/4/0 (below the current file).

## Result

- **No new exact function.** Final real-file gate 26/1/0, rows identical to baseline, guard passes, park drift 0/1, fake scan 0 leads.
- **One byte-inert house-rule fix landed:** `#include "cseries/sort.h"` (genuine owner of `qsort_4byte`) after `cseries/errors.h`.
  It removes the C4013 at `actor_select_firing_position` (already exact). Every `.text` owner has identical size, relocations and
  normalized sha before and after; no owner added or removed vs `build/base`; `CL /Zs /W3` shows no new warning. No credit.

## `_pre_evaluator_attack` (park) - body fully recovered, owner-blocked only by `_point_from_line3d`

January facts: direction i,j,k stored; `along` = (jj+kk)+ii from memory; one `fchs`, then per component
`fld t; fmul v.c; fadd direction.c` kept on the x87 (frame 0x34, no projection slot); magnitude (jj+kk)+ii over those values.
STORE/CALL ORDER law: `evaluation = 0.0f` is stored after `actor_get_weapon_definition` and the MIN, and `evaluation = 6.0f`
after the attack-vector loop.

| shape | change (current real file + one body) | result |
| --- | --- | --- |
| d4 | w2 body (separate `projection` destination) | 1120/39 sha: magnitude inner pair (kk+jj) vs January (jj+kk) |
| d5 | d4 + `dot_product3d(&projection, &projection)` | inert |
| d6 | point_from_line3d **in place** into `direction`, then `magnitude_squared3d(&direction)` | **EXACT** |
| d7 | real body + in-place projection, but declaration-time `evaluation` inits and MIN | sha (store order) |
| d8 | d6 + `20.0f * (distance_from_boundary / 2.0f)` | **EXACT 27/27** |
| d9 | d6 + unparenthesised `distance_from_boundary * 0.5f * 20.0f` | 1104/38 |
| d10 | d8 without the dead `projection` local | **EXACT 27/27**, only this park drifts (to target) |

d6/d8/d10 fail the `_point_from_line3d` emitted-symbol guard (48-byte COMDAT) and use `(real_point3d *)` casts, so nothing was
landed or proposed as a reopen. The in-place destination is the new finding (A22 younger-operand: the helper's destination is its
own source vector). ORCHESTRATOR-PROPOSAL (owner-ruling list C3): `_pre_evaluator_attack` 1120 with
`scratch/workers/w3_path_smoothing_actor_firing_position_etc/afp_d10.c`.

## `path_state_estimated_distance` C4013 - prototype measured and rejected

No header declares it (definition `path.c:1331`). A shadow `path.h` with the prototype after `path_attractor_weight`
breaks `units::_unit_preprocess_node_orientations` (EXACT -> residual) among the 55 path.h consumers. Leave the implicit
declaration (the return value is discarded; byte-inert in this TU).

## Reopen criteria

- `_pre_evaluator_attack`: owner admits a `_point_from_line3d` surplus COMDAT for this TU (body d10 is ready).
- `path_state_estimated_distance` prototype: only inside a combined header packet with its own consumer census.
