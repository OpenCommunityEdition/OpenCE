# A1 (wave 3) - game_engine admission - running log (bank-as-you-go)

## A1-0 setup (2026-09-26 00:34:30 -0700)
- Worktree HEAD 455dffad (branch claude/compiler-application-20260925). Sole writer this wave: source/game/game_engine.c.
- Baseline: `gate.py source/game/game_engine --all` = 180/180 EXACT (base_gate_all.txt); gate obj == build/base
  (keyed_diff: 0 changed/added/removed of 302 sections).
- config.json: game_engine.c index 307 NonMatching. No parks, no object_admission_rejections entry.

## A1-1 prior work found (NOT in the lead's brief) - must build on it, not repeat it
The fifty-objects lane went well past the wave2 review_admit2 refusal:
- wave3 worker game_engine_clean (01-04 packet: removes all (a) views, (b) 18 local prototypes, (c) float, (d) stale
  comments; 4 new focused headers) -> review3 (R01-R04: +hand-expanded distance_squared3d x2, csplit-alias loop bounds,
  goto clamps, invented postgame_statistic_entry...) -> wave4 game_engine_finish (F01-F07: man_out, statistic walks,
  rule-16 switches, update iterator union, 14 hand-inlined list_index calls, ...) -> review4 (F01R/F03aR amended;
  approve=false ONLY on the motion-sensor copy) -> owner_queue/pick_game_engine (cdc8ebd3): optionA alias / optionB
  /Od view copy / variant_no_single_consumer_headers (NP). Owner queue item 6 (Q1 pick, Q2 focused headers, Q3
  debug_player_color + descriptive .bss names) in docs/object_matching_logs/claude_fifty_objects_20260925_owner_queue.md.
- No owner answer to item 6 found in docs/, research/, the lane LEDGER/OWNER_PACKET or the 2026-09-26 rulebook.
- Since cdc8ebd3 none of the packet's files changed except game_allegiance.h (4f236e9f, HCEX allegiance incident enum)
  which F01R newly includes -> declared-name count of game_engine.c differs -> MUST re-measure (populate is a
  count-tie at K=8-12 per pick_game_engine S5).

## A1-2 harness + rebase (00:36-00:58)
- HEAD moved twice while working: 343a3f82 (hud_weapon Q6) and 42fa975e (decals Q9). All roots are rebuilt from the
  current HEAD with tools/build_roots.sh (git archive + git apply; mkroot.py never follows the shared xbox junction).
- tools/sweep.py: production-faithful full-board compile from an alternate root (cwd=root with an xbox junction,
  verbatim relative cflags, ninja's relative backslash source path, private TMP per unit + retry after two transient
  C1083/C1001 collisions) + keyed comparison (coff_compare.section_infos_equal) vs build/base or another sweep.
  CONTROL at 42fa975e: 447/447 SAME vs build/base. (A stale control at 343a3f82 showed decals DIFF only because the
  lead had just rebuilt build/base for 42fa975e - not a packet effect; re-run at 42fa975e is clean.)
- C01 npA (NP alias packet) at HEAD: 180/180; full sweep 447/447 SAME vs build/base. npB (NP view-copy): same.
- C02 vTU (nav trio in genuine hud.h, no hud_nav_points.h): 180/180; full sweep 447/447 SAME.
- Count oracle (LAB ONLY, in-enum dummy constants after MULTIPLAYER_MAXIMUM_PLAYERS): npA flips populate at
  K=21..24(+), vTU at K=9..13 (others 0..24 exact). So vTU = npA + ~12 C1 numbers and sits 9 below its band.
  At cdc8ebd3 the same vTU WAS in the band; the numbering shift since then (game_allegiance.h +3 names and
  real_math.h P1a body text) moved it out. Disclose: count-sensitive, margin 9 names.
- Consumer census (tools/consumers.py, /showIncludes, vTU root): game_engine.h 37, player_control.h 21,
  hud_messaging.h 8, sound_classes.h 6, hud.h 10, players.h 71.

## A1-3 source review of the rebased packet against the 2026-09-26 rulebook (01:00-01:15)
- Style scan (rule 7 one parameter per line / void on its own line; rule 8 explicit return;): 180 definitions,
  0 suspects. No float, no code_/bss_/NonMatching, no consumer-local prototypes (protoscan: all TU prototypes are
  TU-defined functions).
- NEW defects found (not in any prior review):
  - R1 rule 14: `statistics.multiple_kills >= _game_engine_message_killed_by_player` (a count vs an enum that equals 4;
    also in HEAD). Fix `>= 4`.
  - R2 rule 13: build_lighting `flag->type == 4` (+ two explanatory comments) -> existing `_netgame_flag_race_vehicle`.
  - R3 lead item (e): players.h unknown70 / unknown7c / {unknown80,target_hold_time} -> HCEX multiplayer_player_info
    names teleporter_index / player_display_index / player_display_count. R3B (drop the placeholder union) flips
    rasterizer_frame_statistics _rasterizer_frame_statistics_draw (count tie; -1 name in every players.h consumer)
    -> R3A (count-neutral renames, union and its unused `unknown80` byte view kept) adopted.
  - update_purge: `cutoff_time = 900; ... idle_ticks > (short)cutoff_time` reuses the time cutoff as the idle-tick
    threshold. January pins 900 in esi (`mov esi,0x384; cmp word ptr [eax+0x6c],si`), which needs a short-typed
    value held in a variable (constant pins are source-determined); /Od 2020 compares an immediate. Noted, not
    changed (no evidence for any other variable).
  - Remaining raw domain literals with HCEX names: goal_radar 0/1/2 (HCEX enum goal_radar _radar_motion_tracker,
    _radar_nav_point, _radar_none), match_game_type 12/13/14 (HCEX _game_engine_all, _all_non_team, _all_normal)
    and 1/5 (game_engine_ctf/race), multiplayer sound literals 1, 0xE-0x12, 0x1B (HCEX _multiplayer_sound_*).
  - game_engine_playlist_next(long parameter0, long parameter1, long playlist_type): HCEX has NO parameters;
    January's body ignores all three; callers pass (0, 0, 4). Names unrecoverable -> owner question.
  - 14 TU prototypes of build_game_variant_* duplicate game_engine_playlist.h and are not needed (the only caller,
    get_variant_by_name, is defined after every builder).
