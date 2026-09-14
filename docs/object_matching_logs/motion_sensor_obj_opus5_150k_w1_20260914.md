# `motion_sensor.obj` — opus5 150K house-clean lane, wave w1 (2026-09-14)

Unit `source/interface/motion_sensor`. Worker scope: `source/interface/motion_sensor.c` only.
All eight non-exact functions are parked, so every measurement below is a scratch candidate.
**The real file is unchanged.** Final real-file gate: `== exact 11 residual 8 unwritten 0`.
The `_point_from_line3d` guard passes and park drift is 0.

## Baseline

| Function | January padded / relocs | Real-file candidate | Status |
| --- | ---: | ---: | --- |
| `_blip_begin` | 112 / 10 | 112 / 10 | park, sha only |
| `_blip_type_get` | 352 / 21 | 368 / 21 | park |
| `_motion_sensor_blip_set_type_and_size` | 96 / 4 | 96 / 4 | park, sha only |
| `_motion_sensor_update` | 1168 / 43 | 1136 / 44 | park |
| `_render_blip` | 400 / 18 | 384 / 17 | park |
| `_render_motion_sensor` | 768 / 40 | 768 / 40 | park, sha only |
| `_should_draw_object` | 224 / 7 | 208 / 7 | park |
| `_update_motion_sensor` | 864 / 39 | 800 / 37 | park |

Inherited exact functions were not touched: `_motion_sensor_tick`, `_get_motion_sensor_data`, the initialize/dispose family, the tiny-point pair, `_blip_size_get`, `_should_track_object` and `_motion_sensor_draw_screen`.

Duplicate prevention covered:
- the two prior ledgers (`motion_sensor_obj_jonas_census_tick_one_shot_20260829.md`, `lane_reconciliation_batch5_fuzzy_ui_world_20260909.md`);
- the opus5 100K handoff;
- `git log --all`;
- `branch_sweep` (no blob better than the current file).

## Outcomes

### Park reopen candidate A: `scratch/workers/motion_sensor/candA.c`

candA is the current real file plus three bodies. Its whole-TU gate is `exact 14 residual 5`, with no baseline-exact row lost. The guard passes, the owner census against `build/base` and `build/split` finds no new owner, and the fake-match scan reports 0 leads.

1. **`_motion_sensor_blip_set_type_and_size` (96 / 4, EXACT).**
   - Evidence: January materialises zero twice with different widths. The not-a-unit path uses `xor al,al` and the invalid-size path uses `xor eax,eax`. Both feed one `mov [edi+3],al` store.
   - This proves a single result byte stored once, fed from a clamped `short` copy of `unit_definition->unit.blip_type`. The single-exit law applies.
   - HCEA uses the same result-variable topology.
   - Shapes tried:
     - HCEA verbatim: 128, the result is homed to the stack.
     - Short result with an else arm: 80, and `motion_sensor_update` inlined the helper.
     - `char blip_size` plus an in-branch `short size_type` clamped with `VALID_INDEX`: EXACT.
2. **`_should_draw_object` (224 / 7, EXACT).**
   - Evidence: the prologue has `xor bl,bl`, and the failure exit is `mov al,bl`. That is a FALSE-initialised boolean returned once (single-exit law).
   - First shape: `result = FALSE`, then nested firing/throwing → TRUE, otherwise `visible && moving`.
3. **`_blip_type_get` (352 / 21, EXACT).** Evidence from the January bytes:
   - The block layout (gunner test, team return, driver test that jumps back to the team block, dropship) is VC7 cross-jumping two identical seat-team returns. The source therefore tests the gunner and the driver in separate blocks.
   - `setne al` / `add al,3` / `inc al` means the `(enemy != FALSE) + constant` result is assigned to a `char` variable. The early-return spelling gives the 32-bit `neg`/`sbb` form.
   - The seat unit is bound before the call (the `add esp,0x10` merge).
   - The non-vehicle path re-fetches the local player's team as a 32-bit `long`. This re-fetch is January-proven: there is a second `local_player_get_player_index` / `datum_get` pair.
   - Rejected shapes: single-exit nesting (368); early-return return-width variants (352 but different block placement); inline duplicated seat blocks (400).

Parks that must be re-measured because a callee's ABI changed:

| Park | New size / relocs | New sha |
| --- | --- | --- |
| `_motion_sensor_update` | 1136 / 44 | `40e4819de2760a22` |
| `_update_motion_sensor` | 816 / 37 | `5bae910d9c27cad0` |

### Park reopen candidate B: `scratch/workers/motion_sensor/candB.c` (candA + `_blip_begin`)

