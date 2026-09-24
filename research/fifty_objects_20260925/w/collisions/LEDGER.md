# collisions worker ledger (lane claude/fifty-objects-20260925)

## Step 0 - baseline (fresh run; no prior slug dir existed)
- gate --all at e9e62b78: exact 19 residual 1; `_collision_move_point [sha]` 4752.
- real_regions on build/base: 39 regions, 2 REAL = the single Lane B transposition at 0x595/0x598
  (J: 595 mov edx,[edx+0xc] / 598 fmul [ebp-0x30] / 59b mov [ebp-0x50],eax; ours: store-then-fmul-then-load).
- Production already carries the Lane B 20260920 landing (try_22). Recorded inert (DO NOT REPEAT):
  new_clip_count statement position x3, decl order, set_real_vector3d, +=1, break placement, csmemcpy size,
  else-if nesting, prototype count 0..31, unused locals 1..8, macro/point_from_line3d/double clip_velocity_to_plane,
  /Od initialiser chain, epsilon-first fabs compare (astra wave2), loop form do/while(TRUE) (62 regions).
- Lane B reopen evidence wanted: same-compiler exact fn with a 4-dword struct copy emitting L2 L3 S2 S3.

## Step 1 - census of 4-dword copies (scratch/w/collisions/copyscan.py, copyscan_jan.txt / copyscan_base.txt)
- January: 141 16-byte reg copies; 77 `L0 S0 L1 S1 L2 S2 L3 S3`, 15 `L2 L3 S2 S3` (ALL with last load reusing the base reg).
- Our build reproduces `L2 L3 S2 S3` in many EXACT fns (quaternions_multiply, planes_intersect_rectangle, render_camera_mirror,
  collision_test_vector (same TU, x87-interleaved, `L2 L3 S3 S2`), breakable_surface_get_plane_from_designator) -> the order is
  source/context controllable, not a compiler-version gap. Only 5 fns differ J vs ours in copy order (move_point, actor_look_update,
  draw_bitmap_in_rect, biped_update_moving/physics).
- Same header source bsp3d_get_plane_from_designator (cdecl, L2 S2 L3 S3) vs breakable_surface_get_plane_from_designator
  (static custom ABI, L2 L3 S2 S3) -> order depends on register/ABI context, not text alone. Only the LAST pair ever reorders,
  and only when L3 targets the dying base register -> consistent with a post-RA reorder of an independent load above a store.
- In move_point ecx (=&collisions[new_clip_indices[0]].plane) stays live to `push ecx` 0x652 (CSE for line_from_planes3d), hence `mov edx,ecx`.
- object_audit on build/base: every January section/symbol ok except _collision_move_point (44/44 symbols) -> closing it would complete the object (surplus audit pending).

## Step 2 - oracle probes on the real function (lab only, none landable)
- o1 (drop clip_position_to_plane), o2 (drop clip_velocity_to_plane), o3 (swap the two helpers; only 5 REAL regions):
  copy stays `L2 S2 L3 S3` in all -> the int copy order is NOT driven by which fp consumer comes first.
- pa (index `collision_count - 1`), pd (line_from_planes3d arg -> &clip_plane: ecx CSE dead, base not copied, 2 temps
  eax/ecx alternate): still `L2 S2 L3 S3`. pe (copy from `collision->plane`, esi base live): `L0 L1 S0 L2 S1 S2 L3 S3`.
- lab TU (scratch/w/collisions/lab/lab1*.c): with free registers the copy loads hoist freely above stores (L0 L1 S0 ...)
  even when clip_plane is exposed before/after -> no alias edge between the copy's own loads/stores; the order is a
  register-pressure/emission decision. In move_point only eax + the dying base (edx) are free (ebx clip_count, edi
  collision_count, ecx CSE address live to 0x652, esi new_clip_count after 0x5a1).

