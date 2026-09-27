# Remaining-frontier lane: integration handoff (2026-09-26)

Lane `claude/remaining-frontier-20260926`, worktree `C:\halo-worktrees\claude-remaining-frontier-20260926`, base
canonical `8cda1f91f3a037f52ac2eed551936b03df6796c1`. Not pushed; canonical untouched. Toolchain VC7 13.00.9254,
objdiff-cli 3.3.1; no compiler flag, objdiff, symbols.json, parks, semantic ledger or object-status change.
Everything below is **lane-verified, pending independent canonical reconciliation**.

## Commits
| Commit | Kind | Content |
|---|---|---|
| b62f74c1 | research | CENSUS.md, TRIAGE.md (44 unparked functions: 25 HELD, 16 NEGATIVE, 3 OPEN), WORKER_BRIEF.md, unparked_all.json, data_gaps.json |
| 9ed00bf7 | source | `_actor_path_refresh` (source/ai/actor_moving.c): /Od `build_path` flag; EXACT 1,440 B |
| 9f0b3af9 | source | `_display_scenario_help` (source/interface/ui_widget.c): /Od message-only assert condition; EXACT 608 B |
| 7d1b038e | research | worker reports RF-A..RF-G, R0/R1 checkpoint summaries, lead /Od readout and gate log |
| 1b00eeea | research | RF-H report, this handoff (first version) |
| b659a777 | source | `_widget_instance_render_recursive` (source/interface/ui_widget.c): /Od helper shape, clip and colour shapes, RTC names; EXACT 752 B |
| 262dcfa6 | research | wave-2 reports (RF-B/C/E/J/K/L), checkpoint R2 |
| 06645ef4 | source + park | `_player_effect_update_camera_impulse` (source/effects/player_effects.c): Bungie's `realcmp` macro; EXACT 752 B; its park retired in config/parked.json |
| 51e41e98 | research | wave-3/4 reports (RF-B/J/M/N/O/P/Q/R/S), WAVE4_BRIEF.md, checkpoint R3 |
| (this) | research | OWNER_PACKET.md, handoff update |

Upstream status: bnunu/halo-1 `jonas/exact-pilots` (cdf1c42b; `main` 52659b62 differs only in README.md) already
contains independent reconciliations of 9ed00bf7 (as 9c9118d1) and 9f0b3af9 (as 81a11ff9); its source/include/config/
tools trees equal this lane's 9f0b3af9. Integration of the rest: cherry-pick b659a777 and 06645ef4 (independent files;
06645ef4 carries its own park retirement); the research commits are optional records. Each source commit touches only
its own function (keyed diffs: 1 changed, 0 added, 0 removed).

## Board (full clean-build checkpoints, scratch/rf/checkpoint.py; summaries in checkpoints/)
| | R0 (8cda1f91) | R1 (+2 landings) | R2 (+render_recursive) | R3 (+camera_impulse) |
|---|---:|---:|---:|---:|
| Halo code | 1,591,710 | 1,593,739 | 1,594,482 | 1,595,220 |
| Functions | 7,461 / 7,574 | 7,463 | 7,464 | 7,465 |
| Objects | 389 / 468 | 389 | 389 | 389 |
| Data | 2,588,903 / 3,923,451 | = | = | = |
| Strict section owners | 7,633 / 8,252 | 7,635 | 7,636 | 7,637 |
| Stable diff vs previous | - | +2 (2,048 B), 0 regr. | +1 (752 B), 0 regr. | +1 (752 B), 0 regr. |
| Parks | 75 / 0 / 0 | 75 / 0 / 0 | 75 / 0 / 0 | 74 / 0 / 0 |
| Admission | 11 / 0 / 1 / 0 | = | = | = |
| pytest (tools) | 1161 passed, 5 skipped | = | = | = |
| Warnings | 199 | 199 (0 new) | 199 (0 new) | 199 (0 new) |
| fake_match_scan leads | 26 | 26 | 26 | 26 |
No landing completes an object (actor_moving keeps 3 residual functions, ui_widget 4, player_effects 2). The first R3
run hit a transient C1001 internal compiler error in libs/d3d8 lazy.cpp/memory.cpp (pinned compiler hashes verified;
both recompiled cleanly); the full rerun passed (checkpoints/R3_camera_impulse_ICE_run1/).

## Two new levers (both closed functions previously recorded NEGATIVE)
1. **Control-flow flag** (RF-A): /Od stores a byte flag TRUE before a call, overwrites it with an `&&` temporary on one
   branch, then tests it; our folded `if (!a() || (b && c))` cannot store TRUE before the call. The flag decides which
   return block owns the label (LAW B), rebinding three jumps (+12 B, zero instruction differences).
