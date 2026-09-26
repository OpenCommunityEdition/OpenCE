# A1 (wave 3) RESULTS - source/game/game_engine admission packet (final measurement at HEAD fe283cc5)

## Verdict
- 180/180 strict exact.
- All admission battery checks green (battery/).
- Full-board sweep: 447/447 SAME vs build/base and vs head-root objects.
- Zero exact losses.
- Admission is still owner-gated by Q1 (motion-sensor copy: alias or /Od view copy) and by the Q2/Q3b/Q4 dispositions
  (OWNER_QUESTIONS.md).
- Object effect on YES: +1 object (Halo 388 -> 389/468). Function, code and data credit are unchanged; all are
  already credited.

## Starting point: prior work (not in the lead's brief)
- The fifty-objects lane had already cleared most of review_admit2's (a)-(e):
  - game_engine_clean (01-04), review3 (R01-R04), game_engine_finish (F01-F07) and review4 (F01R/F03aR);
  - owner queue item 6 (pick_game_engine: optionA/optionB/variant_no_single_consumer_headers).
- This packet starts from pick_game_engine's no-single-consumer-header variant (01A/01B + 02-05). I rebased it onto
  HEAD, re-verified it and extended it.

## What the packet changes relative to that base (every item pre-registered; CARDS.md)
- **C02 vTU (nav-point trio into hud.h)**: no new header. hud_nav_points.h is NOT created.
  - Evidence: X - HCEX game_engine.c includes interface\hud.h. Tree - hud.h already declares hud_nav_points.c's API.
- **R1 (rule 14)**: `multiple_kills >= _game_engine_message_killed_by_player` becomes `>= 4`. It was a count compared
  against a message enum; HEAD has the same bug.
- **R2 (rule 13)**: build_lighting `flag->type == 4` becomes `_netgame_flag_race_vehicle`. Two stale comments dropped.
- **R3A (lead item e)**: players.h HCEX multiplayer_player_info names:
  - unknown70 -> teleporter_index;
  - unknown7c -> player_display_index;
  - target_hold_time -> player_display_count.
  - The count-neutral rename is used because R3B, which removes the placeholder union, flips rasterizer_frame_statistics
    `_rasterizer_frame_statistics_draw`.
- **R4, R5 and R6 (rule 13)**: HCEX `enum goal_radar`, the game-matching options (`_game_engine_all`, `_all_non_team`,
  `_all_normal`) and `_multiplayer_sound_*`, all TU-local like F01R's other game-engine enums. Adopted as a complete
  set (Q5).
- **R8**: drops 14 unused TU prototypes of build_game_variant_*. They duplicated game_engine_playlist.h, and the only
  caller follows every builder.
- **Q3D**: the invented `debug_player_color` aggregate becomes the genuine short hs global plus HCEX static locals of
  update_teleporter.
  - Evidence: J (word and dword loads), X (HCEX statics), D (/Od 0x5afba0, contiguous flash block with per-component
    stores).
  - No symbols.json change is needed: object_audit PASS and objdiff .data 100%.
- **R9**: find_closest_player_callback takes the HCEX `void *custom_data`, so there is no function-pointer cast
  (rule 27). /Od 0x59aa30 agrees.
- **R10**: update_purge declares HCEX's two iterators, item_iterator and biped_iterator, in disjoint scopes.
  - Evidence: X (both at frame +0x50), D (two iterators), J (a 0x10 frame).
- **R11**: find_netgame_flags uses the conventional for-loop initialiser.
- **R12**: 56 boolean variant stores change from 0/1 to FALSE/TRUE.
- **R13a/b**: the two qsort comparators use their /Od single-exit shape (/Od 0x5b6e00, 0x5b6e60). This drops the early
  `return -1` and the value locals.
- **R14a/b**: two scalar-only bare blocks move into function scope. These were review4 advisories.
- **Comment**: honest labels for the two descriptive .bss statics (Q3b).
- **Q1**: 01A (alias, default) and 01B (view copy) differ only in this function. Their objects are identical.

## Patches (patches/; LF git patches; `git apply --check` clean alone and as a set on the real tree at fe283cc5; a fresh HEAD export plus each set equals the measured roots byte for byte)
- A1_01A_game_engine_c_alias.patch: source/game/game_engine.c, Q1 option A. Mine.
- A1_01B_game_engine_c_viewcopy.patch: source/game/game_engine.c, Q1 option B. Alternative to 01A.
- A1_02_LEAD_game_engine_h.patch: renames only, plus float -> real, the short nav-point parameter and the two
  multiplayer-sound prototypes. This is review4 F03aR. 37 consumer TUs.
- A1_03_LEAD_owner_prototypes.patch: unit_get_local_player_index goes into player_control.h (21 TUs);
  hud_get_font_index and hud_get_text_color into hud_messaging.h (8); sound_class_set_gain into sound_classes.h (6).
  This is F03b.
- A1_04_LEAD_hud_h_nav_point_trio.patch: hud.h gains `union real_point3d;` and a prototypes/HUD_NAV_POINTS.C block.
  10 TUs.
