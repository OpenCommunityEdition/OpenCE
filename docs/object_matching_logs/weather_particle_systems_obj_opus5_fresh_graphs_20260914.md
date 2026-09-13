# `weather_particle_systems.obj` — Opus 5 fresh-graphs wave 2 (2026-09-14)

Residual-only wave on `source/effects/weather_particle_systems.c`. **No source
change was landed**: none of the three written residuals reached strict EXACT
under an admissible source form. This ledger records the structural recoveries
that were proven against January bytes, so the next lane starts from them
rather than from the baseline body.

## 1. Scope and provenance

- Unit `source/effects/weather_particle_systems` → January split
  `build/split/source/effects/weather_particle_systems.obj`.
- Compiler: VC7 13.00.9254 `CL.Exe`, `/nologo /c /O2 /Oy- /DDEBUG /Dxbox`
  (build.ninja flags; January build is flag-uniform, no per-unit flag lever).
- Lane `opus/30k-fresh-graphs-20260914`, worktree
  `C:\halo-worktrees\opus5-30k-fresh-graphs-20260914`.
- Evidence used: January COFF + relocations (authority), Ghidra decomp
  (`scratch/ghidra/out/weather_particle_systems.obj.decomp.c`), HCEA
  `effects/weather_particle_{update_physics,system_new_particle,system_render}.c`
  (later build, semantics only), Stian build-2276 lift
  `src/halo/effects/weather_particle_systems.c` (FUN_000a4310 / FUN_000a4610 /
  weather_particle_system_render).
- Prior ledgers read: `weather_particle_systems_obj_jonas_new_first_shot_20260830.md`,
  `weather_particle_systems_obj_delete_reconciliation_20260907.md`,
  `claude_ui_weather_aim_render_hs_reconciliation_20260908.md` (which retained
  these three bodies as fuzzy with no recorded shape matrix).

## 2. Validated baseline (lane HEAD, unchanged at close)

`gate.py --all --forbid-emitted-symbol _point_from_line3d`:
**exact 17 / residual 3 / unwritten 0 (of 20)**, guard passes. No parked
entries for this unit.

| Residual | Target padded / relocs / sha16 | Base padded / relocs / sha16 | First meaningful divergence |
| --- | --- | --- | --- |
| `_weather_particle_update_physics` | 688 / 29 / da0e0a2b59f80ca1 | 688 / 29 / 9f12c1d59196eafe | `sub esp,0x24` vs `0x20` (frame) |
| `_weather_particle_system_new_particle` | 704 / 41 / 7f9d6a12746c868b | 704 / 41 / 5ab106d3f8f116ef | position.z `fstp` placement; rgb argument pushed as double |
| `_weather_particle_system_render` | 1680 / 47 / 823416f911099e6a | 1680 / 47 / dcf592bb687b0930 | `sub esp,0x4dc` vs `0x4cc` (frame) |

## 3. Accepted controls

The production file is unchanged (`git diff --stat` empty). All experiments
live under `scratch/workers/weather_particle_systems/` (`ve.c`, `n1.c`, `r7.c`
are the best candidates).

## 4. Experiment matrix (whole-TU gate each time; siblings stayed 17/17 exact in every row)

### `_weather_particle_update_physics`

| Shape | Result | Kept? |
| --- | --- | --- |
| v1: velocity integration via `point_from_line3d(&velocity,&acceleration,dt,&velocity)` | 688/29; reproduces January's `fld dt; fld st(0); fmul v.i; fadd p.x` schedule; frame still 0x20; **guard fails** (`_point_from_line3d` COMDAT) | no |
| va: explicit `real_vector3d *acceleration` local | frame 0x24 but j/k become pointer-relative (January uses particle-relative `[edi+0x20]`) | no |
| vd/ve: `point_from_line3d` for velocity AND jitter, `seed = particle_index` assigned before flags/call inside a `{ unsigned long seed ... }` block | 688/29, sha16 663a23cface911fe, **one** residual block: January `mov ecx,[ebp-8]` (type_definition reload) before `fadd [ebp-4]`; guard fails | no (policy) |
| tie probes on ve: `m = m + rr()`, `m = rr() + m`, explicit if/else-if clamp, separate new-magnitude variable (HCEA) | same reload/fadd order (last one worse) | no |
| controls on ve: jitter as explicit `+= jitter->i*0.001f` | 51 blocks | no |
| controls on ve: velocity as explicit `+= accel.i*dt` | 38 blocks | no |

Measured fact: both `point_from_line3d` inlines are load-bearing for January's
bytes (the controls regress sharply). Inference: January's weather TU inlines
`point_from_line3d` twice in this function.

### `_weather_particle_system_new_particle`

| Shape | Result | Kept? |
| --- | --- | --- |
| n1: `#include "bitmaps/bitmaps.h"` (fixes `CL /W3` **C4013 rgb_colors_interpolate undefined**) | 704/41, sha16 b9462009b229fcde; rgb region now identical; 3 residual swaps: position.z `fstp` deferral, acceleration random-range lower-bound homing order, color.alpha `fstp` deferral | no (not exact) |
| n2: Stian form, `real_local_random()` passed directly | worse (January uses the named float temp) | no |
| n3: velocity zeroing as three k,j,i statements | inert vs n1 | no |

