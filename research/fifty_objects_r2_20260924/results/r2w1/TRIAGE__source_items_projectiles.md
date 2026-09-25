# source/items/projectiles

## decision
OWNER

## est_probability
0.15

## route
_projectile_new (432 B) is strict exact (30/30, audit PASS) only when the initial_velocity load feeding the inlined point_from_line3d t-argument is parenthesised. The TU-private named-macro form `((definition)->projectile.initial_velocity)` is also exact. Owner ruling 2026-09-21 #4 treats a decoration-only parenthesis as steering unless the name is first-party-attested; none is known. The only open owner question is consistency with canonical's landed INFERRED named macro ai_debug_attractor_label_height (ai_debug.c:48-65, the same inline-then-call x87 lever) and the collisions.c:1572 precedent the owner said to revisit. Independent and landable now: the r1 semantic_data_matches entry for _projectile_update_section (1,548 B, 6 relocs, sha 7bfaf5fb..., allow_incomplete_unit) and the zero-credit rule-6 patch that replaces the hand-expanded point_from_line3d with the /Od-attested call.

## why_new
No new first-party evidence. Canonical's ai_debug macro is precedent the r1 lane did not put to the owner. The provider blockers r1 listed (_distance3d, _real_random, _valid_real_vector3d_axes2 NODUP) are gone from today's hint packet: link_fail and surplus_different are empty. So the ruling is the object's only blocker.

## prior_negatives_checked
r1 fn_projectiles_source_items_projectiles_projectile_new.md (P1-P14: casts, M8 extra locals, filler, folds x*1.f / TRUE?x:0, double t, local copies, parens on other operands; all inert except an outer paren on the load); projectiles_obj_evidence_closeout_20260919.md; projectiles_obj_opus5_pdb_ida_20260914.md; owner ruling #4 (memory owner-rulings-20260921-lane-b).

## notes
Ready candidate: r1 lab_pfull.c / lab_localrhsparen.c. If the owner says no, RETIRE. The data entry alone is worth +1,548 credited data bytes now.