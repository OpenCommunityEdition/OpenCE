# vehicles worker LEDGER (lane claude/fifty-objects-20260925)
Started 2026-09-23T18:43:55-07:00. Fresh start (slug dir did not exist).
Targets: _update_alien_fighter_physics_new (1088), _update_alien_scout_physics (2464; frame Jan 0x94 vs ours 0xa0)


## Baseline (production build/base, worktree HEAD)
- gate --all: 37 exact / 2 residual / 0 unwritten.
  - _update_alien_fighter_physics_new 1088 [relocs 33!=32, sha]: ours has an extra out-of-line _point_from_line3d call
    (TU defines REAL_MATH_EXTERNAL_POINT_FROM_LINE3D since 5d3444f7 batch8, which made point_from_line3d an external call).
    January has NO _point_from_line3d reloc anywhere in vehicles.obj -> January inlined it at this site.
  - _update_alien_scout_physics 2464/91 [sha]; frame Jan 0x94 vs ours 0xa0.
- object_audit(base): FAIL(3) = the 2 DIFF fns + symbol _update_turret_physics storage 2(Jan extern) vs 3(ours static).
  Surplus: many header-inline COMDATs + 4 invented vehicle_*_target/test helper owners (inherited debt).
- Read: house rules, vehicles_obj.md checkpoint (fighter_new = code_001a5e50, scout = code_001a7e60), astra research7 (scout
  /Od at 0x8f80b0 in halo_cache_symbols.exe; point_from_line2d authenticated; two shapes measured 1342/1313 diffs, frame stayed 0xa0).
  Recorded negatives NOT to repeat: 778 fighter slot/scope/role probes on the hand-expanded form; scout pre-negated rate_forward,
  pd_b-before-pd_a, direct vector inputs, direct-PD (double counts), declaration-order sweeps.

## Prior-lane evidence located (read-only)
- Lane B: /c/halo-worktrees/claude-lane-b-ai-physics-units-20260920/scratch/lane_b/w/vehicles/ (REPORT.md; try_7 fighter EXACT with
  header-inline point_from_line3d + cross_product2d view casts; /Od fighter = 0x8f6a20 pushes &translational_velocity and
  &desired_rotation straight into cross_product2d at 0x8f6cbf-0x8f6ccd).
- Structural lane: /c/halo-worktrees/claude-structural-20260923/scratch/lane/w/units__vehicles/ (REPORT.md; scout M7 frame 0x94 with
  6 casts; M13/M15 775/775 insns, 12 M8 x87 operand-order regions left; provider FAIL then on both helpers).
- This tree (base e9e62b78): action_charge point_from_line3d hand copy is GONE (5e81dabf); actor_combat.c STILL hand-writes
  `real cross_product2d(` behind `#define cross_product2d cross_product2d_inline` (NODUP provider).

## Probe 1 (fighter) - p_nodefine.c = base minus the REAL_MATH_EXTERNAL define/undef only
Hypothesis: January's vehicles TU saw the header __inline (vehicle_update calls _point_from_line3d out of line, fighter inlines it).
Result: fighter [relocs 33!=32, sha] -> [sha] (32/32); vehicle_update still EXACT; 37 exact / 2 residual.
provider_link --baseline=base: _point_from_line3d provider action_charge PASS -> SELECTED-PROVIDER LINK: PASS.

## Probe 2 (fighter) - f1_held.c = p_nodefine + Lane B yaw = cross_product2d((real_vector2d const *)&desired_rotation.forward,
## (real_vector2d const *)&vehicle->object.translational_velocity)/...
Result: fighter EXACT 1088; 38 exact / 1 residual (scout). provider_link: _point_from_line3d PASS, _cross_product2d FAIL
(LNK2005 both orders vs actor_combat.obj - NODUP hand copy). => blocked on actor_combat provider, not on vehicles source.

## Finding: _cross_product2d provenance (January + /Od)
- refcensus.py (my script): January split has ZERO relocations to _cross_product2d anywhere (all-inlined class); January
  actor_combat.obj DEFINES it (selected copy). _point_from_line3d out-of-line refs incl. vehicles(_vehicle_update +0x268).
