# source/game/game_engine

## verdict
OBJECT_COMPLETE_CANDIDATE

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_finish\\final\\source\\game\\game_engine.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_finish\\final\\source\\game\\game_engine.h",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_finish\\final\\source\\game\\player_control.h",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_finish\\final\\source\\interface\\hud_globals_definitions.h",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_finish\\final\\source\\interface\\hud_messaging.h",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_finish\\final\\source\\interface\\hud_nav_points.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_finish\\final\\source\\interface\\hud_nav_points.h",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_finish\\final\\source\\items\\item_collection_definitions.h",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_finish\\final\\source\\scenario\\scenario_netgame_definitions.h",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_finish\\final\\source\\sound\\sound_classes.h",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_finish\\patches\\F01_game_engine_c.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_finish\\patches\\F02_new_focused_headers.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_finish\\patches\\F03a_shared_header_game_engine_h.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_finish\\patches\\F03b_shared_header_prototypes.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_finish\\patches\\F04_hud_nav_points_owner_include.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_finish\\patches\\F05h_optional_players_teleporter_index_h.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_finish\\patches\\F05c_optional_teleporter_index_c_afterF01.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_finish\\patches\\F06_optional_sibling_netgame_flag.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_finish\\patches\\F07_optional_owner_motion_sensor_view_copy.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_finish\\CONSUMERS.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game_engine_finish\\LEDGER.md"
]

## production_changes
All patches are in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\game_engine_finish\patches\. They are LF git patches. `git apply --check` is clean for each one alone and for all together against 16542e46. The full resulting files are in ...\game_engine_finish\final\source\ (LF).

Land F01+F02+F03a+F03b+F04 together. F01 needs the new headers (F02), the renamed constants and members (F03a) and the prototypes (F03b) to compile.

- F01_game_engine_c.patch (source/game/game_engine.c). This is reviewer R01 rebased onto f7cd2e72 (2 conflicts resolved; see LEDGER F0), plus:
  - man_out uses the /Od form `boolean man_out = player->quit_out_of_game || game_engine_player_is_out_of_lives(i) || game_engine_player_is_odd_man_out(i); return man_out;`.
  - populate_statistic_buffer and postgame_statistic_get_rating: HCEX parameters (statistic_buffer, enum postgame_statistic statistic, boolean inverse). get_rating keeps a 32-bit `long inverse`, which is January's ABI. Indexed loops; match_assert followed by a separate if; the /Od locals.
  - get_player_place: `while (TRUE) { if (...) break; ... }`.
  - select_players_to_display: /Od names statistic_buffer and outside_range, plus the /Od local-player count.
  - did_player_win_default: single exit with explicit NONE/TRUE/FALSE stores (both `goto tied` removed). team_has_players: goto + else.
  - TU enums:
    - `enum game_engine_mode` (HCEX; replaces the invented mid-file postgame enum).
    - `enum postgame_statistic` (the HCEX tag; the constants have descriptive names).
    - `enum game_engine_weapons` 0..10: value 10 is `_no_grenades`, from January's behaviour.
    - `enum game_engine_vehicles` 0..4.
    - `_weapon_list_*` 0..13.
    - The continuation flag bits 3, 4 and 5.
  - These constants replace the literals in every switch table, in the postgame_state compares, in the builder stores and in the PINs.
  - Builder flags become one SET_FLAG per bit.
  - HCEX `struct netgame_goal` names (player_index / ignore_player_index / nav_index). The pad members are dropped.
  - Hand-inlines replaced with calls:
    - list_index_to_weapon_definition_index (14 sites in predict_resources, plus a `for` loop);
    - game_globals_get_weapon;
    - game_engine_infinite_grenades_internal;
    - linear_to_non_linear_alpha (x2);
    - game_engine_has_teams (x3);
    - get_place_string (x2);
    - game_engine_test_flag and game_engine_test_trait.
  - weapon_definition_index_to_list_index: /Od indexed form.
  - game_engine_update: the invented `union game_engine_update_iterator` is removed. Three disjoint block-scoped iterators, per-player helper calls, a single `if (game_engine)` exit, and the function-pointer staging temp is removed.
  - GET_GAME_ENGINE_HUD_FORMAT local macro replaced by /Od statement form with a `format` local.
  - HCEX message parameter names (message/message_data/buffer/buffer_size).
