# `aim_assist.obj`: Opus5 150K house-clean lane, wave w1 (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/game/aim_assist.c`; target `build/split/source/game/aim_assist.obj`
  (January 2002 Xbox debug build, VC7 13.00.9254, unchanged repository flags).
- Lane `opus/150k-house-clean-20260914`, base `12f7375d4`. Only `source/game/aim_assist.c` was modified.
  No header, config, parked or build file was touched.
- Prior ledgers read: `aim_assist_obj_jonas_helper_wave_20260821.md`,
  `aim_assist_obj_opus5_fresh_graphs_20260914.md`, `opus5_100k_consolidated_HANDOFF_20260914.md`,
  `research/reconstructions/player_aim_projectile_20260912.md`, and the fresh-graphs worker notes
  (`opus5-30k-fresh-graphs-20260914/scratch/workers/aim_assist.md`, which list 15 player_aim_projectile shapes).
- `branch_sweep`: the current blob is the best (12 exact). `git log --all` shows no other exact body.
- Evidence used:
  - January COFF bytes, relocations and symbol storage classes;
  - the map atlas `scratch/atlas/aim_assist.obj.txt`;
  - HCEA `game/find_aim_assist_targets.c`, `game/local_player_aim_assist.c`, `game/player_aim_projectile.c`,
    `objects/aim_assist_compute_target.c` and `objects/compute_composite_attenuation.c` (names and topology only);
  - codebase idioms.

## 2. Baseline and result

Gate: `python -B tools/campaign/gate.py source/game/aim_assist --all --forbid-emitted-symbol _point_from_line3d`

| | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| Baseline (real file at lane base) | 12 | 3 | 1 |
| Final (real file) | **13** | 2 | 1 |

**Newly strict exact:** `_find_aim_assist_targets` (304 padded, 6 relocations, sha16 `2f48dbfde992d379`).

A row diff of baseline against final shows exactly one change, that function. The final gate also shows:
- the point guard passes;
- the unit has no parks;
- owner census against `build/base`: no candidate-only or base-only owners;
- fake-match scan: 0 leads at HEAD and 0 at final.

| Function | Target (size/relocs/sha16) | Outcome |
| --- | --- | --- |
| `_find_aim_assist_targets` | 304/6 `2f48dbfde992d379` | **EXACT, landed** |
| `_player_aim_projectile` | 832/42 `79f83e1a1b6ec5a6` | owner-blocked; scratch candidate 832/42 `68607485f783d500` (1 push placement) |
| `_aim_assist_compute_target` | 416/16 `03e4e539489a5744` | not landed; real file `a9833e9ce7a975ba` |
| `_local_player_aim_assist` | 368/12 `f0db5ad90b1f9b05` | not landed; real file `9e2c79003ab787d9` |

## 3. Landed change: `find_aim_assist_targets`

Before the change, only the argument loads for the recursive call differed.
- **January:** computes `&targets[target_count]` first (in ecx). It materialises the object index last, in
  the index register: `movsx eax,bx; mov eax,[ebp+eax*4-0x2410]; push ecx; push eax`.
- **Our build:** loaded the array element early into ecx, which displaced the pointer into eax.

A named element local is materialised at its use, which is the element/accessor binding pattern. The source
now binds the current element with the house idiom used by three other `objects_in_clusters` consumers:
`source/ai/actor_moving.c:777`, `source/ai/path_obstacles.c:278` and `source/physics/physics.c:1208`.

```c
short object_number;

for (object_number= 0; object_number<object_count; ++object_number)
{
	long object_index= object_indices[object_number];

	target_count+= find_aim_assist_targets_recursive(parameters, object_index, position, direction, ...
```

Semantics are unchanged.

Rejected alternative: a named `found_count` result local (HCEA topology) produced the baseline sha.

## 4. Not landed

### `player_aim_projectile` (832, unwritten)

It has two blockers.

1. **Owner.** `pin_normal_to_cone3d` (real_math.c:2351) has no header prototype.
   - A consumer-local prototype is inadmissible.
   - The real_math.h placement regressed distant functions in the prior lane, so it is not repeated here.
   - `build/split` shows that January `aim_assist.obj` is its only caller.
2. **Residual.** One scheduling tie remains in the complete fuzzy body. The `push ecx` for the second
   `normalize3d` sits after the three impact-direction stores in January (@0x289); ours sits after the
   first `fsub` (@0x26e). Every other instruction and all 42 relocations align.

