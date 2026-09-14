# `aim_assist.obj`: Opus5 fresh-graphs lane (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/game/aim_assist.c`, target `build/split/source/game/aim_assist.obj`
  (January 2002 Xbox debug build).
- Compiler: VC7 `13.00.9254` (`xbox/bin/vc7/CL.Exe`), the repository's unchanged flags
  (`/O2 /Oy- /DDEBUG /Dxbox`). No flag, header, or config change was made.
- Lane `opus/30k-fresh-graphs-20260914` at HEAD `47d9dd0da` (canonical base `c3e257e9a`).
- Evidence used: January COFF (code, relocations, symbol storage classes), January map atlas
  `scratch/atlas/aim_assist.obj.txt`, Ghidra 12.1.2 decompilation
  `scratch/ghidra/out/aim_assist.obj.decomp.c`, HCEA `game/player_aim_projectile.c`,
  `objects/object_compute_autoaim_target.c`, `game/local_player_aim_assist.c`,
  `game/find_aim_assist_targets.c` (for meaning and names only, never layout).
- Prior ledgers read: `aim_assist_obj_jonas_helper_wave_20260821.md`,
  `claude_ui_weather_aim_render_hs_reconciliation_20260908.md`,
  `lane_reconciliation_10k_batch_7_20260912.md`, `weapons_owner_packet_source_park_20260913.md`,
  `research/reconstructions/player_aim_projectile_20260912.md`. `branch_sweep`: no other blob has
  more exact functions (best 61d5d839 has 7).

## 2. Validated baseline and result

Gate: `python tools/campaign/gate.py source/game/aim_assist --all --forbid-emitted-symbol _point_from_line3d`

| | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| Baseline (lane HEAD) | 9 | 5 | 2 |
| Final (real file) | **10** | 5 | 1 |

Newly strict exact: `_object_compute_autoaim_target` (416 padded, 7 relocations,
normalized sha `f1d2a44acd553fdd`).

Fuzzy (zero credit): `_player_aim_projectile` (832/42, all 42 relocation identities equal,
sha `68607485f783d500` vs target `79f83e1a1b6ec5a6`; one instruction placement).

Remaining rows:

| Function | Target size/relocs/sha16 | Final sha16 | First divergence |
| --- | --- | --- | --- |
| `_player_aim_projectile` | 832/42 `79f83e1a1b6ec5a6` | `68607485f783d500` | `push ecx` for the second `normalize3d` placed before the impact-subtraction stores (January: after) |
| `_aim_assist_compute_target` | 416/16 `03e4e539489a5744` | `a9833e9ce7a975ba` | callee-saved register choice around the inlined composite attenuation (see §4) |
| `_find_aim_assist_targets` | 304/6 `2f48dbfde992d379` | `f43202b963fbf750` | argument-load order for the recursive call |
| `_local_player_aim_assist` | 368/12 `f0db5ad90b1f9b05` | `9e2c79003ab787d9` | frame `sub esp,0x6c` vs January `0x68` |
| `_limit3d` | 96/0 `4ba268790768577e` | `d1fb9fe17bcedd9c` | branch polarity (header-owned body) |
| `_reciprocal_square_root` | 16/1 `6675eafcdc04bcb2` | unwritten | header-owned; see proposal |

Data: this lane makes no data claim.

## 3. Accepted controls (retained in production)

1. **`object_compute_autoaim_target` pill normal as statement-level components.**
   `cross_product3d(&height, direction, &normal)` became three explicit assignments
   (`normal.i= height.j*direction->k - height.k*direction->j;` ...). Evidence: January's sum
   orders (kk, jj, ii) in both the normal's squared magnitude and the cross/normal dot differ
   from every helper-boundary form tried in lab probes (header `cross_product3d` with k,j,i
   temporaries; i,j,k temporaries, static and non-static `__inline`; plain assignment inline),
   and are reproduced exactly only by statement-level components. The second cross product
   (offset x direction) must remain the header inline (components there move 2 instructions).
   This is stated explicitly against the "avoid inlining" house rule: the change is not a
   hand-expansion to steer a tie but the only form January's bytes admit, and it is the same
   idiom as the already-exact `aim_assist_clear_line_of_sight`. Semantics are unchanged.
