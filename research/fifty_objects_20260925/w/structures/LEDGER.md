# structures/structures worker ledger (lane claude/fifty-objects-20260925)

Target: `_sphere_intersects_cluster_portal` (January 432 padded / 418 meaningful / 11 relocs), unparked.
Baseline gate (production): exact 16 / residual 1 (`[size 416!=432, sha]`).

Read first: structures_obj_jonas_sphere_portal_first_shot_20260830 (E01 natural HCEA topology: do not repeat),
structures_obj_opus5_150k_w2_20260914 (v5.c: 432/11, 6 ops, point_from_line3d; inert: word sign type,
real_plane3d* local, top-level decls, point_from_line3d nested as argument; d9/d10 manual expansions worse; r0 section order inert),
structures_obj_opus5_next150_n1_20260915 (sph_v1/sph_v2 hand expansion: 432/11, A/B leaf-age clusters),
structures_fog_inline_closeout_20260923 (point_from_line3d COMDAT now ADMITTED in this TU via _render_debug_fog_planes).

Policy change since those ledgers: `_point_from_line3d` is emitted and admitted in structures.obj
(byte-identical to action_charge selected copy), so a genuine point_from_line3d call is admissible here.

## Probes

### P01 (resume, 2026-09-23) - re-measure w2 v5 body on current production
- Hypothesis: v5 body (genuine point_from_line3d, now admitted in this TU) is 6 ops off as recorded.
- Change: splice v5_body.txt into current production -> p01.c.
- Result: gate 16 exact / residual 1 `[sha]` (432/11). alndiff: 182 vs 182 insns, three 1-slot moves:
  `movzx ax,al` (T 0xed, O 0xeb), `push eax` (T 0xff, O 0xfc), `lea eax,[ebp-0xc]` (T 0x107, O 0x104) - ours each one op earlier.

### F01 - /Od readout FOUND (halo_cache_symbols.exe fn 0x8a0670, 663 B; dump od_sphere_portal.txt)
(od_cone.txt was the iterative clusters_in_sphere 0x8a0c90 which in the later build calls a real_math cone test 0x6d29e0 - not our fn.)
Identified by callee topology: tag_block x3, plane3d_distance_to_point 0x56d580, fabs 0x42dec0, distance_squared3d 0x42dc80,
global_bsp3d_get 0x865cc0, projection_from_vector3d 0x56d870, projection_sign 0x56d950, point_from_line3d 0x42e0d0 (REAL CALL,
standalone statement, result unused), project_point3d 0x4c9600, loop, square_root 0x42e320, convex_hull2d_test_circle 0x6ba0a0.
Called from 0x8a1210 (recursive). RTC names: projected_points[1024] @-0x410, projected_centroid3d[12] @-0x430, projected_centroid2d[8] @-0x440.
Slot/decl order: portal -8, plane_distance -0xc, projected_points -0x410, plane(ptr) -0x418, projection(short) -0x41c,
sign(short: movzx ax,al; mov word) -0x420, centroid3d -0x430, centroid2d -0x440, index(short) -0x448, then -0x44c sqrt, -0x450 fabs, -0x454 dist2 (temps or named?).
Statement order: portal; plane_distance = plane3d_distance_to_point(nested collision plane lookup, position) [position pushed FIRST];
if (radius > fabs(pd) [comiss radius,fabs; jbe F]) && (bounding+radius)*(bounding+radius) > distance_squared3d(position,&portal->centroid);
plane = lookup(global_bsp3d_get()->planes, portal->plane_index); projection = ...(plane); sign = ...(plane, projection);
point_from_line3d(position, plane, -plane_distance, &projected_centroid3d); project_point3d(&c3d, projection, sign, &c2d);
for (index...) project_point3d(lookup(&portal->vertices,index), projection, sign, &projected_points[index]);
convex_hull2d_test_circle((short)count [movzx word], projected_points, &c2d, square_root(r*r - pd*pd)) -> je F; mov al,1; jmp; F: xor al,al.

### F02 - HCEX.pdb (DIA2Dump -sym): params structure, portal_index, point (NOT position), radius; locals projected_centroid3d,
projected_points[0x80], projected_centroid2d (names only; enregistered scalars absent).

