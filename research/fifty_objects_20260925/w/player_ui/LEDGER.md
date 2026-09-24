# player_ui whole-object admission ledger (wave 3, lane claude/fifty-objects-20260925, base 954eebd2)

Task: recover complete January public layout of `playlist_profile` (player_ui local view has opaque
`unused1D[0x47]`), place in genuine owner header as separate patch + consumer list; finish natural
select-any string ownership/coalescing audit.

## Reading (done)
- Rejection: config/object_admission_rejections.json source/interface/player_ui `struct playlist_profile` (source-layout-incomplete).
- docs/object_matching_logs/player_ui_obj_owner_house_rule_reconciliation_20260904.md: 42/42 exact, data 25/25 exact,
  3 surplus select-any strings (empty "", controller_index assert, L"") ; link test failed (LNK1120 163 unresolved) - not a pass.
- playlist_profile.h API already uses `struct game_variant *variant` for get/save/get_from_path.
- game_engine.h `struct game_variant` (0x68) is itself a partial view: short variant_index@0, unused2[0x16]/unknown16@0x16,
  engine_type@0x18, has_teams@0x1C, flags@0x20, unknown24..unknown64, float unknown3C.
- player_ui local `struct playlist_profile`: wchar_t name[12]@0, engine_type@0x18, teams@0x1C, unused1D[0x47], word flags@0x64.

## Finding 1 (HCEX.pdb types, NAMES ONLY)
- HCEX _player_ui_globals: player_data[2], join_players_to_next_multiplayer_game, single_player_controller_index,
  `struct game_variant variant`, user_selected_variant, `_edit_profile_data edit_profile_data` {profile_index, union edit_profile, union original_profile}.
- HCEX game_variant (0x98): wchar_t human_readable_game_description[0x18]@0, long game_engine_index@0x30,
  universal_variant@0x34 (0x48), union game_engine_variant@0x7C (0x18), unsigned short flags@0x94.
- HCEX universal_variant (0x48): teams, pad0..2, long flags, goal_radar, odd_man_out(+pad4..6), respawn_time_growth,
  respawn_time, suicide_penalty, lives, float health, score_to_win, weapon_set, vehicle_options_s vehicle_set, vehicle_set2,
  vehicles_respawn, uchar friendly_fire, long friendly_fire_penalty, uchar team_autobalance, long time_limit.

## Finding 2 (HCEX): there is NO `struct playlist_profile` type in HCEX
- HCEX _edit_profile_data unions are { struct player_profile player_profile; struct game_variant variant; }.
- HCEX playlist_profile_internal { struct game_variant variant; checksum[4] }, _cached_variant_profile { profile_index; struct game_variant variant }.
- January playlist_profile.h API (get/save/get_from_path) already takes `struct game_variant *`; player_ui fills its edit
  union member with playlist_profile_get(index, &member) (sizeof 0x68 == January game_variant 0x68).
- January assert text in ui_widget_event_handler_functions: "failed to retrieve editable game variant" after every
  player_ui_get_edit_playlist_profile() call. => the January "playlist profile" IS `struct game_variant`; the player_ui local
  `struct playlist_profile` is an invented partial-view type name, not a January type.

