# REVIEW: source/game/game_engine (worker scratch/w/game_engine_clean, claim OBJECT_COMPLETE_CANDIDATE), tree 954eebd2

**Verdict: approve = false.** The submitted packet is not admissible as submitted. The worker's own claims
check out byte-for-byte. But the object still contains section-8(g) blockers the packet leaves in place,
one of them inside a function the packet edits. I built and fully verified an amended packet, R01-R04,
that fixes all but a few of them. Some construct classes still need a dedicated pass (see "Open") before
game_engine can be marked COMPLETE.

Evidence and tools are in this directory. LEDGER.md has every probe; lab/ has each variant (.c/.obj/.json).

## 1. Worker claims: independently re-verified (all TRUE)

The independent compile harness is `rcompile.py`. It uses the production cflags from build.ninja and runs
with the shadow root as the working directory, so `__FILE__` and the include spellings match production.

**Controls** (both reproduce build/base; only `.debug$S`, the object name, differs):

- the real-tree game_engine.c;
- the untouched shadow copy of game_engine_ctf.

**The packet:**

- **Patches apply.** `git apply --check` is clean for 01, 02, 03 and 04, both individually and together.
  Applying them to a `git archive 954eebd2` export gives files byte-equal to the worker's final/.
- **Functions.** 180/180 EXACT vs build/split. All 302 section-defining symbols equal build/base under
  coff_compare. The symbol-table multiset is identical to production (853/853) once $L and user-label
  numbering is collapsed.
- **Audit, surplus and link.**
  - object_audit PASS; the audit text is identical to production's.
  - surplus_identity: 11 COMDATs, all identical.
  - provider_link on the candidate: PASS.
  - pdb_storage: 0 disagreements.
  - No parks and no admission rejections for this object.
- **objdiff-cli 3.3.1** (odproj/ mini-project): production, the worker packet and the amended packet all score
  27922/32397 code, 173/180 functions and 3792/3792 data. The 7 undercredited functions are pre-existing.
- **HCEX layouts.** Independent DIA2Dump -type dumps are in hcex_types_review.txt. A compile-time lab
  (lab/layout_check.c) runs 34 offset and size checks, and all pass: hudg 0x450, fonts .index 0x54/0x64,
  carnage 0x3D4, netgame flag 0x94, equipment 0x90/0x5C, starting 0xCC/0x48+0x10k, permutation 0x54, itmc 0x5C.
  - Nit: HCEX spells both `flags` members as `long`. Fixed in R02; the change is inert.
