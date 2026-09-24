# source/physics/collisions

## verdict
FUNCTIONS_EXACT_OBJECT_BLOCKED

## newly_exact_functions
[
 "_collision_move_point"
]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\collisions\\production.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\collisions\\final\\collisions.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\collisions\\final\\matrix_math.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\collisions\\final\\real_math_planes.h",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\collisions\\LEDGER.md",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\collisions\\sweep.py",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\collisions\\gate_local.py"
]

## production_changes
Apply C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\collisions\production.patch with `patch -p1 --binary`. It is verified to apply cleanly to e9e62b78 and keeps CRLF line endings. The resulting files are identical to scratch/w/collisions/final/*. The patch changes three files and leaves real_math.h UNTOUCHED:
(1) source/math/real_math_planes.h: append the `__inline boolean valid_real_plane3d(real_plane3d const *plane) { return valid_real_normal3d(&plane->n) && valid_real(plane->d); }` definition, with a comment, after the line_from_planes3d prototype. The real_math.h:672 prototype stays.
(2) source/math/matrix_math.c: add `#include "real_math_planes.h"` after `#include "real_math.h"`, and delete the plain `boolean valid_real_plane3d(...)` definition at lines 1073-1079. This half is mandatory. With the current NODUP definition, the new collisions COMDAT gives LNK2005 in both link orders. With the patch, matrix_math.obj is byte-identical in every symbol section and its _valid_real_plane3d COMDAT becomes selection ANY.
(3) source/physics/collisions.c: at the 0x3CE assert, replace the hand-expanded `valid_real_normal3d(&collision->plane.n) && valid_real(collision->plane.d)` with `valid_real_plane3d(&collision->plane)`. This is byte-inert, keeps to the house rule and is /Od-attested. Also move `real_point3d position;` from function scope back into the success-arm block, ahead of new_clip_indices. This reverts Lane B's try_13 scope move and is load-bearing.
After the ninja build, check build/report.json. _collision_move_point has 5 internal jump-table relocations, like the strict-exact _object_get_features_in_sphere, which objdiff 3.3.1 already under-credits at 93.8%. If either stays below 100%, add switch-label semantic_matches.json entries:
- move_point: 4752 padded, 226 relocations, normalized sha256 8b2f29007193d3aacd830e10fa99886acd6a3bc1e9cd604483343340f6117e2c.
- object_get_features_in_sphere: 544, 13 relocations, d5292109fb39f3a5537512cc61c6863cac9580ef6f2783e92cc40044e30d8325.
No symbols.json or config.json change. No park to retire, because move_point is unparked and has no admission rejection.

## evidence
Baseline at e9e62b78 was 19/20. move_point had 2 REAL regions: the single Lane B transposition at 0x595/0x598 inside the struct copy `clip_plane = collisions[new_clip_indices[0]].plane`. January's order is L0 S0 L1 S1 L2 L3 S2 S3; ours was L2 S2 L3 S3.

MECHANISM, from oracle probes on the real function:
- Remove the case-1 `valid_real_plane3d(&clip_plane)` call (pg): the copy flips to January's order.
- Route the same address to a TU-visible helper (pg3/pg5): January's order again. pg5 leaves 0 REAL regions.
- Keep the external callee but pass a member address (pg4): no change.
- Conclusion: VC7 treats &clip_plane passed to an OPAQUE external prototype as an escape, so pointer load L3 cannot pass store S2. A callee whose body is visible does not capture.

DOUBLE ATTESTATION that January's valid_real_plane3d was an inlinable function:
(a) At 0x485-0x4E8 January INLINES its body at the in-loop assert: magnitude_squared-1 fabs<eps, then the d exponent test.
(b) At the same site the /Od build makes ONE call, valid_real_plane3d(&collision->plane) at 0x7A5833, to fn 0x6C7730, whose body is `valid_real_normal3d(&p->n) && valid_real(p->d)`.
(c) January calls it out of line at case 1 and at render_camera_mirror. The copy order at 0x57e requires a non-capturing, visible callee.

PACKETS TESTED with a local mirror-tree board sweep (sweep.py; control mirror identical to build/base):
- Broad real_math.h inline, the January-like placement: collisions 20/20, but LOSES _bitmap_copy and _rasterizer_frame_statistics_draw. This is position-inert across 4 positions; k-variants are worse.
  - _bitmap_copy is restorable by dropping the duplicate bitmap_delete prototype.
  - frame_statistics_draw is not restorable: no count from -6 to +30 name-count units works.
- FINAL focused-header packet: **447 TUs compiled, 446 identical to build/base. The only delta is collisions: +_collision_move_point strict and +surplus _valid_real_plane3d. ZERO regressions.**

CANDIDATE collisions.obj:
- 20/20 strict; move_point has 0 REAL regions and sha equals January's.
- object_audit PASS (44/44 January symbols).
- pdb_storage: 0 disagreements.
- surplus_identity: all identical, including the new _valid_real_plane3d, which is section_infos_equal to January's matrix_math copy.
- Pair link of packet collisions with packet matrix_math: PASS in both orders.
- report.json data: .data, .rdata and .bss at 100% (590/590).
- fake_match_scan: 0 leads on the changed collisions.c and real_math_planes.h.

STRIP TESTS:
- Header half removed: 19/20.
- Function-scope `position`: 19/20 (the count-keyed 0x17b/0x17f preheader order).
- Hand-expanded loop site: still 20/20, so byte-inert.

## blockers
Pre-existing and unrelated to this packet, since they also fail on the current build/base collisions.obj. scratch/tools/provider_link.py FAILs (LNK2005 in both orders) for two surplus COMDATs whose January providers are NODUP hand copies:
- _plane3d_negate: plain definition at source/effects/decals.c:896, provider decals.obj.
- _point_in_sphere: plain definition at source/ai/path_obstacles.c:191, provider path_obstacles.obj.
This is the known NODUP-provider category and needs an owner decision. decals.c is closed for this lane.
Secondary: objdiff 3.3.1 under-credits the jump-table functions (see production_changes).
Disclosure for review: the `position` scope choice is keyed to the declaration count. Under this packet block scope is exact. Lane B chose function scope under the old header, and the later /Od build's RTC shows function scope, which is weak evidence from a later revision.

## reopen_criteria
The object becomes OBJECT_COMPLETE_CANDIDATE when the owner rules on the NODUP hand copies. The pattern is the one this packet applies to matrix_math: delete the plain definitions in decals.c:896 and path_obstacles.c:191 so the header __inline provides the COMDAT. Then provider_link.py source/physics/collisions must pass for all surplus symbols, with the full sweep still at 0 regressions. The semantic_matches entries are also needed if objdiff stays below 100% on move_point and object_get_features_in_sphere.
If the owner prefers January's layout (inline in the broad real_math.h), that route first needs a genuine missing-declaration recovery for rasterizer_frame_statistics.c. Its draw function is exact only at the current single-point name count, and the broad-header variant shifts it; see LEDGER step 6.

## task notes
New general law, worth a memory entry. VC7 treats `&local` passed to an OPAQUE external prototype as an escape: a later pointer load cannot be hoisted above a store to that local. A callee whose BODY is visible in the TU does not capture, even when that call site is not inlined.\n- Tell: January hoists the last load of a struct copy above the previous store (L2 L3 S2 S3) where we keep L2 S2 L3 S3.\n- Census tool: scratch/w/collisions/copyscan.py. January has 15 such 4-dword copies and many 3-dword ones. Only move_point, actor_look_update, draw_bitmap_in_rect and the two biped functions differ in copy order from our build.\n- Candidate lever for other residuals whose January callee was a header inline that the reconstruction declares only as a prototype, such as the NODUP hand-copy helper family.\n\nTools for header packets, which never touch tracked files:\n- scratch/w/collisions/sweep.py: mirror-tree sweep of all 447 TUs, about 2-3 minutes at 4 jobs.\n- mkmirror.py\n- gate_local.py: compile with header overrides placed first on /I. The override must shadow both \"math/real_math.h\" and \"real_math.h\".\n- objdelta.py, cmp_obj.py, pairlink.py, comdat_sel.py.\n\nThis also confirms the owner's earlier real_math_planes.h precedent. Adding a body to the broad real_math.h regresses name-count-fragile TUs (bitmap_copy, frame_statistics_draw), and a focused header avoids that.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\collisions\LEDGER.md
