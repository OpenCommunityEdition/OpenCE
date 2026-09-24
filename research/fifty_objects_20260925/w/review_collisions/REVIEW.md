# Adversarial review: source/physics/collisions (verdict FUNCTIONS_EXACT_OBJECT_BLOCKED)
Reviewer lane: claude/fifty-objects-20260925. Base e9e62b78 (worktree clean at start).

## Log (appended as I go)
- Read WORKER_BRIEF, house rules, worker LEDGER.md, production.patch.
- Built patched mirror scratch/w/review_collisions/tree (git archive HEAD source + `patch -p1 --binary` of
  production.patch: applies clean; the 3 files are byte-identical to scratch/w/collisions/final/*; diff -rq vs
  tracked source shows only those 3 files). Only collisions.c and matrix_math.c include real_math_planes.h in
  the patched tree, and no .c includes another .c -> only those 2 TUs can change (include-closure argument).
- Compiled (worker sweep.py, --only) from the patched mirror: collisions DELTA ~move_point +_valid_real_plane3d
  STRICT gained=[_collision_move_point]; matrix_math identical to build/base (all 64 sections section_infos_equal,
  same symbol set); render_cameras identical.
- rows.py (gate-equivalent on the prebuilt obj): collisions 20/20 EXACT, move_point sha 8b2f29007193d3aa,
  226 relocs; matrix_math 35/35 (its _valid_real_plane3d EXACT).
- gate.py --source final/collisions.c with TRACKED headers: 19/20 (header half load-bearing, confirmed).
- object_audit cand.obj: PASS (44/44 symbols); diff vs base audit = only move_point DIFF->ok and +_valid_real_plane3d surplus.
- surplus identity (surplus_cand.py, same logic as surplus_identity.py on cand.obj): 21 surplus COMDATs, 0 not identical;
  _valid_real_plane3d IDENTICAL to January matrix_math copy.
- provider_link.py cand.obj (vs CURRENT base providers): FAIL(3) = _plane3d_negate (decals NODUP), _point_in_sphere
  (path_obstacles NODUP) [both pre-existing on build/base], _valid_real_plane3d (matrix_math NODUP; fixed by the
  matrix_math half). pairlink2.py cand.obj x patched matrix_math.obj: PASS both orders, 0 dup lines.
- HCEX.pdb collision_move_point: `position` listed with function-level locals (HCEX emits no SymTagBlock, so scope
  is not readable there; weak). Names differ from ours (clip_collisions/new_clip_collisions vs clip_indices/new_clip_indices).
- /Od (worker od_move_point.txt, 0x7A52DC..0x7A5325): `position = *old_position` is the 2nd local initialised at
  FUNCTION ENTRY ([ebp-0x18]), and clipped_position is then copied FROM position ([ebp-0x18] -> [ebp-0x40]).
  First-party evidence says `position` is a FUNCTION-SCOPE local. Lane B try_13/try_22 landed function scope on
  exactly that evidence (docs/object_matching_logs/claude_lane_b_attempt_logs_20260920/collisions_REPORT.md), and
  its m8probe proved the 0x17b/0x17f preheader order is a ~50% name-count coin (K=1..31).
- STRIP TESTS (my compiles, mirror variants under scratch/w/review_collisions/var):
  * packet headers + production (function-scope) `position`: move_point NOT exact, 2 REAL = 0x17b/0x17f preheader
    transposition (strip_fnscope.obj).
  * packet headers + /Od initialiser chain (position = *old_position; clipped_position = position; clipped_velocity =
    velocity): same 2 REAL at 0x17b/0x17f (strip_fnscope_odchain.obj).
  * BROAD real_math.h inline (worker hdr/: proto@672 removed, inline after valid_real_normal3d) + production
    FUNCTION-scope position: move_point EXACT (broad_fnscope.obj). Block scope there: also EXACT (broad_blkscope.obj).
  => the block-scope `position` in the packet is not source evidence; it compensates the name-count shift the focused
     header introduces, and it contradicts the /Od attestation. Under the January-like broad placement the /Od-attested
     form is exact without it.
- Strip test, loop site hand-expanded again under the packet headers: move_point still EXACT (byte-inert; the
  `valid_real_plane3d(&collision->plane)` call form is /Od-attested and house-rule correct - no objection to it).
- Broad route re-measured (var/broad: worker hdr real_math.h + patched matrix_math.c + production fn-scope position):
  collisions move_point EXACT, matrix_math identical, but bitmap_drawing LOSES _bitmap_copy (2,784) and
  rasterizer_frame_statistics LOSES _rasterizer_frame_statistics_draw (4,176) - worker's negative reproduced.
- Semantic-entry evidence: January normalized sha256 / relocs match the worker's numbers exactly
  (move_point 4752/226/8b2f2900...117e2c; object_get_features_in_sphere 544/13/d5292109...8d8325, 2 `internal` targets).
  report.json today: object_get_features_in_sphere 93.81% although strict-exact (pre-existing under-credit). Evidence OK
  but moot while the landing is rejected; the features_in_sphere entry is independent of this packet.
- fake_match_scan on the 3 patched files: only the pre-existing matrix_math.c:940 __asm lead (lexical scanner cannot see
  scope steering). parked.json: only matrix_math:_matrix4x3_multiply (bytes unchanged). No admission rejection for collisions.

## Verdict: REJECT (approve=false)

Byte claims are all reproduced (20/20, audit PASS, surplus identical, pair link PASS with the matrix_math half, 0 board
regressions by include closure). The source is not admissible:

1. `real_point3d position;` moved into the success-arm block is steering. The /Od first-party build declares `position` at
   FUNCTION scope (initialised from *old_position at 0x7A52DC, reassigned from collision->point at 0x7A5710, same slot
   [ebp-0x18]); Lane B landed function scope on that evidence. Under the packet's headers the /Od-attested form (with or
   without the initialiser chain) fails only on the 0x17b/0x17f preheader order, which Lane B proved is a ~50% name-count
   coin (K=1..31). The scope choice is semantically inert, contradicts first-party evidence and exists only to re-land a
   coin that the focused header shifted -> fails the strip test ("the plain/attested spelling no longer matches").
   Under the January-like broad real_math.h placement the /Od-attested function scope IS exact, which confirms that
   the block-scope revert compensates for the header choice rather than recovering January source.
2. The focused-header placement is the refused workaround. Owner ruling 2026-09-21 (claude_lane_b_HANDOFF_20260920.md
   section 3 item 4; claude_lanes_AB_HANDOFF_TO_LANE_C_20260921.md rule 7 and lines 507-509): declarations go in the
   genuine owner header (real_math.h, where the prototype already sits at :672) with a full sweep and zero regressions,
   "never a one-consumer header", and "choosing a header position because it spares the victims" is coincidence-hunting.
   The patch comment itself gives declaration-count sensitivity as the reason for the placement. real_math_planes.h's own
   charter is "real_math.c plane-intersection routines that real_math.h does not declare"; valid_real_plane3d is neither
   (declared in real_math.h, defined in matrix_math.c). matrix_math.c would include it only to recover its own deleted body.
3. The genuine-owner broad route (real_math.h inline) is exact for collisions with the /Od-attested scope but costs
   _bitmap_copy (restorable via the genuine duplicate bitmap_delete prototype cleanup) and
   _rasterizer_frame_statistics_draw (not restorable in the worker's -6..+30 oracle). Under the zero-regression rule it
   is held, not landable.

Mechanism finding (keep): VC7 treats &local passed to an opaque external prototype as an escape; a visible __inline body
(even when not inlined at the site) does not capture, so January's collisions TU saw valid_real_plane3d's body.

Reopen criteria: (a) a genuine (non-filler) declaration fix in rasterizer_frame_statistics' include set that restores
_rasterizer_frame_statistics_draw under the broad real_math.h inline, then land broad route + duplicate-prototype cleanup
for bitmap_drawing + matrix_math def removal + loop-site call, KEEPING function-scope position; or (b) an explicit owner
ruling that accepts the frame_statistics_draw loss (recorded and subtracted), or that admits the focused placement AND the
/Od-contradicting scope - state every side effect (+surplus _valid_real_plane3d COMDAT in collisions.obj, matrix_math
COMDAT selection NODUP->ANY) in the request.
