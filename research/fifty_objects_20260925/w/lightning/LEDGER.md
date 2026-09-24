# lightning (_lightning_submit) - wave-2 worker ledger (lane claude/fifty-objects-20260925, base 931ed8dc)

## Baseline
- gate --all on production source: exact 8 / residual 1 (_lightning_submit 2576 [sha]); helper _lightning_offset_marker_position already EXACT at base.
- alndiff base: 756/756 insns; two real packets:
  P1 @0x38e global_z_axis3d fallback copy: Jan `mov ecx,[g]; mov edx,[ecx]; mov eax,[ecx+4]; mov ecx,[ecx+8]` (6-byte mov ecx,[abs]) vs ours eax-pointer (5-byte A1 form) -> 1-byte shift -> `nop` vs `mov edi,edi` pad + downstream movzx edx/ecx, lea eax/edx.
  P2 @0x876 vertex-2: Jan `fstp st(0); mov ecx,[ebp-8]; fstp [esi+0xc]` vs ours fstp before reload.
- All other alndiff rows are the objdiff '$'-literal symbol-vs-defined-noncode annotation (identical).

## Prior ledgers read (do-not-repeat respected)
jonas_offset_helper_fixed_point_20260826, jonas_complete_donor_one_shot_20260828, fable_small_families_20260901 (S1-S16),
credible_fuzzy_completion_20260902, fable_salvage_reconciliation_20260902 (6 component orders, set_real_vector3d, typed ptr, scalar locals),
opus5_150k_w2_20260914 (vertex-2 store orders s1-s5, u locals s6/s7), opus5_150k_w3_20260914 (l3 set_real_vector3d axis, l5 set_real_point3d vertex, l6 set_real_point2d tex, p1 side loop).
NOTE: none of the prior ledgers used the later /Od build.

## /Od readout (halo_cache_symbols.exe fn 0x79d1c0, saved od_submit.txt)
RTC: marker, points[147492], bounds(24), centroid(12), ?(12 @-0x24140 = vertex-loop up), adjusted_color(16), ?(12 @-0x24190 = segment up), displacement(12).
- vertex loop: point=&points[vi] (ptr local), previous/next via ?:, width=points[vi].width*thickness_scale, u=vi*one_over_point_count, assert 280,
  vector_from_points3d(previous,next,&up); cross_product3d(&up,&render.camera.forward,&up); normalize(fn 0x6cc0f0);
  adjusted_color.{a,r,g,b}; pixel=real_argb_color_to_pixel32(&adjusted_color) (named local);
  **point_from_line3d(point,&up,width,vertices)** (fn 0x42e0d0 body == real_math.h: v->i*t + p->x), color, tex.x=u+u_offset, tex.y=0, vertices++;
  **point_from_line3d(point,&up,-width,vertices)** (-width is the ARGUMENT expression, xorps), color, tex.x, tex.y=1, vertices++;
  if (vi==0) bounds.x0=bounds.x1=point->x ... else MIN x0,y0,z0 then MAX x1,y1,z1.
  => current source HAND-EXPANDS point_from_line3d (rule 6 violation) and uses `width = -width` (S8c) instead of the -width argument.
- centroid via set_real_point3d(&centroid,(x1+x0)*.5,...).
- else-arm: segment `up` fallback = ONE aggregate integer copy from global_z_axis3d (pointer loaded once) => aggregate assignment, not 3 component stores.
  octave loop: start=1<<(octaves-oi); increment=start<<1 local BEFORE assert 408; fraction local; jitter_offset=((next-md)*fraction+md)*scale*jitter_scale;
  scale_vector3d(&up, real_random_range(-1,1)*jitter_offset, &displacement) (no named `jitter` local); set_real_point3d(point, (prev+next)*.5+displacement.*); width/color avg; valid=TRUE; scale*=0.5.
  unexplained: [ebp-0x2417c]=0.5f stored once, never read (dead local in later build).

## Probes
- c1 (H: /Od-attested point_from_line3d(position,&up,width,&vertices->point) + point_from_line3d(...,-width,...) replaces the hand expansion and `width=-width`):
  RESULT body BYTE-IDENTICAL to base (same P1+P2 residual); emits a surplus `_point_from_line3d` SELECT_ANY COMDAT (rule-6 exception territory).
  => spelling is codegen-inert for the body; it is the house-rule-correct spelling (rule 6: never hand-expand) but needs the COMDAT exception (identical copy + exact caller + provider link).