## Step 3 - MECHANISM FOUND (oracle probes, lab only)
- pg  (case-1 assert without the `valid_real_plane3d(&clip_plane)` call): copy flips to January's `L2 L3 S2 S3`.
- pg3 (`valid_real_normal3d(&clip_plane.n) && valid_real(clip_plane.d)`, TU-visible header inline called out of line): January order.
- pg4 (member address `&clip_plane.n` cast, still to EXTERNAL valid_real_plane3d): ours (2 REAL) -> not member-vs-whole.
- pg5 (`valid_real_normal3d((real_vector3d const *)&clip_plane)`): **0 REAL regions** (only the call relocation name differs).
- => VC7 treats &clip_plane passed to an OPAQUE external prototype as an escape (pointer load L3 may alias store S2, so S2
  must precede L3); a callee whose body is visible in the TU does not capture -> free order -> January's L2 L3 S2 S3.
- January calls `_valid_real_plane3d` yet has the free order => in January's collisions.c TU the BODY of valid_real_plane3d
  was visible (header __inline not inlined at this site -> COMDAT folded board-wide), not the bare prototype in
  real_math.h:672. Production matrix_math.c:1073 defines it as a plain function (image position just before its first user
  matrix4x3_from_plane at 0xF9CE0 is also consistent with a COMDAT emitted before first use).
- /Od double attestation: at the in-loop plane assert the /Od build makes ONE call `valid_real_plane3d(&collision->plane)`
  (0x7A5829..0x7A5833 -> fn 0x6C7730 = `valid_real_normal3d(&p->n) && valid_real(p->d)`), while January's bytes at 0x485..0x4E8
  show that body INLINED (magnitude_squared-1 fabs<eps, then the d exponent test). So valid_real_plane3d was an inlinable
  (header __inline) function in January's collisions TU: inlined at the loop site (0x3CE), called out of line at case 1 (0x428).
  Production hand-expands it at collisions.c:1212 (`valid_real_normal3d(&collision->plane.n) && valid_real(collision->plane.d)`).
- January references _valid_real_plane3d only from collisions.obj and render_cameras.obj; defined (NODUP census: 1 definer)
  in matrix_math.obj right before its first user matrix4x3_from_plane (COMDAT-before-first-user placement).

## Step 4 - probes toward a landable form
- v1 (TU-local `__inline valid_real_plane3d` definition added to collisions.c + loop site `valid_real_plane3d(&collision->plane)`):
  595 copy now January-ordered but preheader 17b/17f flips (the known name-count coin flip; +2 declared names before the fn). Not landable anyway (consumer-local definition).
- h1 = header override scratch/w/collisions/hdr/math/real_math.h (prototype at real_math.h:672 REMOVED, `__inline boolean
  valid_real_plane3d` defined right after valid_real_normal3d) + collisions.c loop site `valid_real_plane3d(&collision->plane)`
  compiled with scratch/w/collisions/gate_local.py (override dir first on /I):
  **== exact 20 of 20** (move_point 0 REAL regions). New surplus COMDAT `_valid_real_plane3d` in collisions.obj.
  Tools: gate_local.py (local CL wrapper, main file compiled from its own dir), cmp_obj.py (strict rows + surplus delta).
- h1b (override header, collisions.c UNCHANGED i.e. hand-expanded loop site): also 20/20 -> the header inline is the lever;
  the loop-site call form is the house-rule (no hand expansion) + /Od-attested spelling, byte-inert here.
- Surplus `_valid_real_plane3d` COMDAT from collisions (h1.obj) is section_infos_equal to January's matrix_math copy.
- matrix_math: override header + plain definition -> C2084 (as expected). mm1 = definition removed (header inline provides it):
  matrix_math 35/35 exact, objdelta vs production matrix_math.obj IDENTICAL in every symbol section (COMDAT selection
  becomes ANY(2) instead of NODUP(1); January split shows 1 but csplit synthesises it).
- render_cameras (render_camera_mirror calls valid_real_plane3d(&adjusted_plane) out of line in January): override ->
  only `+ _valid_real_plane3d` surplus COMDAT; every other section identical; 20/21 unchanged (build_frustum residual pre-existing, unchanged).
