# render_cameras wave-2 worker ledger (_render_camera_build_frustum)

Base tree 931ed8dc. Baseline gate: exact 20 / residual 1 (build_frustum 3408/112 vs 3376/113; frame 0x84 vs Jan 0x6c).
_pi is now `((real)3.14159265358979323846f)` in real_math.h (float literal) -> the old `_pi` header blocker is landed.
Prior best packets (evidence only, carried diagnostic _pi override):
- waveD bf15 (opus5-100k consolidated scratch/workersD/render_cameras/park_progress_build_frustum.c): frame 0x6c exact via lexical blocks.
- w1 bf2: set_real_vector3d in projection else -> 113 relocs.
- w3 bf5a (opus5-150k scratch/workers/w3_render_cameras/bf5a.c): 3376/113, 48 hunks.
Do-not-repeat (from ledgers): def reorder, decl-order permutations, commutative operand respellings (canonicalised),
fabs operand swap, n.i/n.j/n.k separate stmts, (real) cast removal on projection_scale, midpoint via set_real_point3d.

## Probes
- P0 (control): bf5a build_frustum grafted into current prod (no _pi override needed now) -> 3376/113 [sha], frame 0x6c,
  slot census identical to January; real_regions 48 REAL. Remaining clusters: cross product j/k rows, far x0 operand,
  midpoint inv*half_z order, projection z_far double load + inv*n order, warning dot i-term orders + mov eax,id placement.
- /Od readout (odbuild fn 0x83d600, RTC 0x83f098): locals x_axis,y_axis,z_axis,viewspace_normal,viewspace_plane,
  projected_bounds(16),frustum_vertex,frustum_midpoint,clip_plane. Planes/corners/midpoint via set_real_vector3d /
  set_real_point3d; projection block has a named `z_far` local ([ebp-0x100], loaded once, used twice); warning checks
  use a SHORT counter [ebp-0x108] passed as `index++` to each of the 22 calls; vertex loop index is short.
  January single z_far load + `fld st(0)` dup at 0x84d/0x850 is consistent with the named z_far local (ours loads twice).
- prior mod8 "D=2" was measured on the 3408 production body (diff 635->633), not on bf5a: not transferable.
- LAB mod8 sweep on bf5a (dead `long` locals at body top, instrument only): D=0..8 REAL = 48,43,50,42,49,42,49,41,48 (D8==D0).
  D=4 alone fixes the whole cross-product (0x21c/0x227); odd D fix most warning i-term dots. => cross product and
  warning dots need DIFFERENT relative offsets: the inline-temp count BETWEEN them is wrong (helper routing in between).
- P1 `real z_far = camera->z_far;` as block initializer: 54 REAL, 1077 insns (load hoisted before the if) - worse.
- P2 `z_far = camera->z_far;` assigned after clip_offset (the /Od statement position): 41 REAL, 1080=1080 insns;
  January's single z_far load + dup reproduced; projection residual shrinks to inv*n operand order (852..899).
- P3 short counter `condition_index++` for the 22 ids (block or top-level): function bytes IDENTICAL to lab D=1 on P2
  -> the counter is a pure +1 IL-local (count-only) lever; it does not move the `mov eax,id` placements.
- LAB L1 (half_z declared first): inert. LAB L2 (half_z STATEMENT first): midpoint inv*half_z order fixed (X2 law:
  later-defined frame scalar loads first) but emission order of half_z breaks -> January's order is not a plain
  statement reorder. LAB L3 (inverse pair first): worse (3392).
- P4 = P2 + midpoint via set_real_point3d (/Od-attested helper call): 39 REAL; cross product FULLY fixed (M8 count
  shift); midpoint cluster still differs.
