# `contrails.obj` — opus5 150K house-clean lane, wave w1 (2026-09-14)

## Result

| gate | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| baseline (`12f7375d4`) | 17 | 2 | 0 |
| final real file | **18** | 1 | 0 |

Newly strict EXACT: `_contrail_scale_random_value` (112 padded / 102 meaningful, 2 relocations,
normalized sha16 `45251a9f1f22cc5e`). Every other row is byte-identical to baseline (row diff), including
the residual `_contrail_add_points` (944/28, sha16 `4e3e3d2050699c48`). The `_point_from_line3d` guard
passes, the unit has no parks, the owner census against `build/base` shows no new or lost owner, and
`tools/fake_match_scan.py` reports 0 leads (0 at HEAD). No header or config change.

## `_contrail_scale_random_value` — EXACT

January evidence (alndiff against the 2026-09-13 zero-credit body, 96 bytes):

- `flags` is loaded once into EDX and `flag_bit` once into AL and both survive the two tests; the first mask
  uses ESI (push/pop) and the second shift count is `xor ecx,ecx; mov cl,al; inc cl` — an 8-bit
  `(short)(flag_bit + 1)` truncation. This is the shape of two inlined copies of a four-argument helper whose
  formals are temporaries, not of open-coded tests on the parameters.
- The second copy stores `upper_bound - lower_bound` into a memory home and reloads it for the multiply
  (`fstp/fld/fmul value/fstp`): the helper multiplies its result variable in place.
- `contrail_update_points` (already exact) inlines `contrail_scale_random_value` and therefore the same helper
  twice; it stays exact only with the in-place helper form.
- Same-codebase analogue: `sound_manager.c` `sound_scale_random_value` is written as a call to
  `sound_scale_value`. HCEA `contrail_scale_value.c` / `contrail_scale_random_value.c` give the same semantics.

Retained source: `contrail_scale_value` keeps its result copy and writes `result *= value;`, and
`contrail_scale_random_value` is two helper calls (`minimum` from `lower_bound` with `flag_bit`, `range` from
`upper_bound - lower_bound` with `flag_bit + 1`) followed by `real_local_random_range(0.0f, range) + minimum`.
The out-of-line `_contrail_scale_value` (32) stays exact.

Rejected shapes (measured, scratch only): helper `result = value * scale` (size 112 but x87 fst/keep in the
second copy and `contrail_update_points` broken); helper ternary / if-else / early return (96 bytes); helper
`scale = value * scale` or `scale *= value` (random + update_points exact but out-of-line `contrail_scale_value`
loses its x87 phi — two return blocks); caller variants (named range, nested call, sum order) and `char
flag_bit` are compiler-identical.

## `_contrail_add_points` — not landed

Frame `sub esp,0x214` vs `0x210`. The entire 4-byte gap is one extra home at `[ebp-0x1c]` for the `contrail`
pointer: January frees EDI/EBX inside the point loop to hold `&contrail_point->position` and
`&contrail_point->location` (the latter pushed twice for `scenario_location_from_line`) and keeps `marker` in EBX
across the cone call; ours keeps `contrail` in EBX, recomputes the location address and reloads `marker`. All
28 relocation identities agree.

Shapes: operand order of `fraction * object_velocity` (inert); scoping `direction`/`object_velocity` at the
marker-count block or marker loop (inert); cone-angle and inherited-fraction scaling written as inlined
`contrail_scale_value(contrail->density, definition->..., definition->scale_flags, bit)` calls — the velocity
x87 operand order then matches January (`fld fraction; fmul object_velocity`), size and relocations unchanged.
That helper topology is consistent with the proven `contrail_scale_random_value` shape and is the recommended
starting point: `scratch/workers/player_effects_particles_contrails/c_sh1.c` (worktree-local scratch).

Reopen criterion: a source-level reason for January to keep `&contrail_point->location` in a callee-saved
register (authenticated local pointer or different location API use), starting from the helper-scaling body.