- /Od build: cross_product2d thunk 0x408512 -> body 0x455250 (inside actor_combat's /Od address range 0x44xxxx-0x45xxxx).
  /Od actor_combat caller: fn 0x44ebb0 (call at 0x44ed45: cross_product2d(actor+0x140, &vec) > 0 -> sign +-1).
  Our actor_combat.c has NO use of cross_product2d (only the renamed-header hand copy) -> a provider repair needs that
  genuine use restored (object-closure law). Parked as a side item; scout first.

## Scout: state carried from structural lane (m15.c copied here, re-gated on this tree)
- m15 (structural M15: header-inline pfl3d at 9 /Od sites, 6 view casts, /Od block consts) = 775/775 insns, frame 0x94,
  relocation multiset equal; remaining = 4 inline-helper sites with mirrored x87 operand order:
  S1 +0x227..+0x238 dot_product3d(up,angvel) [ours evaluates j,k,i terms; Jan k,j,i + swapped leaves]
  S2 +0x32f..+0x375 hover<1 cross_product3d(up,forward,&left) (3 products flipped)
  S3 +0x617..+0x623 flag-3 dot_product3d(forward,&translational_velocity)
  S4 +0x661..+0x68f flag-3 cross_product3d(up,forward,&left)
- /Od (0x8f80b0, dbg_scout.asm) facts vs m15 (my full read of the listing):
  * dot_product3d ARGUMENT ORDER: S1 is dot_product3d(&angular_velocity(+0x48), &up(+0x3c)); S3 is
    dot_product3d(&translational_velocity(+0x24), &forward(+0x30)) - both REVERSED in m15.
  * no named `sign` / `sign_a` / `sign_b` locals (ternary results live in /Od compiler temps -0x21x..-0x25x);
    m15 has three named long locals there.
  * steering block has a named real copy of `steering` at [-0xe8] used for every later steering read.
  * alpha_desire = zero; point_from_line2d(&alpha_desire,&offset_from_vertical,-1,&alpha_desire);
    point_from_line2d(&alpha_desire,&angular_velocity,-15,&alpha_desire) (8 inline param temps = 0 mod 8 -> explains
    structural M5 'inert').
  * alpha_bonus.x = sign*K1*fabs(alpha_desire.x) with a block const 1.0 at [-0x174].
  * tail block: its own short loop index [-0x1f8] (not the fn-level one), def ptr, and a mass_point ptr assigned before
    powered_count++; named `delta = target - hover` [-0x208]; hover +=/-= 0.1 or = target.
  * January out-of-line calls (relocs): only normalize2d x2, normalize3d, limit3d, matrix4x3 x3, tag/object getters,
    water_depth, physics_update, tag_block_get_element_with_size, create_ghost_effect -> all other helpers inlined.

## Probe 3 (scout) - D-sweep instrument on v0 (= f1_held + m15 scout), D dead `long` at fn top, D=0..8 (LAB ONLY)
- with and without swapping both dot_product3d argument orders to the /Od order: byte-identical per D (arg order INERT
  here, as M8 says for bare leaves). Best D=6: 8 blocks (S1 j/k tree-order pair only, S2 3, S3 2, S4 2); no D closes any of
  S1(j/k), S3. => residual is NOT a pure function-wide count; S1's j/k term ORDER (Jan k-term first) never moves with D.
- Byte fact: at S1 January addresses `up` through the vehicle base (esi+0x3c..) and angular_velocity through edi; j/k products
  load the edi operand first, i-term loads esi first.

## Object-level blocker noticed (inherited debt)
- base vehicles.obj defines 4 invented private helper owners January lacks: _vehicle_cross_product3d_target,
  _vehicle_dot_product3d_target, _vehicle_triple_product3d_target (used by exact _update_human_boat_physics) and
  _vehicle_dot_product3d_test (used by exact _vehicle_preprocess_node_orientations, line 1458); scout uses
  vehicle_cross_product3d_target at line 2222 (m15 already replaces it). Static law: emitted whenever used.
  => even with both residuals exact, whole-object admission needs boat + preprocess rewritten onto genuine helpers.

## Probe 4 (scout) - t1.c = f1_held + scout transcribed statement-for-statement from /Od 0x8f80b0 (bodies/t1.txt)
Hypothesis: m15's residual is the IL-local/inline-temp census (M8), so the /Od local set + call list should move it.
Changes vs m15: no object_forward/up/angvel pointer locals (/Od addresses &vehicle->object.X directly); dot_product3d
args in /Od order; RTC names (vehicle_matrix, velocity, desired_velocity, desired_acceleration, vehicle_left,
vehicle_forward/left_horizontal, applied_alpha, offset_from_vertical, angular_velocity, alpha_desire, alpha_bonus,
turning_torque, vehicle_horizontal_forward); steering copy local `turn` ([-0xe8]); no named sign locals; two
point_from_line2d for alpha_desire; block const 1.0 (alpha_bonus_scale), 30.0 fade divisor const; loop ptr local; tail
block own short index + mass_point ptr + `delta`; hover +=/-=/= target; param renamed powered_mass_points (HCEX).
Result: scout still [sha] 775/775 but REAL blocks 12 -> 4 (S1 fixed, S4 fixed; S2 3 blocks, S3 1 block). Fighter EXACT.

## Probe 5 (scout) - declaration ORDER per /Od frame (t2a turning_torque in nested block after if/else; t2b hover>0
## decl order max_accel,desired_velocity,desired_acceleration,maximum_speed; t2c tail target-before-counts + nested
## {const 0.1; delta}); t2 = all three. Result: all identical to t1 (4 blocks S2:3 S3:1) -> declaration order INERT.
## Probe 6 (scout) - D-sweep on t1 (lab): S3 closes at D=1..3; S2's i-component 2nd product (a.k*b.j, no disp-0 leaf)
## NEVER moves with D; S2 k2 fixed only at D=6. S2 therefore has a non-count cause.

## Probe 7 (scout) - S2 context probes on t1 (evidence: S2 i2 product is D-invariant = not a count key)
- p1: `vehicle_forward_horizontal = *(real_vector2d const *)&vehicle->object.forward;` BEFORE cross_product3d(...,&vehicle_left)
  (then left copy): S2 CLOSES fully -> REAL blocks 4 -> 1 (only S3 i-term left). Bytes otherwise identical.
- p2 (component-wise forward2d copy): 8 blocks (worse). p3 (applied_alpha zero assigned after the copies): 9 (worse).
=> January statement order = forward 2D copy, then cross product, then left 2D copy (the later /Od build has the cross first).

## Probe 8 (scout) - dot_product3d arg-order and count localisation on p1
- q1 (S3 args swapped to forward,velocity): S3 2 blocks (j/k flip) -> /Od order is right; q2 (S1 swapped): S1 breaks
  -> /Od order right. (Arg order is NOT inert once the pointer locals are gone.)
- dead-local instrument at fn top / before `vehicle` / inside flag-3 block: identical results -> every user local is numbered
  before all inline temps (uniform shift); S2 needs shift 0, S3 needs 1..5 => the gap is INLINE TEMPS between S2 and S3.
- LAB INSTRUMENT (not landable): `square_root(vehicle->vehicle.hover);` as a dead statement at the END of the hover<1
  block (+1 inline temp between S2 and S3): **scout EXACT, gate 39/39**. +2 temps (dead dot_product2d): S3 2 blocks;
  +4: S3 1 block. => January has exactly ONE more inline temp (mod 8) between the S2 cross and the S3 dot than p1.
  Next: bisect the instrument position to find the window, then find the genuine 1-temp construct.
- Position bisect of the +1 instrument on p1 (lab): closes EXACT anywhere from right after the S2 cross_product3d (w1,
  before normalize2d) through w2/w6/w3/w4 to the end of hover<1; after S3 (w5) it does nothing. Intermediate dot2d/pfl
  sites are insensitive to +1. => one missing temp-creating construct somewhere in hover<1 after the S2 cross.
- x1 (t1 + instrument BEFORE the S2 cross): S2 9 blocks -> p1's S2 fix is statement ORDER (forward copy first), not a count.
- c1 (left2d copied from cross_product3d's return value): 7 blocks incl. S1/S4 - rejected.
- n1/n2 (normalize2d via extern alias, lab): 106 blocks - known-callee ABI effect, instrument invalid.
- i1 (forward_h declaration-initialised but declared before applied_alpha): 12 blocks (copy precedes applied_alpha zero).
- i2 (declare applied_alpha = zero FIRST, then vehicle_left, then forward_h = *(real_vector2d const *)&forward (init),
  left_h): == p1 (1 block, S3) -> preferred spelling (rule 17); January order = applied_alpha zero, forward copy, cross.
- p1 D-sweep re-read: S3 is correct for EVERY uniform D != 0 (a tie at exactly our count); S2 needs D == 0.
- f1/f2/f3 (drop (real) casts on the alpha_bonus fabs): S4 breaks, S3 unchanged - rejected. k_a..k_d (hand-writing
  pfl2d/dot2d/pfl3d): worse - rejected.

## Probe 9 (scout) - EVIDENCE: January homes the leveling factor like an inline helper t-parameter
January +0x53a..+0x557: `fld 1.0; fsub up.k; fmul 0.0038785094; fst [ebp-4]; fmul alpha_desire.i; faddp; fld [ebp-4];
fmul alpha_desire.j` = t computed once and HOMED to [ebp-4] (fst) = the inlined point_from_line2d/point_from_line3d
t-parameter signature (same as the (1-hover) pfl3d t-param at +0x5da..+0x5e9). Hypothesis: January spelled the leveling as
point_from_line2d(&applied_alpha, &alpha_desire, leveling_scale*(1-up.k), &applied_alpha) (the later /Od build has two
component statements there).
- l1.c = i2 + that point_from_line2d: **gate `== exact 39  residual 0`** - _update_alien_scout_physics EXACT 2464 (and
  fighter_new EXACT). l2 (named `leveling` local instead) -> S2 6 blocks (user local = uniform shift) - rejected.
=> l1.c is the all-39-exact candidate. Next: house-rule cleanup + section-4 object audit.

## Section-4 audit of l1.obj (all 39 functions strict EXACT)
- object_audit: every January-owned section ok; symbols: 1 DIFF `_update_turret_physics` storage split 2 vs ours 3;
  pdb_storage: PDB-public False (cachebeta publics lack it; _set_real_quaternion/_vehicle_hover ARE public) => split artifact;
  fix = config/symbols.json line `_update_turret_physics` add `"static": true` (in place) + csplit regen (integrator).
- provider_link --baseline=base: NEW surplus _point_from_line2d (action_vehicle) PASS, _point_from_line3d (action_charge)
  PASS, _cross_product2d (actor_combat) FAIL (NODUP hand copy in actor_combat.c).
- provider_link full: FAIL(4) = _cross_product2d (new) + INHERITED _interpolate_scalar (player_control),
  _object_get_bounding_sphere (action_vehicle), _triple_product3d (decals).
- surplus identity (my copy surplus_identity_cand.py): 30 COMDATs, all IDENTICAL to January's selected copies except
  INHERITED _object_get_bounding_sphere DIFFERENT (also DIFFERENT on build/base).
- Invented private owners still emitted (inherited): _vehicle_cross_product3d_target, _vehicle_dot_product3d_target,
  _vehicle_triple_product3d_target (boat), _vehicle_dot_product3d_test (preprocess_node_orientations).
- /W3 warning set: l1 == base (only line shifts from the removed define/undef); the fighter's pre-existing C4133 (pfl3d
  vector/point) and C4013 (matrix3x3_multiply / matrix3x3_rotation_to_quaternion) remain - pre-existing, not new.
- STRIP TEST (strip1.c): removing all 28 representation casts from the new scout ((real_point3d *) x18, (real_point2d *) x6,
  dot_product2d((real_vector2d const *) x4) -> scout section byte+reloc IDENTICAL to l1, 39/39 exact. Casts byte-inert.
- /Od float constants decoded (fl.py) = every literal/const in the candidate (0.5,-0.2,0.8,0.785398185,0.05,0.98,1,
  0.0034906587,0.0069813174,1e-4,2,0.0015514038,-1,-15,0.3,2.5,0.0038785094,0.005817764,0.004,0.002,0.001,30,0.4,0.1).

## Final scout body (bodies/final.txt): l1 + `turn` renamed `steering_input`; gate 39/39 EXACT.
- final_nocast (square_root(fabs(x)*K) without the explicit (real) conversion): S2/S4 break + C4244 warning at that line;
  final_b ((real)fabs(x)*K): breaks. The explicit (real)(...) is the warning-free spelling of an unavoidable double->real
  argument conversion, already present in production's scout; kept and disclosed.
## Optional rule-6 cleanup riding the define removal (vehicle_find_pathfinding_surface_index, exact before and after)
- /Od 0x8fd970 = pathfinding: point_from_line3d(&origin, global_up3d, 0.4f, &origin) and
  point_from_line3d(&origin, &vector, result.t, position) are REAL calls (0x8fda42, 0x8fdad2).
- pf1 (restore the second call, drop the hand expansion + stale comment + `real t`): 39/39 EXACT.
- pf2 (pf1 + the first call replacing `origin.x += global_up3d->i*0.4f` x3): 39/39 EXACT. Adopted (no casts needed).

## Candidate banked: scratch/w/vehicles/cand.c (= pf2.c) -> production.patch (git-apply verified to reproduce cand.c)
- gate: == exact 39 residual 0 (cand_gate.txt). fake_match_scan: 0 leads (base also 0). /W3: same warning set as base.
- symbols_turret_static.patch: `_update_turret_physics` + "static": true (cachebeta PDB: not public; ours already static).

## Side attempt: boat invented-helper debt (boat is exact but uses 3 invented helper owners)
- /Od boat = 0x8f9a10 (od_boat.asm; RTC: magic_force, magic_torque, desired_up, left, forward_cross_velocity).
  /Od shape: const 0.4; speed = fabs(magnitude3d(&velocity)/0.4); rudder = 0.5*turn; if(speed>1) speed=1; rudder*=1-speed;
  3x set_real_quaternion (rotation (0,0,sin,cos), (0,0,0,1) x2); set_real_vector3d(&magic_force,0,0,0);
  point_from_line3d(global_up3d,&forward,-forward.k,&desired_up); if normalize3d != 0 { consts 2pi, 0.0139626; 
  cross(up,fwd,&left); spin = dot(cross(fwd,vel,&forward_cross_velocity), up)*2pi; rotate_vector_about_axis; angle =
  angle_between_vectors3d; if dot(desired_up,left)>0 angle=-angle; dot = dot(angvel,fwd); desired = sign*sqrt(|a|*2K);
  error = PIN(...); torque = xx*error; scale_vector3d(&fwd, torque, &magic_torque) } else set_real_vector3d(&magic_torque,0..)
- boat1.c (/Od transcription, genuine helpers only, no invented owners): 784/784 size, 25 relocs same identities, but
  267 vs 266 insns, 17 REAL blocks (rotation zero-store interleave at +0xae, first cross (up,fwd,&left) operand orders
  +0x14f..+0x184, triple/dot term order +0x18b..+0x1c3).
- boat2.c (= boat1 with header triple_product3d(&forward,&velocity,global_up3d)): same 17 blocks.
- D-sweep (b2d1..7, lab): A/B regions D-INVARIANT -> not a count residual; needs a tree-shape/statement-order cause
  (as scout S2 did). STOPPED here (boat already exact in production; this only targets invented-helper debt, and the
  object stays blocked by out-of-unit NODUP providers regardless). Evidence kept for a future helper-debt lane.
- preprocess_node_orientations: replacing vehicle_dot_product3d_test with header dot_product3d (either arg order):
  325 vs 335 insns, 47 blocks -> the invented static changes codegen substantially; not pursued (helper-debt lane).

## FINAL candidate: scratch/w/vehicles/cand.c (= cand2: final scout + turning_torque declared-initialised in a nested
## block after the if/else, the /Od frame order; byte-neutral) -> production.patch (git apply reproduces cand.c)
- gate: == exact 39 residual 0 (cand_gate.txt; base_gate.txt = 37/2). fake scan 0 leads. /W3 set == base.
- cast strip (cand_strip.c: all scout representation casts + the fighter's 2 cross_product2d casts removed): both
  functions byte+reloc identical -> casts are byte-inert.
- object_audit: January sections all ok; 1 symbol DIFF (_update_turret_physics storage) -> symbols_turret_static.patch.
- provider_link FAIL(4): _cross_product2d (NEW; actor_combat NODUP hand copy), inherited _interpolate_scalar
  (player_control), _object_get_bounding_sphere (action_vehicle; also bytes DIFFERENT vs January copy), _triple_product3d
  (decals). _point_from_line2d / _point_from_line3d new surplus PASS and IDENTICAL.
- invented private owners (inherited, boat/preprocess): _vehicle_{cross,dot,triple}_product3d_target, _vehicle_dot_product3d_test.
VERDICT: FUNCTIONS_EXACT_OBJECT_BLOCKED.
- Split landing option: cand_nofighter.c (= cand.c with the fighter yaw left in production's hand-written 2D cross form):
  scout EXACT, fighter [relocs 33!=32, sha] -> [sha] (improved, still residual), 38 exact / 1 residual, provider link of
  new surplus PASS (_point_from_line2d, _point_from_line3d). -> production_linkclean.patch (git apply verified).

## Methodology notes for future lanes (new, measured here)
1. /Od transcription WITHOUT invented pointer locals (address &vehicle->object.X directly) + /Od helper argument orders
   fixed two of four x87 sites at once; dot_product3d arg order is NOT inert once pointer locals are gone.
2. D-invariant ('tree shape') operand orders on non-displacement-0 products moved with STATEMENT ORDER of an adjacent
   aggregate copy that reads the same object (forward 2D copy before the cross).
3. A lab instrument (dead 1-param inline call) localised the missing IL temp window; the genuine construct was then read
   off January's bytes: `fst [ebp-4]` homing of a once-computed factor = an inlined helper's real t-parameter
   (point_from_line2d), even where the later /Od build spells two component statements.
DONE.
