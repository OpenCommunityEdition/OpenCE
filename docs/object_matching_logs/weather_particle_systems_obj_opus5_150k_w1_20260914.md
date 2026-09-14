# `weather_particle_systems.obj`: Opus 5 150K house-clean, wave w1 (2026-09-14)

This wave covered the three written residuals in
`source/effects/weather_particle_systems.c`. **No source change landed.** No
function became strict EXACT under the current headers. Two functions are
strict EXACT in scratch once a measured header packet is applied, so both are
owner-blocked (see section 5). The render residual went from a register-level
mismatch to five x87 operand-order rows.

## 1. Baseline and scope

- Unit `source/effects/weather_particle_systems`; target `build/split/.../weather_particle_systems.obj`.
- Lane base `12f7375d4`. Real-file gate with `--forbid-emitted-symbol _point_from_line3d`:
  **exact 17 / residual 3 / unwritten 0**, and the guard passes. The unit has no parks.
- Residuals at baseline (target padded / relocs / sha16; real file sha16):
  - `_weather_particle_system_new_particle`: 704 / 41 / `7f9d6a12746c868b`; real `5ab106d3f8f116ef`
  - `_weather_particle_update_physics`: 688 / 29 / `da0e0a2b59f80ca1`; real `9f12c1d59196eafe`
  - `_weather_particle_system_render`: 1680 / 47 / `823416f911099e6a`; real `dcf592bb687b0930`
- Ledgers read before starting: `weather_particle_systems_obj_jonas_new_first_shot_20260830`,
  `_delete_reconciliation_20260907`, `claude_ui_weather_aim_render_hs_reconciliation_20260908`,
  `weather_particle_systems_obj_opus5_fresh_graphs_20260914` (and its worker notes/candidates),
  `particle_systems_obj_opus5_100k_20260914`, `glow_obj_house_rule_reconciliation_20260908`.
- The fresh-graphs candidates were copied unchanged as `scratch/workers/weather_particle_systems/p_n1.c`,
  `p_ve.c` and `p_r7.c`. They have the same base file and were re-measured identically.

## 2. New finding: a named value is behind the deferred x87 store

Fresh-graphs left this reopen criterion open: "what controls VC7 post-call reload / x87 store deferral".

- **Tree scan.** `scan_postcall.py` and `scan_defer.py` walked every January split object. After a call to
  `_real_seed_random_range` or `_real_seed_random`:
  - 81 accepted-exact sites store the x87 result immediately.
  - No accepted-exact site defers the store.
  - All 8 deferring sites are non-exact: weather x2, particle_systems new_particles, glow trailing_particle_new,
    decals new_from_collision, environment fog x2 and transparent geometry.
  - Exact deferral exemplars elsewhere (`uniform_cubic_spline_vector3d`, `path_state_build_path`) show VC7
    postponing an `fstp` past stack reloads, register work and stores to other offsets. It flushes before a
    heap read, a call or an x87 use.
- **Synthetic VC7 proof.** `syn/t1.c` and `syn/t2.c` have the same caller and differ only in the inline body:
  - `return real_seed_random_range(...)` stores immediately.
  - `real result= real_seed_random_range(...); return result;` defers the store, which is January's pattern.
- **In-TU confirmation.** This was an experiment only, since a local helper copy is inadmissible.
  - `x1.c` is n1 plus a named-result `real_local_random_range`. **new_particle is strict EXACT** and all 17
    siblings stay exact.
  - The control `x_n1plain.c` uses the same override mechanics with the plain body. It stays residual.

## 3. Shadow-tree blast radius for the header packet

Harness: `scratch/workers/weather_particle_systems/shadow/blast2.py` / `blast3.py`.

- HEAD `source/` is `git archive`d into `treeA`, `treeB` and `treeC`, each with an `xbox` junction.
- Every build.ninja unit is compiled with ninja's exact relative command line from the tree root. Per-function
  strict status is compared with `build/split`.
- A first harness only prepended a shadow `/I` directory. It was not faithful: finding an unmodified
  `real_math.h` copy at a different path flipped `units::_unit_preprocess_node_orientations`.

| tree | change | result over 446 units |
| --- | --- | --- |
| B | named result in `real_local_random_range` only | **1 loss** (`_unit_preprocess_node_orientations`, the declaration-count effect); park gain for glow; 2 park drifts |
| C | B + delete the verbatim duplicate `ai_reconnect_to_structure_bsp` prototype from `ai/ai_runtime.h` (ai.h keeps it) | **0 losses**; A 6676 → C 6677 exact; glow `_glow_trailing_particle_new` (parked) EXACT; park drifts `__rasterizer_model_draw`, `_bitmap_group_add_bitmap`; non-parked residual hash change `_rasterizer_frame_statistics_draw` |
| D | C + named result in `real_local_random` | no gain, units loss again. Rejected |

Weather under C:

- Real file plus `#include "bitmaps/bitmaps.h"` (p_n1): new_particle EXACT, 18/20.
- `cand_physics.c` (p_n1 plus the fresh-graphs ve body, tidied): 19/20, but it emits `_point_from_line3d`.

## 4. Per-function outcomes

### `_weather_particle_system_new_particle`: OWNER-BLOCKED (header packet C)
- Strict EXACT in scratch under C with only the `bitmaps/bitmaps.h` include added to the real file.
- Shapes tried:
  - a1, a named acceleration magnitude local: inert.
  - a2, HCEA link order: inert.
  - x1 (named result inline): EXACT.
  - Control x_n1plain: residual.