## Finding 3: January field census (all from strictly exact code; game_engine 180/180 E, ui_widget_event_handler_functions 100/100 E)
UI assert labels (January strings) on raw-offset stores in ui_widget_event_handler_functions + game_engine_variant_cleanup clamps:
| off | width | January evidence | HCEX name (same relative offset) |
| 0x00 | wchar[12] | virtual_keyboard_launch(profile, 24, 9); name[11]=0 store @0x16 (short) in cleanup/playlist_profile/event handler | human_readable_game_description (HCEX [0x18]) |
| 0x18 | long | set_game_engine; cleanup PIN(1,5) | game_engine_index |
| 0x1C | bool | 'teams' | universal_variant.teams (+0) |
| 0x20 | long | flags: b0 'other players on radar', b1 'friends on screen', b2 'infinite grenades', b3 'shields'(set=no shields), b4 'invisible players', b5 'starting equipment' | universal.flags (+4) |
| 0x24 | long | 'radar display' 0..2 | goal_radar (+8) |
| 0x28 | bool | 'odd man out' | odd_man_out (+0xC) |
| 0x2C | long | 'respawn time growth' 0/150/300/450 | respawn_time_growth (+0x10) |
| 0x30 | long | 'respawn time' | respawn_time (+0x14) |
| 0x34 | long | 'suicide penalty' | suicide_penalty (+0x18) |
| 0x38 | long | 'number of lives' | lives (+0x1C) |
| 0x3C | float | 'maximum health' 0.5..4 (cleanup PIN 0.25..4) | health (+0x20) |
| 0x40 | long | 'score/kills/captures/laps to win' | score_to_win (+0x24) |
| 0x44 | long | 'weapon set' 0..10 | weapon_set (+0x28) |
| 0x48 | long | 'vehicle set' 0..4 (plain value, cleanup PIN 0..4; predict switch) | vehicle_set (+0x2C; HCEX made it a bitfield union) |
| 0x4C | 0x18 | memset(profile+0x4C,0,0x18) when engine changes (set_game_engine) | union game_engine_variant (+0x7C in HCEX) |
|  ctf | | 0x4C 'assault', 0x4D cleanup bool, 0x4E 'flag must reset', 0x4F 'flag at home to score', 0x50 'single flag' time (ticks) | assault, reset_on_capture, flag_must_reset, flag_at_home_to_score, single_flag_time |
|  slayer | | 0x4C 'death bonus'(stored inverted), 0x4D 'kill penalty'(inverted), 0x4E 'kill in order' | no_death_bonus, no_kill_penalty, kill_in_order |
|  king | | 0x4C 'moving hill' | moving_hill |
|  oddball | | 0x4C 'random start', 0x50 'speed with ball', 0x54 'trait with ball', 0x58 'trait without ball', 0x5C 'ball type', 0x60 'ball spawn count' | random_start, (0x4D ball_spawn_delay), speed_with_ball, trait_with_ball, trait_without_ball, oddball_ball_type, ball_spawn_count |
|  race | | 0x4C 'race type', 0x50 'team scoring' | race_type, team_scoring |
| 0x64 | word | bit0 system default (playlist_profile/player_ui) ; `*(short*)(profile+0x64)=0` | flags (HCEX unsigned short) |
| 0x66 | 2 | tail padding to 0x68 | (none - natural alignment) |
Universal variant in January = HCEX universal_variant prefix through vehicle_set (0x30 bytes); HCEX's vehicle_set2..time_limit are later additions.

## Finding 4: January assert strings name the members (first-party, January image + Oct-2001 2276 prototype)
- January cachebeta.xbe / build/split game_engine_race.obj: "_race_type_normal != game_engine_get_variant()->game_engine_variant.race.race_type"
  => January game_variant has union member `game_engine_variant`, union member `race`, field `race_type`, enum constant `_race_type_normal`
  (same string in the Oct-2001 2276betaP.xbe). MCC /Od build strings use "human_readable_game_description", "universal_variant",
  "goal_radar", "respawn_time_growth", ... (later lineage; names only).
- HCEX enums: game_engine_universal_variant_flags {_allow_motion_tracker_bit=0,_allow_friendly_navpoints_bit,_infinite_grenades_bit,
  _no_shields_bit,_always_invisible_bit,_generic_starting_equipment_bit(5), later 6..8}; unnamed {_variant_is_system_default_bit=0,
  NUMBER_OF_GAME_VARIANT_FLAGS}; race_type {_race_type_normal,_any_order,_flag_rally}; race_team_scoring; oddball_speed; game_engine_trait;
  oddball_ball_type {_oddball_normal,_magic,_terminator}. January UI labels match the flag bits 0..5 one-for-one.

## Tooling (worker-local)
- scratch/w/player_ui/shadow_gate.py: compiles a unit from scratch/w/player_ui/shadow/ so shadowed headers win (verified by
  /showIncludes; aborts if the real header is used); compares EVERY section vs build/base production obj + January exact count.
  Control (no shadow headers): player_ui 75/75 sections identical, game_engine 307/307 identical.
