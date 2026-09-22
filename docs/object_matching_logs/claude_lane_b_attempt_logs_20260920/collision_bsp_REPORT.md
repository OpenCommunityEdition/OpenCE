# collision_bsp Lane B worker REPORT

## Baseline (python tools/campaign/gate.py source/physics/collision_bsp --all), candidate.c == real source (cmp identical)

```
EXACT        48  _add_feature
EXACT       272  _bsp2d_test_pill_recursive
EXACT       160  _bsp2d_test_sphere_recursive
residual   1504  _bsp3d_test_pill_recursive  [sha]
residual    800  _bsp3d_test_sphere_recursive  [sha]
EXACT       128  _collision_bsp_test_pill
EXACT       112  _collision_bsp_test_pill_new
EXACT       672  _collision_bsp_test_pill_new_recursive
EXACT       208  _collision_bsp_test_sphere
EXACT       240  _collision_bsp_test_vector
EXACT       800  _collision_bsp_test_vector_recursive
EXACT       112  _collision_edge_length
EXACT       400  _collision_leaf_test_vector
EXACT       416  _collision_surface_area
EXACT        96  _collision_surface_edge_count
EXACT       560  _collision_surface_find_closest_point2d
EXACT       224  _collision_surface_perimeter
EXACT       224  _collision_surface_polygon
EXACT        80  _collision_surface_project_point2d
EXACT       448  _collision_surface_test_line2d
EXACT       480  _collision_surface_test_pill
EXACT       304  _collision_surface_test_point
EXACT       256  _collision_surface_test_point2d
residual    880  _collision_surface_test_sphere  [size 848!=880, sha]
EXACT       528  _pill_test_vector
EXACT        64  _render_debug_collision_bsp
EXACT       128  _render_debug_collision_edge
EXACT       112  _render_debug_collision_surface
EXACT        80  _render_debug_collision_vertex
EXACT       208  _sphere_test_vector
== exact 27  residual 3  unwritten 0  (of 30 listed)
```


## Run 2 (resume) - evidence + attempts. candidate.c still == real source at this point.

### Recovered from run-1 lab objects (run 1 banked no text)
- lab_sph_D1..D8 / lab_pill_D1..D7 / lab_tv_D1..D8 = M8 instrument (N dead `long` locals at fn top).
  sphere3d: leaf j/k order NEVER flips for D=1..8; D4-D6 only break the node i-term. pill3d: no D closes both node i-terms (D2 fixes 0x67 only, D3/D4 fix 0x4d only).
- probe_sph_pfl (real point_from_line3d call in sphere leaf): byte-identical residual AND emits forbidden `_point_from_line3d` COMDAT (guard FAIL, newsyms VIOLATION).
- lab_pfv (header projection_from_vector3d): worse (header inline body `return j < i` is NOT the body of the exact out-of-line copy in decals.obj).
- lab_t2 / micro2 / lab_site_*: the dot j/k order is sensitive to the SHAPE of OTHER pointer expressions (`data->bsp` read at +4 instead of +0 flips it). Struct layout is fixed by January bytes, so not a lever, but proves j/k is a hidden expression-table key, not operand text.
- dotscan_jan.txt: January-wide census of 3-term x87 dots: 99 kji, 19 jki. In this unit the exact `_collision_bsp_test_vector_recursive` is jki with the SAME helper text that gives kji in the pill node => helper text is not the key.

### /Od build (scratch/orch/odbuild.py): sphere3d = fn 0x7aafe0 (od_sphere3d.txt), pill3d = fn 0x7aa590 (od_pill3d.txt)
- sphere leaf /Od: distance=plane3d_distance_to_point(plane,data->center); point_from_line3d(data->center,&plane->n,-distance,&center_projected_on_plane [RTC name, 12B]); data->projection_axis=projection_from_vector3d(&plane->n); sign=projection_sign_from_vector3d(..)!=(designator&LONG_MIN?1:0); project_point3d; bsp2d recursion; break. Node part in 2020 is an `if`+tail call; January bytes have a real loop (`jns 0x13`) so `while` stays.
- pill /Od: node has `d`, dot, distance0=d, distance1=d+dot; leaf: t=0; if(dot!=0){inverse; t=...; PIN}; helpers projection_from_vector3d, 4x point_from_line3d, plane3d_negate(plane,&data->result->plane); RTC locals p3d(12) p2d(8) p3d(12) v3d(12).

### Measured shapes this run
- S2 sphere: named `distance` + `-distance` at the three uses: 288 insns, 13 hunks/29 rows (fsubr) - WORSE, rejected.
- S3 sphere (DIAGNOSTIC, lab-local inline copy of decals.obj-exact projection_from_vector3d body): 287 insns, projection region byte-identical, leaf j/k unchanged, both i-terms flip (local count -4). Not admissible (header body differs; new COMDAT).
- S4 sphere: `(node->plane_designator & LONG_MAX) | LONG_MIN` (/Od-attested, folds away): byte-identical to baseline, j/k unchanged. Inert.
- P1 pill: real `plane3d_negate(plane,&data->result->plane)` (/Od-attested call, COMDAT already emitted at baseline) instead of hand expansion: 557/557, 20 hunks/34 rows (baseline 21/33): leaf i-term 0x273 now matches. No exact loss.
- P2 pill (DIAGNOSTIC, lab-local inline copies of point_from_line3d + projection_from_vector3d, full /Od leaf shape): BYTE-IDENTICAL to P1 => helper-vs-manual expansion is inert; residual is not the expansion.
- P3 pill: P1 + /Od node locals (distance, distance0=distance, distance1=distance+dot): 21 hunks/38 rows (0x273 reverts). Rejected.

