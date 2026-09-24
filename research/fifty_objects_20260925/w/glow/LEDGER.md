# glow (source/objects/widgets/glow) - wave worker ledger (claude/fifty-objects-20260925)

Tree 931ed8dc. Target: close `_get_particle_world_position` (Jan 1744/43, frame 0xc4; ours 2528/55, 0xf4).

## Prior evidence read (do-not-repeat)
- glow_obj_house_rule_reconciliation_20260908: park origin (defined sides/knots + staging array).
- glow_obj_opus5_150k_w1_20260914: owner_candidate.c EXACT (reproduces sides[] never filled on 2/3-marker
  paths, knots[2]-for-knots[1] typo, point->vector casts). Do-not-repeat: four-statement knot copies,
  bound marker pointer, cross-product reordering, unnamed cos/sin, any defined-C repair aimed at exactness.
- glow_obj_opus5_250k_w3d_20260915: reloc census +12 = exactly the side-control interpolations.
- glow_obj_opus5_next150_n3a_20260915: v3 (cheapest defined fill at Jan frame+43 relocs) still +592 B;
  precondition branch refuted; cast branch admitted. Offered v2 (2464/55, 0xb8) zero-credit rebaseline.
- Owner ruling 20260920 (lane A memory): uninit/OOB read admissible as target-proven BUG ONLY once strict
  exact, with source comment + compile-time layout assertion. Brief s7 (this lane): uninit reads are
  owner-gated - report, do not self-land.

## Probes / findings
- F0 baseline (931ed8dc, real file): gate --all = exact 24 / residual 1 / unwritten 0; residual
  `_get_particle_world_position [size 2528!=1744, relocs 55!=43, sha]`. glow.c unchanged since the
  n3a owner candidate except this one body (diff vs owner_candidate_n3a.c = this function only).
- F1 HCEX.pdb (DIA2Dump -sym, scratch/w/glow/hcex_gpwp.txt): locals traversal_points real_point3d[4],
  traversal_left_vectors real_vector3d[4], traversal_up_vectors real_vector3d[4],
  traversal_time_indices float[4], u, v real_vector3d (v shares 0xC0 with traversal_time_indices =>
  v is the LAST spline result = left vector; u = up result). No delta/position staging array listed.
- F2 NEW /Od readout (odbuild fn 0x798de0 -> scratch/w/glow/od_gpwp.txt; assert lines 0x438/0x43c/0x49d):
  * 2-marker arm: copies pos[0],pos[3],ups[0],ups[3],knots[0],knots[3]; vector_from_points3d(&pos0,&pos3,&forward)
    + point_from_parametric_line(0.25/0.75) on points, same pair on ups; knots[1],knots[2] lerps.
    NO cross_product3d and NO write of traversal_left_vectors[] at all.
  * 3-marker arm, both intervals: same - no left-vector writes. Interval 1: pos[2],ups[2],
    knots[2]=mti[1], vfp/pfpl pairs, then knots[2]=(knots[2]-knots[0])*0.5+knots[0] (store to
    [ebp+8-0xc0] = knots[2]) => the knots[2]-for-knots[1] typo is IN THE LATER FIRST-PARTY SOURCE.
  * default arm loop: short local = marker_order[first+index]; pos[i], ups[i] copies;
    cross_product3d(&up,&forward,&left[i]) REAL call; knots[i]=mti[first+i] LAST in loop body.
  * spline 1 writes straight into particle+0x2c with &traversal_points[] as vector args (cast form);
    spline 2 -> [ebp-0xe8], spline 3 (reads traversal_left_vectors) -> [ebp-0xfc], UNCONDITIONAL.
  * RTC descriptor: traversal_points 48, traversal_up_vectors 48, traversal_left_vectors 48,
    traversal_time_indices 16, forward 12 (= the vector_from_points3d delta), two unnamed 12s.
  => Both January defects (left controls never filled on 2/3-marker paths; knots[1] unset on 3-marker
     interval 1) are double-attested by the later first-party build: they are authentic source text,
     not a January compile artifact. The defined-C route stays closed (n3a v3: +592 B at Jan frame+relocs).