- consumers.py: CL /Zs /showIncludes over all 592 build.ninja units -> includes.json. game_engine.h consumers = 37,
  player_ui.h = 13, playlist_profile.h = 4 (lists in ge_consumers.txt etc.).
- sweep.py: parallel (8) shadow_gate over a unit list.

## Probe 0: shadow game_engine.h with the COMPLETE January game_variant (HCEX names, January types), consumer sources unchanged
- Hypothesis: the struct body change (+~20 declared names: universal_variant/ctf/slayer/king/oddball/race/game_engine_variant tags+members)
  is inert for consumers that never name a game_variant member.
- Result: 27/37 consumers byte-IDENTICAL to production (all sections), incl. player_ui (still with its local view).
  10 fail to compile (they name old members): game_engine, game_engine_ctf/king/oddball/race/slayer, network_game_manager,
  network_server_manager, network_server_message_handler, playlist_profile. -> need member renames.

## Finding 5: oddball ball_spawn_delay at 0x4D IS January-attested
- January builders (game_engine 180/180 exact) write the byte at 0x4D in oddball variants: build_game_variant_oddball /
  reverse_tag `=1`, team_oddball/accumulation/juggernaut/stalker `=0`; game_engine_index there is 3 (oddball).
  HCEX oddball_variant +1 = ball_spawn_delay (unsigned char). -> field included.

## Probe 1: consumer member renames (rename.py -> cand/*.c) + shadow header WITHOUT ball_spawn_delay
- Result: 35/37 identical; game_engine failed to compile (builders use 0x4D); game_engine_race DIFFERENT:
  `_race_engine_player_update` and `_race_touch_flag` LOSS (same size, bytes differ) = a declared-name-count tie.
## Probe 2: + `boolean ball_spawn_delay;` (January-attested, Finding 5); rename restricted to builder `result` (game_engine_place
  has its own `result.flags`)
- Hypothesis: the one genuine missing member restores race's count-tie (TU name-count oracle).
- Result: **37/37 game_engine.h consumers byte-IDENTICAL to production in every section**, January exact counts unchanged
  (race 36/36 back). The race loss/restore is count evidence that the January header declared ball_spawn_delay.

## Probe 3: player_ui candidate (local view removed; union member `struct game_variant variant`; getter returns
  `struct game_variant *`; casts gone) + player_ui.h shadow (drop `struct playlist_profile;`, return type) + HCEX flag enum
  {_variant_is_system_default_bit, NUMBER_OF_GAME_VARIANT_FLAGS} in game_engine.h replacing both TU-local constants
- Sweep 45 units (game_engine.h 37 + player_ui.h 13): 43 identical; units `_unit_preprocess_node_orientations` LOSS (sentinel);
  main `_main_update_time` (PARKED, non-exact) bytes move. = declaration-count effect of +2 enum names.
## Finding 6: HCEX enum is actually THREE constants
- HCEX: unnamed enum {_variant_is_system_default_bit=0, NUMBER_OF_GAME_VARIANT_FLAGS=1, _default_variant_index_mask=0xFFFFFF00}
  (the high byte of game_variant.flags holds the default-variant index: January playlist_profile `flags |= index << 8`,
  ui_widget_game_data_input_functions `flags >> 8`). PDB neighbours: game_engine flag enums + get_score_type => game_engine.h owner (not proof).
## Probe 4: the full HCEX 3-constant enum (variant B)
- units IDENTICAL again (189/189); main `_main_update_time` still moves (1440/118 relocs both, sha f4d7bb79 -> 5f8cfd1a; parked
  unclassified, zero credit) -> would need park re-baseline. Not choosing by count: variant B is recorded as an OPTIONAL packet.
## Probe 5: variant A = struct recovery + player_ui local view removal, flag constant stays TU-local
  (player_ui local renamed `_playlist_profile_system_default_bit` -> HCEX `_variant_is_system_default_bit`; playlist_profile.c unchanged
  local `_game_variant_is_system_default_bit`)