- c2 = c1 + `up = *(real_vector3d const *)global_z_axis3d;` (/Od 0x79df14: ONE aggregate integer copy, pointer loaded once; HCEX: global_z_axis3d is `const union real_point3d *`, up is `union real_vector3d` => the aggregate copy needs a representation cast):
  RESULT P1 closed; only P2 remains (2 hunks: fstp [esi+0xc] vs reload). Cast = owner-gated view copy (load-bearing; strip -> compile error / component copy mismatches).
- ord/ 36-run: all 6x6 permutations of {TX,C,TY} for vertex1 x vertex2 on c2 (point_from_line3d shape). Best = 2 hunks (unchanged P2 or an integer store hoisted above the deferred point.z fstp). No permutation closes P2. (Extends w2 s1-s5, which fixed vertex1.)
  Mechanism observed: a vertex-2 integer store that precedes TX in IL is used as fadd->fstp latency filler (hoisted above fstp z) and then the reload lands before TX (Jan position); Jan has the reload before TX with NO filler.
- c3 = c2 + four /Od-attested shape edits, each measured alone then combined, ALL byte-inert (sha 4a3cf430 == c2):
  s_disp (no `jitter` local; scale_vector3d(&up, real_seed_random_range(...)*jitter_offset, &displacement); /Od 0x79e125..0x79e173),
  s_centroid (set_real_point3d(&centroid,(x1+x0)*.5,...); /Od 0x79dbd9..0x79dc3a), s_jmul (`jitter_scale *=`; /Od 0x79d33d), s_jorder ((next-md)*fraction+md; /Od 0x79e0a4).
- /Od declaration orders (slot order = declaration order in the /Od frame), each alone on c3, ALL byte-inert:
  d1 vertex-loop (point,previous,next,up,width,adjusted_color,pixel,u); d2 end-bolt (vertices,bounds,centroid,shader,one_over,...);
  d3 else (octave_index before scale/up); d4 inner (displacement before fraction); d5 instance_index before points.
- LAB ORACLES (owner-gated class, not landable): d6 /Od dead `long X = point_count++` (/Od 0x79d4e7 stores old point_count to a never-read named 32-bit slot);
  d7 /Od dead `real = 0.5f` (/Od 0x79dd46 slot -0x2417c never read). Both byte-inert => P2 is NOT a declaration/name-count tie.
- ord2/ 66-run: vertex-2 all 24 permutations of {P,TX,C,TY} (P = point_from_line3d) x vertex-1 in {P-TX-C-TY, P-C-TX-TY}, plus vertex-1 18 non-P-first perms: best 2 hunks; none closes P2 (ord2_results.txt).
- Oracle scan (scan_pop.py, all January split objs) for `fstp st(0); <int>; fstp [m]`: only 3 sites (lightning, actor_moving _actor_move_calculate_controlled_by_aiming 0x211 EXACT, structure_visibility dequantize 0xee EXACT - different shape).
  actor_moving oracle: block `best_direction=direction; best_aim_dot=aim_dot(x87); best_facing_dot=facing_dot(int copy)` emits LOAD facing_dot FIRST, then fstp, then mov ebx,edx, then the store => loads float to the top of a block over earlier x87 stores and reg moves.
