# Lane B worker report: source/game/aim_assist

Worker owns source/game/aim_assist.c only. All work is in scratch/lane_b/w/aim_assist/.

## Baseline (verbatim, real source)

    EXACT       256  _aim_assist
    EXACT       240  _aim_assist_clear_line_of_sight
    EXACT       416  _aim_assist_compute_target
    EXACT       272  _autoaim_compute_target
    EXACT       160  _compare_targets
    EXACT        80  _compute_attenuation
    EXACT        48  _compute_composite_attenuation
    EXACT       304  _find_aim_assist_targets
    EXACT       400  _find_aim_assist_targets_recursive
    EXACT        96  _limit3d
    residual    368  _local_player_aim_assist  [sha]
    EXACT       416  _object_compute_autoaim_target
    UNWRITTEN   832  _player_aim_projectile
    EXACT        16  _reciprocal_square_root
    EXACT        32  _set_real_euler_angles2d
    EXACT       240  _unit_get_aim_assist_parameters
    == exact 14  residual 1  unwritten 1  (of 16 listed)

Prior ledgers read: astra research8 (both), opus5 150k w1 + w3, opus5 fresh-graphs. No park entry exists for
either function in config/parked.json.

## Attempt log

### try_0 - _player_aim_projectile: archived 150K body replayed (control)
- Source: opus5-150k-house-clean-20260914/scratch/workers/aim_assist/player_aim_projectile_candidate.c body inserted
  between aim_assist and local_player_aim_assist, plus a TU-local stand-in prototype for pin_normal_to_cone3d.
- Gate: `residual    832  _player_aim_projectile  [sha]` (278==278 insns; the one known difference reproduced:
  January `push ecx` for the second normalize3d at +0x289, ours at +0x26e). Files: try_0_archived.c, pap_try0_full.txt.

### census (evidence, no compile) - push_census.py / push_census.txt
- Over ALL January split objects: 7,885 `lea R,[ebp-X]` / `push R` pairs are scheduled EARLY; only 5 are LATE
  (lea early, x87 stores, push immediately before the call). The other 4 late cases are all forced by OTHER
  argument pushes that must precede (register / float-arg dependencies). player_aim_projectile +0x269/+0x289 is the
  ONLY single-argument late push on the board => the deferral is a property of THIS block's IL, not a general tie.
  The straight-line block starts at +0x19e, so the camera-point update at +0x1f9..+0x237 is inside it.

### try_1 - _player_aim_projectile: Astra typed point setter (the one evidence-backed, never-compiled shape)
- Evidence: Astra research8 - the later Win32 build at 0x58a05f..0x58a09c computes camera_position.{x,y,z} +
  displacement.{i,j,k} and calls a (point *, float, float, float) setter (body stores +0/+4/+8, returns pointer);
  HCEA DIA authenticates set_real_point3d(real_point3d *p, real x, real y, real z) inline in math/real_math.h
  (already in our header at line 1046). Replaces the archived add_vectors3d(&camera_direction,
  (real_vector3d const *)&camera_position, (real_vector3d *)&camera_position) - a point/vector
  representation-punning cast, inadmissible by the brief anyway.
- Change: one statement -> set_real_point3d(&camera_position, camera_position.x + camera_direction.i, ...y/j, ...z/k);
- Gate before: `residual    832  _player_aim_projectile  [sha]`
- Gate after:  `EXACT       832  _player_aim_projectile`
- Full board: `== exact 15  residual 1  unwritten 0  (of 16 listed)`, guard _point_from_line3d passed,
  unitcheck: gains=1 regressions=0.
- Verdict: ACCEPTED -> candidate.c. The deferred push was NOT a scheduling tie: it was the wrong camera-point statement.

### try_2 - prototype-position robustness (diagnostic; candidate unchanged)
- TU-local stand-in prototype moved to the alphabetical include slot (before objects/objects.h): 15 exact.
- pp.py probe (CL /E of the candidate, prototype text moved INSIDE the preprocessed real_math.h region, right after
  fast_normals_interpolate = the position the Astra root probe used, TU-local copy deleted; compiled the .i):
  control pp_control.c 15 exact; pp_hdr_A.c 15 exact. So aim_assist.obj is 15/16 with the prototype in its genuine
  owner header too. No header was edited.

