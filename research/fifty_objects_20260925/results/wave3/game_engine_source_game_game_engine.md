# source/game/game_engine

## verdict
OBJECT_COMPLETE_CANDIDATE

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_clean\\final\\source\\game\\game_engine.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_clean\\final\\source\\game\\game_engine.h",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_clean\\final\\source\\game\\player_control.h",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_clean\\final\\source\\interface\\hud_messaging.h",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_clean\\final\\source\\interface\\hud_nav_points.h",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_clean\\final\\source\\interface\\hud_nav_points.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_clean\\final\\source\\interface\\hud_globals_definitions.h",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_clean\\final\\source\\items\\item_collection_definitions.h",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_clean\\final\\source\\scenario\\scenario_netgame_definitions.h",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_clean\\final\\source\\sound\\sound_classes.h",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_clean\\patches\\01_game_engine_c.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_clean\\patches\\02_new_focused_headers.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_clean\\patches\\03_shared_header_prototypes.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_clean\\patches\\04_hud_nav_points_owner_include.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_clean\\patches\\CONSUMERS.txt"
]

## production_changes
Apply these patches in order from C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\game_engine_clean\patches\. Each passes `git apply --check` on its own and together with the others against 954eebd2. The full CRLF files are under ...\game_engine_clean\final\source\...

1. 01_game_engine_c.patch changes only source/game/game_engine.c (full file: final\source\game\game_engine.c).
   - (a) Removes the local views `rasterizer_debug_options`, `game_engine_hud_globals`, `game_engine_postgame_hud_definition`, `scenario_netgame_flag`, `scenario_netgame_equipment`, `scenario_starting_equipment`, `item_collection_permutation` and `item_collection_definition`, their verify typedefs, the two local tag macros and the two local externs.
   - Uses the genuine names instead:
     - `rasterizer_debug_options.pad3` (the hs global "pad3" at +0x32).
     - `hud_globals->messaging.single_player_font.index` / `multi_player_font.index` for the old 0x54/0x64 views.
     - `hud_globals_definition_get(...)->carnage_report_bitmap.index` for the old 0x3D4 view.
     - HCEX member names `game_type`, `spawn_time`, `item_collection(.index)`, `item.index`, and `struct item_permutation_definition`.
   - (b) Removes all 18 consumer-local prototypes and adds includes: bitmaps/bitmap_group_lookup.h, effects/player_effects.h, game_allegiance.h, game_engine_list.h (also drops the local `extern game_engines[]`), interface/hud_globals_definitions.h, interface/hud_nav_points.h, interface/player_ui.h, items/item_collection_definitions.h, main/console.h (this fixes the C4013 implicit `console_printf`), players_runtime.h, rasterizer/rasterizer.h, rasterizer/rasterizer_debug_options.h, scenario/scenario_netgame_definitions.h, sound/sound_classes.h.
   - Call sites now use the genuine signatures:
     - `rasterizer_draw_unicode_string(&bounds, NULL, NULL, 0, s)`.
     - `short render_type = hud_get_nav_point_render_type(local_player_index, &head_position, &goal->position, NONE)`.
     - `custom_render_nav_point(local_player_index, &goal->position, goal->nav_point_index, render_type)`.
     - `game_engine_render_nav_points(short local_player_index)` (HCEX-attested; drops two `(short)` casts).
   - (c) `real get_blink_alpha`.
   - (d) Removes both stale NonMatching comments and replaces the 40 `_code_XXXXXXXX` names in the symbol comment with their symbols.json names.
   - `handle_custom_starting_equipment` is rewritten in the /Od 0x5b25d0 shape: a for loop over the block with the grant and `break` inside the matching entry, then an index loop to 5, in /Od declaration order. A comment notes that January visits 5 of the 6 item collections.
2. 02_new_focused_headers.patch adds four new files:
   - interface/hud_nav_points.h: find_nav_point, hud_get_nav_point_render_type, custom_render_nav_point.
   - interface/hud_globals_definitions.h: the HCEX hud_globals_definition and its nested messaging, waypoint, multiplayer, defaults, damage_indicators and timer types; `HUD_GLOBALS_DEFINITION_TAG 'hudg'`; `hud_globals_definition_get`; `extern hud_globals`. The shared element types come from unit_hud_interface_definition.h.
   - items/item_collection_definitions.h: HCEX item_permutation_definition and item_collection_definition, plus `item_collection_definition_get`.
   - scenario/scenario_netgame_definitions.h: HCEX scenario_netgame_flag, scenario_netgame_equipment and scenario_starting_equipment.
   - Consumers: only game_engine.c, plus hud_nav_points.c through patch 04.
3. 03_shared_header_prototypes.patch is the SHARED-HEADER patch:
   - game_engine.h: every `float` becomes `real`; `game_engine_render_nav_points(short)`; adds game_engine_update_multiplayer_sound and game_engine_intialize_queued_sounds next to game_engine_play_multiplayer_sound.
   - hud_messaging.h: adds hud_get_font_index, hud_get_text_color and `union real_argb_color;`.
   - sound_classes.h: adds sound_class_set_gain.
   - player_control.h: adds unit_get_local_player_index.
   - Consumer TU lists are in patches\CONSUMERS.txt: game_engine.h 36, hud_messaging.h 7, sound_classes.h 5, player_control.h 21.
