# REVIEW source/game/game_engine

approve: False

## per_object
[
 {
  "unit": "source/game/game_engine (whole-object admission)",
  "approve": false,
  "issues": "The motion-sensor copy is still owner-gated. The default in-loop goal alias is contradicted by the /Od 0x5a20e0 frame and is load-bearing (lab s4 is RESIDUAL). The F07 view copy is also load-bearing, so it fails the s.2 byte-inert strip test. Review3's open blocker is therefore still unresolved, and admission needs the owner to pick the alias or F07."
 },
 {
  "unit": "worker packet F01+F02+F03a+F03b+F04 (as submitted)",
  "approve": false,
  "issues": "F01 keeps get_rating `long inverse` on a refuted 'January ABI' claim: `boolean inverse` (HCEX, /Od movzx byte) is byte-identical, per lab s1, and `long` adds the new C4244. There are also three byte-inert /Od-order nits: render_nav_points' bare block, did_player_win_default's declaration order, and the flag name should be HCEX `_disable_dynamic_light_bit`. Every other claim reproduces: 180/180, sweep 446 SAME plus game_engine, audit/surplus/pdb/provider_link PASS, objdiff equal, consumers equal."
 },
 {
  "unit": "reviewer-amended packet F01R+F02+F03aR+F03b+F04 (zero-credit house-clean)",
  "approve": true,
  "issues": "Verified: git apply --check is clean on the tree; the independent shadow equals amended/; 180/180 EXACT; 0 of 302 sections changed vs build/base and vs the worker's final; object_audit PASS with identical text; 11/11 surplus identical; 0 PDB storage disagreements; provider_link PASS; objdiff 3.3.1 unchanged. The full 447-TU sweep gives 446 SAME plus game_engine (only the always_invis order and the _global_goal+0x400 spelling, both EXACT). /W3 adds no new warnings. It earns no credit on its own; the object is admitted only after the owner's motion-sensor ruling."
 }
]

## checks
Everything was run independently, and everything I wrote is under scratch/w/review4_game_engine/. The tree was clean before and after.

1. Setup and inputs:
   - Tree clean at 16542e46.
   - Read: WORKER_BRIEF sections 0-9, house rules, the lane log's held table, the worker LEDGER F0-F16 and the review3 REVIEW.
   - No parks or admission rejections exist for this object.
2. Patch application:
   - `git apply --check` on the real tree is clean for F01, F02, F03a, F03b and F04, each alone and all together.
   - F05h and F06 are also clean on the tree. F05c and F07 are clean on top of F01 (and on top of the amended F01R).
3. Independent shadow:
   - Built as `git -c core.autocrlf=false archive HEAD source`, then the patches via `git apply` with GIT_CEILING_DIRECTORIES, then converted to CRLF.
   - It equals the worker's final/ for all 10 files.
   - The HEAD shadow equals the working tree for 989/989 files.
4. Compile harness (rc.py):
   - Production cflags from build.ninja, cwd set to the shadow root.
   - gatecmp.py checks gate rows vs build/split and section_infos_equal vs build/base.
   - Worker final: 180/180 EXACT, 0 of 302 sections changed. Control: 180/180, 0 changed.
   - gate.py --source can't compile this packet, because its /I list points at the tree's unpatched headers.
5. Full sweep (sweep.py):
   - Covers all 447 build/base source TUs. Compares raw bytes, relocations by name and the full symbol table, excluding .debug$S, with $L and $label$N numbering canonicalised.
   - Control: 447/447 match build/base.
   - Worker packet: 446 SAME. game_engine differs only in:
     - the position of _game_engine_update_player_always_invis;
     - the `_global_goal+0x400` spelling of `_global_variant+0` in motion_sensor_positions and render_nav_points. Both functions are strict EXACT.
6. /W3 /Zs census over 447 TUs (w3.py, path-normalised):
   - game_engine: C4013 console_printf gone, two C4244 long->short gone, +1 C4244 long->boolean.
   - hud_nav_points: only the pre-existing C4013 moves.
7. Consumer census (/showIncludes over 447 TUs): identical to CONSUMERS.txt.
   - game_engine.h 37, player_control.h 21, hud_messaging.h 8, sound_classes.h 6.
   - New headers 1, 2, 1, 1. players.h 71.