### try_3 - _player_aim_projectile: HCEA/RTC-attested names + natural disjoint scopes
- Evidence: HCEA DIA locals (target_direction, collision_direction, camera_position, camera_direction, target,
  camera_displacement, collision, camera_vector, desired_direction); HCEA shares target/collision and
  camera_displacement/desired_direction homes; January shares target/collision (-0xa4 inside -0xbc) and
  camera_vector/desired_direction (-0x54). => target block, trace block, blend block are sibling scopes; desired_direction
  is its own variable (the archived body reused one `vector` for both).
- Gate: `EXACT       832  _player_aim_projectile`; board `== exact 15  residual 1  unwritten 0  (of 16 listed)`;
  unitcheck gains=1 regressions=0. ACCEPTED -> candidate.c (try_1.c kept as the minimal-diff variant).

### lp_h1 - _local_player_aim_assist: setter-call for the angular velocity (REJECTED)
- Hypothesis from the HCEA line table (lines 217/218 interleave): set_real_euler_angles2d(target_angular_velocity, pitch, yaw).
- Gate: `residual    368  _local_player_aim_assist  [size 352!=368, sha]` - worse; registers unchanged. Rejected (lp_h1.json).

### allocator oracle on the baseline body (diagnostic, orc.py -> Lane A alloc-oracle with a private work dir)
- Our build: tav web pri 43 -> esi; local_player_index web pri 37 (terms 2x7+4x4+2x4-1) -> edi (allowed={edi});
  target_angular_position web pri 31 (4x7+2x6-[1,4,4]) UNCOLOURED; player pri 36, aiming_unit_index pri 24 uncoloured.
- January needs target_angular_position popped BEFORE local_player_index: then lpi's whole web has an empty allowed set
  (esi, edi taken; ebx excluded by the private-ABI `lea ebx` for unit_get_aim_assist_parameters), is split, and its early
  piece takes ebx while the last use reloads [ebp+8] - exactly January's bytes. Needed swing >= 7 priority points.
- Block weights: B0 n=7, B1 n=4 (lpi, player, aiming_unit_index, zoom_level), B2 n=4, B3 n=6. A named zoom_level local
  costs lpi +4/tap -1; a function-scope result variable gives tap +6 / lpi +2. Those are SOURCE facts, not ties.

### try_4 - _local_player_aim_assist: structure recovered from the later Win32 /Od build (win32_589420.txt)
- NEW first-party evidence (read-only capstone of halo_cache_symbols.exe, function 0x589420, located as a caller of
  unit_get_aim_assist_parameters 0x58a550; RTC descriptor 0x589734 names parameters, target, velocity,
  target_velocity, magnetism_velocity):
  1. `mov [ebp-8],0xffffffff` BEFORE the director_get_perspective call and `mov eax,[ebp-8]` at the single exit;
     `[ebp-8] = target.object_index` is the LAST statement of the innermost block => result local initialised to NONE,
     ONE return statement (single-exit law).
  2. `lea edx,[ebp-0x30]; push edx` then `call player_control_get_zoom_level`, `movzx ecx,ax; push ecx` => the zoom level
     is a NESTED call argument, not a named local.
  3. subtract_vectors3d(&target_velocity, &velocity, &magnetism_velocity) is a real call into a named 12-byte local.
  4. two pointer locals [ebp-0xb8]=&target.vector and [ebp-0xbc]=&magnetism_velocity, then two real locals
     (sum of squares, square_root) - an inner block of four locals feeding the yaw/pitch expressions.
- January decides the expression trees: pitch = (h*v->k - (p->i*v->i + p->j*v->j)/h*p->k) / (p->k*p->k + h2).
- Gate before: `residual    368  _local_player_aim_assist  [sha]` (frame 0x6c vs 0x68)
- Gate after:  `EXACT       368  _local_player_aim_assist`; alndiff: 156==156, no normalized instruction differences.
- Board: `== exact 16  residual 0  unwritten 0  (of 16 listed)`. ACCEPTED -> candidate.c.

