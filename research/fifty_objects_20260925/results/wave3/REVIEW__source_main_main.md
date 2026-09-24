# REVIEW source/main/main

approve: True

## per_object
[
 {
  "unit": "source/main/main: production.patch (preferred: review3_main/production_amended.patch) + review3_main/config/symbols.json (25 lines) + 2 park re-baselines",
  "approve": true,
  "issues": "Verified with gate 92/2/1: identical verdicts, and only the 2 residual sections change. Storage matches cachebeta and HCEX. The main_loop private calls and the main_exit call are /Od-attested. January's bytes attest the update_time semantic fixes. Park values are reproduced. The main_exit /Od-shape amendment is byte-identical and recommended. The accessor call site is a byte-undetermined but natural choice."
 },
 {
  "unit": "source/main/main.h: header_swap.patch + symbols.json line 4188 (_main_game_render static)",
  "approve": false,
  "issues": "Byte-identical in all 20 consumers and in main.c. Hold for an owner ruling, because it is exact declared-name-count compensation: removing the prototype alone regresses _player_profile_3wide_list_update, and adding main_crash alone is inert. This is the Layer 2 class. It would also declare a function with no production definition. Zero credit, no urgency. The full-sweep consumer list is in REVIEW.md."
 },
 {
  "unit": "source/main/main: owner_gated_crash.patch / owner_gated_crash_nohdr.patch (_main_crash)",
  "approve": false,
  "issues": "Exact (93/2/0, and the 42-byte literal closes), but it belongs to the owner-gated authentic-NULL-write class. The 20260826 closeout forbids the null-cast spelling. Not landable without an owner ruling."
 },
 {
  "unit": "source/main/main: semantic_data_entry_draft.json (_global_difficulty_level)",
  "approve": false,
  "issues": "Measurements verified: 52 bytes / 10 relocations / 7f308100, section_infos_equal True. Pin only at whole-object admission, as the worker states."
 }
]

## checks
Every check was re-run independently at 954eebd2. The full probe log is scratch/w/review3_main/LEDGER.md and the summary is REVIEW.md in the same slug.

1. Patch application:
   - git apply --check production.patch on the worktree: clean.
   - In a CRLF/autocrlf temp repo, these sequences apply: 1 -> 2 -> 3, and 1 -> crash_nohdr. Each result byte-equals the worker's candidate files (prod2, candidate_main.h, prod_gated_hdr2).

2. gate --all:
   - base.c: 92/2/1.
   - p1 (patch 1): 92/2/1, with the same per-function verdicts as base. update_time changes from [relocs 118!=117, sha] to [sha].
   - p1 + crash_nohdr: 93/2/0.
   - p1 + 2 + 3 with the swap header, gate-equivalent against an emulated split: 93/2/0.

3. Section-level comparison (bytes and relocations by name, 196 non-debug sections):
   - base vs p1: only _main_update_time and _main_frame_rate_debug change, and 23 symbols go from storage 2 to 3.
   - The worker's prod.c vs p1: 0 differ. The bitmap_2d_new cleanup and the attract_mode.h include are byte-inert.

4. alndiff against January:
   - update_time: 609 -> 114 lines. Only the SI/AX phi and the store position remain.
   - frame_rate_debug: 192 -> 127 lines. Only the jump-thread and the dl/bl CLEAR store remain.
   - I checked in January's bytes that the new strict `>` (0x1d7-0x1eb) and the best_interval argument to " des %d" (0x3b6-0x3c5) are what January does.

5. Park re-baseline values reproduced exactly: 1440/117/ad56536c... and 544/51/3c132f90....

6. Split emulation. I ran csplit.exe with the worker's 26-line symbols.json and with my 25-line copy. In both, only main.obj changes across 833 objects, and all 95 target function sections are unchanged.
   - object_audit, p1 + 25 lines: only main_crash and its literal are missing, plus the 2 residual .text sections.
   - object_audit, p1+p2 + 26 lines: the same.
   - object_audit, p1+p2+p3: 169/169 symbols OK.

7. Storage evidence:
   - cachebeta publics: all 26 functions are absent. main_crash, main_loop and main_print_version are present.
   - HCEX.pdb: main_update_time is static, main_game_render is static (float, float), main_crash is external.
   - January .text has no relocations to the 14 statics that exist only as inlined copies.

8. /Od evidence:
   - main_loop 0x6b1150: the 16-call chain uses direct calls (not import thunks), so the callees are statics.
   - The loop exit calls 0x6aff90, which is main_exit.
   - The RTC data at 0x6b4110 names `progress`.
   - /Od has no main_framerate_throttle_enabled. Moving the call to main_rasterizer_throttle, or calling it in both places, changes 0 sections, so its call site is not determined.

9. /W3: base 25 warnings, p1 25 warnings (the same set), so no new warnings.