8. Admission audit on my compiled obj:
   - object_audit PASS; its text is identical to production's.
   - surplus_identity (pointed at the candidate): 11 COMDATs, 0 differ.
   - pdb_storage (candidate): 0 disagreements.
   - provider_link: PASS.
   - objdiff-cli 3.3.1 mini project: prod, final and amended all score 27922/32397 code, 173/180 functions and 3792/3792 data, with 0 per-function deltas.
9. /Od re-dumps and readouts:
   - 0x5a20e0: the motion-sensor frame is count, player and goal_index only, with one 8-byte copy.
   - 0x5b6920: the dead local_player_count is attested.
   - 0x5b6370: get_rating reads `inverse` with movzx byte.
   - 0x59bb90: did_player_win_default's /Od declaration order.
   - 0x5ad780: render_nav_points declares goal_index before head_position.
   - 0x5a0b90: man_out.
   - 0x5aea50 -> 0x5af860 -> 0x5ae9c0: update, always_invis, test_trait.
   - /Od builders 0x5916e0, 0x591b30, 0x593e50, 0x596e30, 0x598820, 0x59a260: one SET_FLAG per bit, but the bit order and meanings differ from January.
10. HCEX enums: flag names (_game_engine_disable_dynamic_light_bit etc.), universal variant flags 0-5, game_engine_weapons 0-13, goal_radar.
11. Lab strip tests:
    - s1: get_rating `boolean inverse` is EXACT.
    - s2: get_rating with declaration and initialisation combined is EXACT.
    - s3: game_engine_update without the bare block is RESIDUAL.
    - s4: motion-sensor component copies without the alias are RESIDUAL.
    - s5: did_player_win_default in the /Od declaration order is EXACT.
    - s6: render_nav_points without the bare block is EXACT.
12. Scans: bare blocks, prototypes, externs, placeholder names and `float` in the final file.
13. Amended packet (F01R + F03aR), re-verified end to end:
    - Applies cleanly to the tree.
    - Independent shadow equals amended/.
    - 180/180; 0 of 302 sections changed vs build/base and vs the worker's final.
    - Audit, surplus, pdb and provider_link all pass.
    - objdiff unchanged.
    - Full sweep: 446 SAME plus game_engine (same two notes).
    - /W3: no new warnings.
    - Optional F05h, F05c, F06 and F07 on top: 446 SAME plus game_engine, 180/180.

## issues
APPROVE=FALSE: the object cannot be self-admitted.

**BLOCKER (owner ruling needed; I cannot fix it): the motion-sensor copy in game_engine_player_get_custom_motion_sensor_positions.**
- Review3 listed this construct as "still blocking OBJECT_COMPLETE". It is still unresolved; the worker itself writes "OWNER picks".
- The default packet keeps the in-loop `struct netgame_goal *goal = &global_goal[goal_index];` alias.
  - /Od 0x5a20e0 contradicts it: the frame has only count, player and goal_index, and the copy is one 8-byte struct copy.
  - The alias is load-bearing: lab s4 (direct component copies) is RESIDUAL.
  - That makes it a non-/Od-attested, load-bearing local, which is owner-gated (brief s.7).
- The alternative, F07's /Od-attested `*(real_point2d *)&...position` view copy, is also load-bearing. It fails the byte-inert strip test that brief s.2 requires for a new 3D-to-2D view cast, so it is owner-gated too.
- An object whose completion depends on an owner pick is not an OBJECT_COMPLETE candidate.

**DEFECT (fixed in my amendment): refuted evidence claim in the worker's F5.**
- The worker kept `long inverse` in postgame_statistic_get_rating, calling it "January's ABI, proven by the dword read" and saying `boolean inverse` is residual.
- Lab s1 changed only the get_rating prototype and definition to `boolean inverse`. Result: 180/180 EXACT and 0 of 302 sections changed. VC7 forwards a boolean parameter with the same dword load.
- HCEX (`unsigned char inverse`) and /Od 0x5b6370 (`movzx eax, byte ptr [ebp+0x10]`) both attest boolean.
- So `long` is not byte-required, contradicts first-party evidence, and is the only source of the packet's new C4244 long->boolean warning.

