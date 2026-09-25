# source/math/geometry

## decision
OWNER

## est_probability
0.02

## route
Function-level owner items: (a) polygon2d, +1008 bytes. Lane D B3 is EXACT with the /Od 0x6bf170 body `dot_product2d((real_vector2d *)point, &plane->n) - plane->d`. Since geometry's local copy was removed (round-1 provider repair, landed), this is now a real_math.h header view cast: structural owner packet 20260923 decision (b), real_math.h reserved. R18 lab: collision_bsp shows 0 section differences with the header body. (b) hull3d_expand, +1808 bytes: h3e_c1 is EXACT only with the /Od pointer-before-range-check order, the January-bug/UB owner class (owner packet s4). Link blockers: _cross_product2d (owner queue item 1, P1, Codex-reserved) and _plane2d_from_points (decals NODUP; decals is closed for this lane).

## why_new
Checked whether polygon3d has a caller-local route: /Od 0x6bd8a0 calls 0x56d580(plane, point), a real plane3d_distance_to_point call. So a caller-local dot_product3d spelling without the header's outer parentheses is NOT attested. The polygon3d tie stays header-bound.

## prior_negatives_checked
Lane D REPORT (claude-lane-d-refresh-20260922/scratch/lane_d/w/math__geometry): polygon3d C2 is EXACT only with point_from_line3d plus the v3 header outer-paren removal, which LOST _decals_delete_permanent_from_cluster, _item_accelerate and _rasterizer_frame_statistics_draw. p3qD/p3eD M8 sweeps and v3a/v3b were inert. Also read: geometry_obj_opus5_150k_w3 rejected shapes, claude_collision_geometry_20260924, round-1 wave1/wave4 provider repairs and reviews.

## notes
Object completion needs 2 rulings, P1, a decals provider repair AND the polygon3d header-paren tie, which trades exact rows. Not completable this round. The rulings alone would gain +2,816 bytes of function credit.