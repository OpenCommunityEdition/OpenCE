# Adversarial review: source/units/vehicles (worker verdict FUNCTIONS_EXACT_OBJECT_BLOCKED)
Reviewer started 2026-09-23. Fresh (review_vehicles did not exist). Notes appended as each check completes.

## R1 patch reproduction
- production.patch / production_linkclean.patch applied (git apply, outside repo) to HEAD source/units/vehicles.c:
  both apply cleanly and reproduce cand.c / cand_nofighter.c byte-for-byte (cmp). symbols_turret_static.patch applies
  cleanly; the only diff vs HEAD symbols.json is line 7052 `_update_turret_physics` + `"static": true` (in place).
## R2 gate (independent re-run)
- cand.c -> review cand.obj: `== exact 39 residual 0 unwritten 0`.
- cand_nofighter.c: 38/1 (fighter_new [sha]). base (production source): 37/2 (fighter [relocs 33!=32, sha], scout [sha]).
## R3 object_audit (independent)
- cand.obj: all 68 January-owned sections ok; 1 symbol DIFF `_update_turret_physics` storage 2/3 (split extern vs ours static). FAIL(1).
- cand_nofighter.obj: FAIL(2) = fighter_new DIFF + the same storage DIFF. base: FAIL(3).
- Surplus delta vs base: cand adds exactly _cross_product2d, _point_from_line2d, _point_from_line3d; cand_nofighter adds
  exactly _point_from_line2d, _point_from_line3d. Literal/D3D/string surplus lists identical to base (inherited).
## R4 surplus identity + provider link (independent; reviewer script surplus_identity_obj.py takes an explicit obj)
- cand.obj 30 candidate-only code COMDATs, only _object_get_bounding_sphere DIFFERENT (inherited: base 27/1 same).
  New: _cross_product2d == actor_combat IDENTICAL; _point_from_line2d == action_vehicle IDENTICAL; _point_from_line3d ==
  action_charge IDENTICAL.
- provider_link --baseline (new surplus only): cand.obj FAIL(1) _cross_product2d LNK2005 both orders vs actor_combat;
  pfl2d/pfl3d PASS. cand_nofighter.obj: PASS (pfl2d, pfl3d).
- Full provider_link: base FAIL(3) (_interpolate_scalar, _object_get_bounding_sphere, _triple_product3d - inherited);
  cand_nofighter FAIL(3) identical set -> option B adds no new link failure.
## R5 source diff read (cand.c vs production), semantic check of the scout rewrite
- Read the full production scout and the candidate side by side. Semantics equivalent statement-for-statement:
  hover>0 accel (scale_vector3d/subtract + k=0 == component form), steering (sign*sqrt == sqrt*sign), alpha_desire via two
  pfl2d (0 - offset - 15*angvel == production level_torque), alpha_bonus (sign(a*t)*1*|a| == |a|*sign(t*a)), control PIN
  terms, leveling pfl2d (applied += alpha_desire*K*(1-up.k) == torque_a/b = level_scale*level_torque + control),
  turning_torque pfl3d x3, flag-3 pfl3d, airborne fade (/30 vs *(1/30): bytes decide), tail hover update (delta form ==
  target clamp form). No logic change found.
## R6 /Od listing authenticity + per-site attestation (regenerated independently with od_linear.py 0x8f80b0..0x8f930b)
- My regeneration == worker's dbg_scout.asm (only annotation text / trailing nop differ). RTC names match the worker's.
- Attested per site in /Od: set_real_quaternion (0x8f65b0) call in the powered loop; steering copy [ebp-0xe8] + block const
  0.0034906587 [ebp-0xe4]; dot_product3d(&angular_velocity,&up) and (&translational_velocity,&forward) arg orders;
  applied_alpha = zero first; forward/left 2D copies are 8-byte INTEGER copies (aggregate view copy, not movss) ->
  `*(real_vector2d const *)&...` attested; dot_product2d(&object.up / &object.angular_velocity, ...) passes the 3D
  object field pointers directly x4 -> view casts attested; pfl2d(&alpha_desire,&offset,-1) / (-15) x2; block const 1.0
  (alpha_bonus_scale) [ebp-0x174]; 9 pfl3d calls (0x42e0d0) incl. turning_torque x2 + magic_torque(1-hover).
- /Od order in hover<1: cross_product3d(up,forward,&vehicle_left) FIRST, then the forward copy (candidate reverses; worker
  evidence: D-invariant S2 closes only with copy-first = non-count evidence; accepted as moderately supported).
