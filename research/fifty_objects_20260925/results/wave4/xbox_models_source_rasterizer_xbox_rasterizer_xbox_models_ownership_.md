# source/rasterizer/xbox/rasterizer_xbox_models (ownership)

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_models\\config\\symbols.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_models\\symbols.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_models\\audit_r.py"
]

## production_changes
config/symbols.json: in-place edits keeping CRLF. Full edited copy: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\rasterizer_xbox_models\config\symbols.json. Patch: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\rasterizer_xbox_models\symbols.patch (git apply --check clean).

Append `, "static": true` to lines 5844 (_D3DDevice_SetRenderState, 1417552), 5845 (_D3DDevice_SetTextureStageState, 1417984), 5847 (_IDirect3DDevice8_SetRenderState@12, 1419072), 5848 (_IDirect3DDevice8_SetTextureStageState@16, 1419616) and 5849 (_IDirect3DDevice8_SetVertexShaderConstant@16, 1419712).

This is independent of the .c patch and earns zero code credit. It needs a csplit regen.

## evidence
pdb_storage: the five D3DINLINE wrapper rows of this object are external in the split, static in ours, and absent from cachebeta publics. Same fix class as the dynavobgeom, shadows and rasterizer_xbox wrapper rows.

csplit regen (build/tools/csplit.exe -i cachebeta.exe -p slug/cfg -o slug/split): rc 0, 833 objects, only rasterizer_xbox_models.obj differs.

Against the regenerated split, best_candidate.obj is 12/14 strict exact. The slug-copied object audit (AUDIT_SPLIT_ROOT) shows 58/58 January symbols equal; only the two residual .text sections differ, FAIL(2).

build/report.json data is 1912/1912 (100%). No object_admission_rejections entry exists. The surplus COMDAT set is unchanged from production.

## blockers
The object is not admissible even when all bytes match:
- the two residual functions above;
- TU-local struct views with opaque spans (rasterizer_debug_options, rasterizer_model_begin_parameters, shader_*_definition, rasterizer_models_frame_statistics);
- the C4013 implicit declaration of rasterizer_frame_statistics_count_static_vertices. The owner header include is blocked by the local frame-statistics view (w3 A7, owner decision).

## reopen_criteria
Reopen the whole object when both functions close. It also needs genuine shared declarations to replace the TU-local views (dynavobgeom precedent) and the rasterizer_frame_statistics.h layout/owner ruling (vertices_by_permutation vs the object counts at +0x14; renaming +0x150..+0x168).

## task notes
Neither function became exact.

Key new mechanism, F1: VC7's per-site inline gating counts an if-region before it folds the condition. `if (TRUE)` or a null test of an already-dereferenced pointer produces no code, yet drops the site's budget by one step. This explains January's out-of-line _subtract_vectors3d in draw, with no visible test in the bytes. With such a region plus the /Od-attested calls, the block scopes and the plasma sub-structure, draw reaches [sha]-only at 99.57%: size, relocations, frame and every slot are exact. That is an owner question (the January condition is unrecoverable), not a landing.

Also measured: plane3d_distance_to_point inlines at any depth when its argument is a constant global address, and stays out of line when the argument is a pointer local. That argument-shape effect contradicts the Lane C law that argument shape is inert, and it explains the draw/env asymmetry.

The transparent-submit function (already exact) is a fragile canary: plasma and many bare-block draw edits flip its tie. The integrator must run the full sweep after applying. The production patch keeps submit and all other 11 exact functions exact against both the current and the regenerated split.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\rasterizer_xbox_models\LEDGER.md has every probe, recorded as hypothesis, evidence, change and result: c1..c9, d1/d3, f1..f3, g1..g6, lab L_*, oracle o/u/f/s series, env e_* and L_h_*. It also has the evidence pass (E1-E6), the objdiff mini-project measurements, the csplit/audit verification, the do-not-repeat list and the reopen criteria.

No tracked file was modified; git status is clean. One stray redirect to /LEDGER.md was attempted before a shell variable was set. It failed with permission denied and nothing was created (verified).

Deliverables are in scratch/w/rasterizer_xbox_models/:
- production.patch (.c only; draw 95.08->95.71, env 95.87->96.39; 12/14 exact kept);
- parked.patch and config/parked.json (both parks re-baselined);
- symbols.patch and config/symbols.json (5 wrapper storage rows; csplit changes only this object);
- best_candidate.c (g6);
- owner_F1_draw_lab_scaffold.c (not landable).
