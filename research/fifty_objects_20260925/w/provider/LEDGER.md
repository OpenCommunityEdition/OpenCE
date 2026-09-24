# Helper-provider repair research (slug scratch/w/provider)

Base: worktree claude-fifty-objects-20260925 (canonical e9e62b78). Worker rules: scratch only.

## Probes (hypothesis -> evidence -> change -> measured result)


### RESUME 1 (after usage-limit cutoff)
- Found: base/*.gate.txt baseline gate rows for 8 providers/consumers (all provider rows EXACT except
  pre-existing residuals: action_charge_perform, actor_aim_grenade, actor_combat_plan_grenade_trajectory,
  decals x2, obstacles_test_circle). tools/{helper_census,conflict_census,callrefs,objcmp}.py written.
  path_obstacles/cand1.c + cand1.obj exist but were never measured in the ledger -> re-measure.

### P1 path_obstacles cand1 (re-measured after resume)
- Hypothesis: path_obstacles hand-writes rotate_vector2d/project_point3d/point_in_circle/point_in_sphere
  (NODUP) behind `#define x x_inline` renames; the real_math.h __inline bodies are byte-identical, and the
  TU genuinely USES point_in_sphere (line ~294, obstacles_get_discs_in_sphere) and point_in_circle (~512).
- Change: cand1.c = production minus the 6 #define/#undef renames and the 4 hand copies; call site
  `point_in_circle_inline(` -> `point_in_circle(`.
- Result: gate 15 EXACT + 1 pre-existing residual (_obstacles_test_circle sha) == production (base/path_obstacles.gate.txt).
  objcmp: _point_in_circle/_point_in_sphere/_project_point3d/_rotate_vector2d sel NODUP->ANY, bytes same.
  helper_census: all four equal=True to January split; point_in_sphere ANY copies in effects/objects/
  breakable_surfaces/collisions/physics are all equal=True.
  NEW real-name surplus: _distance_squared2d, _distance_squared3d (ANY) replace invented-name
  _distance_squared2d_inline/_distance_squared3d_inline/_point_in_circle_inline -> must check vs NODUP definers.

### F1 current-base census (base e9e62b78; base/helper_census_task.txt, base/conflict_census.txt)
- ALREADY REPAIRED at base (ANY, byte-identical to January selected): _fast_ftol (actor_combat ANY, 25 ANY
  copies all equal), _vector_from_points2d (action_obey ANY, 13 equal), _valid_real_vector3d (bored_camera
  ANY, 20 equal). Repairs landed in 5e81dabf (action_charge/action_obey/geometry/decals) and earlier.
  => hud_draw/hud_unit (_fast_ftol) and decals (_vector_from_points2d) rejection entries are STALE on
  those symbols: verify by pair link only.
- STILL NODUP: _point_in_sphere/_rotate_vector2d/_project_point3d/_point_in_circle (path_obstacles),
  _real_random (action_charge), _signed_angular_difference + _interpolate_scalar (player_control),
  _valid_real_vector3d_axes2 (bored_camera), _cross_product2d (actor_combat),
  _distance3d/_distance2d/_distance_squared2d/_object_get_bounding_sphere (action_vehicle).
- _distance3d: 18 ANY copies all sha 01d7809a != January/action_vehicle 4cdb6852.

### P2 action_charge cand1 (action_charge/cand1.c)
- Evidence: January action_charge_setup relocs _get_global_random_seed_address@16b/_real_seed_random@171 =
  real_random() inlined at the genuine use (line ~985, leap chance); January still emits _real_random
  (16 B, 2 relocs) = the header inline's referenced-COMDAT. No other January fn references _real_random.
- Change: remove `#define real_random real_random_inline` + `#undef real_random` + hand copy (7 lines).
- Result: gate rows identical to base (21 EXACT + pre-existing residual _action_charge_perform);
  objcmp: only _real_random sel NODUP->ANY, bytes identical; no other section changed.

### P3 bored_camera cand1 (bored_camera/cand1.c)
- Evidence: January _valid_real_vector3d_axes2 calls _valid_real_normal3d@b,@1b out of line (= header body
  with valid_real_normal3d un-inlined, same as the 14 equal ANY copies); bored_camera_update calls
  _valid_real_vector3d_axes2@1e8 and _valid_real_vector3d@384 out of line.
- Change: remove both renames (+#undefs), the consumer-local prototypes of valid_real_normal3d and
  valid_real_vector3d_axes2 (house rule: no consumer-local protos), and the hand copy.
- Result: 9/9 EXACT (unchanged). objcmp: _valid_real_vector3d_axes2 NODUP->ANY same bytes; new surplus
  _valid_real_normal3d (ANY, = January actor_combat selected 0958b20f; 35 ANY copies, no NODUP) and
  _magnitude_squared3d (ANY, = January action_alert 0d045790; 94 ANY, no NODUP).
- path_obstacles cand1 surplus _distance_squared2d (e12f5936) EQUALS January but action_vehicle holds it
  NODUP => path_obstacles repair must ship WITH the action_vehicle repair (or link fails).

### P4 player_control cand1 (player_control/cand1.c)
- Evidence: January player_control_modify_desired_angles calls _interpolate_scalar@51f out of line (one of
  four uses), signed_angular_difference inlined at its 3 uses (no reloc). Header bodies == hand copies.
  limit2d: no use in this TU, no other board definer -> NOT a conflict; left untouched (removing it would
  lose the January _limit2d row; header limit2d body is early-return shape, differs from the hand copy).
- Change: remove signed_angular_difference/interpolate_scalar renames(+#undefs) and their hand copies.
- Result: 49/49 EXACT (identical rows); objcmp: both sel NODUP->ANY, bytes identical, nothing else moved.
- Pair link objects.obj x base player_control: (see P6).

### F2 actor_combat _cross_product2d -> BLOCKED (no probe spent)
- actor_combat.c has NO use of cross_product2d besides the hand copy (line ~523). Recorded /Od evidence
  (claude_object_closure_20260924.md): the ONLY genuine user is _actor_aim_grenade (/Od 0x44ebb0 calls
  cross_product2d at 0x44ed45). aim_grenade is a frame-scope residual (sub esp 0x28 vs 0x24) stopped at zero
  credit (claude_actor_aim_grenade_20260924.md); restoring its helper calls adds 5 view casts + 5 COMDATs that
  rulings 5/6 admit only in a strict-exact function; aim_grenade forms are HELD (brief sec.2).
  Removing the hand copy without the genuine use would drop January's _cross_product2d row.
  => path_smoothing and decals(_cross_product2d) stay BLOCKED on actor_aim_grenade strict exactness.

### F3 /Od readout of action_vehicle helpers (od/action_vehicle_map.txt; tool tools/odmap.py)
- /Od fns: 0x4417e0 desirable, 0x4419c0 evaluate_seat, 0x4422a0 find_destination (RTC vehicle_center,
  vehicle_radius, avoid_vehicle_direction...), 0x442dd0.., 0x4431d0 perform, 0x4438e0/0x443bb0 setups.
- find_destination: call 0x443f50 (object_get_bounding_sphere, 162 B, strings "D:\P4\...objects/objects.h"
  = __FILE__ in the later build) then call 0x42dc50 = distance3d (39 B) whose callees are 0x42dc80
  (109 B, RTC local 12 B, calls 0x42e3a0 vector_from_points3d + 0x42df70 magnitude_squared3d =
  distance_squared3d) and 0x42e320 (square_root).  => /Od distance3d == square_root(distance_squared3d(a,b)).
- 0x443e70 (39 B) called 4x from evaluate_seat: callees 0x443ea0 (106 B, RTC 8 B local, calls 0x42e350
  vector_from_points2d + 0x42df20 magnitude_squared2d = distance_squared2d) + 0x42e320 square_root.
  => /Od distance2d == square_root(distance_squared2d(a,b)); THIS is the genuine reason January's
  action_vehicle emitted _distance_squared2d (referenced from distance2d's body) - no direct use needed.
  0x443e70/0x443ea0 sit inside the action_vehicle compiland = action_vehicle was the /Od selected provider too.
- 0x444020 (63 B, leaf, called 2x in find_destination) = negate_vector2d candidate (located in-compiland).

### F4 /Od helper bodies (odbuild fn)
- 0x42dc50 distance3d: push b; push a; call distance_squared3d; fstp [esp]; call square_root -> NO local,
  body == `return square_root(distance_squared3d(a, b));` (== action_vehicle hand copy spelling).
- 0x443e70 distance2d: identical shape -> `return square_root(distance_squared2d(a, b));` (NO local).
  Current real_math.h: distance3d/distance2d = magnitude{3,2}d(vector_from_points{3,2}d(a,b,&v)) with a
  `real_vector{3,2}d v;` local each.
- 0x42dc80 distance_squared3d / 0x443ea0 distance_squared2d: RTC aggregate local (12/8 B) = `v` as in header.
- 0x444020 = point_from_line2d (p + v*t), not negate_vector2d.
- Rejected-trial analysis: rasterizer_frame_statistics.c has NO distance3d use; its loss under the trial is
  the decoded TU NAME-COUNT law (trial body removes 1 declared name `v` from real_math.h; that function is
  the canonical name-count-sensitive example, exact only in a width-1 window). Hypothesis H-NC: the trial's
  sibling losses are name-count ties, not distance3d codegen. To test in LAB.

### M1 LAB method validated (tools/labpp.py)
- gate.py appends --cflag /I AFTER build.ninja's /I list, so a lab /I cannot shadow source/math/real_math.h.
  Method: CL /E the TU with the unit's exact flags and lab/inc FIRST on /I (analysis-only preprocessing),
  then `gate.py --source <tu>.i`. #line directives confirm both spellings ("math/real_math.h",
  "real_math.h") resolve to lab/inc copies.
- Control: unmodified real_math.h copy -> action_vehicle .i: 17/17 EXACT; objcmp vs base: ZERO differing
  sections, same selections. Method is byte-neutral.

### P5 action_vehicle cand1 x lab headers (action_vehicle/cand1.c; lab/av_cand1_{base,A,AB}.*)
- cand1 = production minus distance2d/distance3d/distance_squared2d/negate_vector2d renames(+#undefs) and
  hand copies (object_get_bounding_sphere hand copy KEPT: its January reloc names "..\objects\objects.h",
  the header asserts "c:\halo\source\objects\objects.h" -> 8 ANY copies elsewhere differ in reloc target).
- header base (current real_math.h): 15 EXACT, _distance3d residual [sha], _distance_squared2d UNWRITTEN
  (header distance2d body does not reference it), surplus _magnitude3d.
- header A (distance3d = square_root(distance_squared3d(a,b))): 16 EXACT, _distance_squared2d UNWRITTEN.
- header AB (A + distance2d = square_root(distance_squared2d(a,b)), BOTH /Od-attested, F4):
  **17/17 EXACT**, objcmp: distance2d/distance3d/distance_squared2d/negate_vector2d NODUP->ANY, bytes
  identical, NO surplus. The AB spelling reproduces January's exact action_vehicle helper emission SET
  (distance_squared2d emitted only because distance2d's body references it) = independent attestation.

### P6 rejected-trial losses reproduced + object_lights repaired (lab/trial/*, object_lights/cand1.c)
- LAB replication at this base: header A and AB both lose object_lights::_find_point_lights_for_object_in_cluster
  (EXACT in base). alndiff: only the inlined distance3d x87 accumulation (Jan k*k first; AB i*i first) +
  2 rows of '$'-literal reloc display. rasterizer_frame_statistics_draw is ALREADY a residual at this base
  (bytes drift under AB only); ai::_ai_test_line_of_sight is a residual (size 1040!=1008) whose bytes drift
  under A and AB (park staleness question).
- /Od find_point_lights (0x787870, od/find_point_lights.txt): `push center; lea eax,[light+0x30]; call
  distance3d` -> source `distance3d(&light->position, center)`; NO light_position slot (locals: light_index,
  reference_index, light, distance, attenuation, intensity, index, dimmest_index, minimum_intensity).
- Hypothesis (M8: one real local too many flips the inlined x87 leaf): production's
  `real_point3d const *light_position = &light->position;` is unattested and compensates the header error.
- object_lights/cand1.c = production with that local removed, call `distance3d(&light->position, center)`.
  base header: 42 EXACT + find_point_lights residual. **AB header: 43/43 EXACT.**
  => the /Od-attested header + /Od-attested caller are jointly exact; the rejected trial failed because it
  changed only one of the two compensating errors.

### S1 full-board AB sweep launched (tools/sweep.py; results sweep/AB.tsv, resumable)
- Every build.ninja Halo unit whose preprocessed TU uses real_math.h, header AB, overrides =
  {action_vehicle, object_lights, path_obstacles, action_charge, bored_camera, player_control}/cand1.c.
  Per unit: every code section of lab obj vs build/base obj; tags LOSS/GAIN/DRIFT (vs January), NEW/GONE,
  SURPLUS-DIFF (helper not in the unit's January split). Resume: rerun same command, done units skipped.

### M2 /E LAB method is NOT neutral for name-count-sensitive functions (method defect found)
- AB sweep (S1) flagged LOSS decals::_decals_delete_permanent_from_cluster and
  rasterizer_frame_statistics::_rasterizer_frame_statistics_draw. Control: the SAME .i path with the
  UNMODIFIED header already loses both (lab/trial/decals_base: 3 residuals vs production 2; frame_stats
  lab-base residual vs production 10/10 EXACT). Direct gate of production: both EXACT.
  => preprocessing (/E + #line) shifts the TU name-count arena for these two canaries; S1 LOSS rows on them
  are artifacts. action_vehicle/object_lights controls were neutral. Need a no-/E method: mirror tree.
- Math TUs (geometry, matrix_math, real_math) resolve the real header via includer-dir; handled by TU copies
  beside the lab header (sweep/math_{base,AB}.tsv): controls SAME; AB: geometry GAIN
  _convex_hull2d_perimeter (residual->EXACT) + NEW _distance_squared2d.
- SAFETY: mirror/<V>/source/* are JUNCTIONS into source/. Remove ONLY via tools/rmmirror.py (os.rmdir per junction); never rm -rf / shutil.rmtree the mirror.

### M3 mirror method (tools/mkmirror.py, mgate.py, msweep.py) - FAITHFUL
- mirror/<V>/source: junctions to every source dir except math (copied, real_math.h swapped); compile the
  TU copy directly with build.ninja flags, /Isource* remapped. CONTROL m_base.tsv: 446/446 units SAME vs
  build/base (every code section identical) incl. both name-count canaries. Supersedes the /E sweep (S1).

### S2 mirror AB sweep (sweep/m_AB.tsv; overrides = 6 provider cands + object_lights/cand1.c)
- LOSS: bitmap_drawing::_bitmap_copy, rasterizer_frame_statistics::_rasterizer_frame_statistics_draw
  (neither TU calls distance2d/3d -> pure name-count: AB removes 2 declared names (`v`,`v`) from
  real_math.h; both are the documented name-count canaries).
- GAIN: geometry::_convex_hull2d_perimeter (parked, class unclassified -> park would retire).
- DRIFT (residuals): actor_perception_refresh_danger_zone, ai_test_line_of_sight (PARKED),
  player_set_action_result, collisions::_collision_move_point. __rasterizer_model_draw and decals
  SAME under mirror (their /E flags were artifacts).
- All other units: exact counts unchanged (units 189/189, leaf_map 25/25, biped_limp_noodle 6/6,
  object_lights 43/43 with cand1, action_vehicle via cand1). Surplus: _distance3d changes in 17 units,
  NEW _distance_squared3d (12 units) / _distance_squared2d (actor_moving, geometry, path_obstacles),
  GONE _magnitude3d (5 units) -> identity vs January selected copies to verify.

### S3 name-count ORACLE sweep ABn2 (LAB ONLY - dummy names are never a landing) sweep/m_ABn2.tsv
- ABn2 = AB + `struct lab_oracle_name_1;`/`_2;` before distance2d/distance3d (restores the TU name count).
- Result: bitmap_copy and rasterizer_frame_statistics_draw losses VANISH; collisions::_collision_move_point
  drift VANISHES. Remaining deltas are exactly the direct distance3d users + geometry GAIN:
  DRIFT actor_perception_refresh_danger_zone, ai_test_line_of_sight (parked), player_set_action_result.
  => H-NC CONFIRMED: the rejected trial's canary losses are the -1/-2 declared-name shift, not codegen.
  Oracle reading: if AB is January's body, January's TUs declared exactly 2 more names (in the span before
  those canaries, same in both TUs => most plausibly inside real_math.h) than our reconstruction.

### P7 players::_player_set_action_result (players/cand1.c) - GAIN under AB
- /Od 0x5d0d50 (od/f_0x5d0d50.txt): pointer locals unit_position/current_position/new_position are genuine
  (statement-order stores); the two distance3d results go fstp temp -> movss xmm0 -> [spill] interleaved
  with the 2nd call's pushes -> fstp temp -> comiss: ONE expression, NO current_distance/new_distance locals.
- cand1: drop the two real locals; `set_action = distance3d(unit_position, current_position) >
  distance3d(unit_position, new_position);`
- strict (tools/fnexact.py): header base 67/70 (set_action_result residual); **AB and ABn2: 68/70,
  _player_set_action_result EXACT**; the other two residuals (teleport_internal, examine_nearby_device)
  unchanged. Under the current header the /Od shape does NOT close it -> AB is the header that makes the
  /Od-attested caller exact (4th independent attestation after action_vehicle set, object_lights, geometry).

### F5 ai_test_line_of_sight (parked) under AB
- /Od 0x48aec0 (od/ai_los.txt): frame 0xf8 with RTC aggregates collision/perpendicular/p0a/p0b/p1a/p1b/p1c;
  `distance` is a function-scope local with RTC-uninit checks (a goto enters after its assignment). Our
  reconstruction is structurally different (size 1040 vs 1008, parked). Under AB only the inlined distance3d
  x87 leaf order flips (ailos_{base,ABn2}.aln: 1 block); the park would need RE-MEASUREMENT, not a loss.
- actor_perception_refresh_danger_zone: large residual (234 vs 238 aligned diff lines), not parked.

### P8 each half of AB is attested by January bytes separately (mirror A / B)
- geometry::_convex_hull2d_perimeter: A (distance3d only) 26/30 (still residual); **B (distance2d only)
  27/30 EXACT** => January's geometry bytes attest distance2d == square_root(distance_squared2d(a,b)).
- distance3d half attested by: action_vehicle _distance3d bytes (4cdb6852), object_lights (P6),
  players (P7). So both removed `v` names are genuinely absent in January's bodies; the -2 name-count
  deficit is a REAL reconstruction gap elsewhere in the shared span (not a reason to keep the wrong bodies).
- /Od note: 0x4419c0 (evaluate_seat) also calls distance_squared2d directly (0x441c9d) and distance2d 4x,
  but our January-exact evaluate_seat has 2 distance2d and no distance_squared2d -> later-build drift;
  January's action_vehicle _distance_squared2d emission is explained by the B body (P5).

### P9 canary name-count windows under AB + k oracle names (LAB only; mlab/*_ABk*.obj)
- k:        0     1     2     3     4     5
- bitmap_copy          F     T     T     F     F     T
- frame_statistics_draw F    F     T     F     F     F
- => the unique net shift keeping both exact in k=0..5 is k=2 (= today's count). January's span before
  both functions declared exactly 2 more names than AB-ours. Law: "a prototype costs 2" => ONE genuine
  missing prototype in real_math.h (shared by both TUs, the source of the -2) would compensate exactly.

### P10 genuine-name compensation candidates (lab/ABP, lab/ABC)
- January real_math.obj defines 5 functions real_math.h does not declare: line_from_planes3d (declared in
  focused header real_math_planes.h, comment: kept out of real_math.h ONLY because of declaration-count
  sensitivity; collisions_obj_opus5_100k: "regressed two distant exact functions and drifted four parks"),
  pin_normal_to_cone3d (focused real_math_cones.h, aim_assist only), pill_intersects_rectangle2d,
  vector_intersects_triangle2d/3d (no callers). Genuine owner header of all = real_math.h.
- ABP = AB + line_from_planes3d prototype after point_from_planes3d in real_math.h, removed from
  real_math_planes.h. ABC = AB + pin_normal_to_cone3d before sphere_intersects_cone3d, removed from cones hdr.
  Expected: net 0 names for every real_math.h includer except the focused header's own consumer (-2).
- RESULT (sweep/m_ABP.tsv, m_ABC.tsv): BOTH FAIL. ABP loses bitmap_2d_alpha_bleed,
  decals_delete_permanent_from_cluster, race_engine_player_update, race_touch_flag,
  rasterizer_frame_statistics_draw, __rasterizer_model_transparent_geometry_submit (ABC also get_edge_vertex,
  error_geometry_polygon). A 4/5-parameter prototype is NOT a 2-name item (parameters count) -> REJECTED.
- Calibration (oracle, mlab/*_ABq*.obj; bitmap_copy/frame_stats/race_touch_flag): `void f(void);` == k=2
  (all T,T,T); `f(long)` F,F,T (~k=3); `f(long,long)` T,F,T; `f(4 params)` T,F,F. => a genuinely missing
  ZERO-PARAMETER prototype owned by real_math.h compensates AB exactly.

### P11 genuine 2-name candidate: get_random_seed prototype (variant ABR)
- random_math.h is a reconstruction-era header (2026-09-04) holding lock/unlock_global_random_seed,
  get_random_seed, random_seed_debug_log; real_math.h already owns a "prototypes/RANDOM_MATH.C" section.
- CL /Zs /W3 network_client_manager.c: C4013 'get_random_seed' undefined (line 1674); the TU includes
  real_math.h but NOT random_math.h => January needed a visible prototype in a header it includes.
- ABR = AB + `unsigned long get_random_seed(void);` after get_global_local_random_seed_address in
  real_math.h, removed from random_math.h (moved, not duplicated).
- RESULT sweep/m_ABR.tsv (full board, mirror, all cands): **ZERO LOSS**. Canaries bitmap_copy,
  rasterizer_frame_statistics_draw, collision_move_point all SAME. random_math.h consumers (game,
  player_effects, network_server_manager) and network_client_manager: no function change.
  GAIN: geometry::_convex_hull2d_perimeter (park retires), players::_player_set_action_result (cand1),
  object_lights 43/43 (cand1). DRIFT (residuals): actor_perception_refresh_danger_zone (not parked),
  ai_test_line_of_sight (PARKED -> re-measure). Surplus: _distance3d content changes (17 units),
  NEW _distance_squared3d / _distance_squared2d, GONE _magnitude3d (janexact counts unchanged).
- surplus_check (tools/surplus_check.py sweep/m_ABR_objs): _distance3d x18 units, _distance_squared3d x11,
  _distance_squared2d x3, _magnitude_squared3d, _valid_real_normal3d, _convex_hull2d_perimeter ALL
  EQUAL-TO-JANUARY-SELECTED. Only non-equal = _ai_test_line_of_sight (January-owned parked residual).

### L1 link tests (tools/pairlink.py = Link.Exe pair, both orders; provider_link.py)
- production objects.obj provider_link: FAIL x4 (_point_in_sphere/path_obstacles, _real_random/action_charge,
  _signed_angular_difference/player_control, _valid_real_vector3d_axes2/bored_camera); _valid_real_vector3d PASS.
- objects.obj x {path_obstacles,action_charge,player_control,bored_camera}/cand1.obj: PASS both orders,
  ZERO LNK2005 of any symbol.
- new surplus of cands (provider_link --baseline): action_charge/player_control none; bored_camera all PASS;
  path_obstacles cand1: _distance_squared2d FAIL vs production action_vehicle (NODUP), PASS vs
  action_vehicle/cand1_ABR.obj -> cand1 requires the action_vehicle+ABR packet.
- path_obstacles/cand2.c (MINIMAL, header-independent): remove only point_in_sphere rename/#undef/hand copy.
  gate rows identical (15+1 pre-existing residual); objcmp only _point_in_sphere NODUP->ANY same bytes;
  no new surplus; objects.obj x cand2 PASS both orders.
- objects admission (production objects.obj): gate 121/121 EXACT; object_audit PASS; pdb_storage 0
  disagreements (225 split symbols); surplus_identity 26 candidate-only COMDATs, 0 not identical;
  combined Link.Exe objects + path_obstacles/cand2 + action_charge/cand1 + player_control/cand1 +
  bored_camera/cand1: 0 LNK2005/LNK1169 forward AND reverse. => objects OBJECT_COMPLETE_CANDIDATE once the
  4 provider cands land (no objects.c change needed).

### P12 decals is ALSO a NODUP provider blocking leaf_map/biped_limp_noodle (not in their rejection entries)
- provider_link on PRODUCTION leaf_map: FAIL x5 (_distance3d/action_vehicle, _plane3d_negate,
  _project_point2d, _projection_sign_from_vector3d /decals, _project_point3d /path_obstacles);
  biped_limp_noodle: FAIL x2 (_distance3d, _plane3d_from_point_and_normal /decals).
- decals.c renames 6 helpers (+hand copies). plane3d_negate's genuine use = bsp3d.h:55 inline
  (bsp3d_get_plane_from_designator, an EXACT decals row).
- decals/cand1.c (remove all 6 renames + hand copies): 28 EXACT + 5 residual: LOSES
  _decals_delete_permanent_from_cluster (name-count canary; 6 hand-copy defs removed), _plane2d_from_points
  and _project_point2d (header bodies differ from January's selected decals copies; relocs 11!=12).
  plane3d_negate/plane3d_from_point_and_normal/projection_sign_from_vector3d/triple_product3d: same bytes.
- decals/cand2.c (only plane3d_from_point_and_normal rename+copy removed) and decals/cand3.c
  (plane3d_from_point_and_normal, plane3d_negate, projection_sign_from_vector3d, triple_product3d):
  gate rows IDENTICAL (31 EXACT + 2 pre-existing residuals, canary decals_delete_permanent_from_cluster kept);
  objcmp: those helpers NODUP->ANY same bytes; cand3 also drops invented surplus _plane3d_negate_inline.
  plane2d_from_points/project_point2d must stay hand copies (header bodies differ from January).
- leaf_map ALSO emits _project_point2d DIFFERENT from January's selected decals copy (surplus_identity) ->
  second real_math.h body defect blocking leaf_map, independent of distance3d.

### F6 /Od project_point2d (0x56d680, od/pp_0x56d680.txt)
- `short x`/`short y` locals (word stores), asserts projection range + ~(sign&~1), then ONE ternary store
  p3d->n[projection] = (fabs-like(n[proj] - 0.0f) < 0.0001f) ? 0.0f : (...)/n[proj] (later-build fabs spelling).
- => attests the decals hand-copy shape (January-exact): short locals + ternary; the real_math.h body
  (long locals, if/else, >= polarity) is unattested. Name count identical (2 locals + 5 params).

### P13 ABRP = ABR + attested project_point2d body (short locals + ternary) (sweep/m_ABRP.tsv)
- overrides + decals/cand4.c (= cand3 + project_point2d rename/copy removed).
- ALL project_point2d emitters/callers keep exactness (breakable_surfaces 12/12, collision_bsp 27/30 same,
  collision_features 20/20, render_debug 36/36, leaf_map 25/25, decals 31/33 rows identical); surplus
  _project_point2d changes in 5 units.  BUT canaries return: LOSS bitmap_copy, rasterizer_frame_statistics_draw,
  DRIFT collision_move_point => the if/else->ternary respelling shifts the per-TU numbering (F1 key: inline
  body IL structure consumes numbering) although declared names are unchanged.
- ABRP + k oracle names: k=2 restores bitmap_copy/frame_stats/race_touch_flag (k1 T,F,T; k3/k4 F,F,T;
  k5/k6 T,F,T) => the attested ternary body costs 2 units vs the if/else body; needs another genuine
  2-unit (zero-parameter prototype) recovery.
- C4013 evidence also exists for lock_global_random_seed/unlock_global_random_seed (main.c 2902/2983; main.c
  includes real_math.h, not random_math.h). main.c and network_client_manager.c share 58 headers (incl.
  game.h, main.h, real_math.h) => consumer evidence shows the 3 random prototypes were visible via SOME
  shared header, NOT uniquely real_math.h. The canary oracle fixes only the COUNT in canary-visible headers
  (ABR: one zero-param prototype; ABRP: two). Choice of which is UNDERDETERMINED -> owner ruling item.

### P14 ABRPL = AB + attested project_point2d + {lock,unlock}_global_random_seed moved into real_math.h
  (get_random_seed stays in random_math.h) (sweep/m_ABRPL.tsv, overrides incl. decals/cand4.c)
- FULL BOARD: **ZERO LOSS**; GAIN convex_hull2d_perimeter, player_set_action_result; DRIFT only
  actor_perception_refresh_danger_zone (unparked residual) + ai_test_line_of_sight (parked residual).
- surplus_check m_ABRPL_objs: _distance3d x18, _distance_squared3d x11, _distance_squared2d x3,
  _project_point2d in collision_bsp/collision_features/render_debug/leaf_map ALL EQUAL to January selected.
  Exception: breakable_surfaces' _project_point2d still differs (525f43d5 at base too; TU-specific,
  see breakable_surfaces_projection_provider_20260923.md; not in this task's blocked set).

### L2 leaf_map / biped_limp_noodle under ABRPL + candidate providers
- providers compiled under ABRPL mirror: action_vehicle/cand1 17/17, decals/cand4 31/33 (2 pre-existing
  residuals, rows identical), path_obstacles/cand1 15/16 (pre-existing residual).
- leaf_map ABRPL obj: 25/25 EXACT, object_audit PASS, tools/provlink2.py (provider_link + --subst of the
  6 candidate providers) 33 rows PASS, surplus_all: 18 candidate-only COMDATs 0 not identical.
- biped_limp_noodle ABRPL obj: 6/6, audit PASS, provlink2 42 rows PASS, 21 surplus 0 not identical.
- objects production obj with substituted providers: provlink2 PASS; 26 surplus 0 not identical.

### F7 other task consumers at base
- hud_unit: 22/22 EXACT, provider_link PASS (actor_combat _fast_ftol already ANY) -> the _fast_ftol rejection
  is STALE. Remaining non-provider blocker: object_audit FAIL storage of _get_hud_state and
  _hud_update_unit_local_player (split external, ours static, cachebeta public=False; symbols.json has
  _get_hud_state twice: line ~3429 without "static", ~3452 with) -> symbols.json attribution, csplit regen.
- hud_draw: provider_link PASS (fast_ftol resolved) but January function _fast_ftol_C (56 B) UNWRITTEN.
- path_smoothing: 6/6 EXACT; provider_link FAIL only _cross_product2d (actor_combat NODUP) -> BLOCKED (F2).
- decals: 2 residuals (closed lane) + _cross_product2d NODUP + invented surplus _plane3d_negate_inline.

### D1 integrator patch set written (scratch/w/provider/patch/)
- layer1/ (header-independent; each verified rows-identical + objcmp-only-selection):
  action_charge.c.diff, player_control.c.diff, bored_camera.c.diff, path_obstacles.c.diff (cand2),
  decals.c.diff (cand3).
- layer2_ABRPL/ (full-board zero-loss mirror sweep m_ABRPL.tsv): real_math.h(.diff), random_math.h(.diff),
  action_vehicle.c.diff, object_lights.c.diff, players.c.diff, path_obstacles.c.diff (cand1, supersedes
  layer1), decals.c.diff (cand4, supersedes layer1). Parks: retire _convex_hull2d_perimeter; re-measure
  _ai_test_line_of_sight base -> size 1040 relocs 50 sha c524fe19529bfd4a72e95be52a875925bcc929d347b37471eb16b75bd393da78.
- layer2_ABR_alternative/: smaller header packet (distance2d/3d + get_random_seed moved), zero-loss m_ABR.tsv.

### C1 board link census (tools/linkcensus.py -> sweep/linkcensus.tsv; 288 fully code-exact units)
- selected-provider link PASS: production 250/288; Layer1 260/288; Layer2(ABRPL) 268/288; ZERO regressions.
- Layer1 FAIL->PASS (10): camera_scripting, first_person_camera, flying_camera, following_camera,
  orbiting_camera, static_camera (_valid_real_vector3d_axes2); items (_plane3d_negate); objects (4 syms);
  rasterizer_xbox (_plane3d_from_point_and_normal); scenario (_interpolate_scalar).
- Layer2 FAIL->PASS (8 more): actor_firing_position, path, game_engine_king, model_animations, units
  (_distance3d); render_debug (_project_point2d); leaf_map (_distance3d,_project_point2d,_project_point3d);
  biped_limp_noodle (_distance3d).
- still failing at L2: _cross_product2d x5 (actor_combat, BLOCKED F2), recorded_animation_playback(_v1)
  duplicate NODUP pairs, _object_get_bounding_sphere x2 (assert-string provider mismatch), misc.

### C2 admission status of units the layers unlock
- Currently "Matching" but FAIL selected-provider link at production (latent contradictions), cured by
  Layer1: camera_scripting, first_person_camera, flying_camera, following_camera, orbiting_camera,
  static_camera, items, scenario.
- NonMatching + link-clean after the layers, full audit:
  objects (L1): 121/121, audit PASS, pdb 0 disagreements, 26 surplus identical -> COMPLETE CANDIDATE.
  leaf_map (L2): 25/25, audit PASS, 18 surplus identical, provlink2 PASS -> complete candidate (header ruling).
  biped_limp_noodle (L2): 6/6, audit PASS, 21 surplus identical, PASS -> same.
  ai/path (L2): 32/32, audit PASS, pdb 0, 10 surplus identical; production blocker was ONLY _distance3d.
  render/render_debug (L2): 36/36, audit PASS, pdb 0, 11 surplus identical; production blocker ONLY
    _project_point2d (needs the ABRPL project_point2d body, not in ABR).
  NOT complete (symbol-storage attribution, out of provider scope): rasterizer_xbox (21 sym storage),
  actor_firing_position (18), model_animations (5), units (47), game_engine_king (3 pdb non-public).
- No parks in objects/leaf_map/biped_limp_noodle/path/render_debug.
- mirrors removed via rmmirror.py after the campaign (rebuild: python scratch/w/provider/tools/mkmirror.py <V>; lab/<V>/inc keeps every header variant). source/ verified intact, 0 tracked changes.

## FINAL SUMMARY (resume run 1)
- Layer1 (header-independent, landable now): action_charge/cand1, player_control/cand1, bored_camera/cand1,
  path_obstacles/cand2, decals/cand3 -> objects COMPLETE CANDIDATE; cures latent link failures of 8 Matching
  objects (6 cameras, items, scenario) + rasterizer_xbox link. Zero row changes in providers.
- Layer2 (owner ruling: shared-header packet ABRPL): distance2d/distance3d -> square_root(distance_squared*(a,b))
  [/Od + 5 January-byte attestations], project_point2d -> short locals + ternary [/Od + January decals bytes],
  lock/unlock_global_random_seed prototypes moved random_math.h -> real_math.h [C4013 main.c; name-count
  compensation, choice underdetermined vs get_random_seed (ABR alternative)], + action_vehicle/cand1,
  object_lights/cand1 (/Od), players/cand1 (/Od), path_obstacles/cand1, decals/cand4.
  Full mirror board sweep: ZERO LOSS; GAIN geometry::_convex_hull2d_perimeter, players::_player_set_action_result;
  park retire _convex_hull2d_perimeter, re-measure _ai_test_line_of_sight. Unlocks leaf_map, biped_limp_noodle,
  ai/path, render/render_debug (all audits pass).
- BLOCKED: path_smoothing + decals(_cross_product2d; only genuine use in held actor_aim_grenade);
  hud_draw (_fast_ftol_C unwritten; fast_ftol provider already fixed); hud_unit (symbols.json storage).
