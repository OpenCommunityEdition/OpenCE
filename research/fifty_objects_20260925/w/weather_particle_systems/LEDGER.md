# LEDGER weather_particle_systems (claude-fifty-objects-20260925)

Target: _weather_particle_system_render (1680/47, Jan frame 0x4dc, ours 0x4cc). Unparked.

## 0. Prior evidence read (2026-09-23)
- fresh_graphs_20260914: r7.c structural recovery (far_clip_distance, offset vector -> frame 0x4dc, box_position ptr, occluded flag, bounds store order, diameter-first scale).
- 150k_w1_20260914: e1 box_count=1 inside bounds seq (80->27 blocks), e6 block-scoped real box_width (530/530 insns), ra = e6 + dot_product3d(&planes[i].n, box_position) in transform_clip_planes_to_box -> 5 x87 operand-order rows left (4 inlined transform-site term order k*z+i*x+y*j, 1 position.x load order).
- 150k_w3: r0.c == ra.c remeasured: 1680/47 530/530; same 5 rows. Do-not-repeat lists noted.
- helper_recovery_20260923: update_physics landed exact (point_from_line3d COMDAT admitted). render remains.

## 1. Baseline (this lane, base e9e62b78 + current tree)
- production gate: 19 exact / 1 residual (render [sha], 1680 padded). update_physics + new_particle now exact in production.
- P0 cand_ra.c = production + prior ra.c render body + ra transform_clip_planes_to_box body (dot_product3d(&planes[i].n,(real_vector3d const*)box_position)).
  Result: 19/20, render residual 530/530 insns; alndiff rows (excluding category-only reloc name rows):
  T 0x2a7..0x2c1 inner inline transform site (4 rows: Jan k*z first with [eax+4]/[ecx+8], then i*x, then y*j loading box ecx+4 first) and
  T 0x40c position.x (Jan fld [ecx](box) ; fadd [ebx+4](particle), ours particle first). Same as w1/w3 recorded. Reproduced on current tree.

## 2. LAW M8 lab on cand_ra (dead-local instrument, LAB ONLY; lab/mod8lab.py, D dead `long` at top of render body)
- D=0: 5 rows (0x2a7,0x2ad,0x2b4,0x2bb inner transform; 0x40c position.x)
- D=1: 2 rows (0x2bb j-term operand order; 0x40c)   D=5: same 2 rows
- D=2: 4 rows (inner only; 0x40c FIXED!)  D=3,4: 5 rows   D=6,7,8: 9 rows (+0x358..0x378)
- D=8 != D=0 -> instrument perturbs beyond pure mod 8 (period not 8 for whole fn). Inner term order (k first) is count-keyed;
  position.x is count-keyed too (D=2 fixes it). No single D closes all -> not a pure top-of-function count; try placement-scoped D.

