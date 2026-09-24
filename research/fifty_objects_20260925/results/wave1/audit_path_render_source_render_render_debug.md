# source/render/render_debug

## verdict
BLOCKED

## newly_exact_functions
[]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/path_audit/lab/pp2d_body.txt",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/path_audit/lab/math/real_math.h.pp2d",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/path_audit/lab/cand_decals_pp2d.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/path_audit/lab/sweep2_pp2d.txt"
]

## production_changes
None landable yet. Proposal for the provider worker: the real_math.h project_point2d body should become the decals form. That means `short x`/`short y` mapping locals, and a single store `p3d->n[projection] = (fabs(plane->n.n[projection])<_real_epsilon) ? 0.0f : ((plane->d - (p2d->x * plane->n.n[x])) - (p2d->y * plane->n.n[y])) / plane->n.n[projection];` in place of the long locals and if/else. The body is in lab/pp2d_body.txt. It also needs the decals provider repair: drop the project_point2d rename and hand copy at decals.c:216/227/833. Neither can land until the two blockers below are resolved.

## evidence
36/36 EXACT, object_audit PASS (63 January symbols, 0 differ), pdb_storage 0 disagreements. The only failure is `_project_point2d`: it is DIFFERENT from January's selected decals copy, and decals holds a NODUP hand copy, so the link fails with LNK2005 in both orders. January's render_debug references the helper out of line from _render_debug_point2d and _render_debug_line2d.

The decals-form header body (P1) gives render_debug 36/36 and a COMDAT section_infos_equal to January's. collision_bsp, collision_features and leaf_map copies also become identical; breakable_surfaces differs by one per-TU scheduling tie.

Board sweep (lab/sweep2_pp2d.txt), with controls: bitmap_drawing _bitmap_copy goes EXACT->residual under this header in 2 of 2 runs and stays EXACT under the unchanged header, with one `mov edi,[ebp+10h]` moved. That is distant blast radius. The rasterizer_frame_statistics flip also appears in the unchanged-header control, so it is a lab artifact. Splitting the change: short locals + if/else keeps bitmap_copy but the copy is not identical; long locals + ?: regresses bitmap_copy; short + if/else(<) is not identical. Only the full ?: form reproduces January's copy, and the ?: is what flips bitmap_copy. I stopped there under the methodology stop rule.

The decals repair lab (P2) makes the header-emitted _project_point2d exact but regresses _decals_delete_permanent_from_cluster: .bss relocations re-attribute and one scheduling move appears. With a SELECT_ANY decals, render_debug + decals link PASSes in both orders; against production decals it FAILs.

## blockers
Helper `_project_point2d`, provider source/effects/decals. (a) Our real_math.h body differs from January's selected copy, and the only exact body (short locals + ?:) flips bitmap_drawing _bitmap_copy, a known fragile tie. (b) decals defines it as a NODUP hand copy (decals.c:833; the genuine use is at decals.c:1500), and removing the hand copy regresses _decals_delete_permanent_from_cluster.

## reopen_criteria
The provider worker lands a project_point2d header body that is byte-identical to January's selected copy with the bitmap_copy tie held and a zero-regression board sweep, plus a decals SELECT_ANY emission that keeps all decals rows exact. Then rerun surplus_identity and provider_link on render_debug and admit.

## task notes
Tools written under scratch/w/path_audit/tools/: gate_lab.py (a copy of gate.py whose main file compiles from scratch/w/path_audit/lab/ so lab/<dir>/<header> shadows a shared header; lab only), sweep.py and sweep2.py (per-function and per-section board sweeps of base vs lab, with controls), symdis.py, strrefs.py, strcensus.py, symcensus.py, linkpair.py (VC7 pair link in both orders), mkvariant.py, mk_objects_variant.py, object_audit_split.py / pdb_storage_split.py (run against a scratch split via the AUDIT_SPLIT env var).

Caveat for the integrator: in the lab, rasterizer_frame_statistics _rasterizer_frame_statistics_draw flips EXACT/residual even with an UNCHANGED header copy. This is an environment artifact (main-file path/PID), so ignore it in lab sweeps.

Useful follow-ups from the D1 sweep: players _player_set_action_result gets closer (16 -> 6 diff lines; its three `real_point3d const *` pointer locals look like the same M8 steer). Pre-existing pdb_storage disagreements outside this lane: action_vehicle _action_vehicle_evaluate_seat; object_lights _cluster_get_first_light, _cluster_get_next_light, _light_attenuation, _should_render_lights.

Line endings: all touched source files are CRLF, and GNU patch converted a test file to LF, so hand-apply the edits or copy the lab candidate files.

The lab shadow headers were restored to their original copies. The lab/objects shadow is active with an unchanged objects.h; lab/math/real_math.h is .orig.
ledger: C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/path_audit/LEDGER.md
