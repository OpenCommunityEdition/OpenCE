# review source/physics/collisions
approve=False

## checks
All checks were run independently. Notes and objects are in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review_collisions\ (see REVIEW.md).

1. Patch application. I built a mirror tree, scratch/w/review_collisions/tree, from `git archive HEAD` (e9e62b78) and applied production.patch with `patch -p1 --binary`. It applies cleanly and keeps CRLF. The three patched files are byte-identical to scratch/w/collisions/final/*, and `diff -rq` against the tracked source shows only those three files.

2. Scope of the change. After the patch, only collisions.c and matrix_math.c include real_math_planes.h, and no .c file includes another .c file. So only those two TUs can change.
   - collisions: the only delta is move_point plus a new `_valid_real_plane3d`. STRICT gained=[_collision_move_point].
   - matrix_math: identical to build/base (all 64 sections, same symbol set).
   - render_cameras: identical.

3. Strict rows (rows.py, the gate.py logic run on a prebuilt object):
   - collisions: 20/20 EXACT. move_point: 4752 bytes, 226 relocations, sha 8b2f2900…117e2c, which equals January's.
   - matrix_math: 35/35.
   - `gate.py --source final/collisions.c` with the tracked headers: 19/20, so the header half is load-bearing.

4. Object audit and surplus:
   - object_audit on cand.obj: PASS, 44/44 symbols. Compared with the base audit, the only changes are move_point going from DIFF to ok and the new surplus `_valid_real_plane3d`.
   - Surplus identity on cand.obj: 21 COMDATs, all identical. The new `_valid_real_plane3d` equals January's matrix_math copy.

5. Provider link:
   - `provider_link.py cand.obj` against the current base providers: FAIL(3).
     - `_plane3d_negate` and `_point_in_sphere`: these two NODUP failures were already on build/base.
     - `_valid_real_plane3d`: a new failure while matrix_math.c still has its NODUP definition.
   - Pair link of cand.obj with the patched matrix_math.obj: PASS in both orders.

6. Data and scoring:
   - report.json data for the unit is at 100% (590/590).
   - The worker's proposed semantic_matches.json entries check out: sizes, relocation counts and normalized shas match January for move_point (4752/226/8b2f…) and object_get_features_in_sphere (544/13/d529…).
   - object_get_features_in_sphere is strict-exact but already scores only 93.81% in report.json today.

7. Other scans. fake_match_scan finds only the matrix_math.c:940 `__asm` lead, which was already there. parked.json and the admission rejections have nothing that affects this packet.

8. Evidence sources:
   - /Od build (od_move_point.txt): `position = *old_position` is set at function entry (0x7A52DC). The same slot, [ebp-0x18], is reassigned from `collision->point` at 0x7A5710. So the /Od build declares `position` at function scope.
   - HCEX.pdb lists `position` among the function-level locals. HCEX records no block scopes, so this is weak evidence.
   - Lane B's REPORT (try_13, try_22 and the m8probe): the 0x17b/0x17f preheader order is a roughly 50% name-count coin flip.

9. Strip tests (my own compiles):
   - Packet headers with the production function-scope `position`: NOT exact. The two REAL regions are the 0x17b/0x17f preheader transposition.
   - Packet headers with the /Od initialiser chain: the same two REAL regions.
   - Packet headers with the loop site hand-expanded again: EXACT, so the loop-site call is byte-inert.
   - Broad real_math.h inline (worker's hdr/) with the /Od-attested function-scope `position`: move_point EXACT. Block scope there is also EXACT.
   - The same broad route loses `_bitmap_copy` (2,784 bytes) and `_rasterizer_frame_statistics_draw` (4,176 bytes). This reproduces the worker's negative result.

## issues
REJECT. Every byte claim reproduces, but the source is not admissible under the house rules and owner rulings.

1. Scope steering against first-party evidence (the main defect). The patch moves `real_point3d position;` into the success-arm block of collisions.c.
   - The /Od build shows `position` as a function-scope local. It is initialised from `*old_position` at 0x7A52DC and reassigned from `collision->point` at 0x7A5710, in the same [ebp-0x18] slot.
   - Lane B landed function scope on that evidence (claude_lane_b_attempt_logs_20260920/collisions_REPORT.md, try_13/try_22).
   - Under the packet's headers, the /Od-attested form fails, with or without the initialiser chain. It fails only on the 0x17b/0x17f preheader order, which Lane B proved is a roughly 50% declared-name-count coin flip.
   - So the block scope makes no difference to behaviour and contradicts the /Od evidence. It exists only to land a count-keyed coin flip that the focused-header change moved. That fails the STRIP TEST: the attested plain spelling no longer matches.
   - Under the January-like broad real_math.h inline, the /Od-attested function scope is EXACT. This confirms the scope revert is compensating for the header choice, not recovering January source.

2. The focused-header placement is the owner-refused workaround.
   - Owner ruling 2026-09-21 (claude_lane_b_HANDOFF_20260920.md §3 item 4; claude_lanes_AB_HANDOFF_TO_LANE_C_20260921.md rule 7 and lines 507-509; claude_lane_b_rejected_hypotheses_20260920.md lines 38-43): declarations belong in the genuine owner header, real_math.h, where the prototype already is at :672. That requires a full sweep and zero regressions, "never a one-consumer header". Choosing a header position because it spares the victims is coincidence-hunting.
   - The patch's own comment gives declaration-count sensitivity as the reason for the placement.
   - real_math_planes.h's charter is "real_math.c plane-intersection routines that real_math.h does not declare". valid_real_plane3d is neither: it is declared in real_math.h and defined in matrix_math.c.
   - matrix_math.c would include the header only to get back its own deleted body.
   - The ruling also requires a "genuine shared-header __inline" before a new COMDAT is admitted. This is not one.

3. The genuine-owner route is held. The broad real_math.h inline, plus the matrix_math definition removal and the loop-site call, keeps function scope and makes move_point exact. But it loses `_bitmap_copy` (2,784 bytes) and `_rasterizer_frame_statistics_draw` (4,176 bytes).
   - `_bitmap_copy` can be restored by removing the duplicate `bitmap_delete` prototype.
   - `_rasterizer_frame_statistics_draw` could not be restored in the worker's name-count sweep from -6 to +30 units.
   - The zero-regression rule therefore blocks it.

4. Other items:
   - The loop-site change to `valid_real_plane3d(&collision->plane)` is fine. It is /Od-attested (one call at 0x7A5833), byte-inert and follows the house rule.
   - The mechanism finding is sound and worth keeping. VC7 treats `&local` passed to an opaque external prototype as an escape, while a visible `__inline` body does not capture.
   - The object remains blocked regardless by the NODUP surplus link failures `_plane3d_negate` (decals.c) and `_point_in_sphere` (path_obstacles.c), which were already on build/base.
   - The object_get_features_in_sphere semantic entry has valid evidence (544/13/d5292109…8d8325; strict-exact, but report.json shows 93.81%). It is independent of this packet and could go through as its own item.

Reopen criteria:
- (a) Find a genuine (not filler) declaration fix in rasterizer_frame_statistics' include set that restores `_rasterizer_frame_statistics_draw` under the broad real_math.h inline. Then land the broad route together with the bitmap_delete duplicate-prototype cleanup, keeping function-scope `position`.
- (b) Or get an explicit owner ruling that either accepts the frame_statistics_draw loss (recorded and subtracted), or admits both the focused placement and the scope that contradicts the /Od build. The request must state every side effect: a new surplus `_valid_real_plane3d` COMDAT in collisions.obj, and matrix_math's COMDAT selection changing from NODUP to ANY.

No production changes approved.