### J/K TERM-ORDER PARITY LAW (new, measured; micro4-7.c + run-1 lab_site_* objects; 50+ data points, 0 exceptions among live-expression cases)
The j/k term order of an inlined 3-term dot (kji vs jki) flips with the PARITY of the number of DISTINCT (hash-consed, function-wide) LIVE operator expressions created before the dot: integer ADD/SUB/MUL/AND, NEG, compares, casts, ?:, !, x++ each count 1; IND/CONST/VAR/calls, `&&`-vs-nested-if, assert macro form, and DEAD locals count 0; an identical expression repeated counts once (h8 vs hf; L1L2; N2 vs N2L3); expressions AFTER the dot count 0 (h1); a named `real` local adds one (ke/kf vs kd). Explains run-1 site labs exactly (N1,L1,L2 flip; N2,L3 alone do not because ADD(bsp,0xc) stops being shared; N2L3 flips).
Consequence: sphere3d's whole residual is ONE parity bit between the node dot (K, matches) and the leaf dot (January J, ours K); pill3d's two leaf dots need the opposite single flip (January K, ours J).
- D1 (DIAGNOSTIC dead `long lab_dead = node_index + 5;` in both leaves): no flip (dead code is removed before the key is formed) - so a landing needs a LIVE, byte-inert, one-operator difference. None found that is natural:
- assert-form diagnostics lab_as_f1/f2/f3 (if/else, ||, ?: forms): all inert. S4/S5 inert as the law predicts (shared AND; same node count).
Sphere shape cap reached (run-1: pfl, pfv, s1, x1; run-2: S2, S3, S4, S5 + 2 diagnostics). candidate.c unchanged for sphere3d.
- U1 (both private context structs: `unsigned long plane_stack[]`, law-motivated same-size CONV): no flip, byte-identical. Rejected/inert.

### _collision_surface_test_sphere - NEW first-party evidence for the owner-held SSE routine (NEEDS OWNER RULING; candidate.c stays asm-free)
/Od build fn 0x7ae6a0 (od_surfsphere.txt), RTC descriptor names the address-taken locals `dsqr` (4 B) and `vec` (12 B). At the vertex-distance site the 2020 source keeps the SAME scaffold January's bytes show:
- /Od pre-loop `lea eax,[ebp-0x1c]; mov [ebp-0x24],eax` (pointer local = &dsqr)  <->  January 0x59/0x62 `lea eax,[ebp-0xc]; mov [ebp-0x1c],eax`.
- /Od in-loop `mov [ebp-0x38],vertex` / `mov [ebp-0x3c],data->center` (two pointer locals a, b)  <->  January 0xa6/0xa9 `mov [ebp-0x14],eax` / `mov [ebp-0x10],ecx`, reloaded by the kernel (0xac..0xba).
- /Od body is the C fallback `vec[n]=a[n]-b[n]; vec[n]*=vec[n]; dsqr=vec[0]+vec[1]+vec[2]` (scalar, element-indexed `real *`), January body is the packed kernel movss/movhps/subps/mulps/shufps 0x0E/addss/shufps 0x39/addss/movss [eax].
  => first-party source had a dedicated inline routine here with pointer operands and result-by-address; it is NOT distance_squared3d(). This is the "authenticated source" half the 20260914 park criterion lacked.
- surfsphere_asm_variantA.c (sha256 bbb18344...; candidate.c + the 20260914 p1 macro `fast_distance_squared3d(a,b,result)` with three block-local pointer locals + 15 `__asm` lines; `hit_feature = FALSE;` after the breakable return): gate `EXACT 880 _collision_surface_test_sphere`, `== exact 28 residual 2 unwritten 0`; unitcheck gains=1 regressions=0; guard pass; newsyms PASS (0 new); fakescan 15 inline-assembly leads (the asm itself).
- variantB/C (pointer local assigned pre-loop exactly as /Od places it, outer `dsqr`): 307/307 insns, 4 hunks = one frame-slot swap (dsqr -0xc <-> b -0x10); declaration order inert. So January's bytes favour the in-macro result pointer (compiler-hoisted) of variant A.
Not placed in candidate.c: inline asm was owner-held 20260915 ("leave parked for now").

### Final state
candidate.c == real source (cmp identical). gate: `== exact 27  residual 3  unwritten 0  (of 30 listed)`; guard passed; newsyms PASS; fakescan 0 leads; unitcheck NO CHANGE.
Optional fidelity note (not landed, zero byte credit, moves a parked body's sha): P1 real plane3d_negate call in the pill leaf.
