# source/effects/decals

## verdict
BLOCKED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\provider\\decals\\cand3.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\provider\\decals\\cand4.c"
]

## production_changes
As a PROVIDER, apply layer1 cand3 (later layer2 cand4); both keep all 33 rows identical. As an object it stays NonMatching.

## evidence
The _vector_from_points2d conflict is already resolved at base: action_obey emits ANY, and all 13 copies are equal to January. The rejection entry is stale on that symbol. It still fails _cross_product2d (actor_combat NODUP; see path_smoothing) plus 2 pre-existing residuals (_decal_clip_to_surface, _decal_new_from_collision; lane-closed). The invented surplus _plane3d_negate_inline is removed by cand3/cand4. Removing ALL six decals hand copies (cand1) loses 3 rows: _plane2d_from_points and _project_point2d (header bodies differ) and the name-count canary _decals_delete_permanent_from_cluster.

## blockers
Two lane-closed residuals + actor_combat _cross_product2d (held aim_grenade).

## reopen_criteria
New evidence for the two decals residuals + the aim_grenade reopen.

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