2. **Message-only assert condition** (RF-E): `match_vassert` passes the message, not `#expr`, so January's .rdata
   cannot pin the condition; /Od's `text_box && text_box->type == _ui_widget_type_text_box` changed a register
   priority tie.
Caveat (RF-G): /RTCu shadow flags (0 at entry, 1 per assignment, tested before `_RTC_UninitUse` 0x92db80) and byte
temps holding bool call results are not source flags. Censuses: RF-G (44 unparked) found one further lead that cannot
yield credit; RF-H (71 non-asm parked) found none.

## Worker outcomes
| Worker | Target | Verdict |
|---|---|---|
| RF-A | _actor_path_refresh | EXACT, landed |
| RF-B | _decal_new_from_collision | improved, not exact (C05 research patch); frame slot order needs >= 9 / >= 6 IL refs on two locals with January's machine refs |
| RF-C | _physics_update_old | improved, not exact (v6 research patch); new /Od fact A1 (`add_vectors3d` for magic_torque) shifts x87 temps by the predicted +2 |
| RF-D | _real_random_range_evaluate | NEGATIVE; forced-key oracle proves one hidden record decides it (zero credit) |
| RF-E | _display_scenario_help / _ui_check_for_pause_game / _draw_bitmap_in_rect | EXACT, landed / NEGATIVE (one allocator tie; Sept retail witness) / NEGATIVE (Oct retail witness shows our source differs) |
| RF-F | _sphere_intersects_cluster_portal | NEGATIVE, no compiles; /Od supports the current point_from_line3d body |
| RF-G | /Od census, 44 unparked | vector_avoidance timer-hoist lead only (function stays residual: held spilled-`t` class) |
| RF-H | /Od census, parked pool | no HIGH/MED lead; two new /RTCu attestations for held items |
Reopen criteria for every NEGATIVE are in the workers' REPORT.md files.

## Owner questions (nothing below is assumed)
1. `_actor_path_refresh` style: P02 (landed: flag only) or P03 (adds the byte-inert /Od
   `if (!path_available) success = FALSE; else` nesting; patch `workers/RF-A/actor_path_refresh_P03.patch`).
   The flag name `build_path` is descriptive (house rule 15).
2. `_display_scenario_help`: the landed assert conjunct is redundant with the search loop's exit test (the /Od build
   has it; precedent `_widget_instance_text_box_is_focused`). Keep B alone (landed) or take the A+B form with the
   /Od empty-body loop header (`workers/RF-E/display_scenario_help_AB_alternative.patch`, same bytes)?
3. `_real_random_range_evaluate`: RF-D's H-PAREN-ARG (parenthesising only `arguments[1].real_value`) is a
   decorative-parenthesis form; not run. Authorise a one-compile held-form lab, or keep it closed?
4. New first-party attestations for held uninitialised reads (information for existing packets):
   `_effect_allowed_by_environment` `allowed` (/Od 0x56e50b / 0x56e5b0) and `_ai_test_line_of_sight` `collision_t`
   (/Od 0x48aee6 / 0x48b4d6 / 0x48b50c) are declared without initialisers in the /Od build.
5. Information: `_player_teleport_internal`'s production body (1,312 B) is older than the measured best (fifty-objects
   t1/o1, 1,280 B, exact except three allocator decisions); /Od's message-only assert sits at function scope with the
   full condition.

## Waves 2-4 (after the first handoff)
Two more closures (render_recursive: inline-helper /Od shape; camera_impulse: hand-expanded existing macro) and a
complete near-miss census (workers/RF-N) of all 110 measurable residuals with decoded, oracle-proved deciding facts
for most single-mechanism functions (RF-O/P/Q/R). Every remaining admissible route needs either an owner ruling or a
first-party fact no instrument has found: see OWNER_PACKET.md (sections A-C) for the consolidated decisions with
measured gains and losses.

## What remains (goal: all functions, objects and data)
- Functions: 109 unmatched Halo functions after R3 (68 parked; 41 unparked). Every unparked one is HELD (awaiting a ruling or
  needing a held form) or NEGATIVE with a recorded reopen criterion; this lane's instruments are spent on them.
- Data: 1,334,548 B in 7 units, all HELD or BLOCKED (CENSUS.md table; hs 54,780 awaits the Q11 verifier ruling).
- Objects: 79 incomplete; the 11 admission candidates are all held (CENSUS.md).
Further credit needs owner rulings on the held classes (see OWNER_PACKET / HANDOFF files of the compiler-application
and Q11 lanes and the questions above) or new first-party evidence meeting a recorded reopen criterion.

## Hygiene
Worker scratch (sources, objects, traces, tools) stays under `scratch/rf/workers/` (untracked, not committed; no
binaries in git). Evidence copies in this directory are byte-for-byte (`.gitattributes -text -whitespace`). All
workers stopped; no background processes left.
