# observer wave-2 worker ledger (_observer_update_positions)

Tree 931ed8dc. Baseline gate --all (scratch/w/observer/base.obj): exact 25 / residual 1 / unwritten 0.
  residual 1568 _observer_update_positions [size 1648!=1568, relocs 32!=40, sha]
Prior ledgers read: observer_obj_replacement_exact_packet_20260912, opus5_150k_w2/w3, opus5_250k_w3d, research campaign 20260916 (M8).
Prior best body: opus5-150k worktree scratch/workers/observer/pos_a3.c (1568/40, 18 insns x87 rank; hand-expanded axes2 = credibility blocker).
Layer 2 (931ed8dc) landed the conjunction axes2 header body (= w3 "E6"/s1 shape) -> axes2 via header is now the genuine spelling.
Do-not-repeat inherited: delta decl permutations, advance order, !(A&&B&&C), axis/angle split, polar loop tails, (real_vector3d*)position+1.

## Probes
- P0 re-gate at 931ed8dc: a3 (hand-expanded axes predicate) 1568/40 sha, realdiff 18 (hunks 0x27b magnitude, 0x447/0x455 dot, 0x4ab/0x4c8 cross).
  s1 = a3 + genuine header `valid_real_vector3d_axes2(&observer->forward,&observer->up)` (Layer-2 conjunction body, now inlines): 1568/40 sha, realdiff 20.
- M8 instrument (LAB ONLY, dead longs at fn top) on s1: D=1..8 -> 16,18,16,18,20,18,14,20. D=8==D=0 (instrument clean). No D closes;
  the 0x27b normalize-magnitude hunk NEVER moves with D; dot order across D: i,k,j (D0/8), i(a),k,j (D5), k,j,i (others); January k,i,j never appears.
- /Od readout (odbuild fn 0x5302b0, RTC: new_camera_displacements 56 @-0x44, left 12 @-0xac): after the main loop the /Od source has a
  TAIL LOOP: position = observer+0xd0 (reuses position local), parameter_index = 4..<5, forward=[ebp-0x98]=position, up=[ebp-0x9c]=position+0xc,
  if(!valid_real_vector3d_axes2(forward,up)) {cross(up,forward,&left); cross(forward,&left,up); normalize3d(forward); normalize3d(up);}
  position += observer_parameter_real_counts[parameter_index]. Also: snap loop uses a SEPARATE short index [ebp-0x94] (declared after the powers block);
  powers t2..t5 are named locals; value_index [ebp-0x80]; remaining_time [ebp-0x7c]; init order position, velocity, delta, target, a..f, timer.
  The w2 t3-t7 loop tails were measured only with the OLD out-of-line axes2 header (hand-expanded predicate); loop + NEW header is a new combination.
- L1 = a3 main loop + /Od tail loop (position = observer->parameters + 8; for parameter_index=orientation..; forward/up pointer locals; header axes2):
  1568/40 sha, realdiff 14; hunks 0x43b(4) dot, 0x48e(2), 0x4ab(4), 0x4c8(4) cross. The normalize-magnitude hunk 0x27b is FIXED (first time any shape moved it).
- M8 sweep on L1 (lab): D=1..8 -> 8,10,10,8,14,14,17,14; no D closes. F/U first-load table (fu.py): January products
  dot[k,i,j] cross_i[2] cross_j[2] cross_k[2] = Fk Fi Uj | Fk Uk | Fi Fk | Fj Fi ; L1 = Uk Ui Uj | Uj Uk | Uk Ui | Ui Uj (up loaded first everywhere).
- L2 = L1 + /Od-attested main-loop shape: `if (remaining_time > 0.f || !TEST_FLAG(valid)) { short value_index; ... } else { short value_index; snap loop }`
  (the /Od snap loop has its own short at [ebp-0x94], declared after the powers block, i.e. the snap block is the ELSE arm).
  Result 1568/40 sha, realdiff 8: hunks 0x43b(2) dot-k, 0x48e(2) cross-i first product, 0x4ab(2) cross-j first, 0x4d4(2) cross-k second.
  F/U: Uk Fi Uj | Uj Uk | Uk Fk | Fj Uj. Everything else (layout, frame, relocs, magnitude) identical to January.
