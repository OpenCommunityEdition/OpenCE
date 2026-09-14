# `weather_particle_systems.obj` — opus5 150K house-clean lane, wave w3 (2026-09-14)

## Result

| gate (`--forbid-emitted-symbol _point_from_line3d`) | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| baseline (lane HEAD `403108e28`) | 18 | 2 | 0 |
| final real file | 18 | 2 | 0 |

No source change. The row diff is empty, the guard passes, the unit has no parks, the owner census is clean, and the fake scan
reports 0 leads. `git diff --stat` is empty. Since w1, `_weather_particle_system_new_particle` has landed through packet C
(the `real_math.h` named result) and the `bitmaps/bitmaps.h` include in `7fe3891c3`.

## `_weather_particle_update_physics` (688 / 29) — OWNER-BLOCKED

The w1 candidate `cand_physics.c` was re-measured on the current tree as
`scratch/workers/w3_weather_particle_systems_particles_etc/w_cand_physics.c`.

- **Without the guard:** 19/20 exact, and this function is strict EXACT.
- **With the guard:** `FORBIDDEN-EMITTED-SYMBOL _point_from_line3d` (a 48-byte COMDAT). January inlines point_from_line3d twice:
  the velocity integration and the 0.001 jitter. Fresh-graphs showed both inlines are load-bearing.

Laws run as detectors:
- A22 VISIBLE INLINE DECLINED excludes `REAL_MATH_EXTERNAL_POINT_FROM_LINE3D`.
- A per-TU `#define point_from_line3d weather_..._inline` would add a new candidate-only COMDAT owner (admission check 5).

The lane rule puts point_from_line3d owner rulings OUT. Reopen when an owner ruling admits the COMDAT or a header mechanism
inlines without emitting it; then land the `w_cand_physics.c` body.

## `_weather_particle_system_render` (1680 / 47) — NOT LANDED

The w1 best candidate `ra.c` was re-measured on the current tree as `r0.c`: 1680/47, 530/530 instructions, all siblings exact,
and the guard passes. The remaining rows are the ones w1 recorded:

1. **Inlined site.** The `transform_clip_planes_to_box` site inside the 3x3x3 box loop emits `i*x + k*z + j*y`, with the plane
   loaded first. January emits `k*z + i*x + y*j`, with the third term loading the box component first. The same body is exact
   out of line, and the particle-distance loop is exact too.
2. **`position.x`.** Only the x row differs: ours loads the particle first, January loads the box first. The y and z rows already
   load the box first.

Laws checked:
- A22 dot canonicalisation: January's k,i,j order is context-dependent, not one of the canonical orders.
- A22 younger operand: both operands in row 2 are pointer reads.
- A4 helper routing: the box stores do not show the all-loads-then-all-stores pattern.
- A5: the pointer is proven by `test ecx,ecx`.

No non-permutation source difference follows from these laws, and R3 forbids load-order operand swaps. No new shapes were
spent. Reopen on a law that predicts inline-site commutative term order from caller context, with an exact precedent. Start
from `ra.c`.

## Do-not-repeat (additions)

- update_physics: COMDAT-free spellings are covered by the fresh-graphs controls; the per-TU helper rename is inadmissible.
- render: every w1 and fresh-graphs item still applies.
