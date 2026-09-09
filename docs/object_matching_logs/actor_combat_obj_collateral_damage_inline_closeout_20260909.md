# `actor_combat.obj`: collateral-damage inline closeout (2026-09-09)

## Result

`_actor_combat_check_collateral_damage` is strict exact: **872 meaningful code bytes / 880 padded bytes**, with 21 relocations and 293 instructions. This retires its prior instruction-scheduling park without changing any header.

## Source and code-generation evidence

The retained body is the coherent collateral-damage reconstruction already supported by the January target disassembly and the HCEA/Claude reconstruction evidence recorded in `actor_combat_obj_collateral_damage_rejected_20260831.md`. Its only remaining mismatch was the x87 schedule of the first of four repeated three-dimensional squared-distance expressions.

`actor_combat.c` already establishes TU-local January inline variants before including `cseries.h`:

- `vector_from_points3d` -> `actor_combat_vector_from_points3d_inline`
- `magnitude_squared3d` -> `actor_combat_magnitude_squared3d_inline`
- `distance_squared3d` -> `actor_combat_distance_squared3d_inline`

Using that existing typed distance helper at all four natural distance-test sites restores the target's inline schedule exactly. It also removes repeated manual component arithmetic and keeps the source at the established subsystem abstraction level. No expression reordering, volatile/register steering, duplicate helper, inline assembly, representation punning, candidate-only COMDAT, or nonsensical logic was introduced.

The function declaration was already in its owning `source/ai/actors.h`; this packet does not move or add a prototype. The historical header-position blocker therefore does not apply to this closeout.

## Verification

Focused isolated gates, using PID-named scratch objects:

- `source/ai/actor_combat --all`: 20 exact, 5 residual, 9 unwritten; no inherited exact loss.
- `_actor_combat_check_collateral_damage`: `EXACT 880`.
- `_point_from_line3d` emitted-symbol guard: passed for `actor_combat.obj`.
- `source/units/units --all`: 189 exact, 0 residual, 0 unwritten. This independently confirms that the old `units.obj` regression is absent with the current header state.

The full `units.obj` translation unit currently emits its inherited `_point_from_line3d` owner, so its audit gate was intentionally run without the candidate-only guard. This packet does not touch that translation unit or alter its inline schedule.

## Credit

- Strict exact owners: **+1**
- Meaningful exact code bytes: **+872**
- Padded exact code bytes: **+880**
- Fuzzy credit: **0**
- Regressions: **0**
