# review source/units/vehicles
approve=False

## checks
All notes and objects are in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review_vehicles\ (REVIEW.md).
1. Patch reproduction. I applied each patch outside the repo to HEAD's vehicles.c.
   - production.patch gives cand.c byte for byte.
   - production_linkclean.patch gives cand_nofighter.c byte for byte.
   - symbols_turret_static.patch changes only config/symbols.json line 7052, in place.
2. Gate, re-run independently.
   - cand.c: exact 39, residual 0.
   - cand_nofighter.c: 38/1 (fighter_new [sha]).
   - Production: 37/2.
3. object_audit on the review objects.
   - cand.obj: all 68 January-owned sections ok. The only symbol DIFF is the _update_turret_physics storage (2 vs 3).
   - New surplus vs base: exactly _cross_product2d (A only), _point_from_line2d and _point_from_line3d.
4. surplus identity (my copy, which takes an explicit obj). All new COMDATs are IDENTICAL to January's selected copies. The only DIFFERENT one is _object_get_bounding_sphere, which base also has.
5. provider_link.
   - A: FAIL(1), _cross_product2d LNK2005 against actor_combat in both orders.
   - B: new-surplus PASS. Its full FAIL(3) is the same set as base (_interpolate_scalar, _object_get_bounding_sphere, _triple_product3d).
6. /W3 (my warn.py). The warning multiset is identical for base, A and B.
7. Cast strip. I re-gated cand_strip.c: 39/39, and the scout and fighter sections equal cand.obj. The 28 pointer representation casts are byte-inert.
8. /Od listing regenerated with od_linear.py; it matches dbg_scout.asm. Checked site by site:
   - Hunk 2: pfl3d calls at 0x8fda42 and 0x8fdad2.
   - Hunk 3: cross_product2d at 0x8f6cbf..0x8f6ccd; the body at 0x455250 is a.i*b.j - a.j*b.i.
   - Hunk 4 matches /Od everywhere except the two disclosed deviations and a minor tail-block const/delta placement.
9. HCEX DIA2Dump confirms the param name powered_mass_points and all RTC/local names.
10. The symbols.json change for _update_turret_physics is backed by the evidence.
   - cachebeta has no public symbol at 0x1A4F50; its neighbours _vehicle_hover and _vehicle_is_flipped are public.
   - Only vehicles.obj defines or references it.
11. Strip tests on the constructs that were not stripped:
   - /Od-literal leveling: rv_od_leveling.
   - Field copies for the forward/left 2D copies: rv_fc_fwd, rv_fc_left, rv_fc_both.
   - Scout (real) casts: rv_nocast_sqrt, rv_nocast_fabs.
12. build/report.json vehicles data is 1744/1744. There are no parks or admission rejections for the unit.
13. Fallback: fallback_hunks12.patch (hunks 1 and 2 only) was built and checked.

## issues
BLOCKING 1 (not disclosed): the scout adds two aggregate 3D->2D view copies, and both are needed for the match.
- The copies are `real_vector2d vehicle_forward_horizontal = *(real_vector2d const *)&vehicle->object.forward;` and `vehicle_left_horizontal = *(real_vector2d const *)&vehicle_left;`.
- Production uses field copies at both sites. With field copies in cand.c, the scout has 11 REAL regions (forward only), 23 (left only) and 29 (both).
- The brief requires a byte-inert strip test for any new 3D->2D view cast. These casts fail it.
- The owner has not yet ruled on this shape for this function: docs/object_matching_logs/claude_structural_owner_packet_20260923.md section 5 lists _update_alien_scout_physics and asks whether a /Od-attested aggregate view copy qualifies when the field copy does not match.
- The worker's disclosure says all 28 casts are byte-inert and does not mention these two.

BLOCKING 2: the leveling point_from_line2d is chosen only because it shifts the count.
- It contradicts /Od at its own site. /Od 0x8f8b4a..0x8f8bba has two component statements, `applied_alpha.i/j += leveling_scale*alpha_desire.i/j*(1-up.k)`, and 0x444020 is called only twice, both for alpha_desire.
- The worker's claimed January-byte evidence is refuted. The homed t at +0x53a..+0x557 (`fst [ebp-4]`) is produced identically by the /Od component spelling. My rv_od_leveling.obj and the worker's own i2.obj both have those exact bytes there.
- With the component spelling, the only remaining difference is a 2-instruction M8 leaf swap at +0x628 (S3): January has `fld [ebx]; fmul [esi+0x18]` and ours has the reverse.
- The worker's own bisect shows that any +1 inline temp anywhere in the window closes S3, so the evidence does not single out this construct.
- That makes it the count oracle used as a landing. Per the owner gates, the leveling needs an owner ruling or independent evidence.

BLOCKING 3 (disclosed): option (A) emits a _cross_product2d COMDAT that fails LNK2005 against actor_combat's hand copy (the actor_combat provider problem). The fix is owner packet section 1, decision (b), which is still pending.

Because hunk 4 is in both (A) and (B), neither can land now.

MINOR:
- Two (real) casts in the scout are needed for the match: square_root((real)(...)) gives 4 REAL regions without it, and (real)fabs gives 3. Production already uses the same idiom and the worker disclosed the sqrt one. Listed for owner awareness only.
- The candidate declares the tail-block 0.1 const and delta at the top of the block. /Od declares them in a nested block after target (worker's t2c shows this is byte-inert). So the "statement-for-statement" claim is slightly overstated.

LANDABLE WITHOUT A RULING:
- (C) scratch/w/vehicles/symbols_turret_static.patch plus a csplit-only regen of vehicles. The evidence is verified, and it clears the only symbol DIFF.
- Optionally, scratch/w/review_vehicles/fallback_hunks12.patch (hunks 1 and 2: the define removal and the /Od-attested pathfinding point_from_line3d calls).
  - Gate stays 37/2; the fighter improves to [sha] and no row is lost.
  - The _point_from_line3d COMDAT is IDENTICAL and provider_link PASSes.
  - /W3 is unchanged. It earns no exact credit; it is a rule-6 cleanup.

Once the owner rules on section 5 (view copies) and on the count-only leveling pfl2d, (B) could land as proposed (scout EXACT, 38/1). (A) would also need the actor_combat provider repair.
