# Lane B worker report: source/units/vehicles

## Baseline (python tools/campaign/gate.py source/units/vehicles --all) - verbatim

```
EXACT       208  _compute_acceleration
EXACT       160  _compute_airborne_ticks
EXACT       544  _create_crashing_effects
residual    656  _create_ghost_effect  [sha]
EXACT       512  _create_pelican_effect
EXACT       464  _create_slipping_effects
EXACT        32  _set_real_quaternion
EXACT       384  _slowly_stop_vehicle
EXACT       112  _update_alien_fighter_physics
residual   1088  _update_alien_fighter_physics_new  [relocs 33!=32, sha]
EXACT       768  _update_alien_fighter_physics_old
residual   2464  _update_alien_scout_physics  [sha]
EXACT       784  _update_human_boat_physics
EXACT       272  _update_human_jeep_physics
residual    928  _update_human_plane_physics  [sha]
EXACT       336  _update_human_tank_physics
EXACT       800  _update_suspension
EXACT        32  _update_turret_physics
EXACT       256  _vehicle_accelerate
EXACT        16  _vehicle_build_update
EXACT        48  _vehicle_causes_collision_damage
EXACT        16  _vehicle_delete
EXACT      1312  _vehicle_export_function_values
EXACT       304  _vehicle_find_pathfinding_surface_index
EXACT        80  _vehicle_hover
EXACT        48  _vehicle_is_flipped
EXACT       384  _vehicle_moving_near_any_player
EXACT       112  _vehicle_new
EXACT        16  _vehicle_parse_update
EXACT        48  _vehicle_place
EXACT      1024  _vehicle_preprocess_node_orientations
EXACT        96  _vehicle_render_debug
EXACT       176  _vehicle_reset
EXACT       368  _vehicle_stuck
EXACT      2320  _vehicle_update
EXACT        16  _vehicles_dispose
EXACT        16  _vehicles_dispose_from_old_map
EXACT        16  _vehicles_initialize
EXACT        16  _vehicles_initialize_for_new_map
== exact 35  residual 4  unwritten 0  (of 39 listed)
```

Resumed run: candidate.c existed and was byte-identical to source/units/vehicles.c (cmp). No prior REPORT.md.

## Attempt 1 - _create_ghost_effect - ACCEPTED (EXACT)

Evidence (read before compiling): January diff is ONE transposed pair after the
object_get_marker_by_name call: Jan `mov [ebp-4],0` (index) THEN `mov [ebp-0x24],eax`
(32-bit count); ours count-store then index-store. Prior ledger (vehicles_obj.md
"two-store tie ... ten forms") recorded it as a scheduler tie.
NEW evidence: the later first-party debug build halo_cache_symbols.exe
(research/symbol-build-h1-tags-20260906), function at 0x008f5530 (identified by its
"hover thrusters"/"incident"/"normal"/"reflected"/"midpoint" string refs + 'vehi' tag_get):
  008f55d1 mov word [ebp-0x6d8],ax     ; short hover count = call result
  008f55d8 xor edx,edx / mov word [ebp-0x6dc],dx   ; a SECOND short local = 0
  008f55e1 xor eax,eax / mov word [ebp-0x6e0],ax   ; marker_index = 0
  loop test: movsx edx,[idx]; movsx eax,[-0x6d8]; movsx ecx,[-0x6dc]; add eax,ecx; cmp edx,eax; jge
The sibling create_pelican_effect (0x008f5c90) has the IDENTICAL three-slot layout where the
second short is the "jet thrusters" marker count. So ghost is the pelican loop with the jet
count initialised to 0; the loop bound is short+short (an int) - which is exactly why January
holds a 32-bit loop-invariant (`movsx eax,ax` ... `mov [ebp-0x24],eax`) that is stored AFTER the
index zero-store (it is the hoisted invariant sum, not a named long).
Change (candidate.c ~1248-1252): `long marker_count = call();` ->
  `short hover_marker_count = call(); short jet_marker_count = 0; short marker_index;`
  `for (marker_index = 0; marker_index<hover_marker_count+jet_marker_count; marker_index++)`
gate before: residual    656  _create_ghost_effect  [sha]
gate after : EXACT       656  _create_ghost_effect
unitcheck: gains=1 regressions=0 other=0.
Admissibility: the zero-initialised second count is first-party source topology (debug build
stores it and adds it in the loop test); names hover_/jet_ are inferred from the two marker
strings in the sibling. Strip test: without the second count = baseline (non-exact); the local
is not a decoration but an authenticated source variable.

