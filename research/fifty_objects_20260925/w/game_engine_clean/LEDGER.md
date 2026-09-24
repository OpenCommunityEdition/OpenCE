# game_engine_clean LEDGER (wave 3, tree 954eebd2)

Task: make source/game/game_engine admissible: fix house-rule blockers (a)-(e) from
research/fifty_objects_20260925/results/wave2/REVIEW__review_admit2*.md while keeping 180/180.

## P0 baseline
- base.c = copy of source/game/game_engine.c at 954eebd2; gate --all: 180/180 EXACT (gate_base.txt).
- object_audit on base.obj: PASS (audit_base.txt).

## F1 evidence: genuine layouts (HCEX.pdb -type dumps: hcex_type_netgame_flag.txt, hcex_types.txt, hcex_types2.txt)
- scenario_netgame_flag {real_point3d position; float facing; short type; short team_index; long unused[0x20]} = 0x94 (matches view).
- scenario_netgame_equipment {long flags; short game_type[4]; short team_index@C; short spawn_time@E; long run_time_spawned_item_index@10; long unused1[0xB]@14; real_point3d position@40; float facing@4C; tag_reference item_collection@50; long unused2[0xC]@60} = 0x90. View's item_collection_index@5C == item_collection.index.
- scenario_starting_equipment {long flags; short game_type[4]; long unused1[0xC]@C; tag_reference item_collection[6]@3C; long unused2[0xC]@9C} = 0xCC. View's item_collections[k].item_collection_index@48+0x10k == item_collection[k].index.
- item_collection_definition {tag_block permutations; short spawn_time@C; short pad; long unused[0x13]}.
- item_permutation_definition {long unused1[8]; float weight@20; tag_reference item@24; long unused2[8]@34} = 0x54.
- hud_globals_definition {hud_messaging_parameters_definition messaging@0; hud_waypoint_definition waypoint@120; hud_multiplayer_parameters_definition multiplayer@1BC; hud_defaults_definition defaults@2C0; hud_damage_indicators_definition damage_indicators@310; hud_timer_definition timer_definition@360; tag_reference carnage_report_bitmap@3C8; short loading_begin/end, checkpoint_begin/end@3D8..; tag_reference checkpoint_sound@3E0; long unused2[0x18]@3F0}.
  messaging: single_player_font@48 (index@54 = view no_local_player_message_font_index), multi_player_font@58 (index@64 = view local_player_message_font_index).
  view game_engine_postgame_hud_definition.bitmap_group_index@3D4 == carnage_report_bitmap.index.
- rasterizer_debug_options+0x32 is the hs global "pad3" (hs_globals_external.c:1192), genuine header names it pad3.
- Structural finding: struct hud_globals_definition is defined locally in 7 TUs (hud.c, hud_messaging.c, hud_nav_points.c, hud_unit.c, ui_widget.c, hs.c, hs_compile.c); scenario_netgame_flag locally in game_engine{,_ctf,_king,_oddball,_race}.c. No genuine header carries them.

## P1 oracle (lab only): name-count sensitivity of game_engine.c
- probe_count.py inserts K `extern long` dummies after the includes. K = 0..24, 30, 45, 60, 89, 100, 150, 200, 300, 500: ALL 180/180 EXACT.
- Conclusion: game_engine.obj has no count-tied function in this range; in-file declaration changes (removing
  views/prototypes, adding genuine includes) are not expected to flip its own rows. The risk is only in OTHER
  consumers of any shared header I edit.