2. **`player_aim_projectile` fuzzy body** (complete, typed, January order between `aim_assist`
   and `local_player_aim_assist`). Frame-proven structure: the aim-assist `target` and the
   collision record share stack (collision must be in a nested block; fully flat grows the frame
   to 0xf4), `vector` is reused for the `fast_normals_interpolate` result (a separate variable
   grows the frame to 0xc8), `impact_direction` shares the trace block (nesting it shrinks the
   frame to 0xb0). Statement order proven: position advance before the 128-unit ray scale.
   The in-place `scale_vector3d` + `add_vectors3d` (point cast as vector, precedent in
   `device_machines.c` and `collision_features.c`) is proven by the scaled-k spill; explicit
   component adds and `point_from_line3d` (forbidden COMDAT) both diverge.
   `pin_normal_to_cone3d` is called out-of-line in January (reloc) and is defined in
   `real_math.c` but has no header prototype, so the TU carries an ABI-identical file-local
   prototype pending the orchestrator proposal below.

## 4. Experiment matrix

`player_aim_projectile` (all 832/42):

| Shape | sha16 | Result |
| --- | --- | --- |
| archived nested-block candidate (v1) | 68607485 | 1-insn tie |
| plain `if (normalize3d(&impact)==0.f)` | 68607485 | identical |
| `normalize3d(vector_from_points3d(...))` | 68607485 | identical |
| explicit impact subtraction | 68607485 | identical |
| `add_vectors3d(position, direction, position)` | 68607485 | identical |
| impact_direction in nested block | frame 0xb0 | rejected |
| 128 scale before position advance | worse | rejected |
| `point_from_line3d` | guard failure | rejected |
| explicit component adds | 4a850e23 | rejected |
| separate interpolation-result vector | frame 0xc8 | rejected |
| flattened (target in if-scope, one trace block) | 68607485 | **retained** |
| fully flat | frame 0xf4 | rejected |

`object_compute_autoaim_target`: `dot_product3d(&normal,&normal)`, swapped dot args, both:
22d494ed (identical); inline cross probes 22d494ed / d321b6ae / 86ac4d89; components for the
first cross only: **f1d2a44a EXACT (retained)**; components for both: 032db526.

`aim_assist_compute_target`: January storage classes show `_compute_attenuation` and
`_compute_composite_attenuation` are EXTERNAL (class 2; ours static, class 3) and the atlas marks
`compute_attenuation` as an inline COMDAT (`i`). Non-static `__inline real compute_attenuation`
(lab) makes the register assignment match January exactly (sha 0ba9133e); plain non-static or
`__inline` on the composite alone change nothing. Remaining gap under that form: the magnetism
composite evaluates the angle attenuation first (reusing the dead `angle` home as temp).
Declaration-order variants, no `cosine` local, and named attenuation locals in the composite did
not help (the last breaks the composite). Not landed (not exact).

`find_aim_assist_targets`: `targets + target_count`, `(short)(maximum - count)`: identical.

`local_player_aim_assist`: locals at function scope, separate `player_index` local: identical;
explicit relative-velocity components: yaw block moves toward January, frame gap unchanged.

`limit3d` / `reciprocal_square_root` (lab probes shadowing the header body, never landed):
positive-test `if (dot > length*length) { scale; return TRUE; } return FALSE;` makes `_limit3d`
exact; with the scale factor written `length*reciprocal_square_root(dot)` both `_limit3d` and a
naturally emitted `_reciprocal_square_root` are exact (12/16 exact, 0 unwritten, all siblings
kept). A single-exit flag form is rejected (5d474209).

