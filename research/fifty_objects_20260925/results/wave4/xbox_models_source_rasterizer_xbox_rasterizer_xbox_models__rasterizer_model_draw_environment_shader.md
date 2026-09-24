# source/rasterizer/xbox/rasterizer_xbox_models::_rasterizer_model_draw_environment_shader

## verdict
FUZZY_IMPROVED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_models\\production.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_models\\best_candidate.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_models\\env\\L_h_blkpp.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_models\\mk_envh.py"
]

## production_changes
Part of the same production.patch. The environment shader changes are:
- The hand-expanded camera vector is replaced by vector_from_points3d(&camera.position,&local_parameters->centroid,&camera_to_model), as in /Od 0x7f305a.
- perpendicular and parallel become real_argb_color, the /Od RTC 16-byte reflection colours, replacing four brightness/colour locals.
- cc0_error becomes real_rgb_color (/Od 12 bytes); the pixel call passes &cc0_error.
- Fog block, in /Od RTC declaration order: `real camera_plane_distance= plane3d_distance_to_point(&fog.plane,&camera.position)` and `camera_distance = dot_product3d(&camera.forward,&camera_to_model)` (/Od 0x7f37bb and 0x7f385b). These replace the hand-expanded plane distance and dot product inside the PINs.
- vertex_constants and specular_constants, plus a declared-and-initialised detail_map_scale, move into a block around the constant stores (CALL-RESULT ALIAS SCOPE law).
- vertex_shader_permutation is declared right after shader_environment (/Od order).

parked.patch re-baselines this park: base 3104/235, normalized_sha256 a4a4ca436d309778ff1ad21a7b5cbe35e4701fc4b4c22e06ae7263d206dfa9bb, objdiff 96.38508, with an evidence note appended. Zero exact credit.

## evidence
Baseline [sha], 70 real regions, 95.87124%. /Od 0x7f2f80 RTC names: forward, argb reflection_view_perpendicular_color and parallel_color, vsh_constants__texscale 48, vsh_constants__effect 32, cc0 16, cc0_error 12, cc1 12. Named fog locals come from the /Od code.

January inlines plane3d_distance_to_point here. This fits a draw-side finding: a constant global address makes the helper cheap enough to inline at any depth, while a pointer local keeps it out of line.

Measured:
- w3 a2 spellings: 41 real regions (3104).
- Adding the named fog locals and the constants block (odloc + blk) gives 38 real regions and 96.38508%. Submit and draw hashes are unchanged, so no canary moves.
- Variants g3/g4/g5/g6 (declaration+initialisation, detail scale declared in the block, /Od declaration orders) are byte-identical.

Lab only, not landable:
- A TU-local __inline set_real_vector4d plus perpendicular/parallel/constants in one block (L_h_blkpp) reproduces January's 0x3f3..0x46b window. That window is the `mov edx,[esi+0xb4]; mov ecx,edx` copy with stores after the perpendicular stores, which fixes the w3 blocker. Size is still 3120 (97.39%).
- L_h_noblk (w3 s3) scores 97.88% at 3120.
- Direct D3DDevice_SetRenderState for ALPHATESTENABLE: inert.
- Detail scale read twice: 3120.

In this build and in the split, set_real_vector4d's only definer is environment_fog, as a NODUP copy (flags 32 in symbols.json).

## blockers
January's detail-scale copy needs set_real_vector4d as a genuine shared-header __inline. That is a shared real_math.h-class header edit (board-wide declaration-count blast radius) that replaces environment_fog's NODUP hand copy. It is P1 class until this caller is strictly exact.

Remaining +16 bytes: January computes the ALPHATESTENABLE value straight into edi (`movzx edi,byte [esi+0x28]`), while ours computes it in eax and copies it (allocator homing). Plus downstream register rotations and the cc0/cc1 pixel edi/ebx permutation. No decoded source cause.

## reopen_criteria
Reopen when a shared-header packet turns set_real_vector4d into the genuine header __inline, with a board sweep and environment_fog's hand copy removed. Combine it with the measured blkpp scope and a decoded cause for the edi homing of the ALPHATESTENABLE value and the cc0/cc1 pixel edi/ebx permutation.

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
