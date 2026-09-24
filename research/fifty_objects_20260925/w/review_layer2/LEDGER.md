# Adversarial review of Layer 2 commit 931ed8dc (slug scratch/w/review_layer2)

Base: worktree claude-fifty-objects-20260925 at 931ed8dc. Reviewer: scratch only.

## Findings / probes

### R1 committed tree == provider ABRPL packet
- scratch/w/provider/patch/layer2_ABRPL/{real_math.h,random_math.h} are byte-identical (cmp) to the landed
  source/math headers. git diff 931ed8dc touches exactly the 8 files the provider packet lists.

### R2 claim (5) board sweep: CONFIRMED
- `python -m tools.campaign.stable_verdicts snapshot scratch/w/review_layer2/stable_now.json` -> 8245 fns, 7603 exact;
  cmp vs scratch/campaign/b2_stable.json: BYTE-IDENTICAL. diff b1->b2: +2 (_player_set_action_result 240,
  _convex_hull2d_perimeter 96), 0 regressions.
- Freshness of build/base (the snapshot reads it): 273 objs newer than real_math.h (mtime 20:43:39) = b2_ninja1's
  273 CL lines; the 194 older objs were each scanned with CL /Zs /showIncludes (tools/incscan.py; control:
  action_vehicle/game/main hit): NONE includes real_math.h or random_math.h -> build/base reflects the tree.

### R3 helper COMDAT identity census (build/base vs January selected; helper_census.py; helper_census_base.txt)
- _distance2d (3 ANY), _distance3d (19 ANY), _distance_squared2d (6), _distance_squared3d (44), _negate_vector2d (1),
  _point_in_circle (1), _point_in_sphere (6), _rotate_vector2d (3), _magnitude3d (63): ALL ANY, ALL equal to
  January's single selected definer. No NODUP copy of any of them remains on the board.
- _project_point2d: 5 ANY equal (decals, collision_bsp, collision_features, render_debug, leaf_map);
  breakable_surfaces ANY DIFFERS (sha 8a804070 vs January 48bc73c5) - also _project_point3d/_vector_from_points3d
  differ there (TU-specific; pre-existing class, see below).

### R4 standalone helper-body lab (lab_pp.c compiled with decals flags; LAB ONLY)
- project_point2d landed body == January decals _project_point2d (224/12 relocs, 48bc73c5); the pre-L2 if/else
  long-local body differs (11 relocs, f30a455b) -> body change is byte-attested.
- realcmp(plane->n.n[projection], 0.0f) spelling (the literal /Od 0x56d680 shape: fabs-call(n - 0.0f) < 1e-4,
  0x93dd38 = 0.0f) is ALSO byte-identical to January (48bc73c5): January bytes cannot separate the two; /Od
  literally shows the `- 0.0f` operand, i.e. realcmp. Landed spelling = /Od modulo that folded operand.
- /Od project_point2d: short x/y (word stores [ebp-4]/[ebp-8]), 2 asserts, n[x]/n[y] copies, ONE store to
  n[projection] via a temp [ebp-0x10] written in both arms => single ?: store attested (an if/else would store
  p3d->n[projection] directly in each arm).
- distance3d: landed == January action_vehicle _distance3d (4cdb6852); pre-L2 body 01d7809a differs.
- distance2d: landed AND pre-L2 body BOTH == January _distance2d (3c78450b): the standalone bytes do NOT attest
  the distance2d change. Its attestation is (a) /Od 0x443e70 = push b; push a; call distance_squared2d(0x443ea0);
  fstp [esp]; call square_root - no local; (b) January action_vehicle emits _distance_squared2d with no direct
  user (only a body that references it explains the referenced-COMDAT); (c) geometry _convex_hull2d_perimeter
  exact only with the new body (provider P8; board confirms gain).
- /Od distance3d 0x42dc50 = call distance_squared3d(0x42dc80: RTC 12-B v, vector_from_points3d 0x42e3a0 +
  magnitude_squared3d 0x42df70) then square_root(0x42e320) - no local. Confirmed.

