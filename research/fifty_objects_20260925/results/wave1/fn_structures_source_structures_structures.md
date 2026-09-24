# source/structures/structures

## verdict
FUNCTIONS_EXACT_OBJECT_BLOCKED

## newly_exact_functions
[
 "_sphere_intersects_cluster_portal"
]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/structures/production.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/structures/final.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/structures/final_body.txt",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/structures/final.obj",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/structures/LEDGER.md",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/structures/od_sphere_portal.txt",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/structures/hcex_debug_sphere_ppc.txt"
]

## production_changes
Apply C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/structures/production.patch. It uses a/ b/ prefixes and `git apply --check` is clean. It touches only source/structures/structures.c and replaces the whole body of sphere_intersects_cluster_portal (lines 328-406). There are no header, config, symbols.json or parked.json changes, and no park to retire because the function was never parked. The new body:
(1) renames the third parameter `position` to `point` (the HCEX.pdb name).
(2) Declares `real plane_distance = plane3d_distance_to_point(<nested collision-bsp plane lookup>, point);`.
(3) Uses one nested test: `if (fabs(plane_distance) < radius && distance_squared3d(point, &portal->centroid) < (bounding_radius + radius) * (bounding_radius + radius)) { ... }`.
(4) Declares these inner-block locals in /Od order: `double t = -plane_distance;`, `real_point2d projected_points[MAXIMUM_VERTICES_PER_CLUSTER_PORTAL];`, `real_plane3d const *plane = TAG_BLOCK_GET_ELEMENT(&global_bsp3d_get()->planes, portal->plane_index, real_plane3d);`, `short projection = projection_from_vector3d(&plane->n);`, `short projection_sign = projection_sign_from_vector3d(&plane->n, projection);`, `real_point3d projected_centroid3d;`, `real_point2d projected_centroid2d;`, `short vertex_index;`.
(5) Calls `point_from_line3d(point, &plane->n, t, &projected_centroid3d);` as a separate statement, then `project_point3d(&projected_centroid3d, projection, projection_sign, &projected_centroid2d);`.
(6) Keeps the existing vertex projection loop.
(7) Ends with `if (convex_hull2d_test_circle(...)) { return TRUE; }` inside the block and one `return FALSE;` at the bottom.
After applying: run a full ninja build and the stable per-function sweep. The change is .c-local, so only structures.obj should move. Expected gate is 17/17. HOLD FOR RULING: `double t` is the load-bearing construct. docs/common_constants.md says to use `double` only where an API requires it; see blockers.

## evidence
Baseline gate: exact 16, residual 1: `_sphere_intersects_cluster_portal [size 416!=432, sha]`.

The w2 v5 body, measured again on current production (P01), gives 432/11 [sha] with 182/182 instructions. It differs from January only in three single-slot integer/x87 interleaves (movzx ax,al; push eax; lea eax,[ebp-0xc]).

/Od readout FOUND. The later build's version is at halo_cache_symbols.exe 0x8a0670; the earlier od_cone.txt was a different function (0x8a0c90), not this one. It gives:
- the statement order;
- a real standalone point_from_line3d call;
- `short` projection and `short` sign (movzx ax,al; mov word);
- the plane pointer local;
- the RTC names projected_points, projected_centroid3d and projected_centroid2d, which HCEX.pdb corroborates along with the parameter name `point`.

P02 is the full /Od shape: bytes identical to P01, so declaration order, nested vs standalone call and plane vs normal local are all inert. P03, a named sqrt local, is also inert.

Oracle labs all left the interleave unchanged:
- k=1..7 unused in-function dummy locals;
- k=1..16 TU-level declarations;
- k=1..7 used alias locals (these flip the M8 +0x5d term but not the interleave);
- a named `real` negated-distance local;
- hand-written rows.

The interleave is not a name-count, declaration-count or M8 tie. The exact sibling collision_prism_test_point (same two statements, float `-distance`) shows the separated dup/int/fmul form our build emits.

Hand-derived slot model (F04): add one no-code node between each lazy `fld st(0)` and its fmul. Applied to our schedule, this reproduces January's order exactly. That predicts a per-use double-to-real conversion, the units throw_grenade_release FP-intermediate-type lever.

