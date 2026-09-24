# source/ai/actor_firing_position

## verdict
FUNCTIONS_EXACT_OBJECT_BLOCKED

## newly_exact_functions
null

## candidate_files
null

## production_changes
APPLY NOW; these fixes do not depend on the provider blocker and are verified.
(1) Source: apply C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\actor_firing_position_audit\actor_firing_position_cand1.diff to source/ai/actor_firing_position.c. The full file is afp_cand1.c in the same folder.
  - Line 397: drop `static` from global_pre_evaluator_table.
  - Line 409: drop `static` from global_post_evaluator_table.
  - Lines 420-421: `short global_temporary_sort_firing_position_count= 0;` and `struct firing_position *global_temporary_sort_firing_position_array= NULL;`.
(2) config/symbols.json: edit lines in place, never re-serialise. Append `, "static": true` to these 14 entries: _firing_position_store_evaluation_debug (75280), _firing_position_reject (75360), _pre_evaluator_guard (75376), _pre_evaluator_combatmove (75584), _post_evaluator_global (76160), _post_evaluator_pursuit (76384), _post_evaluator_hide (76768), _post_evaluator_uncover (76992), _post_evaluator_attack (77184), _firing_position_compare (77664), _pre_evaluator_global (78592), _pre_evaluator_pursuit (80208), _pre_evaluator_panic (80672), _pre_evaluator_attack (88608). The combined diff for all 4 units is scratch/w/actor_firing_position_audit/symbols_json_all.diff.
(3) Regenerate the split, then run a full ninja.
Do NOT set config.json status to Matching until the _distance3d provider is repaired.

## evidence
Before (production): object_audit FAIL with 18 storage differences. pdb_storage showed two separate faults:
- 14 functions are absent from cachebeta publics, so January had them file-static, but the split says external.
- 4 data symbols are PDB PUBLIC but our source had them static.
Why the sort globals need an initialiser: contribs.json shows module 456 owns its own 8-byte .bss contribution at 0x319ca0, not the linker_common pool. An uninitialised external would be COMMON, so January wrote them with `= 0`/`= NULL`.

After:
- afp_cand1 gate 27/27 EXACT; the .bss and .rdata sections stay byte-, relocation- and alignment-identical.
- csplit regen with the scratch config: of 833 split objects, only this one plus game_engine, real_math and model_animations differ; here the sections are identical and only the 14 storage rows changed.
- object_audit against the scratch split: PASS, 73/73 symbols.
- pdb_storage2: 0 disagreements.
- provider_link: every surplus item passes except _distance3d.
Files: LEDGER.md, final_gate.txt, final_verification.txt, xrefs_all.py, pub_census.py.

## blockers
The surplus _distance3d COMDAT from real_math.h is not byte-identical to January's selected copy in source/ai/action_vehicle.obj. That copy is a no-duplicates (NODUP) hand copy.
- Order differs: ours squares dz first and ends `fstp st(3)`; January's squares dx first and uses `fxch st(2); fsqrt; fstp st(2)`.
- It fails to link against action_vehicle in both orders (LNK2005).
- January's actor_firing_position has no call relocation to _distance3d, so the COMDAT is the normal header-inline emission.
This is the same conflict already recorded for leaf_map and biped_limp_noodle in config/object_admission_rejections.json.

## reopen_criteria
Reopen when the action_vehicle distance3d provider is repaired board-wide: a real_math.h distance3d that is byte-identical to January's selected copy, with no loss of an exact sibling. Then rerun provider_link on this object; it should pass in both link orders, which makes the object admissible.

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
