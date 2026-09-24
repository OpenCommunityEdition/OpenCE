# REVIEW: source/main/main (worker scratch/w/main, verdict FUZZY_IMPROVED), base 954eebd2

Reviewer slug: `scratch/w/review3_main/` (the full probe log is in LEDGER.md).

## Verdict

- **Land:** patch (1), with the 25-line symbols.json. Zero strict credit.
- **Recommended amendment:** `production_amended.patch`, which is byte-identical to (1).
- **Hold:** patch (2), the main.h swap, for an owner ruling. It is technically verified, but it is exact declared-name-count compensation.
- **Owner-gated:** patch (3), the crash patch.
- **Pin only at admission:** the data draft (5).

## What was re-run independently

| Check | Result |
|---|---|
| Patch applicability | `git apply --check production.patch` on the worktree: clean. In a CRLF/autocrlf temp repo, 1 -> 2 -> 3 applies, and so does 1 -> crash_nohdr. The results equal the worker's candidate files (`candidate_production_main.c` = prod2, `candidate_main.h`, `candidate_full_gated_main.c` = prod_gated_hdr2). |
| gate --all, base.c | 92/2/1 |
| gate --all, p1 (patch 1) | 92/2/1. Every per-function verdict matches base. update_time goes from `[relocs 118!=117, sha]` to `[sha]`. |
| Section compare base.obj vs p1.obj | Of the 196 non-debug sections, only `_main_update_time` and `_main_frame_rate_debug` change. 23 function symbols go from storage 2 to 3. |
| Worker prod.c vs p1.obj | 0 sections differ. The bitmap_2d_new duplicate removal and the attract_mode.h include are byte-inert. |
| alndiff vs January | update_time: 609 -> 114 lines, leaving only the SI/AX phi and the store position. frame_rate_debug: 192 -> 127 lines, leaving only the threaded `mov dl,[902]` and the CLEAR dl/bl. |
| Parks | Re-baseline values reproduced exactly: update_time 1440/117/ad56536c..., frame_rate_debug 544/51/3c132f90.... Target measurements are unchanged under the regenerated split. |
| csplit emulation (worker's 26-line symbols.json, and my 25-line copy) | Only main.obj differs from build/split, across 833 objects. |
| object_audit vs emulated split | p1 + 25 lines: only main_crash and the chucky literal are missing, plus the 2 residual .text. p1+p2 + 26 lines: the same. p1+p2+crash: 169/169 symbols ok, and only the 2 residual .text remain. |
| cachebeta publics | All 26 are absent. main_crash, main_loop and main_print_version are present. |
| HCEX.pdb | `static void main_update_time()` and `static void main_game_render(float,float)`. main_crash is not static. |
| January .text relocations | None to the 14 inlined-only statics. |
| /Od 0x6b1150 main_loop | The 16-call chain is made of DIRECT (non-ILT) calls, so they are statics. The loop exit makes a direct call to 0x6aff90, which is main_exit. That attests `main_exit();`. |
| /Od 0x6b4110 | The RTC names `progress`, so the dropped main_loop local belongs to queue_map_private. |
| /W3 | base 25 = p1 25 (same multiset). There are no new warnings. The worker's "0 warnings" was at the default level. |
| fake_match_scan | 0 leads (p1, p12, p123, p1x) |
| surplus_identity | 2 COMDATs, both IDENTICAL |
| provider_link (p1, p12, p1x) | PASS |
| .data `_global_difficulty_level` | split == p1: 52 bytes / 10 relocations / sha 7f308100, section_infos_equal True. |

## Structural-lane review items, now resolved by January bytes

The structural manifest held these back as "`>=`->`>` change and dropped requested_rate assignment need review".

- **`start > initial+slot`:** January 0x1d7..0x1eb is `cmp hi; jl else; jg then; cmp lo; jbe else`, which is a strict `>`. This fixes a genuine error in base.
- **The dropped `requested_rate = field|30` store:** it was dead in base, because it is overwritten by 999 or 60/best before any read. The direct `?:` reproduces January's 16-bit field load. There is no semantic change.
- **The `" des %d"` argument:** January pushes movsx(si), which is best_interval. Base printed requested_rate, which was wrong.
- **`minimum_target_index`:** a descriptive local name, disclosed. There is no first-party name, because HCEX main_update_time is a different body (it uses s_ClockFrequency).

## Accessor call site

The call site of `main_framerate_throttle_enabled()` is NOT attested. January proves only that the static exists and is inlined. The /Od build has no such function. Placing the call in main_rasterizer_throttle instead, or in both places, also changes 0 sections.

The worker's update_time placement is the natural one, and some call must exist. I accept it.

## Amendment (optional, recommended; verified)

`production_amended.patch` is production.patch plus the main_exit body in /Od 0x6aff90 shape:

```c
switch { server: client; server; break; client: client; break; }
dispose sequence
```

The tree's main_exit duplicates the 4-call dispose tail in the server arm and in a `default:` arm. That spelling is undocumented (36d4c588). Now that main_loop inlines main_exit(), the /Od shape is the better source.

Verification:

- gate 92/2/1, and 0 of 196 sections differ from p1.
- fake scan 0, /W3 25 (unchanged), provider_link PASS.
- It applies to the worktree, and patches 2 and 3 still apply after it.

The full file is `candidate_production_main_amended.c`. `main_exit_od_shape.patch` is the delta on its own.

## Patch 2 (header_swap): HOLD for owner ruling

**Verified.** I ran an independent private-dir sweep. Validation: the original main.h gives objects byte-identical to build/base in all 20 consumers. Results:

- **The swap:** 0 differing sections in all 20 consumers, internal label numbering included.
- **Removing the prototype alone:** `_player_profile_3wide_list_update` goes from EXACT to residual `[sha]` in ui_widget_game_data_input_functions.
- **Adding main_crash alone:** byte-inert.

The main_crash prototype is therefore load-bearing, and only as count compensation. This is the same class as the Layer 2 lock/unlock move, which this lane lists under "Held for owner rulings".

It also declares main_crash, which has no production definition while patch 3 is gated. It is zero credit now, and main.obj is not admissible anyway.

Consumers for the integrator's full sweep, if it is landed later:

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
- source/saved games/game_state.c
- source/saved games/player_profile.c
- source/scenario/scenario.c
- source/shell/shell_xbox.c
- main.c

## Patch 3 (crash): owner-gated

It is exact: 93/2/0, and the 42-byte literal closes. Two constraints apply:

- It is in the owner-gated class of authentic NULL dereferences.
- docs/object_matching_logs/main_obj_jonas_crash_policy_closeout_20260826.md explicitly says not to revisit this leaf through a null cast.

It must not be landed without an owner ruling that supersedes that closeout.

## symbols.json

The worker's copy is correct: 26 in-place CRLF edits, not reserialised.

- **Without patch 2:** use `scratch/w/review3_main/config/symbols.json`. It is the worker's copy with line 4188 (`_main_game_render`) reverted, which leaves 25 lines.
- **With patch 2:** use the worker's 26-line copy.

A csplit regeneration is required either way.

## Worker inaccuracies (none blocking)

- "Makes 25 functions static" names 23 functions. Patch 1 makes 23 static, and 2 were already static.
- "/W3 warnings: 0" is wrong: /W3 gives 25, the same as base.
- c8a/c9b (add before the store) repeat structural v20/v28 (target add before the current store), so they are not new negatives.
- The crash probe used the spelling that the 20260826 closeout forbids. It is correctly labelled owner-gated.

## Remaining object-admission blockers (pre-existing, not introduced)

- Consumer-local `extern scripted_camera_set`.
- `extern struct main_hud_globals_definition *hud_globals`, a caller-local view.
- `float` fields in main_globals.
- 4 C4013 implicit declarations: xbox_demos_launch, player_ui_get_single_player_local_player_controller, input_abstraction_update and cache_files_give_time_to_precache.
- `_main_crash` (owner-gated), and the update_time and frame_rate_debug residuals.