Lab TG1 (`double lab_t = -plane_distance;` with a `(real)` cast) was EXACT. Strip test TG2 (no cast) was also EXACT, so the cast is inert. TG3 (a `double plane_distance` in the outer scope) changes the frame and is not January. C1 and C2 (declaration at block top vs. assignment just before the call) both gate the whole TU at exact 17 / residual 0.

Final final.c audit:
- gate 17/17 EXACT; relocdiff 11/11 with 0 differing rows.
- object_audit PASS: all 26 January symbols and sections match, and the surplus list is identical to production's.
- surplus_identity: 12/12 COMDATs identical to January's selected copies.
- pdb_storage: 0 disagreements.
- Data is already 100% in build/report.json (2292/2292).
- fake_match_scan: 0 leads.
- No rejections or parks for the unit.
- /W3 adds one C4244 (double to real argument) and two C4244 (short to boolean). The build uses the default warning level.

## blockers
(1) OBJECT LINK: provider_link.py fails in 2 places (LNK2005 in both orders).
- `_project_point3d` conflicts with the NODUP hand copy in source/ai/path_obstacles.c (`#define project_point3d project_point3d_inline` plus an ordinary definition).
- `_projection_sign_from_vector3d` conflicts with the NODUP hand copy in source/effects/decals.c.
The current production build/base structures.obj fails identically, so this is pre-existing and provider-side, not caused by this change. The recorded fix is an owner decision to make these provider copies header inlines (see NODUP provider link law; nodup_census reports NODUP in path_obstacles.obj and decals.obj). For project_point3d only, the decals.c precedent `#define REAL_MATH_EXTERNAL_PROJECT_POINT3D` would suppress the COMDAT. That was not measured and needs an owner decision.
(2) POLICY RULING: the exact form depends on `double t = -plane_distance;` (strip test: `real t` does not match; the `(real)` cast is inert and was dropped).
- Against: docs/common_constants.md limits `double` to genuine API or vararg needs, and the later /Od build passes -plane_distance directly with no 8-byte slot.
- For: owner-landed production precedents with the identical idiom, source/ai/actor_moving.c:2136 `double t = -distance_along_step;` (commit ea507d5b) and source/units/units.c:11087 `double origin_scale` (the throw_grenade_release closure). HCEX PPC (fneg plus fmadds, no frsp) is neutral.
The owner or orchestrator must admit or reject the double-t idiom here.

## reopen_criteria
If the `double t` idiom is rejected: the best non-double candidate is P02 (scratch/w/structures/p02_body.txt; /Od-attested, 432/11 [sha], 6 insertion/deletion rows in alndiff, zero credit). Reopen it only with a source construct that creates a no-code node between each lazy dup of t and its fmul, i.e. a per-use conversion, without a double variable. The object link blocker closes when the owner converts the path_obstacles.c and decals.c NODUP hand copies to header inlines, or otherwise lets structures.obj's SELECT_ANY copies pair-link. Then re-run provider_link.py and object_audit.py on the applied tree.

## task notes
The resume found almost nothing banked: LEDGER.md had only a header, and the v5.c/v5_body.txt/splice.py/od_cone.txt files were all that survived. Every step since has been appended to the ledger (P01-P03, lab oracles D1/D4/H/U/L5/L6/VA/TG1-3, C1/C2, F01-F06, audit, outcome).\n\nThe lever is new for this unit: a double-typed simple variable passed as a real inline argument. The inliner then substitutes a per-use conversion, which occupies an x87 slot and emits no code. This moves the integer ops one slot later after each lazy `fld st(0)` dup. The same mechanism explains the units throw_grenade_release closure and the actor_moving `double t` fidelity landing.\n\nCorpus tools (dupscan.py, dupctx.py, sepctx.py in the slug dir) list other January functions with the adjacent-dup form. The non-exact ones are actions/actor_moving/projectiles/bipeds/vehicles/collision_bsp/convex_polygon2d_clip_to_plane; see dupscan.txt. They may be candidates for the same lever once the owner rules on it.\n\nNo tracked file was edited and no ninja or git-mutating command was run. `git apply --check` was used on the patch only.
ledger: C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/structures/LEDGER.md
