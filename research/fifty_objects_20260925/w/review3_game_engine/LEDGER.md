# review3_game_engine LEDGER (adversarial review of scratch/w/game_engine_clean, tree 954eebd2)

## R0 environment
- 00:42 (approx): the worktree's `.git` pointer FILE is missing (C:\halo-worktrees\claude-fifty-objects-20260925\.git).
  Not caused by this reviewer (first failure was on a read-only `git archive`; the admin dir
  halo-campaign.git/worktrees/claude-fifty-objects-20260925 still exists, HEAD 954eebd2, index intact).
  Read-only `git --no-optional-locks --git-dir=<admin> --work-tree=. status --short` = clean.
  NOT repaired by me (outside slug). Integrator must restore:
  `gitdir: C:/Users/isabe/Documents/Codex/2026-09-20/files-pasted-by-the-user-halo/work/halo-campaign.git/worktrees/claude-fifty-objects-20260925`
- All git reads below use `--git-dir=<admin>`.

## R1 patches apply
- `git apply --check` 01,02,03,04 individually and together against the tree: clean.
- Independent shadow: `git archive 954eebd2 source` -> shadow/, apply 01..04 -> every patched file byte-equal to
  worker final/ files; real tree untouched.

## R2 independent compile (rcompile.py: production cflags, cwd = shadow root so __FILE__/include spellings = production)
- Control: real tree game_engine.c -> 180/180 vs split, 0 changed sections vs build/base (only .debug$S obj-name differs).
- Control: shadow game_engine_ctf (untouched by 01-04) -> 39/39, 0 changed vs base.
- Patched shadow game_engine.c -> final_ge.obj: 180/180 EXACT vs split; 0 of 302 section-defining symbols changed vs base.
- Symbol table multiset (name, storage, section name, value; $L collapsed) identical to production: 853/853.
- object_audit PASS (audit_final_review.txt, identical to audit of build/base); surplus_identity 11 COMDATs, 0 not identical;
  provider_link on final_ge.obj: SELECTED-PROVIDER LINK PASS; pdb_storage production 0 disagreements (candidate symbols identical).
- Data: report.json 3792/3792 data 100%; objdiff 173/180 fns (7 undercredited, pre-existing, hidden_exact per wave-2 review).
- No parks / no admission rejections for game_engine.

## R3 HCEX layout verification (independent DIA2Dump -type dumps: hcex_types_review.txt) + compile-time lab (lab/layout_check.c)
- 34 offset/size checks compiled with the patched headers: ALL PASS (hudg 0x450, messaging 0x120, fonts .index 0x54/0x64,
  carnage 0x3D4, netgame flag 0x94, equipment 0x90 (spawn_time 0xE, item_collection.index 0x5C), starting 0xCC
  (item_collection[k].index 0x48+0x10k), permutation 0x54 (item.index 0x30), itmc 0x5C).
- Member names match HCEX. Deviation (nit): HCEX scenario_netgame_equipment.flags and scenario_starting_equipment.flags
  are `long`; the header spells `unsigned long` (carried over from the old views).

## R4 full-board sweep (rsweep.py; all 446 `source/**.c` TUs in build.ninja, incl. the 6 "saved games/films" TUs)
- Comparator: every section (name/flags/size/raw bytes except .debug$S; relocations with $L / user-label (storage 6) /
  section-symbol targets normalised to (section,value)) + symbol-table multiset.
- Sentinel (shadow_sentinel: one height_adjust 0->1 in game_engine_rasterize_message) -> DIFF detected in game_engine only.
- First pass (only $L normalised) flagged units: `$state_not_allowed$34582` -> `$34586` user-goto-label suffix in
  `_unit_animation_state_interruptable` (jump-table reloc target, storage 6). Raw bytes identical; same class as $L
  renumbering (game_engine.h +2 names shifts the counter); gate says 189/189 and coff_compare normalises it.
- Final: 446/446 SAME (0 regressions, 0 park drift).

