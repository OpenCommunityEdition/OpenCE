## source/items/projectiles — HELD_CLASS

- construct: _projectile_new: a decoration-only outer parenthesis around the definition->projectile.initial_velocity load that feeds the inlined point_from_line3d t argument. It appears either bare or as a TU-private named macro ((definition)->projectile.initial_velocity).
- precedent: 2026-09-21 Lane B owner ruling #5 and the RB2 refusal: a decoration-only parenthesis is steering. It is narrowed further by the conditions of the 2026-09-20 named-accessor ruling (73c05198).
- completes_object: True
- differences: This is RB2's shape exactly: one parenthesis on a plain field load, idempotent, and every undecorated spelling (P1-P14) is residual. It fails the 73c05198 condition that 'the name must carry meaning that first-party sources attest': HCEX lists only the parameter, and no /Od string or atlas name exists. The ai_debug_attractor_label_height macro (ai_debug.c:48-63, edd8b386, 2026-09-20 17:27) does not cover this. It landed under the earlier inferred-macro 'standing rule', 46 minutes before the 18:13 accessor ruling added the first-party-name condition and a day before RB2, and nobody has re-adjudicated it. It is an inconsistency to raise with the owner together with the collisions.c bare parenthesis (now collisions.c:1581, which RB2 explicitly placed under review). On a yes, the object completes together with the data entry below (triage: link_fail and surplus_different are empty).
- packet: Held; do not apply. research/fifty_objects_20260925/w/projectiles/lab_pfull.c or lab_localrhsparen.c
- evidence: docs/object_matching_logs/claude_lane_b_rejected_hypotheses_20260920.md:45-56 (owner: "A decoration-only parenthesis with no semantic or naming evidence is codegen steering, not admissible source. Revisit the collisions.c precedent separately."). docs/object_matching_logs/claude_lane_b_HANDOFF_20260920.md:81-82. docs/object_matching_logs/claude_lane_a_adjudication_paren_accessor_20260920.md:126-135 ('What the ruling does NOT license'). docs/object_matching_logs/claude_lanes_AB_HANDOFF_TO_LANE_C_20260921.md:321. git show edd8b386 ("landed under the owner's standing rule").

## source/items/projectiles — COVERED_BY_PRECEDENT

- construct: A single-section semantic_data_matches entry for _projectile_update_section with allow_incomplete_unit (1,548 B, 6 relocations, sha 7bfaf5fb...). The cause is the objdiff 3.3.1 '$'-literal defect.
- precedent: The leaf_map _leaf_map_initialize_section allow_incomplete_unit single-section entry (config/semantic_data_matches.json around lines 174-184, ae12a3c1), plus brief section 4.5 (single-section entries are applied by the integrator).
- completes_object: False
- differences: None material. As in the precedent: one data section, byte- and relocation-identical under coff_compare, undercredited only by the '$'-literal defect, unit still incomplete, and .rdata already at 100%. The snapshot must be re-measured at HEAD before the entry is appended.
- packet: C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/projectiles/semantic_data_entry.json (append to config/semantic_data_matches.json; not yet in canonical)
- evidence: config/semantic_data_matches.json leaf_map entry: "allow_incomplete_unit": true, 2512 B / 28 relocations. research/fifty_objects_20260925/results/wave1/fn_projectiles_source_items_projectiles_data_projectile_update_section_.md: 'same as the leaf_map precedent'.

## source/items/projectiles — COVERED_BY_PRECEDENT

- construct: Zero-credit rule-6 cleanup: replace the hand-expanded point_from_line3d with the /Od-attested call point_from_line3d(&velocity,&forward,initial_velocity,&velocity).
- precedent: 2026-09-21 owner ruling #1: _point_from_line3d may be emitted iff January's same object references it out of line. projectiles is one of the 17 objects.
- completes_object: False
- differences: None. The object is byte-identical to build/base, and _projectile_new keeps its park sha 9d8477d2. git apply --check passes at HEAD 434f0151.
- packet: research/fifty_objects_20260925/w/projectiles/production.patch
- evidence: docs/object_matching_logs/claude_lane_b_HANDOFF_20260920.md:65-68 and :156-162 (the list of 17 objects includes projectiles). The house rules (docs/campaign_house_rules.md:29-37, 3c74fa36) forbid hand expansion.

## source/ai/actions — NEW_RULING_NEEDED

- construct: _actor_action_handle_vehicle_entry: a new TU-private squaring macro, #define actor_action_distance_squared(distance) ((distance) * (distance)), at the vehicle_entry square.
- precedent: 2026-09-20 owner ruling for _actor_perception_refresh: a TU-private, descriptively named inferred macro (5186c5bd, source/ai/actor_perception.c:386-432).
- completes_object: False
- differences: The 5186c5bd ruling was function-scoped: "Land _actor_perception_refresh (+2,416) with a TU-private ... inferred macro". Its strongest argument, that only ((d)*(d)) matches, does not hold here: the hand-written (d)*(d) also matches, so the bytes do not prove a macro stood at this site. The owner allowed that weak form only inside actor_perception.c, where the macro already existed ('the admitted spelling of this square'). Landing here would create a new macro in another TU. The structural lane held it for exactly this reason (owner packet s7 row). RB2 (2026-09-21) came later and makes a paren-only difference suspect. Rulings are not to be extended across functions (AB handoff item 3). The object completes only if this item AND the grouped-data item are both a yes.
- packet: research/fifty_objects_20260925/w/actions/owner_gated_exact.patch (git apply --check OK; replaces production_fuzzy.patch)
- evidence: docs/object_matching_logs/claude_lane_a_waveB_frame_and_nearexact_20260920.md:127-129 (the ruling quote). source/ai/actor_perception.c:413-429 (the second-site weak-form note). docs/object_matching_logs/claude_structural_owner_packet_20260923.md:162 ("extends the 2026-09-20 actor_perception ruling to actions.c"). docs/object_matching_logs/claude_lanes_AB_HANDOFF_TO_LANE_C_20260921.md:65-69.

## source/ai/actions — NEW_RULING_NEEDED

- construct: Grouped 46-member data entry (2,404 B across .data/.rdata), which needs verifier_combined_extent.diff applied to tools/semantic_progress.py.
- precedent: Existing grouped semantic data entries (editor_flying_camera 'camera-static-sections', 5019c186, credit_raw_size; the shell_xbox grouped entry).
- completes_object: False
- differences: The existing groups pass the current verifier. This one fails it ('semantic data group does not cover the reported unmatched sections'), because objdiff's running-offset alignment gives 2388 against a member padded sum of 2396, and raw sizes do not reach 2404. Crediting it needs a scorer/tool change or objdiff 3.6.0. Brief section 4.5 allows only single-section entries, so the integrator or owner must accept the tool change.
- packet: research/fifty_objects_20260925/w/actions/verifier_combined_extent.diff + C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/actions/semantic_data_entry.json
- evidence: research/fifty_objects_20260925/results/wave2/fn2_actions_source_ai_actions_data_data_16_rdata_2388_2_404_B_.md:18-27. config/semantic_data_matches.json (group 'camera-static-sections'). docs/object_matching_logs/claude_fifty_objects_20260925.md:317.

## source/ai/actions — COVERED_BY_PRECEDENT

