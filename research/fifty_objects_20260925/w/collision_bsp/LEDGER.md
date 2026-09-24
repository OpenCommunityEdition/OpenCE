# collision_bsp worker ledger (wave 4, base 16542e46)

## 0. Baseline
- base.c == source/physics/collision_bsp.c at 16542e46.
- gate --all: exact 27 / residual 3 / unwritten 0.
  - residual 1504 _bsp3d_test_pill_recursive [sha]
  - residual 800 _bsp3d_test_sphere_recursive [sha]
  - residual 880 _collision_surface_test_sphere [size 848!=880, sha]
- Prior ledgers read: waveD 20260914, 150k w2 20260914, lane B REPORT + rejected hypotheses 20260920.
- Owner-held: collision_surface_test_sphere SSE __asm macro (20260915 "leave parked for now"); no new __asm in production.
- Recorded negatives (do not repeat): operand/argument spelling of dot/plane-distance helpers; point_from_line3d
  (guarded; physics collision_bsp not in the 17-object list?); early fabs locals; named center pointer; dot before distance;
  HCEA result->t > t; dead locals D=1..8 at fn top (M8 instrument); assert form variants; && vs nested if; same-size casts
  (unsigned long plane_stack); S2 named distance; S3 local projection_from_vector3d copy; S4 LONG_MAX|LONG_MIN; P1-P3 pill.

## 1. bsp3d_test_sphere_recursive
- Residual at base: ONE pair: leaf plane-distance dot at T 0x1e7..0x1f3 January loads j-term first (fld [eax+4]; add esp,0xc;
  fmul [edi+4]; fld [eax+8]; fmul [edi+8]) where ours loads k first. Everything else (287/287 insns, 22 relocs) equal.
- /Od 0x7aafe0 readout (od_sphere3d.txt): leaf = distance=plane3d_distance_to_point(plane,data->center) (real call);
  point_from_line3d(data->center,&plane->n,-distance,&center_projected_on_plane [RTC 12B]);
  data->projection_axis=projection_from_vector3d(&plane->n); int tmp=(designator&LONG_MIN)?1:0;
  data->projection_sign=projection_sign_from_vector3d(&plane->n,data->projection_axis)!=tmp; project_point3d(&cpop,axis,sign,&data->center2d);
  bsp2d_test_sphere_recursive(data, reference->root_index); break.  Loop bound /Od: movsx count; add first.
  Assert lines /Od 0x207/0x211 vs January 0x206/0x210 (1-line drift).
- NEW since Lane B (20260920): real_math.h projection_from_vector3d body corrected 4f4c8502 (Sep 22), and the
  all-inlined header-COMDAT class admitted (owner 20260921 #5) -> the /Od helper form is now landable if exact.
- s1 (hyp: full /Od leaf helper form changes IL ordinals/expression count and flips j/k): leaf = distance local +
  point_from_line3d(-distance) + projection_from_vector3d + projection_sign helper. RESULT: byte-identical residual
  (same single j/k pair; 27/3). Helper-vs-expansion INERT for this function (consistent with Lane B P2 for pill).
- Lane B lab files preserved at C:/halo-worktrees/claude-lane-b-ai-physics-units-20260920/scratch/lane_b/w/collision_bsp/
  (micro1-7, lab_site_*, lab_sph_D1-8, lab_pill_D1-7). Lane B kept `while` because "January has a real loop" and never
  measured the /Od tail-call spelling.
- NEW HYPOTHESIS H-TRE: /Od 0x7ab1d3..0x7ab1e8 shows the single-sided descent is a real self-recursive TAIL CALL
  (`bsp3d_test_sphere_recursive(data, node->children[b])`) inside `if (!(node_index & LONG_MIN)) {...} else if (node_index != NONE) {...}`.
  If VC7 /O2 performs tail-recursion elimination, January's loop (test/js top, jns bottom, reference_index homed in the
  node_index param slot) is the COMPILER's loop, and the IL (call args -> param reassignment) carries a different
  expression inventory before the leaf dot -> can flip the j/k parity bit. Test s2.