4. 04_hud_nav_points_owner_include.patch: hud_nav_points.c includes its new header and drops its duplicate custom_render_nav_point prototype.

Optional, measured inert, not required for admission:
- 05_optional_players_teleporter_index_h.patch: players.h `unknown70` -> `teleporter_index`.
- 05_optional_players_teleporter_index_c_after01.patch: the matching game_engine.c edits; applies after 01. Item (e) was report-only.
- 06_optional_sibling_netgame_flag.patch: ctf, king, oddball and race drop their local scenario_netgame_flag and include the focused header.

No symbols.json, config or parked.json change is needed.

## evidence
All results are recorded in scratch\w\game_engine_clean\LEDGER.md.

Final object:
- final_game_engine.obj, a shadow-tree compile of final/, is 180/180 EXACT (gate_final.txt). Every section is identical to the production object build/base/.../game_engine.obj.
- object_audit PASS (audit_final.txt, identical to the baseline audit).
- The symbol table is identical to production apart from the internal $L label numbers, so PDB storage is unchanged (0 disagreements with cachebeta publics).
- 11 surplus COMDATs, all identical. SELECTED-PROVIDER LINK PASS (plink_final.txt).

Layouts and signatures come from HCEX.pdb -type/-sym dumps (hcex_types*.txt, hcex_funcs.txt):
- netgame flag 0x94, equipment 0x90, starting equipment 0xCC, item permutation 0x54, hud_globals_definition 0x450.
- Every offset the code reads matches January's bytes: 0x54/0x64 are single/multi_player_font.index, 0x3D4 is carnage_report_bitmap.index, 0x5C is item_collection.index, 0x48+0x10k is item_collection[k].index.
- The nav-point signatures agree with the hud_nav_points.c definitions.
- game_engine_render_nav_points takes a short in HCEX.

Oracle (lab only): game_engine.c's rows do not move with 0-500 dummy names, so the TU has no count ties.

Header placement was measured before the focused headers were chosen:
- Adding the three nav prototypes to hud.h (+16 names) moves main's parked `_main_update_time`. main moves at k=9 and at every k from 15 to 30.
- Adding the types to scenario_definitions.h and item_definitions.h regresses units `_unit_preprocess_node_orientations` and moves the ai, hud_weapon and main parks.
- The final packet was then swept across the whole board in the shadow tree: 440 source TUs, 0 changed sections, 0 regressions, 0 park drift. Consumer TUs show no new /W3 warnings.

/Od 0x5b25d0 attests the handle_custom_starting_equipment shape, and that rewrite is byte-identical.

## blockers
None remain for admission, provided the reviewer accepts the focused-owner-header placement.

The genuine broad headers could not take these types and prototypes:
- hud.h: +16 names moves the main park.
- scenario_definitions.h and item_definitions.h: they regress the units count sentinel and move 3 parks.
- hud_definitions.h and hud.h cannot take hud_globals_definition without rewriting the 3-5 TUs that define their own copy (hud_nav_points, hud_unit, ui_widget; hud.c, hud_messaging.c).
- The `extern hud_globals` cannot go to hud.h because main.c and interface.c declare it with other local types.

The precedent is the narrow owner headers already in the tree: bitmap_group_lookup.h, players_runtime.h, player_control_runtime.h.

## reopen_criteria
If the owner rejects the focused headers, consolidate the hud tag types board-wide into hud_definitions.h, retiring the local copies in hud.c, hud_messaging.c, hud_nav_points.c, hud_unit.c, ui_widget.c, hs.c and hs_compile.c. The scenario and item types can move into scenario_definitions.h and item_definitions.h once the declared-name gap there is recovered, so that the units sentinel and the ai, hud_weapon and main parks stay put. Rerun sweep.py --all afterwards.

## task notes
Disclosures for the reviewer and integrator:

1. Loop count 5 of 6. The HCEX starting-equipment layout has 6 item collections, but January's bytes and the /Od build both loop 5 (+0x48..+0x88). Production already looped 5 over a 5-entry view. The code keeps 5 and a comment says so. This is not UB and not new behaviour, but it is visible now.

2. Remaining debt in other TUs. hud.c still defines hud_globals with its own incomplete local struct. hud_messaging.c, hud_unit.c, ui_widget.c, hs.c and hs_compile.c keep local hud views; that is pre-existing debt and out of scope here.

3. game_engine_multiplayer_sounds.c does not include game_engine.h. That was already true for game_engine_play_multiplayer_sound.

4. Measurement tooling:
   - Shadow-tree sweep: scratch\w\game_engine_clean\sweep.py, with deps.json from CL /showIncludes. The full-board run is sweep_final_all.log; the 117 libs/* compile errors there are because only source/ was copied into the shadow tree.
   - Reproducible builders: make_cand.py (layers A B2 Bshort C2 C3 D E) and make_shadow.py (HB2 HF HS HC2 HD HNP_owner, plus optional HP and HC_sib).

5. No tracked file was edited, and ninja, configure and git-mutating commands were never run. The worktree git status is clean.

6. Methodology stop rule: not reached. No residuals were chased; every change is byte-inert.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\game_engine_clean\LEDGER.md
