# Triage camera_render_fx (round 2) - LEDGER

Base f6d00a8c, worktree clean at start. No candidate source edits; no claims (triage only).
Objects: observer, render_cameras, cinematics, structures, weather_particle_systems,
player_effects, geometry, connected_geometry, render_debug, flags.

## Step 0: hints packets dumped -> hints_dump.txt

## render_debug
- hints: 36/36 exact, audit PASS, no link fail. Round-1 (audit_path_render, provider_repair, review_admit2)
  + owner queue item 5: blockers = (d) invented render_debug_globals_definition aggregate with
  opaque_after_* pads (D1 descriptive-name split vs D0) and (e) six extern boolean debug_*
  (answered by COMMON evidence; patch 06). Current tree still has the aggregate (render_debug.c:256-306)
  and `extern boolean debug_bsp;` (l.355). Owner queue: "No first-party names exist anywhere
  (cachebeta publics, HCEX.pdb, 2001 maps, atlas)".
- DECISION render_debug: OWNER (owner queue item 5, D1/D0 naming class; no first-party names).

## observer
- hints: _observer_update_positions S1 (production body: csmemset, frame 0x90, 1648). Round-1 fn2_observer:
  cand closes size/relocs/frame (1568/40/0x84), 8 insns x87 commutative-leaf order in the tail
  axes2 dot + first cross_product3d; ~20 natural /Od-attested shapes + ~120 M8 lab variants, stop rule fired.
  production.patch (zero credit structural upgrade) not applied at f6d00a8c (hints still show csmemset extra).
  real_math.h:1718 valid_real_vector3d_axes2 = normal3d(f)&&normal3d(u)&&realcmp(dot(f,u),0).
- DECISION observer: RETIRE (tie; round-1 reopen criteria are all decode-level, none met).

## render_cameras
- hints: _render_camera_build_frustum S1 3376 vs 3408 production, frame 0x6c vs 0x84 (production body).
  Round-1 fn2_render_cameras (~300 compiles): best owner-gated body 3376/113/frame 0x6c, 15 REAL regions:
  CP (1 hunk, M8 key needing +1 IL sym before CP1 / -1 mod 8 after; no attested source fact) and
  W-core (14 dependency-free placement hunks: mov eax,imm arg loads + plane-pointer reloads) which never
  moved under u 0..31, b 0..7, 64-combo /Od toggles. Stop rule fired.
- Probe: HCEX DIA2Dump -sym render_camera_build_frustum -> locals z_axis,y_axis,x_axis,viewspace_plane,
  viewspace_normal,frustum_midpoint,frustum_vertex,clip_plane (all aggregates; = /Od RTC set minus
  projected_bounds). No new scalar inventory -> reopen (b) not satisfied by HCEX symbols.
- DECISION render_cameras: RETIRE (W-core placement tie independent of CP; owner float-bits cast only
  buys the 3376 shape, not exactness).

## cinematics
- hints: _cinematic_render T (381/381, frame 56/56, relocs equal). Round-1 fn2_cinematics: only diff is the
  4-insn OR-operand emission order of the shadow-colour pack (+0x45d); alloc forcesearch proves not a global
  colouring tie; M8/H3/F1/name-count/unused-local oracles inert; context threshold (0 or >=4 conditional colour
  stores, or title_bounds not live) flips it; oct-betaP XBE byte-identical; p1..p7 /Od shapes inert. Stop rule.
- DECISION cinematics: RETIRE (reopen needs a dag.c operand-order decode or January-era source listing).

## structures
- hints: _sphere_intersects_cluster_portal S2 (production hand-expands point_from_line3d, 416 vs 432).
  Round-1 fn_structures: final.c EXACT 17/17 but load-bearing `double t = -plane_distance;` (strip test S1 real t /
  S2 inline negate residual; /Od 0x8a07bb negates inline, no t slot) -> REVIEW rejected pending owner ruling.
  Round-1 object blocker was provider_link FAIL(2) (path_obstacles/decals NODUP project_point3d /
  projection_sign_from_vector3d).
- Probe (f6d00a8c): provider_link on production structures.obj -> PASS (NODUP blockers gone after round-1
  provider packets). Re-gated round-1 final.c (copied to structures_r1_final.c): exact 17/0/0;
  provider_link PASS; object_audit PASS. => the ONLY remaining blocker is the double-t ruling.
