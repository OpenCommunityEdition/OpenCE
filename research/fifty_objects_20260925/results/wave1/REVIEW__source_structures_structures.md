# review source/structures/structures
approve=False

## checks
All checks were run independently. Notes are in C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/review_structures/REVIEW.md.
(1) Patch identity. production.patch was applied to a copy of production structures.c (HEAD e9e62b78, sha 72611997, tracked file untouched). The result is byte-identical to scratch/w/structures/final.c (sha256 2fb89c42). `git apply --check` is clean at the repo root, and the patch touches only the sphere_intersects_cluster_portal body.
(2) Gate. Baseline (production copy): exact 16, residual 1, `_sphere_intersects_cluster_portal [size 416!=432, sha]`. Candidate final.c into review_structures/cand.obj: exact 17, residual 0, unwritten 0. Reproduced.
(3) relocdiff on cand.obj: 11/11 relocations, 0 differing rows, size 432/432, sha equal.
(4) object_audit cand.obj: PASS, 26 January symbols, 0 differ. The surplus list is identical to both the gated production copy and build/base structures.obj.
(5) surplus_identity run against cand.obj (local copy surplus_identity_cand.py): 12 code COMDATs, 0 not identical.
(6) provider_link on cand.obj: FAIL(2), on `_project_point3d` (path_obstacles NODUP) and `_projection_sign_from_vector3d` (decals NODUP). build/base output is identical, and `--baseline` shows no new surplus. This failure was already there and the patch does not introduce it.
(7) pdb_storage: 0 disagreements. build/report.json data is 2292/2292 (100%). Code is 3470/3888; the 418-byte gap is this function.
(8) No parks or rejections exist for the unit. I read the prior sphere ledgers (first_shot, w2, n1, fog_inline_closeout). The point_from_line3d COMDAT is already admitted in this TU.
(9) Reviewer strip test on the load-bearing construct:
- S1 `real t = -plane_distance;`: residual [sha].
- S2 no t, `-plane_distance` passed directly (the /Od spelling): residual [sha]. alndiff shows the same three one-slot moves: movzx ax,al; push eax; lea eax,[ebp-0xc].
(10) I read the /Od dump od_sphere_portal.txt (0x8a0670). Statement order, the standalone call, short projection/sign, the plane local, the RTC names and the single shared false epilogue are all confirmed.
(11) /W3 re-run: the candidate adds C4244 double->real at final.c:367 plus 2x C4244 short->boolean. S2 adds only the two short->boolean warnings.
(12) House-rule pass on the diff, with precedents checked: actor_moving.c:2136 (commit ea507d5b), units.c:11087, and the Lane B manifest reopen bar.

## issues
Not approved. Every worker measurement reproduces, but the one construct that closes the function does not pass admission.

1. STRIP TEST FAILS. Only the `double t` local matches. The type-stripped `real t` (S1) and the plain `point_from_line3d(point, &plane->n, -plane_distance, ...)` (S2) are both residual [sha]. The worker's TG2 stripped only the inert `(real)` cast, not the double. By brief section 2, when only the decorated form matches, the decoration is the fake.

2. CONTRADICTED BY FIRST-PARTY EVIDENCE.
- The /Od readout at 0x8a07bb negates the float local inline at the call (`movss xmm0,[ebp-0xc]; xorps; movss [esp],xmm0`).
- The /Od frame has no slot for any `t`; every user-area slot is accounted for. So the later source passed `-plane_distance` directly.
- In the same TU, the exact January sibling _render_debug_fog_planes also passes `-render.fog.planar_maximum_distance` directly to point_from_line3d (final.c:607-608).
- The worker also shows collision_prism_test_point: the same helper with a float `-distance` gives the separated form.
- The `double` is inferred only from the schedule. It is not attested anywhere.

3. HOUSE RULE. docs/common_constants.md says to use `double` only where an API or vararg promotion requires it; neither applies here. The construct also adds a /W3 C4244 double->real warning at final.c:367 that the plain spelling does not produce.

4. PRECEDENTS DO NOT SETTLE IT.
- actor_moving.c:2136 is a zero-credit fidelity landing in a still-parked function (ea507d5b), with no stated ruling on the double.
- The units tgr closure predates the /Od-attestation practice.
- The Lane B manifest (_actor_action_handle_vehicle_entry) explicitly requires a natural fact "without a double temp".
- If the owner admits it, the 2026-09-20 inferred-construct ruling needs an in-source comment saying it is inferred from January bytes and contradicted by /Od. The patch has no such comment.

5. PROCESS. The proposal says both "apply production.patch" and "HOLD FOR RULING". An integrator cannot apply that mechanically. It is an owner escalation, and the FUNCTIONS_EXACT label holds only if the owner admits the double.

6. OBJECT BLOCKER. provider_link FAIL(2) is real, but it already exists on production and is identical there, so it is not caused by this change.

Everything else in the diff is sound and an improvement:
- It removes production's hand expansion of point_from_line3d (a rule-6 violation) and uses the genuine inlined call.
- `point` and the RTC/HCEX names are correct.
- short sign is attested by January's `movzx ax,al` and the /Od word slot.
- It follows the /Od statement order and the single shared `return FALSE`.

A fallback that needs no ruling is scratch/w/review_structures/s2_inline_neg.c. It uses the /Od-attested plain spelling: 432/11 [sha], three one-slot moves, surplus identical to production. It earns zero credit and would need a park entry under rule 11.

Recommended action: do not apply production.patch now. Send the double-t question to the owner with REVIEW.md R4/R5 as the evidence.
