# source/structures/leaf_map

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\provider\\patch\\layer2_ABRPL",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\provider\\patch\\layer2_ABR_alternative",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\provider\\action_vehicle\\cand1.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\provider\\object_lights\\cand1.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\provider\\players\\cand1.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\provider\\path_obstacles\\cand1.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\provider\\decals\\cand4.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\provider\\sweep\\m_ABRPL.tsv"
]

## production_changes
LAYER 2 (shared-header packet ABRPL; needs an OWNER RULING). (a) source/math/real_math.h <- patch/layer2_ABRPL/real_math.h.diff (full file at patch/layer2_ABRPL/real_math.h). It sets distance2d to `return square_root(distance_squared2d(a, b));` and distance3d to `return square_root(distance_squared3d(a, b));`, removing the `v` locals. It gives project_point2d `short x/y` locals and a single ternary store `(fabs(...)<_real_epsilon) ? 0.0f : ...`. It adds `void lock_global_random_seed(void); void unlock_global_random_seed(void);` after get_global_local_random_seed_address in the RANDOM_MATH.C section. (b) source/math/random_math.h <- layer2_ABRPL/random_math.h.diff (removes those two prototypes). (c) source/ai/action_vehicle.c <- action_vehicle.c.diff (removes the distance2d/distance3d/distance_squared2d/negate_vector2d renames and hand copies; KEEPS object_get_bounding_sphere). (d) source/objects/object_lights.c <- object_lights.c.diff (/Od-attested `distance3d(&light->position, center)`, dropping the unattested light_position local). (e) source/game/players.c <- players.c.diff. (f) source/ai/path_obstacles.c <- layer2_ABRPL/path_obstacles.c.diff (full cand1, supersedes layer1). (g) source/effects/decals.c <- layer2_ABRPL/decals.c.diff (cand4 = cand3 + project_point2d). Parks: retire _convex_hull2d_perimeter (source/math/geometry). Re-measure _ai_test_line_of_sight base to size 1040, relocs 50, sha c524fe19529bfd4a72e95be52a875925bcc929d347b37471eb16b75bd393da78 (target unchanged). Then retire the leaf_map _distance3d rejection and set Matching after the live ninja + stable_verdicts + provider_link. Smaller alternative packet: patch/layer2_ABR_alternative (distance bodies + get_random_seed moved); it does NOT fix project_point2d, so leaf_map and render_debug stay blocked under it.

## evidence
/Od build: distance3d 0x42dc50 and distance2d 0x443e70 are `square_root(distance_squared*(a,b))` with no local; project_point2d 0x56d680 has short locals and a ternary. January-byte attestations of the new bodies: action_vehicle _distance3d 4cdb6852, and action_vehicle 17/17 with the January emission set (the _distance_squared2d emission arises only via the new distance2d body); geometry _convex_hull2d_perimeter becomes EXACT with the distance2d change alone; object_lights _find_point_lights 43/43 with the /Od caller fix; players _player_set_action_result EXACT with the /Od expression. The rejected trial's losses were decoded as TU name-count shifts: an oracle with 2 dummy names restores both canaries, and the canary windows admit only k=2. A `void f(void)` prototype = 2 units (calibrated). The compensating prototypes are genuine: main.c hits C4013 for lock/unlock while including real_math.h but not random_math.h. Mirror-tree method (junction tree, no preprocessing): control 446/446 units SAME. ABRPL full-board sweep: 0 LOSS, 0 compile failures. The preprocessing (/E) method was shown NOT neutral and discarded. leaf_map under ABRPL: 25/25, object_audit PASS, 18 surplus COMDATs all identical to January (including _distance3d, _project_point2d), provlink2 with candidate providers 33 rows PASS, pair links PASS in both orders.

## blockers
Owner ruling on the shared-header packet. Which zero-parameter random prototype(s) live in real_math.h is underdetermined: C4013 evidence exists for get_random_seed (network_client_manager) and for lock/unlock (main.c), and all these TUs also share game.h/main.h. The canary oracle only fixes the COUNT (two for ABRPL). The integrator must run the live ninja + whole-board stable_verdicts and `python -m tools.parked_functions`.