## P2 Layer A (game_engine.c only, existing genuine headers) -> candA.c (make_cand.py A)
Hypothesis: game_engine.c is count-insensitive (P1), and every prototype below is already declared with an
identical or ABI-identical signature by a genuine header, so swapping consumer-local declarations for includes is
byte-inert.
Changes: include bitmaps/bitmap_group_lookup.h, effects/player_effects.h, game_allegiance.h, interface/player_ui.h,
main/console.h, players_runtime.h, rasterizer/rasterizer.h, rasterizer/rasterizer_debug_options.h; drop the local
struct rasterizer_debug_options view + extern (use genuine `.pad3`, the hs global name at +0x32); drop 9
consumer-local prototypes (global_network_game_server_get + network_game_server_switch_to_postgame were verbatim
duplicates of already-included headers; player_ui_game_variant_specified, rasterizer_draw_unicode_string,
bitmap_group_try_and_get_bitmap, player_effect_screen_flash, game_team_is_enemy, player_get_starting_location(_count)
now from their headers); rasterizer_draw_unicode_string calls use the genuine (bounds, clip=NULL,
cursor_reference=NULL, height_adjust=0, string) signature (HCEX param names agree); get_blink_alpha real; the two
stale NonMatching comments removed.
Found on the way: `CL /Zs /W3` shows C4013 implicit `console_printf` in game_engine_update_teleporter -> fixed by
main/console.h (genuine prototype `void console_printf(boolean clear, const char *format, ...)`).
Result: 180/180 EXACT (gate_candA.txt). Remaining warnings are only pre-existing C4244 narrowing.

## P3 Layer B (+ header packet HB/HF/HS) -> candB.c / candBs.c
Tooling: sweep.py (shadow-tree compile of any TU with production cflags remapped into scratch/w/game_engine_clean/shadow;
compares every section-defining symbol with build/base, flags base_exact->shadow_nonexact); deps.json from
CL /Zs /showIncludes over 557 TUs; make_shadow.py applies header layers with CRLF.
Sanity: unmodified shadow, 3 TUs -> 0 changed sections.
Header facts: hud_nav_points.c's public API already lives in hud.h (hud_initialize_nav_points .. hud_render_nav_points),
hud_get_font_index/hud_get_text_color are HUD_MESSAGING.C, sound_class_set_gain SOUND_CLASSES.C,
unit_get_local_player_index PLAYER_CONTROL.C, game_engine_play_multiplayer_sound (same .c as the two queued-sound
fns) is already declared in game_engine.h. Genuine signatures differ from the consumer-local ones for the two nav
functions: definitions (and HCEX) are hud_get_nav_point_render_type(short, real_point3d const *head,
real_point3d const *position, long reference_object_index) -> short and custom_render_nav_point(short,
real_point3d const *position, short nav_index, short waypoint_type); the old game_engine.c protos passed the goal
struct pointer and a (word) cast. HCEX also has game_engine_render_nav_points(short local_player_index).
- candB (A+B) in shadow with HB+HF: game_engine 0 changed sections vs production (180/180).
- candBs (A+B+Bshort: short param, the two (short) casts dropped; HS header) : 0 changed sections.
- sweepB (HB+HF+HS, 56 consumer TUs of hud.h/hud_messaging.h/sound_classes.h/player_control.h/game_engine.h):
  0 regressions, but main.c's PARKED `_main_update_time` bytes move (park drift). Bisect: HB_hud alone (hud.h +3
  nav prototypes + `union real_point3d;`). HB_pc, HB_ge, HF, HS alone: 0 changes in main.c.
- Oracle (lab, dummy externs in hud.h before prototypes/HUD.C): main.c unchanged for k=1..8,10..14; moves at
  k=9 and every k>=15 tested (15..30). The genuine hud.h addition costs ~16 -> in the moving band.
- Decision: the three nav-point prototypes go to a focused owner header interface/hud_nav_points.h (precedent:
  bitmap_group_lookup.h / players_runtime.h narrow owner headers), included only by game_engine.c.
- sweepB2 (HB2 = hud_messaging.h/sound_classes.h/player_control.h/game_engine.h + new hud_nav_points.h; HF; HS;
  candB2 = A+B2+Bshort): 54 TUs, 0 changed sections anywhere (game_engine 180/180 identical to production).