### P02 - full /Od shape (p02_body.txt): param `point`, nested collision plane arg, inner block decls in /Od order
(projected_points, `real_plane3d const *plane`, short projection, short projection_sign, projected_centroid3d, projected_centroid2d, vertex_index),
standalone point_from_line3d(point,&plane->n,-plane_distance,&c3d), then project_point3d, loop, nested-if return TRUE / bottom return FALSE.
- Hypothesis: /Od declaration order (centroids AFTER plane/projection/sign; v5 had them before) shifts IL-local ordinals -> interleave.
- Result: 432/11, alndiff IDENTICAL to P01 (same 3 one-slot moves). Decl order / standalone-vs-nested / plane-vs-normal local all inert. Keep P02 as the /Od-attested base.

### P03 - P02 + named `real projected_radius` (decl after vertex_index, assigned after loop) per /Od slot -0x44c
- Evidence: /Od stores sqrt result to [ebp-0x44c] ABOVE the fabs/dist2 temps (-0x450/-0x454, creation order) and pushes it via
  movss reload (named-local pattern, cf. /Od 0x42e150), whereas a directly nested float-call arg uses `fstp [esp]` (cf. /Od 0x42dc50).
- Result: 432/11, alndiff identical to P01/P02 (inert).

### LAB D4/H (oracle only, not landings) - k=1..7 unused in-function `long` dummies; k=1..8,12,16 TU-level extern decls before the fn
- Result: all 432/11 with the identical 6-op alndiff. The interleave is NOT a declared-name / declaration-count tie (H3/TU-name-count/M8-by-unused-decl ruled out).

### LAB D1 (diagnostic) - boolean projection_sign: removes movzx and reshuffles the whole int stream (homes swap [ebp+8]/[ebp+0x10]); confirms short. Not a candidate.

### LAB U (oracle) - k=1..7 USED pointer-alias locals feeding point_from_line3d's p argument
- Result: k=1,2 identical 6-op diff; k>=3 additionally flips the M8 x-term at +0x5d (plane3d_distance_to_point) but the three
  interleave moves NEVER change. The interleave is not IL-local-ordinal (M8) driven either.

### LAB L5/L6 (diagnostic) - hand rows with `real lab_t` (L5) keep the 3-slot interleave + M8 flips earlier; `double lab_t` (L6)
changes the x87 form itself (fld [esi]; fmul st1). No lever.
Observation: exact sibling _render_debug_fog_planes (same inline) interleaves pushes F I F I F I F I after fld/fchs/fld st0 (eager dups);
January sphere has fld st0 ADJACENT to its fmul at both lazy dups, ours inserts an int op between them.

### LAB VA (diagnostic) - `real lab_distance = -plane_distance;` passed as t: interleave unchanged + M8 flip in distance_squared3d. No.
Decomposition: the whole residual = in OURS an int op is emitted between the lazy dup `fld st0` and its fmul (x: movzx; y: push sign),
which pulls the following int ops one slot earlier (after fmul y exactly 3 int ops fill the slot in both builds). January: dup+fmul adjacent.

### F03 - corpus dup census (dupscan.py/dupctx.py/sepctx.py; January split objs, exactness vs build/base)
- EXACT reference with the SAME source statements: physics/collision_features.c collision_prism_test_point:
  `point_from_line3d(point, &prism->plane.n, -distance, &point_on_plane); project_point3d(&point_on_plane, axis, sign, &point2d);`
  January there interleaves F I F I F(dup) I F(fmul) I I ... i.e. an int op BETWEEN dup and fmul (the SEP form ours emits in sphere).
  So SEP is normal compiler behaviour for this helper; January sphere's ADJ (dup+fmul adjacent twice) needs a different DAG/latency.
- ADJ exact examples: units throw_grenade_release (the 'reused double temp' closure), particle_new (named `real mass` call result),
  code_00039990, actor_combat_update, object_new: dup'd value is a fresh call result / just-computed product.

### F04 - slot model (derived by hand): assume a forward latency scheduler where no-code IL nodes still occupy a slot.
Inserting ONE invisible node between each lazy dup and its fmul (dup -> X -> fmul) in OUR schedule reproduces January's order
EXACTLY (movzx displaced after fmul x -> 3 int ops before fadd x; push sign displaced after fmul y; lea c3d displaced after fadd y).
Candidate X = a per-use conversion of t, i.e. the tgr (units) lever: inliner substitutes a CAST argument `(real)double_var` at each use.

### LAB TG1 - `double lab_t = -plane_distance;` (inner block, after vertex_index) + point_from_line3d(point,&plane->n,(real)lab_t,&c3d)
- Result: **_sphere_intersects_cluster_portal EXACT (432)**, alndiff: no normalized instruction differences. F04 model confirmed.
- Next: strip test (implicit conversion), and look for the GENUINE source that yields a per-use double->real conversion.