10. fake_match_scan on p1/p12/p123/p1x: 0 leads. surplus_identity: 2 COMDATs, both identical. provider_link on p1/p12/p1x: PASS.

11. .data _global_difficulty_level: section_infos_equal is True (52 bytes / 10 relocations / 7f308100).

12. Independent main.h sweep in private directories, with the source directory first on /I. The pipeline reproduces build/base byte-for-byte in all 20 consumers.
   - Swap header: 0 differing sections.
   - Removing the prototype only: _player_profile_3wide_list_update regresses from EXACT to [sha].
   - Adding the main_crash prototype only: byte-inert.

13. Amendment: I wrote main_exit in the /Od 0x6aff90 shape. It passes gate 92/2/1 with 0 of 196 sections different, fake scan 0, provider_link PASS. It applies to the tree, and patches 2 and 3 still apply after it.

## issues
Approve lands patch 1 only, plus the 25-line symbols.json and the two park re-baselines. Patch 2 is held for an owner ruling and patch 3 stays owner-gated.

LAND (zero strict credit, fuzzy improvement):
- Apply C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\main\production.patch.
- Better, apply my verified amendment C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review3_main\production_amended.patch instead. It is production.patch plus main_exit in the /Od 0x6aff90 shape: the switch on connection, then one shared dispose sequence. That replaces the undocumented body that duplicates the dispose tail in the server arm and in a `default:` arm. It is byte-identical in all 196 sections. The full file is scratch\w\review3_main\candidate_production_main_amended.c. The delta alone is main_exit_od_shape.patch.
- Use symbols.json from scratch\w\review3_main\config\symbols.json: the worker's copy with line 4188 (_main_game_render) reverted, so 25 in-place CRLF edits. Regenerate the split.
- Re-baseline the parks:
  - _main_update_time -> 1440/117/ad56536c5eb119b981a9d0b89feb8bc57be3d72a21b4fcdfac4b4939db97ccdc
  - _main_frame_rate_debug -> 544/51/3c132f904203944a7f5f7c3b746a7458964f073c1fc4374aba0988bb5f3db4ff
  - Re-measure objdiff_percent after ninja.
- The structural lane held these bodies back over two review items. January's bytes resolve both:
  - `>` is strict in January.
  - The dropped requested_rate store was dead.
  - The " des %d" argument is best_interval in January, so base printed the wrong value.

HOLD, owner ruling (technically verified): header_swap.patch together with symbols.json line 4188.
- It is exact declared-name-count compensation. Removing the main_game_render prototype alone regresses _player_profile_3wide_list_update. Adding main_crash alone is byte-inert. So the main_crash prototype is load-bearing only as count compensation, the same class as the Layer 2 prototype move listed under "Held for owner rulings".
- It would also declare main_crash, which has no production definition.
- It is zero credit and main.obj is not admissible anyway.
- If it is landed later, the integrator's full sweep must cover:
  - source/bink/bink_playback.c
  - source/game/game.c
  - source/game/game_engine.c
  - source/game/player_control.c
  - source/game/player_queues_new.c
  - source/game/players.c
  - source/hs/hs_globals_external.c
  - source/interface/interface.c
  - source/interface/player_ui.c
  - source/interface/ui_widget.c
  - source/interface/ui_widget_game_data_input_functions.c
  - source/main/d3d_intimacy.cpp
  - source/networking/network_client_manager.c
  - source/networking/network_game_globals.c
  - source/networking/network_game_manager.c
  - source/networking/network_server_manager.c
  - 'source/saved games/game_state.c'
  - 'source/saved games/player_profile.c'
  - source/scenario/scenario.c
  - source/shell/shell_xbox.c
  - source/main/main.c

OWNER-GATED: owner_gated_crash.patch and owner_gated_crash_nohdr.patch.
- The main_crash they add is exact, but it is an authentic NULL write.
- main_obj_jonas_crash_policy_closeout_20260826 explicitly forbids reopening this leaf through a null cast. It needs an owner ruling that supersedes that closeout.

Data draft: semantic_data_entry_draft.json. Its measurements are verified; pin it only at whole-object admission.

Caveat: the call site of main_framerate_throttle_enabled() is a byte-undetermined choice. January proves only that a static exists and is inlined; update_time is the natural site.

Worker inaccuracies (none blocking):
- "25 functions static" lists 23. Patch 1 makes 23 static, and 2 were already static.
- "/W3 warnings: 0" is wrong: /W3 gives 25, the same as base.
- c8a/c9b repeat structural v20/v28 (add before the store).
- The crash probe used the forbidden null-cast spelling, although it is correctly labelled owner-gated.

Pre-existing admission blockers remain, none introduced here:
- extern scripted_camera_set
- the hud_globals caller-local struct view
- float fields in main_globals
- 4 C4013 implicit declarations
- the crash function and the two residual functions