## P4 Layer C (types): candC = A+B2+Bshort+C; headers HC_scen (scenario_definitions.h +3 HCEX structs, siblings
ctf/king/oddball/race drop their identical local scenario_netgame_flag) + HC_item (item_definitions.h +2 HCEX
structs + item_collection_definition_get)
- game_engine in shadow: 0 changed sections (180/180).
- sweepC (97 TUs = consumers of scenario_definitions.h, item_definitions.h + B2 set): REGRESSION units
  `_unit_preprocess_node_orientations` (the known count sentinel) + park drift ai `_ai_test_ballistic_line_of_fire`,
  hud_weapon `_render_weapon_hud`, main `_main_update_time`. => genuine header placement costs names board-wide.
- Bisect: HC_scen alone moves ai/hud_weapon/main parks; HC_item alone moves hud_weapon's `_render_weapon_hud`;
  together (units includes both) they also regress `_unit_preprocess_node_orientations`.
- Decision: HCEX layouts go to focused owner headers included only by game_engine.c:
  scenario/scenario_netgame_definitions.h (netgame flag/equipment, starting equipment) and
  items/item_collection_definitions.h ('itmc' permutation + collection, item_collection_definition_get).
  Siblings (ctf/king/oddball/race) keep their own complete local scenario_netgame_flag (HCEX-identical layout;
  noted as debt, not touched: including the new header there would be a separate measured packet).

## P5 Layer D (hud views) -> candD.c = A+B2+Bshort+C2+D; headers HB2+HF+HS+HC2+HD
Evidence: HCEX hud_globals_definition (messaging@0 single_player_font@48/multi_player_font@58,
carnage_report_bitmap@3C8, ... 0x450 total) and every nested type dumped (hcex_types*.txt); shared element
types hud_absolute_placement/hud_placement/hud_color_definition already live in the genuine
unit_hud_interface_definition.h ("corroborated by HCEX PDB member records").
struct hud_globals_definition cannot go to hud_definitions.h/hud.h without editing the 3-4 TUs that define
their own local copy and include those headers (hud_nav_points.c, hud_unit.c, ui_widget.c / hud.c,
hud_messaging.c), and `extern ... hud_globals` cannot go to hud.h because main.c and interface.c declare it with
other local types (C2373). => focused header interface/hud_globals_definitions.h (full HCEX layout, typed
accessor hud_globals_definition_get with HUD_GLOBALS_DEFINITION_TAG 'hudg', extern hud_globals).
game_engine.c: font reads -> hud_globals->messaging.single_player_font.index / multi_player_font.index;
postgame bitmap -> hud_definition->carnage_report_bitmap.index.
Result: candD in shadow: game_engine 0 changed sections vs production (180/180); /W3 shows only the
pre-existing C4244 narrowing warnings.

## P6 Layer E -> candE.c = A+B2+Bshort+C2+D+E
- `extern struct game_engine *game_engines[];` duplicated game_engine_list.h's genuine extern (header "included in
  hcex build", owner game_engine_list.c) -> include game_engine_list.h, drop the local extern.
- The file's symbol comment still listed 40 `_code_XXXXXXXX` placeholders; each offset maps to exactly one real
  symbols.json name (e.g. 0009C460 -> _default_starting_location_rate_function), same refresh batch 3 did for
  units.c. Refreshed; 0 `_code_` left.