### strip tests - _local_player_aim_assist (lp_variants.py; each removes ONE element of try_4)
| variant | change | gate |
|---|---|---|
| lpv_A_noptr | no pointer locals (direct target.vector.* / magnetism_velocity.*) | `residual    368  _local_player_aim_assist  [sha]` |
| lpv_A1_ponly | only the &target.vector pointer (the w3 l1 lead) | `residual    368  _local_player_aim_assist  [sha]` |
| lpv_B_tworet | two return statements instead of the result local | `residual    368  _local_player_aim_assist  [sha]` |
| lpv_C_zoomlocal | named `short zoom_level` local | `residual    368  _local_player_aim_assist  [sha]` |
| lpv_E_declorder | result local declared AFTER perspective (its initialiser then runs after the call) | `residual    368  _local_player_aim_assist  [sha]` |
| lpv_D_pitch2020 | pitch written as the 2020 build associates it: p->k/h*(sum) | `EXACT       368  _local_player_aim_assist` |
Every load-bearing element is independently attested by the Win32 /Od build (it is not decoration bought for bytes);
the one free choice (pitch association) was resolved to the first-party-attested form (D). -> try_5.c.

### try_5 / pav_A / try_6 - authenticity refinements (all EXACT, all 16/16)
- try_5: D pitch form + descriptive pointer names relative_position / relative_velocity (original scalar names unrecoverable:
  RTC lists aggregates only). 16/16.
- pav_A_cam_to_unit: player_aim_projectile distance written as the 2020 build has it (RTC-named 12-byte local
  camera_to_unit; /Od calls vector_from_points3d then magnitude3d separately in the caller): EXACT. The
  distance3d(&unit->object.position, &camera_position) spelling is byte-identical (kept as try_5_distance3d.c).
- try_6_nestzoom: unit_get_zoom_level(aiming_unit_index) nested as the argument in player_aim_projectile (2020 /Od pushes
  &parameters before calling unit_get_zoom_level): EXACT. Named-local spelling is byte-identical there (try_5b_zoomlocal.c).
- FINAL candidate.c = try_6_nestzoom.c.

## Final checks (candidate.c)
- gate --all + guard (final_gate.txt): `== exact 16  residual 0  unwritten 0  (of 16 listed)`, `== emitted-symbol guard passed`.
- unitcheck: gains=2 regressions=0 other=0.
- fake_match_scan: 0 review leads.
- CL /Zs /W3 (zs_w3.txt): no C4013. Only pre-existing C4244 long->short at the two aim_assist(...) team_index arguments
  (same warning exists in the real source at its line 581; header-owned field type).
- newsyms: OWNERSHIP FAIL is reported for _set_real_point3d, _valid_real, _valid_real_normal3d, _valid_realcmp. These are
  header `__inline` COMDATs that the January LINKER folded into the first emitting object (split owners: ai_debug.obj and
  actor_combat.obj). Census: production build/base already emits _valid_real_normal3d in 33 objects, _valid_real in 44,
  _valid_realcmp in 34, _set_real_point3d in 17 - the systemic header-inline property, not a _point_from_line3d-class owner.
  January aim_assist.obj itself CALLS _valid_real_normal3d through a relocation (+0x38), so the reference is genuine.
- Header-packet emulation (pp.py: CL /E, prototype moved into the preprocessed real_math.h text after
  fast_normals_interpolate, TU-local stand-in deleted, .i compiled): 16/16. Alphabetical include slot: 15/15 at try_2.

## HEADER PACKET REQUIRED TO LAND
candidate.c carries a clearly marked TU-local STAND-IN prototype for pin_normal_to_cone3d (defined in
source/math/real_math.c:2320, declared nowhere). Landing needs ONE of:
 (a) source/math/real_math.h, after line 561 (fast_normals_interpolate), house style one parameter per line:
     boolean pin_normal_to_cone3d(real_vector3d const *normal, real_vector3d const *direction, real sine, real cosine, real_vector3d *result);
     KNOWN HAZARD (Astra research8): this exact edit flips _decals_delete_permanent_from_cluster (declaration count).
 (b) the P9 focused owner header math/real_math_cones.h (sole consumer aim_assist.c). P9 was held "until the body is
     strict exact" - it now is. Zero blast radius; token-equivalent to the stand-in measured here.
Then delete the stand-in block from aim_assist.c.