- Instrument placement (LAB ONLY, L2 base): dead longs after `up` or after `left` in the tail = byte-INERT (G=1..8);
  dead longs anywhere before the tail (fn top, loop body, powers block) or just BEFORE `forward` in the tail shift the tail orders.
  => tail keys hang on the ordinals of the user pointer locals forward/up (and earlier symbols), not on inline temps declared later.
  Across ALL instrument variants the dot k-term never loads Fk first and cross-i first product never loads Fk first (January does both):
  consistent with research X5 (same-base fresh leaves: the operand at displacement 0 from its base loads SECOND) -> in ours forward==position (disp 0).
- Tail spellings on L2 (loop kept): Ta position=&observer->forward.i, Tb (real*)&observer->forward, Tc up=(real_vector3d*)position+1,
  Te left hoisted to loop-body level: ALL byte-identical to L2 (realdiff 8). Inert; do not repeat.
- M3 = L2 main loop + s1 tail (no loop, &observer->forward args): realdiff 24 (magnitude hunk back, +0x5b2/0x5c0).
- M1 = L2 main loop + block tail with pointer locals `real_vector3d *forward = &observer->forward; *up = &observer->up;` (w2 t1 form, now with the
  NEW inlining header): realdiff 8, magnitude FIXED; F/U = Fi Fk Fj | Fk Fj | Fi Fk | Fj Fi (cross 5/6 right; only cross-i 2nd + dot order wrong).
  => the magnitude hunk is fixed by the forward/up POINTER LOCALS (not by the loop).
- L3 = L2 with the /Od pointer-local declaration order (position, velocity, delta, target): identical result to L2 (realdiff 8, same F/U). Inert.
- Tg = L2 with a tail loop WITHOUT forward/up locals (casts of position inline in every helper call): realdiff 18, magnitude hunk returns;
  cross F/U = M1's. Refuted (and not /Od-attested).