## R5 consumer census (rconsumers.py, CL /Zs /W3 /showIncludes, shadow) + warnings
- CONSUMERS.txt is INCOMPLETE (worker deps tool dropped paths with spaces; worker sweep = 440 TUs, not 446):
  game_engine.h 37 (+ saved games/playlist_profile), hud_messaging.h 8 (+ saved games/game_state),
  sound_classes.h 6 (+ game/game_engine itself), player_control.h 21 (same), players.h (optional 05) 71 (+ saved games/game_state);
  new headers: hud_nav_points.h {game_engine, hud_nav_points}, the other three {game_engine}.
  All of them are SAME in my 446-TU sweep, so this is a documentation defect only; corrected list in CONSUMERS_review.txt.
- /W3 warning sets real vs shadow over 446 TUs: only path-prefix differences, plus game_engine.c loses C4013 console_printf.

## R6 FINDING (blocker, section 8(g) hand-expanded header helper): game_engine_update_teleporter
- Source (production AND candidate): `vector_from_points3d(&previous_flag->position, &unit->object.position, &delta);
  if (delta.j * delta.j + delta.i * delta.i + delta.k * delta.k > 1.0f)` = hand-expanded magnitude_squared3d with a
  scrambled j,i,k operand order. Wave-2 review had "noted, not asserted" it; the worker did not address it.
- Evidence: /Od 0x5afba0 (string xref "failed to teleport %d") calls 0x42dc80 with (previous_flag, &unit->position);
  /Od 0x42dc80 = distance_squared3d (vector_from_points3d 0x42e3a0 + magnitude_squared3d 0x42df70, RTC 12-byte temp).
  /Od compares it against r*r with r = function-top local 1.0f (0x93dd5c) (the other top local is 0.5f 0x93dd50).
- Lab (lab/variant.py on the patched shadow file):
  tp_v1 `if (distance_squared3d(&previous_flag->position, &unit->object.position) > 1.0f)` (delta local dropped) ->
        180/180 EXACT, 0 changed sections vs base (no new surplus).
  tp_v2 vector_from_points3d + magnitude_squared3d(&delta) -> residual; tp_v5 nested magnitude_squared3d(vector_from_points3d(..)) -> residual;
  tp_v3 STRIP TEST of the j,i,k order (plain i,j,k) -> residual => the scrambled order IS the lever (fake-match class).
- => FIXABLE: adopt tp_v1 (the /Od spelling). Amended patch to follow.

## R7 FINDING (blocker, section 8(g)): find_closest_player_index hand-expanded distance_squared3d
- Source: `real x = cand.x - cam.x; real y = ...; real z = ...; real distance_squared = y * y + (x * x + z * z);`
  (decoration parentheses + scrambled order).
- /Od 0x59aae0 (xref of the 0.13083334f and 400.0f constants; RTC names position/direction/buffer/target_position/
  autoaim_target_*): `distance_squared3d(&position, &target_position)` via 0x42dc80, then `< 400 local && < 30*30`
  (so the double 400/900 comparison is authentic). Later build adds parent-object handling (target_position), so only
  the helper call is attested.
- Lab: fc_v1 `real distance_squared = distance_squared3d(&camera_position, &candidate->object.position);` -> 180/180,
  0 changed sections vs base. STRIP TEST of the decorated form: fc_v2 (x*x+y*y+z*z) and fc_v3 (no parens) residual
  => the parentheses/order are the lever (fake-match class). FIXABLE with fc_v1.

## R8 FINDING (blocker, csplit-relocation-alias trap / invented cross-object reference): `&global_variant` loop bound
- game_engine_player_get_custom_motion_sensor_positions and game_engine_render_nav_points (the latter is edited by the
  packet) walk `goal` with `do {...} while ((long)goal < (long)&global_variant);` + comment "January uses the adjacent
  global_variant address as the signed loop bound". Introduced 2026-07-25 (c8a6ed6d, 5d7fd76e), before the no-fake rule.
  January's `cmp esi, imm; jl` is VC7 strength reduction of a signed `goal_index < 32` loop; the one-past address is
  merely NAMED _global_variant by csplit (memory: csplit-relocation-alias trap).
