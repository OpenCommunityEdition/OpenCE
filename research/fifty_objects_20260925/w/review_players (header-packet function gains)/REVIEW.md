# Adversarial review: geometry + players header-packet function gains (Layer 2 ABRPL)

- **Reviewer slug:** `scratch/w/review_players (header-packet function gains)/`
- **Base:** e9e62b78. The worktree is clean: no tracked file was touched, and `git status` is empty.
- **Worker under review:** `scratch/w/provider` (LEDGER.md, 389 lines).
  - Patches: `scratch/w/provider/patch/layer2_ABRPL/{players.c,real_math.h,random_math.h}.diff`
  - Candidate: `scratch/w/provider/players/cand1.c`
- **Record:** every probe is in LEDGER.md (R0-R15) in this directory.

## Verdict: approve = false. POLICY HOLD (exact-under-ruling); no technical defect found

Every technical claim reproduced independently. The two gains are real, cleanly attributable to the /Od- and January-attested helper bodies, and cause no regression on the full board.

They cannot land on their own merit, for two reasons:
- The helper-body hunks regress two name-count canaries unless a declaration-count compensation lands with them.
- The worker's own ledger (P13/P14 "UNDERDETERMINED -> owner ruling item") and the players reopen criterion both leave the compensation choice to the owner. That criterion reads "owner decision on the real_math.h distance3d/magnitude inline spelling" (players_obj_opus5_150k_w3_20260914.md).

Approving now would pre-empt that ruling. Campaign precedent handles this the same way (review_weapons, review_weather_particle_systems: approve=false, hold, no defect).

## Reproduced (my own harness)

### Method
- `trees/base` and `trees/ABRPL` are full COPIES of source/, not junctions. The worker's header diffs were applied with `patch -p1` and CRLF was restored.
- The patched headers equal the worker's lab and patch copies. `diff -rq` shows only real_math.h and random_math.h differ.
- `trees/full` = ABRPL plus every Layer 1 and Layer 2 `.c` diff. Each patched file equals the worker's candidate.
- Tools written for this review:
  - `rvgate.py`: build.ninja flags with `/Isource*` remapped into the tree.
  - `seccmp.py`: every defined section via `coff_compare.section_infos_equal`, plus the COMDAT selection byte and storage.
  - `rvsweep.py`: the full board.
- **Control:** trees/base gives **447/447 units SAME** as build/base.

### players (source/game/players)

| Build | Header | Result |
|---|---|---|
| production | current | gate.py 67/70 (residuals: examine_nearby_device, set_action_result, teleport_internal) |
| candidate | current | 67/70, identical rows; only `_player_set_action_result` bytes move (neutral) |
| production | ABRPL | 67/70 |
| candidate | ABRPL | **68/70, `_player_set_action_result` EXACT** (240/5/ab7ce57f == January) |

