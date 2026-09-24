# Fifty-object lane: owner ruling queue (measured at cdc8ebd3)

Every item below is a packet that is ready to apply to cdc8ebd3 and has been measured. The worker ledger for each item recorded its gate, whole-object audit, provider-link and data measurements. None of these packets has landed. Each item is one yes/no (or pick) question. The "Would complete" column lists every object that becomes COMPLETE on a yes. The integrator then applies the packet in APPLY_ORDER and runs the full batch gate: ninja, the stable 8,245-function diff, parks, admission, fake scan, pytest and diff-check.

If every item were a yes, **18** more Halo objects would complete (379 -> 397; +36 over the lane base, before any regression the full gate might show).

Packets and evidence: `research/fifty_objects_20260925/w/owner_queue/<lane>/` (patches/, APPLY_ORDER, RULING.md, MEASURED.md, LEDGER.md). The integrator kit is in `research/fifty_objects_20260925/integrator_kit/`.

**Canonical moved during the lane.** `jonas/exact-pilots` is now at dbcea3d6, 9 commits past this lane's base e9e62b78. It independently admitted models (a595bbc2, file-scope `= { 0 }` statics, the same form as item "models .bss" option P1), hs_scenario_definitions (c5fe0f40/d99a8658, separate data definitions) and hs_globals_external (3da50d63). **The models and hs_scenario_definitions items below are therefore SUPERSEDED by canonical**: take canonical's version at reconciliation and drop this lane's packets. Excluding them, a yes on every remaining item completes 16 objects.

| # | Item | Would complete | Functions gained | Apply-ready |
|---:|---|---|---|---|
| 1 | actor_combat P1 | ai/path_obstacle_avoidance, ai/path_smoothing, math/real_math, physics/collision_features, ai/path_structure_bsp | - | yes |
| 2 | rasterizer_xbox .bss split with 9 descriptive static names (whole-object admission) | rasterizer/xbox/rasterizer_xbox | - | yes |
| 3 | hs_scenario_definitions data split with 9 descriptive static names (whole-object admission, rejection retirement) **(SUPERSEDED by canonical)** | hs/hs_scenario_definitions | - | yes |
| 4 | object_lights | objects/object_lights | - | yes |
| 5 | render_debug | render/render_debug | - | yes |
| 6 | game_engine admission pick | game/game_engine | - | yes |
| 7 | rasterizer_xbox_hardware_geometry | rasterizer/xbox/rasterizer_xbox_hardware_geometry | - | yes |
| 8 | models .bss **(SUPERSEDED by canonical)** | models/models | - | yes |
| 9 | dead_camera _dead_camera_update | camera/dead_camera | _dead_camera_update | yes |
| 10 | saved_game_files _saved_game_files_enumerate_available_to_local_player_index | saved games/saved_game_files | _saved_game_files_enumerate_available_to_local_player_index | yes |
| 11 | first_person_weapons patch B (authentic unguarded NULL dereference of first_person_weapon_animations; rebased onto cdc8ebd3 after patch A landed in b9a8d587) | interface/first_person_weapons | _first_person_weapon_update | yes |
| 12 | player_profile R1-R3 (uninitialised serialised 512-byte block; seek failure skips file_close; close(&file, NONE) after delete-while-open), rebased onto cdc8ebd3 after zero-credit packet A landed in 6e3e2d35; R3 BUG comment amended | saved games/player_profile | _player_profile_create_default_profiles_on_disk, _player_profile_write_thread_proc@4, _player_profile_new | yes |
| 13 | game_engine_king | game/game_engine_king | - | yes |
| 14 | glow _get_particle_world_position | objects/widgets/glow | _get_particle_world_position | yes |

Items landed but pending a ruling, which are not in the table above:

- **Layer 2** (931ed8dc): the lock/unlock_global_random_seed prototype move.
- **P7** (466698b8): the dead hud_draw.h include in rasterizer_xbox_models.

Both are described in the main ledger, with their revert costs.

Held items with no object at stake are listed in the main ledger's held-items table:

- main header_swap and main_crash;
- transport `net_startup_debug();`;
- the damage A and hs cleanup COMDATs;
- actions grouped data;
- breakable_surfaces B1;
- network_connection;
- rasterizer_xbox_profile;
- bitmap_drawing.

## 1. actor_combat P1: cross_product2d (remove the NODUP hand copy and restore the /Od-attested call in _actor_aim_grenade)

