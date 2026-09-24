# Review ledger: geometry + players header-packet function gains (Layer 2 ABRPL)

Reviewer slug: scratch/w/review_players (header-packet function gains)/
Base worktree: claude-fifty-objects-20260925 (canonical e9e62b78). Scratch only.

## Steps
- R0 read WORKER_BRIEF, provider LEDGER.md (389 lines), patch/layer2_ABRPL/{players.c,real_math.h,random_math.h}.diff.
- R1 method: full COPIES of source/ (no junctions) at trees/base and trees/ABRPL (patch/layer2_ABRPL real_math.h.diff +
  random_math.h.diff applied with patch -p1, CRLF restored). Patched headers == worker lab/ABRPL/inc and patch full copies
  (ignoring EOL). diff -rq trees: only real_math.h + random_math.h differ. Own harness rvgate.py (build.ninja flags,
  /Isource* remapped into tree, TU outside the unit dir, unit dir appended on /I like gate.py) + seccmp.py (every defined
  symbol section via coff_compare.section_infos_equal + COMDAT selection byte + storage).
- R2 players.c.diff: git apply --check OK; patched file == worker players/cand1.c (EOL-insensitive).
- R3 players gate rows (my compiles):
  gate.py production: 67/70 (examine_nearby_device size, set_action_result sha, teleport_internal size/relocs).
  gate.py candidate (current header, --out cand.obj): 67/70 identical row text.
  rvgate base-tree prod: 67/70; CONTROL seccmp vs build/base players.obj: 0/140 differing (harness faithful).
  rvgate base-tree cand: 67/70, only _player_set_action_result bytes differ vs build/base (still residual sha).
  rvgate ABRPL prod: 67/70 (set_action_result residual; header alone does not close it).
  rvgate ABRPL cand: **68/70, _player_set_action_result EXACT** (reproduced). Surplus deltas vs build/base:
  _distance3d content changed (ANY), NEW _distance_squared3d. -> identity check next.
- R4 players surplus (surplus_obj.py = surplus_identity logic on an explicit obj): production 12 candidate-only COMDATs,
  1 NOT identical (_distance3d vs January selected action_vehicle copy). ABRPL cand: 13, **0 not identical**
  (_distance3d now IDENTICAL to action_vehicle, NEW _distance_squared3d IDENTICAL to action_alert). => the header
  distance3d body is attested by January's own out-of-line _distance3d bytes (independent of players' gain).
  object_audit players ABRPL cand: FAIL(3) = examine_nearby_device, teleport_internal (pre-existing) + storage of
  _player_examine_nearby_unit (2/3, pre-existing); production FAIL(4). Players stays NOT complete (as claimed).
- R5 geometry (production geometry.c, no change): rvgate base tree 26/30 (control seccmp 0/129 differ vs build/base;
  gate.py production 26/30 same rows). ABRPL: **27/30, _convex_hull2d_perimeter EXACT**; only other delta NEW
  _distance_squared2d (IDENTICAL to January action_vehicle). surplus ABRPL 30 defs, 1 not identical =
  _plane2d_from_points vs decals (PRE-EXISTING at production, 29/1). object_audit ABRPL: 3 residual sections
  (hull3d_expand, polygon2d/3d_clip) 90 January symbols 0 differ.
  alndiff base vs January: only an x87 leaf-order block in the inlined distance2d accumulation (fld st(1)/fmulp vs
  fld st(2)/fmul) -> M8-class ordinal effect; weak alone, strong with R4/F4.
- R6 /Od re-dumped independently (od/): 0x443e70 distance2d and 0x42dc50 distance3d = push b; push a; call
  distance_squared*; add esp,4; fstp [esp]; call square_root; NO frame -> `return square_root(distance_squared*(a,b));`
  0x443ea0 / 0x42dc80 distance_squared2d/3d have the 8/12-byte RTC `v` local. CONFIRMED.
  /Od 0x5d0d50 player_set_action_result: frame 0x20; named slots player(-4) set_action(-5 byte) unit_position(-0xc)
  current_position(-0x10) new_position(-0x14); -0x18/-0x1c/-0x20 are expression temps: call1 fstp [-0x18],
  movss xmm0 <- [-0x18], pushes of call2 INTERLEAVED with movss [-0x1c] <- xmm0 (spill across call), call2 fstp
  [-0x20], comiss [-0x1c],[-0x20]; jbe -> 0 => ONE expression `distance3d(unit, current) > distance3d(unit, new)`,
  args (unit_position, current_position) / (unit_position, new_position) in that order. CONFIRMED.
  NOTE (pre-existing, not in the diff): /Od has NO entry store of set_action (no `= FALSE`) and the last arm is
  `set_action = action_result > player->action_result` (cmp; jle -> 0 else 1). Production keeps `boolean
  set_action = FALSE;` + `else if (...) set_action = TRUE;`. To be probed (R8).
- R7 launched full-board sweeps (rvsweep.py): trees/full = base + ABRPL headers + ALL layer1+layer2 .c diffs
  (patched files == worker cands: action_charge/player_control/bored_camera cand1, path_obstacles cand1, decals
  cand4, action_vehicle cand1, object_lights cand1, players cand1); control = trees/base. 447 units each.