**NITS (fixed, all byte-inert and verified):**
- render_nav_points:
  - HEAD's bare `{ long goal_index; for ...}` block after unit_get_head_position contradicts /Od 0x5ad780's declaration order, where goal_index (-0x10) comes before head_position (-0x24). This is law (d).
  - Lab s6 (no block; goal_index declared after player) is EXACT, so the block is decoration.
- did_player_win_default declares player before winning_team_index. /Od 0x59bb90 has winning_team_index first (slots -0x14 then -0x18). Lab s5 is EXACT.
- The flag name `_game_engine_disable_dynamic_lighting_bit` should be HCEX's verbatim `_game_engine_disable_dynamic_light_bit`. The rename is count-neutral.

**Verified acceptable (disclosed):**
- game_engine_update's three disjoint bare-block iterators.
  - They are load-bearing: lab s3 without the block is RESIDUAL.
  - The escape law (e) justifies them: the iterators' addresses reach data_iterator_new/object_iterator_new.
  - No /Od attestation is possible, because the 2020 build moved the postgame loops out of update. The /Od declaration order is consistent.
- The /Od-attested dead local_player_count.
- The strip-inert, /Od-attested `= 0` initialisations.
- The TU-local flag bits 3, 4 and 5, which avoid breaking the header's name-count canaries.
- The focused headers, following the matrix_math.h precedent.
- The weapon-set-10 name `_no_grenades`, based on January's behaviour (HCEX has 13 = no_grenades).
- The worker's sweep count was 446 TUs; build.ninja has 447. My 447-TU sweep gives the same result.

**Advisory, zero-credit follow-ups (these predate the packet and do not block it):**
- Scalar-only bare blocks not audited against /Od:
  - post_rasterize_post_game's entry_index block;
  - update_player_no_shield's player block;
  - update_teleporter's message and angle blocks;
  - postspawn's grenade block.
- The goal_radar 0/1/2 literals, where HCEX has `enum goal_radar`.
- The `parameter0`/`parameter1` placeholder names in game_engine_playlist_next. Its TU prototype also duplicates game_engine_playlist.h.
- The 2020 byte local `inverse` in /Od select_players_to_display is not present in the packet.
- Other sites still mix `if (game_engine)` with /Od's `game_engine_running()`.
- game_engine_globals, global_stage and timeout_for_endgame_sound are still extern.

**PRODUCTION CHANGES (zero credit; landable as a verified house-clean packet; admission waits on the owner's alias-vs-F07 pick). Land these together:**
- C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review4_game_engine\patches\F01R_game_engine_c.patch. It replaces F01; the full file is amended\source\game\game_engine.c.
- The worker's F02_new_focused_headers.patch, unchanged.
- C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review4_game_engine\patches\F03aR_shared_header_game_engine_h.patch. It replaces F03a; the full file is amended\source\game\game_engine.h.
  - It is a shared header with 37 consumer TUs, the same list as CONSUMERS.txt:
    - ai/encounters, camera/dead_camera;
    - game/{game, game_engine, game_engine_ctf, game_engine_king, game_engine_list, game_engine_oddball, game_engine_race, game_engine_slayer, game_globals, game_statistics, players};
    - interface/{hud, hud_messaging, hud_unit, interface, motion_sensor, player_ui, ui_widget};
    - items/{items, projectiles, weapons};
    - main/main, math/random_math;
    - networking/{network_client_manager, network_client_message_handler, network_game_manager, network_server_manager, network_server_message_handler};
    - objects/{damage, object_lights, objects};
    - render/render, `saved games/playlist_profile`;
    - units/{bipeds, units}.
- The worker's F03b_shared_header_prototypes.patch, unchanged. Consumers: player_control.h 21, hud_messaging.h 8 (includes `saved games/game_state`), sound_classes.h 6.
- The worker's F04_hud_nav_points_owner_include.patch, unchanged.
- The worker's F02/F03b/F04 patches are in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\game_engine_finish\patches\.
- Optional F05h/F05c/F06/F07 still apply on top and sweep clean. F07 is the owner's motion-sensor choice.
- config/symbols.json: no change.

Evidence: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review4_game_engine\REVIEW.md and LEDGER.md (R0-R11), plus lab\s1..s6.
