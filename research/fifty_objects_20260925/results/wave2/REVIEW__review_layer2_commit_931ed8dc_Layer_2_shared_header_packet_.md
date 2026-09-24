# review review_layer2 (commit 931ed8dc, Layer 2 shared-header packet)
approve=False

## per_object
[
 {
  "unit": "source/math/real_math.h",
  "approve": false,
  "issues": "Body changes VERIFIED and approvable: distance3d (/Od 0x42dc50, January 4cdb6852), distance2d (/Od 0x443e70; the standalone bytes cannot tell old from new, but the change is attested by January action_vehicle's _distance_squared2d emission and the geometry _convex_hull2d_perimeter gain) and project_point2d (/Od 0x56d680, January 48bc73c5). The added `void lock_global_random_seed(void); void unlock_global_random_seed(void);` lines are canary-load-bearing count compensation. Without them 3 exact functions regress (V1). The pair is count-selected: V5 and V6 are equivalent, and the evidence-equal full move V2 regresses 4. The provider required an owner ruling and none is recorded. Hold for that ruling plus documentation. Never revert the two lines alone."
 },
 {
  "unit": "source/math/random_math.h",
  "approve": false,
  "issues": "This is the other half of the same move and has the same status as real_math.h. get_random_seed and random_seed_debug_log stay here only because moving them regresses 4 exact functions. get_random_seed still gives C4013 in network_client_manager.c. The supplied genuine random_math.c does not include random_math.h, so this header is a 2026-09-04 reconstruction. It needs an owner ruling and a disclosure note."
 },
 {
  "unit": "source/ai/action_vehicle.c",
  "approve": true,
  "issues": "None. 17/17 EXACT. _distance2d, _distance3d, _distance_squared2d and _negate_vector2d are now ANY copies equal to January (section_infos_equal). provider_link PASS; surplus 12 with 0 non-identical. The object_get_bounding_sphere hand copy is correctly kept (known assert-string conflict). Depends on the real_math.h body change."
 },
 {
  "unit": "source/ai/path_obstacles.c",
  "approve": true,
  "issues": "None. 15/16; the _obstacles_test_circle residual is unchanged. _rotate_vector2d, _project_point3d and _point_in_circle are ANY copies equal to January. The call now uses the genuine point_in_circle name. provider_link PASS; surplus 15 with 0 non-identical. Its _distance_squared2d surplus needs the action_vehicle change and the header."
 },
 {
  "unit": "source/effects/decals.c",
  "approve": true,
  "issues": "None new. 31/33; the 2 residuals are unchanged and the lane is closed. _project_point2d is ANY and equal to January (48bc73c5). provider_link fails only on _cross_product2d (actor_combat NODUP), which pre-dates this commit. The plane2d_from_points rename and REAL_MATH_EXTERNAL_PROJECT_POINT3D remain and were not touched by Layer 2. Depends on the header project_point2d body."
 },
 {
  "unit": "source/objects/object_lights.c",
  "approve": true,
  "issues": "None. `distance3d(&light->position, center)` is /Od-attested at 0x787870: light+0x30 is pushed directly and the frame has no light_position slot. 43/43 EXACT. provider_link fails only on _object_get_bounding_sphere, which pre-dates this commit. Depends on the header."
 },
 {
  "unit": "source/game/players.c",
  "approve": true,
  "issues": "None. The single-expression compare is /Od-attested at 0x5d0d50 (xmm0 spill across the second call; no distance locals). _player_set_action_result is EXACT (68/70). The /Od-shaped third arm gives 256 != 240, so the landed `= FALSE` form is correct. Under the old header it is neutral (67/70), so it can stay even under the fallback."
 },
 {
  "unit": "config/parked.json",
  "approve": true,
  "issues": "The _convex_hull2d_perimeter retirement is correct (now EXACT). The _ai_test_line_of_sight re-measure is valid: parked_functions reports stale [] and invalid []. Both must be reverted with the header if the owner rejects (they are included in the fallback diff)."
 }
]

## checks
Evidence and every probe are in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review_layer2\LEDGER.md (R1-R10). Tools are in scratch/w/review_layer2/tools/: labgate.py, mkvariants.py, incscan.py, c4013.py, clflags.py.

