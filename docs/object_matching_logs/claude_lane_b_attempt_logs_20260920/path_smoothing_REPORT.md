# path_smoothing worker report (Lane B)

Baseline gate: `== exact 5  residual 1  unwritten 0  (of 6 listed)`; `_find_tangent_point` 336 [sha].
January facts: frame 0x24, 2 rets, 129 insns. Only real diff: J `fld [i]; fmul st(3)` (tl stays on x87, popped by 2x `fstp st(0)` after tp[1].x)
vs ours `fxch st(2); fmul [i]` (tl consumed), + `fxch st(1)` before `fsub st(1)`, one `fstp st(0)`.
/Od build fn 0x4cd3c0 (od_find_tangent_point.txt): vector_from_points2d + magnitude_squared2d + square_root + 2x set_real_point2d(&tangent_points[n], x, y)
+ 2x vector_from_points2d(point,&tangent_points[n],&tangent_vectors[n]) + cross_product2d(&tv[0],&tv[1]); else: radius_vector + normalize2d + point_from_line2d.
RTC locals: center_to_point(8) tangent_points(16) tangent_vectors(16) radius_vector(8).

## S1 set_real_point2d x2 (from /Od) - try_1.c
Evidence: /Od calls 0x4c9b10 (p,x,y) twice. Gate before/after: residual 336 [sha] / residual 336 [sha]; alndiff identical to baseline. Verdict: inert.

## DIAG (not landings): mechanism of the residual
- dsweep.py: D=0..8 dead locals inserted (a) before all locals, (b) between center_to_point and tangent_length: all 9+9 byte-identical to baseline (6 diff rows). => M8-INSENSITIVE (frame aggregate x frame scalar leaf pair).
- diagA: extra later use of tangent_length (so it is NOT dying at i*tl): ours emits `fld st(2); fmul [ebp-0x14]` = (tl, i). January is `fld [i]; fmul st(3)` = (i, tl).
  => residual is the commutative operand ORDER of the displacement-0 leaf product c2p.i*tl (ours tl-first, January i-first), NOT an x87 lifetime/dying tie. Prior "stack-lifetime tie" label is refuted; j*tl is (j, tl) in both builds (tree-shape ordered).

## S3 full /Od-faithful shape - try_3.c == candidate.c  ** EXACT **
Evidence (/Od fn 0x4cd3c0): vector_from_points2d(center,point,&center_to_point) [call 0x42e350]; magnitude_squared2d(&center_to_point) [0x42df20];
locals order c2p[-0x10], distance_squared[-0x18], ratio[-0x1c], tangent_length[-0x20]; square_root [0x42e320]; 2x set_real_point2d [0x4c9b10];
2x vector_from_points2d(point,&tangent_points[n],&tangent_vectors[n]); cross_product2d(&tv[0],&tv[1]) [0x455250] > 0.0f != clockwise used directly as the index
(no named cross/index locals: only temps -0x64/-0x68/-0x6c); else: normalize2d [0x42e020] + point_from_line2d(center,&v,radius,tangent_point) [0x444020].
January confirmation: frame 0x24 = tangent_points(16)+5 cells, else-arm builds the normalize2d vector in the SAME cells [ebp-0x14/-0x10] the then-arm reads i/j from
(so one vector variable in January; /Od `radius_vector` not adopted), cross = tv1.j*tv0.i - tv1.i*tv0.j (0xbc..0xc2) = cross_product2d(&tv[0],&tv[1]), 2 rets, _normalize2d REL32.
Gate before: residual 336 [sha]; after: `EXACT 336 _find_tangent_point`, `== exact 6  residual 0  unwritten 0  (of 6 listed)`. unitcheck gains=1 regressions=0.

## S5 strip set_real_point2d from S3 (explicit tangent_points[n].x/.y stores) - try_5.c
Gate: residual 336 [sha] (exact 5). newsyms PASS. Verdict: set_real_point2d is LOAD-BEARING (strip test: plain spelling does not match). Rejected.

## S4 S3 + /Od-named `real_vector2d radius_vector` local in the else arm - try_4.c == FINAL candidate.c  ** EXACT **
Evidence: /Od RTC descriptor `[ebp-0x5c] size 8 radius_vector`, used by vector_from_points2d/normalize2d/point_from_line2d in the else arm.
January: frame stays 0x24 and all 336 bytes match (VC7 overlaps it with center_to_point's cells [ebp-0x14/-0x10]); my earlier "frame refutes it" guess was WRONG.
Gate: `EXACT 336 _find_tangent_point`, `== exact 6  residual 0  unwritten 0  (of 6 listed)`. Both S3 and S4 exact -> chose the first-party-attested naming.

## Strip tests on S4
- S6 (try_6.c) top helpers stripped to hand-spelled subtraction/squares: residual 336 [sha] -> vector_from_points2d + magnitude_squared2d are LOAD-BEARING.
- S7 (try_7.c) tail helpers stripped (hand-spelled tangent_vectors + cross): still EXACT -> tail helpers byte-inert; kept because /Od shows them as real calls
  (manual expansion of an available helper is inadmissible).
Lever = the two /Od-attested helper sites (top pair + set_real_point2d); each alone is insufficient (S1: set_real_point2d alone inert; S5/S6: either missing -> residual).

## FINAL CHECKS on candidate.c (= try_4.c)
- gate --all + guard: `== emitted-symbol guard passed (1 forbidden names checked)`, `== exact 6  residual 0  unwritten 0  (of 6 listed)`.
- unitcheck: gains=1 regressions=0 other=0.
- fake_match_scan: 0 review leads.
- newsyms: **OWNERSHIP FAIL** - candidate newly DEFINES `_set_real_point2d` (.text inline COMDAT); January split path_smoothing.obj does not define it.
  Context (comdat_census.py, read-only): January's image owns `_set_real_point2d` in path_obstacles.obj (symbols.json file_offset 328720; first user in link order);
  9 other landed build/base objects already emit the same unreferenced COMDAT copy (real_math, breakable_surfaces, collision_bsp, collision_features, render_debug,
  leaf_map, structures, structure_lens_flares, vehicles), and this unit's BASELINE already emits 9 sibling inline COMDATs January's split lacks
  (_set_real_vector2d, _vector_from_points2d, _magnitude_squared2d, _cross_product2d, _point_from_line2d, _normalize2d, _magnitude2d, _scale_vector2d, _square_root).
  It is the systemic link-folded header-inline class, not a _point_from_line3d-style owner. No .c-level way to call the helper without the COMDAT (header is frozen).
  NEEDS ORCHESTRATOR/OWNER RULING before landing. Fallback with newsyms PASS = baseline (no change); try_5.c passes newsyms but is NOT exact.