### R5 lab header method (tools/labgate.py, tools/mkvariants.py; LAB ONLY)
- METHOD DEFECT found+fixed: putting real_math.h copies both at <lab>/ and <lab>/math/ makes the two spellings
  open TWO distinct files; the second is skipped by the #ifndef guard but the file is still opened (its
  #pragma once sits inside the guard) and that alone flipped rasterizer_frame_statistics_draw (name-count
  canary) to residual. TU path is inert (NOH / X/y controls exact).
- Fixed method: ONE copy at <lab>/math/real_math.h, lab /I dirs placed BEFORE the build.ninja /I list, TU copied
  into <lab>. /showIncludes proves every real_math.h open is the lab file.
- CONTROL V0 (byte copies): 9 canary units (bitmap_drawing, rasterizer_frame_statistics, game_engine_race,
  bitmap_utilities, decals, geometry, rasterizer_xbox_models, error_geometry, collisions): basediff 0 for EVERY
  code section vs build/base. Method faithful.

### R6 random-prototype move: strip test + full-genuine-move test (9 canary units; lab/V1..V4; LAB ONLY)
- V1 = landed header WITHOUT the move (lock/unlock back in random_math.h) = provider ABRP:
  LOSS _bitmap_copy (2784), _rasterizer_frame_statistics_draw (4176), _get_edge_vertex (416);
  DRIFT _collision_move_point (residual). => the move is LOAD-BEARING (strip test fails).
- V2 = FULL genuine move (lock, unlock, get_random_seed, random_seed_debug_log all into real_math.h's
  RANDOM_MATH.C section; random_math.h emptied): LOSS _bitmap_2d_alpha_bleed, _decals_delete_permanent_from_cluster,
  _rasterizer_frame_statistics_draw, __rasterizer_model_transparent_geometry_submit.
- V3 = get_random_seed moved instead of lock/unlock (ABR + P body): LOSS bitmap_copy, frame_statistics_draw;
  DRIFT collision_move_point.
- V4 = landed + project_point2d condition as realcmp(plane->n.n[projection], 0.0f): all 9 units identical to V0
  (canary-inert here; not board-swept).