### LAB TG2 (strip test of TG1) - same but pass `lab_t` with NO (real) cast: **EXACT**. The cast is inert decoration; the lever is the
double-typed simple variable argument (inliner substitutes a per-use double->real conversion = the F04 invisible node).
### LAB TG3 - `double plane_distance` (outer): frame 0x41c, qword home - NOT January.

### F05 - precedent/policy for the double lever
- PRODUCTION precedent, same idiom: source/ai/actor_moving.c:2136 `double t = -distance_along_step;` then `offset.i = step_vector.i*t + to_step.i;`
  (owner commit ea507d5b "Wave E fidelity landings", 2026-09-20). Also units.c:11087 `double origin_scale;` + point_from_line3d(..., (real)origin_scale, ...)
  (tgr closure; the '(real)' there is the same inert cast our TG2 strip test removes).
- docs/common_constants.md: "Use `double` only where the genuine API or default vararg promotion requires it." -> needs orchestrator/owner ruling disclosure.
- Later /Od build passes -plane_distance directly (xorps of the float local) - no 8-byte slot there; January-only evidence is the byte schedule.

### C1 / C2 - whole-TU candidates with the double lever (p02 /Od shape + `double t`)
- C1: `double t = -plane_distance;` FIRST decl of the inner block (actor_moving precedent form), point_from_line3d(point, &plane->n, t, &projected_centroid3d)
- C2: `double t;` last decl, `t = -plane_distance;` right before the call
- Result: BOTH gate **exact 17 / residual 0** (whole object code). Declaration position inert. C1 chosen (decl+init per house rule 17, precedent shape).

### FINAL candidate - final.c (= C1 with the two helper calls reformatted one-arg-per-line; byte-inert)
- gate --all: **exact 17 / residual 0 / unwritten 0**. /W3: +1 C4244 double->real at point_from_line3d arg, +2 C4244 short->boolean at
  project_point3d (short sign is byte-proven: movzx ax,al; /Od word slot). Build uses default warning level (no /W in build.ninja).

### AUDIT (section 4) on final.obj
1. gate --all 17/17 EXACT.
2. object_audit: PASS - all 26 January-owned sections/symbols match (storage/section/offset), candidate surplus list IDENTICAL to production's.
4. surplus_identity: 12 code COMDATs all IDENTICAL to January's selected copies.
   provider_link: FAIL (2) - _project_point3d vs path_obstacles.obj NODUP hand copy, _projection_sign_from_vector3d vs decals.obj NODUP;
   production build/base structures.obj fails IDENTICALLY (pre-existing, provider-side; not introduced by this change).

### F06 - HCEX (2011 PPC) body located by pattern in HCEX_debug.exe at 0x82d11dd0 (stwu -0x4a0 = PDB frame 0x4A0; PDB RVA
0xD0D3D0 does NOT map directly in this image). Projection block: `fneg f0,f27` then three `fmadds` (single precision), no frsp.
Neutral for the double lever: fneg of a float value is exactly representable, so double t -> real param needs no frsp. HCEX inlines
projection_sign/project_point3d entirely (2011 codegen), no further shape evidence. Dump: hcex_debug_sphere_ppc (see ppc.py).

### OUTCOME (2026-09-23)
- final.c sha256 2fb89c42..., final.obj 6857ee88...; production.patch (a/ b/ prefixes; `git apply --check` clean) replaces ONLY the
  sphere_intersects_cluster_portal body in source/structures/structures.c. No header/config/symbols.json change; no park to retire (fn unparked).
- gate 17/17 EXACT (+432 padded / 418 meaningful); relocdiff 11/11 0 differing; object_audit PASS (26 January symbols, 0 differ);
  pdb_storage 0 disagreements; data already 100% (2292/2292 in build/report.json); fake_match_scan 0 leads; no rejections/parks for the unit.
- Verdict FUNCTIONS_EXACT_OBJECT_BLOCKED: (1) provider_link FAIL(2) NODUP hand copies (path_obstacles.c project_point3d, decals.c
  projection_sign_from_vector3d) - pre-existing, identical on production structures.obj; (2) disclose `double t` for owner ruling
  (docs/common_constants.md double rule vs owner-landed precedent actor_moving.c:2136 + units.c:11087).
