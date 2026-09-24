# review source/math/geometry + source/game/players (Layer 2 ABRPL header-packet function gains)
approve=False

## checks
All work is in "C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review_players (header-packet function gains)\" (LEDGER.md R0-R16, REVIEW.md). No tracked file was touched; git status is empty.

(1) Build method. I made full COPIES of source/ (no junctions) as trees/base, trees/ABRPL and trees/full. trees/full = ABRPL plus every Layer 1 and Layer 2 .c diff. The patches were applied with patch -p1 and CRLF was restored. The patched files equal the worker's lab headers and cand files. I wrote my own tools: rvgate.py (build.ninja flags with /Isource remapped into the tree), seccmp.py (section_infos_equal plus COMDAT selection and storage), and rvsweep.py (full board). Control: trees/base gives 447/447 units SAME as build/base.

(2) gate.py on the candidate (--out cand.obj, current header): 67/70, the same rows as production. Candidate under ABRPL: 68/70, with _player_set_action_result EXACT (240/5/ab7ce57f, equal to January). Production players.c under ABRPL stays residual.

(3) geometry production under ABRPL: 27/30, with _convex_hull2d_perimeter EXACT (96/0/71292feb). The only other delta is a NEW _distance_squared2d, identical to January's action_vehicle copy.

(4) object_audit. players stays incomplete; the 3 failures are pre-existing. geometry has 90 January symbols and 0 differ. Data is 100% for both, and no data section changed.

(5) Surplus identity (surplus_obj.py):
- players under ABRPL: 0 of 13 non-identical. Production has 1 non-identical, _distance3d.
- Across all 28 changed units, every _distance3d (17 units) and _project_point2d (4 units) flips from DIFFERENT to IDENTICAL.
- No new non-identical surplus anywhere.

(6) Provider link (provider_link with my full-tree providers substituted):
- players: PASS.
- geometry: FAIL on _cross_product2d and _plane2d_from_points. Both are pre-existing; production also fails _distance2d, which Layer 2 cures.

(7) Full-board sweep of the whole packet: 419 SAME, 28 DIFF, 0 LOSS, 0 compile failures.
- GAIN: exactly the two functions.
- DRIFT, residuals only: actor_perception_refresh_danger_zone (alndiff 234->238), and the parked ai_test_line_of_sight (1040/50, sha 61a3ce7f -> c524fe19, alndiff 264->270).
- The canaries (bitmap_copy, frame_statistics_draw, collision_move_point) are SAME.

(8) Attribution (header variants A = distance3d body only, B = distance2d body only, AB = both):
- The players gain needs A plus the no-locals caller; B alone leaves it residual.
- The geometry gain needs B; A alone leaves it residual.
- Both gains hold under A/B, AB and ABRPL (name-count shifts of -1, -2 and 0), so they are not name-count ties.
- AB alone loses _bitmap_copy and _rasterizer_frame_statistics_draw. A alone and B alone each lose _rasterizer_frame_statistics_draw.

(9) /Od re-dumped independently:
- 0x443e70 (distance2d) and 0x42dc50 (distance3d) are frameless: square_root(distance_squared*(a,b)).
- 0x5d0d50 (player_set_action_result): the distance results are expression temps with a movss spill across the second call, i.e. one expression with arguments (unit, current) and (unit, new).

(10) Strip / fidelity test. I also adopted the rest of the /Od shape (no '= FALSE', and a last arm 'set_action = action_result > player->action_result'). It gives 256 != 240 under both headers, so the diff correctly takes only the January-attested part. The new spelling has no decoration.

(11) CL /Zs /W3 warnings:
- players: identical (18).
- main.c: loses its 2 C4013s (lock/unlock).
- network_client_manager: keeps its get_random_seed C4013.

(12) Ledger history. The geometry park reopen criterion ('genuine j-first helper') is met. The players reopen criterion is explicitly 'owner decision on the real_math.h distance3d/magnitude inline spelling'.

## issues
No technical defect. Verdict: POLICY HOLD (exact-under-ruling), consistent with the review_weapons and review_weather_particle_systems precedent.

Blocking:
- The two gains need the real_math.h distance2d/distance3d body change. Those hunks cannot land alone: header AB regresses bitmap_drawing::_bitmap_copy and rasterizer_frame_statistics::_rasterizer_frame_statistics_draw by a pure name-count effect, since neither TU uses distance2d/3d.
- Landing therefore needs a declaration-count compensation chosen by the oracle count. ABRPL moves lock/unlock_global_random_seed from random_math.h into real_math.h's RANDOM_MATH.C section; the ABR alternative moves get_random_seed instead.
- These are genuine prototypes, not fillers, but which ones January had is underdetermined. ABRPL cures main.c's C4013s and leaves network_client_manager's get_random_seed C4013; ABR does the reverse. C4013 on void(void) or int-returning calls does not change codegen.
- The worker routes this to the owner (P13/P14), and the players ledger's reopen criterion is explicitly an owner decision. A review approval would pre-empt that ruling.

Once the owner admits the Layer 2 header packet (ABRPL as written, or ABR if project_point2d is dropped), approve with these conditions:
- (a) Apply real_math.h, random_math.h and ALL Layer 1 and Layer 2 .c diffs as one change. Never apply the distance hunks alone.
- (b) Credit geometry::_convex_hull2d_perimeter (96/0/71292feb) and players::_player_set_action_result (240/5/ab7ce57f). Retire the _convex_hull2d_perimeter park.
- (c) Re-measure the _ai_test_line_of_sight park base: 1040/50/c524fe19529bfd4a72e95be52a875925bcc929d347b37471eb16b75bd393da78, plus a fresh objdiff %. A parked function whose bytes move fails PROGRESS.
- (d) Record the non-exact fuzzy drift of _actor_perception_refresh_danger_zone (alndiff 234->238 lines) and of ai_test_line_of_sight (264->270).
- (e) Run a full ninja and diff every function row.

Non-blocking:
- scratch/w/provider/patch/layer2_ABRPL/players.c.diff is house-rule clean. It removes the unattested current_distance/new_distance locals in favour of the /Od-attested single expression, with no decoration.
- It is neutral under the current header (67/70, only the residual's bytes move), so it may be staged standalone at zero credit.
- geometry and players both remain incomplete objects. geometry has a pre-existing provider-link failure on _cross_product2d (BLOCKED) and on the _plane2d_from_points decals hand copy. players has 2 residuals and the _player_examine_nearby_unit storage mismatch.

Files: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review_players (header-packet function gains)\REVIEW.md, LEDGER.md, sw_full.tsv, sw_base.tsv, surplus_full_vs_prod.txt, objs\.