- READING: exactly the lock+unlock subset (+4 name units) balances the -4 from the body corrections; the evidence
  for the destination (C4013 in main.c; supplied genuine random_math.c includes real_math.h and NOT random_math.h;
  real_math.h already owns a RANDOM_MATH.C section) applies EQUALLY to get_random_seed (C4013 in
  network_client_manager.c) and random_seed_debug_log, and moving those too breaks 4 other exact functions.
  => destination plausible/genuine, but the SUBSET is chosen by the name-count oracle (provider P13 itself:
  "choice UNDERDETERMINED -> owner ruling item"; wave1 result.json blockers: "Owner ruling on the shared-header
  packet"). No owner ruling found in docs/ or scratch/campaign/. Commit message presents it as evidence-driven
  placement and omits that it is canary-load-bearing count compensation.

### R7 C4013 census (CL /Zs /W3; tools/c4013.py) for random-seed consumers
- landed: main.c 0; network_client_manager.c:1674 C4013 'get_random_seed' STILL present; others 0.
- V1 (pre-move): main.c:2902/2983 C4013 lock/unlock + network_client_manager get_random_seed.
- V2 (full move): 0 everywhere.
=> the commit's cited evidence ("main.c calls them without random_math.h") applies identically to get_random_seed,
   which stays behind only because moving it (V2/V3) regresses exact functions.

### R8 gates / links / surplus on the touched units (production source, gate.py --all)
- action_vehicle 17/17; path_obstacles 15/16 (_obstacles_test_circle pre-existing); decals 31/33 (2 pre-existing,
  closed lane); object_lights 43/43; players 68/70 (examine_nearby_device, teleport_internal pre-existing);
  geometry 27/30 (3 pre-existing). b1->b2 per-unit: only players +_player_set_action_result, geometry
  +_convex_hull2d_perimeter.
- provider_link: action_vehicle PASS, path_obstacles PASS, players PASS; decals FAIL only _cross_product2d
  (actor_combat NODUP, pre-existing, brief sec.7), object_lights FAIL only _object_get_bounding_sphere
  (action_vehicle NODUP hand copy, assert string, pre-existing, brief sec.7).
- surplus_identity: action_vehicle 12/0 not-identical, path_obstacles 15/0, decals 21/0, players 13/0;
  object_lights 1 DIFFERENT (_object_get_bounding_sphere, pre-existing); geometry 1 DIFFERENT
  (_plane2d_from_points, header body != January decals copy, untouched by L2, pre-existing).

### R9 caller respellings vs /Od
- players 0x5d0d50: frame 0x20; slots player/set_action(byte)/unit,current,new position pointers; the first
  distance3d result goes fstp temp -> xmm0 -> spilled to a second temp across the second call -> comiss: a single
  expression; no current_distance/new_distance slots. ATTESTED. Statement order of the 3 pointer stores matches.
  Lab: the /Od-shaped third arm (`else set_action = action_result > player->action_result;`, no = FALSE)
  gives size 256 != 240 -> January keeps the landed init form; not a defect.
- object_lights 0x787870: `push center; mov eax,light; add eax,0x30; push eax; call distance3d(0x42dc50)`;
  frame 0x30 has no light_position slot (locals light_index, state(RTC 4B), light, distance, attenuation,
  intensity, index, dimmest_index, minimum_intensity, temp). ATTESTED. (RTC names the reference cursor `state`;
  ours `reference_index` - pre-existing naming, out of L2 scope.)

### R10 the subset is count-only: V5/V6 (LAB ONLY)
- V5 = {lock, get_random_seed} moved (unlock stays), V6 = {unlock, get_random_seed}: on all 9 canary units both are
  IDENTICAL to the landed state (basediff 0 everywhere). => bytes fix only the COUNT (two zero-parameter prototypes);
  which two is not byte-determined. lock+unlock is a reasonable tie-break (main.c calls both, C4013 pair) but the
  number moved is the oracle's.
- parks: `python -B -m tools.parked_functions` stale [] invalid [] at the landed tree.
- Fallback patch (only if the owner rejects the count-selected move): scratch/w/review_layer2/fallback_revert_except_players.diff
  (= git diff 931ed8dc 931ed8dc~1 for every touched file except players.c; `git apply --check` clean). players.c
  alone under the pre-L2 header: 67/70 (neutral, lab/PRE) and /Od-attested, so it may stay.
  Reverting ONLY the two prototype lines is WRONG: V1 shows it regresses 3 exact functions (7,376 B).

## VERDICT
- Verified correct: distance3d (/Od 0x42dc50 + January bytes 4cdb6852), distance2d (/Od 0x443e70 + emission set +
  geometry gain; standalone bytes are inert), project_point2d (/Od 0x56d680 short locals + single ?: store; January
  48bc73c5); all emitted helper COMDATs equal January's selected copies board-wide; the 3 provider .c files keep
  every January row; object_lights/players respellings /Od-attested and house-rule clean; stable snapshot identical
  to b2_stable.json and build/base provably fresh.
- NOT approvable without an owner ruling: the lock/unlock prototype move is load-bearing count compensation
  (strip test fails), the pair is count-selected (V5/V6 equivalent), the evidence-equal full move (V2) regresses 4
  exact functions, get_random_seed keeps its C4013, the provider explicitly required an owner ruling, and the commit
  message does not disclose any of this.
- Optional (not required): realcmp(plane->n.n[projection], 0.0f) is the literal /Od spelling, byte-identical to
  January and canary-inert on 9 units (V4); would need a full-board sweep before landing.