- protoscan.py (reviewer's tool) on candE: 0 NOT-DEFINED-HERE prototypes; remaining externs are the TU's own
  COMMON globals (game_engine_globals, global_stage, timeout_for_endgame_sound).
- Result: shadow compile identical to production (0 changed sections; 180/180).

## P7 handle_custom_starting_equipment shape (lab -> adopted as layer C3)
Trigger: with the HCEX 6-entry item_collection array, the old hand strength-reduced pointer walk
(`long *item_collection = &...[0].index; item_collection += sizeof(...)/sizeof(*...)`) plus the
`while (TRUE) {...continue; return;}` search read as implausible source.
Evidence: /Od 0x5b25d0 (found by the only `push 0xcc` element size in the range; od_5b25d0.txt): outer
`for (index = 0; index < count; index++)` whose matching entry holds the whole grant and then `break`s;
inner `for (i = 0; i < 5; i++)` reading `starting_equipment->item_collection[i].index` twice
(cmp [..+0x48] then reload); ternary temp for `first_weapon ? 2 : 0`; frame order scenario, index,
starting_equipment, first_weapon, i, definition_index, weapon_index, placement_data, weapon.
Probe 1 (lab/loop_for.c, index for-loop with a local): byte-identical. Probe 2 (lab/hcse_od.c, full /Od shape
in /Od declaration order): byte-identical (0 changed sections). Adopted as C3 (candF).
Loop count 5 of 6 HCEX entries is January's (bytes +0x48..+0x88; /Od also 5): comment says so; it is not a new
behaviour (production looped 5 over a 5-entry view).

## P8 full-board sweep of the final packet (candF + HB2 HF HS HC2 HD HNP_owner)
sweep.py --all: 557 TUs; 440 source/ TUs compiled, 0 changed sections anywhere (0 regressions, 0 park drift);
117 "errors" are libs/* TUs absent from the source-only shadow (they include no Halo header).
Owner-TU warnings unchanged (sound_classes, player_control, hud_messaging, game_engine_multiplayer_sounds,
hud_nav_points): the new prototypes agree with their definitions.

## P9 optional extras (measured, not required)
- (e) players.h `long unknown70;` -> `long teleporter_index;` (HCEX multiplayer_player_info.teleporter_index at
  player+0x6C+4) + the 6 game_engine.c uses: 70 players.h consumer TUs, 0 changed sections (identifier text is
  count-inert). Task says report only -> patches/05_optional_* (players.h part applies to the tree; the .c part
  applies after 01). HCEX also names +0x7C multiplayer.player_display_index (players.h `unknown7c`), not touched.
- Siblings ctf/king/oddball/race dropping their HCEX-identical local scenario_netgame_flag for the new focused
  header: 4 TUs, 0 changed sections -> patches/06_optional_sibling_netgame_flag.patch.

## FINAL (candF + packets 01-04)
- final_game_engine.obj (shadow compile of final/): 180/180 EXACT (gate_final.txt); object_audit PASS
  (audit_final.txt); symbol table identical to production except internal $L label ids -> PDB storage
  unchanged (pdb_base.txt: 0 disagreements); surplus 11 COMDATs identical (surplus_base.txt, sections
  unchanged); SELECTED-PROVIDER LINK PASS (plink_final.txt).
- Full-board shadow sweep (440 source TUs): 0 changed sections, 0 regressions, 0 park drift.
- All patches `git apply --check` clean individually and together (01,02,03,04 + optional 05h, 06).
- Blockers: (a) all seven views gone (rasterizer_debug_options -> genuine header `.pad3`; hud globals/postgame ->
  HCEX hud_globals_definition; scenario_netgame_flag/equipment/starting_equipment and item permutation/collection
  -> HCEX layouts); (b) 0 consumer-local prototypes of other files' functions (protoscan), plus the C4013
  console_printf and the game_engines[] extern; (c) real everywhere (also game_engine.h); (d) both NonMatching
  comments removed and the 40 stale `_code_` names in the symbol comment refreshed; (e) reported (+optional patch).
- Disclosure for the reviewer: the HCEX types/prototypes went to FOCUSED owner headers because the genuine broad
  headers were measured to regress: hud.h (+16 names) moves main's parked _main_update_time; scenario_definitions.h
  + item_definitions.h regress units::_unit_preprocess_node_orientations and move 3 parks; hud_definitions.h/hud.h
  cannot take hud_globals_definition without rewriting 3-5 TUs that define their own copy. Precedent: narrow owner
  headers (bitmap_group_lookup.h, players_runtime.h, player_control_runtime.h, matrix_math.h).