- DECISION structures: OWNER (double-t inferred-construct ruling; a yes completes the object today).

## weather_particle_systems
- hints: render S2 (production), one_over_char_max storage 3 vs January 2, _real_local_random link FAIL.
  Round-1 fn_weather + REVIEW: fin2 20/20 EXACT + audit PASS but load-bearing `short unused_count = 0;`
  (/Od [ebp-0x24] init-only slot, invented name) = Lane D class I held item (HANDOFF_20260922 l.142).
  Storage-only fix (drop static on one_over_char_max; cachebeta public 0x25AAFC) is separable, zero credit.
- Probe (f6d00a8c): provider_link production -> FAIL(1) _real_local_random vs effects.obj (still).
  Census: January split defines _real_local_random ONLY in effects.obj (named-result `add esp,4` copy);
  first_person_weapons references it; our base emits it in effects, weather, breakable_surfaces,
  rasterizer_xbox_environment_fog. Weather's copy is unreferenced surplus from the header inline
  (real_math.h:1504 `return real_seed_random(...)`, pop ecx body). Header named-result fix = tree D of
  opus5_150k_w1 (0914): "no gain, breaks units" (declaration-count). Base has since changed (Layer 2),
  but it is a real_math.h edit inside the Codex reservation + a name-count tie family (brief s8).
- DECISION weather_particle_systems: OWNER (class-I /Od dead-scalar ruling) + provider dependency
  (_real_local_random header body; real_math.h reserved).

## player_effects
- hints: 3 residuals (add_continuous_effect S2 320, update_camera_impulse T 752, get_camera_effect_matrix S2 1312/1280).
  Round-1 wave4 player_effects: add_continuous EXACT only with Marathon PIN spelling in cseries.h (costs 5 exact
  board-wide; header-text key = name/decl-count family); camera_matrix cm2 /Od form 22 diff lines (cm2b 8),
  cross_product3d leaf order coupled to set_real_point3d; camera_impulse ikj vs kji dot order, all levers inert.
- Probe: pdb_storage -> 8 disagreements, all "split 2 / ours 3 / PDB-public False": _effect_scale_factor,
  _effect_scale_value, _get_shake_matrix, _player_effect_globals, _player_effect_update_camera_impulse,
  _player_effect_update_camera_shake, _player_effect_update_screen_flash, _render_screen_flash_type_map.
  Ours is already correct (static); the fix is symbols.json "static": true on those 8 rows (csplit regen).
  Zero credit, first-party (cachebeta publics) admission hygiene only.
- DECISION player_effects: RETIRE (3 function ties exhausted); note symbols.json storage packet.

## geometry
- hints: 27/30; polygon2d T 1008, polygon3d S2 1120, hull3d_expand S2 1808; link FAIL _cross_product2d
  (actor_combat P1, Codex-reserved) + _plane2d_from_points (decals NODUP; decals closed this lane).
- Read: Lane D REPORT (claude-lane-d-refresh-20260922/.../math__geometry/REPORT.md), structural owner packet
  20260923 s3/s4, collision_geometry 20260924, round-1 geometry provider repair (landed: geometry.c no longer
  hand-copies plane2d_distance_to_point).
  * polygon2d: B3 EXACT needs `dot_product2d((real_vector2d *)point,&plane->n) - plane->d` (/Od 0x6bf170);
    now that the geometry-local copy is gone this is a real_math.h header view cast (owner decision (b),
    structural owner packet; R18 lab: header /Od body keeps collision_bsp 0 diffs). real_math.h reserved.
  * hull3d_expand: h3e_c1 EXACT only with pointer-before-range-check (/Od 0x6bbbd0) = January-bug/UB owner class.
  * polygon3d: C2 EXACT only with point_from_line3d inline + header plane3d_distance_to_point outer-paren removal
    (v3), which LOST 3 exact rows board-wide (decals_delete_permanent_from_cluster, item_accelerate,
    rasterizer_frame_statistics_draw). Probe: /Od 0x6bd8a0 calls 0x56d580 (plane,point) = real
    plane3d_distance_to_point call, so a caller-local dot_product3d spelling is NOT attested. No new route.
- DECISION geometry: OWNER (2 function rulings: header cast, UB pointer order; plus P1 + decals provider);
  object completion additionally blocked by the polygon3d header-paren tie (not completable this round).