- NOTE: override must shadow BOTH include spellings ("math/real_math.h" x149, "real_math.h" x38).
- sweep.py (mirror-tree board sweep, no tracked edits): control mirror on physics = 16/16 identical to build/base.

## Step 5 - board sweep of packet H1 (real_math.h: proto@672 removed + inline after valid_real_normal3d; matrix_math.c def removed; collisions.c loop site)
- sweep_pkt.txt: 441 TUs, 436 identical to build/base, 5 deltas:
  collisions GAINED _collision_move_point (+surplus COMDAT); render_cameras +surplus COMDAT only;
  **LOST _bitmap_copy (bitmap_drawing) and _rasterizer_frame_statistics_draw**; physics ~_physics_update_old (already non-exact).
  Control mirror compiles of bitmap_drawing / rasterizer_frame_statistics are identical to build/base -> losses are real
  header-position (declaration/ordinal) effects of the packet. H1 is NOT landable as is.
- Header-position variants (scratch/w/collisions/variant.py; r*=proto@672 removed, k*=proto kept; def after
  valid_real_normal3d / axes3 / matrix4x3 / end-of-file), targeted full sweeps (sweep_v_*):
  * r-normal/r-axes3/r-matrix/r-end: IDENTICAL outcome (position inert): +move_point, -_bitmap_copy, -_rasterizer_frame_statistics_draw, ~physics_update_old.
  * k-normal/k-axes3/k-matrix/k-end: move_point changed but NOT exact; -frame_statistics_draw, -_decals_delete_permanent_from_cluster,
    -_bitmap_2d_alpha_bleed, ~__rasterizer_model_draw. Worse.
  => the losses are declaration-count/ordinal ties in unrelated TUs, not header position.
- sweep.py regex fixed for "saved films/games" (6 TUs missed before): control + r-normal packet both identical there.
  FINAL r-normal packet census over all 447 TUs: +_collision_move_point; LOST _bitmap_copy, _rasterizer_frame_statistics_draw;
  ~_physics_update_old (non-exact both ways); surplus +_valid_real_plane3d in collisions & render_cameras only.
- Name-count oracle (dummy decls in lab copies only; NEVER landable), per [[tu-name-count-oracle]] currency:
  bitmap_drawing (_bitmap_copy): base exact at +0,+3..+6 extern-var units, fails +1,+2; under r-normal fails +0,+1, exact +2..+6
  => r-normal == base+1 unit for this TU. `int f(int a);` behaves as 3 units.
  frame_statistics_draw: base exact ONLY at +0 (fails +1..+6); r-normal fails +0..+6 => also base+1, needs -1 unit.
  rnone (prototype removed, no body) breaks both too (base-3). Inline definition therefore costs ~4 units, prototype 3.

## Step 6 - compensation search (oracle / genuine-cleanup candidates), packet = r-normal header + matrix_math + collisions
- declcensus (copied to scratch/w/collisions/declcensus.py): non-CRT duplicate prototypes: bitmap_drawing TU `bitmap_delete`
  (bitmaps.h:27 and bitmaps_internal.h:24); frame_statistics TU `stack_walk_disregard_symbol_names` (cseries.h:240 and cseries_windows.h:62).
- mx/b1 (drop bitmaps_internal.h copy) or mx/b2 (drop bitmaps.h copy): bitmap_drawing IDENTICAL to build/base again (bitmap_copy restored).
- mx/c1, mx/c2 (drop either stack_walk copy): frame_statistics_draw still lost.
- lab oracle: removing 1..6 of frame_statistics.c's reconstruction `typedef char verify_*` asserts never restores it under r-normal
  (and removing any breaks it under the base header) -> its plateau is a single point; not reachable by +-6 local units.
- 25-point wider oracle (extern-var dummies K=7..30 under r-normal): frame_statistics_draw never restored -> the broad-header
  packet cannot be compensated inside that TU by count alone. Broad real_math.h route parked.
