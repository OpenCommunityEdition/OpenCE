# source/units/vehicles

## verdict
FUNCTIONS_EXACT_OBJECT_BLOCKED

## newly_exact_functions
[
 "_update_alien_scout_physics",
 "_update_alien_fighter_physics_new"
]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\vehicles\\cand.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\vehicles\\production.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\vehicles\\cand_nofighter.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\vehicles\\production_linkclean.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\vehicles\\symbols_turret_static.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\vehicles\\LEDGER.md",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\vehicles\\bodies\\final2.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\vehicles\\dbg_scout.asm",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\vehicles\\cand_gate.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\vehicles\\cand_object_audit.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\vehicles\\cand_provider_link.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\vehicles\\cand_surplus_identity.txt"
]

## production_changes
Two landing options. Each patch is git-apply verified to reproduce its candidate byte for byte.

(A) FULL: `git apply scratch/w/vehicles/production.patch` on source/units/vehicles.c, which gives scratch/w/vehicles/cand.c. The gate is 39/39 EXACT.
Hunks:
1. Delete the `#define REAL_MATH_EXTERNAL_POINT_FROM_LINE3D` / `#undef` pair around the real_math.h include. January's vehicles TU saw the header __inline: `_vehicle_update` calls `_point_from_line3d` out of line at +0x268 and the fighter inlines it. vehicles is one of the owner's 17 out-of-line-reference objects.
2. `vehicle_find_pathfinding_surface_index`: replace the hand expansions (and the stale comment) with two genuine calls, both real calls in the /Od build 0x8fd970 at 0x8fda42 and 0x8fdad2:
   - `point_from_line3d(&origin, global_up3d, 0.4f, &origin);`
   - `point_from_line3d(&origin, &vector, result.t, position);`
   The function stays EXACT and needs no casts. This is a rule-6 cleanup.
3. Fighter `_update_alien_fighter_physics_new`: `yaw = cross_product2d((real_vector2d const *)&desired_rotation.forward, (real_vector2d const *)&vehicle->object.translational_velocity)/...`. This is Lane B's held form, /Od-attested at 0x8f6cbf..0x8f6ccd. The two casts are byte-inert (strip test).
4. `update_alien_scout_physics` rewritten from bodies/final2.txt. It is a statement-for-statement /Od 0x8f80b0 transcription:
   - RTC names; no pointer locals; /Od dot_product3d argument orders; /Od block-start real consts.
   - A `steering_input` copy of steering.
   - Two point_from_line2d calls for alpha_desire.
   - Nested tail-block counters.
   - The third parameter renamed powered_mass_points (the HCEX name).
   Plus two deviations backed by January bytes:
   - `vehicle_forward_horizontal` declaration-initialised (8-byte copy) BEFORE the cross_product3d; applied_alpha zero first.
   - The leveling term as `point_from_line2d((real_point2d *)&applied_alpha, &alpha_desire, leveling_scale*(1.0f-vehicle->object.up.k), (real_point2d *)&applied_alpha)`.
   It uses 28 byte-inert representation casts ((real_point3d *) x18, (real_point2d *) x6, dot_product2d((real_vector2d const *) x4)). Stripping them all leaves the section byte and reloc identical. Disclose these under ruling 6.

Caveat for (A): the newly emitted `_cross_product2d` COMDAT is IDENTICAL to January's selected copy (actor_combat) but FAILS provider_link (LNK2005). The cause is that source/ai/actor_combat.c still hand-writes `real cross_product2d(` behind `#define cross_product2d cross_product2d_inline`. So (A) is landable only together with an actor_combat provider repair, or an owner waiver.

(B) LINK-CLEAN NOW: `git apply scratch/w/vehicles/production_linkclean.patch`, which gives cand_nofighter.c. It is hunks 1, 2 and 4 without the cross_product2d hunk.
- Gate: 38 exact / 1 residual. The scout goes to EXACT. The fighter improves from [relocs 33!=32, sha] to [sha] but stays residual.
- Only row changed vs base: scout.
- New surplus `_point_from_line2d` (action_vehicle) and `_point_from_line3d` (action_charge) PASS provider_link and are IDENTICAL.

(C) SEPARATE, trivial: apply scratch/w/vehicles/symbols_turret_static.patch, then run a csplit-only regen of the vehicles split.
- It adds `"static": true` to config/symbols.json line 7052, `_update_turret_physics` (in-place edit; do not re-serialize).
- Evidence: cachebeta publics lack it (pdb_storage: PDB-public False). No January object references it. Ours is already static.
- This clears object_audit's only symbol DIFF.

Parks to retire: none. No parked.json, semantic or rejection entries exist for either function.

## evidence
Baseline (production build/base): gate 37 exact / 2 residual. Fighter [relocs 33!=32, sha]; scout [sha], frame 0xa0 vs January 0x94.

Fighter: the only cause of the reloc surplus is the TU-wide REAL_MATH_EXTERNAL_POINT_FROM_LINE3D. p_nodefine gives [sha] with 32/32 relocs. Adding Lane B's /Od-attested cross_product2d makes it EXACT (f1_held).

