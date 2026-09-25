# precedent_audit LEDGER (started Thu Sep 24 09:29:12 PDT 2026)

## Precedent catalogue (verified at 434f0151 / 05255584)
- PR-A1 2026-09-20 squaring macro: 5186c5bd; source/ai/actor_perception.c:386-432; ledger claude_lane_a_waveB_frame_and_nearexact_20260920.md:125-130
  quote: "Land _actor_perception_refresh (+2,416) with a TU-private, descriptively named inferred macro and documented evidence."
  conditions: TU-private #define, sibling-named, inferred-disclosure comment, strip table, strict exact. Second site (actor_emotion_unopposable_retreat) uses it "by owner ruling (2026-09-20) because it is the admitted spelling of this square".
- PR-A2 ai_debug_attractor_label_height: edd8b386 (2026-09-20 17:27), ai_debug.c:48-63; landed "under the owner's standing rule" (inferred TU-private named construct). Predates the 18:13 accessor conditions and RB2.
- PR-A3 named accessor ruling 2026-09-20 18:13: 73c05198; claude_lane_a_adjudication_paren_accessor_20260920.md "RULING" section. Conditions: conventional hygienic paren, idempotent; every undecorated alternative measured failing; NAME must carry meaning FIRST-PARTY sources attest.
- PR-B 2026-09-21 Lane B rulings: claude_lane_b_HANDOFF_20260920.md:63-86 (#1 pfl 17 objects, #2 all-inlined COMDAT class 5 conditions, #3 rule-24 per-site, #4 genuine owner header/no one-consumer header, #5 decoration-only paren = steering, collisions.c:1572 to be revisited, #6 side effects). RB2 refusal: claude_lane_b_rejected_hypotheses_20260920.md:45-56.
  NOTE numbering: memory file numbers differ (paren=#4, rule24=#6); round-1 docs mix both.
- PR-AB consolidated table claude_lanes_AB_HANDOFF_TO_LANE_C_20260921.md:313-331: #8 no #pragma steering ever; #9 semantically inert steering rejected; #10 January bug only once strict exact + quoted insns + comment + layout typedef assertion; #14 unnamed unreferenced globals HELD.
- PR-3ba2eb91 (2026-09-21 11:33) action_obey aggregate view copy `alignment_vector = *(real_vector2d const *)&actor->input.facing_vector;` admitted under rule-24 with explicit caveat (not strip-testable; /Od 0x43c7c7 integer mov pair; member-wise alternatives residual). Owner quote in commit: "Admit only with per-site /Od attestation, byte-inert cast, compatible prefix layout, strict exactness, and ledger disclosure. Land the three; keep the fighter held."
- PR-0915 opus5_250k_house_clean_checkpoint_20260915.md:20-31: strict on all held items (SSE asm macro held; env_fog/models .bss descriptive splits held; anchor split held).
- ai.c pragma: 1a6da50d (2026-09-09) inherited; Lane A R9 "read strictly, house rule 26 would not admit a code-generation directive proposed fresh"; Lane C handoff 5.2 hud_draw _fast_ftol_C HELD "rule 8 stands"; AB table #8. NOT an admitted precedent.
- object-closure reconciliation 2026-09-24 (claude_object_closure_canonical_reconciliation_20260924.md:43-62): canonical REJECTED genuine real_math.h distance3d body because full sweep lost _rasterizer_frame_statistics_draw (4,176) + object_lights fn -> precedent: frame_statistics count-canary loss NOT accepted.

## More precedents found
- PR-5e81dabf (2026-09-23) volatile admitted ONLY with HCEX/HCEA type-record support (fourth_five_objects_admission_20260923.md "The cache volatile qualifiers have independent HCEX/HCEA type-record support, not a register-forcing guess").
- PR-e942f338 realcmp consolidated in real_math.h because January assert strings attest it; "realcmp_epsilon is untouched" (still TU-local, inferred, sound_dsound_xbox.c:61 from 2885ba82).
- PR-Lane C rulings file C:/halo-worktrees/claude-lane-c-graphics-ui-20260920/scratch/lane_c/owner_rulings.md: R6 hud_draw pragma "Hold it; no pragma ... audit the existing ai.c pragma separately"; R8 /Od dead arrays admitted only because /Od RTC descriptors NAME them, "rule 8 is not weakened generally"; R9 _eat_my_shorts duplicated branch admitted because "January and /Od independently attest"; R10/R14c,d reachable UB held; R13/R14a,b admitted only when provably irrelevant.
- PR-26601453 (2026-09-16) laws_w3 A26 duplicated-assignment else-if admitted only with distinct January rejection blocks; commit text calls bink "the rejected _bink_query_analog_controller_buttons, whose edges share one block".
- PR-editor_flying_camera dead `left` pair (fifty-objects ledger claude_fifty_objects_20260925.md:169): admissible because same /Od RTC name + statement pair + zero anchor drift + count-only control NOT exact.
- PR-a4ba37a9 (2026-08-27) units double origin_scale temp at inlined pfl (predates 08-30 house rules/strip test; not /Od-checked). actor_moving.c:2136 `double t` landed only in a fuzzy function (ea507d5b ZERO bytes banked) -> not an admission precedent.
- PR-bsp3d.c:82-94 TU-local plane-distance helpers: 772e3d24 (2026-08-20) predates house rules; static-surplus policy question never answered.
- PR-f5698ba2 (2026-09-20) owner rejected a redundant equal-value store as "semantically inert steering" (_actor_look_update (c)).
- Extern-prototype-over-static-definition (interface Variant B): tree scan found ZERO Halo sites (no precedent; C4211 absent from docs).
- Empty then-arm: tree scan finds only first_person_weapons.c:1707 (parked, zero credit).
- allow_incomplete_unit single-section data entries: leaf_map _leaf_map_initialize_section (config/semantic_data_matches.json ~l.176, ae12a3c1).

## git apply --check (read-only) at HEAD 434f0151
OK: projectiles/production.patch; review_vehicles/fallback_hunks12.patch; vehicles/symbols_turret_static.patch;
    owner_queue/objlights_render_debug/patches/{01,02,03_NO_ALT,05,06}; names_xbox_hs_scen/patches/independent/rasterizer_xbox_debug_static_wrappers_symbols.json.patch;
    breakable_surfaces_audit2/patches/{breakable_surfaces.c,config.json}.patch; draw_string/ownership.patch; actions/production_fuzzy.patch;
    transport_endpoint_set_winsock/storage_only.patch; interface/{production,symbols_static}.patch; structures/production.patch;
    rasterizer_geometry/production.patch; bink_playback/production.patch
FAIL (symbols.json line drift, regenerate in place): breakable_surfaces_audit2/patches/symbols.json.patch (@23111), draw_string/symbols_json.patch (@23268), bink_playback/storage.patch (@23296)
FAIL (not a git patch): weapons/symbols_static.diff
FAIL (Packets A/C already landed): actor_perception/production.patch -> regenerate blockage-only hunk from final_full

## Classification (completes_object = a yes on this item + its listed covered companions completes the object)
- projectiles _projectile_new paren/named accessor: HELD_CLASS (RB2/#5; PR-A3 first-party-name condition unmet). ai_debug PR-A2 predates PR-A3/RB2 -> inconsistency to put to owner, not a covering precedent. completes=true on a yes.
  + COVERED zero-credit: w/projectiles/production.patch (ruling #1, projectiles in the 17). + COVERED data: fifty-objects-20260925 scratch/w/projectiles/semantic_data_entry.json (leaf_map allow_incomplete_unit precedent).
- actions squaring macro: NEW_RULING_NEEDED (PR-A1 is function-scoped; hand `(d)*(d)` also exact so bytes don't prove a macro; structural s7 held as extension). actions grouped data: NEW_RULING_NEEDED (verifier/tool change; brief s4.5 single-section only). production_fuzzy.patch zero-credit covered (alt if macro refused).
- actor_perception blockage: NEW_RULING_NEEDED (view copy = 3ba2eb91 class, but empty then-arm + redundant else store have no precedent; f5698ba2 rejected redundant equal-value store; tree has 0 credited empty then-arms). production.patch fails apply -> regen blockage-only hunk from final_full.c.
- action_charge: HELD_CLASS (dead target_point fails the editor_flying_camera count-only-control condition: x_suicide/x_top/dummy longs also exact => filler oracle). C1/C2 view copies alone would be 3ba2eb91 class.
- structures double t: NEW_RULING_NEEDED (a4ba37a9 predates strip test and was not /Od-contradicted; actor_moving:2136 zero-credit only; /Od 0x8a07bb contradicts).
- rasterizer_geometry staged temp: NEW_RULING_NEEDED (/Od 0x82ea80 has no scalar slot; closest a4ba37a9).
- bink q3: NEW_RULING_NEEDED (26601453 rejected this site under A26; Lane C R9 needs January+/Od; cross-build evidence is new type). storage.patch needs symbols.json regen. _code_001b5850 violates house rule 7 (descriptive rename allowed by rule 7 for private fns).
- vehicles scout: HELD_CLASS (count-oracle leveling pfl2d contradicting /Od 0x8f8b4a). fighter: HELD_CLASS (owner "keep the fighter held" 3ba2eb91 + Codex P1). fallback_hunks12 + symbols_turret_static: COVERED zero-credit.
- interface: NEW_RULING_NEEDED (Variant B: 0 Halo extern-then-static sites; Variant A = declaration-count loss held class).
- ui_widget dpad split: HELD_CLASS (invented .bss name; brief s8 / 0915 / data-lane provenance rule).
- rasterizer_xbox_lights: HELD_CLASS (R15 hand accumulation = RB3/AB#4 manual expansion; option 2 = P1 class).
- weather class I: NEW_RULING_NEEDED (Lane C R8 requires RTC NAME; editor_flying_camera requires RTC name) + Codex real_math reservation for provider.
- breakable B2: HELD_CLASS (brief rule 6 hand-copy of header helper; /Od calls genuine helper; bsp3d precedent 772e3d24 predates rules). B1: HELD_CLASS (count loss). audit2 sub-packet: COVERED zero-credit (symbols.json.patch needs regen).
- rasterizer_xbox names: HELD_CLASS (brief s8 item 2; HCEX enum improves #8/#9 stems only).
- render_debug D0/D1: HELD_CLASS. 05/06: COVERED zero-credit (rule-6 exception / objects.c tentative-definition precedent).
- bitmap_drawing: HELD_CLASS (AB #14 unnamed globals HELD).
- object_lights Q1: HELD_CLASS (brief s8; rule-6 exception text requires byte-identical). 01+02+03_NO_ALT: COVERED.
- game_engine: HELD_CLASS (Q3 pad aggregate + descriptive statics contradicted by HCEX static locals); Q1-B IS 3ba2eb91 class; Q2 governed by ruling #4 (variant_no_single_consumer_headers; hud_nav_points.h still invented).
- weapons: HELD_CLASS (class F interface bug + descriptive weapon_update_section). 11 static flags: COVERED.
- network_connection: HELD_CLASS (N+1 OOB read; redundant success=TRUE = AB#9, structural s6 reject).
- draw_string: HELD_CLASS (NULL deref). ownership.patch COVERED (HCEX names, a595bbc2 = {0}); symbols_json.patch needs regen.
- transport_endpoint_set_winsock: HELD_CLASS (staging boolean = AB#9 inert flag; invented call).
- transport_endpoint_winsock: HELD_CLASS (uninitialised thread read).
- players: HELD_CLASS (RB2 explicit; wrapper macro edits RESERVED real_math.h and reverts admitted 73c05198 accessor).
- collisions: HELD_CLASS (frame_statistics count loss; canonical 2026-09-24 rejected identical loss).
- collision_bsp: HELD_CLASS (SSE asm held 0915).
- geometry: polygon2d NEW_RULING_NEEDED (header cast in RESERVED real_math.h; structural s3(b) unanswered); hull3d HELD_CLASS (UB).
- connected_geometry: edge HELD_CLASS (uninit; note Lane C R14(a) provably-unreachable precedent); vertex realcmp_epsilon NEW_RULING_NEEDED.
- rasterizer_xbox_profile: B1 HELD, B2 HELD (varargs UB; ai_script ratification never recorded), B3 NEW (5e81dabf requires HCEX type record).
- hud_draw: HELD_CLASS (Lane C R6 explicit "Hold it; no pragma"; ai.c pragma is inherited and flagged for audit, not admitted).
- path_obstacles: HELD_CLASS (brief s2 held + RB2). storage fix covered.
- cross-cutting frame_statistics canary: HELD_CLASS.
More apply checks: rasterizer_xbox_profile/patches/01-06 OK, 07 FAIL (@object_admission_rejections.json:17); draw_string/bug_body_after_ownership OK;
transport_endpoint_winsock/production.patch FAIL (@c:340, storage packet already landed -> regen bug-only hunk from cand_final);
network_connection/production OK; collisions/production OK; weapons p1/p2/p3 OK; weather/production OK; action_charge/production OK;
actions/owner_gated_exact OK; objlights 03_YES/04_YES OK. rasterizer_xbox_debug already Matching in config (index 126): the independent static-wrapper patch removes its object_audit FAIL(8).
collisions.c bare paren now at line 1581 (was 1572).
_fast_ftol rejection for hud_draw still present (config/object_admission_rejections.json ~l.21).
DONE: final structured audit returned.