- HEAD moved again: 26684ca8 (main), 09f5208f (physics). Roots rebuilt; control 447/447 SAME at 09f5208f.
- c1a = vTU + R1 + R2 + R3A: game_engine 180/180; full sweep 447/447 SAME vs build/base (09f5208f).

## A1-4 further genuine repairs, each pre-registered (cards C04-C11) - all on top of c1a (01:10-01:36)
- C04 R4/R5/R6 HCEX domain constants (goal_radar enum, _game_engine_all/_all_non_team/_all_normal, _multiplayer_sound_*),
  TU-local like F01R's other game-engine enums: R6 ALONE flips populate; every combination with R4 or R5 is 180/180;
  the pre-registered "complete set or nothing" stance -> complete set adopted (disclosed count coupling).
- C05 R8 dropped 14 vestigial TU prototypes of build_game_variant_* (unused: get_variant_by_name follows every
  builder; game_engine_playlist.h declares them): 180/180. Count oracle on c3: populate band at K=55..59 (mod 64), i.e.
  removing 5-9 names before populate would flip it; adding up to 54 is safe.
- C06 Q3D debug_player_color: the invented anonymous aggregate -> `short debug_player_color = NONE;` (hs global,
  _hs_type_short_integer) + seven update_teleporter static locals with HCEX names (screen_flash_type short per January's
  word load, max_intensity, alpha, red, green, blue, duration). J .data layout identical; 180/180; object_audit PASS
  (text identical to production); objdiff .data 100% WITHOUT any symbols.json change (failed prediction, good news).
- C07 LAB: the two .bss statics as HCEX static locals land after _global_autogenerate_* (layout differs, audit FAIL)
  -> kept as file statics with an honest descriptive-name comment (rule 15); owner question Q3b.
- C08 R9 callback `void *custom_data` (HCEX; /Od 0x59aa30), removes the function-pointer cast; R10 update_purge's two
  HCEX iterators (item_iterator, biped_iterator) in disjoint block scopes (January frame 0x10 = one shared slot):
  both 180/180.
- C09 R12 56 boolean variant stores 0/1 -> FALSE/TRUE; R11 find_netgame_flags conventional for-initialiser: 180/180.
- finalA/finalB (alias / view copy) = vTU + the above; git-apply packets rebuild them byte-for-byte; full sweep
  447/447 SAME (vs build/base and vs head-root objects) at 09f5208f; /W3: only game_engine changes (C4013 console_printf
  and two C4244 long->short gone, double->float becomes double->real); fake_match_scan 0 leads (8 files); protoscan 10 TU
  prototypes, all TU-defined; object_audit PASS; pdb_storage 0/269; surplus 11/0; provider_link PASS; objdiff 3.3.1 =
  production; semantic ledger 180/180 each.
- C10 vGenI (itmc layout into item_definitions.h): 0 exact losses, hud_weapon _render_weapon_hud residual bytes move
  (-0.014% objdiff). OPTIONAL add-on only (hud_weapon is an active cross-lane target).
- C11 vGenS2 (equipment layouts into scenario_definitions.h): exact losses (ai_communication speech timers, units
  node orientations) + parked ai body moves -> NOT adopted.

## A1-5 finalisation (01:36-01:50)
- C12 R13a/b comparators in /Od single-exit shape (/Od 0x5b6e00 / 0x5b6e60): 180/180 -> adopted.
- C13 R14a/b scalar-only bare blocks hoisted: 180/180 -> adopted.
- HEAD moved to 217bad07/328b6148 (breakable_surfaces Q15) and fe283cc5 (render_debug owner-header prototypes incl.
  structures.h +1 prototype, which reaches game_engine.c). Everything re-run at fe283cc5 with tools/verify_final.sh:
  gate 180/180 (finalA, finalB), keyed 0/302, sweeps 447/447 SAME (vs build/base and vs head objects), consumer table
  84 TUs 0 diffs, audit PASS (text == production), pdb 0/269, surplus 11/0, provider link PASS, protoscan 10 TU-defined,
  /W3 only game_engine (-4 warnings, +0), fake scan 0, objdiff 3.3.1 == production, semantic ledger 180/180.
- The optional itmc add-on (C10) now flips units at fe283cc5 -> WITHDRAWN (patch files renamed WITHDRAWN_*).
- Q1 strip refreshed (179/180; IV base +4). Count oracles: game_engine populate band K=53..57; units canary band moves
  from +12 (HEAD) to +8 (packet) names.
- Deliverables: patches/, OWNER_QUESTIONS.md, RESULTS.md, battery/, cards/, CARDS.md.