- hlab (LAB ONLY helper-body injection via #define rename after includes): control hc3 (same 3-local cross body) == L3; hc2 = /Od cross_product3d body (2 named locals k,j; result->i direct): identical to L3. cross_product3d local count inert here.
- L6 = L3 with `up` declared before `forward` (contradicts /Od slot order; lab info): realdiff 18; D-sweep never reaches January.
  Across L/L6 families the dot k-term is ALWAYS Uk-first and cross-i first product ALWAYS Uj-first (January Fk in both);
  across M1/s1 families Fk wins both but the dot is never k,i,j (k,j,i or i,k,j). January = k,i,j AND Fk-first: no family reaches both.
- L7 = forward/up declared at function scope (assigned in tail): byte pattern identical to L2 and its D-sweep identical to m8L2 => ordinals follow IL first-definition order, not declaration position (consistent with dead-after-up being inert).
- gb (LAB): G dead longs BETWEEN forward and up (L3 base), G=1..7: realdiff 8,10,14,10,12,12,14; dot-k and cross-i-first never flip to Fk. L-family (forward==position) is shape-locked away from January.
- M4 (block, up = forward + 1): realdiff 8 = M1 pattern. Lf1 (loop, up = forward + 1; /Od refutes: /Od computes up from position): realdiff 6, dot j,i,k, cross = M1 pattern; D-sweep 12,8,14,14,20,12,16, never January. January's cross-i pair (Fk first AND Uk first) is reached by no family.
- L2 == L3 == Ta == L7 byte-identical function (sha1 914d5e1c3140acbe, 1568/40); January sha1 812f276668f0c910.
- HCEX PDB (DIA2Dump -sym): observer_update_positions local `struct observer_derivative new_camera_displacements` (+ enregistered param) - agrees
  with the /Od RTC name; `left` from /Od RTC. Names adopted in the candidate (byte-inert).
- F1 = L3 + authentic names (new_camera_displacements per HCEX+/Od; tail reset spelled `position = &observer->forward.i;` = Ta, inert):
  1568/40, realdiff 8, 25 siblings EXACT. STRIP TEST of every real*->real_vector3d* cast in the body (F1strip, casts removed, C4133-class
  warnings only): byte-identical function (sha1 914d5e1c3140acbe) => casts are type-only, byte-inert. Same cast idiom already landed EXACT in
  this TU (observer_find_displacement).
- LAB go/gt: dead locals right after `observer` (G=1..7) or after `timer` reproduce exactly the m8L2 state set (8 states); none is January. L-family exhausted as a count question.
- X1 (loop; forward local, up as inline expression): 20 (magnitude broken) but cross-i pair = January's (Fk Uk); X2 (up local, forward expr): 18; X3 (block; forward local, up=&observer->up expr): 16, cross all F-first; X4: 20. Mixed provenance moves cross-i but breaks the magnitude hunk (which needs BOTH pointer locals).
- LAB X1 D-sweep: D=6 reproduces January's ENTIRE first cross product (Fk Uk | Fi Fk | Fj Fi) but the dot stays j-first/U-first and the magnitude hunk stays broken; no D fixes all.
- Mt1/Mt2 (block tail; forward/up pointer locals defined at FUNCTION TOP, /Od-contradicted): 8 / 6; M-family states only (dot k,j,i or i,k,j, cross-i2 Fj). Not credible vs /Od; not adopted.
STOP RULE: ~20 natural shapes + ~120 lab-instrument variants; residual is x87 commutative-leaf order (4 hunks/8 insns) in the tail axes2 dot + first cross_product3d only.

## Final (wave 2)
- Best credible candidate: scratch/w/observer/cand.c (= F1_full.c; body scratch/w/observer/pos_F1.c), obj cand.obj.
  gate --all (+ --forbid-emitted-symbol _point_from_line3d/_sine/_cosine): exact 25 / residual 1 (_observer_update_positions 1568 [sha] only).
  Size 1568 == Jan, relocs 40 == Jan (relocation multiset equal; only the --source literal naming artifact), frame 0x84 == Jan, 471 == 471 insns.
  normalized sha256 (candidate, --source build) 985364af30c99c4d640d08e280c7903126651e0f83f85997de14475e569294a0 (Jan 8dce5e18..., base 4c164150...).
  object_audit: only the target row DIFF; 49 January symbols 0 differ; surplus list identical to production. pdb_storage: 0 disagreements. fake_match_scan: 0 leads.
- Remaining machine difference (8 insns, 4 hunks, all x87 commutative operand order; F=forward(0xd0..0xd8), U=up(0xdc..0xe4)):
  0x43b dot k-term: Jan fld F.k fmul U.k, ours fld U.k fmul F.k
  0x48e cross(up,forward) i-comp 1st product: Jan fld F.k fmul U.j, ours fld U.j fmul F.k
  0x4ab j-comp 1st: Jan fld F.i fmul U.k, ours U.k*F.i ; 0x4d4 k-comp 2nd: Jan fld F.i fmul U.j, ours U.j*F.i
- production.patch (git apply --check OK) replaces the production body (1648/32, frame 0x90, csmemset + unattested aggregate copy) with F1.
  Integrator must also refresh config/parked.json measurements.base for _observer_update_positions (park stays; function not exact).
- Reopen criteria: a decoded C2 per-leaf key rule for R1 pointer-local leaves that predicts January's AAB k,i,j dot together with the
  (Fk,Uk) cross-i pair; or January-side evidence that forward/up have different provenance in the tail (lab: forward-local + up-expression
  with D=6 reproduces the whole first cross product; block/address forms give the Fk-first dot; no natural form gives both).
