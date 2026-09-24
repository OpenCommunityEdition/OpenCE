# source/models/model_animations

## verdict
FUNCTIONS_EXACT_OBJECT_BLOCKED

## newly_exact_functions
null

## candidate_files
null

## production_changes
APPLY NOW; this fix is verified and needs no source change.
config/symbols.json: edit lines in place and append `, "static": true` to _animation_is_compressed (1113712), _animation_keyframe_search (1115488), _animation_get_keyframe_rotation (1117056), _animation_get_keyframe_translation (1117840), _animation_get_keyframe_scale (1118608). These are in scratch/w/actor_firing_position_audit/symbols_json_all.diff.
Then regenerate the split. Do NOT set Matching until _distance3d is repaired.

## evidence
Before: object_audit FAIL with 5 storage rows. The split says external, but our source is static and the names are absent from cachebeta publics. All relocations naming these functions are inside model_animations.obj (xrefs_all.py over every split object).

After the scratch-split regen: only these 5 rows changed from 2 to 3, and all 68 sections are identical. object_audit on the production build/base obj: PASS, 73/73 symbols. pdb_storage2: 0 disagreements. gate: 32/32 EXACT. provider_link: every surplus item passes except _distance3d, including _fast_ftol against actor_combat.

## blockers
The surplus _distance3d COMDAT from real_math.h differs from January's selected NODUP hand copy in source/ai/action_vehicle.obj (x/z squaring order), and fails to link in both orders (LNK2005). January's model_animations has no call relocation to _distance3d.

## reopen_criteria
Reopen after the board-wide action_vehicle distance3d provider repair. Then rerun provider_link on this object; it should pass in both orders.

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