- /Od LEVELING (0x8f8b4a..0x8f8bba): nested-block const leveling_scale [ebp-0x188], then TWO component statements
  `applied_alpha.i += leveling_scale*alpha_desire.i*(1-up.k)` (and .j). NO pfl2d call there (0x444020 is called x2 only).
## R7 STRIP TEST of the leveling point_from_line2d (the decisive check)
- rv_od_leveling.c = cand.c with ONLY the leveling pfl2d replaced by the /Od first-party spelling (two component
  statements `applied_alpha.i/j += leveling_scale*alpha_desire.i/j*(1.0f-vehicle->object.up.k)`).
- gate: 38 exact / 1 residual (scout [sha]); real_regions: 775/775 insns, 1 REAL region = a 2-insn x87 leaf swap at
  +0x628 (S3 dot_product3d i-term: Jan `fld [ebx]; fmul [esi+0x18]`, ours reversed). Everything else identical.
- The worker's "Probe 9 EVIDENCE" (January +0x53a..+0x557 `fld 1; fsub up.k; fmul K; fst [ebp-4]; fmul a.i; faddp;
  fld [ebp-4]; fmul a.j` = homed pfl2d t-parameter) is NOT discriminating: rv_od_leveling (and the worker's own i2.obj)
  produce those exact bytes at +0x53a..+0x557 with the component spelling (dis_range, identical listing).
- => the pfl2d at the leveling site is supported ONLY by its IL-temp count effect on a distant M8 leaf-order tie
  (S3, 0xd0 bytes later), and it CONTRADICTS the /Od readout at its own site (/Od calls pfl2d exactly x2, both for
  alpha_desire). This is the declaration/name-count ORACLE used as a landing: the plain attested spelling does not match,
  the respelling matches only through a count shift. Per the owner gates (count oracle = locate genuine constructs only,
  never land a count-shifter; STRIP TEST) this deviation is NOT admissible without owner ruling.
## R8 strip test of representation casts + /W3
- cand_strip.c (worker's; all 28 scout pointer casts + the 2 fighter cross_product2d casts removed) re-gated by me:
  39/39 EXACT; scout and fighter_new sections section_infos_equal to cand.obj -> casts byte-inert (confirmed).
- /W3 (reviewer warn.py, unit flags + /Zs /W3): warning multiset (code+message, line-number-free) identical for base,
  cand.c and cand_nofighter.c (27 each; pre-existing C4133 pfl3d fighter line, C4013 matrix3x3_*, C4244s).
## R9 /Od attestation of hunks 2 and 3 (regenerated)
- /Od 0x8fd970 (pathfinding): call 0x42e0d0 (pfl3d) at 0x8fda42 with (&origin, global_up3d, 0.4, &origin) and at 0x8fdad2
  with (&origin, &vector, result.t, position) -> hunk 2 attested (real calls). (Pre-existing, not in diff: /Od builds
  `vector` with scale_vector3d(global_down3d, 2.0, &vector); production keeps add_vectors3d(down,down) - exact, untouched.)
- /Od 0x8f6a20 (fighter): 0x8f6cbf..0x8f6ccd pushes &object.translational_velocity (+0x24) and &desired_rotation
  (forward at +0) into 0x455250 = cross_product2d body (a.i*b.j - a.j*b.i) -> hunk 3 call + 3D->2D view casts attested.
  (/Od then does *unknown308 / unknown2f8; candidate keeps production's /unknown2f8*unknown308 which is January-exact.)
## R10 HCEX + remaining /Od shape checks
- HCEX.pdb update_alien_scout_physics: params vehicle_index, steering, powered_mass_points, mass_points (rename attested);
  locals magic_force, magic_torque, velocity, vehicle_matrix, desired_acceleration, applied_alpha, vehicle_left(_horizontal),
  vehicle_forward_horizontal, alpha_desire, turning_torque, vehicle_horizontal_forward (names attested). Static fn.
- /Od hover>0 block, powered loop (ptr local + set_real_quaternion call), water-depth const, tail block (short counters,
  own short index [-0x1f8], def ptr, mass_point ptr before powered_count++, target, delta) all match the candidate.
- Minor, non-blocking: /Od initialises the 0.1 const [-0x204] AFTER `target` is computed => the later source declares
  {maximum_hover_change; delta} in a nested block after the PIN; candidate declares both at the tail-block top (worker's
  t2c shows the nested form is byte-inert). The "statement-for-statement /Od transcription" claim is slightly overstated.
- cachebeta publics: no public at 0x1A4F50 (_update_turret_physics; neighbours _vehicle_hover 0x1A4F00 and
  _vehicle_is_flipped 0x1A4F70 ARE public); only vehicles.obj defines/references it in build/split -> (C) evidence OK.
## R11 PENDING OWNER DECISION found: aggregate 3D->2D view copies (structural owner packet 20260923 section 5)
- docs/object_matching_logs/claude_structural_owner_packet_20260923.md section 5 asks the owner "does a /Od-attested
  aggregate view copy qualify when the field-copy spelling does not match?" and lists `_update_alien_scout_physics`.
  No ruling found in docs/ or memory. The brief (section 2) requires a BYTE-INERT strip test for any new 3D-to-2D view cast.
- cand.c has two aggregate 3D->2D view copies the worker's cast-strip did NOT (cannot) strip:
  `vehicle_forward_horizontal = *(real_vector2d const *)&vehicle->object.forward;` and
  `vehicle_left_horizontal = *(real_vector2d const *)&vehicle_left;` (/Od-attested as 8-byte integer copies).
  Worker's p2 (field-copy forward) = 8 blocks -> load-bearing. Measuring the field-copy forms on cand.c next.
- MEASURED on cand.c (field-copy replacements, everything else unchanged):
  rv_fc_fwd (forward field copy, still before the cross): scout residual, 11 REAL regions.
  rv_fc_left (left field copy): 23 REAL regions. rv_fc_both: 29 REAL regions.
  => BOTH aggregate 3D->2D view copies are LOAD-BEARING (not byte-inert). They are new vs production (production uses
  field copies). The worker's disclosure ("28 byte-inert representation casts ... stripping them all leaves the section
  identical") omits these two load-bearing casts. Admission of this exact shape is the OPEN owner question (section 5).
## R12 secondary strip tests (inherited idiom, for owner awareness)
- rv_nocast_sqrt (`square_root(fabs(steering_input)*K)` without the explicit (real)): scout 4 REAL regions.
- rv_nocast_fabs (`alpha_bonus_scale*fabs(alpha_desire.i/j)` without (real)): 3 REAL regions.
  Both casts are load-bearing. They are the same idiom production's scout already uses (production is non-exact), and
  (real)fabs has real float-rounding semantics; the sqrt one is the warning-free (C4244) form of an implicit conversion.
  Not a blocker on its own; the worker disclosed the sqrt one. Listed so the owner sees every load-bearing decoration.
- build/report.json source/units/vehicles data 1744/1744 (100%) confirmed; candidate changes no data section.

## R13 fallback measured (reviewer, not requested): hunks 1+2 only = scratch/w/review_vehicles/fallback_hunks12.patch
- = first two hunks of production.patch (define/undef removal + pathfinding pfl3d calls). git apply verified.
- gate 37/2 (fighter_new [relocs 33!=32, sha] -> [sha]; scout [sha] unchanged; no row lost). New surplus only
  _point_from_line3d, IDENTICAL to action_charge, provider_link PASS; /W3 multiset == base. Zero exact credit; it is a
  rule-6 cleanup (removes the hand-expanded pfl3d in vehicle_find_pathfinding_surface_index, /Od-attested calls).

## VERDICT: approve = false
Verified OK: patches reproduce candidates; gate 39/39 (A) and 38/1 (B); object_audit (all January sections ok, 1 storage
DIFF); surplus identity (new COMDATs identical); provider_link (A FAIL _cross_product2d, B PASS); /W3 set unchanged;
28 pointer representation casts byte-inert; /Od attests hunk 2, hunk 3 and almost all of hunk 4; HCEX names; (C) evidence.
Blocking:
1. Two NEW aggregate 3D->2D view copies in the scout are LOAD-BEARING (field copies: 11/23/29 REAL regions). The brief
   requires a byte-inert strip for new 3D->2D view casts, and this exact shape for this exact function is the OPEN owner
   question in claude_structural_owner_packet_20260923.md section 5. Not disclosed as load-bearing by the worker.
2. The leveling point_from_line2d contradicts the /Od at its own site (/Od: two component statements; pfl2d called x2
   only). Its claimed January-byte evidence (homed t at +0x53a..+0x557) is refuted: the /Od component spelling produces
   identical bytes there (rv_od_leveling.obj, and the worker's own i2.obj). What it actually fixes is a 2-insn M8 leaf
   swap at +0x628 (S3) through an IL-temp count shift; the worker's own bisect shows ANY +1 temp in the window does the
   same. That is the count oracle used as a landing -> owner ruling required.
3. (A) also blocked on the actor_combat NODUP cross_product2d provider (owner packet section 1 decision) - disclosed.
Landable without a ruling: (C) symbols_turret_static.patch + csplit-only regen of vehicles (evidence verified);
optionally fallback_hunks12.patch (zero credit, rule-6 cleanup).