- **Question:** Should patch 01 be admitted while _actor_aim_grenade is still not strict exact (576 bytes vs January's 544)? Patch 01 deletes actor_combat.c's hand-written NODUP cross_product2d copy and its #define/#undef cross_product2d_inline rename, so the genuine real_math.h __inline emits January's actor_combat-owned _cross_product2d section. It also spells the /Od-attested call as cross_product2d((real_vector2d const *)&actor->input.facing_vector, &aim_vector2d) > 0.0f. All 88 actor_combat sections stay byte-identical; the only change is the _cross_product2d COMDAT selection, NODUPLICATES -> ANY. If YES, patches 02 and 03 (or 03b) also apply, and 5 objects are admitted (Halo 379 -> 384).
- **Would complete:** source/ai/path_obstacle_avoidance; source/ai/path_smoothing; source/math/real_math; source/physics/collision_features; source/ai/path_structure_bsp
- **Packet:** `research/fifty_objects_20260925/w/owner_queue/p1_cross_product2d`
- **Apply-ready at cdc8ebd3:** yes

**Evidence for.** 1. /Od fn 0x44ebb0 is _actor_aim_grenade (from its RTC names). At 0x44ed38-0x44ed45 it pushes &horizontal_aim_vector and actor+0x140 (facing_vector, a real_vector3d), then calls cross_product2d (fn 0x455250). That is a real call with the 3D facing vector as the 2D operand, and it is the only cross_product2d caller in the actor_combat range.
2. January's own bytes bind &facing_vector once (lea ecx,[edi+0x174] at 0xe4) and read both inlined 2D products through that register. This is the pointer-parameter signature. h.j*f.i-h.i*f.j at 0x105-0x120 is the only 2D cross product in the whole object.
3. January selected actor_combat's _cross_product2d. The genuine header inline emits a byte-identical copy, so P1 adds NO surplus. It removes a Rule-6 hand copy of the same class as the 17 removed by the Layer 1/2 repairs.
4. The only known EXACT diagnostic of aim_grenade (G1) is built on the /Od helper-call list, with _cross_product2d coming from the header.
5. The same cast expression is already landed in Matching objects: action_obey:2593 and :1259, and action_vehicle:395.
6. Status-quo cost: 10 objects carry an LNK2005 on _cross_product2d, and 5 of them are otherwise complete.

**Evidence against.** 1. The caller _actor_aim_grenade is not strict exact (576 vs 544, the held declaration-scope/goto residual). Both law (i) and the view-cast rule require a strictly exact caller, and byte exactness cannot tell the call apart from the hand expansion at this site.
2. The /Od build is a later revision; January's scope for horizontal_aim_vector differed from it (frame 0x24 vs 0x28).
3. P1 restores only cross_product2d. The adjacent /Od-attested dot_product2d(&facing,&h) stays hand-expanded, because restoring it would newly emit a _dot_product2d COMDAT from the non-exact caller.
4. Precedent risk: admitting a byte-inert header-COMDAT respelling in a non-exact caller. The proposal limits this to sections the same object already owns in January's split, with no new surplus.

**Still blocked / integrator notes.** Nothing in the packet is blocked. The integrator still owes the full batch gate: ninja, stable_verdicts sweep, parks, admission audit, fake scan and pytest. Workers may not run these, but since only actor_combat.c changes and it is byte-identical apart from the COMDAT selection, a zero-regression result is expected.

actor_combat itself stays NonMatching: aim_grenade and plan_grenade_trajectory are residual, and it has 4 pre-existing _actor_combat_*_inline externals with no January provider.

These objects lose the cross_product2d failure but remain incomplete:
- decals: 2 residuals; patch 03b optionally retires its now-premise-less _cross_product2d rejection
- collision_bsp: 3 residuals
- actor_looking: _actor_look_update
- geometry: 3 residuals plus a _plane2d_from_points conflict
- bipeds: 9 residuals plus an _object_get_type conflict

**Measured.** All measured at cdc8ebd3; the tree was clean before and after.

Patches: the wave-2 patch applies unchanged, because actor_combat.c did not change between 931ed8dc and cdc8ebd3. The patched file is byte-identical to wave-2 p1.c. git apply --check is clean for 01, 02 and 03 each, for 01+02+03 together, and for 01+02+03b together. A sandbox apply on the cdc8ebd3 blobs gives 497 -> 502 Matching statuses and 8 -> 7 rejections.

actor_combat gate --all: production, P1 and P1-strip all give 32 EXACT plus the same 2 residuals (_actor_aim_grenade 576/544 sha e9bba5b0; parked _actor_combat_plan_grenade_trajectory 288/5 sha ff9a7477, which equals the park base). A keyed raw-COFF diff over 88 sections finds exactly 1 difference between production and P1: _cross_product2d NODUP -> ANY. P1 against P1-strip: 0 differences, so the view cast is byte-inert. object_audit output is identical for production and P1: FAIL(2), the two residuals only, 0 of 60 symbols differ. /W3 warnings are the same 17 for P1; the strip adds only C4133. The fake scan finds 0 leads in either version.

The five objects (tree source, no change; each gate object is keyed-identical to build/base):
- path_obstacle_avoidance: 24/24 EXACT, audit PASS (52 symbols, 0 differ), PDB 0, surplus 0 of 11 not identical
- path_smoothing: 6/6, PASS (15), PDB 0, 0 of 10
- real_math: 84/84, PASS (134), PDB 0, 0 of 35
- collision_features: 20/20, PASS (30), PDB 0, 0 of 29; _projection_from_vector3d is IDENTICAL to January's decals copy and links PASS, so its rejection is stale
- path_structure_bsp: 6/6, PASS (13), PDB 0, 0 of 10; the HCEX tables are already landed and the split is current

provider_link against production actor_combat: each of the five is FAIL(1), on _cross_product2d only. Against P1 (provlink2 --subst=source/ai/actor_combat:P1 obj, both orders): all PASS, every row. objdiff 3.3.1 report shows all five at 100% code, functions and data. The admission audit, emulated with the five marked complete and the rejection from patch 03 removed, reports 0 contradicted and 0 revoked.

Board census: there are 11 _cross_product2d definers. All 10 non-actor_combat copies are SELECT_ANY and section_infos_equal to January's copy. All 10 FAIL the pair link against production and PASS against P1. After P1:
- actor_looking, decals and collision_bsp have no link failures left, but still have function residuals (15/16, 31/33, 27/30).
- geometry still fails on _plane2d_from_points and has 27/30 functions.
- bipeds still fails on _object_get_type and has 42/51 functions.

No symbols.json, header, parked.json or semantic_*.json change is needed.

## 2. rasterizer_xbox .bss split with 9 descriptive static names (whole-object admission)

- **Question:** Should rasterizer_xbox be admitted as Matching? The invented rasterizer_xbox_d3d_globals aggregate and its 23 #define redirects become 23 '= 0' file statics. 14 of the names are bare in January's IDirect3D*() strings. The 9 below are not first-party: - node_matrix_constants (+0x000): INVENTED. - bitmap_dimensions_non_blocking (+0x840): HCEX-convention stem (HCEX static-local 'dimensions'; the HCEX scope form is refuted for January's .bss order). - bitmap_dimensions (+0x844): HCEX-convention stem, same as above. - global_d3d_texture_render_primary[2] (+0x84C): convention stem (January global_d3d_{texture,surface}_<target> pattern plus the HCEX/Od _rasterizer_target_render_primary enum). - global_d3d_surface_render_primary (+0x854): convention stem; January attests the sibling _render_primary_z. - global_d3d_texture_render_secondary_z (+0x860): convention stem (render_secondary and the _z suffix are attested). - global_d3d_surface_render_secondary_z (+0x868): convention stem, same as above. - global_d3d_texture_render_primary_copy (+0x8A0): INVENTED '_copy' suffix. - global_d3d_surface_render_primary_copy (+0x8A4): INVENTED '_copy' suffix. Yes = apply patches 01-05. No = nothing lands; the object stays NonMatching with object_audit FAIL(22).
- **Would complete:** source/rasterizer/xbox/rasterizer_xbox
- **Packet:** `research/fifty_objects_20260925/w/owner_queue/names_xbox_hs_scen`
- **Apply-ready at cdc8ebd3:** yes

**Evidence for.** - Fake-stringification: January's strings name 14 of the globals bare, and no string contains 'rasterizer_xbox_d3d_globals', so the aggregate is invented.
- '= 0' declaration order reproduces January's .bss exactly. The strip test fails the audit on 23 rows, so the initialisers are layout-attested.
- There is no cachebeta public in 0x45E028..0x45E8CF, so every name is static.
- Every other audit row is first-party: HCEX static-local srcblend/destblend/blendop_table; the filthy_bitmap_defaults_initialize rename (atlas, 2001 map, HCEX); d3d_palette strings; January's D3DDevice_ flicker relocations.
- Counter-precedents: rasterizer_xbox_shadows was admitted with 2 disclosed descriptive gap statics; collision_debug's descriptive split landed for function credit; eb33baec landed the descriptive macro name match_assert_stack_frame.

**Evidence against.** - The 2026-09-15 owner ruling prefers no descriptive or invented static names in symbols.json; it held the env_fog (21) and models (13) .bss splits.
- The 2026-09-24 canonical held path_obstacle_avoidance because its 'BSS relabel lacks authenticated private identities'.
- 3 of the 9 names are invented outright (node_matrix_constants and the two _copy names).
- The packet's block comment says the error strings name 'these globals' bare, which overstates it for the 9. RULING.md proposes an optional byte-inert disclosure comment; it is not in the patch.

**Still blocked / integrator notes.** Only the owner's policy ruling on the 9 names. There is no technical blocker.

Rebase note: a NEW required config change at cdc8ebd3. 05_parked.json.patch re-baselines __rasterizer_model_draw, because models.c lost its hud_draw.h include and patch 02's prototype removal now flips a declaration-count tie in that parked fuzzy body. It has no strict effect and needs no source change.

Independent and not name-gated: the reviewer-approved rasterizer_xbox_debug static-wrapper symbols patch (object_audit FAIL(8) -> PASS, gate 18/18, split radius 1) is included under patches/independent/.

**Measured.** At cdc8ebd3 (HEAD later moved to eb33baec, a docs-only commit; patches re-checked clean there).

Apply checks:
- All 4 wave-2 patches apply as-is. I regenerated them with zero offsets as 01-04.
- The post-image .c equals the worker's cand_L.c, and the header equals the worker's header.

Split and gate:
- csplit with the patched config: of 833 objects, only rasterizer_xbox.obj changes. The control split equals build/split.
- gate, candidate plus patched header (shadow; the #error control proves it is used) vs the emulated split: 95/95 EXACT, and the _point_from_line3d guard passes.

Audits:
- object_audit (split override): PASS. 188 sections ok (.bss 2230 8-align, blend .rdata 108 4-align, .data 6) and 216/216 symbols. Production is FAIL(22).
- provider_link: PASS 15/15 in both orders.
- Storage vs cachebeta: 216/0 disagreements (production has 25).
- /W3 warnings identical to production; fake_match_scan 0.

Data: objdiff 3.3.1 identical for production and candidate: data 9048/9048, code 91/95 raw with the same 4 pre-existing under-credits.

Header blast radius (7 includers):
- 6 objects are byte-identical.
- rasterizer_xbox_models: the PARKED __rasterizer_model_draw moves (sha 89b0d7ea -> cbfa8585 at the same 5168 bytes / 348 relocs; objdiff 95.08453 -> 95.07629; strict status unchanged at 12/2).
- tools.parked_functions reports STALE 1 without a re-baseline, and active 2 / stale 0 with the new 05_parked.json.patch.

Names are load-bearing: reverting only the .bss rows gives gate 86/95 (9 reloc-identity residuals) and audit FAIL(15).

Combined application with item 2 (either route) is clean, and the post-images are cmp-identical when applied outside the repo.

## 3. hs_scenario_definitions data split with 9 descriptive static names (whole-object admission, rejection retirement) (SUPERSEDED by canonical; see top)

- **Question:** Should hs_scenario_definitions be admitted as Matching? The invented wrapper structs, #pragma pack, 11 __declspec(align(4)) and the field_table_alignment padding member become 18 separate January data definitions. This retires the source-layout-incomplete rejection and adds 9 non-first-party static names: - hs_scripts_block_fields (+0x018): HCEX-convention stem; <block>_fields plus January's public block name. - hs_globals_block_fields (+0x090): HCEX-convention stem. - hs_references_block_reference (+0x110): INVENTED; no convention exists for a TU-local any-tag tag_reference_definition. - hs_references_block_fields (+0x11C): HCEX-convention stem. - hs_source_files_block_fields (+0x17C): HCEX-convention stem. - data_array_header_bs_codes (+0x1CC): HCEX-convention stem; <name>_bs_codes plus January's own string 'data_array_header'. - data_array_header_bs_definition (+0x204): HCEX-convention stem. - syntax_node_bs_codes (+0x218): HCEX-convention stem; stem is January's string 'syntax_node'. - syntax_node_bs_definition (+0x240): HCEX-convention stem. Yes = route A (A01-A04) or route C (C01-C03 + A02-A04; C adds HCEX's MAXIMUM_HS_*_PER_SCENARIO enum to the shared header). No = nothing lands; the rejection stays; data stays 484/1112.
- **Would complete:** source/hs/hs_scenario_definitions
- **Packet:** `research/fifty_objects_20260925/w/owner_queue/names_xbox_hs_scen`
- **Apply-ready at cdc8ebd3:** yes

**Evidence for.** - It retires every objection the rejection cites: invented aggregates, pack/align directives and the padding member.
- .data alignment 8 arises naturally from the two field arrays of 64 bytes or more; the 0x8C..0x90 gap is VC7's per-object alignment pad.
- The candidate's .rdata literal order equals January's. This attests the block-before-fields definition order, and January's leaf_map shows the same idiom.
- The data credit is native, with no semantic_data_matches entry.
- 8 of the 9 names come mechanically from January's own names (public block names, byte_swap_definition name strings) via HCEX-attested Bungie conventions: 10 *_block_fields and 23 *_bs_definition in HCEX.
- Route C's enum is HCEX's exact enum, and January's hs_compile assert text attests it.

**Evidence against.** - The 2026-09-15 owner ruling prefers no descriptive or invented static names in symbols.json.
- hs_globals_external was rejected because a later build does not authenticate its names; here HCEX has no hs_scenario_definitions compiland at all, so no build authenticates these names.
- hs_references_block_reference is invented.
- The packet has no disclosure comment for the names; RULING.md offers an optional byte-inert one.
- Two new C4090 warnings, caused by the reconstructed tag_groups.h 'char **names' member type.

**Still blocked / integrator notes.** Only the owner's policy ruling on the 9 names; there is no technical blocker. The owner may rename any single name, for example #3, as a pure rename in A01+A02.

The function rename (the reviewer fallback) already landed in 6e3e2d35, so no fallback remains. A no leaves the object at the current rejection with 484/1112 data.

**Measured.** Rebase at cdc8ebd3:
- The worker's A, B and C2 no longer applied, because 6e3e2d35 landed the static byte_swap_script_syntax_data rename.
- A01 and C03 are the worker's final post-images (cand1.c/cand2.c, CRLF) diffed against the tree; they carry only the data split.
- A02 is the 9 inserted static rows only.
- D is split into A03 (config.json) and A04 (rejection removal).
- C1 is reproduced as C01 (header) + C02 (hs.c).
- Every patch passes git apply --check, alone and per route, and together with item 1. Post-images applied outside the repo are cmp-identical.

Split: csplit with the edited config changes only hs_scenario_definitions.obj (833 compared).

Gate, audit and storage:
- gate vs the emulated split: EXACT 224 _byte_swap_script_syntax_data 1/1 for both routes (route C with the shadow header and an #error control), guard passed.
- object_audit: PASS for both routes. 23 sections ok, including .data 628/628 with align 4/4 (January's 8-byte alignment), and 40/40 symbols. Production is FAIL(1) on alignment.
- Storage: 40/0; none of the 9 names is a cachebeta public.
- provider_link: nothing to link, PASS.

Data:
- objdiff 3.3.1: 1112/1112 native, +628 data bytes (production 484/1112, .data 92.773094).
- Without the 9 symbols.json rows, .data is 48.931007%, so the rows are load-bearing.

Route C consumer sweep (re-run because hs.c changed heavily in 6e3e2d35):
- hs 447/1 (1410 sections, 0 differ), hs_compile 61/3 (0 differ), hs_runtime 64/2 (0 section diffs; 64 $L names shift only).
- objdiff per-function maps are identical to build/base.

Other checks: /W3 adds 2 C4090 (byte-inert, disclosed); fake_match_scan 0. No parks involved.

## 4. object_lights: the surplus _object_get_bounding_sphere copy that carries its per-TU __FILE__ literal, plus data_gap's packet (4 statics, the should_render_lights call, the _lights_section entry)

- **Question:** Should we admit object_lights' surplus _object_get_bounding_sphere COMDAT as it is? It matches January's selected action_vehicle copy in size (144), normalized bytes, all 9 relocation sites/types and 7 of 9 targets. Only its two __FILE__ DIR32 targets differ: they point to "c:\halo\source\objects\objects.h", which is January object_lights' own 33-byte .rdata literal, instead of "..\objects\objects.h". Rule 6 as written ('byte-identical to the selected provider') cannot be met by any source, because object_audit requires that January-owned literal and VC7 emits it only when this COMDAT references it. YES: apply 01, 02, 03_YES, 04_YES and object_lights becomes Matching. NO: 01 and 02 can still land at zero credit, and optionally 03_NO_ALT (+1,541 data under allow_incomplete_unit).
- **Would complete:** source/objects/object_lights
- **Packet:** `research/fifty_objects_20260925/w/owner_queue/objlights_render_debug`
- **Apply-ready at cdc8ebd3:** yes

**Evidence for.** The January string census gives one owner per spelling. The c:\ objects.h literal is defined only in object_lights, and no relocation anywhere references it. The ..\ literal is defined by action_vehicle and imported by render_objects. _object_get_bounding_sphere is defined by action_vehicle and imported (UNDEF) by the other 8 users, object_lights included. NEW: of 9,420 January literals across all 833 split objects, exactly one has no referrer, and it is this literal, so its only possible referrer is object_lights' own discarded COMDAT copy. The /Od build asserts in 0x443f50 (object_get_bounding_sphere) with "D:\P4\Halo1\source\objects/objects.h", which shows the header used plain __FILE__. Link order is consistent: the image is in descending module order and action_vehicle (module 461) precedes object_lights (152), so a pick-first link keeps the ..\ copy January selected. The P2 control (a single-spelling header) loses the January-owned .rdata: the audit reports it MISSING and data credit drops by 1,100.

**Evidence against.** The rule 6 text is not met literally, so this is a new exception class: an authentic non-identical surplus COMDAT, proven by an orphan January literal. A whole-program relink that put object_lights first would select the c:\ copy. That cannot happen with January's link order and changes no January-owned section.

**Still blocked / integrator notes.** Nothing, once the owner answers yes. Advisory: object_lights also carries 5 consumer-local externs with no definer: debug_lights, debug_object_lights, light_data, light_cluster_partition, debug_rasterizer_light_count. They are the same class as render_debug (e). Precedent does not treat that class as a blocker (render_objects and scenario are Matching with such externs). Optional patch 07 resolves them as tentative definitions: byte-inert, and links clean against January's linker_common in both orders.

**Measured.** Every patch applies at cdc8ebd3. `git apply --check` is clean for each patch alone and for the YES set, the YES+(e) set, the NO set and 01-08 together. The patches are LF; applied one at a time in a throwaway repo with core.autocrlf=true, they reproduce the measured trees byte for byte. The data_gap object_lights.patch rebased unchanged (every hunk offset +4 by the landed OBJECTS_H_FILE define). symbols.json needs 4 in-place "static": true rows; the edited copy is ./symbols.json. Control: csplit with an unmodified config copy reproduced build/split 833/833. With the 4 rows (cfg_OL), only object_lights.obj changes. object_lights is 43/43 EXACT against the emulated split. object_audit PASS: 71 January symbols, 0 differ. The production object against the same split FAILs on exactly the 4 storage rows. pdb_storage: 0 disagreements. The other object_lights_rendering.h includers (render 13/13, render_objects 22/22) are objcmp-IDENTICAL to build/base. surplus: 12 candidate-only COMDATs, 11 identical; the only different one is _object_get_bounding_sphere (only the DIR32 rows at +37/+76 differ). provider_link PASS in both orders for all 33 surplus names. Mini objdiff 3.3.1: code 12294/12294, 43/43, data 1104/2645. Running tools.semantic_progress.apply_semantic_data_matches on the entry credits +1541 (2645/2645) for both the YES entry (unit complete) and the NO_ALT entry (allow_incomplete_unit). The negative control fails closed as expected. fake_match_scan: 0 leads. /W3: only the pre-existing C4133 and C4244 warnings. The full tree built from the patch files only (treeFINAL) gives the same rows: action_vehicle 17/17 and every includer identical.

## 5. render_debug: helper/vehicle patch (a)-(c), plus the held (d) invented render_debug_globals_definition aggregate and (e) six consumer-local extern boolean debug_*

- **Question:** (d) How should render_debug's 29,708-byte cache state be spelled? D1: seven separate zero-initialised file statics in the aggregate's member order, with descriptive names render_debug_cache_{strings,entries,game_time,entry_count,string_offset,entry_overflow_reported,string_overflow_reported}. This needs a symbols.json rename plus 6 static rows (lab/D1_OPTION_*.patch). D0: admit the existing aggregate, with its two opaque pad arrays, as it is. Or HOLD. Either yes, with 05 (+06) and 08, makes render_debug Matching. (e) is answerable without a ruling: January's definer is render_debug.c itself (tentative definitions the linker pooled into COMMON). The integrator only needs to confirm optional patch 06, bare `boolean debug_bsp;` etc., the same form as the objects.c precedent.
- **Would complete:** source/render/render_debug
- **Packet:** `research/fifty_objects_20260925/w/owner_queue/objlights_render_debug`
- **Apply-ready at cdc8ebd3:** yes

**Evidence for.** (e): The six are January linker_common COMMON records 50-55 (1 byte each), referenced only by render_debug.obj and hs_globals_external.obj. Pool oracle over all 242 records: 102 of 105 single-referencer records follow module order. hs_globals_external references 81 pool records and is the position-admissible owner of none of them. The six sit between render_objects' cluster and render's _render, and render_debug (module 87) is the only referencer in that range. The 2001 Bungie linker maps show the six exist exactly when render_debug.obj is linked: 08-15 beta has 5 toggles and no render_debug_structure_decals; 09-25 beta adds debug_permanent_decals together with its only user, render_debug_structure_decals; 09-25 retail has no render_debug.obj and none of the six, while render_objects/render records and other hs toggles remain. The later /Od build interleaves the six byte by byte with render_debug's own cache variables (0xf0afb0-0xf0afc0), which only one object's .bss can do. (d): January's short/gap/short/gap/short/bool/bool layout equals separate `= 0` statics; the aggregate needs pad members (the law (j) signature of an invented aggregate). In the /Od build, hs toggles sit inside the cache-variable span, so it was not a struct there.

**Evidence against.** (d): No first-party names exist anywhere (cachebeta publics, HCEX.pdb, the 2001 maps, the atlas), so D1 adds 7 descriptive .bss names, an already-held class. The zero initialisers are layout-evidenced, not text-attested. (e): The 2026-09-22 canonical reconciliation excluded a 116-definition COMMON wave because pool adjacency cannot prove TU ownership (rasterizer_frame_statistics counterexample). This packet uses per-symbol evidence (map presence/absence and /Od interleaving), but the /Od build is a later revision, so the integrator should confirm the evidence standard. Advisory: render_debug has 6 C4013 implicit declarations (ai_debug_render and collision_debug_render lack their header includes; 4 callees have no header prototype at all). Production has them too; render.c is Matching with 2 C4013.

**Still blocked / integrator notes.** (d) needs the owner's choice between D1 and D0 (the naming class). Without it render_debug stays NonMatching, though 05 and 06 can land at zero credit. The C4013 debt is a reviewer advisory that needs owner-header edits with a declaration-count sweep; it was not touched here.

**Measured.** The review_admit2 (a)-(c) patch (05) applies unchanged at cdc8ebd3; render_debug.c and vehicle_datum.h have not changed since 931ed8dc. render_debug: 36/36 EXACT, object_audit PASS (63/0), pdb_storage 0. surplus: 12 COMDATs, 0 not identical; the new _point_from_line3d is identical to action_charge's selected copy, and its callers are strictly exact. provider_link PASS in both orders. vehicle_datum.h is included only by actor_moving (31/36), physics (13/17) and vehicles (37/39), all objcmp-IDENTICAL to build/base; no header includes it. Mini objdiff 3.3.1 matches production: code 8235/9448, 34/36 on the known hidden_exact under-credit rows, data 30236/30236. (e) patch 06: 36/36, objcmp IDENTICAL; the only change is 6 external symbols UNDEF(0) -> COMMON(1). It links with January's linker_common.obj and with base hs_globals_external in both orders: 0 LNK2005/LNK1169, all six resolved (production leaves 12 unresolved lines). (d) lab (never lands): zero-initialised separate statics reproduce January's .bss exactly (0/0x400/0x7400/0x7404/0x7408/0x740A/0x740B, 29708 B). The whole-TU D1 lab is 36/36 and passes object_audit (69/0) against a lab split. Uninitialised statics give name-hash order and FAIL the audit, so the initialisers are load-bearing and layout-proven. 08 (status flip) applies cleanly. The treeFINAL rebuild from patch files gives the same rows. fake_match_scan: 0 leads.

## 6. game_engine admission pick: the motion-sensor copy in game_engine_player_get_custom_motion_sensor_positions. (A) the in-loop alias `struct netgame_goal *goal = &global_goal[goal_index];` vs (B) F07's /Od-attested `positions[count] = *(real_point2d *)&global_goal[goal_index].position;`. Each is packaged as a complete admission packet: the review4 house-clean (F01R/F01RB, F02, F03aR, F03b, F04) plus the status flip. There is also a variant without the one-consumer focused headers.

- **Question:** Q1: game_engine is 180/180 strict exact and passes the whole-object audit either way, and A and B give byte-identical objects. Do you admit (A) the in-loop alias, which the /Od frame at 0x5a20e0 does not have and which is load-bearing, or (B) the /Od-attested view copy, which is load-bearing and so fails the byte-inert condition of your rule-24 narrow admission? Or neither, and game_engine stays held? Q2 (only if Q1 is yes): the 2026-09-21 'do not invent a one-consumer header' ruling applies to F02. Do you take F02 as is (4 new headers), or the variant_no_single_consumer_headers packet? The variant puts the hudg, itmc and netgame layouts back in game_engine.c, their only consumer, and is byte-identical in all 447 TUs. It keeps hud_nav_points.h (one non-owner consumer), because moving the nav-point trio into the genuine hud.h breaks _populate_statistic_buffer through a declared-name-count tie. Q3 (disclosure): admitting game_engine also admits the pre-existing debug_player_color anonymous aggregate with `word pad` members, and the descriptive .bss names _game_engine_teleport_message_ticks and _game_engine_teleport_flash_fade_function. HCEX has these values as static locals of game_engine_update_teleporter. Do you accept them, or are they the held invented-aggregate / descriptive-static class?
- **Would complete:** source/game/game_engine
- **Packet:** `research/fifty_objects_20260925/w/owner_queue/pick_game_engine`
- **Apply-ready at cdc8ebd3:** yes

**Evidence for.** For B: /Od 0x5a20e0 does one 8-byte aggregate copy. The index is computed once (shl ecx,5) and x and y move as an integer mov pair; this build uses integer movs for struct copies (get_goal_position at 0x59d5b9) and movss for float scalars. The frame is only count, player and goal_index. real_point2d is a layout prefix of real_point3d, and HCEX's real_point3d has no 2D member. The caller is strictly exact. January's assert strings stringify Bungie view casts in actors, actor_looking, actor_moving and observer. For A: plain C, no cast. The same alias idiom is first-party in this TU: /Od goal_matches_player at 0x5b23c0 keeps a goal pointer slot [ebp-8] = &global_goal[i].

**Evidence against.** Against A: the /Od frame has no goal pointer slot and copies an aggregate rather than components. The alias is load-bearing (strip -> RESIDUAL). That makes it a non-/Od-attested, load-bearing named local: an owner-gated class, which review3 listed as blocking OBJECT_COMPLETE. Against B: rule-24 narrow admission requires a byte-inert cast, and this one is load-bearing: its strip gives the same RESIDUAL. Ruling 2 of 2026-09-21 keeps rule 24 strict for new punning casts. Both options also inherit the Q3 construct, which the owner has not ruled on: an anonymous debug_player_color aggregate with word pad members. HCEX has those values as static locals red/green/blue/duration of game_engine_update_teleporter. The 2020 /Od build puts unrelated globals between debug_player_color and the flash values.

**Still blocked / integrator notes.** Needs the owner's pick for Q1 (or hold). The focused-header ruling (Q2) must be settled: F02 as is, or the TU-local variant, which still needs hud_nav_points.h. The debug_player_color / descriptive-static disclosure (Q3) must be settled too. Whoever lands this must re-sweep it together with any concurrent owner-queue packet that edits game_engine.h, player_control.h, hud_messaging.h, sound_classes.h, hud.h or game_engine_king.c. F06 (optional, not packaged) edits game_engine_king.c, a separately held item, and does not fit the NP variant. Full ninja plus stable_verdicts remain the official gate. Nit, not changed: the dead local local_player_count shadows the public function local_player_count().

**Measured.** All at cdc8ebd3; the tree was clean before and after. git apply --check is clean for all 19 patch files alone, and for each packet's full set in one call: optionA, optionB, NP-alias and NP-view-copy. F07 alone does not --check at cdc8ebd3, so optionB ships a standalone F01RB (= F01R + F07, byte-checked). Rebuilding each packet from a HEAD export gives exactly the shadow that was measured. Compiles use the production cflags, cwd = shadow root, and coff_compare.section_infos_equal: optionA, optionB and both NP variants are 180/180 EXACT with 0/302 sections changed vs build/base; the control is 180/180. The optionA and optionB objects are identical: same order and no content difference. Full 447-TU sweep with canonical COFF: control head vs build/base is 447/447 SAME. optionA and optionB each give 446 SAME plus game_engine. game_engine differs only in three ways, all EXACT: always_invis is emitted later; the csplit alias `_global_goal+0x400` replaces `_global_variant+0` at 2 relocations; and $L numbering changes. NP vs optionA: 447/447 SAME. The 37 game_engine.h consumers all fall inside the sweep. Admission battery (every candidate): object_audit PASS with text identical to production; pdb_storage 0/269 disagreements; surplus 11 COMDATs, all identical; provider_link PASS, both orders, 39 surplus externals. objdiff 3.3.1 mini project: 27922/32397 code, 173/180 functions, 3792/3792 data, 0 deltas vs prod. audit_semantic_matches accepts 180/180, with the same 7 semantic-coff-only functions as production. tools/audit_object_admission already lists game_engine as a candidate with 0/0 gaps, so the status flip will not be revoked: Halo objects 379 -> 380. /W3: no new warnings. The C4013 console_printf warning and two long->short warnings are gone. Strip test (lab): with neither the alias nor the cast, the function is RESIDUAL, because the induction-variable base shifts to position.y (+4). Both spellings are load-bearing. Ruling variants: the three single-consumer layouts TU-local (vTUnp) are byte-identical everywhere. The genuine broad headers all fail. item_definitions.h moves hud_weapon's _render_weapon_hud. scenario_definitions.h plus the F06 siblings is a strict regression in ai_communication (_ai_communication_update_speech_timers) and makes the parked _ai_test_ballistic_line_of_fire stale. hud.h costs game_engine.c 23 names and flips _populate_statistic_buffer (count oracle, lab only: K=8-12 flips it). hudg cannot move without rewriting 7 HUD TUs.

## 7. rasterizer_xbox_hardware_geometry: spelling of the no-op MoveResourceMemory wrapper at 0x158460 (A = IDirect3DResource8_MoveResourceMemory vs B = D3DVertexBuffer_MoveResourceMemory)

- **Question:** January emits two static no-op wrappers: 0x158450 = _D3DResource_MoveResourceMemory@8, and 0x158460 = one directly called MoveResourceMemory forwarder. Which spelling should rasterizer_vertex_buffer_new use for that one call? A: IDirect3DResource8_MoveResourceMemory((D3DResource *)d3d_vertex_buffer, D3DMEM_VIDEO), with 0x158460 named _IDirect3DResource8_MoveResourceMemory@8. B: D3DVertexBuffer_MoveResourceMemory(d3d_vertex_buffer, D3DMEM_VIDEO), with 0x158460 named _D3DVertexBuffer_MoveResourceMemory@8. The argument and the position after the vertex-buffer Unlock block are byte-inert, and a 3-line comment in the source says so. Either A or B completes the object; if you reject both, the hg0 fallback is zero-credit hygiene only.
- **Would complete:** source/rasterizer/xbox/rasterizer_xbox_hardware_geometry
- **Packet:** `research/fifty_objects_20260925/w/owner_queue/pick_hwgeom_models/patches`
- **Apply-ready at cdc8ebd3:** yes

**Evidence for.** Both options: January's emitted static wrappers prove the call exists (static-COMDAT law, whole-image ret-8 census). The d3d8.h emission order excludes every other forwarder spelling. A only (style census, not byte proof): all 33 of January's Halo-side resource lib calls go through IDirect3D*8_ compat wrappers, with 0 native resource-level spellings. January emits _IDirect3DResource8_Release@4 in progress_bar, and the tree has IDirect3DResource8_Release((D3DResource *)...) at progress_bar.c:1812. This TU already uses typed compat names. B: needs no cast.

**Evidence against.** No first-party evidence names the spelling. The /Od build is the PC DX9 port, HCEX links no hardware_geometry code, and the October 2001 beta has the same pair without names. D3DMEM_VIDEO and the call position are chosen and byte-inert; the comment discloses that. A needs an XDK-convention (D3DResource *) cast; it is byte-inert, since B produces the same bytes without it.

**Still blocked / integrator notes.** Nothing, once the owner picks A or B. If neither, the object stays NonMatching: the two _code_ placeholder stubs remain (house rule 7 names; external where January is static).

**Measured.** Rebase: the source file is unchanged at cdc8ebd3 (blob c2bb9095) and symbols.json rows 5813-5825 are unchanged. The wave-2 patches apply clean. Only the symbols patch index line was refreshed, and a config.json status flip (NonMatching -> Matching) was added. No real change was needed. csplit: split_ctl == build/split (833/833); split_hgA and split_hgB each change only rasterizer_xbox_hardware_geometry.obj. A vs split_hgA: gate --all 17/17 EXACT (_point_from_line3d guard passed); object_audit PASS (25 symbols, 0 differ); pdb storage 0 disagreements (production has 13); 0 candidate-only code COMDATs; provider_link PASS in both orders (6 pre-existing rows: 3 pooled literals + 3 SDK tables); fake_match_scan 0. B vs split_hgB: identical results. cand_A and cand_B have byte-identical Halo sections (vertex_buffer_new 5442bf32, triangle_buffer_new e90e066b) and both 0x158460 bodies are sha 2a2f950f; only the name differs. objdiff 3.3.1 mini report: 898/898 code, 584/584 data for production, A and B, so there is no credit change. hg0 fallback vs split_hg0: 17/17, audit PASS, link PASS, but pdb shows 2 disagreements (the external _code_00158450/460 stubs), so it is not admissible. verify_apply.py: every packet passes git apply --check in the repo and sequential scratch application, and the post-images equal the measured files byte for byte. This also holds stacked with each models packet.

## 8. models .bss: file-scope `= {0}` statics in January (reverse-use) order (P1; variant P2 with fv as the HCEX static local) vs the first-party in-block static-local form (SL) (SUPERSEDED by canonical; see top)

- **Question:** Do you admit models.obj with load-bearing `= {0}` initialisers on the render_model defaults, declared in January's .bss order? There are two forms. P1: four file-scope statics fv, change_colors, effect, region, each `= {0}`. P2: default_function_values as an uninitialised static local at render_model function scope (as HCEX records it), plus the other three as `= {0}` file statics. Both contradict the later first-party form: HCEX -lines and the /Od build declare all four uninitialised inside their if (!param) blocks. The strip test fails: without `= {0}` the layout falls back to hash order. YES (P1 or P2) completes models and retires the bss-symbol-layout-mismatch rejection. NO means landing models_C1 (zero-credit storage fix) or models_optSL (first-party form, measured FAIL(4)); the object then stays blocked until first-party names for the three other defaults are found.
- **Would complete:** source/models/models
- **Packet:** `research/fifty_objects_20260925/w/owner_queue/pick_hwgeom_models/patches`
- **Apply-ready at cdc8ebd3:** yes

**Evidence for.** Measured VC7 law: uninitialised statics are laid out in decorated-name hash order, then `= {0}` statics in declaration order. P1 and P2 are the only name-independent exact forms. Project precedents for a load-bearing `= {0}` that fixes .bss layout: rasterizer_lights (cdc8ebd3), and the player_ui player1_profile_path `= { 0 }` (f7cd2e72); source/ has 69 column-0 static `= {0}` definitions. P1 adds no new names, and program semantics are unchanged. P2 keeps HCEX's first-party static-local storage for default_function_values.

**Evidence against.** HCEX.pdb and HCEX_Release.pdb record default_function_values as a Static Local of render_model, not a file static. HCEX -lines (the +2-line gaps at 124/128..154/158) and the /Od profile-scope sizes show all four declared inside their if-blocks, uninitialised. The P1/P2 declaration order is the exact reverse of the use and in-block order, with no first-party support. The strip test fails (the redundant-zero-initialiser class). The standing rejection's reopen clause forbids matching-only constructs. Three of the four names are third-party (halocea).

**Still blocked / integrator notes.** If the owner says NO: models stays NonMatching. The first-party SL form gives offsets 32/48/96/0 vs January's 0/16/64/104, because January's order depends on the decorated-name hash of three lost names, and hash-steering names are forbidden. Not built in this wave (it would be a new form): P2 with fv inside its if-block, which by the law should lay out identically.

**Measured.** Rebase: models.c/h are unchanged since 954eebd2. The wave-3 C1 patches apply clean and reproduce new/models.{c,h}; OWNER_OPTION_P1 reproduces new/models_owner_option_P1.c. No real change was needed. The packets were rebuilt as standalone per-file patches, because a stacked models.c pair fails `git apply --check` in-repo under core.autocrlf. SL and P2 are the wave-3 lab L1 and P2 forms rebased on C1 (the diff vs the labs is the C1 hunks only). csplit: split_mC1, split_mSL and split_mP2 each change only models.obj. Gate --all is 12/12 EXACT for every candidate; render_model is identical in all four (2144 B, sha bb666c87, 98 relocs). object_audit: production FAIL(4) (offsets 72/88/32/0); C1 FAIL(4); P1 PASS 30/0 (0/16/64/104); P2 PASS 30/0 (0/16/64/104); SL FAIL(4) (32/48/96/0). pdb storage is 0 for C1, P1, P2 and SL (production has 2). surplus_identity: _negate_vector3d and _set_real_point3d are IDENTICAL and pre-existing. provider_link PASS (11 rows) and fake scan 0 for every candidate. models.h shared-header sweep (shadow header, #error control failed every compile, so the shadow was used): first_person_weapons 33/34 (same residual), model_animations 32/32, objects 121/121, render_objects 22/22, render_sky 1/1, all IDENTICAL between the current and C1 headers. objdiff 3.3.1 mini report: 5038/5038 code and 2368/2368 data for every candidate (credit-neutral). The combined hwgeom+models symbols.json changes are independent (csplit), and verify_apply.py passes ALL OK.

## 9. dead_camera _dead_camera_update: authentic uninitialised next_unit_index read (source/camera/dead_camera.c)

- **Question:** Can source/camera/dead_camera.c declare `long next_unit_index;` with no initialiser and no else arm, under the original-bug policy? That deletes the current defined stand-in `else { next_unit_index = (long)result; }` and its comment. When no next player is found, the test then reads the indeterminate local. The methodology-template BUG comment in RULING.md would be added. YES/NO.
- **Would complete:** source/camera/dead_camera (4/4 strict, 6/6 sections, 6/6 symbols, PDB storage 0, surplus identical, provider_link PASS 19/19, data 100%)
- **Functions gained:** _dead_camera_update (1,248 padded / 1,235 meaningful)
- **Packet:** `research/fifty_objects_20260925/w/owner_queue/ub_dead_camera_saved_game`
- **Apply-ready at cdc8ebd3:** yes

**Evidence for.** - January +0x176 `mov eax,[ebp+0x10]` is the fallback of the phi, and the slot is never written before it. Production's defined stand-in differs by exactly this one instruction (`mov eax,esi`).
- n1 20260915: every defined initialiser costs an extra store that VC7 never elides.
- The later /Od build (0x522c90) adds `= NONE` (mov [ebp-8],-1) at the top of the block with no else arm, i.e. the fix was an initialiser. The HCEA donor also uses -1.
- /W4 C4701 on the candidate.
- Landed precedent: ai_communication.c:2527 (7a1d0d91) leaves cause_point uninitialised under the same BUG template.
- The whole-object audit is clean.

**Evidence against.** - Earlier decisions rejected this exact read: dead_camera_obj_codex_checkpoint ('Recreating the uninitialized read is forbidden'), legacy_residual_donor_truth_audit_20260913 (rejected the 1,235-byte donor), and opus5_150k_w1 (skipped as UB). They were taken under briefs that forbade UB, not as owner rulings under the original-bug policy.
- The read is formally undefined C. The current stand-in is defined and has the same January runtime value.
- There is no _RTC_UninitUse attestation, because the /Od build contains the fix.
- House rules 20 and 21 and the source-credibility gate allow only the narrow original-bug exception.

**Still blocked / integrator notes.** Nothing technical. Only the owner ruling is outstanding. The integrator must run a full ninja and the stable sweep (workers may not run ninja), then flip config.json:743 to Matching. There is no park to retire.

**Measured.** Patch 01 (CRLF form, plus an index_lf form) passes `git apply --check`, alone and together with 02/03, in worktree and --cached modes, at both cdc8ebd3 and eb33baec. Applied outside the repo, it reproduces landing.c byte for byte.

The code change is the same as the held owner_update_bug.patch. Only the comment moves to the methodology BUG template, and it is byte-inert: 26/26 sections and all symbols are identical to the held form.

- Gate --all: production 3/1/0; candidate EXACT 4/4 (new 288, update 1248, next_player 176, has_allies 112).
- head.obj equals build/base. The candidate differs only in the _dead_camera_update section.
- object_audit against the current split: PASS. No symbols.json change is needed; the _dead_camera_constants static landed in batch 1 and the split already has storage 3.
- pdb_storage: 0 disagreements.
- surplus_identity: 8 COMDATs, all identical to January's selected copies. All are already emitted by production.
- provider_link: PASS 19/19 in both orders. The wave-1 _valid_real_vector3d_axes2 FAIL is gone.
- Data: .rdata 48/48. fake_match_scan: 0 leads. /W3: clean.
- /W4: exactly one new warning, C4701 on next_unit_index (the compiler confirms the disclosed path).
- January bytes: [ebp+0x10] is only read before +0x176, so January deterministically uses the caller's result pointer.

## 10. saved_game_files _saved_game_files_enumerate_available_to_local_player_index: indeterminate count stored on saved-game-files mutex failure (source/saved games/saved_game_files.c)

- **Question:** Can number_of_available_profiles be declared without an initialiser and zeroed only after count_enumerated_profiles_in_mapfile(), inside the successful general-mutex branch? On mutex failure, *number_of_profiles then receives an indeterminate value (January stores the low word of the caller's player_index). The held BUG comment would be added, and this reverses the 20260904/20260908 rejection recorded in the parked.json park. YES/NO. (Patch 02 is a zero-credit storage fix that needs no ruling.)
- **Would complete:** source/saved games/saved_game_files: strictly exact 46/46 and audit-exact against the emulated split. Clean completion is CONDITIONAL: 3 PDB-storage disagreements remain unless the lab helper-storage follow-up is reviewed and landed, or the owner accepts the three functions as external.
- **Functions gained:** _saved_game_files_enumerate_available_to_local_player_index (368 padded / 359 meaningful)
- **Packet:** `research/fifty_objects_20260925/w/owner_queue/ub_dead_camera_saved_game`
- **Apply-ready at cdc8ebd3:** yes

**Evidence for.** - January bytes show the count zeroed only after the count call, inside the success branch. The failure tail stores the untouched player_index home.
- The /Od 0x8646c0 build zeroes the accumulator twice: at the declaration (the later fix) and after the count, which is January's position.
- Every defined alternative was already refuted from the bytes (checkpoint 20260908, n1 20260915, Lane D 20260922). The only defined source that matches would reuse player_index as the counter, which contradicts /Od and makes no sense as code.
- /W4 C4701.
- ai_communication.c:2527 precedent.
- Clean gate, audit and link results. Header blast radius 0.

**Evidence against.** - The config/parked.json park explicitly records 'reproducing that undefined behavior is rejected even though it makes the section strict exact'. The same position appears in claude_fable_opus_reconciliation_checkpoint_20260908 and saved_game_files_obj_opus_reconciliation_20260904.
- The indeterminate value escapes through an out-parameter the caller can see, which is harder to defend than dead_camera's internal read.
- Rule 17 (combine declaration and initialisation) is locally reversed. /Od attests only the late zeroing position.
- There is no _RTC_UninitUse attestation, because the /Od build contains the fix.

**Still blocked / integrator notes.** 1. The owner ruling on 03. Patch 02 can land without a ruling.
2. Whole-object COMPLETE under brief section 4, step 3: 3 PDB-storage disagreements remain (_saved_game_files_take_mapfile_mutex, _saved_game_files_release_mapfile_mutex, _saved_game_files_enumerate_default_files).
   - /Od attests the mapfile-helper calls at the 6 hand-expanded sites.
   - enumerate_default_files has no /Od call-site attestation.
   - The fix is a real source change, preserved as lab/LAB_ONLY_saved_game_files_helper_storage_on_top_of_03.diff. It needs its own review. Alternatively, the owner accepts the three as external.
3. Do not flip config.json:398 to Matching on the ruling alone.
4. The integrator must run a full ninja and the stable sweep, then retire the enumerate_available park via tools.campaign.unpark.

**Measured.** Source is unchanged since wave 1. Patches 02 (storage; its +/- lines equal the held production_storage.patch) and 03 (the held production_bug.patch rebased onto 02) pass --check in both EOL forms, alone and together, at cdc8ebd3 and eb33baec. Applied sequentially outside the repo, they reproduce the candidate .c, the header and symbols.json byte for byte.

- Gate --all: production 45/1; 02 only 45/1 (storage flip only); 03 only 46/46; 02+03 46/46. Shadow headers were verified with an #error probe.
- csplit emulation: the control run reproduces 833/833 split objects. The edited symbols.json (line 7376, in place) changes only saved_game_files.obj, and only the storage of _enumerate_memory_units.
- object_audit, candidate against the emulated split: PASS (152 symbols, 0 differ).
- surplus: 0 code COMDATs. provider_link: PASS 11/11 in both orders. Data 100% (.bss 540, .data 8, .rdata 5608).
- Header blast radius on 6 includers (game, player_ui, ui_widget, virtual_keyboard, player_profile, playlist_profile): identical gate rows, sections and symbols.
- fake_match_scan: 0 leads. /W3: unchanged at 33. /W4: one new C4701 on number_of_available_profiles.
- January bytes re-verified: +0x91 spill, +0x94 xor edi, and the +0x156 `mov dx,[ebp+8]` failure tail.
- pdb_storage: 3 disagreements remain (take/release_mapfile_mutex, enumerate_default_files). The wave-1 'proven external' dismissal is refuted by law (h).
- LAB variant (static helpers called at the hand-expanded sites): 46/46, sections identical, PDB storage 0, audit PASS against a lab split, link PASS, blast radius 0.

## 11. first_person_weapons patch B (authentic unguarded NULL dereference of first_person_weapon_animations; rebased onto cdc8ebd3 after patch A landed in b9a8d587)

- **Question:** May _first_person_weapon_update read first_person_weapon_animations->animations.count without a NULL test at two sites (moving animation and overcharged jitter)? The pointer comes from `count ? TAG_BLOCK_GET_ELEMENT(...,0,...) : NULL`. The patch also drops REAL_MATH_EXTERNAL_{SQUARE_ROOT,MAGNITUDE_SQUARED3D,MAGNITUDE3D}, so magnitude3d inlines as in January and the TU emits three header __inline COMDATs from a strictly exact caller. The rule-6 exception conditions hold.
- **Would complete:** source/interface/first_person_weapons
- **Functions gained:** _first_person_weapon_update (1,533 meaningful / 1,536 padded)
- **Packet:** `research/fifty_objects_20260925/w/owner_queue/ub_fpw_player_profile`
- **Apply-ready at cdc8ebd3:** yes

**Evidence for.** January bytes: `xor eax,eax; jmp`, then `cmp dword [eax+0x10],3/0xf` with no test, at both sites. The later /Od build (0x61fcea and 0x61fd99) also reads without a NULL test. The HCEA 2011 decompiled source (first_person_weapon_update.c, around lines 138-145) has the same unguarded read. With the guards restored, the only difference from January is 4 inserted instructions, so no defined spelling can match. The path is latent: switch_weapons commits weapon_index only when first_person_weapon_animations.count is nonzero. BUG-commented authentic defects already exist in 18 source files, including uninitialised reads.

**Evidence against.** The source dereferences a pointer that the statement before it may set to NULL, so house rule 21 ('undefined/nonsensical logic') applies literally even though the path is unreachable. The defined-behaviour body is already in the tree at zero credit (park 95.21506). Refusing costs one function.

**Still blocked / integrator notes.** Nothing technical. The item needs only the owner's yes on the authentic-defect class. The integrator must then run the full ninja build and the stable sweep; only this TU changes.

**Measured.** Patches 01_fpw_B_source, 02_fpw_B_retire_park and 03_fpw_B_status_matching pass git apply --check at cdc8ebd3 (and on eb33baec, a docs-only commit that landed during the run). Tree: gate 33/1. With B: gate --all 34 exact / 0 residual / 0 unwritten, _point_from_line3d guard passed. No split regeneration is needed: a control csplit from cdc8ebd3 config matches build/split on all 833 objects. object_audit against the emulated split PASSES, with 45 January symbols and 0 differences. The only new surplus is _magnitude3d, _magnitude_squared3d and _square_root. All three are IDENTICAL to January's selected copies (action_charge, action_alert) and have 0 NODUP definers; provider_link passes both orders for all 12 surplus symbols. pdb_storage: 0 disagreements. Data 536/536. objdiff 3.3.1 against the emulated split: 29/34, with update at 100 (tree 28/34, update 95.21506). The other 5 functions are objdiff 3.3.1 false negatives already credited by semantic_matches.json or the accepted_ledger, and all 5 are gate-EXACT, so the strict view is 34/34. fake_match_scan finds the same single authenticated lead as the tree. /W3 shows 0 TU-local warnings; the one extra line is the existing real_math.h(810) warning. No header is touched, so there is no blast radius. January bytes confirm both unguarded reads (0x23e/0x255 and 0x28d/0x2a5). The BUG comment's claim that the path is unreachable was checked in switch_weapons.

## 12. player_profile R1-R3 (uninitialised serialised 512-byte block; seek failure skips file_close; close(&file, NONE) after delete-while-open), rebased onto cdc8ebd3 after zero-credit packet A landed in 6e3e2d35; R3 BUG comment amended

- **Question:** Does the owner reverse the 2026-09-09 saved-game rejection and admit all three January failure-path defects with BUG comments? R1: the `union player_profile_block block;` in write_thread_proc and create_default has no initialiser, so 444 stack bytes are written to disk. R2: create_default folds file_set_position into the open condition, so a failed seek leaks the file. R3: player_profile_new deletes the file while it is still open, sets the index to NONE, then calls saved_game_file_close(&profile_file, NONE), which fails its assertions. All three are required for the object.
- **Would complete:** source/saved games/player_profile
- **Functions gained:** _player_profile_create_default_profiles_on_disk (275/288); _player_profile_write_thread_proc@4 (349/352); _player_profile_new (456/464)
- **Packet:** `research/fifty_objects_20260925/w/owner_queue/ub_fpw_player_profile`
- **Apply-ready at cdc8ebd3:** yes

**Evidence for.** Since the 2026-09-09 rejection, all three shapes have first-party /Od attestation. R1: 0x8596d0 and 0x85acf0 have no initialiser on block. R2: 0x85ae02 has the seek inside the open condition. R3: 0x85d1fa sets NONE, then 0x85d20c runs the shared close. HCEX.pdb gives the union player_profile_block type and the profile_file/full_path local names. No defined spelling can reproduce the bytes; earlier waves measured the alternatives. BUG-commented authentic defects already exist in the tree, for example the uninitialised cause_point read in ai_communication (7a1d0d91).

**Evidence against.** The owner already rejected exactly these three shapes on 2026-09-09. R1 discloses 444 stack bytes into a saved file. The /Od build attests only the missing initialiser: in the later format the block is fully overwritten, so the disclosure is January-format-specific. R3 passes NONE to an asserted API and deletes a file that is still open. R2 leaks a handle on seek failure. House rule 21 applies.

**Still blocked / integrator notes.** Nothing technical. The object needs yes on all three of R1, R2 and R3; with a partial yes it stays incomplete (R1 blocks write_thread and create_default, R2 blocks create_default, R3 blocks new). Disclosed non-blockers: read and get_from_path keep `byte block[512]`, although HCEX types read's local as the union; the globals type names stay descriptive unless optional 04a is used. The integrator must then run the full ninja build and the stable sweep.

**Measured.** The old owner_gated_over_production.patch still applies at cdc8ebd3 and reproduces final_og.c exactly. The R3 BUG comment was amended to state the delete-while-open; this is comment-only and changes 0 of 60 sections. Patches 04_pp_R1R3_source, 05_pp_retire_parks and 06_pp_status_matching pass git apply --check, individually and together with 01-03. Tree: gate 19/3. Candidate: gate --all 22 exact / 0 residual / 0 unwritten, guard passed, with no split regeneration needed. object_audit PASSES, with 51 January symbols and 0 differences. The surplus list is identical to the tree's (no new surplus, 0 code COMDATs), and provider_link passes both orders for all 8 surplus symbols. pdb_storage: 0 disagreements. Data 1644/1644. objdiff 3.3.1: 100.0, code 4119/4119, 22/22 (tree 95.34539). fake_match_scan: 0 leads. /W3 is identical to the tree, with 0 TU-local warnings. No header is touched. January bytes were re-read: R3 at 0x151 delete, 0x159 `or esi,-1`, 0x164 close(&file, esi); R2 at 0xa3 `je 0xec`, which skips file_close; R1 has no fill before file_write(0x200). There is also an optional variant with the byte-inert HCEX type names advised by reviewer 3: optional/04a, then 04b, applied as two separate git apply commands. It gates 22/22 and all 60 sections are identical.

## 13. game_engine_king: _find_next_hill keeps January's uninitialised no-candidate return (next_hill), plus the cachebeta storage fix, for whole-object admission

- **Question:** Do you admit game_engine_king.obj as a complete object when _find_next_hill keeps January's uninitialised no-candidate return, disclosed by the policy-form 'BUG (preserved for exact matching)' comment? If yes, which spelling: 01, the review2 packet's early-return form, or 01A, the /Od 0x5bd630 single-exit form (`next_hill = king_engine_hills[hill_index]; break;` then one `return next_hill;`)? The two produce byte-identical objects. A NO applies only if_declined/king_source_storage_only.patch + 02 at zero credit, but it leaves open whether the function credit production already takes for this read (_find_next_hill, 112 B, since 2026-09-02) should stand.
- **Would complete:** source/game/game_engine_king
- **Packet:** `research/fifty_objects_20260925/w/owner_queue/ub_king_glow/king`
- **Apply-ready at cdc8ebd3:** yes

**Evidence for.** January bytes: at +03, push ecx reserves [ebp-4], which is never written; at +52, mov eax,[ebp-4] returns it on the loop-exhausted path.

The later first-party /Od build (0x5bd630) sets its RTC init flag only on the found path, then calls _RTC_UninitUse with the string "next_hill". So the local's name and the read are both in the later source. The function is single-exit there.

Playbook 'Original bugs' gate: January proves the read, an independent build corroborates it, and every defined form measured is 28/29 (5 at 931ed8dc, 1 at cdc8ebd3).

The function and the whole object reach strict exact with a clean section-4 audit.

Owner precedents (2026-09-20):
- ai_script.c is Matching and carries an owner-admitted read of an unset stack slot: a missing vsprintf argument makes it read the saved-EDI home (ai_script.c:2765).
- _actor_emotion_update: an out-of-bounds read admitted at 8e4309dd.

The read has been in production since 2026-09-02; the ruling certifies it, it adds no behaviour.

**Evidence against.** Brief sections 2 and 7 make this class owner-gated, and dead_camera is still held on the same class.

On 2026-09-20 the owner excluded the uninitialised-read donors _dead_camera_update and _compare_profile_sections (astra rejected-hypotheses, item 8). Those were not /Od-authenticated, but the class is the same.

King would be the first Matching object with a load-bearing C4700.

In the packet form (01), next_hill is never assigned, so a reviewer could read it as a local whose only role is the match. The /Od single-exit form (01A) answers this: it is byte-identical and raises no C4700.

Behaviour: on a map with one distinct hill and moving hills enabled, January stores garbage in king_globals.hill_id. The corrected build keeps the current hill.

**Still blocked / integrator notes.** Only the owner ruling. On a YES the integrator still has to run the full ninja, the stable function diff, parks, the admission audit and pytest. The header edit reaches only king.c. Nothing technical is open.

**Measured.** Tree: HEAD eb33baec, which is cdc8ebd3 plus one ledger-only commit; clean before and after.

Rebase: the review2 OWNER_GATED source patch applied unchanged. The result is review2's owner_bug_comment.c plus exactly the two f7cd2e72 game_variant renames. The semantic_data_matches patch failed (b9a8d587 had appended entries), so I regenerated it. All patches are regenerated as canonical git patches 01-05, plus alt/01A and if_declined/.

Apply checks: git apply --check passes on each patch, on 01-05 combined, and on king + glow combined. --whitespace=error-all is clean. Trial-applying onto tip copies reproduces king/new byte-for-byte (CRLF), and the JSON parses.

Gates (shadow patched header; a #error probe proved the patched header is the one used):
- 01: 29/29 EXACT against build/split and against the emulated split (csplit with the edited symbols.json). The emulated split differs from build/split only in game_engine_king.obj.
- 01A and storage-only: also 29/29. Their objects have 59 sections, 0 section differences and 0 symbol-row differences against 01.
- Only warning: C4700 next_hill. The /Od single-exit form raises none.
- Defined result in the /Od shape (next_hill = hill_id): 28/29, so the read stays load-bearing.

Audit and linkage:
- object_audit against the emulated split: PASS, 39/39 January symbols, .data 136/136 and .bss 560/560. The controls FAIL(23) on storage, as expected.
- pdb_storage: 0 disagreements (production has 23).
- Surplus: 22 sections, all identical to January's copies. provider_link: 22 rows, PASS in both orders. 0 NODUP definers.
- 0 UNDEF references to the 23 re-storaged names in build/base (621 objects) or build/split (833).
- The header is included only by king.c. fake_match_scan: 0 leads.
- The _king_engine data entry measures 136 / 23 / 4009f5b6... with target == base in 8 of 8 combinations.

Emulated chain (objdiff 3.3.1; the control mini row equals the production row):
- Halo complete units 379 -> 380; complete_code +4,865; complete_data +876; matched_data +136.
- The data credit validates without allow_incomplete_unit. The .bss ownership snapshot validates.
- Revoked []; parks 86/0/0; admission 11/0/2/0 with 0 contradicted.
- With glow also applied: 381.

## 14. glow _get_particle_world_position: two uninitialised stack-array reads (traversal_left_vectors never built on the 2- and 3-marker paths; traversal_time_indices[1] never set on 3-marker interval 1)

- **Question:** Do you admit glow.obj as a complete object when get_particle_world_position reproduces January's two uninitialised stack-array reads? The first is that the 2- and 3-marker paths never build traversal_left_vectors, which spline 3 reads. The second is that 3-marker interval 1 writes its midpoint knot into [2], so [1] is read unset. Each carries a policy-form BUG comment. These are in-bounds reads of automatic arrays with no struct-layout dependency, so do you also waive the compile-time layout assertion that the 2026-09-20 ruling attached to _actor_emotion_update? On a NO nothing is applied and the current defined-C park (2528/55, 49.07%) stays; review2 rejected the only defined fallback.
- **Would complete:** source/objects/widgets/glow
- **Functions gained:** _get_particle_world_position (1,744 padded / 1,729 objdiff bytes)
- **Packet:** `research/fifty_objects_20260925/w/owner_queue/ub_king_glow/glow`
- **Apply-ready at cdc8ebd3:** yes

**Evidence for.** Both defects are target-proven by January's control flow. The later first-party /Od build (0x798de0) contains both verbatim:
- Its RTC descriptor names traversal_left_vectors [ebp-0xa8], which only the default loop's cross_product3d writes.
- In case 1, both the time_indices stores (0x799533 and 0x799639) go to [2].

HCEX.pdb gives the same local names.

The bounded defined search fails: n3a v3 is +592 B, P8 has 71 REAL regions, and this wave's single-defect repairs are each 24/1.

The function and the object become strict exact with a clean section-4 audit.

Owner precedents (2026-09-20): Matching ai_script.c has an admitted unset-stack-slot read, and _actor_emotion_update's out-of-bounds read was admitted at 8e4309dd.

The reads have no layout dependency, so no assertion is meaningful.

**Evidence against.** The class is owner-gated (brief sections 2 and 7), dead_camera is held on the same class, and the owner refused the 2026-09-20 uninitialised donors (dead_camera, compare_profile_sections).

The behaviour is real and visible: glows with 2 or 3 markers use stack garbage for the left offset (v*cosine), and on 3-marker interval 1 the second spline knot is garbage.

The packet adds point<->vector view casts. They meet the brief's rule (/Od-attested per site, compatible layout, byte-inert strip, strictly exact caller, disclosed) but still need to be admitted.

Byte-inert nits: NUMBEROF vs the /Od signed literal 4; the locals sine/cosine shadow the real_math.h inlines; the BUG comment's range is end-exclusive (correct as written).

**Still blocked / integrator notes.** Only the owner ruling, including the layout-assertion waiver. On a YES, 01, 02 and 03 must land together, followed by the integrator's full ninja, stable diff, parks, admission audit and pytest. Only glow.c changes; no header.

**Measured.** Rebase:
- glow.c/.h are untouched since e9e62b78. The glow.c and parked.json parts of wave-2 production.patch apply unchanged.
- The rebased glow.c == scratch/w/glow/cand_final.c byte-for-byte.
- The config.json hunk failed (b9a8d587 flipped light_volumes.c two lines above), so I regenerated it as a one-line status flip.
- Patches 01-03 are canonical git patches. git apply --check passes on each, on 01-03 combined, and on king + glow combined. --whitespace=error-all is clean. Trial-applying reproduces glow/new byte-for-byte.

Gates (gate.py --all --forbid-emitted-symbol _point_from_line3d):
- Packet: 25/25 EXACT; _get_particle_world_position EXACT 1744.
- Tip control: 24/1, [size 2528!=1744, relocs 55!=43, sha].
- 0 TU-local /W3 warnings.
- objcmp tip vs packet: 1 of 56 sections differs (this function); 0 symbol-row differences.

Strip tests at cdc8ebd3:
- Knot repaired only: 24/1 [sha].
- Left vectors = {0} only: 24/1 [size 1776!=1744, sha]. Each defect is therefore independently load-bearing.
- index < 4: 25/25 (byte-inert).
- review2's no-casts variant: 25/25, but raises C4133, so the casts are type correctness, not decoration.

Audit and linkage:
- object_audit: PASS, 36/36 symbols; the surplus list is identical to production's.
- Surplus: 20 sections, all identical to January's copies. provider_link: 20 rows, PASS in both orders. 0 NODUP definers.
- pdb_storage: 0 disagreements. Data .rdata 392/392; no data entry is needed.
- January bytes re-read: left vectors live at [ebp-0xc4, ebp-0x94). They are written only by the default loop (+0x21b/+0x23e/+0x259) and read by the spline-3 leas (+0x637..+0x64c). Case 1 jumps to +0x5c6, which stores [ebp-8]; +0x5d2 then reads [ebp-0xc] unwritten.
- /Od RTC names confirmed.

Emulated chain:
- Halo complete units 379 -> 380; complete_code +6,987; complete_data +392; matched_code +1,729; matched functions +1.
- Parks 86 -> 85 (0 stale or invalid); revoked []; admission 11/0/2/0.
- Applying 01 without 02 fails progress with 'parked-function evidence is stale or invalid'.
- With king also applied: 381.