## 3. /Od readout (halo_cache_symbols.exe fn 0x57fb00..0x580760, od_render.txt) - NEW evidence (prior lanes had none)
RTC aggregates: build_sprite_data(164) box_offsets(312=26 pts; ours box_positions) clip_planes(80) clip_plane_d_transforms(520; ours box_plane_distances)
 weather_polyhedra_indices(16) box_deltas(12; ours box_offsets[3]) viewer_box(24; ours bounds) box_delta(12; ra's offset vector CONFIRMED)
 box(24; ours box_bounds) plane_evaluations(20; ours particle_plane_distances) position(12) viewer_to_particle(12; ours camera_to_position)
/Od slot inventory (2020 build; declaration topology evidence only):
 fn: system -8, definition -0xc, structure_bsp -0x10, type_index(short) -0x14
 type scope: type -0x18 (=&system->types[i] inline), type_definition -0x1c, particle_index(long) -0x20, DEAD short -0x24 (=0 before `if particle_count`, never read)
 if-block: build_sprite_data, box_offsets[26], box_count(short) -0x214 = 0 (box 0 uses box_offsets[box_count] then box_count++ as a separate stmt),
   real -0x218 = type->box_width (passed to build_clipping_planes; later MIN(fadeout_end, it)), clip_planes, clip_plane_d_transforms,
   weather_polyhedra_indices, weather_polyhedra_count(short) -0x49c, box_deltas[3], i,j,k shorts, viewer_box(rect3d)
   viewer_box: x0=y0=z0=0.f chained; x1=y1=z1=type->box_width chained; then x0+=bo[0].x, y0+=.y, z0+=.z, x1+=.x, y1+=.y, z1+=.z
   box_deltas[0]=-box_width, [1]=0.f, [2]=box_width
 inner: box_delta.i/j/k = box_deltas[i]/[j]/[k]; box.x0=vb.x0+bd.i; y0; z0; x1; y1; z1; frustum test; assert line 674 (Jan 673);
   box_offsets[box_count].x/y/z = box.x0/y0/z0; transform(cpdt[box_count], clip_planes, &box_offsets[box_count]); box_count++ separate
 after loops: build_sprites_begin(&bsd, type->particle_count, td->bitmap.index, &td->shader, 0)
 particle loop: particle -0x514 (declared in loop body), box_offset ptr -0x518 = NULL, box_index -0x51c, plane_index -0x520 (SHARED by both plane loops), plane_evaluations -0x538
   plane_evaluations[p] = CALL 0x56d580(&clip_planes[p], &particle->position) (2-arg real fn = plane3d_distance_to_point)
   for (box_index=0; box_index<box_count && !box_offset; box_index++) { boolean visible(byte -0x53d)=TRUE; for (plane_index=0; plane_index<5 && visible; plane_index++) visible = plane_evaluations[p] + cpdt[box_index][p] < 0; if (visible) box_offset = &box_offsets[box_index]; }
   if (box_offset) { real -0x570 = MIN(td->distance_fadeout_end, far); position.x = particle->position.x + box_offset->x (PARTICLE FIRST) ...;
     viewer_to_particle = position - camera.position (explicit); depth(-0x56c) = CALL dot_product3d(&viewer_to_particle, &camera.forward);
     if (depth>td->fadein_start && max>depth) { fade_in(-0x574)=PIN(..); fade_out(-0x578)=1-PIN(..);  byte -0x57d = TRUE; short poly idx -0x57c;
        polyhedron ptr -0x584; short plane idx -0x588; NO named plane local (plane3d_distance_to_point(TAG_BLOCK_GET_ELEMENT(...), &position) inline arg);
        if (plane idx == count) { flag=FALSE; break; }   if (flag) { direction ptr -0x58c ... build_sprite(... 2.f*radius*td->runtime_one_over_sprite_width ...) } } }

## 4. Probes on the /Od-faithful rewrite (splice_body.py <base> body_X.c X.c; rungate.sh X)
- od1 (body_od1.c): /Od topology + names (structure, box_offsets, clip_planes, clip_plane_d_transforms, weather_polyhedra_indices/count,
  box_deltas, viewer_box chained-assign + `+=`, box_delta, box, box_count=0 ... box_count++ after box-0 transform, far_clip_distance assigned
  after find, particle declared in `while` body, box_offset=NULL, plane_evaluations via plane3d_distance_to_point (no hand expansion),
  maximum_distance=MIN(..) declared after position/viewer_to_particle/depth, position = particle->position + box_offset (PARTICLE FIRST per /Od),
  polyhedron plane test without named plane local, visible flag TRUE->FALSE, `2.f*radius*w`).
  Hypothesis: prior ra.c residual rows are M8 ordinal keys driven by local inventory; /Od inventory should move them.
  RESULT: 530/530; position.x row CLOSED (particle-first /Od spelling), inner j-term operand row CLOSED; remaining inner i/k term order (3 rows)
  + sprite scale row (2.f*radius*w -> ours (r*w)*2).  [first compile had assert-path backslashes eaten by heredoc; fixed]
- od2 = od1 + sprite scale `runtime_one_over_sprite_width*(2.f*particle->radius)` (r7 spelling; /Od-consistent: /Od loads 2.f then *radius then *w,
  which both `(2.f*r)*w` and `w*(2.f*r)` produce) -> sprite row CLOSED. REMAINING: 3 rows, inner transform site term order (Jan k-term first).
- M8 lab on od2 (D dead `long` at top of render): D=2 -> STRICT EXACT (lab only); D=6 leaves only 0x40c; D=8 == D=0 (clean period 8).
  => od2's only residual is an IL-ordinal COUNT defect: January has od2+2 (mod 8) locals/inline temps ahead of the inner transform keys.
  /Od attests ONE extra declaration od2 lacks: the dead `short X = 0;` at type scope after particle_index. Need to locate the second.
- placement lab (lab/place.py, +2 dead longs): TOP, type scope, end of if-block decls, inner scope, particle-loop scope, fade block -> ALL strict EXACT.
  => function-wide count key (user locals numbered before all inline temps). January = od2 + 2 (mod 8).
- /Od frame census (all [ebp-X] refs): every gap is an /RTCs guard (4B each side of each RTC aggregate); no hidden slot. /Od inventory == od2 + dead short X only.
- x1 (/Od dead `short X = 0;` after particle_index) alone: residual 3 rows (as +1). x1b (X=0 + 1 initialized dead long): EXACT.
  x1u/x1ub (uninitialized unused decls): no effect -> only IL-visible (initialized/referenced) locals count. Need ONE more genuine local.
- LAB v1 (od2 with vector_from_points3d replaced by explicit viewer_to_particle stores, as the /Od shows): D=2 no longer closes; D=5 closes the
  inner rows but breaks 0x40c. => inline temps of LATER sites also shift the inner-site key (the key symbols are optimizer temps numbered after
  ALL IL locals + inline temps); position.x (0x40c) key tracks user-local count only (broken for D in {4,5,6}).
- /Od check: 2020 vector_from_points3d is a REAL function (0x42e3a0, called by distance_squared3d 0x42dc80), yet /Od render computes
  viewer_to_particle with explicit subtractions -> the source wrote explicit statements at this site (per-site first-party attestation).
- LAB v1 D=1 EXACT, v1 D=7 residual (inner key period 4); **v1X = v1 + /Od dead `short X = 0;` after particle_index: STRICT EXACT** (render fn).
  => both /Od-attested inventory facts (explicit viewer_to_particle: -3 inline temps; dead short: +1 user local) together close the count key.

## 5. Final candidate fin1.c (= production + ra transform body [dot_product3d] + body_fin.c render)
- body_fin = od2 + explicit viewer_to_particle (/Od) + `short unused_count = 0;` after particle_index (/Od dead short) + polyhedron plane
  counter renamed polyhedron_plane_index (no shadowing; names inert).
- gate --all: **exact 20 / residual 0 / unwritten 0** (render EXACT 1680). fin1.obj kept.
- controls (whole-TU gate): fin body + production hand-expanded transform body -> render residual (dot_product3d helper body load-bearing, /Od-attested);
  fin1 minus unused_count -> render residual (the /Od dead short is load-bearing); strip_cast (transform without the (real_vector3d const *) view
  cast) -> 20/20 EXACT (cast byte-inert; only removes C4133).
- CL /Zs /W3: fin1 has exactly production's warning set (3 pre-existing C4244 long->short in other fns + header warnings); no new warnings.

## 6. Whole-object audit
- object_audit fin1: all 20 .text + data sections ok; 1 symbol DIFF: _one_over_char_max storage January 2 (EXTERNAL) vs ours 3 (STATIC)
  (pre-existing: production base obj has the same DIFF). pdb_storage: cachebeta PUBLIC -> January storage is external.
- fin2 = fin1 with `static` dropped from `real const one_over_char_max = 1.f/255.f;` (unreferenced in the TU; no other definer in source or
  cachebeta publics): gate 20/20 EXACT; object_audit: 30/30 January symbols match, all January-owned sections ok.
  Surplus list identical to production base object (header COMDAT helpers, literals, .drectve, get() assert string).
- ctl_nocond (drop the /Od-attested redundant `&& !box_offset` loop condition; break kept): 20/20 EXACT -> byte-inert; kept /Od form.
- fake_match_scan fin2: 0 leads (production: 0).
- surplus_identity (production obj; candidate surplus list is identical): 14 header COMDATs, 13 IDENTICAL to January's selected copy,
  _real_local_random DIFFERENT (ours `pop ecx` header-inline body vs January effects.obj `add esp,4` named-result body).
- provider_link fin2: all PASS except _real_local_random FAIL (LNK2005 vs effects.obj NODUP hand copy, both orders). SAME FAIL on the
  production base object (pre-existing since new_particle landed; nodup_census: effects.obj NODUP). provider_link --baseline=build/base:
  "No surplus external definitions: PASS" -> candidate introduces no new surplus.
- report.json: data 949/949 (100%) already; code 4114/5779 -> 5779/5779 with render.
- production.patch (git diff; LF, applies with core.autocrlf=true as in this worktree) verified in an isolated autocrlf repo:
  result byte-identical (CRLF) to fin2.c == final_weather_particle_systems.c.

## 7. VERDICT: FUNCTIONS_EXACT_OBJECT_BLOCKED
All 20 functions strict EXACT, all 30 January symbols (after the one_over_char_max storage fix) and all January sections match.
Remaining object blocker (pre-existing, outside this unit): _real_local_random surplus COMDAT vs effects.c NODUP hand copy
(known NODUP provider-link class; fix is an owner decision in real_math.h/effects.c, previously measured to disturb units).
Owner-visible disclosures: (1) `short unused_count = 0;` is a DEAD declaration landed on first-party /Od attestation (slot ebp-0x24,
init-only in the 2020 /Od build) - it is load-bearing for the M8 count key (control ctl_noX residual); (2) explicit viewer_to_particle
statements replace vector_from_points3d at this one site on /Od attestation (2020 has vector_from_points3d as a real function yet
render computes it inline-explicitly); (3) (real_vector3d const *) view cast in transform is byte-inert (strip test) and /Od-attested
(dot_product3d(&planes[i], box_position) real call).