- R8 FULL-BOARD sweep results (rvsweep.py, 447 cl units each):
  CONTROL trees/base: **447/447 SAME** (every defined section + COMDAT selection + storage == build/base) -> harness faithful.
  trees/full (L1+L2 packet): 419 SAME, 28 DIFF, **0 LOSS**, 0 compile failures. GAIN players::_player_set_action_result,
  geometry::_convex_hull2d_perimeter. DRIFT (residuals only): actor_perception::_actor_perception_refresh_danger_zone
  (not parked; alndiff lines 234->238), ai::_ai_test_line_of_sight (PARKED; 1040/50 sha 61a3ce7f -> c524fe19, alndiff
  264->270; park base must be re-measured, the worker's c524fe19... value is confirmed). Canaries bitmap_copy,
  rasterizer_frame_statistics_draw, collision_move_point SAME. All other DIFF rows = surplus/selection only
  (see sw_full.tsv). No January-owned data section changed anywhere.
- R9 surplus identity full vs production for all 28 DIFF units (surplus_full_vs_prod.txt): every _distance3d surplus
  (17 units) flips DIFFERENT -> IDENTICAL to January's selected action_vehicle copy; _project_point2d in collision_bsp/
  collision_features/render_debug/leaf_map flips DIFFERENT -> IDENTICAL; NO new non-identical surplus anywhere.
  Remaining non-identical rows are all pre-existing (breakable_surfaces x7, _object_get_bounding_sphere x2,
  geometry _plane2d_from_points).
- R10 provider link (provlink2 = provider_link + --subst; verified content-identical to scratch/tools/provider_link.py
  apart from the --subst hook), providers substituted with MY full-tree objs:
  players full: PASS (production players FAILS _distance3d vs action_vehicle NODUP -> cured).
  geometry full: FAIL(2) = _cross_product2d (actor_combat NODUP, BLOCKED F2) + _plane2d_from_points (decals hand copy);
  both pre-existing at production (production FAIL(3) incl. _distance2d, which Layer 2 cures). Not introduced.
- R11 ATTRIBUTION (mkvariant.py trees A/B/AB = base + only the distance3d / distance2d / both helper bodies):
  players cand: A 68/70 EXACT, AB 68/70, ABRPL 68/70; B 67/70 (residual). players prod (locals): residual under A/AB/B/ABRPL.
  geometry: B 27/30 EXACT, AB 27/30, ABRPL 27/30; A 26/30 residual.
  => players gain = distance3d body + no-locals caller; geometry gain = distance2d body. Neither needs the
  project_point2d (P) or lock/unlock (L) parts; gains stable across TU name-count shifts -1/-2/0 (A,B / AB / ABRPL)
  => not a name-count tie.
  BUT canaries under AB alone: LOSS bitmap_drawing::_bitmap_copy and rasterizer_frame_statistics::
  _rasterizer_frame_statistics_draw, DRIFT collision_move_point (A and B each lose frame_statistics_draw).
  => the header bodies CANNOT land without a name-count compensation; the compensation (L = lock/unlock moved to
  real_math.h, or R = get_random_seed) is the worker's declared owner-ruling item (choice underdetermined).
- R12 C4013 census (warn.py, CL /Zs /W3): players warnings identical base vs full (18); geometry unchanged; main.c loses
  the 2 C4013 for lock/unlock under full; network_client_manager KEEPS C4013 get_random_seed under full (ABR would cure
  it but leave main.c's). Neither alternative cures all random-seed C4013s -> confirms underdetermination; also C4013 on
  void(void)/int-returning calls is codegen-neutral, so it is weak evidence of January's header layout.
- R13 STRIP / /Od-fidelity test: players.v2_od.c = cand + the rest of the /Od shape (no `= FALSE` init, last arm
  `set_action = action_result > player->action_result;`): 256 != 240 under BOTH headers -> January rejects that part of
  the /Od (later revision). The diff takes from /Od only the part January bytes attest (no distance locals); the
  retained `= FALSE` + `else if ... TRUE` is pre-existing and January-attested (exact). The candidate spelling carries no
  decoration (plain one-expression form, attested pointer locals kept) -> nothing to strip.
- R14 data: build/report.json players 100% data (5349/5349), geometry 100% (2288/2288); seccmp shows no data section
  change in either obj under ABRPL. Park measurements: _convex_hull2d_perimeter full == January 96/0/71292feb (retire);
  _player_set_action_result full == January 240/5/ab7ce57f.
- R15 history check: geometry park reopen criterion (opus5 150k w2/w3): "a genuine j-first helper or source" -> the
  /Od+January-attested distance2d body meets it; prior rejected forms were hand-inlined `j*j+i*i` (not reused here).
  players reopen criterion (opus5 150k w3): "owner decision on the real_math.h distance3d/magnitude inline spelling"
  -> i.e. explicitly an OWNER decision. Fable rejections (noinline, hand-expanded distance) are not reused.
- R16 VERDICT written to REVIEW.md: approve=false, POLICY HOLD (exact-under-ruling), no technical defect. All claims
  reproduced; gains require an owner-ruled declaration-count compensation (AB alone regresses 2 canaries). Precedent:
  review_weapons / review_weather_particle_systems. players.c.diff neutral standalone (may be staged at zero credit).