- P5 = P4 without left_plane_z/bottom_plane_z locals (/Od spells `1.0f + center_x` inline in the plane and
  `(center_x + 1.0f) * far_x` in projected_bounds; VC7 CSEs it into January's [ebp-0x1c] home): 37 REAL; far x0
  hunk 0x5ad FIXED. Cross product back to 1 hunk (0x22d) because the IL count dropped by 2.
- LAB L4 fabs operand swap / L5 `n.i = inv*scale*n.i` / L6 both: all byte-inert vs P5 (canonicalised; confirms ledgers).
- LAB L7/L8 projection-else variants: field stores (L7,E7) make `inv` load first in the fabs products (January) but lose
  the n.i x87 phi + fld 0.0 reloc (112); chained/other orders no better. set_real_vector3d keeps phi+reloc but n-first.
- LAB L9 projection-block decl order: inert. L10 midpoint z statement first: emitted literally, no MID fix.
- perm_far.py: all 30 statement orders of inv_x/far_x/inv_y/far_y/half_z -> current order is best (37).
- LAB L11 (no far_x/far_y locals): 77 REAL, 3392 - far locals are real.
- LAB L12 (world_vertices[4] copy after midpoint): 139 REAL - statement order of the copy is right as is.
- LAB L13 (union local + `if` around the FOV diagnostic, current production form): 3392 / 93 REAL. January's direct
  integer read `mov ecx,[edi+0x28]` inside the vassert needs `*(long *)&camera->vertical_field_of_view` (bf5a form;
  /Od 0x83d805 also reads the bits straight from the camera field; landed precedent units.c:11494).
- Toggle matrix on P5 (T1 counter, T2 no viewport int locals, T3 explicit negation instead of plane3d_negate [/Od],
  T4 side planes via set_real_vector3d [/Od], T5 near/far via set_real_vector3d [/Od], T7 short vertex index [/Od]):
  T7 inert. Best 34 REAL: T3+T5 (CP 0x227 + MID + PROJ + W-core), T2+T3+T4, T2+T3+T5. T3 fixes the first warning
  check's pointer/term order (0x99f..0x9ac) = a real source fact (/Od shows 4 explicit negations, no call).
- clusters.py over the 64-combo matrix: W-core of 14 hunks (mov eax,id placement at checks 5/10/15/21 + fld [esi+8]
  vs pointer-load order at midpoint checks 17/18/19) is invariant under every toggle; MID (11) and PROJ (8) invariant.
  No combo gives CP=0 and W=14 together (best CP=0: T1+T2+T3+T5 with W=21).
- LAB L14 (reciprocal spelled inline `1.0f / projection_x_scale`, no inverse locals): 33 REAL, MID operand order fixed.
- **P7 = P6(T3+T5) + the /Od division spelling** `far_x_scale = -camera->z_far / projection_x_scale;` and midpoint
  `-half_z * bounds_center_x / projection_x_scale` (no inverse locals; /Od 0x83dd72 and 0x83e006 verbatim): VC7 itself
  CSEs the divisor into January's `fld 1.0; fdiv px; fst [ebp-0x20]` reciprocal. **23 REAL; MID cluster fully fixed.**
  => January wrote divisions, exactly as the later /Od build; the inverse locals were a lift artefact.
- **P8 = P7 + the /Od-literal projection block** (no inverse_plane_z local; else-branch 4 field stores; `clip_offset =
  -view_plane.d / view_plane.n.k; z_far = camera->z_far; projection_scale = (real)(z_far / ((z_far - clip_offset) *
  (fabs(n.i / n.k) + 1.0f + fabs(n.j / n.k)))); n.i = projection_scale * n.i / n.k; ...; d = -projection_scale *
  clip_offset;` + explicit negation): PROJ cluster FULLY fixed incl. the n.i/n.k x87 phi and the 113th reloc (fld 0.0).
  32-variant matrix (else F/S x sum x rows x d x c): only rows `scale * n / n.k` + field-store else give PROJ=0; the
  sum/d/c spellings are canonicalised (inert). set_real_vector3d else = PROJ 4; `*= scale / n.k` rows = reloc 112.
- P8 mod8: D=1/3 -> W=14 (core only), CP=1; D=2 -> CP=0 but W=21.
- **P9 = P8 + /Od short counter `condition_index++` (block form): 15 REAL** = CP 1 hunk (0x216, j-row a.k*b.i order;
  M8) + W-core 14 (scheduling of `mov eax,id` at checks 5/10/15/21 and `fld [esi+8]` vs plane-pointer reload at
  midpoint checks 17/18/19). FAR/MID/PROJ all zero, frame 0x6c, 3376/113.
- LAB L16 counter type `word` (the /Od pushes it with movzx): byte-identical to short.
- LAB L17 (instrument: dead `magnitude_squared3d(&camera->forward);` call) placed before CP / between CP and W /
  after W: after-W inert; before-CP fixes CP (=P9+1 key shift) but moves W; between moves only W.
  => inline temps are numbered in SOURCE order after all user locals; CP needs +1 before it, W needs 0 or +2 net.
- LAB ub.py two-knob sweep on P9 (u dead user locals at top, b dead `magnitude_squared3d(&camera->up)` calls = +1 temp
  each, placed before `frustum->z_near = ...`): the ONLY (u,b) giving CP=0 and W=14 is (1,7), i.e. January has one more
  IL symbol before the first cross product and one fewer (mod 8) between CP1 and the far section. REAL 14 there.
- P10 = P9 + T2 (no viewport int locals) + T4 (side planes via set_real_vector3d) = the full /Od-literal inventory:
  CP=0 but W=16 (check-1 i-term 0x9da/0x9e3). ub sweep on P10: only (u=0,b=6) gives CP=0 & W=14.
- T5-off combos (near/far via field stores) x {T2,T4}: none gives CP=0 & W=14 (x: CP2/W16; T2: CP1/W14; T4: CP1/W14;
  T2+T4: CP2/W16). set_real_vector3d instances AFTER CP1 still move CP1 -> numbering is not purely source order for them.
- ub needs: P9 (1,7); q_t4 [T5 off,T4 on] (1,7); q_t2 [T5 off,T2 on] (6,0); P10 [T2,T4,T5] (0,6).
- LAB Q2 (no z_far local, /Od division form): PROJ=4 -> the named z_far local is a January fact (keep).
- LAB L19 `(real)fabs(...)` casts: byte-identical to plain `fabs(...)` (strip test: the cast is decoration; drop it).
- LAB L20 `long` counter: +1 IL symbol (truncation temp): CP=0, W=21. Not a W-core lever.
- W-core per-check study (checks 5/10/15/21: `mov eax,id` slot; checks 18/19/20: plane-pointer reload before vs after
  `fld [esi+8]`): dependency-free placement differences; invariant under every toggle, u/b instrument, counter type,
  cast and inventory change tried (~200 compiles).
- LAB L21 `1.0f + bounds_center_x` (/Od literal) in side-plane set_real_vector3d: byte-identical to `bounds_center_x + 1.0f`.
- LAB L22 on P10: +6 (== -2) instrument temps anywhere between CP1 and the far section -> CP=0 & W=14 (REAL 14).
- LAB L23 explicit z-axis negation instead of negate_vector3d: worse (CP=2) - negate_vector3d is right (/Od call 0x43e7f0).
- LAB L24 helper defined first (January section order): inert (confirms waveD).
- LAB L25 union local (no if) for the FOV bits: OTHER=47 - only the direct `*(long *)&camera->vertical_field_of_view`
  read reproduces January's `mov ecx,[edi+0x28]` (disclosed view cast; landed precedent units.c:11494).
- Cleaned candidate (authentic /Od names, reformatted) first gate: 21 REAL - bisected to dropping the inner grouping
  parens in `1.0f / ((hw / vh * vw) * tangent)`: VC7 reassociates the flat product chain; the parens are LOAD-BEARING,
  i.e. decoration-only parens by the strip test (owner rule) -> must be replaced by a genuine source structure.
- Replacement: the /Od-attested separate statement `half_bounds_width = half_bounds_width / viewport_height *
  viewport_width;` (/Od 0x83d766 reassigns the half-width slot [ebp-8] before the projection scale) then
  `projection_x_scale = 1.0f / (half_bounds_width * field_of_view_tangent);` -> byte-identical to P9, no parens needed.
- cand_c (final body + unused warning-condition enum removed): gate exact 20 / residual 1, build_frustum 3376/113,
  REAL 15, normalized sha bf7d0cf4...91a5; objdiff 3.3.1 fuzzy 88.98603 -> 98.6946 (scratch/w/render_cameras/odproj).
- Wide u sweep 8..31 on cand_c: period exactly 8; W-core (14) never resolves at any count -> stop rule (dependency-free
  placement only, after >5 evidence-based shapes and ~300 compiles).

## Final (stop rule reached)
- W-core (14 hunks: `mov eax,id` slot at checks 5/10/15/21, plane-pointer reload vs `fld [esi+8]` at checks 18/19/20)
  is dependency-free placement, invariant under every lever (u 0..31, b 0..7, 64-combo toggle matrix, counter type,
  casts, inventory, helper definition order). CP1 residual (1 hunk, 0x216) is an M8 IL-count key: January has one more
  symbol before CP1 and one fewer (mod 8) between CP1 and the far section (lab (u,b)=(1,7)); no attested source fact found.
- Landable (house-rule clean) candidate: scratch/w/render_cameras/render_cameras.c / production.patch
  (FOV diagnostic kept as production's union+if): 3392/113, frame 0x6c, objdiff 94.318436, sha 59bd8a23...e3a0.
- Owner-gated best: render_cameras_owner_gated.c / production_owner_gated.patch (`*(long *)&camera->vertical_field_of_view`
  direct bit read, /Od 0x83d805 attests the direct integer field read; precedent units.c:11494): 3376/113, frame 0x6c,
  REAL 15, objdiff 98.6946, sha bf7d0cf4...91a5.
- Both: 20/21 other functions stay EXACT, provider link PASS, fake scan 0, object_audit only DIFF = build_frustum.
  New SELECT_ANY surplus _set_real_point3d/_set_real_vector3d (provider-link PASS), _plane3d_negate surplus gone.
- Park `_render_camera_build_frustum` must stay parked; measurements.base must be refreshed if either body lands.
- FIX: mkunion.py/l13.py had lost one backslash level in the assert path literal (surplus `??_C@_0CD@...` string, provider
  link FAIL). fixpath.py repaired cand_u.c / render_cameras.c / L13 / L25; re-gated: exact 20 / residual 1,
  3392/113, sha 59bd8a23..., objdiff 94.318436, provider link PASS. production.patch regenerated (git apply --check OK).
  (Code bytes were unaffected; only the literal's relocation target.)
- Park refresh values if a body lands:
  landable  -> base {size 3392, relocation_count 113, normalized_sha256 59bd8a23e4655c735566084045c1996849ca90ebb17294450456c8fd9146e3a0}, objdiff 94.318436
  gated     -> base {size 3376, relocation_count 113, normalized_sha256 bf7d0cf4713777d20496f3be84e0755dfea411fd9bf22cf10715cf200eae91a5}, objdiff 98.6946
