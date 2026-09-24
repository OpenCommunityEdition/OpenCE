# review4_game_engine LEDGER (adversarial review of scratch/w/game_engine_finish; tree 16542e46)

## R0 setup
- Tree clean at 16542e46 (git status empty). Read WORKER_BRIEF s.0-9, house rules, lane log (game_engine not in held table),
  worker LEDGER F0-F16, review3 REVIEW.md. No parks / no admission rejections name source/game/game_engine.

## R1 independent shadow + patch application
- shadow_lf = `git -c core.autocrlf=false archive HEAD source` + `git apply` F01,F02,F03a,F03b,F04 (GIT_CEILING_DIRECTORIES=slug);
  all applied cleanly. mkcrlf.py -> shadow/ (CRLF like the working tree). shadow_base (HEAD export, CRLF) == working tree
  byte-for-byte (989/989). Patched files == worker final/ (EOL-normalised) for all 10 files.
- `git apply --check` on the real tree: F01, F02, F03a, F03b, F04 each alone and all together: clean. Tree still clean.
- rc.py (own harness; production cflags from build.ninja, cwd = shadow root, same srcrel spelling) + gatecmp.py:
  control (shadow_base) 180/180 EXACT vs split, 0/302 changed vs build/base.
  final (shadow)        180/180 EXACT vs split, 0/302 changed vs build/base (coff_compare section_infos_equal).
  (gate.py --source cannot be used for this packet: its /I list points at the tree's unpatched headers.)

## R2 own full sweep (sweep.py; 447 build/base source TUs; A = HEAD shadow, B = F01..F04 shadow; production cflags)
- Comparator: every section except .debug$S (holds the /Fo path): name, flags, size, raw bytes, relocations by target NAME;
  full symbol table (name, value, section name, type, storage); $L / `$label$NNNN` numbering canonicalised by first
  appearance (header edits shift the label counter; first pass without canonicalisation showed 29 label-only DIFFs, and
  units differed only in `$state_not_allowed$NNNN` numbering).
- CONTROL: all 447 A objs == build/base (same canon) -> harness reproduces production exactly.
- RESULT: 446 SAME, 1 DIFF = source/game/game_engine only:
  (a) order: _game_engine_update_player_always_invis emitted later (and its .debug$F follows it);
  (b) _game_engine_player_get_custom_motion_sensor_positions +0x83 and _game_engine_render_nav_points +0xba: raw reloc
      `_global_variant`+0 -> `_global_goal` with addend 0x400 in the bytes. Both still EXACT vs split (coff_compare).
  => worker's sweep claim reproduced (my count is 447 TUs; worker said 446).
- WARN text changed (same objs) for game_engine_king, libtiff tif_swab, tif_lzw -> to be checked in the /W3 pass.

## R3 /W3 /Zs census (w3.py; 447 TUs, both shadows, path-normalised)
- Only 2 TUs change: hud_nav_points (pre-existing C4013 'error' just moves 351->346 because of F04's include edit) and
  game_engine: C4013 console_printf gone; 6 -> 4 C4244 long->short; double->float becomes double->real (same pow return);
  long->byte kept; +1 C4244 'function' long->boolean at 5549 (get_rating -> populate `inverse`). Worker claim confirmed
  (worker omitted the two vanished long->short warnings; improvement, not a defect).

## R4 source review: key /Od checks so far
- /Od 0x5a20e0 (custom_motion_sensor_positions) re-dumped (od_5a20e0.txt): frame 0xc = count(short,-4), player(-8),
  goal_index(-0xc); NO goal pointer slot; one 8-byte struct copy (`shl ecx,5; mov edx,[x]; mov eax,[y]; mov [esi+ecx*8],..`).
  => the DEFAULT packet's in-loop `struct netgame_goal *goal = &global_goal[goal_index];` alias is CONTRADICTED by the /Od
  frame and is load-bearing (worker's ms_n1 strip -> residual). review3 listed this construct as "still blocking
  OBJECT_COMPLETE"; the worker did not resolve it (defers to owner: alias vs F07 load-bearing view copy). Both remaining
  exact forms are owner-gated classes (brief s.2 view-cast rule needs a byte-inert strip test; s.7 non-/Od-attested
  load-bearing local). => object admission needs an owner ruling.
- /Od 0x5b6920 select_players_to_display: [ebp-0x1dc] local_player_count = 0, incremented in loop 1 when
  player->local_player_index != NONE, never read again (dead in /Od too). Worker's dead counter is /Od-attested and
  byte-inert (law f). /Od also has a byte local [ebp-0x1cd] = 0 passed as populate's 3rd arg (2020 named `inverse`
  local) that the worker omitted (omission, not an addition; noted only).
- Builders: /Od 2020 builders (0x5916e0, 0x591b30, 0x593e50, 0x596e30, 0x598820, 0x59a260) DO spell one SET_FLAG per bit
  (constant-condition ternaries; od_builder_order.py). Their bit ORDER (1,2,3,4,0,5,7,8 / 3,4,2,5,0,6,1,7,8) and bit
  semantics differ from January (2020 has bits 6-8, bit5 set where January clears it), so the worker's ascending order
  is not contradicted by attestable evidence. OK.

## R5 consumer census (consumers.py: CL /Zs /showIncludes over all 447 TUs in the patched shadow)
- game_engine.h 37, player_control.h 21, hud_messaging.h 8, sound_classes.h 6, hud_globals_definitions.h 1,
  hud_nav_points.h 2, item_collection_definitions.h 1, scenario_netgame_definitions.h 1, players.h 71:
  EQUAL to the worker's CONSUMERS.txt (incl. `saved games/` paths). Full sweep (R2) already covers every TU anyway.
- Prototype owners verified: unit_get_local_player_index -> player_control.c:337; hud_get_font_index/hud_get_text_color
  -> hud_messaging.c:1240/1252; sound_class_set_gain -> sound_classes.c:410; nav-point trio -> hud_nav_points.c;
  multiplayer-sound pair -> game_engine_multiplayer_sounds.c (no dedicated header exists; game_engine.h is the closest).
- New focused headers follow the landed matrix_math.h precedent; HCEX layouts use `unused` members (review3 checked 34
  offsets). Not in HCEX's header list (which only lists headers with line info) - naming precedent only.

## R6 admission audit of my compiled final obj (final_ge.obj from shadow = HEAD + F01..F04)
- object_audit PASS (269 January symbols, 0 differ); audit text identical to production's (paths stripped).
- surplus (surplus_cand.py = surplus_identity pointed at the candidate): 11 candidate-only code COMDATs, 0 not identical.
- pdb_cand.py (pdb_storage on candidate symbols): 269 split symbols, 0 disagreements with cachebeta publics.
- provider_link source/game/game_engine final_ge.obj: SELECTED-PROVIDER LINK PASS.
- objdiff-cli 3.3.1 mini project (odproj/): prod and final both 27922/32397 code, 173/180 fns, 3792/3792 data;
  0 per-function deltas; motion_sensor_positions and render_nav_points 100% in final despite `_global_goal+0x400`.
- build/report.json (production): same 7 sub-100 fns (remap_vehicle/weapon, populate, internal_rasterize_score, update,
  nonplayer_post_rasterize, post_rasterize) = pre-existing objdiff under-credit (all strict EXACT).

## R7 lab strip/variant tests (labvar.py: edit final game_engine.c, compile in shadow_lab with production cflags)
- /Od 0x5b6370 get_rating: `movzx eax, byte ptr [ebp+0x10]` = 2020 `boolean inverse` (HCEX: unsigned char inverse too).
- s1_rating_boolean: get_rating `boolean inverse` (prototype + definition) -> 180/180 EXACT, 0/302 changed vs final.
  **REFUTES the worker's F5 claim** ("gr_a1 boolean inverse RESIDUAL ... long inverse = January's ABI, proven by the dword
  read"): VC7 forwards a boolean parameter with the same dword load. `long inverse` contradicts HCEX + /Od, is not
  byte-required, and is the sole source of the new C4244 long->boolean warning. DEFECT (fixable).
- s2_rating_noinit: `long player_count = populate_statistic_buffer(...)` (combined) -> EXACT, 0 changed: the worker's
  `= 0` then assignment is strip-inert and /Od-attested (mov [ebp-0x1d0],0 then store) - acceptable either way.
- s3_update_noblock: first player_iterator declared at the top of `if (game_engine)` (no bare block) -> _game_engine_update
  RESIDUAL. The bare block is load-bearing; justified only by the escape law (e) (iterators' addresses reach
  data_iterator_new/object_iterator_new) - no /Od attestation possible (2020 moved the postgame loops out of update:
  /Od 0x5aea50 calls 0x5a6650/0x59b8e0 there). /Od declaration order (player_iterator first local) is consistent.
- s4_motion_components: direct component copies without the alias -> custom_motion_sensor_positions RESIDUAL
  (confirms the alias is load-bearing).
- s5_dpw_declorder: did_player_win_default with the /Od declaration order (team0, team1, winning_team_index, player;
  /Od 0x59bb90 slots -0xc,-0x10,-0x14,-0x18) -> EXACT, 0 changed. Worker declares player before winning_team_index
  (contradicts /Od order; byte-inert nit).

## R8 amended packet (make_amended.py -> patches/F01R_game_engine_c.patch, patches/F03aR_shared_header_game_engine_h.patch)
- F01R = worker F01 + (1) get_rating `boolean inverse` (HCEX unsigned char; /Od 0x5b6370 movzx byte) in prototype +
  definition; (2) did_player_win_default /Od declaration order (winning_team_index before player);
  (3) HCEX verbatim `_game_engine_disable_dynamic_light_bit` (HCEX <unnamed-tag> enum: disable_dynamic_light,
  disable_integrated_lights, 5_or_more, 9_or_more, game_over_sound_disabled). F03aR = worker F03a + (3) (count-neutral).
- HCEX also confirms universal flags 0..5 = allow_motion_tracker, allow_friendly_navpoints, infinite_grenades, no_shields,
  always_invisible, generic_starting_equipment; game_engine_weapons 0..13 (10 covenant, 13 no_grenades) as the worker says.
- git apply --check on the real tree: F01R, F03aR alone and F01R+F02+F03aR+F03b+F04 together: clean.
- shadowA = HEAD export + F01R,F02,F03aR,F03b,F04 (git apply) == amended/ files. rc.py: 180/180 EXACT vs split,
  0/302 changed vs build/base, 0/302 changed vs worker final obj.

## R9 amended packet verification
- Full sweep (shadow_base vs shadowA, 447 TUs): 446 SAME + game_engine (only the same order + `_global_goal+0x400`
  spelling as the worker packet). Amended game_engine obj: 180/180 vs split, 0/302 changed vs build/base.
- /W3 census (shadow_base vs shadowA): only hud_nav_points (pre-existing C4013 moves) and game_engine: C4013 gone,
  two long->short gone, NO new warnings (the worker packet's C4244 long->boolean is gone with `boolean inverse`).
- Audit on amended_ge.obj: object_audit PASS (text identical to production), 11 surplus COMDATs identical, 0 PDB storage
  disagreements, provider_link PASS.
- Optional F05h/F05c/F06/F07 apply cleanly on top of the amended packet (F05h and F06 also on the bare tree); sweep of
  amended+optionals (shadowO): 446 SAME + game_engine (same notes); game_engine 180/180.

## R10 render_nav_points bare block (pre-existing, but the packet edits this function)
- /Od 0x5ad780 frame: player_index -8, player -0xc, goal_index -0x10, unit_index -0x14, head_position -0x24 (RTC):
  goal_index is declared BEFORE head_position. The packet keeps HEAD's bare `{ long goal_index; for ... }` block AFTER
  unit_get_head_position (declared after head_position) -> contradicts the /Od declaration order (law d).
- s6_nav_noblock (block removed; `long goal_index;` declared right after `player`) -> 180/180 EXACT, 0 changed: the block is
  byte-inert decoration. Added to the amended F01R as fix (4). make_amended.py regenerated; shadowA rebuilt from HEAD +
  F01R,F02,F03aR,F03b,F04: == amended/, 180/180, 0/302 vs base, 0/302 vs worker final; audit PASS (identical text),
  surplus 11/0, pdb 0, provider_link PASS, objdiff 3.3.1 27922/32397, 173/180, 3792/3792 (same as prod).
- Optional F05h/F05c/F06/F07 still `git apply --check` clean on top of the rebuilt amended shadow.
- Emission order: January-owned fn order edit-ops vs January: base 31, amended 31 (only always_invis moves; audit keys by
  owner symbol, not order).
- Other pre-existing bare blocks (not new; advisory only): post_rasterize_post_game 1860/1907/1994, update_player_no_shield
  3177, build_lighting 3199, update_teleporter 3472/3488/3531/3577, postspawn 7492 (line numbers in amended file). Several
  hold address-taken locals (escape law); the scalar-only ones (1907, 3177, 3531, 3577, 7492) were not /Od-audited here.

## R11 final
- Final amended sweep (shadowA with F01R incl. fix 4): 446 SAME + game_engine (order + `_global_goal+0x400` only);
  control 447/447 == build/base; /W3: no new warnings (C4013 + two long->short removed).
- VERDICT approve=false: object admission blocked by the unresolved owner-gated motion-sensor copy (alias contradicted by
  /Od frame + load-bearing; F07 view copy load-bearing). Amended packet F01R+F02+F03aR+F03b+F04 is verified and landable
  as a zero-credit house-clean packet; admission after the owner picks alias vs F07. REVIEW.md written.
