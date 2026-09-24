# source/ai/path_obstacle_avoidance

## verdict
BLOCKED

## newly_exact_functions
[]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/path_audit/plink_path_obstacle_avoidance.txt",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/path_audit/surplus_path_obstacle_avoidance.txt"
]

## production_changes
None. The provider repair for source/ai/actor_combat _cross_product2d is owned by the provider worker.

## evidence
24/24 EXACT, object_audit PASS (52 January symbols, 0 differ), pdb_storage 0 disagreements. All 11 surplus COMDATs are IDENTICAL to January's selected copies. The provider link fails only on `_cross_product2d` (provider actor_combat): LNK2005 in both orders. actor_combat.c renames the header inline (#define cross_product2d cross_product2d_inline at :129) and hand-writes a NODUP copy at :523, with no call anywhere in actor_combat.c. January census: there are ZERO out-of-line references to _cross_product2d anywhere on the board, and January's path_obstacle_avoidance neither defines nor references it. January's actor_combat row is therefore an emitted-but-unreferenced COMDAT from an inlined use.

## blockers
Helper `_cross_product2d`. The selected provider is source/ai/actor_combat, which holds a NODUP hand copy at actor_combat.c:523 with no genuine use. Our copy is byte-identical but fails with LNK2005 in both orders.

## reopen_criteria
actor_combat emits _cross_product2d as a SELECT_ANY header inline, by restoring January's genuine inlined use and removing the rename and hand copy while keeping actor_combat's rows exact. Then rerun provider_link.py on source/ai/path_obstacle_avoidance and admit.

## task notes
Tools written under scratch/w/path_audit/tools/: gate_lab.py (a copy of gate.py whose main file compiles from scratch/w/path_audit/lab/ so lab/<dir>/<header> shadows a shared header; lab only), sweep.py and sweep2.py (per-function and per-section board sweeps of base vs lab, with controls), symdis.py, strrefs.py, strcensus.py, symcensus.py, linkpair.py (VC7 pair link in both orders), mkvariant.py, mk_objects_variant.py, object_audit_split.py / pdb_storage_split.py (run against a scratch split via the AUDIT_SPLIT env var).

Caveat for the integrator: in the lab, rasterizer_frame_statistics _rasterizer_frame_statistics_draw flips EXACT/residual even with an UNCHANGED header copy. This is an environment artifact (main-file path/PID), so ignore it in lab sweeps.

Useful follow-ups from the D1 sweep: players _player_set_action_result gets closer (16 -> 6 diff lines; its three `real_point3d const *` pointer locals look like the same M8 steer). Pre-existing pdb_storage disagreements outside this lane: action_vehicle _action_vehicle_evaluate_seat; object_lights _cluster_get_first_light, _cluster_get_next_light, _light_attenuation, _should_render_lights.

Line endings: all touched source files are CRLF, and GNU patch converted a test file to LF, so hand-apply the edits or copy the lab candidate files.

The lab shadow headers were restored to their original copies. The lab/objects shadow is active with an unchanged objects.h; lab/math/real_math.h is .orig.
ledger: C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/path_audit/LEDGER.md
