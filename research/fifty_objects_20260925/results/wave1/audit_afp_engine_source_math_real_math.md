# source/math/real_math

## verdict
FUNCTIONS_EXACT_OBJECT_BLOCKED

## newly_exact_functions
null

## candidate_files
null

## production_changes
APPLY NOW; these fixes do not depend on the provider blocker and are verified.
(1) Source: apply C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\actor_firing_position_audit\real_math_cand2.diff to source/math/real_math.c. The full file is rm_cand2.c.
  - Delete the invented `struct real_math_constant_block` and the `real_math_constants` aggregate.
  - Define four file statics, in this order and with the same initialisers: `static real_matrix4x3 const private_identity4x3`, `static real_matrix4x3 const private_negative_identity4x3`, `static real_quaternion const private_identity_quaternion`, `static real_rectangle3d const private_null_rectangle`.
  - Point the 31 global_* pointer initialisers at those statics, for example `&private_identity4x3.up` and `&private_null_rectangle`. The existing casts are unchanged.
(2) config/symbols.json: edit lines in place. Rename line `2600880 _real_math_constants static` to `_private_identity4x3` (keep static:true). Insert directly after it, in ascending order: `{ "file_offset": 2600932, "flags": 0, "name": "_private_negative_identity4x3", "static": true },`, `{ "file_offset": 2600984, "flags": 0, "name": "_private_identity_quaternion", "static": true },`, `{ "file_offset": 2601000, "flags": 0, "name": "_private_null_rectangle", "static": true },`. The file uses CRLF line endings. These edits are in symbols_json_all.diff.
(3) Apply source and symbols.json together, then regenerate the split and run a full ninja. The January split's .data relocations get renamed to the new statics.
No other config or source names real_math_constants. Do NOT set Matching until the 4 provider conflicts are resolved.

## evidence
Before: object_audit FAIL, 2 problems:
- _real_math_constants storage mismatch (January static, ours external).
- The 168-byte .rdata section alignment: January ALIGN_4 (contribs module 222, flags 0x40300040) versus ours ALIGN_8.

R1 (static only) fixed the storage, but the alignment stayed 8.

The aggregate-vs-scalar alignment rule says any object of 64 bytes or more forces 8-byte alignment. January's 4-byte alignment therefore proves the 144-byte aggregate did not exist.

HCEX.pdb (compiland real_math.obj, saved as hcex_real_math_compiland.txt) lists 4 file statics: private_identity4x3, private_negative_identity4x3, private_identity_quaternion and private_null_rectangle. Their sizes (52, 52, 16, 24) put them at January offsets 0, 0x34, 0x68 and 0x78; global_projection3d_mappings sits at 0x90. January's only references to the block are inside real_math's own .data.

R2 (rm_cand2): 84/84 EXACT. The .rdata section is now 0x40300040 (ALIGN_4), with symbols at 0/52/104/120 and 144, exactly matching January.

Against the scratch split: object_audit PASS, 134/134 symbols; pdb_storage2 0 disagreements. The _real_math_constants 'NO JANUARY PROVIDER' row is gone.

## blockers
The surplus helper COMDATs are byte-IDENTICAL to January's selected copies, but those providers are no-duplicates (NODUP) hand copies (nodup_census), so linking fails with LNK2005 in both orders:
- _cross_product2d: source/ai/actor_combat.obj
- _project_point3d: source/ai/path_obstacles.obj
- _projection_sign_from_vector3d: source/effects/decals.obj
- _triple_product3d: source/effects/decals.obj
January's own point_in_triangle3d calls _project_point3d (x3) and _projection_sign_from_vector3d, so real_math's copies are legitimate. The defect is the hand copies in the providers.

## reopen_criteria
Reopen when actor_combat, path_obstacles and decals replace their NODUP hand copies with the genuine real_math.h inlines, with every exact caller kept and zero board regressions. Then rerun provider_link on rm_cand2.obj; it should pass in both orders.

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
