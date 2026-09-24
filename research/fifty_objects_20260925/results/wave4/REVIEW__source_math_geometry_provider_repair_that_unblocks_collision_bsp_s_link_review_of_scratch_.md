# REVIEW source/math/geometry (provider repair that unblocks collision_bsp's link) - review of scratch/w/collision_bsp/geometry_provider_repair.patch

approve: True

## per_object
[
 {
  "unit": "source/math/geometry",
  "approve": true,
  "issues": "Byte-inert everywhere: the only object change is `_plane2d_distance_to_point` COMDAT selection NODUP->ANY. Still 27/3 exact; the 3 parked residuals are unchanged and the parks stay valid. Pre-existing and out of scope: provider_link FAIL(2) (cross_product2d P1, plane2d_from_points decals hand copy), and vector_intersect_plane2d hand-expands the helper."
 },
 {
  "unit": "source/physics/collision_bsp",
  "approve": true,
  "issues": "Not modified by this patch. Its latent LNK2005 against geometry on `_plane2d_distance_to_point` is removed; the pair link with cand.obj is clean in both orders, and st1.obj + cand.obj is also clean. The object is still not complete: 3 residual .text functions and the held `_cross_product2d` P1 conflict."
 }
]

## checks
All notes are in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review4_geometry__provider_repair_that_unblocks_collision_bsp_s_link_\REVIEW.md (R1-R20). The tree was clean at 16542e46 for the whole review.

R1 Patch applies and round-trips: git apply --check passes for the geometry patch alone and together with collision_bsp production.patch. HEAD blob + patch == worker geom/g1.c byte-for-byte (sha256 192ee6c3...).

R2 Header body: the real_math.h:1301 `__inline` body text is identical to the deleted hand copy. geometry.h has no prototype. The only users are geometry.c lines 712, 1166 and 1171 and collision_bsp.c lines 1435 and 1995.

R3 gate.py --all: HEAD and candidate are both exact 27 / residual 3 / unwritten 0, and the two outputs are identical.

R4 Full object comparison (slug objcmp.py: section_infos_equal, raw bytes, flags, COMDAT selection, and every symbol's storage/section/value). head.obj vs cand.obj has exactly ONE difference: `_plane2d_distance_to_point` selection changes from NODUP (1) to ANY (2); bytes and storage are unchanged. Production geometry.obj == head.obj.

R5 January's 32 bytes appear in only one split object board-wide (geometry).

R6/R7 Link order and pair links: collision_bsp (0x136C00) and bsp2d (0x136590) both follow geometry (0xF4F00) in January's link order. Production collision_bsp.obj linked with production geometry.obj (or head.obj) gives LNK2005 in BOTH orders, so the current tree has a latent defect. Linked with cand.obj (and with the prior reviewer's st1.obj), there is no LNK2005/LNK1169 in either order.

R8 provider_link (slug copy): identical for head and cand. It FAILs on 2 pre-existing items: cross_product2d (held P1) and plane2d_from_points (decals NODUP hand copy).

R9 object_audit: identical for head and cand (3 residual .text DIFFs, 90 January symbols with 0 differences) and equal to the worker's audit_g1.txt. The 3 parked.json measurements match cand exactly, so no park goes stale. There are no rejection entries.

R10 Law (i) / owner ruling 2 (all-inlined header-inline COMDATs): the COMDAT is owned by January's geometry object and is already emitted today. `_points_dimension2d` is a strictly exact user of the inline (EXACT 160). The residual caller's source and bytes are unchanged.

R13 Precedent: owner-landed commit 5e81dabf removed the identical rename + hand copy for vector_intersect_plane3d from the same #define block.

R14 Symbol storage: cachebeta lists `_plane2d_distance_to_point` as public, so no symbols.json change is needed.

R15 /W3 /Zs warnings: 13 in both, same set.

R16 objdiff-cli 3.3.1 single-unit projects: identical rows for head and cand (8838/12751 code, 2288/2288 data, 27/30 functions), matching build/report.json.

R17 Surplus identity: unchanged (1 pre-existing plane2d_from_points difference in both).

R18 Adversarial single-body test (lab only, shadow real_math.h in the slug, verified with /showIncludes). I put the /Od-attested body `dot_product2d(point,&plane->n) - plane->d` (from /Od function 0x6bf170) in the header. collision_bsp shows 0 section differences, and its bsp2d sphere/pill functions stay EXACT. In geometry only the already-residual convex_polygon2d_clip_to_plane changes. So one shared header body is consistent with every exact user.

R19 bsp2d_test_point with the /Od helper-call spelling (lab only) is EXACT. It LNK2005s against production geometry but links clean with the candidate.

R20 House-rule / strip-test / section-7-9 scan: the patch only deletes code and adds no decoration. It changes no header, so there is no consumer list. Nothing includes geometry.c.

## issues
No blocking issues. APPROVE scratch/w/collision_bsp/geometry_provider_repair.patch as proposed, with no amendment. It can be applied mechanically:
- `git apply` the patch; no header, symbols.json, config or park change is needed.
- The only change to geometry.obj is the COMDAT selection byte for `_plane2d_distance_to_point` (NODUP -> ANY).
- gate rows, objdiff rows, parks and data stay unchanged.
- It removes the current tree's latent collision_bsp<->geometry LNK2005 in both link orders.

Non-blocking notes:
(1) The worker cites "owner ruling 20260921 #5". The all-inlined header-inline COMDAT admission is ruling #2 in claude_lane_b_HANDOFF_20260920.md section 3 and in the lanes-AB table. Correct the number in any ledger text.
(2) Pre-existing problems this patch does not introduce:
- geometry stays 27/3.
- geometry's surplus provider_link FAILs on `_cross_product2d` (held P1) and `_plane2d_from_points` (decals NODUP hand copy, also not identical).
- geometry.c vector_intersect_plane2d and bsp2d.c bsp2d_test_point hand-expand the plane2d helper; the /Od build shows real calls in bsp2d.
(3) Follow-ups the repair enables, NOT part of this patch:
- bsp2d.c could use its /Od-attested helper call (lab: still EXACT, and links clean only after this repair).
- Lane D's convex_polygon2d_clip_to_plane route needs the /Od-attested header body `dot_product2d((real_vector2d *)point, &plane->n) - plane->d`. That now has to be a real_math.h body packet with a full sweep; the lab shows collision_bsp is byte-inert to it.
(4) January image section order does not decide the question either way. January puts its selected header-inline copies in a mid-object block in every Layer-1 object, while our VC7 emits them at the top. Section order earns no credit.
(5) collision_bsp stays NOT complete: 3 residual .text functions and the held `_cross_product2d` P1 provider conflict.