- MICRO-TU (micro/m0.c: only the vertex loop + centroid + draw in a fresh function, same TU headers): vertex-2 codegen differs (last
  component `fmul [up.k]` consumes t, NO `fstp st(0)`), and the reload lands BEFORE the TX2 fstp (January's relative order).
  => the reload-vs-TX2 order is sensitive to function context; micro is a lab to bisect toward the full function.
- tail.py compact emission census over ord2 outputs (Jan = `C1 TY1 Z1 TX1 NEG X Y Z POP RL TX2 C2 TY2 INC T`; ours `... Z POP TX2 RL C2 TY2 ...`):
  RL lands after POP (Jan position) whenever an integer store precedes TX2 in vertex-2 IL, but that store is then used as the
  fadd->fstp(Z) latency filler (`... C2 Z POP RL TX2 TY2`). Jan = P,C2,TX2,TY2 IL order with C2 NOT hoisted as filler (inferred).
  Hypothesis H-fwd: C2 in Jan is not a plain `= pixel` store available early (e.g. value-numbered/forwarded from a memory read), so it cannot fill.
- H-fwd REFUTED: f1-f3 `vertices->color = vertices[-1].color` is a real load (754 insns, 7-8 hunks), no forwarding.
- H-vi-ref REFUTED: t1 TX2=`vertex_index*one_over_point_count + u_offset` not CSE'd (2592 B); t2/t3 P via &points[vertex_index].position byte-identical to c3.
- Emission census (tail.py over ordout/ord2out, 60 variants) - decoded regularities (no scheduler in C2; dag.c emission):
  (1) vertex-1 C1/TY1 are ALWAYS pulled as fillers into the Z1 fadd->fstp stall, even when IL-after TX1 (TX1 = new x87 computation fld u/fadd).
  (2) vertex-2: TX2 (fstp of the long-lived CSE value uu) is emitted glued to POP (`Z POP TX2 RL`) whenever NO vertex-2 integer store was
      emitted while uu was on the x87 stack; if any vertex-2 integer store is emitted in that window (as Z filler or before NEG) the order is
      `... POP RL TX2 ...` (Jan's RL position) - but then that store is visibly displaced, which Jan does not show.
  (3) only ONE vertex-2 integer store is ever pulled as a Z filler; others are emitted after TX2 regardless of IL order.
  => Jan's `Z POP RL TX2 C2 TY2 INC T` is not reachable by any statement order of the {P,TX,C,TY} x {P,TX,C,TY} space with ordinary field stores.
- Board scan (scan_blk.py): 224 January blocks place a branch-feeding stack load BEFORE a trailing x87 store (common idiom, e.g. aim_assist
  0xb0 `fst angle; mov edi,[ebp+8]; fstp target->angle`), so Jan's RL placement is the compiler's usual preference; ours is the exception
  caused by TX2 being glued to the POP (rule 2).
- LAB name-count oracle nc/ (0..40 dummy extern declarations immediately before lightning_submit, on owner_gated_best): all 41 byte-identical
  (sha 4a3cf430) => P2 is NOT a TU name-count tie.
- k1 lab (integer use of vertex_index between P2 and TX2): not informative (reload moved, 2592 B).

## Storage finding (NEW, double-attested)
- cachebeta publics: `_lightning_globals` ABSENT (all 7 public fns + strings present) => file-static in January.
- HCEX.pdb DIA2Dump: `lightning_globals` DataKind 5 = **File Static**, type `struct <unnamed-tag>`.
- Split/symbols.json still say external (storage 2); production defines it external. object_audit passes today only because both sides are wrong.
- Fix (zero credit, byte-inert): lightning.c `static struct lightning_globals lightning_globals = {0};`, remove the `extern` from lightning.h
  (only lightning.c and widgets.c include it; widgets.obj rebuilt in place with the edited header: 20/20 sections identical; lightning.obj 34/34
  sections identical, storage 2->3), symbols.json line 23106 `"static": true` (427 data entries already use the flag) + csplit regen.
  Files: storage.patch (git apply --check OK), config/symbols.json (edited in place, CRLF preserved).

## Deliverables
- production.patch = storage fix + byte-inert /Od-attested shape edits (c4: s_disp, s_centroid, s_jmul, s_jorder, d1-d5 orders, adjusted_color name);
  lightning.obj sections 34/34 identical to production (submit sha 9885e979 unchanged => park hash unchanged); /W3 only the inherited C4146/C4244.
- storage.patch (storage only), od_shape.patch (inert edits only) - each `git apply --check` clean.
- owner_gated_best.c = production.patch + `up = *(real_vector3d const *)global_z_axis3d;` + point_from_line3d(position,&up,+/-width,&vertices->point):
  submit 2576/80, ONE adjacent transposition left (Jan `POP; mov ecx,[ebp-8]; fstp [esi+0xc]` vs ours `POP; fstp [esi+0xc]; mov ecx,[ebp-8]`).
  Its `_point_from_line3d` COMDAT is byte-identical to January's selected copy (action_charge, 48 B, 9b763841) and provider_link PASSES,
  but rule 6 exception also needs a strictly exact caller => not landable while P2 remains.

## Final state / stop
Stop rule fired: >100 evidence-based shapes; the only difference left (with the owner-gated cast) is a dependency-free reload/x87-store
transposition decided by dag.c emission (rule (2) above), unreachable by statement order, declaration order, name count, or helper spelling.
Reopen: (a) owner ruling admitting the /Od-attested aggregate view copy for the axis fallback, AND (b) a decoded C2 emission rule (or a first-party
January-era source fragment) that detaches the TX2 fstp from the POP without displacing C2/TY2 - e.g. a live C2.dll trace of the uu fstp
emission decision on owner_gated_best vs the p_P-TX-C-TY_P-C-TX-TY variant (only C2 differs).