## 5. Do-not-repeat

- Re-spelling the second `normalize3d`/impact subtraction in `player_aim_projectile` (five forms
  CSE-identical); `point_from_line3d` there or in `object_compute_autoaim_target`.
- Declaration-order or composite-local variants for `aim_assist_compute_target`.
- Pointer-arg spelling/casts for the `find_aim_assist_targets` recursive call.
- Writing `_reciprocal_square_root` in `aim_assist.c` without the header `limit3d` change (no
  genuine in-TU caller exists in the current header).

## 6. Residual classification

- `player_aim_projectile`: scheduling (argument push placement after a call with pending stack
  adjust). Measured.
- `aim_assist_compute_target`: ABI/linkage (measured, lever found) + scheduling (temp/evaluation
  order, inference).
- `find_aim_assist_targets`: scheduling (register load order). Measured.
- `local_player_aim_assist`: ABI/register home assignment causing a structural frame gap, plus
  x87 scheduling. Measured; cause of the register choice unresolved.
- `limit3d`, `reciprocal_square_root`: source/topology in a shared header. Measured by probe.

## 7. Orchestrator proposals

1. `source/math/real_math.h` `limit3d` body -> positive test with
   `scale_vector3d(vector, length*reciprocal_square_root(dot), vector); return TRUE;` then
   `return FALSE;`. Unlocks `_limit3d` + `_reciprocal_square_root` in this object. Gate the
   inlined callers `real_math.c:3026`, `vehicles.c:1233,2183` and the whole board.
2. `source/math/real_math.h`: prototype
   `boolean pin_normal_to_cone3d(real_vector3d const *normal, real_vector3d const *direction, real sine, real cosine, real_vector3d *result);`
   (then drop the file-local prototype in `aim_assist.c`). Declaration-count hazard: whole board.
3. `source/game/aim_assist.h`: `long player_aim_projectile(long player_index, real_point3d const *position, real_vector3d *direction);`
   Consumers: `aim_assist.c`, `game_engine.c`, `player_control.c` (and weapons.c when it calls it).

## 8. Reopen criteria and disposition

- `object_compute_autoaim_target`: strict exact, eligible.
- `player_aim_projectile`: active fuzzy; reopen only with a source/scheduling explanation for the
  deferred argument push.
- `aim_assist_compute_target`: reopen with the `__inline` compute_attenuation form plus new
  evidence for the magnetism pair order.
- `local_player_aim_assist`: reopen with evidence that changes the register allocation of
  `local_player_index`/`target_angular_position` (frame 0x68).
- `find_aim_assist_targets`: reopen only with a different argument expression/type.
- `limit3d`, `reciprocal_square_root`: reopen when proposal 1 is applied.
- Object remains `NonMatching` (10/16).

## Lead admission notes (2026-09-14)

- `_object_compute_autoaim_target` (416 padded / 413 meaningful) is admitted
  strict exact: the pill normal is written as the three explicit
  height x direction components, the same form this function already uses for
  its perpendicular vector.
- `_limit3d` (96) and `_reciprocal_square_root` (16) became strict exact
  through the separate header prerequisite `c86aa48bd`, which corrected the
  real_math.h `limit3d` inline to January's branch polarity. No aim_assist.c
  source change was needed for them.
- `_player_aim_projectile` is **not landed** (remains unwritten, zero credit).
  Its only compliant prototype placement for `pin_normal_to_cone3d` is
  `source/math/real_math.h`, and a bisection proved that adding that single
  declaration regresses three unrelated exact functions and drifts four parks
  (declaration-count sensitivity). A consumer-local prototype is not admissible.
  The complete 832/42 fuzzy body is preserved at
  `scratch/workers/aim_assist/player_aim_projectile_fuzzy_withheld.c`.
  Reopen when real_math.h can take the declaration without regressions (for
  example together with a planned whole-board real_math.h reconciliation).