R1: The landed real_math.h and random_math.h are byte-identical (cmp) to scratch/w/provider/patch/layer2_ABRPL/.

R2, claim (5): `python -m tools.campaign.stable_verdicts snapshot scratch/w/review_layer2/stable_now.json` gives 8,245 functions, 7,603 exact. The file is byte-identical to scratch/campaign/b2_stable.json. The b1->b2 diff is +2 (_player_set_action_result 240, _convex_hull2d_perimeter 96) with 0 regressions. build/base is current: 273 objects are newer than real_math.h (matching the 273 CL lines in b2_ninja1). Each of the 194 older objects was checked with CL /Zs /showIncludes, and none of them includes real_math.h or random_math.h.

R3/R4, claim (1):
- /Od readouts:
  - 0x42dc50 distance3d is call distance_squared3d, then square_root, with no local.
  - 0x443e70 distance2d has the same shape.
  - 0x56d680 project_point2d has short x/y (word stores), two asserts, and one store to n[projection] through a temp, i.e. a single ?: expression.
- helper_census (section_infos_equal) over build/base:
  - These copies are all ANY and all equal to January's single selected definer: _distance2d x3, _distance3d x19, _distance_squared2d x6, _distance_squared3d x44, _negate_vector2d, _point_in_circle, _rotate_vector2d x3, _point_in_sphere x6, _magnitude3d x63. No NODUP copy is left.
  - _project_point2d is equal in 5 units. Only breakable_surfaces differs, and it already differed before Layer 2 (as did its _project_point3d and _vector_from_points3d).
- Standalone lab (lab_pp.c):
  - The new distance3d and project_point2d bodies equal January (4cdb6852 and 48bc73c5). The old bodies do not.
  - The OLD distance2d body is also byte-identical to January. The distance2d change is therefore attested only by /Od, by January's action_vehicle _distance_squared2d emission, and by the geometry gain.
  - realcmp(n[proj], 0.0f), the literal /Od shape, is byte-identical to the landed fabs() spelling.

R5, lab header method:
- Copies at both <lab>/ and <lab>/math/ opened a second guarded file. That alone flipped _rasterizer_frame_statistics_draw, so it was a method defect and I fixed it.
- Fixed method: one copy at <lab>/math/, with the lab /I placed first.
- Control V0: all 9 canary units are identical to build/base in every code section: bitmap_drawing, rasterizer_frame_statistics, game_engine_race, bitmap_utilities, decals, geometry, rasterizer_xbox_models, error_geometry, collisions.

R6/R10, claim (2), the random prototype move:
- V1 (the landed header without the move) loses _bitmap_copy 2784, _rasterizer_frame_statistics_draw 4176 and _get_edge_vertex 416, and changes the bytes of _collision_move_point (already a residual).
- V2 (all four RANDOM_MATH.C prototypes moved) loses _bitmap_2d_alpha_bleed, _decals_delete_permanent_from_cluster, _rasterizer_frame_statistics_draw and __rasterizer_model_transparent_geometry_submit.
- V3 (get_random_seed only) loses bitmap_copy and frame_statistics_draw.
- V5 {lock, get_random_seed} and V6 {unlock, get_random_seed} are identical to the landed state.
- V4 (realcmp spelling) has no effect on these 9 units.

R7, C4013 (CL /Zs /W3):
- Landed: main.c 0. network_client_manager.c:1674 still gives C4013 for get_random_seed.
- Before the move: main.c:2902/2983 for lock/unlock, plus the get_random_seed warning.
- Full move: 0 everywhere.

Genuine-source evidence: the supplied random_math.c (haloleak2024/halo1) includes cseries.h, real_math.h, geometry.h and game_engine.h, but NOT random_math.h. random_math.h itself was created 2026-09-04 during reconstruction.

R8, claim (3):
- gate --all on production source:
  - action_vehicle 17/17.
  - path_obstacles 15/16 (_obstacles_test_circle was already a residual).
  - decals 31/33 (both residuals pre-existing, lane closed).
  - object_lights 43/43.
  - players 68/70.
  - geometry 27/30.
