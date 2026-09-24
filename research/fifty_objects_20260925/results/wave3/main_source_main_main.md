# source/main/main

## verdict
FUZZY_IMPROVED

## newly_exact_functions
[
 "_main_crash (OWNER-GATED: exact only in owner_gated_crash*.patch; not landable)"
]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\main\\production.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\main\\header_swap.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\main\\owner_gated_crash.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\main\\owner_gated_crash_nohdr.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\main\\candidate_production_main.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\main\\candidate_full_gated_main.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\main\\candidate_main.h",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\main\\config\\symbols.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\main\\semantic_data_entry_draft.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\main\\LEDGER.md",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\main\\sweep\\sweep.py",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\main\\sweep\\gatecmp.py",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\main\\sweep\\mainc.py",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\main\\sweep\\consumers_all.txt"
]

## production_changes
All files are under C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\main\. The patches use LF line endings, `git apply --check` is clean against 954eebd2, and the apply sequences were verified in a CRLF/autocrlf temp repo.

(1) production.patch: source/main/main.c only. Landable at zero strict credit; the full result is candidate_production_main.c.
- Makes 25 functions static. They are not cachebeta publics, so they were file-static in January; HCEX.pdb independently shows `static void main_update_time()`. The functions: create_local_players, main_change_map_name, main_exit, main_framerate_throttle_enabled, main_initialize_time, main_load_core_private, main_lost_map_private, main_new_map, main_queue_map_private, main_reset_time, main_respawn_private, main_revert_map_private, main_run_demos_private, main_save_core_private, main_save_map_private, main_saving_map_private, main_setup_connection, main_skip_cinematic_private, main_skip_private, main_switch_to_structure_bsp_private, main_update_time, main_won_map_private, sort_controllers_ascending. main_frame_rate_debug and main_reset_map_private are already static.
- Makes main_loop call the private helpers again instead of hand-expanding their bodies (rule 13). VC7 inlines the calls byte-identically and still emits the static copies; without the calls, 12 static functions vanish. It also calls main_exit() at the end and drops the now-unused precache_progress local.
- main_update_time: `framerate_throttle = main_framerate_throttle_enabled();`. January emission order is reset_time, throttle_enabled, update_time.
- Structural-lane fuzzy bodies (evidence in claude-structural-20260923 REPORT.md): update_time v25 and frame_rate_debug f2 (`sample_index++;` as its own statement).
- Drops the duplicate `extern bitmap_2d_new` prototype, which bitmaps_internal.h already declares.
- Replaces the local `extern game_end_credits_start` with `#include "interface/attract_mode.h"`. Both cleanups are byte-inert in every section.
- gate --all: exact 92, residual 2, unwritten 1. The EXACT rows are identical to base, and no row is lost.
- Parks to RE-BASELINE (not retire):
  - _main_update_time base becomes 1440/117/ad56536c5eb119b981a9d0b89feb8bc57be3d72a21b4fcdfac4b4939db97ccdc (was 118 relocs).
  - _main_frame_rate_debug base becomes 544/51/3c132f904203944a7f5f7c3b746a7458964f073c1fc4374aba0988bb5f3db4ff.
  - Re-measure objdiff_percent after ninja.

(2) header_swap.patch: SEPARATE shared-header patch; apply after (1). It changes source/main/main.h and adds `static` to main_game_render in main.c.
- Removes the `void main_game_render(double)` prototype from main.h: it is not public and only main_loop calls it.
- Adds the genuine public `void main_crash(char const *reason);` before main_print_version.
- The swap is count-neutral by construction (disclosed). Each half has independent evidence.
- Removing the prototype alone regresses ui_widget_game_data_input_functions `_player_profile_3wide_list_update` (1232 B), a declared-name-count tie.
- Private-dir sweep of all 20 consumers (shadow header verified): the swap is byte-identical in every section, 833/849 function verdicts unchanged.
- Consumers for the integrator's full sweep: source/bink/bink_playback.c, source/game/game.c, source/game/game_engine.c, source/game/player_control.c, source/game/player_queues_new.c, source/game/players.c, source/hs/hs_globals_external.c, source/interface/interface.c, source/interface/player_ui.c, source/interface/ui_widget.c, source/interface/ui_widget_game_data_input_functions.c, source/main/d3d_intimacy.cpp, source/networking/network_client_manager.c, source/networking/network_game_globals.c, source/networking/network_game_manager.c, source/networking/network_server_manager.c, 'source/saved games/game_state.c', 'source/saved games/player_profile.c', source/scenario/scenario.c, source/shell/shell_xbox.c, plus main.c itself (92/95 with the swap).
- Full header: candidate_main.h.

(3) OWNER-GATED, NOT LANDABLE: owner_gated_crash.patch (after 1+2) or owner_gated_crash_nohdr.patch (after 1 only) adds _main_crash. The full gated file is candidate_full_gated_main.c.

(4) config/symbols.json: edited copy at scratch/w/main/config/symbols.json with lines edited in place. It adds `, "static": true` to 26 lines: 4107, 4108, 4128, 4143-4162, 4182, 4183, 4188. Line 4188 (_main_game_render) goes only with patch (2). A split regen is needed for object_audit to agree.

(5) Data entry draft for later: semantic_data_entry_draft.json. Pin it only when the whole object is admitted.

