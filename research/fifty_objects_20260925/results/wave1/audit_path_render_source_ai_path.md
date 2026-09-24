# source/ai/path

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
[]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/path_audit/path_distance3d_1_real_math_h.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/path_audit/path_distance3d_2_object_lights_c.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/path_audit/path_distance3d_3_action_vehicle_c.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/path_audit/lab/math/real_math.h.distance3d",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/path_audit/lab/cand_ol_nolocal.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/path_audit/lab/cand_av_dist.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/path_audit/lab/sweep2_d1.txt"
]

## production_changes
Three edits, all CRLF files. GNU patch rewrote the file as LF in a test, so hand-apply these edits or copy the lab files byte-for-byte. (1) source/math/real_math.h, the distance3d body (around lines 1195-1196): replace `real_vector3d v;` and `return magnitude3d(vector_from_points3d(a, b, &v));` with the single line `return square_root(distance_squared3d(a, b));`. (2) source/objects/object_lights.c:1834-1835: replace `real_point3d const *light_position = &light->position;` and `real distance = distance3d(light_position, center);` with `real distance = distance3d(&light->position, center);`. (3) source/ai/action_vehicle.c: delete `#define distance3d distance3d_inline` (line 73), `#undef distance3d` (line 79), and the hand copy `real distance3d(real_point3d const *a, real_point3d const *b) { return square_root(distance_squared3d(a, b)); }` (lines 202-207). This third edit is the action_vehicle provider repair, so dedupe it with the provider worker. (4) Re-measure the park for source/ai/ai _ai_test_line_of_sight in config/parked.json: measurements.base changes to 1040 B / 50 relocs / normalized_sha256 c524fe19529bfd4a72e95be52a875925bcc929d347b37471eb16b75bd393da78; re-take objdiff_percent. It stays residual, but its moving bytes would otherwise fail the PROGRESS check. (5) Full ninja, regression gate and stable sweep. Then admit source/ai/path as Matching. No symbols.json change and no split regeneration are needed.

## evidence
Production audit: path is 32/32 EXACT, object_audit PASS (74 January symbols, 0 differ), pdb_storage 0 disagreements. The only failure is `_distance3d`. January's path.obj references it out of line (from _path_state_build_path). Our COMDAT differs from January's selected action_vehicle copy in x87 operand order, and action_vehicle defines it as an ordinary hand copy (NODUP), so the link fails with LNK2005 in both orders.

Decode: January's selected copy equals the `square_root(distance_squared3d(a,b))` form. With that header body (probe D1), path stays 32/32, its _distance3d is section_infos_equal to January's, and all 19 TUs that emit _distance3d produce byte-identical copies. The one regression in a 20-TU sweep was object_lights _find_point_lights_for_object_in_cluster. That turned out to be a steering local: with the natural call `distance3d(&light->position, center)` (D2) object_lights is 43/43 and object_audit PASS. With the old header body both that spelling and an argument swap fail, so this is a real strip-test result.

D3 (action_vehicle without the rename and hand copy; the genuine out-of-line use at action_vehicle.c:555 stays) gives 17/17, object_audit PASS, and the header-emitted _distance3d owns January's row. Pair link of path + action_vehicle PASSes in both orders.

Board sweep with D1+D2+D3 (lab/sweep2_d1.txt): 466 units, 0 January-status changes, 0 compile failures. Content-only moves: three residuals (_ai_test_line_of_sight, which is parked; _actor_perception_refresh_danger_zone, 234->238 diff lines; _player_set_action_result, 16->6 diff lines, closer). _distance_squared3d is newly emitted in 11 TUs, all identical to January's action_alert copy, and that provider links PASS. _magnitude3d is no longer emitted in 4 TUs.

All other path surplus is IDENTICAL and links PASS. `_point_from_line3d` is emitted even though January's path.obj does not reference it out of line. It is identical and links PASS, which satisfies the current rule-6 exception in docs/campaign_house_rules.md; flag it if the older 17-object PFL ruling is still enforced. The .drectve surplus is systemic.

## blockers
None remaining in path itself once the three edits land. The admission depends on the action_vehicle provider repair (edit 3), which the provider worker may also be doing, and on the integrator's full-build verification.

## reopen_criteria
If the integrator's full build shows any January-status regression or the regression gate fails, stop and report the row. Do not revert only the header body: path, object_lights and action_vehicle must move together.

## task notes
Tools written under scratch/w/path_audit/tools/: gate_lab.py (a copy of gate.py whose main file compiles from scratch/w/path_audit/lab/ so lab/<dir>/<header> shadows a shared header; lab only), sweep.py and sweep2.py (per-function and per-section board sweeps of base vs lab, with controls), symdis.py, strrefs.py, strcensus.py, symcensus.py, linkpair.py (VC7 pair link in both orders), mkvariant.py, mk_objects_variant.py, object_audit_split.py / pdb_storage_split.py (run against a scratch split via the AUDIT_SPLIT env var).

Caveat for the integrator: in the lab, rasterizer_frame_statistics _rasterizer_frame_statistics_draw flips EXACT/residual even with an UNCHANGED header copy. This is an environment artifact (main-file path/PID), so ignore it in lab sweeps.

Useful follow-ups from the D1 sweep: players _player_set_action_result gets closer (16 -> 6 diff lines; its three `real_point3d const *` pointer locals look like the same M8 steer). Pre-existing pdb_storage disagreements outside this lane: action_vehicle _action_vehicle_evaluate_seat; object_lights _cluster_get_first_light, _cluster_get_next_light, _light_attenuation, _should_render_lights.

Line endings: all touched source files are CRLF, and GNU patch converted a test file to LF, so hand-apply the edits or copy the lab candidate files.

The lab shadow headers were restored to their original copies. The lab/objects shadow is active with an unchanged objects.h; lab/math/real_math.h is .orig.
ledger: C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/path_audit/LEDGER.md
