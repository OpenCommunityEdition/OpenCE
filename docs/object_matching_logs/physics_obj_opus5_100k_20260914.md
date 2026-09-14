# `physics.obj` opus5 100K consolidated lane (2026-09-14)

## Result

One new strict-exact function lands in production:

| Function | Padded / meaningful | Relocs | Normalized SHA-256 (16) | Result |
|---|---:|---:|---|---|
| `_physics_compute_biped_collision` | 1136 / 1133 | 43 | `6f544fa6bbdc541c` | **new strict exact** (landed) |

Per-TU gate: baseline `exact 10 residual 4 unwritten 3` -> final
`exact 11 residual 4 unwritten 2`. No baseline-exact row regressed; the
`_point_from_line3d` guard passes; both parks are byte-stable.

Two further functions are **strict exact in a verified candidate but not
landed**, because the caller needs a `real_math.h` declaration that workers may
not add (see ORCHESTRATOR-PROPOSAL):

| Function | Padded | Relocs | SHA-256 (16) | Status |
|---|---:|---:|---|---|
| `_physics_update_new` | 1984 | 66 | `8cc42cd03258ede2` | exact in candidate, header-blocked |
| `_rotate_vectors3d_by_angular_velocity` | 432 | 24 | `00064ac648e168b3` | exact when rooted by the caller above |

## Scope and provenance

- Translation unit `source/physics/physics.c`, target `build/split/source/physics/physics.obj`
  (SHA-256 `2d7273c20036601c6ee6a87a032a12e38d598840f7614cecba225910fe11d46c`).
- Lane base `b8cd50113` (branch `opus/100k-consolidated-20260914`).
- Compiler: Microsoft VC7 13.00.9254 `CL.Exe`, flags `/nologo /c /O2 /Oy- /DDEBUG /Dxbox`
  plus the repository include paths.
- Names/topology: January debug map atlas (`scratch/atlas/physics.obj.txt`),
  January PDB publics (`scratch/pdb200-current/pdb_symbols.json`: publics include
  `_physics_compute_new`, `_physics_test_point`, `_physics_update_new`; the private
  helpers, including `_physics_compute_biped_collision`, are absent -> static),
  Ghidra decompilation of the January object, HCEA reconstructions
  (`physics_compute_biped_collision.c`, `physics_update_new.c`,
  `physics_compute_vehicle_collision.c`) for semantics only.

## Validated baseline

| Row | Target padded | Relocs | Baseline candidate |
|---|---:|---:|---|
| `_compute_ground_plane` (PARKED) | 336 | 14 | 336/14 sha `ea2976dc...` (target `edeff771...`) |
| `_physics_compute_new` (PARKED) | 2944 | 49 | 3120/51 sha `4ab38244...` (target `a6981d09...`) |
| `_physics_compute_vehicle_collision` | 1168 | 14 | 1152/14 (target `5aae4029...`) |
| `_physics_update_old` | 5168 | 115 | 5280/114 (target `bee48328...`) |
| `_physics_compute_biped_collision` | 1136 | 43 | unwritten |
| `_physics_update_new` | 1984 | 66 | unwritten |
| `_rotate_vectors3d_by_angular_velocity` | 432 | 24 | body present, unrooted (not emitted) |

