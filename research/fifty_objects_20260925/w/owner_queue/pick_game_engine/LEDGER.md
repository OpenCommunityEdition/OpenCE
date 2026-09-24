# owner_queue/pick_game_engine LEDGER (wave 5; tree cdc8ebd3; nothing lands)

## S0 setup
- Tree at cdc8ebd3 (git status clean). Read WORKER_BRIEF s.0-10, lane log held table, review4_game_engine REVIEW/LEDGER,
  game_engine_finish LEDGER F0-F16, wave4 REVIEW__source_game_game_engine.md.
- Owner ruling cited as "#4 (no one-consumer focused headers)" = memory owner-rulings-20260921-lane-b item 3
  ("Use real_math.h, the genuine owner ... Do not invent a one-consumer header").
- Status flip mechanism = config/config.json `{ "name": "source/game/game_engine.c", "index": 307, "status": "NonMatching" }`
  (scratch/campaign/admit.py --match). tools/audit_object_admission.py (read-only) at HEAD: game_engine is already a
  CANDIDATE with function_gap 0 / data_gap 0 in the STRICT report (the 7 objdiff-undercredited fns remap_vehicle,
  remap_weapon, populate, internal_rasterize_score, update, nonplayer_post_rasterize, post_rasterize are credited by
  build/semantic_report.json accepted_ledger 'semantic-coff'); no semantic_matches.json entry needed. Saved
  admission_audit_head.json.

## S1 rebase / apply at cdc8ebd3
- Commits since review4's base 16542e46: 466698b8 (hud_draw.h inline; hud_nav_points.c edited) + cdc8ebd3 (hygiene; no
  game_engine files). `git apply --check` on the real tree at cdc8ebd3: F01R, F02, F03aR, F03b, F04 each alone: clean;
  set A (all five in one call): clean. F07 needs F01R first (one-call set with F07 fails: git apply checks every patch
  against the original tree) -> option B will ship a standalone F01R-B game_engine.c patch instead of F01R+F07.
- Harness in tools/ (common.py, mkshadow.py, buildall.py, cmpdirs.py, gatecmp.py; adapted from review4, SLUG-guarded).
  Shadows: head (git archive HEAD source, CRLF) == working tree 989/989 files byte-identical. optA = head + F01R,F02,
  F03aR,F03b,F04 (all "Applied cleanly"); optB_seq = optA + F07 (applied cleanly sequentially).

## S2 measured at cdc8ebd3 (options A and B, current held packet)
- game_engine compile in each shadow (production cflags from build.ninja, cwd = shadow root; tools/gatecmp.py =
  coff_compare.section_infos_equal): head 180/180, 0/302 changed vs build/base (control); optA 180/180, 0/302 changed;
  optB_seq 180/180, 0/302 changed. No compile warnings at default level.
- Full 447-TU sweep (tools/buildall.py + tools/cmpdirs.py; canonical COFF: all non-.debug$S sections raw+relocs by name,
  full symbol table, $L/$label$N canonicalised):
  * CONTROL head vs build/base: 447/447 SAME (build/base is current at cdc8ebd3; harness reproduces production).
  * head vs optA: 446 SAME + game_engine DIFF. head vs optB_seq: 446 SAME + game_engine DIFF.
  * tools/gediff.py characterisation (both options identical): (a) _game_engine_update_player_always_invis emitted later
    (C2 defers it: it now calls game_engine_test_trait, defined later in the file); (b) raw reloc spelling
    `_global_variant`+0 -> `_global_goal` with addend 0x400 in the bytes (1 byte) at custom_motion_sensor_positions+0x83
    and render_nav_points+0xba (csplit-relocation-alias; section_infos_equal resolves both to the same .bss byte ->
    EXACT); (c) $L label numbering only in remap_vehicle/remap_weapon/internal_rasterize_score/populate/nonplayer_post_
    rasterize/update/post_rasterize (bytes identical).
  * optA vs optB_seq game_engine.obj: same section order, NO content difference at all -> the pick is byte-neutral;
    A and B are the same object.
