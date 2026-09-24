# source/render/render_objects

## verdict
BLOCKED

## newly_exact_functions
[]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/path_audit/cand_ro_b1.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/path_audit/render_objects_lighting_storage_B1.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/path_audit/render_objects_lighting_storage_B1_symbols_json.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/path_audit/config_b1/symbols.json",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/path_audit/cand_ro_a.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/path_audit/render_objects_lighting_storage_A.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/path_audit/render_objects_lighting_storage_A_symbols_json.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/path_audit/render_objects_ogbs_1_action_vehicle_c.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/path_audit/render_objects_ogbs_2_objects_h.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/path_audit/lab/cand_av_ogbs.c"
]

## production_changes
This storage fix is independent of the helper blockers and can be applied now; it keeps 22/22. B1 (HCEX-attested, primary): (1) source/render/render_objects.c: delete the file-scope line `struct render_lighting lighting_storage = { 0 };` (line 307). Insert `	static struct render_lighting lighting_storage;` as the first line inside object_get_cached_render_lighting(...) { ... }, before `long render_state_index = ...`. (2) config/symbols.json line 23248: edit the line in place, without re-serializing, from `{ "file_offset": 4980856, "flags": 0, "name": "_lighting_storage" },` to `{ "file_offset": 4980856, "flags": 0, "name": "?lighting_storage@?1??object_get_cached_render_lighting@@9@9", "static": true },`. (3) Regenerate the split with csplit and rebuild. Only source/render/render_objects.obj changes in the split (verified byte-for-byte over all 833 objects). Fallback A, if the owner prefers the file-scope form: make line 307 `static struct render_lighting lighting_storage = { 0 };` and add only `, "static": true` to line 23248. The helper blockers are not landable; proposals for the provider worker: render_objects_ogbs_1_action_vehicle_c.patch (drop action_vehicle's object_get_bounding_sphere rename and hand copy) and render_objects_ogbs_2_objects_h.patch (objects.h:491-492 file string -> "..\\objects\\objects.h"). The objects.h edit needs an owner ruling on per-TU __FILE__ modelling first.

## evidence
22/22 EXACT, object_audit PASS against the current split. pdb_storage flags `_lighting_storage`: split 2, ours 2, but it is absent from the cachebeta.pdb publics, so January had it non-external. HCEX lists lighting_storage as a Static Local of object_get_cached_render_lighting. No other .c references it.

B1 candidate: 21/22 against the current split, the one difference being reloc-identity from the name. Against the scratch split built from config_b1: object_audit PASS (41 symbols, 0 differ), pdb_storage 0 disagreements, and the .bss layout is identical to January's (static local at 0, render_object_globals at 120, debug_inactive_objects at 1148 external, overflow at 1149, 1150 B). Candidate A is 22/22 and also PASSes against split_a. The initialised static-local form was REFUTED: .bss becomes 1148 B and the local moves to offset 1032. As a control, candidate A against the production split fails only on `_lighting_storage` storage 2/3, so the symbols.json edit is required together with the source edit.

Helpers: `_object_get_bounding_sphere` differs from January's selected action_vehicle copy only in the assert FILE string. Ours is "c:\halo\source\objects\objects.h"; January's is "..\objects\objects.h". January's own render_objects-owned _object_get_render_bounding_sphere also uses "..\", so January's render_objects copy was identical to the selected one. A raw scan of cachebeta.exe finds zero code references to the c:\ string (an orphan attributed to object_lights), and 4 references to "..\objects\objects.h" (action_vehicle and render_objects).

Lab B2 (objects.h "..\" + action_vehicle without its hand copy): 0 function changes across all 9 users; action_vehicle 17/17 and object_audit PASS; both copies identical; render_objects + action_vehicle link PASS in both orders. However, object_lights then loses its January-owned 33 B .rdata string ??_C@_0CB@NDAECIIM (object_audit FAIL).

`_plane3d_negate` is byte-identical to January's decals copy, but decals holds a NODUP hand copy (decals.c:896) with no call in decals.c. January's out-of-line references to it come only from collisions, bipeds and units. `_plane3d_distance_to_point` links PASS.

## blockers
(1) `_object_get_bounding_sphere`, provider source/ai/action_vehicle: a NODUP hand copy (action_vehicle.c:168, with its rename at :82), plus the objects.h hardcoded assert string. A single header string cannot serve both object_lights, which needs the c:\ string for its January-owned .rdata, and render_objects/action_vehicle, which need "..\". It needs an owner ruling on per-TU __FILE__ modelling. (2) `_plane3d_negate`, provider source/effects/decals: a NODUP hand copy with no genuine use in decals.c, so the link fails with LNK2005 in both orders.

## reopen_criteria
Two conditions. First, the owner rules on how objects.h assert strings vary per TU (a TU in the header's own directory sees c:\halo\source\<dir>\<h>, other directories see ..\<dir>\<h>), and object_lights keeps its string while action_vehicle's object_get_bounding_sphere hand copy is replaced by the SELECT_ANY header inline (patches verified in lab). Second, the decals provider emits plane3d_negate SELECT_ANY from a genuine January use. Then apply B1, regenerate the split, rerun object_audit, provider_link and surplus_identity, and admit.

## task notes
Tools written under scratch/w/path_audit/tools/: gate_lab.py (a copy of gate.py whose main file compiles from scratch/w/path_audit/lab/ so lab/<dir>/<header> shadows a shared header; lab only), sweep.py and sweep2.py (per-function and per-section board sweeps of base vs lab, with controls), symdis.py, strrefs.py, strcensus.py, symcensus.py, linkpair.py (VC7 pair link in both orders), mkvariant.py, mk_objects_variant.py, object_audit_split.py / pdb_storage_split.py (run against a scratch split via the AUDIT_SPLIT env var).

Caveat for the integrator: in the lab, rasterizer_frame_statistics _rasterizer_frame_statistics_draw flips EXACT/residual even with an UNCHANGED header copy. This is an environment artifact (main-file path/PID), so ignore it in lab sweeps.

Useful follow-ups from the D1 sweep: players _player_set_action_result gets closer (16 -> 6 diff lines; its three `real_point3d const *` pointer locals look like the same M8 steer). Pre-existing pdb_storage disagreements outside this lane: action_vehicle _action_vehicle_evaluate_seat; object_lights _cluster_get_first_light, _cluster_get_next_light, _light_attenuation, _should_render_lights.

Line endings: all touched source files are CRLF, and GNU patch converted a test file to LF, so hand-apply the edits or copy the lab candidate files.

The lab shadow headers were restored to their original copies. The lab/objects shadow is active with an unchanged objects.h; lab/math/real_math.h is .orig.
ledger: C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/path_audit/LEDGER.md
