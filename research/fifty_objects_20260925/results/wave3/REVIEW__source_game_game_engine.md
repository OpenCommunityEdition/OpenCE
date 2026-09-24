# REVIEW source/game/game_engine

approve: False

## per_object
[
 {
  "unit": "source/game/game_engine",
  "approve": false,
  "issues": "The worker packet 01-04 is byte-exact and every claim checks out: 180/180, audit PASS, surplus 11 identical, provider link PASS, objdiff equal to production, and a 446-TU sweep with 0 changes. Admission is still blocked by section-8(g) constructs that the packet leaves in place. (1) update_teleporter and find_closest_player_index hand-expand distance_squared3d with a scrambled order or decoration parentheses; /Od calls the helper, and the helper spelling is EXACT while the plain-order strip tests are residual. (2) The `(long)goal < (long)&global_variant` csplit-alias loop bounds in render_nav_points and custom_motion_sensor_positions; the /Od-shaped `for (goal_index < 32)` loops are EXACT. (3) goto/hand-PIN clamps, a dead `result != NONE` check, an entry_placement[-6] pointer walk, and comma-chain hand-expansion of the TU's own ranking comparator; all are EXACT once fixed. (4) The invented opaque `postgame_statistic_entry {long values[7]}` (HCEX statistic_buffer), a hand-lowered struct return, and a decoration cast. The amended packet R01-R04 fixes all of these and is fully verified (scratch\\w\\review3_game_engine\\patches, final\\). The object still needs a dedicated pass for game_engine_man_out's hand-inlined is_out_of_lives with its gotos, the motion-sensor `goal` alias (the /Od view-copy alternative is owner-gated), rule-16 bare-int switches, and the remaining statistic-buffer pointer walks. CONSUMERS.txt misses the saved-games TUs; the corrected list is CONSUMERS_review.txt."
 }
]

## checks
All work is in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review3_game_engine\ (LEDGER.md R0-R12, REVIEW.md, lab\*).

HARNESS (rcompile.py)
- Uses the production cflags from build.ninja and runs with the shadow root as cwd, so __FILE__ and include spellings equal production.
- Controls: the real-tree game_engine.c and the untouched shadow game_engine_ctf both equal build/base; only .debug$S (the object name) differs.

WORKER PACKET 01-04
- git apply --check is clean individually and together against the tree. A git archive of 954eebd2 plus 01-04 is byte-equal to the worker's final/.
- 180/180 EXACT vs build/split; 0 of 302 section-defining symbols changed vs build/base.
- Symbol multiset identical to production (853/853, with $L and user labels collapsed).
- object_audit PASS, identical to production's audit. surplus_identity: 11 COMDATs, all identical. provider_link on the candidate: PASS. pdb_storage: 0 disagreements. No parks or rejections.
- objdiff-cli 3.3.1 mini-project: production, worker and amended objects all score 27922/32397 code, 173 fns, 3792/3792 data.
- HCEX types: independent DIA2Dump -type dumps. 34 compile-time offset/size checks in lab\layout_check.c all pass.
- HCEX signatures (-sym) match: game_engine_render_nav_points(short), the nav-point prototypes, rasterizer_draw_unicode_string, sound_class_set_gain.
- pad3 is January's own hs global string.
- /Od 0x5b25d0 confirms the handle_custom_starting_equipment shape and /Od declaration order.
- rsweep.py full sweep, with a sentinel check: all 446 source TUs (the worker covered 440) changed 0 sections.
- /W3 over 446 TUs: only the C4013 console_printf warning disappears.
- rconsumers.py consumer census: CONSUMERS.txt misses the saved-games TUs.

WHOLE-OBJECT SOURCE SCAN
Evidence: /Od (odbuild: 0x5afba0, 0x42dc80, 0x59aae0, 0x5a20e0, 0x5ad780, 0x5a4360, 0x59bb90, 0x5b6f70, 0x5a0b90), HCEX -sym/-type (statistic_buffer, get_player_place), and a strip-test lab of about 25 variants (lab\variant.py, mk_*.py).

AMENDED PACKET R01-R04
- git apply --check clean individually and together. A fresh archive plus R01-R04 equals final\ byte-for-byte.
- 180/180 EXACT; audit PASS (identical to production); provider_link PASS; symbol multiset identical; objdiff 3.3.1 equal to production; fake_match_scan 0 leads; /W3 unchanged apart from C4013.
- 446-TU sweep: 445 identical. game_engine differs only in two raw relocation spellings, which are strict EXACT.
- Optional R05/R06 on top: same sweep result.

The tracked tree was verified clean (git status) throughout.

## issues
APPROVE=FALSE: the packet as submitted is not admissible. Every worker claim re-verified TRUE, but section-8(g) blockers remain in the object, one inside a function the packet edits.

BLOCKERS LEFT BY THE PACKET
Each fix was measured alone: 180/180 and 0 changed sections vs base.

