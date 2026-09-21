# breakable_surfaces worker log (Lane B, 2026-09-21) - resume state

candidate.c == source/physics/breakable_surfaces.c (cmp clean). NO CHANGE.
gate: `residual   4032  _breakable_surface_effect  [sha]` / `== exact 11  residual 1  unwritten 0  (of 12 listed)`.
guard pass, newsyms 0/0 OWNERSHIP PASS, fakescan 0 leads, unitcheck NO CHANGE.

## _breakable_surface_effect
January: frame 0x1240 both, 1 ret both, 117/117 relocs same multiset, 1156 vs 1155 insns, end 4020/4022.
- R1 +0x37b/+0x389/+0x392: cross_product3d(&s_normal,&surface_plane.n,&t_normal); Jan `fld plane.n.k|j; fmul s_normal.i`, ours reversed (6 rows, 0 B).
- R2 +0x97d: Jan `fxch st(1); fadd [ebp-0xf8]` consumes x87-held jitter.y; ours `fld [ebp-0xf8]; fadd st(2)` + extra `fstp st(0)` at +0x9b5 (+2 B). Ledger closes.

/Od fn 0x7a0ca0 (od_breakable_surface_effect.txt; asserts 253/268/350/390 = Jan 251/266/348/388, spans equal):
- 0x7a0e73 call 0x56d870(&surface_plane) -> projection_axis (ONE helper call); 0x7a0e91 call 0x56d950 -> projection_sign.
- fn 0x56d870 body: i,j,k=fabs; if (k>=j && k>=i) return 2; else if (j>=i) return 1; else return 0  == Jan +0x1b1..+0x1f3 and decals.c exact copy; real_math.h:969 body differs.
- 0x7a1aeb/0x7a1b2c/0x7a1d3a/0x7a1e81: call 0x42e0d0 = point_from_line3d (4 real calls, no pointer/distance locals).
- 2020-only extras, unconfirmable in Jan, NOT used: dead word copy of surface_vertex_index [-0x11e8], dead long counter [-0x120c], real 0.75 [-0x1244]; no u/scale locals.

Attempts:
1. DIAG v_D0/D1/D3 (unreferenced long locals at top) and v_E1/E3 (`long x = 0;`): 5 regions/8 rows unchanged -> not an M8 count key.
2. try_prior_q2.c (S1 pointer-normal block): residual [sha], R1 gone, R2 left; `normal` pointer local not /Od-attested -> not adopted.
3. try_prior_p2.c (S1 + genuine point_from_line3d at 2 grid sites): EXACT 4032, 12/12; guard FORBIDDEN-EMITTED-SYMBOL _point_from_line3d (48 B).
4. try_prior_r1.c (genuine call at all 4 sites = /Od shape): EXACT 4032, 12/12; same guard failure.
5. Census of 35 prior w2 objects (no recompiles): R2 fixed only with NO named pointer local in the grid block; named s_plane.n/t_plane.n reads flip the else-arm term order (Jan k,j,add,i; named basin j,k,add,i; flat helper i,j,add,k). Only inline parameter substitution gives both. Stopped.

Verdict: residual-unchanged. NEEDS OWNER RULING (point_from_line3d COMDAT accounting) + header packet real_math.h:969 body.
