# source/physics/collision_bsp

## decision
OWNER

## est_probability
0.05

## route
The three residuals split two ways. _collision_surface_test_sphere (880 B; frame 0x2c vs 0x24) is exact only with the unit-local SSE fast_distance_squared3d __asm macro, which the owner held on 2026-09-15: needs a ruling or authentic source. _bsp3d_test_sphere_recursive and _bsp3d_test_pill_recursive are RETIRE: they share an unexplained x87 term-order parity bit, and the pill also has a class-4 int/x87 interleave. The object also needs the RESERVED Codex P1 packet for _cross_product2d.

## why_new
Nothing new. The geometry _plane2d_distance_to_point NODUP repair from r1 wave4 has landed (link_fail now lists only _cross_product2d).

## prior_negatives_checked
r1 wave4 collision_bsp_* (5 result files plus 2 reviews); collision_bsp_obj_opus5_150k_w2_20260914.md (SSE intrinsics i1/i2 measure 928 B, scalar C and inline refuted); Lane B collision_bsp_REPORT.md (50+ parity labs); collision_bsp_obj_sphere_cluster_20260904.md.

## notes
27/30. Even on an asm yes, the two bsp3d parity residuals remain. The object is realistically not completable this round.