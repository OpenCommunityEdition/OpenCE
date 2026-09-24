# source/game/game_engine

## verdict
FUNCTIONS_EXACT_OBJECT_BLOCKED

## newly_exact_functions
null

## candidate_files
null

## production_changes
APPLY NOW; these fixes do not depend on the provider blocker and are verified.
(1) Source: apply C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\actor_firing_position_audit\game_engine_cand3.diff to source/game/game_engine.c. The full file is ge_cand3.c in the same folder.
  - Add `static` to the prototype and definition lines of 12 functions: game_engine_build_lighting, game_engine_infinite_grenades_internal, game_engine_post_rasterize_in_game, game_engine_rasterize_in_game_score, game_engine_predict_resources, game_engine_update_purge, game_engine_update_weapons, game_engine_update_item_spawn, find_closest_player_callback, find_closest_player_index, game_engine_get_type, game_engine_press_start_to_begin.
  - Remove the hand-written copies of the game_engine_get_type body (`long game_type = NONE; if (game_engine) game_type = game_engine->type;`) and pass `game_engine_get_type()` directly as the match_game_type argument, in three places: game_engine_get_starting_location_rating, game_engine_update_item_spawn and handle_custom_starting_equipment. The now-unused game_type locals are removed.
  - In game_engine_nonplayer_post_rasterize, add `game_engine_press_start_to_begin();` before `break;` in `case 0: case 1:`.
(2) config/symbols.json: append `, "static": true` to _find_closest_player_callback (617600), _game_engine_press_start_to_begin (619872), _find_closest_player_index (632448), _game_engine_update_item_spawn (635808). These are in symbols_json_all.diff.
(3) Regenerate the split, then run a full ninja.
Do NOT set Matching until _distance3d is repaired.

## evidence
Before (production): object_audit FAIL on 8 functions that the split marks static but our source defined as external. pdb_storage found 4 more functions (find_closest_player_callback/_index, press_start_to_begin, update_item_spawn) that are external in both the split and our build but absent from cachebeta publics.

Census (pub_census.py): unreferenced externals DO appear as publics (for example _static_camera_new and _collision_test_pill_new). So a function missing from the publics is file-static, even when no relocation names it.

That raised a problem for _game_engine_get_type and _game_engine_press_start_to_begin. January emits both as statics with no relocation anywhere, which means they were referenced by calls that were inlined away. Our source had no caller for either, so making them static would drop them; this was measured in G2 (press_start becomes UNWRITTEN).

Where the callers came from:
- get_type: in the /Od build, fn 0x59df80 is called from 0x59dbc0 (starting_location_rating, passing the result straight into match_game_type), 0x5aefc0 (update_item_spawn) and 0x5b25d0 (handle_custom_starting_equipment). The HCEA sources game_engine_update_item_spawn.c and handle_custom_starting_equipment.c make the same call.
- press_start_to_begin: /Od fn 0x5a1600 is nonplayer_post_rasterize. Its jump table at 0x5a172c sends cases 0 and 1 to `call 0x5a6510`, an empty stub whose only caller is that site. HCEA says press_start_to_begin is an empty function.
- Names are attested by HCEX.pdb (find_closest_player_index/_callback, update_item_spawn, get_type) and by HCEA (press_start_to_begin).

After:
- ge_cand3 gate 180/180 EXACT, including get_type (32), press_start (16), nonplayer_post_rasterize (160), update_item_spawn (336), starting_location_rating (96) and handle_custom (304).
- Scratch-split regen: all 263 sections identical; only 4 storage rows changed.
- object_audit: PASS, 269/269 symbols.
- pdb_storage2: 0 disagreements.
- provider_link: the 8 'NO JANUARY PROVIDER' rows are gone; only _distance3d fails.
The tools/audit_object_admission.py base run already lists this unit with function_gap 0 (build/report.json's objdiff under-credit is covered by the semantic report), so no semantic_matches entry is needed.

## blockers
The surplus _distance3d COMDAT from real_math.h differs from January's selected NODUP hand copy in source/ai/action_vehicle.obj. It fails to link in both orders (LNK2005: '_distance3d already defined'). January's game_engine has no call relocation to _distance3d.

## reopen_criteria
Reopen after the board-wide action_vehicle distance3d provider repair. Then rerun provider_link on ge_cand3.obj; it should pass in both orders.

## task notes
No tracked files were edited, and no ninja, configure or git-mutating command was run.

Law confirmed on this board: cachebeta.pdb publics include externals that nothing references. So a missing public means file-static, and a static that January emits with no relocation was called in source through inlined calls. When a hand-written copy of a static's body sits where January called the static, you must restore the call. That applies to game_engine_get_type and press_start_to_begin, whose call sites are confirmed by the /Od build and HCEA.

The alignment rule for loose statics versus one aggregate also holds for .rdata: splitting real_math_constants into the four HCEX statics turned 8-byte alignment into January's 4-byte.

Stale comment: game_engine_get_starting_location_rating has a comment saying 'NonMatching with the same EBX/EDI mirror', but the function is EXACT. I left it; the integrator may drop it.

All changes are local to the .c files, and the symbols.json edits change only these 4 split objects; the other 829 splits are byte-identical. So no blast radius is expected, but the integrator must still regenerate the split and do a full sweep.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\actor_firing_position_audit\LEDGER.md

Evidence and patch files, all in the same folder:
- Candidate sources: afp_cand1.c, ge_cand3.c, rm_cand2.c
- Source patches: actor_firing_position_cand1.diff, game_engine_cand3.diff, real_math_cand2.diff
- symbols.json patch: symbols_json_all.diff (27 lines), plus the scratch config/ and regenerated split/
- Results: final_gate.txt, final_verification.txt, hcex_real_math_compiland.txt, admission_audit_base.json
- Tools: xrefs_all.py, pub_census.py, object_audit_split.py, pdb_storage2.py, split_diff.py, od_empty_stubs.py, od_small_refs.py