- A1_05_LEAD_hud_nav_points_c_duplicate_prototype.patch: hud_nav_points.c drops its duplicate custom_render_nav_point
  prototype, because hud.h now owns it. This file belongs to another unit, so it goes to the lead; it is optional for
  compilation.
- A1_06_LEAD_players_h_hcex_member_names.patch: count-neutral renames. 71 TUs.
- WITHDRAWN_A1_07*: the itmc layout in item_definitions.h. It was 0-loss at 09f5208f and flips units at fe283cc5.
- All of 01 and 02-06 must land together, because game_engine.c needs the renamed header names.
- config/config.json status flip: the lead's, after the full gate. It is not included.

## Verification at fe283cc5 (battery/)
| Check | Result |
|---|---|
| 01 gate | revgate is gate.py's logic run from an alternate root; gate.py --source cannot see header patches. head 180/180, finalA 180/180, finalB 180/180. |
| 02 keyed diff vs build/base | finalA, finalB and finalA-vs-finalB each 0/302 changed. |
| 02 raw disclosure diff | Section order: always_invis emitted later, and the .data/.bss owner symbol order changes. 7 new static-local symbols. Relocation spellings resolve to identical destinations: `_global_goal+0x400` for `_global_variant+0` (2 functions) and the static locals for `_debug_player_color+N` (update_teleporter, 7 rows). |
| 03 full-board sweep, 447 TUs | control head 447/447 SAME; finalA and finalB 447/447 SAME vs build/base and 447/447 SAME vs the head-root objects. |
| 04 consumer census and per-TU revgate A/B | 84 consumer TUs, 0 differences. Per header: game_engine.h 37, player_control.h 21, hud_messaging.h 8, sound_classes.h 6, hud.h 10, players.h 71, all 0 differences. |
| 05 object_audit | PASS; text identical to production's audit. |
| 05 pdb_storage | 0/269 disagreements. |
| 05 surplus_identity | 11 COMDATs, 0 not identical. |
| 05 provider_link | PASS both orders on all 40 surplus externals; no new surplus. |
| 05 protoscan | 10 TU prototypes, all defined in the TU; HEAD had 18 consumer-local ones. |
| 05 /W3 census, 447 TUs | Only game_engine changes: C4013 console_printf and two C4244 long->short disappear, and double->float is now reported as double->real. No new warnings. |
| 05 fake_match_scan | 0 leads on all 8 touched files (HEAD also 0). |
| 06 objdiff-cli 3.3.1 mini-project | 27922/32397 code, 173/180 functions, 3792/3792 data, 0 per-function deltas vs production. |
| 06 audit_semantic_matches | accepted_ledger 180/180, with the same 7 semantic-coff-only functions as production. |
| 07 patch apply checks | Clean. Rebuild identity holds. |
| 08 Q1 strip | 179/180. |
| 09 count oracles | populate band at K=53..57. The units canary band moves from 12 to 8 names. |
| Lead's pending structures.h edit (now in fe283cc5) | HEAD and finalA both 180/180. |

## Failed predictions (kept)
- C04: I predicted that R4+R6 and R5+R6 would flip populate. Both were exact; only R6 alone flips.
- C06: I predicted objdiff .data would drop without a symbols.json change. It stayed at 100%.
- C10: the optional itmc move lost nothing at 09f5208f. At fe283cc5 it flips units. Withdrawn.
- C01's outcome held (180/180), but its reasoning did not:
  - the populate band moved by 13 K-units between cdc8ebd3 and HEAD (npA 21..25 vs vTUnp 8..12), so the net C1
    shift was -13 or +51 mod 64;
  - I had predicted a -3 shift from game_allegiance.h's three names alone;
  - other header edits since cdc8ebd3 (e.g. real_math.h P1a body text) also moved the numbering.

## Noted, not changed (review items without enough evidence, or other units' debt)
- update_purge's `cutoff_time = 900; ... idle_ticks > (short)cutoff_time`:
  - January pins 900 in esi and does a word compare, so a short-typed variable must exist;
  - /Od 2020 uses an immediate;
  - no evidence names any other variable.
- `_game_engine_message_*` against HCEX's `game_engine_message_*` spelling.
- HCEX local names in find_closest_player_index (buffer, direction, position, autoaim_target_*).
- game_variant.flags = 1 literal in the builders.
- HCEX nests teleporter_index etc. under `multiplayer`.
- The unused `unknown80` byte view is kept in players.h, because removing it flips `_rasterizer_frame_statistics_draw`.
- Three COMMON externs remain: game_engine_globals, global_stage and timeout_for_endgame_sound (lane Q10).
- Other TUs:
  - race.c and slayer.c call game_show_score_extended and game_engine_did_player_win_default through implicit
    declarations (C4013).
  - hs_globals_external.c has `extern byte debug_player_color[];` (the genuine type is short).
  - ui_widget_event_handler_functions.c and hud_nav_points.c keep local prototypes of game_engine functions.
  - 7 HUD TUs keep local hud_globals_definition views.
  - king, oddball and race keep local scenario_netgame_flag copies.