- provider_link:
  - PASS: action_vehicle, path_obstacles, players.
  - decals FAILs only on _cross_product2d, and object_lights only on _object_get_bounding_sphere. Both conflicts pre-date Layer 2 and are already listed in the brief.
- surplus_identity: 0 non-identical copies in action_vehicle (12), path_obstacles (15), decals (21) and players (13). object_lights (_object_get_bounding_sphere) and geometry (_plane2d_from_points, a header body Layer 2 did not touch) each have one DIFFERENT copy that was already there before this commit.

R9, claim (4):
- /Od 0x5d0d50: the first distance3d result goes fstp to a temp, then into xmm0, and is spilled across the second call before comiss. That is one expression, with no current_distance/new_distance slots.
- A lab with the /Od-shaped third arm gives size 256 != 240, so the landed `= FALSE` form is the one January's bytes support.
- /Od 0x787870: push center, then light+0x30, then call distance3d. There is no light_position slot.

Parks: `python -B -m tools.parked_functions` reports stale [] and invalid [].

## issues
Layer 2 is technically correct:
- The three helper bodies match the /Od build and January's selected copies.
- Every emitted helper copy is byte-identical to January's selected section.
- The provider objects keep every January row.
- The object_lights and players respellings are /Od-attested.
- The board sweep claim is reproduced exactly.

**The lock/unlock_global_random_seed move is an owner-gated item that was landed without a recorded owner ruling.** The move goes to a genuine owner header: real_math.h already has a RANDOM_MATH.C section, the supplied genuine random_math.c includes real_math.h and not random_math.h, and main.c showed C4013 for both functions. But it is also canary-load-bearing name-count compensation:
- The strip test fails. With the landed header minus the move, _bitmap_copy, _rasterizer_frame_statistics_draw and _get_edge_vertex (7,376 B) are lost, and the bytes of _collision_move_point (already a residual) change.
- The bytes decide only the count, not which prototypes. Moving {lock, get_random_seed} or {unlock, get_random_seed} gives results identical to the landed move.
- The evidence-equal full move regresses 4 other exact functions: _bitmap_2d_alpha_bleed, _decals_delete_permanent_from_cluster, _rasterizer_frame_statistics_draw and __rasterizer_model_transparent_geometry_submit. The full move covers lock, unlock, get_random_seed and random_seed_debug_log.
- get_random_seed still gives C4013 in network_client_manager.c:1674 after the landed move. That is the same class of evidence the commit cites for main.c.
- The provider (P13, and the wave1 result.json blocker) said explicitly that the choice is underdetermined and needs an OWNER RULING.
- The commit message presents the move only as evidence-driven placement ("main.c calls them without random_math.h"). It does not say the move is load-bearing or that the subset was picked to balance the count.

Required amendment, no source change needed if the owner accepts:
1. Put the V1/V2/V5/V6 evidence (my ledger R6/R10) to the owner, including that the full genuine move is blocked by 4 regressions.
2. Record the ruling, with a Layer 2 section in docs/object_matching_logs/claude_fifty_objects_20260925.md. That section should say the move compensates the -4 declared-name shift from the body corrections, name the 3 functions it protects, and give the reopen criterion: move get_random_seed and random_seed_debug_log and retire random_math.h once the other count gaps are recovered.

If the owner rejects:
- Do NOT revert only the two prototype lines. Per V1 that regresses 3 exact functions.
- Apply scratch/w/review_layer2/fallback_revert_except_players.diff. It is `git diff 931ed8dc 931ed8dc~1` for real_math.h, random_math.h, action_vehicle.c, path_obstacles.c, decals.c, object_lights.c and config/parked.json, and `git apply --check` is clean.
- That fallback loses _convex_hull2d_perimeter (its park is restored) and _player_set_action_result.
- players.c may stay: it is /Od-attested and neutral under the old header (67/70).

Optional, not required: the literal /Od spelling of the project_point2d condition is realcmp(plane->n.n[projection], 0.0f). It is byte-identical to January and has no effect on 9 canary units, but it would need a full-board sweep before landing.

Pre-existing items, not caused by Layer 2: breakable_surfaces' _project_point2d/_project_point3d/_vector_from_points3d still differ from January (content changed, still non-identical). Also the /Od RTC name for find_point_lights' cursor is `state`, not `reference_index`.
