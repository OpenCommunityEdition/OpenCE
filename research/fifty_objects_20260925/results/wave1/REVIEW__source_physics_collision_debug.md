# review source/physics/collision_debug
approve=False

## checks
All checks were run independently at worktree HEAD e9e62b78 (clean tree). Notes are in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review_collision_debug\REVIEW.md.

1. gate.py --source (production copy) --all: EXACT 4192 _collision_debug_render, 1/1. The candidate object is identical to build/base (objeq, 36 sections).
2. object_audit: all 17 January sections ok. 29 symbols, 1 differs: _collision_debug_cube_vertices, storage 2 in the split vs 3 in ours. This matches the worker.
3. Storage evidence:
   - pdb_storage: that same single disagreement, PDB-public False.
   - My own grep of cachebeta_publics: every other collision_debug global in this object is a public, and there is no public at 0x0028B2A8.
   - A scan of all 833 build/split and 621 build/base objects finds no reference to _collision_debug_cube_vertices outside collision_debug.obj.
   - contribs.json module 140 has exactly 17 contributions (1 code, the 96-byte cube array at selection 0, 13 literals, .data 16, .bss 28962). There is no hidden code.
4. Split regenerated with only line 19561 changed (byte-level edit in place, 1-line diff), using build/tools/csplit.exe into a scratch folder:
   - Of 833 objects, only collision_debug.obj differs, and its only difference is the storage class of cube_vertices.
   - Audit against the new split: PASS, 29/29.
   - Gate against the new split: EXACT.
   - One-unit objdiff 3.3.1 report: code 4182/4182, data 29278/29278 (the same as the old split). build/report.json is also 100/100, so no semantic_data entry is needed.
5. Surplus:
   - surplus_identity: 4 of 4 identical.
   - My own surplus_all.py compares every January definer (not only provs[0]) on raw bytes and flags: 18 of 18 identical.
   - provider_link: 18/18 PASS in both orders.
   - nodup_census: no NODUP definers.
   - Census of every build/base copy of the 7 helpers: all have the same hash as January's single selected copy.
   - .drectve contains only the standard LIBC/OLDNAMES directives.
6. No parks, rejections or semantic entries exist for this unit. fake_match_scan: 0 leads.
7. Strip tests (whole object compared with objeq): with the add_vectors3d point casts removed, the cube-vertex cast removed, both removed, or 6.2831854820251465 replaced by _pi*2.f, the object is IDENTICAL every time. None of these constructs is buying the match. The add_vectors3d(point, vector, point) call is attested in the /Od build at 0x7b0c5c (target 0x44e8e0 has the add_vectors3d body).
8. Full /Od call census of the render function (od_linear.py 0x7af800..0x7b1ee4, 43 callees) mapped against the production calls. This is where the finding below came from.
9. Fix variant mag3d.c:
   - EXACT, and audit PASS against the regenerated split. objdiff 100/100, fake scan 0.
   - The 3 extra COMDATs (_magnitude3d, _magnitude_squared3d, _square_root) are identical to January's selected copies (action_charge/action_alert) and have no NODUP definers.
   - provider_link: 21/21 PASS.
10. Both patches I wrote apply with patch --binary, and their results are byte-equal to the tested files.

## issues
BLOCKING: the source hand-expands a header helper, and the object would be marked Matching with it.

The /Od first-party build calls magnitude3d in the features block: 0x7b1dc6 calls 0x432950, which calls magnitude_squared3d 0x42df70 and then square_root 0x42e320 / 0x42e2f0. That is real_math.h:1140 magnitude3d applied to &collision_debug_vector. Production instead writes:
`radius = (real)sqrt(collision_debug_vector.i * collision_debug_vector.i + collision_debug_vector.j * collision_debug_vector.j + collision_debug_vector.k * collision_debug_vector.k) * 0.5f + collision_debug_height * 0.5f + collision_debug_width;`

This breaks brief rule 6 ("never hand-expand ... any header helper") and campaign rule 13. It also goes against the owner's 20260924 reconciliation standard: "no ... hand expansion ... admitted". It is left over from the era when helper COMDATs were forbidden. The owner's commit 5671843f converted point_from_line3d, scale_vector3d, add_vectors3d and set_real_vector3d to helpers but missed this one. It is the only helper in the /Od census that is hand-expanded. The fabs/acos/fmod wrappers in the /Od build are artifacts of the later C++ build's <cmath> overloads, not helpers in the source. So the worker's claim "No source change is needed" is wrong.

The fix is measured and admissible:
- The genuine spelling `radius = magnitude3d(&collision_debug_vector) * 0.5f + collision_debug_height * 0.5f + collision_debug_width;` is EXACT.
- It adds only three SELECT_ANY COMDATs, all identical to January's selected copies. 17/36/21 Matching units already emit them.
- provider_link: 21/21 PASS. Audit PASS. objdiff 100/100.

Everything else in the proposal checks out independently. The static symbols.json row is correct, the split regeneration changes only this object, surplus and link pass, and the data is complete.

Minor: the worker's patches/symbols.json.patch also carries rows for units that are BLOCKED (breakable_surfaces, rasterizer_xbox, hardware_geometry). Do not apply it as a whole for this unit.

Non-blocking: the three raw double literals 6.2831854820251465 in the fmod calls are a folded-constant readout that no person would type. `_pi*2.f` (the spelling real_math.h itself uses) gives an identical object; see scratch/w/review_collision_debug/mag3d_twopi.c. That cleanup is optional.

CORRECTED PRODUCTION CHANGES for the integrator, applied mechanically:
(1) config/symbols.json line 19561, edited in place with no re-serialization: `{ "file_offset": 2667176, "flags": 0, "name": "_collision_debug_cube_vertices" },` becomes `{ "file_offset": 2667176, "flags": 0, "name": "_collision_debug_cube_vertices", "static": true },`
    - Patch: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review_collision_debug\symbols.json.collision_debug.patch
(2) source/physics/collision_debug.c: replace the 4-line radius sqrt expression, around line 553, with `radius = magnitude3d(&collision_debug_vector) * 0.5f`. The next two continuation lines stay unchanged. Keep CRLF line endings.
    - Patch: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review_collision_debug\collision_debug.c.patch
    - Post-image: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review_collision_debug\mag3d.c
    - Optional: mag3d_twopi.c instead.
(3) Regenerate the split, then run the full ninja build and the stable sweep. Expect 0 regressions, and collision_debug should stay EXACT 1/1.
(4) Only then change config/config.json line 515 for source/physics/collision_debug.c from "status": "NonMatching" to "Matching".

The admission record should list:
- 7 surplus code COMDATs: _add_vectors3d, _point_from_line3d, _scale_vector3d, _set_real_vector3d, _magnitude3d, _magnitude_squared3d, _square_root.
- 3 SDK tables: _D3DPRIMITIVETOVERTEXCOUNT 88, _D3DSIMPLERENDERSTATEENCODE 328, _D3DTEXTUREDIRECTENCODE 16.
- 11 literals.
- That all of these are identical to January's selected copies and pass the selected-provider link in both orders.