1. Hand-expanded header helper in game_engine_update_teleporter.
   - Source: `vector_from_points3d(...,&delta); if (delta.j*delta.j + delta.i*delta.i + delta.k*delta.k > 1.0f)`.
   - /Od 0x5afba0 calls distance_squared3d (/Od 0x42dc80).
   - `distance_squared3d(&previous_flag->position, &unit->object.position) > 1.0f` is EXACT.
   - Strip test: plain i,j,k order is residual, so the scrambled order is the lever.
2. Hand-expanded distance_squared3d in find_closest_player_index.
   - Source: `real distance_squared = y * y + (x * x + z * z)`; /Od 0x59aae0 calls the helper.
   - The helper call is EXACT. Without the parentheses or with plain order it is residual (decoration is the lever).
3. csplit-relocation-alias trap.
   - Where: render_nav_points (which the packet edits) and custom_motion_sensor_positions, as `while ((long)goal < (long)&global_variant)`, with a false comment.
   - /Od 0x5ad780 and 0x5a20e0 are `for (goal_index < 32)` loops.
   - Natural loops are EXACT: coff_compare resolves _global_goal+0x400 to the same destination, and objdiff 3.3.1 also scores 100%.
4. Rules 13, 18 and 21. All are EXACT when fixed:
   - `goto result` clamp -> `PIN` (/Od 0x5a4360).
   - Dead `if (result != NONE)` in did_player_win_default -> /Od 0x59bb90 winning-team local.
   - rasterize_in_game_score `entry_placement[-6]` walk and hand PIN -> indexed loop plus PIN.
   - get_place_string hand PIN -> PIN.
   - populate_statistic_buffer comma chains that hand-expand the TU's own comparator -> `sort_statistic_buffer_ranking(previous, entry) == 0`.
5. Invented opaque aggregate: `struct postgame_statistic_entry {long values[7]}`.
   - Replaced by HCEX `struct statistic_buffer {player_index, score, custom, kills, deaths, assists, place}` (inert).
   - The hand-lowered struct return becomes HCEX's by-value `game_engine_get_player_place(long)` (EXACT).
   - Strip test: the `(unsigned long)` cast in is_place_tied is decoration; the plain form is EXACT.

MINOR
- HCEX `long flags` (the header uses unsigned long): fixed, inert.
- Focused headers get disclosure comments (matrix_math.h precedent).

DOC DEFECT
- patches\CONSUMERS.txt misses the path-with-space TUs, and the worker's sweep compiled only 440 TUs.
- Correct counts:
  - game_engine.h: 37, including saved games/playlist_profile.
  - hud_messaging.h: 8, including saved games/game_state.
  - sound_classes.h: 6, including game/game_engine.
  - player_control.h: 21.
  - players.h: 71.
- Full list: CONSUMERS_review.txt. All of these TUs came out identical in my 446-TU sweep.

FIX PRODUCED AND VERIFIED
Patches are in scratch\w\review3_game_engine\patches\; full files in ...\final\source\.
- R01_game_engine_c.patch replaces worker 01; source/game/game_engine.c only.
- R02_new_focused_headers.patch replaces worker 02 (comments and long flags).
- R03_shared_header_prototypes.patch is worker 03 unchanged; it is the shared-header patch.
- R04_hud_nav_points_owner_include.patch is worker 04 unchanged.
- Verification: 180/180, audit PASS, provider link PASS, objdiff equal to production, 446-TU sweep with 0 regressions and 0 park drift.
- Optional: R05 (players.h teleporter_index, plus a game_engine.c part regenerated after R01) and R06 (siblings).
- The integrator must still run full ninja and check report.json for _game_engine_player_get_custom_motion_sensor_positions and _game_engine_render_nav_points. They are strict EXACT and the objdiff 3.3.1 mini-report scores them 100%.
- Recommendation: land R01-R04 as a zero-credit source-quality packet. Do NOT mark game_engine complete yet.

STILL OPEN (blocks OBJECT_COMPLETE; not fixed)
- game_engine_man_out: two `goto man_out` plus a hand-inlined game_engine_player_is_out_of_lives. /Od 0x5a0b90 is a single `||` chain; every plain spelling is residual, so the construct is load-bearing and not /Od-attested.
- Motion-sensor copy: needs the pre-existing `goal` pointer alias (direct-index component copy is residual). The /Od 8-byte point2d copy form is a load-bearing view copy, which is owner-gated.
- Rule 16: bare-integer switch cases remain (statistic 0..4 is HCEX enum postgame_statistic, and others).
- populate_statistic_buffer and select_players_to_display: `entry++` pointer walks, and the `if (count >= MAX) {assert} else {store}` shape.
- Kept with evidence:
  - team_has_players `goto done`: /Od-consistent; the if/else form is residual.
  - Owner-landed BUG comment for the vestigial CTF team parameter.
  - The 5-of-6 starting-equipment loop comment.

INCIDENT
- The worktree's .git pointer file vanished at about 00:42 (not caused by this reviewer) and was back at 00:57.
- The tracked tree was verified clean.
- Shadow `git apply` needs GIT_CEILING_DIRECTORIES: inside the repo it silently ignores out-of-cwd paths.