- F02_new_focused_headers.patch (NEW files): hud_globals_definitions.h, hud_nav_points.h, item_collection_definitions.h and scenario_netgame_definitions.h. These are R02, plus the HCEX `_netgame_starting_equipment_flag_no_grenades_bit` / `_plasma_greandes_bit` (sic) enum in scenario_netgame_definitions.h, whose only includer is game_engine.c.
- F03a_shared_header_game_engine_h.patch (SHARED HEADER, 37 consumer TUs). Contents:
  - R03's game_engine.h hunks: float->real, `short local_player_index`, and the two multiplayer-sound prototypes.
  - Flag-bit renames to HCEX semantics: disable_dynamic_lighting, disable_integrated_lights, 5_or_more_players, allow_friendly_navpoints.
  - `unknown2C` -> `pregame_post_rasterize` and `unknown44` -> `update`.
  - Parameter renames in format_message and set_goal_position.
  - The declared-name count is the same as HEAD+R03. Adding the three new bits to the header broke the units and network_client_message_handler canaries, so those bits are TU-local.
  - Consumers: ai/encounters camera/dead_camera game/{game, game_engine, game_engine_ctf, game_engine_king, game_engine_list, game_engine_oddball, game_engine_race, game_engine_slayer, game_globals, game_statistics, players} interface/{hud, hud_messaging, hud_unit, interface, motion_sensor, player_ui, ui_widget} items/{items, projectiles, weapons} main/main math/random_math networking/{network_client_manager, network_client_message_handler, network_game_manager, network_server_manager, network_server_message_handler} objects/{damage, object_lights, objects} render/render `saved games/playlist_profile` units/{bipeds, units}.
- F03b_shared_header_prototypes.patch (SHARED HEADERS, R03 unchanged):
  - player_control.h, 21 consumers: ai/ai_debug camera/{bored_camera, director, editor_flying_camera, following_camera} effects/player_effects game/{aim_assist, game, game_engine, player_control, players} hs/hs_library_external input/input_abstraction interface/{first_person_weapons, hud, hud_draw, hud_weapon, interface, motion_sensor, ui_widget} main/main.
  - hud_messaging.h, 8 consumers: game/game_engine game/players hs/hs interface/{hud, hud_messaging, interface, player_ui} `saved games/game_state`.
  - sound_classes.h, 6 consumers: game/game game/game_engine sound/{game_sound, sound_classes, sound_definitions, sound_manager}.
  - The full list is in ...\game_engine_finish\CONSUMERS.txt.
- F04_hud_nav_points_owner_include.patch (source/interface/hud_nav_points.c) = R04.
- OPTIONAL, verified on top:
  - F05h + F05c: players.h `unknown70` -> `teleporter_index` (71 consumers) plus the game_engine.c part regenerated after F01.
  - F06: R06, sibling engines include scenario_netgame_definitions.h.
  - F07 (OWNER): the /Od-attested motion-sensor view copy.
- config/symbols.json: no change.

## evidence
Every step is in ...\game_engine_finish\LEDGER.md (F0-F16). Lab variants are in lab/.

- Rebase check: the rebased R01-R04 packet alone at HEAD gives 180/180, and the 446-TU sweep gives 445 SAME plus game_engine's raw reloc spelling.
- Final packet, compiled from an independent shadow_F (`git archive HEAD source` + F01..F04 == final/ byte-for-byte), with production cflags:
  - 180/180 EXACT vs build/split.
  - 0 of 302 section-defining symbols changed vs build/base.
  - Symbol multiset identical (853/853).
  - object_audit PASS; the audit text is identical to production's.
  - provider_link PASS.
  - 11 surplus COMDATs, all identical.
  - pdb_storage 0 disagreements.
  - objdiff-cli 3.3.1 mini-report: production and final both 27922/32397 code, 173/180 fns (the 7 undercredited are pre-existing) and 3792/3792 data, with 0 per-function deltas.
  - No parks and no admission rejections for this object.
- Full sweep of 446 TUs vs build/base: 445 SAME. game_engine differs only in:
  - (a) section order: C2 now emits update_player_always_invis later, because it calls test_trait, which this file defines later. Production's order already differs from January's.
  - (b) the known `_global_goal+0x400` spelling of `_global_variant+0`. The resolved target is identical and coff_compare reports EXACT.