## Attempts 2-10 - _update_alien_fighter_physics_new

Baseline gate: residual   1088  _update_alien_fighter_physics_new  [relocs 33!=32, sha]   (39 REAL regions, real_regions.py)

ROOT CAUSE OF THE RELOC SURPLUS (not an inline-frequency site problem): vehicles.c does
`#define REAL_MATH_EXTERNAL_POINT_FROM_LINE3D` around real_math.h, so point_from_line3d is an
EXTERN in this whole TU and can never inline. January inlines it here (+0x105..+0x145) AND calls it
out of line in _vehicle_update (+0x268, reloc to _point_from_line3d present in build/split vehicles.obj,
whorefs.py) - i.e. January's TU saw the header __inline and necessarily emitted/linked a COMDAT.
 - try_2 (remove the two define/undef lines only): fighter_new relocs 32/32, vehicle_update still EXACT,
   36 exact, BUT our object then emits the 48-byte _point_from_line3d COMDAT -> guard FAIL (policy).

NEW EVIDENCE: later first-party debug build halo_cache_symbols.exe, function 0x008f6a20 (unoptimised,
RTC; identified by 'vehi'/'phys' tag_get + powered_mass_points.count==2 + call topology), and HCEA DIA
locals (magic_force, magic_torque, desired_velocity, acceleration, angle, rotation, rotation_quaternion,
current_rotation, desired_rotation, axis). The debug build shows, in order:
  scale_vector3d(&fwd, speed, &desired_velocity); throttle ?: ; compute_acceleration(.., &acceleration);
  scale_vector3d(&acceleration, mass, &magic_force); scale_vector3d(&magic_force, seat_power, &magic_force)  <- IN PLACE
  ... yaw = cross_product2d(&desired.forward, &velocity)... ; quaternion_to_angle_and_vector(&q,&angle,&axis);
  scale(&axis, .., &desired_angular_velocity); subtract(.., &angular_acceleration);
  scale(&angular_acceleration, (zz+yy+xx)/3, &magic_torque); scale(&magic_torque, seat_power, &magic_torque);
  FIVE real locals initialised 0.05, 0.2, 0.005, 0.01, 0.05 (0x8f6ebc..0x8f6f04);
  spin = magnitude3d()/..; delta = PIN/MIN or -MAX/MAX; ONE `thrust += delta` after the if/else.
January side: force slot [-0x4c] is written by scale(mass) then scaled IN PLACE by seat_power
(+0xee..+0xfd), slot A [-0x34] is shared by desired_velocity and magic_torque, slot B [-0x40] by
acceleration and the quaternion axis, [-4] is shared by angle and later scalar temps -> the early
locals live in a block that closes before axis/torque are born (VC7 only overlaps scope-disjoint locals).

 - try_3 flat debug-build locals: frame 0xb4 (Jan 0xa4), size 1104 - rejected.
 - try_4 two blocks {desired_velocity,acceleration,throttle} / {matrices,quaternion,yaw}: frame 0xa8, force
   block now byte-identical; angle at function scope costs its own slot - rejected.
 - try_5 angle + scale(&axis) inside the rotation block: frame 0xa4, ALL slots identical; 2 x87 order
   regions left (cross_product3d k-term operand order, magnitude3d sum order) - both IL-ordinal (M8) keyed.
 - try_6 + debug-build locals: five `real const` thrust constants, no `average` local, single
   `thrust += thrust_delta`: magnitude3d region closes; k-term remains.
 - try_7 + `cross_product2d((real_vector2d const *)&desired_rotation.forward, (..)&velocity)` as the debug
   build calls it (header-inline config): gate = EXACT (37 exact, 0 regressions).
     BUT: guard FAIL (_point_from_line3d COMDAT) and newsyms OWNERSHIP FAIL (+_cross_product2d COMDAT,
     unreferenced; VC7 emits a COMDAT for every used header __inline).  => NEEDS OWNER RULING. Not landable.
 - try_8 = try_7 with the REAL_MATH_EXTERNAL define restored: guard PASS, 4 REAL regions (all = the one
   call site), OWNERSHIP FAIL (+_cross_product2d).
 - try_9 = try_8 + manual expansion of point_from_line3d (named t): size 1104, not exact; manual expansion is
   rejected-on-sight anyway - abandoned, no spelling sweep.
 - try_10 = try_6 with the define restored (no cross_product2d): guard PASS, OWNERSHIP PASS, unitcheck
   gains=1 regressions=0, fighter_new 39 -> 5 REAL regions (call site + k-term operand pair).  ACCEPTED as
   candidate.c state (residual-improved, landable); try_7.c kept as the EXACT-pending-ruling variant.