Data: January `.data` is 44 bytes (four physics globals, `depths_of_hell`, and a
12-byte unnamed function-local table); `.bss` holds
`_debug_physics_disable_penetration_freeze` (1 byte). After this lane the
candidate `.data` is 44/44 byte-identical (the table is
`physics_compute_biped_collision`'s `scales[] = {0.5f, 0.25f, 1.0f}`).
No data credit is claimed here.

## Accepted controls (landed)

`physics_compute_biped_collision` (static; January relocation skeleton, PDB
publics, assert at line 830):

- pill base tested with `collision_model_test_point`, otherwise a sphere of
  radius `height*0.5f + width` around the pill center against
  `MAX(width - 1/64, 1/64)`; single-exit `collision` boolean;
- one `acceleration` vector reused in place (direction, normalized, `k += 0.8f`,
  renormalized, `scale/add/scale` by `MAX(vehicle_speed, 0.1f)`, vehicle velocity,
  `0.5f`) - January writes every step into the same frame slot;
- pill base advanced by `2*acceleration` before `collision_fix_pill`;
- `cause_damage` boolean expression with `>` predicates
  (`last vehicle != instance || game_time > exit + 90`) `&&`
  (`vehicle_speed > 1/15 || distance_squared3d(velocities) > 1/900`): January's
  `je`/`jne` after `fcomp` proves the `>` spelling (a `<=` spelling emits `jp`);
- driver credit through `vehicle->unit.driver_object_index` and `object_get`;
  `damage.scale = 1.0f` stored before `SET_FLAG(damage.flags, _damage_area_of_effect_bit, TRUE)`;
- killed-unit effect: `scale` from the local `scales` table (unsigned NUMBEROF
  compare), then origin, then `scale_vector3d(&acceleration, -1.0f, &direction)`.

Supporting additions: `#include "game/game.h"` (`game_time_get`, otherwise
C4013), `#include "units/unit_definitions.h"` (`unit_definition_get`), a
TU-local complete copy of `struct game_globals_falling_damage` with a 0x98 size
assert and a disclosure comment naming the identical copies in
`objects/damage.c` and `units/bipeds.c`. Disclosed casts: two
`(real_point3d const *)` casts for `distance_squared3d` on the two translational
velocity vectors (January calls `distance_squared3d` on velocities) and one
`(struct object_datum *)` datum upcast (precedent `items/weapons.c`).

## Experiment matrix

| Function | Shape | Size / relocs | Outcome |
|---|---|---|---|
| biped_collision | HCEA-shaped body | 1088/43 | reverted |
| biped_collision | in-place acceleration, base advance, radius/width locals | 1136/43, frame +4 | intermediate |
| biped_collision | single-exit `cause_damage`, `>` predicates, scale-before-flag | 1136/43 EXACT | **landed** |
| update_new | research body + stuck flags + debug global | 2000/66, frame 0x1c8 | reverted |
| update_new | reload `instance->physics`, `set_real_point3d` COM | 1984/66, frame 0x1c0 | intermediate |
| update_new | helpers, block-scoped inertia matrices, double-literal ternary | 1984/66, 2 dot orders | intermediate |
| update_new | explicit/argument-swapped dot spellings (x5), declaration/copy orders (x4) | tie flips velocity-correction x87 lifetime | reverted |
| update_new | `PHYSICS_POINT_FROM_LINE3D` velocity correction + `dot_product3d(&worst_delta, &n)` | 1984/66 EXACT (+ rotate 432/24 EXACT) | header-blocked |
| vehicle_collision | flag at block end / named offset | 1152/14 | reverted |
| vehicle_collision | inline `point_from_line3d` | 1168/14, 1-insn store tie | guard fails (COMDAT) |
| vehicle_collision | `PHYSICS_POINT_FROM_LINE3D`, 12 flag positions | 1168/14, store tie at every position | not landed |
| update_old | initializers after matrix call | 5248/114 | not landed |
| update_old | full decoded rewrite with the macro | 5024/111, 453 masked diffs | not landed |

Candidate artefacts: `scratch/workers/physics/` (`unv_final.c`,
`ready_after_matrix3x3_multiply_header.c`, `update_old_body.c`, `vcm_*.c`).

## Do-not-repeat

- `physics_update_new`: open-coded `linear_velocity += n*scale` cannot reproduce
  January's multiplier lifetime; declaration/copy-order permutations and explicit
  dot spellings are inert or flip the tie. The point_from_line3d expansion is the
  mechanism.
- Do not use `point_from_line3d` directly in physics.c (emits the forbidden
  COMDAT); use the TU-local scalar expansion.
- `physics_compute_vehicle_collision`: all 12 statement positions for
  `collision = TRUE` measured; none places the byte store where January does.
- Do not add a physics.c-local `matrix3x3_multiply` prototype or rely on its
  implicit declaration.

## Residual classification

- `_physics_compute_vehicle_collision`: scheduling tie (one byte store), measured.
- `_physics_update_old`: x87 operand-order/lifetime and register ties plus
  January's inline expansion of `valid_real_vector3d_axes2` (relocation
  multiset: 3x `__real@3f50624de0000000`, +2 `1.0f`, no call); structure largely
  decoded.
- `_compute_ground_plane`, `_physics_compute_new`: parked, untouched, byte-stable.
  Inference (not measured): `physics_compute_new` shares the fused `a + b*s`
  sites that the point_from_line3d expansion reproduces.

## ORCHESTRATOR-PROPOSAL

Add to `source/math/real_math.h` next to `matrix3x3_transpose`:

```c
real_matrix3x3 *matrix3x3_multiply(
	real_matrix3x3 const *a,
	real_matrix3x3 const *b,
	real_matrix3x3 *result);
```

Evidence: owner `source/math/matrix_math.c:304` (exact, same signature);
January `physics_update_new` relocations @0x1a7/@0x1d4; unlocks 2,416 padded
bytes. Known risk: the 2026-09-08 sweep saw this declaration regress
`_rasterizer_dynamic_geometry_initialize`; the full board must be re-measured.
Consumers: every includer of `math/real_math.h`. After it lands, apply
`scratch/workers/physics/ready_after_matrix3x3_multiply_header.c`.

## Reopen criteria

- `physics_update_new` / rotate helper: the prototype above lands board-safe.
- `physics_compute_vehicle_collision`: new evidence for the collision-flag
  statement form.
- `physics_update_old`: after the macro lands, evidence on the
  `valid_real_vector3d_axes2` inline decision.

## Disposition

`physics.obj` remains NonMatching at 11/17 strict functions. Only
`_physics_compute_biped_collision` receives new exact credit.

## Header packet (orchestrator, checkpoint 4)

`matrix3x3_multiply` is declared in the focused owner header `math/matrix_math.h`, whose genuine owner is matrix_math.c; real_math.h placement regressed distant exact functions. The prepared body landed. Full build: physics_update_new (1,984) and rotate_vectors3d_by_angular_velocity (432) are exact, 0 regressions, parks clean. The vehicle +0x478 store uses the pre-existing canonical `stuck_mass_point_flags` field (5d3444f79); nothing was invented here.