## connected_geometry
- hints: 7/10; find_or_add_vertex T 192 (parked), find_or_add_edge S2 240/256, triangle_coplanar T 384 (parked).
- Read n4 ledger (0915), structural owner packet s3/s4 (0923), Lane D REPORT
  (claude-lane-d-refresh-20260922/.../tool__connected_geometry/REPORT.md):
  * edge: EXACT with uninitialised `boolean direction;` (/Od 0x8bb620 _RTC_UninitUse("direction")) = January-bug
    owner class (owner packet s4 proposes admitting RTC-attested rows as a class). Fallback init costs 1 insn.
  * vertex: EXACT only via realcmp_epsilon macro (TU-local; only realcmp 2-arg is January-string-attested) -> owner.
  * coplanar: stop rule; best 6 insns only with unattested macro; FP paren-count mod-4 map: January yzx/ikj lies in
    neither measured cycle. No new evidence found.
- DECISION connected_geometry: OWNER for 2 rows (edge bug ruling, vertex macro); object completion additionally
  blocked by triangle_coplanar tie (RETIRE-level) -> not completable this round.

## flags
- hints: 15/16; only _flag_update T (parked). _flag_render_proper exact since cf097d7f (for-initializer counters);
  the held `x * height` item is moot.
- Probe: gate production -> exact 15 residual 1 (_flag_update [sha]).
- Probe: /Od readout (new to me, but NOT new to the campaign): odbuild file "widgets\flags.c" -> 0x7958d0,
  0x7959e0 (vertex accessors), 0x795ee0 (render_proper, RTC parameters/centroid); callers -> 0x797610 = /Od
  flag_update (RTC attachment_location..parent_vector; frame 0x3dc). Lane D REPORT
  (claude-lane-d-refresh-20260922/.../objects__widgets__flags/REPORT.md) already used 0x797610: S1..S26;
  best S5 (attachment_index_for_row first row-body decl) 390/390 insns, row EDX web gone, remaining one F2
  slot-merge membership difference; S10 (/Od reuse weight_sum = 1/weight_sum) fixes every slot but tail becomes
  fmul [mem] (1168). /Od tail 0x797f0c: `wsum = 1/wsum; new_position.x = new_position.x * wsum` = S11 (inert).
- DECISION flags: RETIRE (F2 merge-membership tie; Lane D reopen criterion not met). S5 = optional zero-credit fuzzy.

## render_debug (re-check)
- Probe: render_debug.c still carries the review_admit2 defects (vehicle_runtime_datum view l.200-312,
  REAL_MATH_EXTERNAL_* l.135-137, `extern boolean debug_*` l.355-360). Owner-queue packet
  research/fifty_objects_20260925/w/owner_queue/objlights_render_debug/patches 05, 06, 08:
  `git apply --check` exit 0 for each at f6d00a8c. January strings name no cache variable
  ("render debug cache overflow." etc.) -> no first-party names for D1.

## Zero-credit packets re-checked (git apply --check exit 0 at f6d00a8c)
- observer/production.patch, render_cameras/production.patch, structures/production.patch,
  cinematics/optional_fidelity.patch, player_effects/production.patch (research/fifty_objects_20260925/w/...).
- weather round-1 fin2.c re-gated: 20/20 EXACT, object_audit PASS, provider_link FAIL(1) _real_local_random.

## FINAL triage
| unit | decision | blocker |
| observer | RETIRE | x87 leaf-order tie (8 insns), round-1 stop rule |
| render_cameras | RETIRE | W-core placement tie (14 hunks) + CP M8 key; owner float-bits cast only for shape |
| cinematics | RETIRE | OR-operand DAG order tie (4 insns), all oracles inert |
| structures | OWNER | double-t inferred construct (strip test fails; /Od contradicts); yes => complete today |
| weather_particle_systems | OWNER | class-I /Od dead scalar ruling + _real_local_random provider (real_math.h, reserved) |
| player_effects | RETIRE | 3 ties exhausted; symbols.json static x8 hygiene available |
| geometry | OWNER | header cast (polygon2d), UB order (hull3d_expand), P1, decals; polygon3d header-paren tie |
| connected_geometry | OWNER | bug ruling (edge), macro ruling (vertex); coplanar tie remains |
| render_debug | OWNER | owner-queue item 5 (d) D1/D0 naming; 05/06/08 apply clean |
| flags | RETIRE | F2 slot-merge tie; Lane D /Od readout exhausted |
No tracked file edited; no claims taken; tree clean (git status empty).
