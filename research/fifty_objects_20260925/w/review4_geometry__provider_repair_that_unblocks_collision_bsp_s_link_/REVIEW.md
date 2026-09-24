# Review 4: source/math/geometry provider repair (unblocks collision_bsp link)
Reviewer slug. Base 16542e46 (git status clean at start).
Proposal: scratch/w/collision_bsp/geometry_provider_repair.patch (drop `#define plane2d_distance_to_point ..._inline`
rename + `#undef` + hand-written body at geometry.c:546; genuine real_math.h:1301 `__inline` used).

## Log
- R1 git apply --check: geometry patch alone OK; together with collision_bsp production.patch OK (HEAD 16542e46, clean tree).
  Working-tree geometry.c (CRLF) == worker geom/base_geometry.c. HEAD blob + patch (GNU patch on LF copy, CRLF'd)
  == worker geom/g1.c byte-for-byte.
- R2 header body: real_math.h:1301 `__inline real plane2d_distance_to_point` body text == the deleted hand copy
  (`(plane->n.i*point->x + plane->n.j*point->y) - plane->d`). geometry.h has no prototype. Only users:
  geometry.c 712/1166/1171 and collision_bsp.c 1435/1995. No other `_inline` rename of this helper anywhere.
- R3 gate (my compiles, PID-isolated): HEAD source (wt copy) and patched source both `exact 27 residual 3 unwritten 0`;
  gate outputs byte-identical (residuals convex_hull3d_expand / convex_polygon2d_clip_to_plane / convex_polygon3d_clip_to_plane, sha).
- R4 objcmp (slug objcmp.py: every non-debug section section_infos_equal + raw bytes + flags + COMDAT selection, and
  every symbol storage/section-key/value): head.obj vs cand.obj = exactly ONE difference: `_plane2d_distance_to_point`
  selection 1 (NODUP) -> 2 (ANY); raw bytes equal, flags equal, storage 2 both. build/base geometry.obj vs head.obj: 0 diffs.
  => every function incl. the 3 parked residuals is unchanged (no park drift); no symbol change.
- R5 January bytes of `_plane2d_distance_to_point` (558bec...5dc3 + 6x90, 32 B) occur in exactly ONE split object
  (geometry) board-wide => no static/per-TU copy anywhere in January; one external definer.
- R6 link-order discriminator: collision_bsp image range 0x136C00 > geometry 0xF4F00, so geometry precedes collision_bsp
  in January's link order; with both copies SELECT_ANY the linker keeps the FIRST (geometry's) and discards
  collision_bsp's => January's single copy in geometry's range is exactly what the header-inline hypothesis predicts
  (had collision_bsp preceded geometry, its copy would have been kept in its own range). Not a contradiction.
- R7 pair links (slug pairlink.py, VC7 Link.Exe, temp exe under slug/tmp): production collision_bsp.obj + production
  geometry.obj -> LNK2005 `_plane2d_distance_to_point` in BOTH orders (latent defect in the CURRENT tree);
  + my head.obj -> same LNK2005 both orders; + my cand.obj -> 0 LNK2005/LNK1169 both orders.
- R8 provider_link (slug copy, temp under slug) for geometry itself: head.obj and cand.obj outputs identical: FAIL(2)
  pre-existing `_cross_product2d` (actor_combat held P1) and `_plane2d_from_points` (decals NODUP hand copy);
  every other geometry surplus PASS. The patch neither adds nor removes geometry surplus.
- R9 object_audit (read-only tool) head vs cand: output identical (3 DIFF = the 3 parked .text residuals; 90 January
  symbols 0 differ) and == worker audit_g1.txt. Parks: the 3 geometry parked.json measurements (target+base size/relocs/
  normalized sha) match cand.obj exactly => no stale-park PROGRESS failure. No object_admission_rejections entry.
- R10 law (i)/ruling-2 conditions: users of the inline in geometry: `_points_dimension2d` (static, line 712) = EXACT 160,
  and `_convex_polygon2d_clip_to_plane` (parked residual; its source text is unchanged by the patch and its bytes are
  unchanged). The COMDAT is January-OWNED by geometry (not surplus) and already emitted today (as NODUP); a strictly
  exact user exists, so the COMDAT is not justified by a non-exact caller (P1 hazard absent).
- R11 citation nit: the worker cites "owner ruling 20260921 #5"; the all-inlined header-inline COMDAT admission is
  ruling #2 in docs/object_matching_logs/claude_lane_b_HANDOFF_20260920.md s3 and the lanes-AB table (#5 there is the
  accessor/parenthesis ruling). Substance correct; fix the number in any ledger text.
- R12 January image order (slug order.py) is NOT discriminating: in every Layer-1 object January places its selected
  header-inline copies as a contiguous mid-object block (e.g. action_charge: after _action_charge_is_leaping) while our
  VC7 emits them at the top of the object; geometry's January order geosphere_dispose, convex_hull3d_verify,
  plane2d_distance_to_point, vector_intersect_plane2d, vector_intersect_plane3d(=already an exact real_math.h inline)
  is compatible with the header-inline reading. Section order carries no credit and the admitted Layer-1 repairs have
  the same top-vs-mid difference.