- **Signatures.** All of the following agree with their definitions and with HCEX:
  - `game_engine_render_nav_points(short)`;
  - the nav-point prototypes;
  - `rasterizer_draw_unicode_string(bounds, clip, cursor_reference, height_adjust, string)`;
  - `sound_class_set_gain`, `unit_get_local_player_index`, `hud_get_font_index` and `hud_get_text_color`;
  - the two multiplayer-sound prototypes (`intialize` is January's public spelling).
- **`.pad3`** is January's own hs global string ("pad3", rasterizer_debug_options+0x32).
- **handle_custom_starting_equipment.** It matches /Od 0x5b25d0: the for loop, the grant plus break, the
  5-entry inner loop, the double read, and a declaration order equal to the /Od frame order.
- **Full-board sweep.** rsweep.py uses a sentinel-verified comparator over every section, relocation and
  symbol. Over all 446 `source/**.c` TUs: 0 changed.
  - The worker's sweep compiled only 440 TUs, because its dependency tool dropped paths containing spaces.
  - A /W3 comparison over the 446 TUs shows only that the C4013 `console_printf` warning is gone.

**CONSUMERS.txt is incomplete** (documentation defect only; my sweep covers these TUs). The corrected list
is CONSUMERS_review.txt:

- game_engine.h has 37 consumers, including `saved games/playlist_profile`.
- hud_messaging.h has 8, including `saved games/game_state`.
- sound_classes.h has 6, including game_engine itself.
- player_control.h has 21.
- players.h (optional 05) has 71, including `saved games/game_state`.

## 2. Blockers the packet leaves in place

Every fix below was first measured alone in lab/ against the patched file: 180/180 EXACT and 0 changed
sections vs base.

1. **Hand-expanded header helper: game_engine_update_teleporter.**
   - Source: `vector_from_points3d(..., &delta); if (delta.j*delta.j + delta.i*delta.i + delta.k*delta.k > 1.0f)`.
   - /Od 0x5afba0 calls distance_squared3d. /Od 0x42dc80 is distance_squared3d, built from
     vector_from_points3d and magnitude_squared3d.
   - The fix `distance_squared3d(&previous_flag->position, &unit->object.position) > 1.0f` is EXACT (tp_v1).
   - STRIP TEST: plain i,j,k order is residual (tp_v3), so the scrambled order is the lever.
2. **Hand-expanded header helper: find_closest_player_index.**
   - Source: `real x/y/z = ...; distance_squared = y * y + (x * x + z * z)`.
   - /Od 0x59aae0 calls distance_squared3d.
   - The fix `distance_squared3d(&camera_position, &candidate->object.position)` is EXACT (fc_v1).
   - STRIP TEST: without the parentheses, or in plain order, it is residual (fc_v2/v3), so the decoration is the lever.
3. **csplit-relocation-alias trap.** Both render_nav_points (which the packet edits) and
   custom_motion_sensor_positions use `do {...} while ((long)goal < (long)&global_variant);`.
   - They carry the false comment "January uses the adjacent global_variant address".
   - The construct dates from 2026-07-25 (c8a6ed6d, 5d7fd76e), before the no-fake rule.
   - January's `cmp esi, imm; jl` is VC7's strength reduction of a signed `goal_index < 32` loop. csplit merely
     names the one-past address `_global_variant`.
   - /Od 0x5ad780 and /Od 0x5a20e0 are plain `for (goal_index < 0x20)` loops.
   - The natural for loops are EXACT (rn_1, ms_n2). coff_compare resolves `_global_goal+0x400` to the same .bss
     destination, and objdiff 3.3.1 also scores them 100%. No relocs.json change is needed.
4. **Rules 13, 18 and 21 (all EXACT when fixed):**
   - `goto result` clamp becomes `PIN(fade, 0.0f, 1.0f)` (/Od 0x5a4360 ternary clamp).
   - The dead `if (result != NONE)` in did_player_win_default, where result is 0 or 1, is nonsensical logic.
     It becomes /Od 0x59bb90's winning-team local. The whole-function single-exit form is residual, so one
     `goto tied` stays.
   - rasterize_in_game_score's `entry_placement[-6]` pointer walk and hand PIN become an indexed for loop
     plus `PIN(...)`.
   - get_place_string's hand PIN becomes `PIN`.
   - populate_statistic_buffer's comma-expression chains hand-expand the TU's own comparator. They become
     `sort_statistic_buffer_ranking(previous, entry) == 0`.
5. **Invented opaque aggregate.** `struct postgame_statistic_entry { long values[7]; }` becomes HCEX
   `struct statistic_buffer {player_index, score, custom, kills, deaths, assists, place}` (inert).
   - The hand-lowered struct return `entry = *game_engine_get_player_place(&entry, player_index)` becomes
     HCEX's by-value `static struct statistic_buffer game_engine_get_player_place(long)` (EXACT).
   - STRIP TEST: `(unsigned long)0x80000000` in is_place_tied is decoration; the plain form is EXACT.

## 3. Amended packet (patches/R01..R04; full files in final/)

- **R01** = worker 01 plus fixes 1-5 above. It changes source/game/game_engine.c only.
- **R02** = worker 02 plus HCEX `long flags` plus focused-header disclosure comments (the matrix_math.h
  precedent). All inert.
- **R03 and R04** = worker 03 and 04, unchanged. R03 is the SHARED-HEADER patch; the integrator's full
  sweep must cover CONSUMERS_review.txt.
- **Apply check.** `git apply --check` is clean for R01..R04 individually and together against the current
  tree. A fresh `git archive` export plus R01..R04 equals final/ byte-for-byte.
- **Gates.**
  - 180/180 EXACT (gate_amended.txt).
  - object_audit PASS, identical to production.
  - provider_link PASS.
  - Symbol multiset identical to production.
  - objdiff 3.3.1 equal to production.
  - /W3 unchanged apart from C4013 disappearing.
  - fake_match_scan: 0 leads.
- **Full sweep** (446 TUs): 445 are byte-identical. The only difference in game_engine is the raw relocation
  spelling in _game_engine_player_get_custom_motion_sensor_positions and _game_engine_render_nav_points,
  which is `_global_goal+0x400` instead of `_global_variant+0`. Both are strictly EXACT under coff_compare.
  The integrator should confirm report.json after its full ninja.
- **Optional patches.** R05 (players.h `teleporter_index` plus a game_engine.c part regenerated after R01)
  and R06 (sibling netgame-flag headers) apply on top; the 446-TU sweep gives the same result.

## 4. Open: still blocking OBJECT_COMPLETE (not fixed; needs a dedicated source pass)

- **game_engine_man_out.** Two `goto man_out` plus a hand-inlined copy of game_engine_player_is_out_of_lives
  (it re-runs `player_get`). /Od 0x5a0b90 is one `||` chain that calls is_out_of_lives (0x5a26c0). Every plain
  `||` spelling is residual (mo_v1..v3), so the construct is load-bearing and not /Od-attested.
- **Motion-sensor copy.** The kept `goal` pointer alias is load-bearing: direct-index component copies are
  residual (ms_n1). /Od shows no pointer slot and an 8-byte point2d copy. That form, ms_n3, is EXACT but is a
  load-bearing 3D-to-2D view copy, which is owner-gated.
- **Rule 16.** Many bare-integer switch cases remain: statistic 0..4 is HCEX `enum postgame_statistic`, and
  there are the 3/9/10 and 0..3 tables.
- **populate_statistic_buffer / select_players_to_display.**
  - `entry = entries; do {...entry++}` walks.
  - the `if (count >= MAX) { match_assert(count < MAX) } else {store}` shape.
  - HCEX gives select_players_to_display a different parameter order `(player_index, out, statistic, max)`.
    January's ABI decides; not asserted.
- **Kept, with evidence.**
  - team_has_players `goto done`: /Od's double-jmp shows a jump statement and an else, and the if/else form
    is residual.
  - The owner-landed `BUG (preserved for exact matching)` vestigial CTF team argument.
  - handle_custom_starting_equipment's 5-of-6 loop, disclosed in a comment.
- **Advisory.** HCEX names the teleporter message timer as the static local `blocked_message_delay`. The TU
  uses the global `game_engine_teleport_message_ticks`.

## 5. Environment incident

At about 00:42 the worktree's `.git` pointer file disappeared. My first `git archive` failed with "not a git
repository", so I did not cause it. It was back at 00:57. The tracked tree was verified clean before and
after the incident; every git read used `--git-dir=<admin>`.

`git apply` for shadow trees must run with `GIT_CEILING_DIRECTORIES`: inside the repo it silently ignores
out-of-cwd paths. One such no-op attempt happened here and changed nothing (`git status` clean).