- **Surplus, production:** 1 of 12 COMDATs is non-identical (`_distance3d` vs January's action_vehicle copy).
- **Surplus, candidate under ABRPL:** 0 of 13 non-identical. `_distance3d` is now IDENTICAL, and the new `_distance_squared3d` is IDENTICAL to action_alert's copy.
- **provider_link** (Layer 2 providers substituted with my objs): PASS. Production FAILs `_distance3d`.
- **object_audit:** players stays NOT complete (examine_nearby_device, teleport_internal, and `_player_examine_nearby_unit` storage 2/3, all pre-existing).
- **Data:** 100% (5349/5349). No data section changes.

### geometry (source/math/geometry): no source change

- Current header: 26/30. ABRPL: **27/30, `_convex_hull2d_perimeter` EXACT** (96/0/71292feb == January).
- The only other change is a NEW `_distance_squared2d`, IDENTICAL to January's action_vehicle copy.
- object_audit: 90 January symbols, 0 differ. Three residual sections remain.
- provider_link:
  - `_distance2d` and `_distance_squared2d` PASS.
  - It still FAILs `_cross_product2d` (actor_combat NODUP, already BLOCKED) and `_plane2d_from_points` (decals hand copy).
  - Both failures are pre-existing, not introduced.

### Attribution

Header variants built from base: A = distance3d body only; B = distance2d body only.

| Header | players candidate | players production | geometry |
|---|---|---|---|
| A | EXACT | residual | residual |
| B | residual | residual | EXACT |
| AB | EXACT | residual | EXACT |
| ABRPL | EXACT | residual | EXACT |

- The players gain needs the distance3d body **and** the /Od no-locals caller.
- The geometry gain needs the distance2d body.
- Neither needs the project_point2d (P) or lock/unlock (L) parts.
- Both gains hold across TU name-count shifts of -1, -2 and 0, so they are not name-count ties.

### Evidence for the header bodies (re-dumped myself)
- **/Od helpers.** 0x443e70 (distance2d) and 0x42dc50 (distance3d) are:
  `push b; push a; call distance_squared*; add esp,4; fstp [esp]; call square_root`, with NO frame.
  That is `return square_root(distance_squared*(a, b));`.
- **January's own bytes:**
  - All 17 units' surplus `_distance3d` flips from DIFFERENT to IDENTICAL to January's out-of-line copy.
  - action_vehicle emits January's exact helper set.

### Evidence for the players caller (/Od 0x5d0d50)
- The named slots are player, set_action, unit_position, current_position and new_position.
- The two distance results are expression temps: `fstp` then `movss xmm0`, spilled across the second call's pushes, then `comiss`, `jbe`. So it is ONE expression, `distance3d(unit, current) > distance3d(unit, new)`, with args in that order.
- **Strip / fidelity check.** Also adopting the rest of the /Od shape gives 256 != 240 under both headers, so January rejects that part of the later revision. That rest of the shape is: no `= FALSE`, and a last arm of `set_action = action_result > player->action_result`.
- The diff takes only the part January's bytes attest. The retained `= FALSE` / `else if ... TRUE` is pre-existing and exact.
- The new spelling has no decoration: no parentheses, casts, volatile, pragma or dummy. There is nothing to strip.

### Full board, whole packet (trees/full)
- 447 units: 419 SAME, 28 DIFF, **0 LOSS**, 0 compile failures.
- GAIN: exactly the two functions above.
- DRIFT, residuals only:
  - `_actor_perception_refresh_danger_zone`: not parked; alndiff 234 -> 238 lines.
  - `_ai_test_line_of_sight`: PARKED; 1040/50, sha 61a3ce7f -> c524fe19 (the worker's value is confirmed); alndiff 264 -> 270.
- The canaries `_bitmap_copy`, `_rasterizer_frame_statistics_draw` and `_collision_move_point` are SAME.
- Surplus: no new non-identical COMDAT anywhere. `_distance3d` (17 units) and `_project_point2d` (4 units) flip to IDENTICAL.

## House rules (players.c diff)
The diff removes two local declarations and replaces three statements with one natural expression.
- The `real` type is unchanged. There is no new name, prototype, macro, cast or assertion.
- Parameter-list rules are unaffected. One-line call arguments are ordinary project style.
- The semantics are sensible: switch the action when the current action object is farther away than the new one.
- `CL /Zs /W3` warnings are identical before and after (18).
- **House-rule clean.**

## Why it is a hold, not an approval
1. **The helper bodies cannot land alone.**
   - AB alone: LOSS `_bitmap_copy` and `_rasterizer_frame_statistics_draw`, DRIFT `_collision_move_point`.
   - A alone and B alone each lose `_rasterizer_frame_statistics_draw`.
   - Neither canary TU uses distance2d/3d, so this is a pure name-count effect.
2. **The compensation is chosen by the oracle count.** ABRPL moves lock/unlock_global_random_seed from random_math.h into real_math.h's existing RANDOM_MATH.C section. They are genuine prototypes, not fillers, in a header consistent with owner ruling 3 (2026-09-21).
   - Which prototypes January had there is underdetermined:
     - ABRPL cures main.c's two C4013s but leaves network_client_manager's `get_random_seed` C4013.
     - The ABR alternative does the reverse.
   - C4013 on void(void) or int-returning calls does not change codegen, so it is weak evidence of the layout.
   - The worker routes this to the owner, and I concur.
3. The packet also carries project_point2d (P). That is outside this unit's gains, but it is what forces the two-prototype compensation.

## Conditions for approval once the owner admits the Layer 2 header packet
If the owner admits it (ABRPL as written, or ABR if P is dropped):
- Apply the header and all Layer 1 and Layer 2 `.c` diffs as one change. Never apply the distance hunks alone.
- **Credit:** geometry `_convex_hull2d_perimeter` and players `_player_set_action_result`. Retire the `_convex_hull2d_perimeter` park.
- Re-measure the `_ai_test_line_of_sight` park base: 1040/50/c524fe19529bfd4a72e95be52a875925bcc929d347b37471eb16b75bd393da78, plus a fresh objdiff %.
- Record the fuzzy drift of `_actor_perception_refresh_danger_zone`.
- Run a full ninja and diff every function row.

**Standalone option.** players.c.diff is neutral under the current header (67/70, only the residual's bytes move). It is /Od-attested and may land alone at zero credit if the integrator wants it staged.