- Admission battery on optA and optB objs (identical results): object_audit PASS (text identical to production's audit
  modulo obj path); pdb_cand (pdb_storage on candidate symbols): 269 split symbols, 0 disagreements; surplus_cand: 11
  candidate-only code COMDATs (sine, cosine, arctangent, square_root, vector_from_points3d, scale_vector3d,
  magnitude_squared3d, magnitude3d, normalize3d, distance_squared3d, distance3d), 0 not identical (same set as
  production); provider_link: SELECTED-PROVIDER LINK PASS (both orders, 39 surplus externals incl. literals/D3D tables).
- /W3 /Zs census head vs optA (447 TUs): 2 TUs change. hud_nav_points: pre-existing C4013 'error' line 351 -> 346 only
  (F04 removed 5 lines). game_engine: C4013 console_printf gone; long->short C4244 6 -> 4; double->float becomes
  double->real (same pow return); long->byte kept (line moves); NO new warning.
- Consumer census (/showIncludes, optA): game_engine.h 37, player_control.h 21, hud_messaging.h 8, sound_classes.h 6,
  hud_globals_definitions.h 1, hud_nav_points.h 2, item_collection_definitions.h 1, scenario_netgame_definitions.h 1,
  players.h 71 (= review4 CONSUMERS). All 37 game_engine.h consumers are inside the full sweep: 36 SAME + game_engine.

## S3 NEW pre-existing admission risk found while checking the TU globals (not in either option; not in reviews 3/4)
- `debug_player_color` is an anonymous TU struct `{ short value; word pad; short teleporter_flash_type; word
  teleporter_flash_pad; real teleporter_flash_maximum_intensity; real_argb_color teleporter_flash_color; real
  teleporter_flash_duration; }` owning January's whole 0x20-byte .data (split owners: only `_debug_player_color`).
  `word pad` members in a TU-private aggregate = the law (j) signature of an invented aggregate (held class of
  rasterizer_xbox_profile / render_debug).
- HCEX.pdb (DIA2Dump -sym game_engine_update_teleporter, hcex_update_teleporter.txt): the teleporter flash values are
  FUNCTION-STATIC LOCALS of game_engine_update_teleporter: `float red`, `green`, `blue`, `duration` (initialised .data,
  ascending), plus static locals `int blocked_message_delay` and `long fade_function` (.bss) - which January's split names
  descriptively `_game_engine_teleport_message_ticks` / `_game_engine_teleport_flash_fade_function` (symbols.json).
  HCEX `debug_player_color_definition` is only the hs_global_external record.
- 2020 /Od exe (data only): debug_player_color at 0xa1dfd0 (used by 0x5a1f60); unrelated globals follow at
  0xa1dfd4..0xa1dfe8 (0xa1dfe8 -> "game_engine_update", used by 0x5aea50); the flash values sit at 0xa1dfec..0xa1dffc,
  each referenced ONLY by update_teleporter 0x5afba0. => no first-party support for one aggregate spanning
  debug_player_color and the flash values.
- Not a rebase issue; landed long ago (GE09, docs/object_matching_logs/game_engine_obj.md). Disclosed in RULING.md as
  an owner question that the game_engine admission implicitly answers. No source form tried (out of this wave's scope).

## S4 strip test re-measured at cdc8ebd3 (lab only; edits/lab_strip.py, shadow lab_strip)
- Hypothesis: both exact spellings are load-bearing (review3 ms_n1 / review4 s4). Change: option A with the alias
  removed = option B with the cast removed = plain `positions[count].x = global_goal[goal_index].position.x;` /
  `.y` (the cast-free B spelling `positions[count] = global_goal[goal_index].position;` does not compile: real_point3d
  -> real_point2d).
- Result: 179/180, custom_motion_sensor_positions RESIDUAL (1 changed of 302 vs base). alndiff_strip.txt: 4
  instruction diffs, all the strength-reduced induction-variable base: ours `mov esi,&_global_goal+4` / `[esi-4]`,
  `[esi]` / `cmp esi,&_global_goal+0x404`; January `+0`, `[esi]`, `[esi+4]`, `+0x400`. Same size, same relocation count.
  => the alias (A) and the view cast (B) are both LOAD-BEARING; neither passes a byte-inert strip test.
- Note (harness): git apply writes CRLF even in the LF export (global core.autocrlf=true); variant.py now normalises
  copies to LF before scripted edits; every compiled shadow is CRLF-normalised like the working tree.

## S5 owner ruling "no one-consumer focused header" (memory owner-rulings-20260921-lane-b #3) vs F02
- Consumers (optA /showIncludes): hud_globals_definitions.h 1 (game_engine), item_collection_definitions.h 1,
  scenario_netgame_definitions.h 1, hud_nav_points.h 2 (game_engine + its owner hud_nav_points.c via F04).
  None is in the tree or in HCEX's header list (hud.h, hud_definitions.h, item_definitions.h, scenario_definitions.h
  are). All four headers' own comments say they exist to dodge declaration-count ties in the broad genuine headers.
  => the ruling applies to all four (strictly to the three single-consumer ones; hud_nav_points.h has a single
  non-owner consumer and is a new invented header replacing the genuine owner hud.h).