- R13 same-object precedent: commit 5e81dabf (owner-landed "verified shared-helper ownership") removed the IDENTICAL
  construct for `vector_intersect_plane3d` from this very #define block of geometry.c (rename + hand copy -> header
  inline, now SELECT_ANY, 4 definers, EXACT, objdiff 100%). The plane2d rename dates from 94f4620b (Jul 14, when
  geometry.c held only this one function, so an unused header inline would not have been emitted). The collision_bsp
  helper call landed in f861776b (Sep 13), before provider-link checks existed (Sep 23) => the latent LNK2005 in R7.
- R14 cachebeta public `_plane2d_distance_to_point` at 0xF4FC0 => external storage (2) correct for a header-inline
  COMDAT; symbols.json line 4219 needs no change. No tool/test/config references the rename or the hand copy.
- R15 /W3 /Zs census (slug w3.py): head 13 warnings, candidate 13, identical multisets (no C4013 etc.).
- R16 objdiff-cli 3.3.1 single-unit projects (slug objproj_head / objproj_cand, target build/split): identical
  function and section rows; code 8838/12751, data 2288/2288 (100%), 27/30 fns; equals build/report.json.
  `_plane2d_distance_to_point` 100%. (SELECT_ANY January-owned helpers are credited: e.g. geometry
  `_vector_intersect_plane3d`, action_charge `_real_random` at 100% in build/report.json.)
- R17 surplus identity (inline, head vs cand): 30 candidate-only code COMDATs each; 1 pre-existing non-identical
  (`_plane2d_from_points` vs decals' January copy) in BOTH; unchanged by the patch.
- R18 ADVERSARIAL single-body test (lab only; shadow real_math.h under slug via prepended /I, /showIncludes confirms both
  `real_math.h` and `math/real_math.h` resolve to the shadow; control shadow == no-shadow: 0 section diffs):
  the later first-party /Od build shows plane2d_distance_to_point (0x6bf170) as `dot_product2d(point, &plane->n) -
  plane->d` (od_plane2d_distance_to_point.txt) with callers in geometry (0x6bcdf0 x3, 0x6bf310), bsp2d
  (0x7a2fd0 = bsp2d_test_point) and collision_bsp (0x7aa210, 0x7aa430). With that /Od body in the header:
  collision_bsp = 0 section differences (bsp2d sphere/pill stay EXACT; its COMDAT still == January geometry copy);
  patched geometry: only `_convex_polygon2d_clip_to_plane` (already residual) changes, `_points_dimension2d` and
  `_plane2d_distance_to_point` stay EXACT. => one shared header body is consistent with every exact user in both TUs;
  no evidence that January's geometry and collision_bsp needed different bodies (which would have refuted the premise).
  Follow-up (NOT part of this patch): the Lane D route for `_convex_polygon2d_clip_to_plane` (their B3 needs the /Od
  body) now becomes a real_math.h body packet with a sweep instead of an edit of a geometry-local hand copy.
- R19 bsp2d cross-check (lab only): January bsp2d.obj (0x136590) also follows geometry in link order. bsp2d_test_point
  with the /Od-attested helper call (`plane2d_distance_to_point(&node->plane, point)`) is EXACT (same bytes as today's
  hand expansion) and emits the COMDAT; it LNK2005s against production geometry.obj and links clean with cand.obj
  in both orders. So the repair also unblocks the /Od-attested (non-hand-expanded) spelling in bsp2d.c (follow-up).
- R20 house rules / section 7-9 scan of the diff: deletions only (a rename macro pair and a hand copy of a header
  helper = rule 6 / law (g) violation REMOVED). No new name, cast, local, block scope, prototype, header, float,
  asm, pragma, volatile or data. Nothing to strip-test (no decoration added; the result IS the plainer spelling).
  No header edit => no consumer list; nothing includes geometry.c (build.ninja: geometry.obj only). Parks unaffected.

## Pre-existing, not introduced by this diff (disclosure)
- geometry stays 27/3 (3 parked residuals), provider_link FAIL(2) for its surplus (`_cross_product2d` = held P1,
  `_plane2d_from_points` = decals NODUP hand copy) and 1 non-identical surplus (`_plane2d_from_points`).
- geometry.c `vector_intersect_plane2d` and bsp2d.c `bsp2d_test_point` hand-expand the plane2d helper text (the /Od
  build shows real calls in bsp2d); both exact today; candidates for later cleanup, out of scope here.
- Worker citation "ruling 20260921 #5" should read #2 (all-inlined header-inline COMDAT admission).

## Verdict: APPROVE geometry_provider_repair.patch as proposed (no amendment needed)
- Byte-inert on every geometry section and symbol; the ONLY object change is the COMDAT selection of the January-owned
  `_plane2d_distance_to_point` (NODUP 1 -> ANY 2), which removes the CURRENT tree's latent collision_bsp<->geometry
  LNK2005 (both orders) and matches the owner-landed same-object precedent (5e81dabf) and the Layer-1/2 class.
- Authenticity: January has one copy (geometry), geometry precedes collision_bsp and bsp2d in link order, a strictly
  exact geometry user exists, and the /Od-attested body is consistent with all exact users => header inline.
- Integrator: `git apply scratch/w/collision_bsp/geometry_provider_repair.patch` (sha256 192ee6c3...; checks clean alone
  and with collision_bsp production.patch at 16542e46). No header, no symbols.json, no config/park change. Expect
  build/base/source/math/geometry.obj to differ from today only in that one COMDAT selection byte; gate 27/3, objdiff
  rows, parks and report unchanged. collision_bsp remains NOT complete (3 residual .text + held `_cross_product2d` P1).