- /Od 0x5a20e0 (motion sensor) and /Od 0x5ad780 (render nav points; both callers of /Od goal_matches_player 0x5b23c0):
  `for (goal_index = 0; goal_index < 0x20; goal_index++)` indexing global_goal[goal_index] directly (shl 5), no pointer slot.
- Lab: rn_1 (render: for loop, direct global_goal[goal_index] indexing = /Od shape) EXACT; rn_2 (pointer local) EXACT.
  ms_n1 (motion: for loop, direct indexing, component .x/.y copies) residual; ms_n2 (for loop + in-loop
  `struct game_engine_goal *goal = &global_goal[goal_index];`, same idiom as goal_matches_player) EXACT;
  ms_n3 (direct indexing + `*(real_point2d *)&global_goal[goal_index].position` copy = /Od 8-byte copy shape) EXACT but a
  load-bearing view copy (owner-gated) -> not used.
  All EXACT variants: 0 changed sections vs base; coff_compare resolves `_global_goal+0x400` and `_global_variant+0` to
  the same .bss destination, so NO relocs.json change is needed (strict). objdiff 3.3.1 may score the relocation name
  differently -> integrator must check report.json for these two rows (semantic-coff proof covers strict matches).

## R9 whole-object source scan: MORE pre-existing section-8 / rule-13/18 constructs (all byte-inert to fix)
Each lab variant = patched shadow game_engine.c + one edit, compiled with production cflags; all EXACT rows below are
180/180 with 0 changed sections vs build/base.
- post_rasterize_in_game: `if (fade < 0) {fade = 0; goto result;} if (fade > 1) fade = 1; ... result:`.
  /Od 0x5a4360 shows PIN-style nested ternary. pr_v1 `fade = PIN(fade, 0.0f, 1.0f);` EXACT (goto NOT load-bearing);
  pr_v2/pr_v3 also EXACT.
- did_player_win_default: 3x `goto tied` + dead `if (result != NONE)` (result is 0/1 there = nonsensical logic).
  /Od 0x59bb90 (only /Od caller of team_has_players 0x5b6f70): winning-team local = tie ? NONE : (t0 > t1 ? 0 : 1),
  else team_has_players(0) ? 0 : 1; then NONE check. dpw_v1 (that local + one `goto tied`) EXACT; full single-exit
  dpw_v2 residual (January keeps the tied exit).
- team_has_players `goto done`: if/else form th_v1 residual; /Od shows a jump statement in the then-branch followed by
  an else (double jmp) -> goto is /Od-consistent. KEPT.
- game_engine_rasterize_in_game_score: `long *entry_placement = &entries[0].values[6]; ... entry_placement[-6]; ...
  entry_placement += NUMBEROF(entries[0].values);` in `if (n>0) do{}while` + hand-expanded PIN of the place.
  ris_v1 (for loop, entries[entry_index].values[0]/[6]) EXACT; ris_v2 (+ `PIN(... & 0x7F, 0, 15)`, as the same TU
  already spells it in another function) EXACT.