## evidence
_main_crash:
- January: 11 B, `mov dword ptr [0], offset literal; ret`, one DIR32 at +6.
- /Od 0x6aff50 has the same single store with no frame slot, so there is no named pointer local.
- The /Od caller 0x5e5380 passes one dword argument (hs `main_crash(char const *reason)`).
- Candidate source is `*(char const **)NULL = "chucky was here!  NULL belongs to me!!!!!"; return;`. It is EXACT on the first compile (sha abe94492...), 0 EXACT rows moved, and it closes the 42-B literal.

Storage:
- pdb_storage flagged 26 main.obj functions that are absent from the cachebeta publics.
- /Od main_loop 0x6b1150 attests a chain of 16 `if (flag) xxx_private();` calls.
- January has no in-object relocations to 12 of the non-public functions, yet emits all of them. That proves they were static and inlined from calls: static plus unreferenced drops them (measured in c2).
- Final audit of the gated candidate (prod_gated_hdr2 + swap main.h):
  - gate: 93/2/0.
  - object_audit: only the 2 residual .text rows and the 26 storage rows that symbols.json fixes.
  - Storage vs cachebeta: 0 disagreements.
  - Surplus: 42 sections, unchanged. surplus_identity: _fast_ftol and _arctangent IDENTICAL. provider_link: PASS.
  - fake_match_scan: 0 leads; /W3 warnings: 0.

Data gap census (1,848 B = .rdata 1,796 + .data 52):
- .rdata is 97.51%, with no credit because data credit is per whole section. The only missing January-owned section is the 42-B chucky literal owned by _main_crash; with the crash patch every .rdata section audits ok.
- .data `_global_difficulty_level` (52 B) is resolved-equal under coff_compare: 10 DIR32 relocations to the level-path literals, sha 7f308100... objdiff 3.3.1 reads 22.77% because of its '$'-literal defect.
- .bss: 100%.

New negatives recorded in LEDGER.md, none repeating earlier recorded shapes:
- update_time:
  - c5: best_interval merged into selected_interval on the v25 base.
  - c6a/b/c: the copy moved to after the loop gives 1456 B.
  - c7a: an else-arm edge copy is coalesced away, identical output.
  - c8a/c9b: add-before-store puts the phi in ECX, 1456 B.
  - c9a: store as an assignment expression, identical output.
  - Name-count oracle n=1..30: all identical.
- frame_rate_debug:
  - Name-count oracle n=1..40: all identical.
  - Standalone threading labs: extern, static, f1 arm shape, long index and small body all fail to thread; a lab goto gives a different layout.
  - Reading: January is our CFG plus one post-layout jump-thread.

## blockers
The whole object is still blocked:
- _main_crash is owner-gated (brief s7, authentic NULL-write class; crash_policy_closeout_20260826). It needs an explicit owner ruling.
- _main_update_time: the only residual is the join phi. January keeps it in SI and ours in AX, and January's store to vblank_interval_current sits between the int64 conversion and the add. That is an IL-order fact no tested C statement order reproduces. About 36 shapes have been tried across lanes; the stop rule fired.
- _main_frame_rate_debug: January jump-threads the (reset && debug) edge past the global re-test. That gives a second sample-index load, costing 8 B and 1 relocation. January also stores framerate_sample_index from DL where ours uses the pinned BL. Stop rule fired.
- Further admission debt in main.c, left untouched and needing header/owner work (brief s8 g):
  - `struct main_hud_globals_definition { byte reserved00[0x54]; long font_tag_index; }` with a consumer-local `extern ... *hud_globals` view: no header defines the genuine `struct hud_global_data`.
  - A consumer-local `extern void scripted_camera_set(...)`: camera_scripting.h does not declare it.
  - A TU-local `struct game_options` definition, duplicated in game.c and network_game_manager.c.
- The .data section needs the '$'-defect semantic entry at admission.
- The .rdata section needs the crash literal.

## reopen_criteria
- _main_crash: an owner ruling that admits the authentic intentional NULL write. The patch is ready: 16/16 B, 1/1 relocations, sha abe94492.
- _main_update_time: a VC7 IL-construction mechanism that emits the vblank_interval_current store between the int64 conversion of selected_interval and the add, or that gives the join phi best_interval's register (SI). Do not retry c5-c9, v1-v28, or the name-count sweep.
- _main_frame_rate_debug: a VC7 13.00.9254 trigger for jump-threading a re-tested GLOBAL boolean. The same trigger probably also explains the missing constant propagation into the CLEAR store (DL vs BL). Arm order, storage class, index type, body size and goto have all been measured and do not trigger it.
- Object admission also needs:
  - patches 1+2 plus the symbols.json regen;
  - the '$' data entry;
  - a genuine hud_global_data type/header;
  - a camera_scripting.h prototype for scripted_camera_set;
  - a single-owner struct game_options.

## task notes
No tracked file was edited, and no ninja, configure or git-mutating command was run in the worktree. Git ran only in a throwaway temp repo under the slug, which has been deleted.

The header sweep used a private per-consumer directory. Nothing was written to the shared scratch/ root, so parallel workers were not disturbed.

The oracle/*.c files contain dummy declarations. They are lab-only and must never land.

Integrator order:
1. production.patch, then the symbols.json lines except 4188.
2. Optionally header_swap.patch plus line 4188. This one needs the full ninja sweep across the listed consumers.
3. Re-baseline the 2 parks.

Crash patches and the data entry wait for owner rulings and object admission.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\main\LEDGER.md