- Variant vTU (edits/v_tu.py on optA): the three layouts folded back TU-local into game_engine.c verbatim (their only
  consumer; hudg + itmc + netgame structs, both macros, both constants, `extern struct hud_globals_definition
  *hud_globals;` like hud_messaging/hud_nav_points/hud_unit do), nav-point trio moved into genuine hud.h (new
  `prototypes/HUD_NAV_POINTS.C` section + `union real_point3d;`), hud_nav_points.h deleted, hud_nav_points.c keeps F04's
  forward-decl removal (hud.h now declares it), game_engine.c includes interface/hud.h.
  RESULT: game_engine 179/180 - _populate_statistic_buffer RESIDUAL (alndiff_vTU_populate.txt: one `mov edi,[ebp+8]`
  scheduled one slot later = tie). Full sweep head vs vTU: 446 SAME + game_engine: hud.h +3 prototypes disturbs NO
  other TU at cdc8ebd3 (the worker's 954eebd2 main.c park drift is gone); the cost is game_engine.c itself having to
  include hud.h (23 new function names: hud_initialize .. hud_play_sound).
- Count oracle (LAB ONLY, tools/count_oracle.py, K dummy enum constants after MULTIPLAYER_MAXIMUM_PLAYERS in vTUnp):
  K=1-7 exact, K=8-12 populate RESIDUAL, K=13-40 exact => populate_statistic_buffer is a declared-name-count tie; the
  hud.h include lands it in a flipping band. Genuine-hud.h placement of the nav trio is therefore infeasible without
  name-count compensation (held Layer-2 class).
- Variant vTUnp (edits/v_tunp.py on optA): only the THREE single-consumer headers folded TU-local (same text as vTU);
  hud_nav_points.h + F04 kept. RESULT: 180/180, 0/302 changed vs base; full sweep head vs vTUnp 446 SAME + game_engine
  (same two notes); optA vs vTUnp 447/447 SAME (canonical) => byte-identical to option A in every TU.

## S6 genuine broad-header placement of the layouts (the ruling's preferred home) - measured, INFEASIBLE
- hudg -> hud_definitions.h / hud.h: not compilable without a HUD-wide rewrite (7 TUs define their own struct
  hud_globals_definition: hs, hs_compile, hud, hud_messaging, hud_nav_points, hud_unit, ui_widget; main.c/interface.c
  declare `hud_globals` with other types). Kept TU-local in every genuine-header variant.
- optA6 = option A + F06 (sibling ctf/king/oddball/race drop their local scenario_netgame_flag for
  scenario_netgame_definitions.h; F06 still applies at cdc8ebd3 with -7/-4 line offsets): sweep 446 SAME + game_engine.
- vGen (edits/v_gen.py on optA6: itmc -> item_definitions.h, netgame structs + flag enum -> scenario_definitions.h,
  siblings via F06, hudg TU-local, hud_nav_points.h kept): game_engine 180/180, but sweep 443 SAME + 4 DIFF:
  ai_communication `_ai_communication_update_speech_timers` EXACT -> RESIDUAL (strict regression, 45 -> 44 exact);
  ai `_ai_test_ballistic_line_of_fire` bytes move (PARKED -> park goes stale); hud_weapon `_render_weapon_hud` moves.
- Bisect: vGenI (itmc -> item_definitions.h only; netgame TU-local; base optA): 445 SAME + game_engine + hud_weapon
  (`_render_weapon_hud`, non-exact, not parked: 4 bytes change, alndiff diff lines 794 -> 796 = slightly worse).
  vGenS (netgame -> scenario_definitions.h only + F06 siblings): same 4 DIFFs as vGen.
- => no layout can move to its genuine broad header at zero regressions at cdc8ebd3; same conclusion as the
  worker's 954eebd2 measurements (different victims now).

## S7 objdiff 3.3.1 + semantic audit on the candidates (odproj/, mini project; target = build/split obj)
- objdiff-cli 3.3.1 report: prod, optA, optB, vTUnp all 27922/32397 code, 173/180 fns, 3792/3792 data; 0 per-function
  fuzzy deltas vs prod.
- `python -B -m tools.audit_semantic_matches --project odproj ...` (outputs only odproj/semantic_report.json):
  accepted_ledger 180/180 for every candidate; the same 7 functions (nonplayer_post_rasterize, post_rasterize,
  remap_vehicle, remap_weapon, update, internal_rasterize_score, populate_statistic_buffer) credited by
  'semantic-coff' only, exactly as in production's build/semantic_report.json. 0 rejected, 0 unit errors.
- => after the status flip, the strict report keeps game_engine at 0 function gap / 0 data gap (it is already a
  tools/audit_object_admission.py CANDIDATE at HEAD), so revoke_incomplete_units will not revoke it:
  Halo objects 379 -> 380.

## S8 packets assembled (patches/), all checked at cdc8ebd3
- optionA/: 01 = review4 F01R verbatim (alias), 02 = worker F02 verbatim, 03 = review4 F03aR verbatim, 04 = worker F03b
  verbatim, 05 = worker F04 verbatim, 06 = NEW status flip (config.json game_engine.c NonMatching -> Matching; generated
  by tools/pgen.py from the HEAD blob, LF, 1 line).
- optionB/: 01 = NEW F01RB (HEAD -> optB_seq game_engine.c; = F01R + F07 in one standalone patch, since F07 alone
  does not --check at cdc8ebd3), 02-06 as A. edits/f07.py on optA == F07 applied (byte check).
- variant_no_single_consumer_headers/: 01A = NEW F01R-NP (HEAD -> vTUnp game_engine.c, alias), 01B = NEW F01RB-NP
  (HEAD -> vTUnpB, view copy), 02 = NEW F02-NP (hud_nav_points.h only, text identical to F02's), 03-06 as A.
- `git apply --check` on the real tree: all 19 patch files alone: clean; each packet's full set in ONE call (files are
  disjoint): optionA OK, optionB OK, NP-A OK, NP-B OK. git status clean before/after.
- Independent reconstruction: HEAD export + each packet's source patches via mkshadow (pkA, pkB, pkNPA, pkNPB) ==
  the measured shadows optA, optB_seq, vTUnp, vTUnpB byte-for-byte (all files) => every S2/S5/S7 measurement applies
  to the shipped patches.
- vTUnp/vTUnpB battery: object_audit PASS (text == optA's), pdb 0 disagreements, surplus 11/0, provider_link PASS;
  vTUnpB obj == vTUnp obj (no content diff); /W3 optA vs vTUnp: same 6 game_engine warnings modulo line numbers.
- /Od 0x5b23c0 goal_matches_player DOES keep a goal pointer slot ([ebp-8] = &global_goal[i]) - the alias idiom is
  authentic in this TU, just not in 0x5a20e0. January assert strings attest Bungie view casts:
  actors `(real_vector2d *) &actor->input.facing_vector`, actor_looking `(real_vector2d *) &actor->output.facing_vector`,
  actor_moving `(real_vector2d *) movement_direction`, observer `(real_point3d *) &observer->focus_offset`.

## S9 deliverables written
- patches/ (optionA, optionB, variant_no_single_consumer_headers; APPLY_ORDER.txt), MEASURED.md, RULING.md.
- RULING Q1 = alias (A) vs /Od-attested view copy (B) vs hold; Q2 = F02 focused headers vs the TU-local variant
  (hud_nav_points.h still needed); Q3 = disclosure of the pre-existing debug_player_color aggregate + descriptive
  .bss names (HCEX: static locals of game_engine_update_teleporter).
- Nit noted: dead local `local_player_count` (select_players_to_display) shadows the public function
  local_player_count(); byte-inert rename possible, not done (no new source forms this wave).
- apply_ready: every patch `git apply --check` clean at cdc8ebd3 (alone and per packet), every packet rebuilds the
  measured shadow byte-for-byte, and all gate/audit/link/semantic claims re-measured here hold. Tree clean at end.