- Not landed: the header is orchestrator-owned, and a written residual lands only when exact.
- Reopen: packet C is admitted. Then add the include, which is candidate `p_n1.c`.

### `_weather_particle_update_physics`: OWNER-BLOCKED (C + point_from_line3d emission policy)
- The fresh-graphs single residual (`mov ecx,[ebp-8]` before `fadd`) is the same named-value deferral.
- `p_ve.c` and `cand_physics.c` are strict EXACT under C, but they emit the forbidden 48-byte `_point_from_line3d` COMDAT.
- `cand_physics.c` uses `(real_point3d *)` casts for the vector arguments, following the accepted exact
  precedents units.c:11133 and real_math.c:2567.
- Reopen: C is admitted AND there is a mechanism or ruling that inlines point_from_line3d without emitting it.

### `_weather_particle_system_render`: NOT-LANDED (structural recovery; 5 x87 operand-order rows)
All shapes start from fresh-graphs r7 (1664/47, 80 blocks). Every row kept 17 siblings exact and passed the guard.

1. **e1: `box_count = 1;` just before the bounds stores.** January's `mov ebx,1` sits inside the bounds
   sequence, after the last call.
   - Result: 1680 padded, and the register and slot allocation now match (type in ESI/[ebp-8], system kept
     in EBX). Blocks fell from 80 to 27.
   - Placement controls: at declaration 46 blocks, after bounds 43, after offsets 82.
2. **e6: e1 plus a block-scoped `real box_width = type->box_width;` for the bounds.**
   - Reproduces `fld [type+4]; fadd st(1)` x3, then `fstp st(0)`, then a memory reload for the offsets.
   - The synthetic t8 vs t3/t5/t6/t7/t9 comparisons show that only a named value whose lifetime ends after
     the bounds gives this. Instruction count becomes 530/530.
3. **ra: e6 plus `dot_product3d(&planes[i].n, (real_vector3d const *)box_position)` in
   `transform_clip_planes_to_box`.** The standalone function stays EXACT, and the particle clip-distance row closes.
   - Remaining rows: inner inline transform-site term order (4 rows) and position.x load order (1 row).
   - Probes rb (`plane3d_distance_to_point` for particle distances) and rc (`dot_product3d(point, n)`) were worse.

The best candidate is `scratch/workers/weather_particle_systems/ra.c`, 1680/47. It was not landed. Its least
proven element is the nested block around the bounds width.

## 5. Orchestrator proposals

1. **Header packet C.**
   - `source/math/real_math.h`, `real_local_random_range`: replace
     `return real_seed_random_range(get_global_local_random_seed_address(), lower_bound, upper_bound);` with
     `real result= real_seed_random_range(get_global_local_random_seed_address(), lower_bound, upper_bound);`,
     a blank line, then `return result;`.
   - `source/ai/ai_runtime.h`: delete the duplicate `void ai_reconnect_to_structure_bsp(void);`.
   - Consumers: `scratch/workers/weather_particle_systems/shadow/consumers_real_math_h.txt` (188 files) and
     `consumers_ai_runtime_h.txt` (8). Direct callers of the inline: bored_camera, dead_camera, contrails,
     particle_systems, particles, weather_particle_systems, first_person_weapons, glow, breakable_surfaces,
     environment_fog and sound_manager.
   - Required park re-measures: `__rasterizer_model_draw` 5168/348 →
     `112b4321bee9992ef063e93af69c91faba64a7ecf830fa5b7ff4e657832c26ee`; `_bitmap_group_add_bitmap` 992/59 →
     `0bd88e42b1b36f4e9df5a8bed007887a18a1443d6f895f15ea9d275f2f6526c4`.
   - Isolated header tests do not compose. On the live tree, re-gate `units --fn _unit_preprocess_node_orientations`
     and the parks.
2. **Weather `#include "bitmaps/bitmaps.h"`**, together with packet C. It fixes C4013 (a `real` argument
   pushed as a double) and makes new_particle exact.
3. **point_from_line3d emission ruling** for weather update_physics.
4. **Park reopen:** glow `_glow_trailing_particle_new`. The natural body is unchanged and becomes strict EXACT
   under C. The park's rejected Opus donor needed named spill temporaries. This lever supplies the named value
   inside the genuine header inline instead.

## 6. Do-not-repeat

- Named-result `real_local_random` (tree D): no gain, and it breaks units.
- Weather new_particle: a named acceleration magnitude local, and the HCEA link-store order (both inert).
- Render: `short box_count = 1;` at declaration; box_count after the bounds or after the offsets;
  `plane3d_distance_to_point` for particle distances; point-first `dot_product3d` in the transform helper.
- Do not use a shadow `/I` prefix for header blast tests. Compile from a copied tree root instead.

## 7. Checks and disposition

- Final real-file gate: exact 17 / residual 3 / unwritten 0. The guard passes, and `git diff --stat` is empty.
- Owner census against `build/base`: identical (only the `.debug$S` path differs).
- Fake scan: 0 leads for the real file, `ra.c`, `cand_physics.c` and `p_n1.c`.
- Worker notes: `scratch/workers/weather_particle_systems.md`.
- Evidence scripts: `scan_postcall.py`, `scan_defer.py`, `syn/`, `shadow/blast2.py`, `shadow/blast3.py`.