- Result: **45/45 consumers byte-IDENTICAL to production in every section** (player_ui 75/75, units 420/420, game_engine 307/307,
  ui_widget_event_handler_functions 473/473; shadows verified used via /showIncludes).

## Finding 7: player1 profile path is a SEPARATE file static in January (HCEX + January bytes)
- HCEX player_ui compiland: `File Static char[0x100] player1_profile_path` at RVA 0x16976C0 = right after player_ui_globals
  (RVA 0x1695710, size 0x1FAC -> 0x16976BC, aligned). HCEX player_ui_globals has NO path member and no pad members.
- January: player_ui_dispose/initialize clear exactly 0x230 bytes (production needed an invented PLAYER_UI_DISPOSE_SIZE=0x230
  because it put the path INSIDE the struct with a `byte unknown22D[3]` pad); csplit's 0x330 `_player_ui_globals` extent is the
  public symbol swallowing the following nameless static. config/relocs.json: only 6 January refs to +0x230, all from
  player_ui_remember_player1_profile / player_ui_get_player1_last_used_profile_index (confined -> file static); cachebeta publics lack it.
- VC7 .bss law: uninitialised statics come first (name-hash); `= 0` ones follow in declaration order. January has globals@0,
  path@0x230 => the path is zero-initialised and declared after player_ui_globals.
## Probe 6 (variant A2): A + remove explicit pads unknown35[3]/unknown155[3]/unknown22D[3] (HCEX has none; natural alignment),
  path -> `static char player1_profile_path[0x100] = { 0 };` after player_ui_globals, memsets use sizeof(player_ui_globals)
  (+ size assert 0x230), PLAYER_UI_DISPOSE_SIZE removed.
- uninitialised path first: path@.bss 0, globals@0x100 (law confirmed; rejected). With `= { 0 }`: globals@0, path@0x230.
- Result: player_ui 75/75 sections IDENTICAL to production, 42/42 January exact (coff_compare resolves both to
  `_player_ui_globals`+0x230). object_audit PASS (67 January symbols, 0 differ).
- Needs symbols.json line `{ "file_offset": 4538160, "flags": 0, "name": "_player1_profile_path", "static": true }` after
  `_player_ui_globals` so the split carries the static (objdiff infers BSS symbol sizes from the next symbol).
  simsplit.py (patched split copy, 6 relocs retargeted): A2 = 68/68 symbols equal, 67/67 sections equal;
  current production would then miss the static (so the symbols.json line and A2 must land together).

## Finding 8: the recovered member path reproduces January's assert literal by plain stringification
- game_engine_race `_race_type_normal != ...` was a match_vassert with an explicit January string because the old macro alias
  spelled the expression differently. With the recovered layout the plain `match_assert(file, 0x2D4, _race_type_normal !=
  game_engine_get_variant()->game_engine_variant.race.race_type)` stringifies to January's literal exactly; race 66/66 sections
  identical (the .rdata literal COMDAT included). Adopted in the race rename.