- P1 owner candidate (n3a owner_candidate_n3a.c, copied to cand_owner.c) at 931ed8dc: gate --all + point guard = EXACT 25/0/0; _get_particle_world_position EXACT 1744.
- P2 authentic names (HCEX/RTC: traversal_points/up_vectors/left_vectors/time_indices, forward=delta, u=up result, v=left result): EXACT 25/0/0 (byte-inert as expected).
- P3 /Od-attested short marker-order local (/Od [ebp-0x108]) as a BLOCK-scoped `short ordered_marker_index`
  in the default loop: residual [sha], 2 REAL regions: final-expression x87 schedule at +0x670
  (fld [ebp-0x94]/fmul st(2) vs fmul st(1)/fld/fmul st(3)) - M8 ordinal shift. January-refuted at block scope.
- P4 /Od-attested `marker_index = PIN(...); particle->parent_marker_index = marker_index;` (dead store in the
  optimized build): EXACT 25/0/0, byte-inert. /Od 0x798f04 shows exactly this two-statement form.
- P5 same short local at FUNCTION scope (declared after marker_index, /Od slot order): identical 2 REAL
  regions (+0x670 x87 schedule). The /Od marker-order local is January-refuted in both scopes -> later-revision
  addition; keep the no-local loop (w1 c1/d2 form). Stop probing this local.
- P6 cand_final.c = authentic names + /Od PIN form + rewritten BUG comments (whole-TU = real file with only this body replaced): EXACT 25/0/0, point guard pass. Rows: cand_final.rows.
- F3 /Od 0x79ce50 (8 callers, all in 0x798de0) = point_from_parametric_line(point, vector, t, out) with the
  z-component added to point->y (`addss xmm0,[ecx+4]` for the +8 store) - the y-base quirk is the helper's
  own body in the later build too; its callers pass &traversal_up_vectors[] (per-site /Od attestation of the
  point<->vector view sites).
- P7 STRIP TEST (casts): removing every (real_point3d *)/(real_vector3d *) view cast in the body -> still EXACT 25/0/0. The casts are type-only (byte-inert), NOT load-bearing.
- A1 section-4 audit on cand_final.obj: object_audit PASS (36 January symbols, 0 differ; every January .text/.rdata
  section ok). Candidate-only surplus = the SAME list as build/base (10 real_math.h SELECT_ANY code COMDATs +
  real_math.h assert string + __real constants + .drectve). surplus_identity (candidate obj): 10/10 IDENTICAL to
  January's selected providers. provider_link cand_final.obj: SELECTED-PROVIDER LINK PASS (all 20 surplus).
- A2 pdb_storage: 36 split symbols, 0 disagreements. Data: build/report.json glow .rdata 392/392 = 100% (only string
  literals; no .data/.bss). No object_admission_rejections entry. fake_match_scan cand_final.c: 0 leads.
- A3 production.patch written (glow.c body -> cand_final body; parked.json glow entry removed; config.json glow.c
  NonMatching -> Matching). `git apply --check` clean at 931ed8dc. OWNER-GATED (brief s7: uninitialised reads).
- P8 fallback (only if the owner DECLINES the BUG form): cand_fixed.c = cand_final with both defects repaired in the
  /Od idiom (cross_product3d endpoint left vectors + vector_from_points3d/point_from_parametric_line interior left
  vectors; midpoint knot assigned to [1]). 2544/55, frame equal to January (sub esp not in the diff), 745 vs 566
  insns, 71 REAL regions; 24 siblings EXACT. Same shape class as n3a v1. Zero credit; defined behaviour.
- A4 patch application test (fresh copy + git apply): glow.c == cand_final.c, parked.json/config.json == new/ copies.

## Verdict
FUNCTIONS_EXACT_OBJECT_BLOCKED (owner gate). cand_final.c closes glow.obj 25/25 strict exact with a clean section-4
audit, but only by preserving two authentic original defects (uninitialised traversal_left_vectors on the 2/3-marker
paths; traversal_time_indices[1] unset on 3-marker interval 1). Brief s7 makes preserved uninitialised reads
owner-gated. NEW this wave: the later first-party /Od build (0x798de0) contains BOTH defects verbatim, plus the
vector_from_points3d/point_from_parametric_line routing on the up vectors and the `forward` delta local; HCEX gives the
authentic local names used in cand_final; the point/vector casts are byte-inert (strip test P7).
Defined-C route: closed (n3a v3 +592 B at January frame+relocs; P8 corrected body 2544/55, 71 REAL regions).
Reopen / land: owner admits the two BUG-commented defects for this function -> apply production.patch as is.
If declined: optional zero-credit park rebaseline to cand_fixed.c (defined, /Od-faithful, frame-exact) - integrator's call.
Do-not-repeat (new): /Od short marker-order local in the default loop (block or function scope) -> +0x670 x87 flip.