### `_weather_particle_system_render` (all on top of n1)

| Shape | January evidence | Result |
| --- | --- | --- |
| r1 `real far_clip_distance = type->box_width;` after `weather_polyhedra_find`, used by `build_clipping_planes` and the fade-out MIN (`fade_out_end`) | `[ebp-0x18]` float copy made after the find call, read only at those two sites | copy pattern matches; frame 0x4d0 |
| r2 box_bounds stores x0,y0,z0,x1,y1,z1 | January store order | store order matches |
| r3 `real_point3d *box_position = NULL` set in the visible-box loop, `if (box_position)` | `lea ecx,...; test ecx,ecx; je` | matches |
| r4 `boolean occluded` flag for the weather-polyhedron test | occluded path jumps straight to the next particle; no index compare after the loop | matches |
| r5 `real_vector3d offset` copied from `box_offsets[i],[j],[k]` then added to bounds | 12-byte aggregate at `[ebp-0x84]`, only `.k` materialised at `[ebp-0x7c]` | **frame 0x4dc matches**; 3x3x3 block matches up to register names |
| r6 explicit `n.i*x + n.j*y + n.k*z - d` for particle clip distances | January term order i*x, k*z, j*y (the plane3d_distance_to_point inline gives k*z, i*x, y*j) | first two terms match, third operand order still swapped (point-first variant inert) |
| r7 sprite scale `runtime_one_over_sprite_width*(2.f*particle->radius)` | `fld radius; fadd st(0),st(0); fmul w` | matches (`radius*2.f*w` inert) |
| inert/worse: position operands flipped; type_definition before type (worse); `type` at function scope | — | reverted |

Best candidate r7.c: 1664/47 (padded size now 1664 vs 1680), 80 diff blocks.

## 5. Do-not-repeat

- update_physics: do not retry the magnitude-update spelling family
  (`+=`, `m = m + rr`, `m = rr + m`, if/else clamp, separate variable); do not
  use a pointer local for acceleration; do not write velocity/jitter explicitly.
- new_particle: dropping the named colour-interpolation local; separate
  velocity zero statements.
- render: `radius*2.f*w`; flipping position addition operands; point-first
  clip-distance spelling; moving `type` to function scope; declaring
  type_definition before type.
- Never land `point_from_line3d` in this TU while the ActionCharge
  NODUPLICATES ownership rule forbids the emitted COMDAT.

## 6. Residual classification

- update_physics: **policy-blocked + one scheduling tie.** Structure is solved
  (ve.c) but needs `point_from_line3d`; after that, one reload/`fadd` order swap.
- new_particle: **scheduling ties** after a genuine defect repair (n1): x87
  store deferral and homing order.
- render: **register-allocation tie** after structural recovery (r7.c):
  January puts `type` in ESI (spilled to `[ebp-8]`, box_width x87-CSE'd off an
  EAX reload) and keeps `system` in EBX across the type loop; ours puts `type`
  in EBX and reloads `system` at the loop head. All remaining deltas follow.
- Cross-function observation (inference): in all three functions January
  reloads a spilled pointer immediately after the `real_seed_random_range`
  call, before the x87 op consuming the result; ours reloads at the use. The
  common factor is the `real_local_random_range` inline, so a header-level
  inline-shape difference is a candidate cause (see
  real-math per-caller inline boundary notes); not proven.

## 7. Orchestrator proposals

1. **point_from_line3d emission policy for this TU.** January inlines it twice in
   `weather_particle_update_physics` (controls above). Options: admit the
   per-TU rename (`#define point_from_line3d weather_point_from_line3d_inline`
   around the real_math include, as actor_combat.c/action_charge.c do) despite
   no January call reloc, or a header mechanism that avoids emitting
   `_point_from_line3d` for fully inlined uses. Alone this yields no exact
   credit today (one tie remains).
2. **Admit the `bitmaps/bitmaps.h` include as a correctness fix.** It removes an
   implicit declaration that pushes an 8-byte double to a callee reading a
   4-byte `real` (live runtime defect), is byte-neutral for all 17 exact
   siblings, and is withheld only by the "written residual lands only if
   exact" rule.

## 8. Reopen criteria and disposition

- update_physics: proposal 1 resolved AND new evidence for the reload-before-
  `fadd` schedule. Start from `ve.c`.
- new_particle / all three: evidence about what controls VC7's post-call
  reload/x87 store deferral (e.g. a sibling January object with the same
  `real_local_random_range` idiom that is exact). Start from `n1.c`.
- render: a January-proven source reason for `type` to lose EBX (for example an
  additional `system` use inside the type loop). Start from `r7.c`; keep the
  recovered structure (far_clip_distance, offset vector, box_position pointer,
  occluded flag, bounds store order, diameter-first scale, explicit clip
  distances).

Disposition: object remains **17/20 strict exact**, active (not parked), no
credit claimed for any residual.
