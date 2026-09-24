# source/ai/path_smoothing

## verdict
BLOCKED

## newly_exact_functions
[]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/path_audit/plink_path_smoothing.txt",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/path_audit/surplus_path_smoothing.txt"
]

## production_changes
None. The same actor_combat _cross_product2d provider repair is owned by the provider worker.

## evidence
6/6 EXACT, object_audit PASS (15 January symbols, 0 differ), pdb_storage 0 disagreements. All 10 surplus COMDATs are IDENTICAL. The provider link fails only on `_cross_product2d` against actor_combat's NODUP hand copy (LNK2005 in both orders). January's path_smoothing neither defines nor references _cross_product2d, and there are no January out-of-line references to it anywhere. This matches the earlier object-closure-lane finding.

## blockers
Helper `_cross_product2d`: provider source/ai/actor_combat holds a NODUP hand copy (actor_combat.c:523) with no use in actor_combat.c. Our bytes are identical, but the link fails in both orders.

## reopen_criteria
The same as path_obstacle_avoidance: once the actor_combat provider emits _cross_product2d as SELECT_ANY with its rows exact, rerun provider_link and admit.

## task notes
Tools written under scratch/w/path_audit/tools/: gate_lab.py (a copy of gate.py whose main file compiles from scratch/w/path_audit/lab/ so lab/<dir>/<header> shadows a shared header; lab only), sweep.py and sweep2.py (per-function and per-section board sweeps of base vs lab, with controls), symdis.py, strrefs.py, strcensus.py, symcensus.py, linkpair.py (VC7 pair link in both orders), mkvariant.py, mk_objects_variant.py, object_audit_split.py / pdb_storage_split.py (run against a scratch split via the AUDIT_SPLIT env var).

Caveat for the integrator: in the lab, rasterizer_frame_statistics _rasterizer_frame_statistics_draw flips EXACT/residual even with an UNCHANGED header copy. This is an environment artifact (main-file path/PID), so ignore it in lab sweeps.

Useful follow-ups from the D1 sweep: players _player_set_action_result gets closer (16 -> 6 diff lines; its three `real_point3d const *` pointer locals look like the same M8 steer). Pre-existing pdb_storage disagreements outside this lane: action_vehicle _action_vehicle_evaluate_seat; object_lights _cluster_get_first_light, _cluster_get_next_light, _light_attenuation, _should_render_lights.

Line endings: all touched source files are CRLF, and GNU patch converted a test file to LF, so hand-apply the edits or copy the lab candidate files.

The lab shadow headers were restored to their original copies. The lab/objects shadow is active with an unchanged objects.h; lab/math/real_math.h is .orig.
ledger: C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/path_audit/LEDGER.md