- populate_statistic_buffer: comma-expression chains `(previous_value = ..., entry_value = ..., a < b || a > b) || ...`
  hand-expand the TU's own sort_statistic_buffer_ranking comparator. psb_v1 `!sort_statistic_buffer_ranking(previous,
  entry)` EXACT, psb_v2 `== 0` EXACT.
- NOT fixed here (needs a dedicated pass): `struct postgame_statistic_entry { long values[7]; }` is an invented opaque
  view of HCEX `struct statistic_buffer {player_index, score, custom, kills, deaths, assists, place}` (49 constant-index
  uses); game_engine_get_player_place hand-lowers HCEX's by-value `struct statistic_buffer
  game_engine_get_player_place(long)` (`entry = *game_engine_get_player_place(&entry, player_index)`); is_place_tied
  `(values[6] & (unsigned long)0x80000000) > 0`; remaining `entry = entries; do {... entry++}` walks in
  populate_statistic_buffer / select_players_to_display.

## R10 more lab results (on the amended file)
- Layer I: HCEX `struct statistic_buffer {player_index, score, custom, kills, deaths, assists, place}` replacing the
  invented `postgame_statistic_entry {long values[7]}` (30 type uses, 44 field uses): 180/180, 0 changed sections.
- Layer J (strip test): is_place_tied `(entry->place & (unsigned long)0x80000000) > 0` -> plain `& 0x80000000` EXACT
  (cast = decoration); TEST_FLAG spelling residual (ipt_j1/j3).
- Layer K: HCEX by-value `static struct statistic_buffer game_engine_get_player_place(long player_index)` with
  `entry = game_engine_get_player_place(player_index);` EXACT (hand-lowered struct return unnecessary).
- scenario_netgame_definitions.h HCEX `long flags` (x2): EXACT, inert.
- game_engine_man_out: /Od 0x5a0b90 = `player->quit_out_of_game || is_out_of_lives(0x5a26c0) || is_odd_man_out`;
  our body has 2x `goto man_out` + a hand-inlined is_out_of_lives (re-player_get). mo_v1/v2/v3 (|| chains, with/without
  result local) all RESIDUAL -> open item, not fixed.

## R11 AMENDED PACKET (patches/R01..R04; final/ = full files)
- R01 = worker 01 + A..K (teleporter/find_closest distance_squared3d, nav/motion natural goal loops, PIN x2,
  winning-team local, comparator call, statistic_buffer, cast strip, by-value get_player_place, unused locals removed);
  R02 = worker 02 + disclosure comments + HCEX `long flags`; R03/R04 = worker 03/04 unchanged.
- git apply --check: R01..R04 individually and together clean vs the tree. Independent shadow_R (git archive 954eebd2 +
  R01..R04) == final/ byte-for-byte.
- gate: 180/180 EXACT (gate_amended.txt); object_audit PASS, identical to production audit; provider_link PASS;
  symbol multiset identical to production; objdiff-cli 3.3.1 mini-report (odproj/): prod, worker and amended all
  27922/32397 code, 173 fns, 3792/3792 data (the `_global_goal+0x400` relocation spelling scores 100%).
- rsweep shadow_R: 446 TUs, 445 byte-identical, game_engine differs only in the raw relocation spelling of
  _game_engine_player_get_custom_motion_sensor_positions and _game_engine_render_nav_points (coff_compare EXACT).
- /W3: game_engine C4146 x1 + C4244 x19 (same as worker; C4013 gone). fake_match_scan: 0 leads.
- Optional R05 (players.h teleporter_index + regenerated game_engine.c part after R01) and R06 (siblings): apply on top;
  sweep 446 with them: same result.

## R12 layer L + final re-verification (fresh shadow_R from R01..R04)
- L: get_place_string hand PIN -> `long string_index = PIN(entry->place & 0x7F, 0, 15);` EXACT (gps_v1); single-exit
  gps_v2 residual (January keeps two returns).
- Regenerated R01 (A..L) + R05c; R01..R04 `git apply --check` clean individually/together; fresh shadow_R == final/.
- 180/180; audit PASS (identical to production); provider_link PASS; objdiff 3.3.1 equal to production;
  rsweep 446: 445 identical + game_engine raw-reloc spelling only (strict EXACT); /W3 unchanged minus C4013;
  fake scan 0; optional R05/R06 on top: same sweep.
- VERDICT: approve=false for the submitted packet; amended R01..R04 verified; object still blocked by the Open list
  in REVIEW.md section 4 (man_out, motion-sensor copy alias, rule-16 switches, statistic walks).