- FOCUSED-HEADER route (precedent: real_math_planes.h was created for exactly this declaration-count sensitivity):
  hdrf = real_math.h UNCHANGED (prototype stays) + `__inline boolean valid_real_plane3d` appended to math/real_math_planes.h
  (only collisions.c includes it). collisions h1 (fn-scope position): 19/20, preheader 17b/17f flips (count coin);
  **h1s = h1 with `position` back at success-block scope (the pre-Lane-B production form): 20/20 EXACT, 0 REAL regions.**
- mx/B full sweep (447/447 TUs, incl. saved films/games): **446 identical to build/base; only collisions differs:
  +_collision_move_point strict (sha 8b2f2900... == January) and +surplus _valid_real_plane3d. ZERO regressions.**
  B = real_math_planes.h gets the inline; matrix_math.c drops its plain definition and includes real_math_planes.h
  (matrix_math.obj byte-identical in every symbol section; its _valid_real_plane3d COMDAT becomes selection ANY(2));
  collisions.c = h1s. real_math.h untouched.
- Link: packet collisions + packet matrix_math PASS both orders; packet collisions + CURRENT matrix_math (NODUP) FAIL
  (so the matrix_math half is mandatory); base render_cameras + packet matrix_math PASS.
- Audits on packet collisions.obj: object_audit PASS (44/44 symbols); pdb_storage 0 disagreements; surplus_identity 20/20
  identical (+ new _valid_real_plane3d == January matrix_math copy). provider_link: PRE-EXISTING FAILs (also on current
  build/base) _plane3d_negate (NODUP hand copy decals.c:896) and _point_in_sphere (NODUP hand copy path_obstacles.c:191).
  report.json data for the unit: .data/.rdata/.bss 100% (590/590). objdiff under-credits strict-exact
  _object_get_features_in_sphere (93.8%, jump-table internal labels; no semantic_matches entry yet).

## Step 7 - FINAL packet F (scratch/w/collisions/final/*, scratch/w/collisions/production.patch)
- Files: source/math/real_math_planes.h (+__inline valid_real_plane3d), source/math/matrix_math.c (+#include "real_math_planes.h",
  -plain valid_real_plane3d definition), source/physics/collisions.c (loop-site `valid_real_plane3d(&collision->plane)`;
  `real_point3d position;` back to success-block scope). real_math.h untouched. Patch applies cleanly (patch -p1), CRLF kept.
- mx/F full sweep: 447 TUs, 446 identical to build/base, collisions: +_collision_move_point strict, +surplus _valid_real_plane3d. 0 losses.
- Strip tests under packet header: loop-site call form BYTE-INERT (kept for house rule: /Od attests one call, no hand
  expansion); function-scope `position` -> 19/20 (0x17b/0x17f preheader coin, count-keyed) so block scope is load-bearing;
  final collisions.c with the TRACKED headers -> 19/20 (header half is load-bearing).
- fake_match_scan: 0 leads on final collisions.c / real_math_planes.h; matrix_math.c lead (line 940 __asm) is pre-existing.
- move_point has 5 internal (jump-table) relocations like _object_get_features_in_sphere (objdiff 3.3.1 93.8% under-credit):
  integrator should expect a switch-label semantic_matches.json entry for both if report.json stays <100%.
- REMAINING OBJECT BLOCKERS (pre-existing, identical on current build/base): provider_link FAIL for surplus _plane3d_negate
  (NODUP hand copy decals.c:896) and _point_in_sphere (NODUP hand copy path_obstacles.c:191). Owner-decision category.
- NEW LAW (general): VC7 treats `&local` passed to an OPAQUE external prototype as an escape (a later pointer load may not be
  hoisted above a store to that local); a callee whose BODY is visible in the TU (header __inline, even when not inlined at
  that site) does not capture. Tell: January hoists the last load of a struct copy above the previous store (L2 L3 S2 S3)
  where we keep L2 S2 L3 S3. Candidate lever for other residuals whose January callee was a header inline that our
  reconstruction declares only as a prototype (e.g., the NODUP hand-copy helper family).