- Control sweep of pristine HEAD: 446 SAME.
- With the optional F05-F07 on top: 180/180, and the sweep result is the same.
- /W3: only game_engine.c changes. C4013 is gone. A C4244 long->boolean is added; it is January's ABI, proven by a dword read in get_rating and a byte read in populate.

Key /Od attestations:
- man_out 0x5a0b90: named byte local assigned from the || chain.
- populate 0x5b5e60: the assert-anchor span of 53 lines equals January's (712/765 vs 1162/1215), and generate_title_string's anchor keeps the same +450 offset.
- update 0x5aea50: calls update_player_always_invis 0x5af860, which calls test_trait 0x5ae9c0.
- Two lookup helpers: list_index 0x5b4150 calls game_globals_get_weapon 0x5b1170; post_rasterize_in_game and target_name call linear_to_non_linear_alpha 0x5b4100.
- Other helper calls: title/in_game_score call has_teams 0x5a06a0; post_game/in_game_score call get_place_string 0x5b2000; internal_rasterize_score calls game_engine_test_flag(1).
- Weapon set 10: the decoded /Od jump tables give set 13 (no_grenades) January's set-10 behaviour.
- The /Od functions are laid out alphabetically; odmap.py maps them and diffs call edges.

## blockers
None of the section-8(g) blocker classes remain. The owner needs to decide:

- Motion-sensor copy. Both forms are EXACT.
  - The default keeps the reviewer's in-loop `struct netgame_goal *goal` alias, which is not in the /Od 0x5a20e0 frame.
  - F07 is the /Od-attested one-statement copy `positions[count] = *(real_point2d *)&global_goal[goal_index].position;`. It is load-bearing (direct component copies pick the +4 strength-reduction base), so the house rules make it owner-gated.
- Policy items, all disclosed in the LEDGER:
  - descriptive `_postgame_statistic_*` constant names (HCEX has only the tag);
  - set 10 named `no_grenades` against HCEX's `covenant` numbering;
  - TU-local continuation flag bits 3/4/5, because game_engine.h's declared-name count is fragile;
  - the /Od-attested dead `local_player_count` and the strip-inert `= 0` inits.
- Zero-credit follow-ups, not done:
  - `extern` COMMON globals that the pool places in game_engine.c;
  - engine test_flag, trait and sound literals, which would need count-fragile game_engine.h names.

## reopen_criteria
- The owner rules on F07 against the alias. On F07 the object needs no further source work.
- Or a reviewer finds another missing /Od helper call among the 141 /Od functions not yet mapped (tools: odmap.py plus odmap_extra.json in the slug).
- Any future game_engine.h edit must leave the declared-name count unchanged. Sweep units and network_client_message_handler first; +3 names and -2 names both flipped `_unit_preprocess_node_orientations`.

## task notes
No tracked file was modified; git status stays clean at 16542e46. All work is under scratch/w/game_engine_finish/.

**Evidence tools (all in the slug):**
- rcompile.py, rcompile_F.py, rcompile_O.py: production-cflag shadow compiles.
- rsweep.py: 446-TU sweep.
- rconsumers.py and warnfilter.py: consumer census and /W3 comparison.
- make_patches.py: patch generation.
- lab/*.py: edit generators. The lab/*.json edit sets are applied in combine.py order.
- od_scan.py and odmap.py: /Od build call-edge census. The /Od functions are sorted alphabetically per TU.

**Incident-rule note:** once, `rm -rf` ran on a relative slug path after a `cd` in the same chain. The target was inside my own slug (scratch/w/game_engine_finish/shadow_F) and nothing else was touched. Every later destructive command used absolute slug paths.

**Integrator actions after the full ninja:**
- Confirm in report.json that `_game_engine_player_get_custom_motion_sensor_positions` and `_game_engine_render_nav_points` still score 100%. The reviewer's and my objdiff 3.3.1 mini-projects both show 100%.
- The COMMON-owner question for game_engine_globals, global_stage and timeout_for_endgame_sound is a separate zero-credit follow-up.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\game_engine_finish\LEDGER.md