- s2 (H-TRE: /Od tail-call recursion `if{...}else recurse(children[b])` + `else if (node_index != NONE)` leaf, no while):
  RESULT byte-identical to base (VC7 /O2 DOES do tail-recursion elimination here; produces January's loop exactly).
  Inert for the j/k bit. (Fact: the /Od recursive spelling and the while loop are byte-equivalent in this function.)
- Emission-order finding: January emits in bottom-up order edge_count, polygon, add_feature, render_debug x4, edge_length,
  perimeter, area, project_point2d, test_point2d, find_closest_point2d, test_line2d, surface_test_sphere, surface_test_point,
  sphere_test_vector, test_pill_new_recursive, bsp2d_sphere, leaf_test_vector, pill_test_vector, test_pill_new, bsp3d_sphere,
  test_vector_recursive, surface_test_pill, test_sphere, test_vector, bsp2d_pill, bsp3d_pill, test_pill. Ours defers
  test_pill_new/test_pill/test_sphere/test_vector to the end (public-first definition order) and emits add_feature after test_line2d.
  => January's DEFINITION order differs from ours. Assert anchors: polygon 0xe1=225, sphere3d 0x206=518, pill3d 0x498=1176;
  only ~293 January lines between polygon and sphere3d => January's definition order is NOT its emission order either
  (deferral happened). TU name-count law (lane C 20260921): names declared BEFORE a function in the TU span set arena
  ordinals -> definition order is a candidate source fact for the hidden-key ties.
- /Od build link order is ALPHABETICAL (add_feature 0x7aa190, bsp2d_pill, bsp2d_sphere, bsp3d_pill, bsp3d_sphere,
  collision_bsp_test_pill, ...): gives no definition-order evidence.
- HCEX.pdb line info (hcex_lines.txt): 2011 file order polygon 213, project_point2d 243, test_point2d 258, find_closest 297,
  test_line2d 397, test_sphere 475, bsp3d_sphere 507-592, bsp2d_sphere 597, surface_test_sphere 619-755, [756-842 ?],
  add_feature 843, test_vector 904, tvr 946, leaf_test_vector 1074, surface_test_point 1115, test_pill 1163, bsp3d_pill 1182,
  bsp2d_pill 1340, surface_test_pill 1373, pill_test_vector 1443, sphere_test_vector 1527 (top-down by query).
  HCEX emission order = LEVEL ORDER (repeated deferral passes, ties in def order) - reproduced exactly by the model
  "compile in def order; defer a function whose TU callee is not yet compiled; re-scan deferred list until empty".
  Same model reproduces OUR emission order. January emission = level order with pass1 = [edge_count, polygon, add_feature,
  rd_vertex, rd_edge, rd_surface, rd_bsp, edge_length, perimeter, area, p2d, tp2d, fcp, tl2d, sts, stp, stv, pnr],
  pass2 = [bsp2d_sphere, ltv, ptv, test_pill_new], pass3 = [bsp3d_sphere, tvr, stpill], pass4 = [test_sphere, test_vector,
  bsp2d_pill], pass5 = [bsp3d_pill], pass6 = [test_pill] => January is TOP-DOWN like HCEX (callers before callees), unlike
  ours (public first, private bottom-up). add_feature is defined before surface_test_sphere in January (pass 1).
- ORACLE (lab only, never lands): K=0..24 opaque `struct oracle_k;` names at file scope right before bsp3d_test_sphere_recursive:
  all three residual hashes (947b5817 / 0f9c2342 / b8b40ff3) and 27/3 UNCHANGED for every K. => the TU name-count
  (arena ordinal) key does NOT govern these x87 order ties; the key is in-function (M8 / Lane B parity law). Definition
  order of OTHER functions is therefore not a lever for them either (it only changes names before the function).
- /Od loop bound reads `movsx edx,word[count]; add edx,[first]` => 2020 source spells `leaf->bsp2d_reference_count +
  leaf->first_bsp2d_reference_index` (count first); ours spells first + count. Same op count; IL node order differs. Test s4.
- s4 (/Od count+first loop bound in sphere3d): byte-identical residual. INERT.
- Read lane C research (claude-lane-c .../scratch/research/x87-scheduling/FINDINGS.md): LAW T (single-expression sum emits
  reversed k,j,i; accumulator form emits forward i,j,k). January's sphere leaf is j,k,i = neither; so LAW T does not apply.
  astra research5 (20260920): sphere leaf j/k is a nonzero-offset term pair, outside M8.
- LAB L1sp (instrument only, never lands): `lab_sink_long(node_index > 5);` (one live GT + extern call) inserted at the top of
  the LEAF WALK of BOTH bsp3d functions => the sphere leaf dot flips to January's j,k order AND the pill's two leaf dots
  flip to January's k,j order simultaneously (all other diffs are the added call). CONFIRMS Lane B's single shared parity
  bit in situ: January has an odd count of extra distinct live operator expressions between the node dot and the leaf dot
  in BOTH functions (most naturally in the shared leaf walk or node tail idiom).
- p1 (pill: /Od 2020 push form `(plane & LONG_MAX) | (front ? LONG_MIN : 0)`): WORSE (+1 insn, many new hunks at 0x11e..).
  => January's pill push is the two-arm ?: (current source); the 2020 /Od source differs from January here. Rejected.
- Diagnostic parity map (lab only, instruments; lab_diag1.py): sphere leaf dot flips with: long stack_index (D3), dropping
  `| LONG_MIN` (D4), dropping `& LONG_MAX` (D5), dropping the NONE test (D7); NOT with children[1] for children[b] (D6, even).
  => every single live op between node dot and leaf dot toggles it; no /Od-attested construct differs there.
- Lane B dead-local instrument objects re-read with tools/dots.py (i-term first operand per D):
  pill n1(0x3c) D0..7 = eax,eax,eax,ECX,ECX,eax,eax,eax (Jan ecx); n2(0x59) = ecx,ecx,EAX,ecx,ecx,ecx,ecx,ecx (Jan eax);
  L1(0x23e) = eax,eax,eax,eax,edi,edi,edi,eax (Jan eax); L2(0x265) = edi x7, EAX at D7 (Jan eax).
  sphere node/leaf S0..8 = (eax,edi)(eax,edi)(eax,edi)(eax,eax)(ecx,eax)(ecx,eax)(ecx,edi)(eax,edi)(eax,edi) (Jan eax,edi).
  M8 fit: n1 operands (n-temp = point-temp - 2); n2 (n-temp = vector-temp + 1). January needs relative shifts n1 +3/+4,
  n2 +2, L2 +7(-1): i.e. January has 1-2 FEWER ordinals between the n1 and n2 temps and 3 fewer between n2 and L2 than ours.
- KEY M8 observation: the full /Od helper-form leaf removes exactly 8 named locals (absolute_i/j/k, projection,
  plane_distance, negative_distance, negative_dot, result_plane) => local count unchanged mod 8, and the helper temps are
  created after the leaf dots => explains why Lane B P2 (full /Od leaf) was byte-identical to P1.
- pv1 (pill: full /Od 2020 helper-form leaf: projection_from_vector3d, projection_sign, point_from_line3d x4 incl. the
  (real_point3d*) sweep-vector view, -plane3d_distance_to_point arg, plane3d_negate): L2 i-term now matches (eax) =
  Lane B P1 effect; hunks 30 -> 29; n1/n2 i-terms and leaf j/k unchanged. Emits point_from_line3d/projection_from_vector3d
  COMDATs (only admissible if the caller becomes strict exact).
- pv1 + D dead locals (instrument): D0 (eax,ecx,eax,eax) D1 (eax,EAX,eax,eax) h28, D2 same h24, D3 (ECX,ecx,edi,eax)...
  => with pv1 the node sites respond DIFFERENTLY than with base although pv1 removed exactly 8 locals: the node i-term
  ordinals depend on the LEAF's inline-helper composition (consistent with Lane B S3: a leaf helper flipped node i-terms).
- Tail labs on pv1 (instrument): T1/T2 (extra helper dot sites at the function end): node i-terms UNCHANGED (later inline
  sites do not renumber earlier ones). T3 (one live named `real` local declared at the END): n1 -> ECX and n2 -> EAX, i.e.
  BOTH node i-terms become January's, and L1/L2 i-terms stay January's (eax,eax). => the node i-term operands are the
  named `plane` local vs an inline temp; January has +1 (mod 8) named local declared AFTER `plane` relative to pv1.
  /Od 2020 node has exactly one more named local than ours: `d` plus `distance0 = d` (d,dot,distance0,distance1). Test pv2.
- pv2 = pv1 + /Od node locals (`real distance = plane3d_distance_to_point(...); real dot = ...; real distance0 = distance;
  real distance1 = distance + dot;`): ALL FOUR i-terms now January's (n1 ecx, n2 eax, L1 eax, L2 eax). 26 hunks.
  Remaining: (2) leaf j/k at 0x23e/0x265, (3) 0x3cc k,i,j, (4) int/x87 interleave 0x4da/0x4ea/0x520/0x52c.
- Parity instruments on pv2 (I1 long stack_index, I2 no NONE test, I3 sink): leaf j/k flip to January AND the 0x3cc hunk
  VANISHES => class (3) is the same parity bit as class (2) (and as the sphere's leaf). Only class (4) is independent.
- pb1 (base + /Od node locals only) and pb2 (pb1 + real plane3d_negate call, no new COMDAT): node i-terms UNCHANGED
  (eax,ecx), L2 still edi, 30 hunks => the i-term fixes need the /Od leaf helper composition (pv1) AND the node locals.
  pv2 is therefore the best reconstruction but it emits point_from_line3d/projection_from_vector3d COMDATs (P1 class unless
  the pill becomes strict exact).
- pc1 = pb2 + real projection_from_vector3d (no point_from_line3d): n1 ECX, n2 EAX (fixed) but L1 -> edi (broken), 28 hunks.
  Every i-term-correct variant needs a NEW header COMDAT (projection_from_vector3d and/or point_from_line3d) from a non-exact
  caller (P1 class) => not landable while the pill is fuzzy. pv2 retained as evidence only.
- owner_gated_asm.c (= Lane B surfsphere_asm_variantA.c, base + unit-local SSE __asm macro) still gates
  EXACT 880 _collision_surface_test_sphere at 16542e46 (28/2). Owner-held (no new __asm, 20260915); NOT landable.
- NEW HYPOTHESIS H-FOR: C1 parses a for-header's increment expression BEFORE the loop body, a while-loop's end-of-body
  increment AFTER it. So `for (...; ...; stack_index++)` creates the ADD before the leaf dot, `while` after => -1 live op
  before the dot = the parity flip, with (probably) identical loop code. Test a while-form stack loop / reference loop.
- w1b (reference loop as while with end-of-body increment): byte-identical to base (dot order unchanged) => H-FOR REFUTED
  for the reference loop. w1a/w1c (stack loop as while): code changes massively (720 bytes, frame differs) => January's
  stack loop is a `for`. Parity bit still unexplained.
- k4 (project_point3d(point_from_line3d(...)) nested, pv2+parity instrument): identical to k0 -> class-4 interleave inert.
  c4a/c4b (operand order of the hand-expanded start/sweep components): class 4 inert. tools/c4.py signature:
  JAN "IXXIIIXIXXX XXIIIXIXXXX" vs ours "IXIXIIIXXXX XIXIIIXXXXX" (int ops one x87 op early in both blocks), unchanged by
  D (M8), parity, helper/hand-expansion, nesting, operand order.

## 2. Admission findings (whole-object audit, brief section 4)
- pdb_storage: `_collision_bsp_usage_times` is NOT a cachebeta public (only collision_bsp.obj defines it; no January object
  references it) => January file-static. st1.c = base + `static` on its definition: gate 27/3 unchanged, all sections equal,
  ours storage 3. Needs config/symbols.json `"static": true` on its line (edited copy: config/symbols.json) + csplit-only regen.
  => production.patch (+ symbols.json line op). Data stays 100% (.bss 16 B).
- provider_link on the full candidate: TWO LNK2005 conflicts block eventual completion:
  (a) `_cross_product2d` vs actor_combat.obj NODUP hand copy (the held P1 item; collision_bsp is a 6th object it blocks);
  (b) `_plane2d_distance_to_point` vs geometry.obj NODUP hand copy (`#define plane2d_distance_to_point ..._inline` rename +
  hand-written body in source/math/geometry.c). NEW. Only geometry and collision_bsp define the symbol board-wide.
  geom/g1.c = Layer-1-style repair (drop the rename and the hand copy; the genuine real_math.h inline is used): geometry gate
  27/3 unchanged, EVERY geometry function hash unchanged (incl. the 3 parked residuals), `_plane2d_distance_to_point` EXACT
  32 B with SELECT_ANY (2) instead of NODUP (1), object_audit output identical, pair link with collision_bsp clean in both
  orders. January's own link had collision_bsp emitting the used-inline COMDAT, which is only link-compatible with an ANY
  geometry copy => the header inline is the authentic provider form. => geometry_provider_repair.patch (separate unit).
- Surplus COMDATs of base: all 21 byte-identical to January's selected providers (surplus_identity).

## 3. Final disposition (stop rule reached)
- _collision_surface_test_sphere: owner-held SSE __asm (exact with the held macro, owner_gated_asm.c); no new evidence.
- _bsp3d_test_sphere_recursive: ONE parity bit (leaf dot j/k). /Od shapes s1/s2/s4 inert; TU-name oracle inert; H-FOR refuted.
- _bsp3d_test_pill_recursive: decomposed into (1) M8 i-terms [fixed by pv2 = /Od helper leaf + /Od node locals, but needs
  point_from_line3d + projection_from_vector3d COMDATs (both byte-identical to January's selected copies) => only landable
  if the function becomes strict exact], (2)+(3) the SAME parity bit as the sphere, (4) int/x87 interleave (independent).
- Reopen: a January-era source fact adding/removing exactly ONE live operator expression in the shared node-tail/leaf-walk
  idiom (it must flip BOTH bsp3d functions; lab L1sp proves any one op there does). Then sphere = exact from base or s1;
  pill = pv2 + that fact + a class-4 fact (scheduler interleave of project_point3d argument setup).
