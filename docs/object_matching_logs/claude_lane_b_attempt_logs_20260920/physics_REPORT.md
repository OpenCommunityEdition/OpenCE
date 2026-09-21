# physics WAVE 3 report (append-only)

Baseline (real source): == exact 13  residual 4  unwritten 0  (of 17 listed)

## _physics_compute_vehicle_collision (1168)
- /Od fn 0x7bb7d0: real call point_from_line3d(&center0,&axis,mass_point0->radius-penetration,&point) (fn 0x42e0d0, 4 args),
  `collision = TRUE` is the LAST statement of the if-block (0x7bbbc8), 4x set_real_vector3d zero init, RTC names total_force0/1,total_torque0/1,center0/1,axis,point,force0/1,radius0/1,torque0/1.
- January: inlines the helper at 0x204..0x227 (`fld [esi+0x68]; fsub st(2); fld dir.i; fmul st(1); fadd p0.x ...`), frame 0xb0, 1 ret.
- A1 real call + flag at block end: `[size 1152!=1168, sha]` -> `[sha]`, 355/355 insns. ONLY diff: `mov byte [ebp-1],1` at rel idx 78 (after `fstp [ebp-0xa0]`) vs January idx 79 (after `fmul [ebp-0x70]`). Emits _point_from_line3d COMDAT (48 B) -> needs 5a.1 admission = exact caller.
- A2 set_real_vector3d x4 instead of `= {0,0,0}`: byte-inert (reverted; production spelling kept).
- Flag position map (7 shapes, diagnostic): start / mid-scale / after point / mid-vfp / before cross#1 -> idx 1; between crosses / after crosses / after force adds / END -> idx 78. Two canonical slots only => block is emitted in two windows; the store goes to the head of the window its statement falls in.
- Window anchor: statements added BEFORE the block (diag pre1/pre2) do not move it (block-anchored). Prior w1..w8 (statements added inside window 1) move the boundary 1:1 with instruction count => window ~78 pre-peephole instructions.
- E1 `0.5f*(radius-distance)`: inert. E2 `2.0f*(...)`: inert. E3 `2.0f*(mass_scale*(penetration*(g/d)))`: same final FP insns, flag moves to idx 76 => leaf-left/computed-right products cost one HIDDEN pre-peephole instruction each. January needs ONE FEWER hidden instruction in window 1 than ours; every product in my source is already computed-left. No admissible candidate found for the -1.
- D0..D8 local-count sweep (prior wave objects): flag stays at 78 -> not M8.
- V4 `penetration*(g/d)*...`, V5 `mass_scale*(...)`: single reversals inert (78); only the right-deep nest E3 moves it (76) => hidden count follows TREE SHAPE, not operand order. 12 shapes spent (cap 10) -> STOP. Residual: one 4-byte store one instruction early, all 355 instructions otherwise identical.

## whole-TU real-helper conversion (try_w3_real_all.c = wave_prev body + 21 macro sites -> real point_from_line3d calls, vector args cast (real_point3d [const] *) as collision_features.c/real_math.c/ai_debug.c do)
- gate: compute_new `[size 3120!=2944, relocs 51!=49, sha]` -> `[sha]` (910 vs 911 insns, relocs 49/49); update_old `[size 5456!=5168, relocs 118!=115, sha]` -> `[size 5184!=5168, sha]` (1621 vs 1627, relocs 115/115); vehicle `[sha]`. unitcheck gains=0 regressions=0 other=3. Every site inlines (no reference to _point_from_line3d), but the COMDAT is emitted (48 B).

## _physics_compute_new (2944) - [sha], 23 real diff blocks
- /Od fn is 0x7b9d40 (aligned-frame prologue `push ebx; mov ebx,esp`; odbuild func_start cannot find it - use w3_odfn.py). Inline-call inventory matches ours exactly (9 pfl3d, 10 add, 8 scale, 6 dot, 3 set, 2 cross, 2 pin_fraction, 1 magsq).
- /Od has a DEAD local `real x = mass_point_definition->mass*gravity` at [ebp-0x28] (written once @0x7b9ef5, never read), declared after powered_mass_point. Tried it (`mass_point_weight`): byte-inert (23 blocks) -> not added.
- /Od weight = fraction*fraction*pinned*pinned*ground_scale with a SEPARATE pinned local; January 0x410..0x420 = A*A*F*F*scale (fld st0; fmul st1; fmul st2; fmulp st2) -> January != /Od here, ours matches January.
- Diff classes: 6-9 operand-commute flips (0x1a7,0x1bb,0x3dc,0xa4f,0xa85,0xac4 + 0x512, 0x974) ; antigrav chain order (January X*frac*strength*MASS then *alignment `fmulp st(2); fstp st(0)`, ours *alignment then *mass) ; pin_fraction arg temp ecx vs edx + January reloads powered_mass_point from [ebp-0xc] after the call (0x412) where ours keeps it in ecx.
- M8 instrument (dead `long x = 0;` xD, D=0..7, at function top AND at loop top: identical => count-only): NO D clears all flips (site 0x1a7 needs D=2, 0xa85 needs D=5, 0x3dc ok for D=1..6). Uninitialised dead locals are not IL-visible (all D identical). So the flips are not one missing local; do not re-sweep.
