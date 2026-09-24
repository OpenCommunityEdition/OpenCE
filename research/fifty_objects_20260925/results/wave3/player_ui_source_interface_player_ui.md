# source/interface/player_ui

## verdict
OBJECT_COMPLETE_CANDIDATE

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\player_ui\\patches\\01_game_engine_h_game_variant_layout.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\player_ui\\patches\\02_game_variant_member_renames.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\player_ui\\patches\\03_player_ui_h_game_variant.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\player_ui\\patches\\04_player_ui_c.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\player_ui\\patches\\05_symbols_json_player1_profile_path.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\player_ui\\patches\\06_admission_config.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\player_ui\\patches\\optB_delta_owner_flag_enum.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\player_ui\\config\\symbols.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\player_ui\\config\\object_admission_rejections.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\player_ui\\config\\config.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\player_ui\\cand_A2\\source__interface__player_ui.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\player_ui\\shadow_A\\game\\game_engine.h",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\player_ui\\shadow_A\\interface\\player_ui.h",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\player_ui\\all_consumers.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\player_ui\\final_sweep_A2.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\player_ui\\LEDGER.md"
]

## production_changes
Apply in order. Every patch passes `git apply --check` against 954eebd2. A temp copy of the 14 affected files was patched in sequence and matched the candidate sources byte for byte (EOL-normalised). All patches are under C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\player_ui\patches\:
(01) 01_game_engine_h_game_variant_layout.patch. SHARED HEADER source/game/game_engine.h. It replaces the partial game_variant (variant_index/unused2/unknownXX/float) with the complete January layout, using HCEX names and January-proven types. `struct universal_variant` is {boolean teams; byte pad0,pad1,pad2; unsigned long flags; long goal_radar; boolean odd_man_out; byte pad4,pad5,pad6; long respawn_time_growth, respawn_time, suicide_penalty, lives; real health; long score_to_win, weapon_set, vehicle_set}. The engine variants are ctf{assault, reset_on_capture, flag_must_reset, flag_at_home_to_score, long single_flag_time}, slayer{no_death_bonus, no_kill_penalty, kill_in_order}, king{moving_hill} and oddball{random_start, ball_spawn_delay, long speed_with_ball, trait_with_ball, trait_without_ball, oddball_ball_type, ball_spawn_count}; race is {long race_type, team_scoring}. They sit in `union game_engine_variant`. `struct game_variant` is {wchar_t human_readable_game_description[12]; long game_engine_index; struct universal_variant universal_variant; union game_engine_variant game_engine_variant; word flags;}. The patch keeps verify_game_variant_size (0x68) and drops the unknown16 offset typedef.
(02) 02_game_variant_member_renames.patch. A compile dependency of 01, so apply them together. It renames member accesses in 10 TUs: game_engine.c (the 26 builders map engine-specific bytes by their game_engine_index; cleanup; motion sensor), game_engine_ctf/king/oddball/race/slayer.c (the #define aliases unknown4C.byteN/unknownXX are removed; the slayer aliases had wrongly used CTF names), network_game_manager.c, network_server_manager.c (offsetof(universal_variant.teams)), network_server_message_handler.c and playlist_profile.c (name[11] store written as [MAXIMUM_GAME_VARIANT_NAME_LENGTH - 1]). In game_engine_race.c, the match_vassert at 0x2D4 becomes a plain match_assert, because the recovered expression now stringifies to January's literal exactly.
(03) 03_player_ui_h_game_variant.patch. SHARED HEADER source/interface/player_ui.h: drops `struct playlist_profile;`, and player_ui_get_edit_playlist_profile now returns `struct game_variant *`.
(04) 04_player_ui_c.patch. The local struct playlist_profile and its size typedef are gone; the union member is `struct game_variant variant` and the casts are gone. `_playlist_profile_system_default_bit` becomes HCEX's `_variant_is_system_default_bit` (still TU-local). The explicit pads unknown35[3], unknown155[3] and unknown22D[3] are removed. The path is now HCEX's separate `static char player1_profile_path[0x100] = { 0 };`, declared after player_ui_globals. The memsets use sizeof(player_ui_globals) and the invented PLAYER_UI_DISPOSE_SIZE is removed. A player_ui_globals==0x230 size typedef is added, and the symbol comment is updated.
(05) 05_symbols_json_player1_profile_path.patch. One line is inserted after `_player_ui_globals`: `{ "file_offset": 4538160, "flags": 0, "name": "_player1_profile_path", "static": true },`. The edited full copy is scratch/w/player_ui/config/symbols.json. Regenerate csplit for player_ui afterwards. 04 and 05 must land together: with 05 applied, production's current player_ui would lack the static symbol.
(06) 06_admission_config.patch. Removes the player_ui object_admission_rejections entry and sets config.json player_ui.c status to Matching (copies in scratch/w/player_ui/config/). Optionally, update the reason text of the existing semantic_matches entry for _set_local_player_controls_from_player_profile, which still cites the playlist boundary.
OPTIONAL, not needed for admission: optB_delta_owner_flag_enum.patch, applied on top of 01-05. It moves HCEX's full unnamed enum {_variant_is_system_default_bit=0, NUMBER_OF_GAME_VARIANT_FLAGS, _default_variant_index_mask=0xFFFFFF00} into game_engine.h and removes the TU-local constants in player_ui.c and playlist_profile.c. Cost: the parked zero-credit main `_main_update_time` moves from 1440/118/f4d7bb79 to 1440/118/5f8cfd1a03e2d4faddfd96d46f25ce9676ab650b4f12f4a8351bb5dc9f1bfba2 and needs a park re-baseline; units stays 189/189.
CONSUMERS FOR THE FULL SWEEP (45 unique units). game_engine.h (37): ai/encounters, camera/dead_camera, game/game, game/game_engine, game/game_engine_ctf, game/game_engine_king, game/game_engine_list, game/game_engine_oddball, game/game_engine_race, game/game_engine_slayer, game/game_globals, game/game_statistics, game/players, interface/hud, interface/hud_messaging, interface/hud_unit, interface/interface, interface/motion_sensor, interface/player_ui, interface/ui_widget, items/items, items/projectiles, items/weapons, main/main, math/random_math, networking/network_client_manager, networking/network_client_message_handler, networking/network_game_manager, networking/network_server_manager, networking/network_server_message_handler, objects/damage, objects/object_lights, objects/objects, render/render, saved games/playlist_profile, units/bipeds, units/units. player_ui.h (13): game/game, game/player_control, game/player_rumble, game/players, hs/hs, input/input_abstraction, input/input_xbox, interface/player_ui, interface/ui_widget, interface/ui_widget_event_handler_functions, networking/network_client_manager, saved games/game_state_xbox, saved games/player_profile. The list is in scratch/w/player_ui/all_consumers.txt, computed by CL /Zs /showIncludes over all 592 build.ninja units.

## evidence
The January 'playlist profile' IS struct game_variant. HCEX's edit-profile unions are {struct player_profile player_profile; struct game_variant variant;}; there is no playlist_profile type anywhere in HCEX, and playlist_profile_internal/_cached_variant_profile hold a game_variant. The January playlist_profile.h API already takes game_variant*. January prints "failed to retrieve editable game variant" after every player_ui_get_edit_playlist_profile().

Layout: every offset and width is attested by strictly exact January code (game_engine 180/180, ui_widget_event_handler_functions 100/100). The UI assert labels on raw-offset stores give the fields: 0x1C 'teams'. 0x20 flags bits 0-5: 'other players on radar', 'friends on screen', 'infinite grenades', 'shields', 'invisible players', 'starting equipment'. 0x24 'radar display', 0x28 'odd man out', 0x2C 'respawn time growth', 0x30 'respawn time', 0x34 'suicide penalty', 0x38 'number of lives', 0x3C 'maximum health', 0x40 score/kills/captures/laps to win, 0x44 'weapon set', 0x48 'vehicle set'. At 0x4C, `memset(profile+0x4C,0,0x18)` runs on an engine change. The ctf, slayer (stored inverted, i.e. the no_ fields), king, oddball and race labels sit at HCEX's exact relative offsets. The January builders write oddball byte 0x4D, which is ball_spawn_delay. 0x64 is the word flags. HCEX names were used only where January offsets independently attest the field. The universal variant is HCEX's 0x30-byte prefix; HCEX's later vehicle_set2..time_limit tail and its 24-char name are not transplanted. The flags field stays unsigned (January `shr eax,1` at ui_widget_event_handler_functions _playlist_profile_initialize_indicator_options+0x1b3).

First-party January string: "_race_type_normal != game_engine_get_variant()->game_engine_variant.race.race_type" (January image and the Oct-2001 2276 build). A plain match_assert now reproduces this literal byte for byte.

Path static: HCEX has a File Static char[0x100] player1_profile_path directly after player_ui_globals. January dispose/initialize clear exactly 0x230 bytes. Only 6 January relocations reach +0x230, all from 2 player_ui functions, and the name is absent from cachebeta publics. By the VC7 .bss law, the path is `= {0}` and declared after the globals (the uninitialised spelling was measured: it lands at .bss 0).

Measurements: a shadow-header gate compiles each candidate so the modified headers win, which /showIncludes verified. Across all 45 consumers, every section is byte+reloc IDENTICAL to production and January exact counts are unchanged (final_sweep_A2.txt). player_ui: 42/42 EXACT, 75/75 sections identical; object_audit PASS (67 January symbols, 0 differ); pdb_storage 0 disagreements; surplus_identity 0; fake_match_scan 0 leads over 11 .c and 2 headers. simsplit.py applies the symbols.json line to a copy of the split and gives 68/68 symbols and 67/67 sections equal.

String audit, completed: the 3 surplus strings "", the 61-byte controller_index assert and L"" are each an UNDEFINED reference in January's player_ui split, so their use is natural. Each is section_infos_equal to January's single selected copy, with COMDAT selection 2 on both sides. All 44/2/13 build/base definers are select-any; none is NODUP. provider_link.py passes in both orders.

Declaration-count oracle (not used to choose): omitting ball_spawn_delay or HCEX's explicit pads flips race `_race_engine_player_update`/`_race_touch_flag`, which corroborates the HCEX spelling at count level.

Full probe history is in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\player_ui\LEDGER.md.

## blockers
None found in the source or the object. Integrator actions: (a) apply 01+02 together and 04+05 together; (b) regenerate csplit for player_ui after the symbols.json line; (c) run the full ninja, parks, admission audit and pytest over the 45 listed consumers. I did not run ninja, csplit or objdiff; the objdiff .bss data score is inferred from identical symbol sets in the simulated split. Residual judgement calls for the reviewer: the flag constant `_variant_is_system_default_bit` stays TU-local in player_ui.c (the owner move is optB, which costs a park re-baseline); HCEX's explicit pad0..pad6 members are kept as first-party spelling; oddball/race members are typed `long` because their enums are currently TU-local in game_engine_oddball.c/race.c (HCEX types them with those enums); the name array bound is the literal [12].

## reopen_criteria
If the integrator sweep shows any moved section in the 45 consumers, bisect between patch 01's struct and the player_ui.h forward-declaration change (both are declared-name-count changes). If the owner prefers the flag enum in game_engine.h, apply optB and re-baseline the _main_update_time park.

## task notes
No tracked file was edited, and I ran no ninja, configure or git-mutating command in the worktree. I briefly used a throwaway `git init` in scratch/w/player_ui/tmp/applytest to test the patch series in order; that directory has been deleted.

Tools live in scratch/w/player_ui/ and are reusable by other workers:
- shadow_gate.py: shadow-header gate. SHADOW_ROOT selects the shadow tree; it checks the shadow was used via /showIncludes and compares every section with production and January.
- sweep.py: runs the gate over a unit list, 8 in parallel.
- consumers.py: CL /Zs /showIncludes consumer census.
- simsplit.py: simulates a symbols.json static insertion on a copy of the split.
- mk_variants.py and mk_patches.py: rebuild everything reproducibly.

Follow-ups I noticed and did not attempt:
- ui_widget_event_handler_functions still uses raw `(byte *)profile + 0x..` offsets and its own views (playlist_profile_item_options_prefix, playlist_profile_data{byte[0x68]}). All of these can now become struct game_variant fields.
- ui_widget_game_data_input_functions has a `struct ui_game_variant` partial view.
- game_engine.c uses raw TEST_FLAG bits 3/4/5, and the `_game_variant_*` flag names differ from HCEX's game_engine_universal_variant_flags (_allow_motion_tracker_bit ... _generic_starting_equipment_bit, one-for-one with the January UI labels).
- The oddball/race enums are TU-local.

I deleted the 106 MB hcex_types.txt so curate.py stays small; regenerate it with `DIA2Dump -t HCEX.pdb`.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\player_ui\LEDGER.md