## reopen_criteria
If the owner rejects the compensating-prototype choice: find a different first-party-evidenced zero-parameter declaration (2 units) or equivalent in the canary-visible span. Verify with the canaries bitmap_drawing::_bitmap_copy, rasterizer_frame_statistics::_rasterizer_frame_statistics_draw and game_engine_race::_race_touch_flag under the mirror method.

## task notes
Key files (all under C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\provider\):
- patch\layer1\*.diff (landable now).
- patch\layer2_ABRPL\ (recommended header packet: real_math.h/random_math.h full files + diffs, plus 5 .c diffs).
- patch\layer2_ABR_alternative\ (smaller header packet; does not fix project_point2d).
- sweep\m_base.tsv (control), m_ABRPL.tsv, m_ABR.tsv, m_ABn2.tsv (oracle), linkcensus.tsv.
- tools\ (mkmirror.py, mgate.py, msweep.py, rmmirror.py, pairlink.py, provlink2.py = provider_link + --subst, surplus_all.py, fnexact.py, surplus_check.py, odmap.py, helper_census.py, linkcensus.py).
- od\ (/Od dumps).

Method warnings for the integrator and other workers:
(1) Lab compiles via CL /E preprocessing shift the TU name-count arena and falsely flag canaries such as _decals_delete_permanent_from_cluster and _rasterizer_frame_statistics_draw. Use the mirror tree instead.
(2) mkmirror.py creates Windows junctions into source/. Remove mirrors only with rmmirror.py, never rm -rf; all mirrors are already removed.

Owner-ruling items:
(a) The header packet relies on the name-count law. Only the count of compensating zero-parameter prototypes in canary-visible headers is fixed by the oracle. Which random_math prototypes (get_random_seed vs lock/unlock) belong in real_math.h is underdetermined by consumer C4013 evidence.
(b) random_math.h itself is a 2026-09-04 reconstruction header.

Out-of-scope findings:
- _object_get_bounding_sphere: January's action_vehicle copy asserts "..\\objects\\objects.h" while the header hard-codes "c:\\halo\\source\\objects\\objects.h". The 8 ANY emitters (actor_moving, ai_debug, hud_nav_points, motion_sensor, object_lights, render_objects, bipeds, +1) therefore differ in relocation identity. It stays NODUP in action_vehicle and is kept.
- breakable_surfaces emits TU-specific non-identical copies of _project_point2d/_project_point3d/_plane3d_* (pre-existing).
- The link census also shows symbol-storage attribution gaps in rasterizer_xbox, actor_firing_position, model_animations, units and game_engine_king. These are the only remaining blockers for those now link-clean units.
- The units _point_from_line3d rejection predates the folded-COMDAT rule (3c74fa36); under Layer 2, units.obj is link-clean.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\provider\LEDGER.md (389 lines, every probe recorded). The resume found an empty ledger plus pre-cutoff tools and baseline gate rows, and continued from those. Sequence:
- P1-P4: provider candidates for path_obstacles, action_charge, bored_camera and player_control. Rows identical; NODUP->ANY with identical bytes.
- F2: actor_combat cross_product2d is BLOCKED on the held aim_grenade.
- F3/F4: /Od readout. distance3d and distance2d are square_root(distance_squared*(a,b)) with no local.
- P5: action_vehicle cand1 is 17/17 only under the AB header.
- P6: object_lights /Od caller fix.
- M1/M2: the /E preprocessing lab method is NOT neutral for name-count canaries. M3: a junction mirror-tree method; control 446/446 SAME.
- S2/S3: AB losses are pure name-count, proven by an oracle that is never landed.
- P7/P8: independent January-byte attestations (players, geometry).
- P9: canary windows admit only k=2. A void prototype = 2 units.
- P10: moving the multi-parameter plane/cone prototypes breaks canaries.
- P11/P14: moving random_math zero-parameter prototypes (C4013 evidence) gives ZERO-LOSS full-board sweeps. ABR moves get_random_seed; ABRPL moves lock/unlock and adds the /Od-attested project_point2d body.
- P12/P13: decals is also a NODUP provider; project_point2d attested by /Od.
- L1/L2/C1/C2: link tests and a board link census. Layer 1: 250->260/288 link-clean, 0 regressions. Layer 2: 268/288, 0 regressions.
- D1: patch set written. Mirrors were removed safely afterwards (junctions only); source/ verified intact, 0 tracked changes.