- construct: Zero-credit fuzzy re-baseline production_fuzzy.patch: vehicle_entry is down to one 5-byte FP/int transposition, with the park re-baselined to 7ef7a0e0. Use it only if the squaring macro is refused.
- precedent: House rule 11 and the 2026-09-07 user clarification: preserve the best credible house-rule-compliant fuzzy source at zero credit.
- completes_object: False
- differences: None. It adds no credit and changes no other section. It is mutually exclusive with owner_gated_exact.patch. git apply --check passes.
- packet: research/fifty_objects_20260925/w/actions/production_fuzzy.patch
- evidence: docs/campaign_house_rules.md:17 (rule 11) and :49-54 (user clarification 2026-09-07).

## source/ai/actor_perception — NEW_RULING_NEEDED

- construct: _actor_perception_aiming_vector_test_blockage (Packet B). It needs three constructs: (1) the /Od-attested aggregate view copy source_planar_direction = *(real_vector2d const *)source_vector; (2) an empty then-arm at hes<0.36; (3) a redundant explicit else blockage = 0;. All three are load-bearing.
- precedent: 3ba2eb91 (2026-09-21): the owner-admitted action_obey aggregate view copy (source/ai/action_obey.c:1259) covers construct (1). No precedent covers (2) or (3).
- completes_object: True
- differences: (1) Covered: /Od 0x4710a0 shows an 8-byte integer aggregate copy, the layout is a compatible prefix, the function is strict exact at 44/44, and the field copy is residual (384!=400). action_obey had the same not-strip-testable caveat and the owner admitted it. (2) The empty then-arm has no credited precedent: a tree scan finds one empty then-arm, first_person_weapons.c:1707, in a parked zero-credit function. (3) A redundant equal-value store was rejected by the owner as 'semantically inert steering' (f5698ba2, _actor_look_update (c); AB table #9). The only difference here is /Od attestation. Structural owner packet s5 put this function to the owner explicitly and has not been answered. The ruling request must also name the four newly emitted COMDATs (_normalize2d, _scale_vector2d, _dot_product2d, _negate_vector3d), per ruling #6.
- packet: Blockage body from C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/actor_perception/final_full.c. research/fifty_objects_20260925/w/actor_perception/production.patch no longer applies at HEAD (Packets A/C landed), so regenerate a blockage-only hunk. Retire the blockage park.
- evidence: git show 3ba2eb91: owner quote "Admit only with per-site /Od attestation, byte-inert cast, compatible prefix layout, strict exactness, and ledger disclosure. Land the three; keep the fighter held." plus the action_obey caveat ('cannot be strip-tested ... every undecorated alternative ... residual'). docs/object_matching_logs/claude_structural_owner_packet_20260923.md:127-144. research/fifty_objects_20260925/results/wave1/REVIEW__source_ai_actor_perception.md issue (b). git show f5698ba2.

## source/ai/action_charge — HELD_CLASS

- construct: _action_charge_perform. (a) Load-bearing aggregate view copies C1/C2. (b) An /Od-only dead target_point copy and RMW (target_point.z += 0.4f, never read). (c) A named-result subset chosen to fit the count.
- precedent: Brief section 2 and section 8: filler/dummy declarations and name-count compensation are an ORACLE, never a landing. The admitted dead-pair precedent (editor_flying_camera 'left' pair) requires that a count-only control is NOT exact.
- completes_object: True
- differences: (a) alone would be the 3ba2eb91 aggregate-copy class (/Od integer copies at 0x4310c8/0x4311c7). (b) fails the editor_flying_camera condition. An unattested dead RMW in the suicide arm (x_suicide), the same code at the top of the block (x_top), and 3-7 dummy 'long m8_N = 0' locals are all EXACT, so the bytes cannot tell the /Od code from a filler. HCEX has no target_point. (c) Several subsets are exact and the full /Od set is residual. If the owner overrode the hold, a yes would complete the object (22/22, audit PASS).
- packet: Held; do not apply. research/fifty_objects_20260925/w/action_charge/production.patch; minimal variant C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/review_action_charge/v/no_a4_vec.c
- evidence: research/fifty_objects_20260925/w/review_action_charge/REVIEW.md B1-B3. docs/object_matching_logs/claude_fifty_objects_20260925.md:169 (editor_flying_camera: 'a count-only control (R4) is not exact'). WORKER_BRIEF_R2.md section 2 lines 47-48 and section 8 lines 196-198.

## source/structures/structures — NEW_RULING_NEEDED

- construct: _sphere_intersects_cluster_portal: an inferred `double t = -plane_distance;` temp feeding the inlined point_from_line3d.
- precedent: Closest: a4ba37a9 (2026-08-27), the units throw_grenade_release reused `double origin_scale` temp (source/units/units.c:11087-11108). Also the 2026-09-20 inferred-construct disclosure rule (5186c5bd).
- completes_object: True
- differences: a4ba37a9 predates the 2026-08-30 house rules, the strip test and the /Od-readout practice, and it was never checked against /Od. Here the later /Od build contradicts the temp: 0x8a07bb negates inline and the frame has no t slot. The same-TU exact sibling _render_debug_fog_planes also passes -x directly. Both strip forms (real t, and the plain call) are residual. The temp also adds a /W3 C4244, and common_constants.md allows double only for API or vararg needs. actor_moving.c:2136's `double t` landed only in a zero-credit fuzzy body (ea507d5b), so it is not an admission. If admitted, an in-source inferred-and-contradicted disclosure comment is required.
- packet: research/fifty_objects_20260925/w/structures/production.patch (git apply --check OK at HEAD) + the disclosure comment
- evidence: git show a4ba37a9 ('a single reused double temp is exact'). research/fifty_objects_20260925/results/wave1/REVIEW__source_structures_structures.md issues 1-4. git show ea507d5b ('ZERO bytes banked').

## source/rasterizer/rasterizer_geometry — NEW_RULING_NEEDED

- construct: _uncompress_int32_to_real_vector3d: a staged int-to-real conversion temp (`real value; value = (real)(long)(compressed<<21); ...`).
- precedent: Closest: the a4ba37a9 units FP-intermediate reused temp. The 2026-09-20 strip-test rule says to escalate, not rule unilaterally.
- completes_object: True
- differences: The strip test fires: plain and /Od spellings are residual, and every staged form is exact. First-party /Od 0x82ea80 contradicts the temp: its frame (0x18 = guards + v + cookie) has no scalar slot, and it does one store per component. a4ba37a9 faced no contrary /Od evidence and predates that rule. If admitted, it needs a disclosure comment and the park must be retired.
- packet: research/fifty_objects_20260925/w/rasterizer_geometry/production.patch (git apply --check OK) or the t7 variant + disclosure comment; retire the instruction-scheduling park
- evidence: research/fifty_objects_20260925/results/wave1/REVIEW__source_rasterizer_rasterizer_geometry.md issues 1-4 ('The one precedent for a scheduling temp is the units FP-intermediate reused temp. It predates the /Od-readout rule and did not face contrary /Od evidence.').

## source/bink/bink_playback — NEW_RULING_NEEDED

- construct: _bink_query_analog_controller_buttons: the q3 else-if split, where START and BACK each assign button_pressed = TRUE in separate arms instead of one ||.
- precedent: laws_w3 A26 as applied in 26601453 (2026-09-16), which admits a duplicated-assignment else-if only when January shows distinct rejection blocks. Lane C owner ruling R9 (2026-09-21, _eat_my_shorts) admits a duplicated branch when January and /Od independently attest it.
- completes_object: True
- differences: 26601453 names this very function as 'the rejected _bink_query_analog_controller_buttons, whose edges share one block': January merges all three je into one mov bl,1 block. R9's /Od condition cannot be met because /Od and HCEA have only bink stubs. The only new evidence is cross-build (Oct 2276betaP/2276P match q3 in 4/4 contexts), a type of evidence no ruling has accepted. Completion also needs the storage packet, which is covered (static bink_pointer_blocks/bitmap/texture/block_count per cachebeta publics) but must be regenerated. Note that _code_001b5850 still breaks house rule 7 (docs/campaign_house_rules.md:13-14 permits a descriptive private-function name); the owner should confirm the name.
- packet: research/fifty_objects_20260925/w/bink_playback/production.patch (git apply --check OK) + storage.patch (FAILS at config/symbols.json:23296; re-cut lines 23299/23300/23301/23304 in place) + csplit regen
- evidence: git show 26601453 (A26 criterion and the explicit rejection of this site). C:/halo-worktrees/claude-lane-c-graphics-ui-20260920/scratch/lane_c/owner_rulings.md:131-138 (R9 'January and /Od independently attest'). docs/object_matching_logs/bink_playback_obj_opus5_next150_n3a_20260915.md:140-146.

## source/units/vehicles — HELD_CLASS

- construct: _update_alien_scout_physics: two load-bearing aggregate 3D->2D view copies plus a leveling point_from_line2d.
- precedent: Brief section 8: declared-name-count compensation is an oracle only. Review BLOCKING 2 found the leveling point_from_line2d is chosen only because it shifts the count.
- completes_object: False
- differences: The leveling point_from_line2d contradicts /Od 0x8f8b4a..0x8f8bba (two component statements; 0x444020 is called only twice, both for alpha_desire), and the claimed January-byte evidence was refuted. That makes it the count oracle used as a landing. The two view copies alone would be the 3ba2eb91 aggregate-copy class, but that does not cure the leveling. The object also needs the held fighter (below) and removal of the invented vehicle_*_product3d_target helpers (vehicles.c:1925-1949, 1382).
- packet: None to apply (held option B in research/fifty_objects_20260925/w/vehicles/production.patch)
- evidence: research/fifty_objects_20260925/results/wave1/REVIEW__source_units_vehicles.md:39-51. WORKER_BRIEF_R2.md:196-198. docs/object_matching_logs/claude_structural_owner_packet_20260923.md:136-140.

## source/units/vehicles — HELD_CLASS

- construct: _update_alien_fighter_physics_new: /Od-attested cross_product2d view casts plus removal of REAL_MATH_EXTERNAL_POINT_FROM_LINE3D. This emits a _cross_product2d COMDAT.
- precedent: Explicit owner hold (2026-09-21, 3ba2eb91): "Land the three; keep the fighter held."
- completes_object: False
- differences: Rulings #1-#3 are otherwise met: the casts are byte-inert and /Od 0x8f6cbf-0x8f6ccd attests them. However, the owner's explicit hold stands, and the new _cross_product2d fails LNK2005 against actor_combat's NODUP copy. That fix is the RESERVED Codex P1 packet. Re-ask under ruling #6 only after Codex reports.
- packet: None to apply (scratch/lane_b/orch_land/vehicles_fighter.c in the Lane B worktree; research/fifty_objects_20260925/w/vehicles/production.patch option A)
- evidence: git show 3ba2eb91 (owner quote). docs/object_matching_logs/claude_lane_b_HANDOFF_20260920.md:92. WORKER_BRIEF_R2.md:163-166 (Codex reservation).

## source/units/vehicles — COVERED_BY_PRECEDENT

- construct: Zero-credit fallback: remove the TU-wide REAL_MATH_EXTERNAL_POINT_FROM_LINE3D define, add the /Od-attested pathfinding point_from_line3d calls (37/2, provider link PASS), and set "static": true on _update_turret_physics.
- precedent: 2026-09-21 owner ruling #1: vehicles is one of the 17 objects that reference _point_from_line3d out of line. Storage follows cachebeta publics, as in canonical 05255584.
- completes_object: False
- differences: None. git apply --check passes for both patches. csplit-only regen, then a full ninja.
- packet: research/fifty_objects_20260925/w/review_vehicles/fallback_hunks12.patch + research/fifty_objects_20260925/w/vehicles/symbols_turret_static.patch
- evidence: docs/object_matching_logs/claude_lane_b_HANDOFF_20260920.md:65-68 and :156-162 (the 17 objects include vehicles). git show 05255584 (wrapper and converter rows made static per cachebeta publics).

## source/interface/interface — NEW_RULING_NEEDED

- construct: Header variant for the now-static interface_get_weapon_hud_index. Variant A removes the interface.h prototype. Variant B keeps the extern prototype over a static definition.
- precedent: Variant A: ruling #4 (genuine owner header, full sweep, ZERO regressions) and the 2026-09-24 canonical rejection of a genuine real_math.h body that lost _rasterizer_frame_statistics_draw. Variant B: no precedent.
- completes_object: True
- differences: Variant A regresses _rasterizer_frame_statistics_draw (4,176 B) through a pure declaration-count tie (+3 dummy externs restore it), which falls in the held declaration-count-loss class. Variant B is an extern-then-static redeclaration (C4211 at /W4; C89 6.1.2.2 undefined). A tree scan of every static function definition in source/*.c against the header prototypes finds ZERO Halo sites, and C4211 appears nowhere in docs. The owner must pick; no genuine count-balancing packet exists (-3 + 2a + 5b = 0 has no solution).
- packet: research/fifty_objects_20260925/w/interface/production.patch + symbols_static.patch (both git apply --check OK) [+ production_variantA_header.patch for A]; retire the parked _interface_get_weapon_hud_index entry in the same change
- evidence: research/fifty_objects_20260925/results/wave1/REVIEW__source_interface_interface.md checks 8-10 and issues. docs/object_matching_logs/claude_object_closure_canonical_reconciliation_20260924.md:43-50 ('the global header edit was rejected'). docs/object_matching_logs/claude_lane_b_HANDOFF_20260920.md:79-80.

## source/interface/ui_widget — HELD_CLASS

- construct: _widget_instance_process_one_event_recursive: split out a new .bss static, dpad_event_times[MAXIMUM_NUMBER_OF_LOCAL_PLAYERS][NUMBER_OF_DPAD_DIRECTIONS], at storage+0x870 (symbols.json static row at file_offset 4541104), together with the structural-lane P3 body.
- precedent: Brief section 8 (the owner-queue item-2 class: descriptive .bss split names stay owner-gated). The 2026-09-15 owner ruling (no descriptive or invented static names in symbols.json). The 2026-09-20 data-lane rule that symbols.json names need authenticated provenance.
- completes_object: False
- differences: None. The name has no first-party source (HCEX _widget_globals has none, /Od has no dpad code, cachebeta has no public, the atlas has no data names). The P3 body is ruling-free zero-credit fuzzy, but it does not close the row without the split. Five tie residuals remain in any case.
- packet: Held: C:/halo-worktrees/claude-structural-20260923/scratch/lane/w/interface__ui_widget/v_split_final.c + splitcfg/
- evidence: docs/object_matching_logs/opus5_250k_house_clean_checkpoint_20260915.md:20-31. docs/object_matching_logs/claude_lane_a_data_section_credit_20260920.md:89-91, 111-116 ('Nothing can attest these names, so they stay'). WORKER_BRIEF_R2.md:191-195.

## source/rasterizer/xbox/rasterizer_xbox_lights — HELD_CLASS

- construct: (1) cand_g2_diagnostic_R15: the vector_from_points3d return pointer plus a hand-written 3-statement dot_product3d brightness accumulation. (2) cand_g4 with rule 6's strict-exact-caller condition waived. Either option also needs the consumer-local global_window_parameters view replaced.
- precedent: The R15 rejection (hand-expanded dot accumulation), backed by the refusal of manual expansion where a real call exists (AB table #4, RB3). Option (2) is the brief section 8 P1 class (a new header COMDAT from a non-exact caller).
- completes_object: False
- differences: None. (1) hand-expands dot_product3d, which is exactly the R15/AB#4 construct. (2) asks to waive the strict-exact-caller condition, which is the P1 class. Both also keep a consumer-local global_window_parameters view with opaque pads, the same defect that keeps canonical dynavobgeom unadmitted. The genuine rasterizer.h:250 type has not been measured on g2/g4.
- packet: None to apply (research/fifty_objects_20260925/w/rasterizer_xbox_lights/cand_g2_diagnostic_R15.vs_tree.diff, g4_owner_gated.patch held)
- evidence: research/fifty_objects_20260925/w/rasterizer_xbox_lights/LEDGER.md:13-15 (R14/R15/R16). docs/object_matching_logs/claude_lanes_AB_HANDOFF_TO_LANE_C_20260921.md:320. docs/object_matching_logs/claude_lane_b_rejected_hypotheses_20260920.md:57-62. docs/object_matching_logs/claude_fifty_objects_canonical_reconciliation.md:36-41 (dynavobgeom hold).

## source/effects/weather_particle_systems — NEW_RULING_NEEDED

- construct: _weather_particle_system_render: an /Od-attested dead scalar with an invented name, `short unused_count = 0;` (/Od init-only slot [ebp-0x24] at 0x57fbb4; Lane D class I).
- precedent: Lane C owner ruling R8 (2026-09-21) admitted /Od dead arrays because the /Od RTC descriptors NAME them ('rule 8 is not weakened generally'). The editor_flying_camera dead-pair admission required the same RTC name, zero anchor drift and a non-exact count-only control.
- completes_object: False
- differences: This is a scalar with no RTC descriptor, so the name 'unused_count' is invented, and no count-only control was measured. Lane D class I has had no ruling since 2026-09-22. Completion also needs the _real_local_random provider repair: a real_math.h body change inside the RESERVED Codex real_math unit, which needs a full-board re-sweep because tree D once broke units.
- packet: research/fifty_objects_20260925/w/weather_particle_systems/production.patch (git apply --check OK; held)
- evidence: C:/halo-worktrees/claude-lane-c-graphics-ui-20260920/scratch/lane_c/owner_rulings.md:84-100. docs/object_matching_logs/claude_lane_d_refresh_HANDOFF_20260922.md:142. research/fifty_objects_20260925/results/wave1/REVIEW__source_effects_weather_particle_systems.md issues.

## source/effects/weather_particle_systems — COVERED_BY_PRECEDENT

- construct: Zero-credit storage fix: drop `static` from one_over_char_max (cachebeta public 0x25AAFC).
- precedent: Storage follows cachebeta publics (the standing rule behind every storage packet, e.g. 05255584).
- completes_object: False
- differences: None. The review measured render unchanged at 19/20 and 30/30 symbols.
- packet: No packet file: a one-token edit in source/effects/weather_particle_systems.c (included in research/fifty_objects_20260925/w/weather_particle_systems/production.patch)
- evidence: research/fifty_objects_20260925/results/wave1/REVIEW__source_effects_weather_particle_systems.md check (10). scratch/tools/pdb_storage.py.

## source/physics/breakable_surfaces — HELD_CLASS

- construct: B2: the TU-local regrouped helper _breakable_surface_plane_distance, a 32 B static surplus that January lacks (breakable_surfaces.c:42).
- precedent: Brief rule 6: 'never hand-expand or hand-copy ... any header helper'. RB3: the owner refuses manual expansion of a helper whose real call exists.
- completes_object: False
- differences: The cited bsp3d precedent (bsp3d.c:82-94 TU-local plane-distance helpers, 772e3d24 of 2026-08-20) predates the 2026-08-30 house rules, and the owner never answered the static-surplus policy question. /Od 0x7a0ca0 calls the genuine plane3d_distance_to_point at 4 sites, so the local copy contradicts first-party evidence. B1 is held separately.
- packet: None to apply (research/fifty_objects_20260925/w/breakable_surfaces_audit2/patches_owner_gated held)
- evidence: WORKER_BRIEF_R2.md:40-43. docs/object_matching_logs/claude_lane_b_rejected_hypotheses_20260920.md:57-62. research/fifty_objects_20260925/results/wave1/audit_physics_rast_physics_breakable_surfaces.md:59,82. git log -S bsp3d_plane_distance_to_point (772e3d24).

## source/physics/breakable_surfaces — HELD_CLASS

- construct: B1: the /Od-attested named-local body of real_local_random in real_math.h. It adds one declared name to every includer, which loses _rasterizer_frame_statistics_draw and _bitmap_copy.
- precedent: Declaration-count-loss class: ruling #4 requires zero regressions, and canonical rejected the identical frame_statistics loss on 2026-09-24. real_math is RESERVED for Codex.
- completes_object: False
- differences: None: the same victim as the rejected distance3d header body.
- packet: None
- evidence: docs/object_matching_logs/claude_object_closure_canonical_reconciliation_20260924.md:43-50. docs/object_matching_logs/claude_lane_b_HANDOFF_20260920.md:79-80. WORKER_BRIEF_R2.md:163-166.

## source/physics/breakable_surfaces — COVERED_BY_PRECEDENT

- construct: Zero-regression sub-packet: call the genuine bsp3d_get_plane_from_designator instead of the hand copy; drop the per-TU '/Ow','/QIfist' override (config.json:520); set "static": true on _breakable_surface_effect and _globals.
- precedent: Rule 9 and rule 13 (genuine owner helpers, no hand copies). Rule 22 (/QIfist only where the evidence requires it). Storage follows cachebeta publics.
- completes_object: False
- differences: None: 12/12 stays exact, and the six helper COMDATs become identical to January's selected copies. symbols.json.patch fails git apply --check at config/symbols.json:23111 and must be re-cut in place. The .c and config patches apply.
- packet: research/fifty_objects_20260925/w/breakable_surfaces_audit2/patches/{breakable_surfaces.c.patch, config.json.patch, symbols.json.patch (regenerate)} + csplit regen
- evidence: docs/campaign_house_rules.md:12-13, 18, 35-41 (rules 6, 9, 13 and the rule-6 exception text), rule 22 at :39-44. research/fifty_objects_20260925/results/wave1/audit_physics_rast_physics_breakable_surfaces.md.

## source/rasterizer/xbox/rasterizer_xbox — HELD_CLASS

- construct: Owner-queue item 2: a .bss split into 23 `= 0` file statics with 9 non-first-party names. The new HCEX target enum evidence (_rasterizer_target_z) would let names #8/#9 become global_d3d_{texture,surface}_z.
- precedent: Brief section 8 (the rasterizer_xbox descriptive-name split stays owner-gated unless first-party evidence removes the ruling). 2026-09-15 owner ruling. 2026-09-20 authenticated-provenance rule.
- completes_object: True
- differences: The HCEX enum attests only enum member names, not variable names. It improves #8/#9 from invented to convention stems, but node_matrix_constants stays invented and #2-#7 stay convention stems. The `= 0` initialisers alone are covered by a595bbc2. rasterizer_xbox_shadows' disclosed descriptive gap statics were a lane judgement call, not an owner ruling. A yes completes the object (95/95, audit PASS, provider_link 15/15).
- packet: research/fifty_objects_20260925/w/owner_queue/names_xbox_hs_scen/patches/1_rasterizer_xbox/01-05 (held)
- evidence: docs/object_matching_logs/claude_fifty_objects_20260925_owner_queue.md:98-147 (evidence against: 2026-09-15 ruling, 2026-09-24 path_obstacle_avoidance hold). WORKER_BRIEF_R2.md:191-195.

## source/rasterizer/xbox/rasterizer_xbox_debug — COVERED_BY_PRECEDENT

- construct: Independent storage patch: 8 XDK wrapper rows (D3DDevice_SetRenderState, IDirect3DDevice8_* wrappers) set to "static": true.
- precedent: The canonical R2 batch 1b (05255584) made the XDK wrapper rows static per cachebeta publics for hardware_geometry. It is the same class.
- completes_object: True
- differences: None. The unit is already labelled Matching (config index 126). This patch removes its object_audit FAIL(8), so the label becomes truthful. csplit radius is rasterizer_xbox_debug.obj only; the gate stays 18/18. git apply --check passes.
- packet: research/fifty_objects_20260925/w/owner_queue/names_xbox_hs_scen/patches/independent/rasterizer_xbox_debug_static_wrappers_symbols.json.patch + csplit regen
- evidence: git show 05255584 ('13 wrapper rows go static'). research/fifty_objects_20260925/w/owner_queue/names_xbox_hs_scen/patches/APPLY_ORDER.txt:30-32.

## source/render/render_debug — HELD_CLASS

- construct: Owner-queue item 5 (d): the invented render_debug_globals_definition aggregate. D0 keeps it with its opaque_after_* pad members. D1 uses seven descriptive `= 0` file statics.
- precedent: Brief section 8 (render_debug D1 descriptive names stay owner-gated). Brief section 9: pad members in a TU-private struct signal an invented aggregate, which blocks admission (D0). The 2026-09-15 and 2026-09-20 name-provenance rules.
- completes_object: True
- differences: None. No first-party names exist (cachebeta, HCEX, the 2001 maps and the atlas all lack them). The /Od build and January's layout both contradict the D0 aggregate. The `= 0` layout proof matches a595bbc2, but that precedent had authentic names. Either answer, with 05/06/08, completes the object.
- packet: research/fifty_objects_20260925/w/owner_queue/objlights_render_debug/patches/08_IF_D_RULED_render_debug_status_matching.patch (+ lab/D1_OPTION_render_debug_separate_statics.patch and lab/D1_OPTION_symbols_json.patch for D1)
- evidence: research/fifty_objects_20260925/w/owner_queue/objlights_render_debug/RULING.md Q2d. WORKER_BRIEF_R2.md:191-195, 216-219.

## source/render/render_debug — COVERED_BY_PRECEDENT

- construct: Zero-credit 05 and 06. 05: genuine point_from_line3d calls (/Od 0x84a920/0x84a510/0x848640), the genuine vehicle_datum plus accessor macro, and removal of the dead REAL_MATH_EXTERNAL defines. 06: tentative `boolean debug_*` definitions.
- precedent: 05: the folded-inline COMDAT exception (docs/campaign_house_rules.md:29-37, 3c74fa36; ruling #2): the new _point_from_line3d is byte-identical, the callers are strictly exact at 36/36, and the provider link passes. 06: the objects.c tentative-definition precedent (Matching), with per-symbol linker_common evidence.
- completes_object: False
- differences: 06 rests on per-symbol COMMON, /Od and 2001-map evidence rather than pool adjacency, but the 2026-09-22 canonical reconciliation excluded an adjacency-only COMMON wave, so the integrator must confirm the evidence standard. git apply --check passes for 05 and 06.
- packet: research/fifty_objects_20260925/w/owner_queue/objlights_render_debug/patches/05_render_debug_genuine_helpers_and_vehicle.patch, 06_OPTIONAL_E_render_debug_tentative_debug_toggles.patch (apply one at a time)
- evidence: docs/campaign_house_rules.md:29-37. docs/object_matching_logs/claude_lane_b_HANDOFF_20260920.md:69-75. research/fifty_objects_20260925/w/owner_queue/objlights_render_debug/RULING.md Q2e and APPLY_ORDER.txt.

## source/bitmaps/bitmap_drawing — HELD_CLASS

- construct: An unreferenced 16-byte initialised .data prefix (ff x10, 00 x6) before the public translation_table. It needs a name and a type.
- precedent: AB owner-rulings table #14 (data lane): 'An unreferenced global January defines: admitted only with verified owner, type, value and linkage from January's own cachebeta.pdb; unnamed globals HELD'.
- completes_object: True
- differences: None. The owner is proven (section contributions), but the type is unrecoverable (char[10], short[5] and long[3] all give the same layout), and there is no name in HCEX, the atlas, the Sept-2001 maps or /Od. A yes on a named and typed global would complete the object (7/7 exact; the data gap is the whole 2,644 B section).
- packet: None (no admissible packet; do not invent an extent-filling global)
- evidence: docs/object_matching_logs/claude_lanes_AB_HANDOFF_TO_LANE_C_20260921.md:330. docs/object_matching_logs/bitmap_drawing_prefix_evidence_20260924.md.

## source/objects/object_lights — HELD_CLASS

- construct: Q1: admit the surplus _object_get_bounding_sphere COMDAT, whose two __FILE__ DIR32 targets point to object_lights' own c:\halo\source\objects\objects.h literal instead of January's selected ..\objects\objects.h.
- precedent: Brief section 8 lists the 'object_lights __FILE__ surplus' as owner-gated. The rule-6 exception text requires the emitted copy to be byte-identical to January's selected copy.
- completes_object: True
- differences: The rule-6 exception condition is not met: 7 of 9 relocation targets are identical and 2 __FILE__ targets differ. The packet asks for a new exception class (an authentic non-identical surplus, proven by January's single orphan literal). A yes completes the object (43/43, audit PASS).
- packet: research/fifty_objects_20260925/w/owner_queue/objlights_render_debug/patches/01, 02, 03_YES, 04_YES (all git apply --check OK)
- evidence: docs/campaign_house_rules.md:29-37. WORKER_BRIEF_R2.md:191-195. research/fifty_objects_20260925/w/owner_queue/objlights_render_debug/RULING.md Q1.

## source/objects/object_lights — COVERED_BY_PRECEDENT

- construct: Ruling-independent parts: 01+02 (should_render_lights, light_attenuation and cluster_get_first/next_light made static per cachebeta; the /Od-attested should_render_lights() calls) and 03_NO_ALT (_lights_section single-section data entry with allow_incomplete_unit, +1,541 data).
- precedent: Storage follows cachebeta publics (05255584). The leaf_map allow_incomplete_unit single-section data entry (config/semantic_data_matches.json, ae12a3c1).
- completes_object: False
- differences: None. 01 and 02 must land together with a csplit regen. Use 03_NO_ALT only if Q1 is refused or undecided (03_YES replaces it on a yes).
- packet: research/fifty_objects_20260925/w/owner_queue/objlights_render_debug/patches/01_object_lights_static_helpers.patch, 02_object_lights_symbols_json.patch, 03_NO_ALT_object_lights_semantic_data_entry_incomplete.patch (all git apply --check OK)
- evidence: research/fifty_objects_20260925/w/owner_queue/objlights_render_debug/APPLY_ORDER.txt ITEM 1. config/semantic_data_matches.json leaf_map entry.

## source/game/game_engine — HELD_CLASS

- construct: Admission pick. Q1: the alias versus the /Od view copy `positions[count] = *(real_point2d *)&global_goal[goal_index].position;`. Q2: the one-consumer focused headers. Q3: the anonymous debug_player_color aggregate with word pad members, plus the descriptive .bss names _game_engine_teleport_message_ticks and _game_engine_teleport_flash_fade_function.
- precedent: Q3: brief section 9 (pad members signal an invented aggregate) and the descriptive-name hold. Brief section 8 lists the game_engine pick as owner-gated. Q1-B itself IS the 3ba2eb91 action_obey aggregate-copy class, and Q2 is governed by ruling #4.
- completes_object: True
- differences: Q1-B meets every 3ba2eb91 condition: /Od 0x5a20e0 is an 8-byte integer mov pair with no pointer slot, the layout is a compatible prefix, the function is 180/180 strict exact, and the member-wise spelling is residual with the same non-strip-testable caveat. So Q1 can be settled by precedent in favour of B. Q2: ruling #4 selects variant_no_single_consumer_headers, but hud_nav_points.h is still an invented header (the genuine hud.h move breaks _populate_statistic_buffer by count). Q3 keeps the object held: HCEX records the flash values as static locals red/green/blue/duration of game_engine_update_teleporter, which contradicts one aggregate. A yes on Q3 (and on the hud_nav_points.h header) would complete the object.
- packet: research/fifty_objects_20260925/w/owner_queue/pick_game_engine/patches/optionB + variant_no_single_consumer_headers (re-sweep against current game_engine.h consumers)
- evidence: git show 3ba2eb91 (action_obey caveat). source/ai/action_obey.c:1259. research/fifty_objects_20260925/w/owner_queue/pick_game_engine/RULING.md Q1-Q3. docs/object_matching_logs/claude_lane_b_HANDOFF_20260920.md:79-80.

## source/items/weapons — HELD_CLASS

- construct: (A) _weapon_place keeps its authentic long return and is bound to the void datum_place slot with a BUG comment. (B) A descriptive profile_section static name, _weapon_update_section.
- precedent: Brief section 2 ('Reproducing an authentic January bug needs ... owner approval - report, don't land'); Lane D class F (return ABI), which is unruled. For (B), the 2026-09-15 no-descriptive-static-names ruling.
- completes_object: True
- differences: None: the round-1 reviewer placed a policy hold on both. The new evidence (long in two HCEA/HCEX PDBs, the cast byte-inert, the units regression gone) justifies a re-ask, not a self-admission. A yes on both completes the object (p1+p2+p3 atomically with a csplit regen).
- packet: research/fifty_objects_20260925/w/weapons/p1_weapons_c.patch, p2_owner_header.patch, p3_symbols_json.patch (all git apply --check OK; held)
- evidence: research/fifty_objects_20260925/results/wave1/REVIEW__source_items_weapons.md issues A-D. docs/object_matching_logs/claude_lane_d_refresh_HANDOFF_20260922.md:139 (class F).

## source/items/weapons — COVERED_BY_PRECEDENT

- construct: Zero-credit storage: "static": true on 11 symbols.json rows, plus static on the weapon_magazine_finish_reload prototype (weapons.c:449) and definition (:1382).
- precedent: Storage follows cachebeta publics (05255584). The first-party Sept-2001 cachebeta.map 'Static symbols' lists these as static functions.
- completes_object: False
- differences: None. The result is 78/79 with object_audit 97/0 (only _weapon_place fails). symbols_static.diff is a plain line diff, not a git patch; its lines 4013-4054 still match HEAD.
- packet: research/fifty_objects_20260925/w/weapons/symbols_static.diff (apply by line surgery) + `static` at source/items/weapons.c:449 and :1382, then a csplit regen
- evidence: research/fifty_objects_20260925/results/wave1/REVIEW__source_items_weapons.md 'Admissible now without a ruling'. TRIAGE__source_items_weapons.md why_new (Sept-2001 map).

## source/networking/network_connection — HELD_CLASS

- construct: (1) _network_server_close_client_connection: the authentic N+1 capacity loop bound, which reads one past the list. (2) _network_connection_connect: a redundant unreliable-arm `success = TRUE;`.
- precedent: (1) Brief section 8 (every January-bug candidate: overruns) and structural packet s4. (2) AB table #9 (semantically inert steering rejected), and structural packet s6 recommends rejecting it.
- completes_object: True
- differences: None. (1) is a reachable out-of-bounds read, the class Lane C R10/R14(c,d) held. (2) is attested only by January's block layout; the 2020 build has no such path, and it fails the strip test. A yes on both completes the object (23/23, audit PASS).
- packet: Held: research/fifty_objects_20260925/w/network_connection/production.patch + owner_gated_close_client.patch
- evidence: docs/object_matching_logs/claude_structural_owner_packet_20260923.md:120, 146-151. docs/object_matching_logs/claude_lanes_AB_HANDOFF_TO_LANE_C_20260921.md:325-326.

## source/text/draw_string — HELD_CLASS

- construct: _bitmap_draw_string: January's inverted `if (!bounds)` test followed by bounds-> reads (a NULL dereference), with a BUG comment.
- precedent: Brief section 8 (January-bug class: NULL dereference). Lane C R10: reachable UB is held even when exact.
- completes_object: True
- differences: None. /Od 0x8b2090 corroborates the inverted test, but reachable UB stays held. A yes plus the ownership packet completes the object (28/28).
- packet: Held: research/fifty_objects_20260925/w/draw_string/bug_body_after_ownership.patch (git apply --check OK)
- evidence: WORKER_BRIEF_R2.md:185-190. C:/halo-worktrees/claude-lane-c-graphics-ui-20260920/scratch/lane_c/owner_rulings.md:140-145 (R10).

## source/text/draw_string — COVERED_BY_PRECEDENT

- construct: Zero-credit ownership packet: split the invented _draw_string_globals into four HCEX-attested file statics with `= { 0 }` ordering.
- precedent: a595bbc2 (models: `= { 0 }` file statics restoring January's .bss order, with authentic names). R2 batch 1a 7979cf8f (first-party names).
- completes_object: False
- differences: None: the names are HCEX-attested. symbols_json.patch fails git apply --check at config/symbols.json:23268 and must be re-cut in place (line 23279 is still _draw_string_globals). ownership.patch applies.
- packet: research/fifty_objects_20260925/w/draw_string/ownership.patch + symbols_json.patch (regenerate) + csplit regen
- evidence: docs/object_matching_logs/models_obj_explicit_zero_bss_owner_20260924.md. git show a595bbc2 and 7979cf8f.

## source/bungie_net/network/transport_endpoint_set_winsock — HELD_CLASS

- construct: (1) _poll_endpoint_set: a staging boolean `select_failed = select_result < 0 || select_result == SOCKET_ERROR` that no /Od body attests. (2) _transport_initialize: explicit zero field stores. (3) net_startup_debug static reached only through an invented call.
- precedent: (1) AB table #9 (a flag whose only effect is bytes is rejected) and structural packet s7 (held). (3) The round-1 review rejected the invented call as a fake dependency. (2) has no precedent.
- completes_object: False
- differences: (2) is attested in January bytes and 3 builds but is still one byte short, so it would need a NEW ruling and would close nothing. compare_entries is an EAX/ECX tie (RETIRE). The object cannot complete even if every item is a yes.
- packet: Held: research/fifty_objects_20260925/w/transport_endpoint_set_winsock/owner_gated.patch. storage_only.patch passes git apply --check (zero-credit storage).
- evidence: docs/object_matching_logs/claude_structural_owner_packet_20260923.md:163-164. docs/object_matching_logs/claude_lanes_AB_HANDOFF_TO_LANE_C_20260921.md:325.

## source/bungie_net/network/transport_endpoint_winsock — HELD_CLASS

- construct: _connect_endpoint_process@4: January's uninitialised `struct thread_reference *thread;` is read on take_mutex failure, with a BUG comment.
- precedent: Brief section 8 (January-bug class: uninitialised reads). Structural packet s4 row, unruled.
- completes_object: True
- differences: None. The failure path is reachable, so the Lane C R13/R14(a) 'provably irrelevant' admissions do not apply. A yes completes the object (30/30).
- packet: Held: cand_final body. research/fifty_objects_20260925/w/transport_endpoint_winsock/production.patch no longer applies (it fails at transport_endpoint_winsock.c:340 because the storage part already landed); regenerate a bug-body-only hunk from cand_final.vs_tree.diff.
- evidence: docs/object_matching_logs/claude_structural_owner_packet_20260923.md:111. WORKER_BRIEF_R2.md:185-190.

## source/game/players — HELD_CLASS

- construct: _player_examine_nearby_device: a decoration-only parenthesis on the fast_vector_intersects_sphere radius, now proposed as a same-name callee wrapper macro in real_math.h.
- precedent: RB2 explicit owner refusal (2026-09-21) of this very function.
- completes_object: False
- differences: None, and the wrapper form is worse. It edits the RESERVED real_math.h and real_math.c, and it deletes the owner-admitted LINE_OF_FIRE_PILL_SPHERE_RADIUS accessor from ai.c, reverting 73c05198. No first-party name exists. _player_teleport_internal is RETIRE, so the object cannot complete.
- packet: None (research/fifty_objects_20260925/w/players/owner_gated_wrapper_macro.patch must not land)
- evidence: docs/object_matching_logs/claude_lane_b_rejected_hypotheses_20260920.md:45-56. research/fifty_objects_20260925/w/players/owner_gated_wrapper_macro.patch (removes ai.c:316-330).

## source/physics/collisions — HELD_CLASS

- construct: _collision_move_point: the genuine real_math.h __inline valid_real_plane3d body plus the /Od function-scope `position`. It loses _rasterizer_frame_statistics_draw (4,176) and _bitmap_copy.
- precedent: Declaration-count-loss class: ruling #4 requires zero regressions, and canonical rejected the identical frame_statistics loss on 2026-09-24 (the distance3d header body). real_math is RESERVED for Codex.
- completes_object: True
- differences: None: the same victim function, and no count from -6 to +30 restores it. The rejected focused-header/block-scope packet must not be re-offered. The collisions.c bare parenthesis (now line 1581) is itself under RB2 review.
- packet: Held: research/fifty_objects_20260925/w/collisions/production.patch (git apply --check OK)
- evidence: docs/object_matching_logs/claude_object_closure_canonical_reconciliation_20260924.md:43-50. research/fifty_objects_20260925/results/wave1/REVIEW__source_physics_collisions.md.

## cross-cutting (real_math.h / _rasterizer_frame_statistics_draw) — HELD_CLASS

- construct: The owner question 'accept losing _rasterizer_frame_statistics_draw (exact only at today's declared-name count) to land genuine real_math.h bodies' (collisions valid_real_plane3d, breakable B1 real_local_random).
- precedent: Canonical 2026-09-24: the genuine distance3d body was rejected because the sweep lost this exact function. Ruling #4 requires zero regressions. Layer 2 (931ed8dc) was accepted only because it preserved this function's exactness.
- completes_object: False
- differences: None. Canonical has already declined the same trade. It needs the owner to reverse that stance or a funded frame_statistics genuine-declaration attack, sequenced after the Codex real_math report.
- packet: None
- evidence: docs/object_matching_logs/claude_object_closure_canonical_reconciliation_20260924.md:43-50. docs/object_matching_logs/claude_fifty_objects_20260925.md:81-130 (Layer 2 strip test R6 V1).

## source/physics/collision_bsp — HELD_CLASS

- construct: _collision_surface_test_sphere: a unit-local SSE __asm fast_distance_squared3d macro.
- precedent: 2026-09-15 owner ruling: the collision_bsp SSE __asm macro stays parked.
- completes_object: False
- differences: None. Two bsp3d parity residuals (RETIRE) and the RESERVED Codex P1 _cross_product2d dependency also remain.
- packet: None (research/fifty_objects_20260925/w/collision_bsp/owner_gated_asm.vs_tree.diff held)
- evidence: docs/object_matching_logs/opus5_250k_house_clean_checkpoint_20260915.md:22-24.

## source/math/geometry — NEW_RULING_NEEDED

- construct: (a) polygon2d: the /Od 0x6bf170 plane2d_distance_to_point body `dot_product2d((real_vector2d *)point, &plane->n) - plane->d` as a real_math.h header view cast. (b) _convex_hull3d_expand: pointers formed before the range checks.
- precedent: (a) Ruling #3 (per-site rule-24 admission) and e942f338 (realcmp consolidated in real_math.h). (b) The January-bug/UB class (structural packet s4), which is HELD.
- completes_object: False
- differences: (a) The cast would live in a shared header body, so every consumer instantiates it. real_math.h is RESERVED for Codex, and the full-board sweep has not been measured at HEAD. Structural packet s3 decision (b) ('admit the geometry cast') is unanswered, so ruling #3's strict-exactness and zero-regression conditions are not yet shown. (b) is HELD_CLASS (UB). The object also needs the Codex P1 and a decals _plane2d_from_points provider repair (decals is closed this lane), so it is not completable.
- packet: Held: C:/halo-worktrees/claude-lane-d-refresh-20260922/scratch/lane_d/w/math__geometry (B3, h3e_c1)
- evidence: docs/object_matching_logs/claude_structural_owner_packet_20260923.md:76-101 and :117. git show e942f338. docs/object_matching_logs/claude_lane_b_HANDOFF_20260920.md:76-78.

## source/tool/connected_geometry — HELD_CLASS

- construct: (a) _connected_geometry_find_or_add_edge: an uninitialised `boolean direction;` (/Od _RTC_UninitUse("direction")).
- precedent: Brief section 8 (January-bug class: uninitialised reads). Structural packet s4 (the RTC-attested class admission is unanswered).
- completes_object: False
- differences: A closer owner precedent exists that could justify a re-ask. Lane C R14(a) (2026-09-21) admitted an uninitialised `long best_axis;` because its unassigned path is unreachable for finite inputs. Here the read happens only when edges.count < 0, which a valid dynamic array never has. The brief still holds the class, so it is not self-admissible. triangle_coplanar is a RETIRE tie, so the object cannot complete.
- packet: Held: C:/halo-worktrees/claude-lane-d-refresh-20260922/scratch/lane_d/w/tool__connected_geometry/candidate_ruling_direction.c
- evidence: docs/object_matching_logs/claude_lane_d_refresh_manifest_20260922.json (find_or_add_edge blocker text). C:/halo-worktrees/claude-lane-c-graphics-ui-20260920/scratch/lane_c/owner_rulings.md:199-215 (R14 a/b admitted). WORKER_BRIEF_R2.md:185-190.

## source/tool/connected_geometry — NEW_RULING_NEEDED

- construct: (b) _connected_geometry_find_or_add_vertex: a TU-local realcmp_epsilon(a,b,epsilon) macro copied verbatim from sound_dsound_xbox.c:61.
- precedent: e942f338 (realcmp owned by real_math.h because January assert strings attest it). The 5186c5bd inferred TU-private macro rule.
- completes_object: False
- differences: realcmp_epsilon is not January-string-attested: it is itself an inferred 2026-09-13 macro (2885ba82), and e942f338 left it untouched. A second TU-local copy runs against e942f338's consolidation. Placing it in real_math.h instead would touch the RESERVED unit. The only load-bearing element is the hygienic parenthesisation (plain fabs fails), which puts it close to the RB2/first-party-name condition.
- packet: Held: C:/halo-worktrees/claude-lane-d-refresh-20260922/scratch/lane_d/w/tool__connected_geometry/cand_e15_macro.c
- evidence: git show e942f338 ('realcmp_epsilon is untouched'). source/sound/sound_dsound_xbox.c:61. docs/object_matching_logs/claude_structural_owner_packet_20260923.md:84-86.

## source/rasterizer/xbox/rasterizer_xbox_profile — HELD_CLASS

- construct: B1: de-aggregate the four invented TU-private structs (pad02/pad06/pad12/pad16/reserved06) into loose statics with descriptive .bss names, plus an unattested 4-byte .bss object at +0x45c.
- precedent: 2026-09-15 owner ruling (no descriptive or invented static names). Brief section 9 (pad members signal an invented aggregate). AB #14 (unnamed globals HELD).
- completes_object: False
- differences: None. HCEX has no profile TU statics.
- packet: None for B1 (research/fifty_objects_20260925/w/rasterizer_xbox_profile LEDGER P3)
- evidence: docs/object_matching_logs/opus5_250k_house_clean_checkpoint_20260915.md:20-31. docs/object_matching_logs/claude_lanes_AB_HANDOFF_TO_LANE_C_20260921.md:330.

## source/rasterizer/xbox/rasterizer_xbox_profile — HELD_CLASS

- construct: B2: the /Od-attested NONE-branch '### PROFILE: %s -- tell Bernie!' insufficient-varargs defect (/Od 0x8004c0), with a BUG marker.
- precedent: Brief section 2 and section 8 (January bug: owner approval required, report and don't land).
- completes_object: False
- differences: The only canonical landing of an insufficient-vararg bug, ai_script _ai_scripting_migrate_and_speak (5d3444f7, re-disclosed in 8fd76228), is recorded as 'owner ratification required', and no ratification exists in docs. So it is not a covering precedent.
- packet: Held: research/fifty_objects_20260925/w/rasterizer_xbox_profile/patches (08 BUG-marker patch per triage)
- evidence: docs/object_matching_logs/claude_lane_a_ai_core_HANDOFF_20260920.md:110-137, :328. docs/object_matching_logs/claude_lane_a_ai_core_admission_manifest_20260920.json:78 (owner_ratification_required true).

## source/rasterizer/xbox/rasterizer_xbox_profile — NEW_RULING_NEEDED

- construct: B3: confirm the three load-bearing volatile qualifiers already in canonical (rasterizer_xbox_profile.c:184, :215, :284). Stripping them gives 12/16.
- precedent: 5e81dabf (2026-09-23): volatile was admitted for cache_files_decompress_windows only because of HCEX/HCEA type-record support.
- completes_object: False
- differences: No HCEX type record exists for the profile statics (review3 V10), so the admitting condition fails. The only support is the semantic comment about asynchronous D3D callback writes. The ready zero-credit window/API owner patches 01-06 apply cleanly (rule 9 / ruling #4; they need a full sweep of the shared-header consumers). 07 fails at object_admission_rejections.json:17.
- packet: research/fifty_objects_20260925/w/rasterizer_xbox_profile/patches/01-06 (zero-credit, git apply --check OK); 07 must be regenerated
- evidence: docs/object_matching_logs/fourth_five_objects_admission_20260923.md ('The cache volatile qualifiers have independent HCEX/HCEA type-record support, not a register-forcing guess'). docs/object_matching_logs/claude_structural_owner_packet_20260923.md:63-74.

## source/interface/hud_draw — HELD_CLASS

- construct: _fast_ftol_C: a tightly bracketed #pragma optimize("", off)/on around the helper.
- precedent: Explicit owner refusal, Lane C R6 (2026-09-21): "Hold it; no pragma. Preserve _hud_draw_meter exactness, record the ready 56-byte candidate, and audit the existing ai.c pragma separately." Also AB #8 ('No #pragma steering, ever') and brief section 2 (the held 'hud_draw optimizer pragma').
- completes_object: True
- differences: The triage's 'ai.c precedent is admitted' is incorrect. ai.c:2902/3047 is an inherited landing (1a6da50d, 2026-09-09) that Lane A disclosed as not admissible if proposed fresh (R9), and the owner ordered it audited separately. The new non-16-byte COMDAT census is supporting evidence for a re-ask, not a ruling. Separately, the _fast_ftol candidate-only-comdat-owner rejection is still in config/object_admission_rejections.json (around line 21); triage reports it stale. A pragma yes would complete the object.
- packet: Held: C:/halo-worktrees/claude-lane-c-graphics-ui-20260920/scratch/wave1/hud_draw.c (or opus5 150k_w1 v2.c)
- evidence: C:/halo-worktrees/claude-lane-c-graphics-ui-20260920/scratch/lane_c/owner_rulings.md:19-50. docs/object_matching_logs/claude_lane_c_graphics_ui_HANDOFF_20260920.md:216-224. docs/object_matching_logs/claude_lane_a_ai_core_rejected_hypotheses_20260920.md:134-147. docs/object_matching_logs/claude_lanes_AB_HANDOFF_TO_LANE_C_20260921.md:324.

## source/ai/path_obstacles — HELD_CLASS

- construct: _obstacles_test_circle: a parenthesised field operand `radius + (disc->radius)` or a TU-private accessor ((disc)->radius).
- precedent: Brief section 2 names 'path_obstacles bare-parenthesis match' as a held item never to retry. RB2 and ruling #5.
- completes_object: True
- differences: None. The 73c05198 named-accessor admission requires a first-party-attested name, and none exists (/Od, the atlas and HCEX have no disc-radius accessor). A yes plus the storage fix below would complete the object. Separately covered at zero credit: `static` on _global_obstacle_colors plus "static": true at symbols.json line 22351 (in place) and a csplit-only regen (absent from cachebeta publics).
- packet: None for the paren. Storage fix has no packet file (a triage-described edit to source/ai/path_obstacles.c and config/symbols.json:22351).
- evidence: WORKER_BRIEF_R2.md:59-61. docs/object_matching_logs/claude_lane_b_rejected_hypotheses_20260920.md:45-56. docs/object_matching_logs/claude_lane_a_adjudication_paren_accessor_20260920.md:126-135.