## Probe 7 (final, variant A2 reproducible via mk_variants.py): 45/45 consumers byte-IDENTICAL to production (every section).
## Whole-object audit of player_ui (candA2_player_ui.obj, shadow_A headers)
- gate: 42/42 January exact; 75/75 sections identical to production.
- object_audit.py: PASS (67 January symbols, 0 differ); surplus = .drectve + 3 select-any strings.
- simsplit.py (symbols.json line applied to a split copy): 68/68 symbols, 67/67 sections equal.
- pdb_storage.py: 0 disagreements (the new static is absent from cachebeta publics, as it must be).
- surplus_identity.py: 0 code COMDATs. fake_match_scan on all 11 candidate .c + 2 headers: 0 leads.
## String ownership / coalescing audit (the rejection's open item) - DONE
- The 3 surplus strings "" (??_C@_00CNPNBAHC@), "(controller_index>=0) && (controller_index<MAXIMUM_GAMEPADS)" (61 B), L"" are
  NATURAL: January's player_ui split holds an UNDEFINED reference to each (so January's player_ui used exactly these strings; the
  linker coalesced them into action_obey / input_abstraction / game_engine). Sources: main_set_multiplayer_map_name(""),
  player_ui_autolevel_enabled match_assert line 320 (#expr stringification), message = L"".
- Our copies are section_infos_equal to January's single selected copy; COMDAT selection 2 (select any) on both sides.
- Census of build/base: 44 / 2 / 13 definers of the three names, ALL select-any COMDATs (no NODUP) -> COFF coalescing is
  guaranteed; provider_link.py pair links PASS in both orders against the January-selected providers.
- (The 2026-09-04 whole-program link attempt failing LNK1120 is not needed: the lane's accepted standard is the pair link.)

## Finding 9: universal flags stay `unsigned long` (January bytes beat HCEX's `long`)
- January ui_widget_event_handler_functions _playlist_profile_initialize_indicator_options +0x1b0: `mov eax,[eax+0x20]; shr eax,1;
  and eax,1` = unsigned extraction of bit 1 (a signed field gives sar). Production type kept.
## Oracle 1 (NOT a landing choice): HCEX explicit pads pad0..2/pad4..6 removed (natural alignment instead)
- 44/45: game_engine_race `_race_engine_player_update` + `_race_touch_flag` LOSS (same pair as Probe 1 without ball_spawn_delay).
  => the race TU's declaration count agrees with January only with HCEX's explicit pad members AND ball_spawn_delay: count-level
  corroboration that January's game_engine.h spelled universal_variant/oddball_variant as HCEX does. The HCEX spelling was chosen
  on first-party evidence before this measurement.

## Optional owner variant B (delta on top of A2): HCEX 3-constant flag enum in game_engine.h, both TU-local constants removed
- Sweep 45: 44 identical; main `_main_update_time` (PARKED unclassified, zero credit, frame 0x30 vs January 0x34 = structural)
  moves 1440/118 f4d7bb79... -> 1440/118 5f8cfd1a03e2d4faddfd96d46f25ce9676ab650b4f12f4a8351bb5dc9f1bfba2 (alndiff 608 -> 606 lines,
  neutral). Needs a park re-baseline; units sentinel stays 189/189. Not needed for admission; offered as owner-clean follow-on.

## FINAL DELIVERABLES (scratch/w/player_ui/patches, all `git apply --check` clean vs 954eebd2, verified to reproduce the candidates)
- 01_game_engine_h_game_variant_layout.patch  SHARED HEADER (37 game_engine.h consumers)
- 02_game_variant_member_renames.patch        10 consumer .c (compile dependency of 01)
- 03_player_ui_h_game_variant.patch           SHARED HEADER (13 player_ui.h consumers)
- 04_player_ui_c.patch                        player_ui.c (A2)
- 05_symbols_json_player1_profile_path.patch  (+ edited full copy config/symbols.json; one inserted line after _player_ui_globals)
- 06_admission_config.patch                   rejection entry removed + config.json status Matching (+ copies in config/)
- optB_delta_owner_flag_enum.patch            OPTIONAL, needs _main_update_time park re-baseline
- Consumer sweep list: all_consumers.txt (45 units); final_sweep_A2.txt = 45/45 identical in every section.
- Reproduce: python scratch/w/player_ui/mk_variants.py; python scratch/w/player_ui/mk_patches.py;
  SHADOW_ROOT=scratch/w/player_ui/shadow_A python scratch/w/player_ui/sweep.py scratch/w/player_ui/all_consumers.txt out.txt --srcdir scratch/w/player_ui/cand_A2
## Follow-ups (not blockers for player_ui, not attempted): ui_widget_event_handler_functions raw `(byte *)profile + 0x..` accesses and its
  `playlist_profile_item_options_prefix`/`playlist_profile_data` views can now use struct game_variant; ui_widget_game_data_input_functions
  `struct ui_game_variant` view; game_engine.c raw TEST_FLAG bits 3/4/5 and `_game_variant_*` flag names vs HCEX
  game_engine_universal_variant_flags; oddball/race enums are TU-local (HCEX types the variant members with them).
- hcex_types.txt (106 MB DIA2Dump -t output) deleted to keep curate.py copies small; regenerate with DIA2Dump -t HCEX.pdb.