gate after (candidate.c): residual   1088  _update_alien_fighter_physics_new  [relocs 33!=32, sha]

## Attempts 11-29 - _update_human_plane_physics

Baseline gate: residual    928  _update_human_plane_physics  [sha]   (39 REAL regions; frame 0xf8 both)

January byte ledger (read first): (a) both create_pelican_effect calls clean their own args separately
(`add esp,0xc` .. `add esp,4`; `add esp,0x20` .. `add esp,4`) = ONE common tail call duplicated per edge, i.e.
if/else with a single pelican call after it; (b) throttle is squared once (`fld st(0); fmulp`) before the flag
tests; (c) factor layout puts the flag-2 arm LAST = `!flag2 ? (flag3 ? 1.0 : 0.75) : 0.25`; (d) hover is
stored then RELOADED from [esi+0x444] while thrust is forwarded through [ebp-4] across the normalize3d call;
(e) vel.i/vel.j are copied with integer movs into an 8-byte local [-0x18..-0x14] (a real_vector2d) that later
shares bytes with the address-taken `angle` [-0x14] => disjoint lexical scopes; (f) final force/torque
scaling is the scale_vector3d inline shape (seat_power param temp at [ebp-4]); (g) moment scale is computed
AFTER scale_vector3d(&axis, angle*(1/30), &scaled); (h) products are in plain source order:
(tern*(1-t))*seat, (sum)*gravity*mass, (speed-dot)*thrust*mass*0.05.
Later first-party debug build (halo_cache_symbols.exe 0x008fa4e0, identified by flag&2 -> csmemset ->
single create_pelican_effect tail, 'vehi'/'phys' tag_gets) corroborates: throttle squared in place; nested
?: for the factor; `interpolate_scalar(&vehicle->hover, desired, 0.05f)` (callee 0x5dc070 has exactly the
real_math.h interpolate_scalar body); 8-byte struct copies into two real_vector2d locals + cross_product2d;
three real locals 0.05/1.3/1.05 initialised right before `dot`, three more pi/2, pi/30, 0.05 right before
the 2D copies (= block-start const locals); scale_vector3d(&force/&torque, seat_power, same).

 - try_11 if/else single tail + squared throttle + thrust local + 2D velocity copy (fighter_old's admitted
   `.n` pointer-loop idiom) + scale_vector3d tail + moment after scale: 37 REAL, frame 0xfc (hover value got
   store-forwarded into a new slot).
 - try_12..18 bisect of the forwarding: not the thrust local, not throttle spelling, not the tail helper, not
   the 2D copy, not if/else; VC7 forwards BOTH hover and thrust across the call in this structure.
 - try_19 `interpolate_scalar(&vehicle->vehicle.hover, ..., 0.05f)`: hover no longer forwarded, frame 0xf8,
   39 -> 10 REAL.  (the store goes through the helper's pointer parameter)
 - try_20 nested ternary (debug-build shape): factor layout fixed, 8 REAL. mass/gravity source order INERT.
 - DIAGNOSTIC D-sweeps (never landed): dot i-term is M8-keyed (closes for +4..+7 initialised locals);
   hover and lift product orders are NOT count-keyed.
 - l1 `lift = (...)*global_gravity; lift *= physics->mass;` fixes the lift order AND the late
   `mov ecx,[ebp+0x10]` schedule; grouping parens / operand order (p1,p2,l2,l3) do not.
 - c1 six `real const` block locals from the debug build: dot i-term closes (2 REAL left: hover product).
 - h1..h6 hover statement shapes: only h6 closes:
       factor = !flag2 ? (flag3 ? 1.0f : 0.75f) : 0.25f;  factor *= 1.0f-throttle;
       interpolate_scalar(&vehicle->vehicle.hover, factor*vehicle->unit.seat_power[0], 0.05f);
   gate: EXACT       928  _update_human_plane_physics      (try_28 / try_29 with consts in their using blocks)
 - strip tests: set_real_vector2d copy (s2a) FAILS (copy is propagated away; January keeps an int-mov 8-byte
   copy) -> the fighter_old `.n` loop idiom is kept (in-file admitted precedent); single-expression hover and
   lift FAIL; literals instead of the six const locals FAIL (M8 i-term).
 - OWNERSHIP: using interpolate_scalar makes our object newly emit the unreferenced 64-byte
   `_interpolate_scalar` COMDAT -> newsyms OWNERSHIP FAIL. It is byte-identical to January's single folded
   copy (split credits it to game/player_control.obj; sha 9b8ea19a97a24174 both) - same class as the
   `_random_range` exception, but NOT granted. Ownership-clean spelling (`hover += PIN(...)`, v_clean):
   frame 0xfc, 31 REAL - NOT adopted (frame regression vs baseline 0xf8).
   => plane left at BASELINE in candidate.c; EXACT variant preserved in candidate_owner_ruling.c.

## candidate_owner_ruling.c (NOT the landable candidate) - measured
   = candidate.c + plane h6 body + header-inline point_from_line3d (define/undef removed) + cross_product2d
   in fighter_new.  gate: == exact 38  residual 1  unwritten 0; unitcheck gains=3 regressions=0.
   newsyms: OWNERSHIP FAIL (+_cross_product2d, +_interpolate_scalar, +_point_from_line3d); guard FAIL.
   All three emitted COMDATs are byte-identical to January's folded copies (symbytes.py):
   _point_from_line3d (action_charge.obj) 9b763841f8519177, _interpolate_scalar (player_control.obj)
   9b8ea19a97a24174, _cross_product2d (actor_combat.obj) eb266b2f8e53f8f6.
   January's own _vehicle_update calls _point_from_line3d out of line (+0x268), so January's vehicles TU
   demonstrably had the header inline visible. NEEDS OWNER RULING.

## _update_alien_scout_physics - evidence pass, NO source change (residual-unchanged)

Baseline gate: residual   2464  _update_alien_scout_physics  [sha]   frame Jan 0x94 / ours 0xa0 (101 REAL regions)
Slot census (alndiff, unique ebp-N refs): January homes 24 cells below the 52-byte matrix (matrix at -0x94);
ours homes 27 (matrix at -0xa0; surplus cells -0x58/-0x68/-0x6c and a different refcount profile on
-0xc/-0x18/-0x1c/-0x20).  Relocation-target multiset already equals January (91/91).
New evidence read: later first-party debug build 0x008f80b0 call topology (dbg_8f80b0.asm, thunks resolved by
body): set_real_quaternion(&state[i].rotation,0,0,0,1) in the antigrav loop, scale_vector3d(&unit.throttle,
maximum_speed,&desired_velocity), subtract_vectors3d(&desired_velocity,&velocity,&desired_acceleration),
limit3d, matrix transform, then NINE point_from_line3d accumulations into magic_force/magic_torque
(0x8f8477, 0x8f86bb, 0x8f8c55, 0x8f8c8f, 0x8f8cbf, 0x8f8e07, 0x8f8e46, 0x8f8fd9, 0x8f9025), two
point_from_line2d (Astra), cross_product3d x3, final scale_vector3d(&magic_force/&magic_torque, antigrav) x2.
RTC names: magic_force, magic_torque, vehicle_matrix, velocity, desired_velocity, desired_acceleration,
vehicle_left, vehicle_forward_horizontal, vehicle_left_horizontal, applied_alpha, offset_from_vertical,
angular_velocity, alpha_desire, alpha_bonus, turning_torque, vehicle_left, vehicle_horizontal_forward.
Our source hand-expands every one of those accumulations as scalar `+=` triples because this TU makes
point_from_line3d an extern (REAL_MATH_EXTERNAL_POINT_FROM_LINE3D).  The two functions that DID close in
this pass (fighter_new, plane) closed only when the real header inlines were used, and each manual expansion
measured differently from the helper (IL param temps shift M8 ordinals; pointer-param stores block x87
store-forwarding; hand scalars home extra cells).  A faithful scout needs the same: real helper calls at
the nine sites -> inadmissible under the _point_from_line3d ownership ruling (and point_from_line2d would be
a new COMDAT).  No shapes were spent: Astra's two helper shapes + the ledger's direct-PD/lifetime sweeps
already measured the hand-expanded family (best 1313 differing bytes, frame still 0xa0).
Reopen: owner ruling admitting the header-inline point_from_line3d (+ byte-identical helper COMDATs) in
vehicles.obj; then transcribe 0x008f80b0 statement-for-statement (block-start const locals mark the lexical
blocks: 0.5 water depth at function top; 0.8 in the flag-3 arm; 0.05/0.98 in the on_ground arm; ...).

## FINAL STATE (candidate.c)  - python tools/campaign/gate.py source/units/vehicles --source scratch/lane_b/w/vehicles/candidate.c --forbid-emitted-symbol _point_from_line3d --all
```
== emitted-symbol guard passed (1 forbidden names checked)
EXACT       208  _compute_acceleration
EXACT       160  _compute_airborne_ticks
EXACT       544  _create_crashing_effects
EXACT       656  _create_ghost_effect
EXACT       512  _create_pelican_effect
EXACT       464  _create_slipping_effects
EXACT        32  _set_real_quaternion
EXACT       384  _slowly_stop_vehicle
EXACT       112  _update_alien_fighter_physics
residual   1088  _update_alien_fighter_physics_new  [relocs 33!=32, sha]
EXACT       768  _update_alien_fighter_physics_old
residual   2464  _update_alien_scout_physics  [sha]
EXACT       784  _update_human_boat_physics
EXACT       272  _update_human_jeep_physics
residual    928  _update_human_plane_physics  [sha]
EXACT       336  _update_human_tank_physics
EXACT       800  _update_suspension
EXACT        32  _update_turret_physics
EXACT       256  _vehicle_accelerate
EXACT        16  _vehicle_build_update
EXACT        48  _vehicle_causes_collision_damage
EXACT        16  _vehicle_delete
EXACT      1312  _vehicle_export_function_values
EXACT       304  _vehicle_find_pathfinding_surface_index
EXACT        80  _vehicle_hover
EXACT        48  _vehicle_is_flipped
EXACT       384  _vehicle_moving_near_any_player
EXACT       112  _vehicle_new
EXACT        16  _vehicle_parse_update
EXACT        48  _vehicle_place
EXACT      1024  _vehicle_preprocess_node_orientations
EXACT        96  _vehicle_render_debug
EXACT       176  _vehicle_reset
EXACT       368  _vehicle_stuck
EXACT      2320  _vehicle_update
EXACT        16  _vehicles_dispose
EXACT        16  _vehicles_dispose_from_old_map
EXACT        16  _vehicles_initialize
EXACT        16  _vehicles_initialize_for_new_map
== exact 36  residual 3  unwritten 0  (of 39 listed)
```
unitcheck: SUMMARY gains=1 regressions=0 other=0   (+ _create_ghost_effect residual 656 -> EXACT 656)
newsyms  : candidate newly DEFINES 0 symbol(s), no longer defines 0 / OWNERSHIP PASS
fake scan: Scanned 1 file(s); 0 review lead(s)
CL /Zs /W3: same warning set as the real source minus one C4101 (unused `scaled` removed). The two C4013
  (matrix3x3_multiply, matrix3x3_rotation_to_quaternion implicit in fighter_new) are PRE-EXISTING at baseline;
  pointer-only arguments so no double-push; prototypes belong in math/real_math.h (header packet, not needed
  for bytes - fighter_new is EXACT with them implicit in candidate_owner_ruling.c).

candidate_owner_ruling.c (NOT landable without rulings): == exact 38  residual 1  unwritten 0; unitcheck
gains=3 regressions=0; guard FAIL; OWNERSHIP FAIL (+_cross_product2d +_interpolate_scalar +_point_from_line3d,
each byte-identical to January's single folded copy).

## Scout addendum (after the final-state block above; candidate.c unchanged)
 - try_30 (owner-ruling config only): replaced the nine hand-expanded accumulations with the debug build's
   point_from_line3d calls and the final six `*= antigrav` with scale_vector3d x2: scout output IDENTICAL to the
   hand-expanded form (101 REAL regions, frame 0xa0, 38 exact overall). So the helper-vs-expansion choice is
   NOT the scout's frame defect; rejected as zero-gain.
 - Byte ledger of the 12-byte frame surplus: ours homes the hover<1 block's cross-product `left` in its OWN
   12-byte cell [-0x60..-0x58] (stores +0x345/+0x35e/+0x36d, loads +0x5ae/+0x5b9/+0x5c4). January has no
   such cell: its `vehicle_left` is at [-0x4c..-0x44] (fstp +0x34b/+0x364/+0x379, fld +0x592/+0x5a0/+0x5ab),
   bytes that in ours belong to the hover>0 block's locals; January's local_velocity sits at [-0x60..-0x58]
   directly under the matrix (-0x94). January also makes the two horizontal 2D vectors by 8-byte int-mov
   struct copies (forward2d [-8..-4] from [ebx],[ebx+4]; left2d [-0x1c..-0x18] from the stored `left`),
   the same copy shape the plane needed. Remaining decision = which sibling-block locals January overlaps
   with `vehicle_left` (a scope/type-shape layout question over the whole hover<1 block, RTC names above).
   Not attempted: needs a statement-for-statement transcription of 0x008f80b0 (1,100 debug instructions).