New shapes this wave, all sha `68607485`:
- `collision` scoped to a block ending after the impact subtraction;
- `normalize3d(vector_from_points3d(...))` at the second site only;
- the prototype at the focused-header include position;
- `impact_direction` declared with `aim_direction`.

Candidate: `scratch/workers/aim_assist/player_aim_projectile_candidate.c`. It is the current real file plus the
body, plus a token-identical stand-in for the proposed header. Its 13 sibling rows are unchanged.

### `aim_assist_compute_target` (416)

The register half of the fresh-graphs lever is reproduced and confirmed:
- January COFF storage classes: `_compute_attenuation` and `_compute_composite_attenuation` are external (class 2).
- The atlas marks `compute_attenuation` as inline (`i`).
- A non-static `__inline real compute_attenuation` gives January's registers and zero register.

The remaining difference is the magnetism composite. VC7 evaluates the angle attenuation first, so the dying
`angle` home `[ebp+0x18]` takes the product temporary; January keeps the temporary at `[ebp+0x14]`.

Shapes tried:
- in-block `distance`/`angle` read back from `target` (store order wrong);
- only `distance` in-block: pair order becomes January's, but a split-home copy appears;
- only `angle` in-block;
- an explicit product, which is identical, so the ordering is liveness-driven.

Not exact, so not landed.

### `local_player_aim_assist` (368)

The frame gap is structural: 0x6c against January's 0x68.
- January's frame is exactly target, parameters and two velocity vectors.
- January enregisters `local_player_index` in ebx (reloaded from `[ebp+8]` after the private-ABI call),
  `target_angular_position` in edi and `target_angular_velocity` in esi. It homes `player` and
  `aiming_unit_index` in the two pointer parameters' homes.
- Ours gives `local_player_index` edi, so `player` needs `[ebp-4]`.
- The x87 half: January multiplies as `fld [vector]; fmul st(r)`, keeps ri/rj alive until the epilog, and
  consumes rk in place.

Shapes tried:
- HCEA early returns (352 bytes);
- a single-exit result local (result lands in ebx; frame unchanged);
- inline relative-velocity expressions (352; the subtractions become lazy);
- velocity locals declared before `target` (identical).

## 5. Do-not-repeat (in addition to the fresh-graphs list)

- `player_aim_projectile`:
  - collision-lifetime blocks around the trace;
  - nested `normalize3d(vector_from_points3d())` at site 2;
  - `impact_direction` declaration placement;
  - prototype position.
- `aim_assist_compute_target`: in-block distance/angle read-backs; an explicit magnetism product.
- `local_player_aim_assist`:
  - early returns;
  - a result local;
  - inline relative expressions;
  - declaration placement.
- `find_aim_assist_targets`: done. Do not revert to the direct `object_indices[...]` argument.

## 6. Orchestrator proposal

**New focused owner header `source/math/real_math_cones.h`.** Precedent: `math/real_math_planes.h` and
`math/matrix_math.h`, commit `6c7ab19db`.
- Content: the standard focused-owner preamble, then includes of `cseries/cseries.h` and `math/real_math.h`,
  then:

  ```c
  boolean pin_normal_to_cone3d(real_vector3d const *normal, real_vector3d const *direction, real sine, real cosine, real_vector3d *result);
  ```

  (one parameter per line). The exact text is in `scratch/workers/aim_assist.md`.
- Sole consumer: `source/game/aim_assist.c`, with `#include "math/real_math_cones.h"` after `items/weapons.h`.
- Alternative: append the prototype to `math/real_math_planes.h`, whose consumers `collisions.c` and
  `physics.c` must then be gated.
- Credit: this unlocks landing the complete `player_aim_projectile` body at zero credit now, and full credit
  once the push tie closes.
- Reviewer caution: the body keeps `add_vectors3d` with point-to-vector casts for the in-place camera
  advance. There is precedent (device_machines.c:341, collision_features.c:139), but the 20260912 research
  note flagged it.

## 7. Reopen criteria

| Function | Reopen when |
| --- | --- |
| `player_aim_projectile` | The header proposal is applied, **and** a source explanation exists for the deferred second `normalize3d` push. (Site 2 differs from site 1 only in following a call in the same block.) |
| `aim_assist_compute_target` | January-grounded evidence shows `distance`/`angle` liveness past the magnetism composite, combined with the non-static `__inline compute_attenuation`. The in-block `distance` shape (`tb`) is the closest lead. |
| `local_player_aim_assist` | New evidence changes the `local_player_index` versus `target_angular_position` register weights, or gives a January-grounded spelling of the angular-velocity math that produces `fld [vector]; fmul st(r)`. |

The object remains NonMatching (13/16).