- **`_blip_begin` (112 / 10, EXACT). This needs an owner ruling.**
  - The January callee reads `reference` in ESI, `in_multiplayer` at `[ebp+0xc]` and `local_player_index` at `[ebp+0x10]`. It never reads `[ebp+8]`.
  - The only caller pushes a literal 0 dword there (`push 0`, `add esp,0xc`).
  - The PC build shows the same contract (Stian `event_manager.c`: "param_2 is never read").
  - Name and type are not recoverable: `long 0` and `real 0.0f` emit identical bytes.
  - candB uses `long unused` with a disclosure comment. The repo already has `long unused` parameters in `actor_perception_find_sense_position` and `profile_rasterizer_stalls`.
- The caller `_render_motion_sensor` is a parked body. Its call site changes and it re-measures at 784 / 40 (`de9701add36aae94`). The outer loop head now aligns exactly as in January.

candB gate: `exact 15 residual 4`, no sibling loss, guard passes, census is clean, fake-match scan reports 0 leads.

### Owner-blocked exact body: `_motion_sensor_update` (1168 / 43, EXACT in `scratch/workers/motion_sensor/v13.c`)

Evidence recovered from January:
- The scan path uses `short local_player_indices[4]`, `short blip_counts[4] = {0}` (initialised after `local_player_count()`) and `real_point3d camera_positions[4]`.
- A camera position is fetched only when `unit_index != NONE`, and nothing is zeroed.
- The object z is flattened to the player's camera z. The range test is `distance_squared3d(camera, object) <= range²`, and it is skipped when `game_engine_running()`.
- `players_full` is a `long`. The full-player case is the else arm. `motion_sensor_blip_set_type_and_size` inlines.
- `done = FALSE` follows the header stores.
- The player-list increment comes before `local_player_get_next`.

**Blocker 1 (uninitialised read).** January leaves `camera_positions[i]` uninitialised for a local player whose unit is NONE. The object loop then reads it. The brief forbids preserving uninitialised-read bugs. The current parked body zeroes the positions, which January does not.

**Blocker 2 (owner census).** The census adds `_distance_squared3d` and `_vector_from_points3d` inline COMDATs.

This body is reported as an orchestrator proposal, not as a park reopen.

### Not landed

**`_render_blip`**
- Recovered from January:
  - single-exit range test (`test ah,5; jp`);
  - `pow(x, 0.7)` with a double literal;
  - pulse constant `0x3dd6774d`, reproduced by `0.104719735f`, which equals `float(3.141592f/30)`; `_pi/30` gives `0x3dd67750`;
  - in-place rotation with a y temporary.
- Best scratch body `rb7`: 384 / 18. The rotation, magnitude, literals and frame are identical to January.
- Residual: January keeps the scale factor live on the x87 stack (`fld y; fmul st(1)` … `fstp st(0)`), and the colour pointer is scheduled early in ESI.

**`_update_motion_sensor`**
- Both range tests square a literal 0.0f third component, loaded from `__real@00000000` and not folded. Every legal spelling tried was folded by VC7:
  - `real_vector3d` with k = 0;
  - `set_real_vector3d`;
  - `dot_product3d`;
  - a function-scope k;
  - flattened `distance_squared3d`.
- The `distance_squared3d(&reference{ref.x,ref.y,object.z}, &object)` shape (848 / 37) does reproduce January's hoisted `ref.x` load, `ref.y` integer copy and `fsubr` pattern.

**`_render_motion_sensor`**
- The residual is a scheduling tie: the sensor-index integer statement interleaves with the x87 weight chain.
- Inner-loop alignment pads follow the code offsets.
- Ten shapes were tried; none is exact.

## Rejected / do-not-repeat

- `_render_motion_sensor`: statement order, index inline, `long` history index, `(active+10-i)`, do-while inner loops, `if (count)` guard. All are sha or size shifts only.
- `_render_blip`: `rotate_vector2d` into a separate local (frame 0x10); `scale_vector2d` ×3; compound and CSE scaling; `inverse_distance * x` orders. All consume the factor.
- `_update_motion_sensor`: every constant-zero third-component spelling above (folded).
- `/Op` diagnostic compile: no effect on these functions. It is not a lever and was not proposed.

## Proposals

1. **Park reopen, candA.** `_motion_sensor_blip_set_type_and_size`, `_should_draw_object`, `_blip_type_get`. Re-baseline `_motion_sensor_update` and `_update_motion_sensor`.
2. **Park reopen, candB (owner ruling on the unused parameter).** `_blip_begin`. Re-baseline `_render_motion_sensor` at 784 / 40.
3. **Owner ruling, `_motion_sensor_update`.** The January-exact body preserves an uninitialised `camera_positions` read for unit-less local players, and it emits two surplus inline COMDATs.

## Reopen criteria

- `_render_blip`: a source spelling that keeps a scalar multiply factor live after its last use, or cross-TU exact source with the same pattern.
- `_update_motion_sensor`: an idiom whose inlining leaves an unfolded `0*0` third component.
- `_render_motion_sensor`: a spelling that emits `sub eax,[ebp-0x18]` and finishes the integer sensor index before the x87 weight chain.
- `_motion_sensor_update`: an owner ruling that permits the uninitialised read.