Scout path, measured (all in LEDGER.md):
- m15, the structural lane's best (775/775 insns, frame 0x94), leaves 12 real blocks at 4 inline-helper x87 sites S1..S4.
- t1, the /Od transcription (no pointer locals, /Od argument orders): 4 blocks.
- p1 / i2, forward 2D copy before the cross: 1 block. S2 closes; the i2 product was D-invariant, so this is statement order, not a count.
- l1 / final, leveling as point_from_line2d: 39/39 EXACT.

The l1 evidence is in January's own bytes at +0x53a..+0x557: `fld 1; fsub up.k; fmul 0.0038785094; fst [ebp-4]; fmul alpha_desire.i ... fld [ebp-4]; fmul alpha_desire.j`. This is the homed t-parameter signature of an inlined point_from_line2d/point_from_line3d.

Negatives and lab work:
- Rejected spellings: l2 (named local), c1 (cross return value), p2/p3, i1, k_a..k_d, f1..f3, final_nocast, final_b.
- Lab only, never landed: a D-sweep of dead locals and dead-call instruments localised the gap to exactly one inline temp between S2 and S3.
- q1/q2: the /Od dot_product3d argument orders are right; swapping either breaks.
- Declaration order is inert (t2a/b/c).

Checks on the final cand.c:
- gate == exact 39 residual 0 (cand_gate.txt).
- fake_match_scan: 0 leads.
- /W3 warning set identical to base apart from line numbers.
- Cast strip (cand_strip.c): both functions byte and reloc identical.
- object_audit: every January-owned section ok; the only symbol DIFF is _update_turret_physics storage.
- surplus_identity: 30 COMDATs, all IDENTICAL except the inherited _object_get_bounding_sphere.
- build/report.json vehicles data: 1744/1744 (100%).

## blockers
Whole-object admission is blocked by items OUTSIDE the two functions.

1. Provider links fail, FAIL(4):
   - _cross_product2d: new with option A. actor_combat.c keeps a NODUP hand copy. Its genuine use is at /Od actor_combat fn 0x44ebb0 (call at 0x44ed45); our actor_combat.c has no cross_product2d use yet.
   - _interpolate_scalar: inherited, provider player_control.
   - _object_get_bounding_sphere: inherited, provider action_vehicle. Also bytes DIFFERENT vs January's selected copy; the objects.h inline differs.
   - _triple_product3d: inherited, provider decals.
2. Invented private helper owners January lacks, all inherited and still used by EXACT functions:
   - _vehicle_cross_product3d_target, _vehicle_dot_product3d_target and _vehicle_triple_product3d_target (_update_human_boat_physics).
   - _vehicle_dot_product3d_test (_vehicle_preprocess_node_orientations).
   Attempts, all stopped and recorded:
   - boat1/boat2, an /Od 0x8f9a10 transcription with genuine helpers: same 784 B size, 267 vs 266 insns, 17 D-invariant blocks.
   - preprocess with header dot_product3d: 47 blocks.
3. The _update_turret_physics storage needs the symbols.json static fix plus a csplit regen (patch provided).

## reopen_criteria
Once the actor_combat, player_control, decals and action_vehicle providers drop their NODUP hand copies for the header inlines (owner decision), rerun provider_link: option A should then be link-clean. For actor_combat, restore the genuine cross_product2d use from /Od fn 0x44ebb0 first. Separately, fix the objects.h object_get_bounding_sphere inline so it matches January's action_vehicle copy byte for byte.

To remove the invented helper debt, rewrite the boat and preprocess onto genuine helpers while staying exact. Start from boat1/boat2 (the /Od transcription): the remaining A/B regions are D-invariant, so look for statement-order or aggregate-copy causes (the scout S2 lesson), not local counts.

When all of that holds, rerun the section-4 audit on cand.c for OBJECT_COMPLETE_CANDIDATE.

## task notes
New levers measured in this unit, for other lanes:\n\n1. Once invented pointer locals are removed, the helper argument order in a /Od transcription is NOT inert (q1/q2). With header inlines visible, the /Od transcription alone cut the scout from 12 to 4 regions.\n2. D-invariant ('tree shape') x87 operand orders on products with no displacement-0 operand moved with the statement order of an adjacent aggregate copy that reads the same object: the forward 2D copy placed before cross_product3d.\n3. Use a dead 1-param inline call as a LAB instrument to localise a missing IL temp window between two sites. Then look in January's bytes for a helper parameter signature: an `fst [ebp-N]` homing of a factor computed once marks an inlined point_from_line t-parameter. That found the genuine point_from_line2d the later /Od build had spelled as two component statements.\n4. An explicit `(real)(...)` on an unavoidable double-to-real argument conversion is byte-relevant: removing it breaks S2/S4 and the plain form raises C4244. It is kept from production's existing spelling and disclosed. The owner may want to rule on whether such warning-silencing conversions pass the strip test.\n\nCandidate provenance:\n- Fighter: Lane B `/c/halo-worktrees/claude-lane-b-ai-physics-units-20260920/scratch/lane_b/w/vehicles`.\n- Scout base: the structural lane's m15 at `/c/halo-worktrees/claude-structural-20260923/scratch/lane/w/units__vehicles`, which was then superseded by my /Od transcription.\n\nNo tracked file was edited, and no ninja, configure or git-mutating command was run.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\vehicles\LEDGER.md. It records every probe as hypothesis, evidence, change and measured result: baseline, prior-lane evidence located, probes 1-9, the section-4 audit, the boat/preprocess side attempts, the final candidate, the split-landing option, and methodology notes.
