# Shared-header evidence audit, round 2

Baseline: `012f7283230d13be8427ef7e93f73f3ef686111f`, 2026-09-25.
Read-only evidence review: no source/config changes, no compiler runs, no credit.
The independent review was read before its worker report.

## Decision-relevant result

There is **no newly established, policy-cleared compensating declaration fix**
for `_rasterizer_frame_statistics_draw` in this audit. The old zero-regression
PA packet is not a new breakthrough and remains held. A fresh broad-owner
header experiment is worthwhile because canonical context changed, but may not
select duplicate declarations by the resulting bytes.

One previously unmentioned source witness weakens the assumption that the
stack-walk declaration must belong in `cseries_windows.h`; it does not by itself
identify the original header. Details below.

## Latest holds supersede the early worker recommendation

`docs/object_matching_logs/claude_halo_r3_20260924.md:171-173` records:

- 3a: stack-walk duplicate choice **held**, header ownership unproved.
- 3b: bitmap-delete route **held**: neither the zero-loss route contradicting
  HCEX ownership nor an ownership-correct route losing exact rows is approved.
- 3c: the `valid_real_plane3d` COMDAT itself is allowed under the existing
  narrow rule, but that does not clear 3a/3b or confer collision credit.

The independent review is
`research/fifty_objects_20260925/results/wave1/REVIEW__source_physics_collisions.md`.
It rejects the focused `real_math_planes.h` placement and inner-block `position`
as steering. `/Od` has `position` initialized at function entry and reassigned
in the same slot. Keep function scope. Its reviewed positive finding is the
visible, noncapturing callee-body escape effect, not the rejected workaround.

## Exact current source locations and historical corrections

| Item | Current source | Evidence / restriction |
|---|---|---|
| `valid_real_plane3d` | prototype `source/math/real_math.h:832`; body `source/math/matrix_math.c:1073` | January inlines it at the loop assert but calls it out of line elsewhere; `/Od` call at `0x7A5833` to `0x6C7730`. Broad owner-header inline plus provider-body removal is the genuine route. |
| `bitmap_delete` duplicates | `source/bitmaps/bitmaps.h:27`, `source/bitmaps/bitmaps_internal.h:24` | Public `bitmaps.h` is HCEX-attested; `bitmaps_internal.h` is a campaign invention and itself includes the public header. Ownership favors removing the internal duplicate, not the public declaration. |
| `stack_walk_disregard_symbol_names` duplicates | `source/cseries/cseries.h:240`, `source/cseries/cseries_windows.h:62` | Both are campaign-authored. Neither binary symbol ownership nor implementation file name proves the original header. |
| `real_local_random` | `source/math/real_math.h:1662` | Still direct-return form; old PA used the separately evidenced named-result form with the plane inline. This is not a new proposal. |
| scalar nonuniform spline | `source/math/real_math.h:1722` | Already repaired and assert-attested in canonical. Do not schedule the stale scalar-spline repair. |
| vector nonuniform spline | `source/math/real_math.h:1784` | Still manual `v10..v20` expansion; independently evidenced genuine helper form exists, but prior full-header combinations regress canaries. Not a free count fix. |

The early review says either bitmap duplicate removal restores `_bitmap_copy`.
That does **not** mean the alternatives are board-equivalent. The later canary
ledger F8/F9 reports that removing the internal copy also loses
`_unit_preprocess_node_orientations` (1,920 padded bytes), whereas deleting the
public copy avoids that loss. The latter byte-selected choice is precisely the
held one.

## Source witness for stack-walk ownership: useful but not decisive

User-supplied source:
`C:/Users/isabe/Downloads/haloleak2024/halo1/random_math.c`.
Fresh SHA-256 verification:
`3d4a67b28efc9f6e9b33ed83efee72d27a90672737fcad4648fecc69a16d5680`,
equal to `docs/user_source_reconstruction_map_20260906.md`.

It includes `cseries.h`, `real_math.h`, `geometry.h`, `game_engine.h` at lines
5-8, then `game.h` at line 104 inside `DUMP_RANDOM_SEED_DEBUG_INFO`. It calls
`stack_walk_disregard_symbol_names` at lines 125/127 and `stack_walk_with_context`
at 126, with **no explicit `cseries_windows.h` include**.

This is genuine evidence against treating the platform header as an automatic
owner merely because the implementation is `stack_walk_windows.c`. However:

1. The diagnostic branch is disabled in the supplied source, so successful
   normal compilation does not establish that these calls had prototypes.
2. C89 could permit implicit declarations in this branch.
3. The original included headers are absent; a transitive include remains
   possible, and present reconstructed headers do not settle it.
4. The supplied source is cross-build evidence with the provenance limits
   already recorded in the source-map document, not the original January header.

Thus this witness supports investigating the common `cseries.h` API, but **does
not lift the owner hold** or authorize choosing the public copy to cancel a
compiler count shift.

## Independently evidenced future header content, not new counting recipes

`research/fifty_objects_r2_20260924/w/real_math_h_recovery/LEDGER.md` and
`january_real_math_map.md` preserve January helper-batch and cross-build evidence
for `vector_intersect_plane2d`, `valid_real_vector2d`, and `valid_real_normal2d`
as shared-header inline candidates. Their current bodies remain at
`source/math/geometry.c:544`, `source/game/players.c:1065`, and
`source/ai/actor_looking.c:998`. Their presence/bytes are stronger evidence than
the exact source-line placement; do not infer the latter from a favorable count.

The old P2 combined January tier is already measured and held with canary losses.
Do not relabel it as a new solution. Similarly, the 16 public function names in
the old inventory prove symbols exist, not that all 16 prototypes belonged in
January `real_math.h` at a selected location.

Two corrections matter when reading old accounting:

- The six frame-statistics layout-check typedefs consumed **17** C1 records,
  not six. The recovery ledger explicitly corrects the canary F9 calculation.
- `/Od` floating spills can be SSE/x87 conversion temporaries, not named C
  locals. A slot alone does not authenticate another local in a header helper.

## Recommended bounded continuation

1. Replay only the genuine plane-inline/provider/body-call packet on current
   canonical with untouched controls; retain function-scope `position`.
2. Diagnose actual current regressions, not the historical pair by assumption.
3. If bitmap cleanup is included, use the genuine public header and treat any
   units regression as part of the packet to explain independently.
4. Keep the stack-walk duplicate unchanged absent stronger provenance or an
   explicit ruling. The supplied-source witness is a lead, not closure.
5. If no authentic zero-loss package emerges, preserve exact receipts and the
   newly isolated compiler effect. Do not replay dummy-name/header-position
   sweeps or declare all legitimate source impossible.

Production was untouched; this audit makes no new-byte claim.
