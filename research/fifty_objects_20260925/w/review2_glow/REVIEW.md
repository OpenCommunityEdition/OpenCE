# REVIEW2 glow (adversarial) - source/objects/widgets/glow, verdict under review FUNCTIONS_EXACT_OBJECT_BLOCKED

Tree 931ed8dc. Reviewer works only in scratch/w/review2_glow/.

## Log
- R0 read WORKER_BRIEF (s0-s7), house rules, campaign log, glow ledgers w1/w3d/n3a, worker LEDGER, patch, od_gpwp.txt, hcex_gpwp.txt.
- R1 `git apply --check scratch/w/glow/production.patch` at 931ed8dc: clean. Trial apply on fresh copies (scratchpad): glow.c == cand_final.c, parked.json/config.json == glow/new/config copies (cmp); both JSON parse.
- R2 gate baseline (real file, guard): exact 24 / residual 1 (`_get_particle_world_position [size 2528!=1744, relocs 55!=43, sha]`) - matches worker.
- R3 gate cand_final.c (copy in my slug, --all --forbid-emitted-symbol _point_from_line3d): exact 25 / residual 0 / unwritten 0; guard passed; _get_particle_world_position EXACT 1744.
- R4 object_audit cand_final.obj: PASS, 36 January symbols 0 differ, every .text/.rdata ok. Candidate-only surplus list byte-identical to the surplus of build/base glow.obj and of my base.obj (diff of the surplus block: only the PASS/FAIL line differs).
- R5 surplus_identity (my CAND_OBJ variant, 1-line diff from scratch/tools/surplus_identity.py): 10 code COMDATs, 0 not identical. provider_link cand_final.obj: SELECTED-PROVIDER LINK PASS (20 surplus symbols, both orders).
- R6 pdb_storage: 36 split symbols, 0 disagreements. _get_particle_world_position absent from cachebeta publics (static; candidate keeps `static`); symbols.json line 5061 "static": true. No symbols.json change needed.
- R7 January bytes (my own disasm, jan_gpwp.txt): [ebp-0xc4..-0xa0] appear ONLY as the four leas feeding spline 3 (+0x637..+0x64c) plus the default-loop indexed stores [ebp+edx-0xc4/-0xcc/-0xc8] (+0x21b/+0x23e/+0x259). 3-marker case 1: fld [esi+0x23c] (+0x2ea, marker_time_index[1]) stays on x87, midpoint fsub [ebp-0x10]/fmul 0.5 then jmp +0x5c3 -> fstp [ebp-8] (+0x5c6); [ebp-0xc] never stored in case 1, read at +0x5d2. z components use [ebp-0x4c] (points[0].y) base = y-base quirk. All BUG-comment offsets verified.
- R8 /Od (odbuild) callee identities: 0x42e3a0 = vector_from_points3d (b-a into out), 0x43e710 = cross_product3d, 0x79ce50 = point_from_parametric_line with y-base z, 0x455320 -> 0x4552f0 -> 0x455430 -> ucrtbased!sin (resolved via IAT 0xf56b8c) computed FIRST into [ebp-0x11c]; 0x455220 -> ... -> ucrtbased!cos into [ebp-0x120]. So /Od attests two homed trig locals, sin first; u(spline2)*sin, v(spline3)*cos. Consistent with candidate. NOTE: the 3-level wrapper chain is the shape of the real_math.h sine()/cosine() __inline (-> <cmath> float overload -> sinf) in a C++ /Od build, i.e. the later source likely spelled sine(angle)/cosine(angle); the candidate keeps the base's (real)sin/(real)cos and names its locals `sine`/`cosine`, which SHADOW the real_math.h helpers of the same names.
- R9 /Od default loop bound: `movsx ecx,[ebp-0x10c]; cmp ecx,4; jge` = SIGNED compare -> later source had a signed bound (literal 4), not NUMBEROF (size_t -> unsigned jae). Base had `index < 4`; candidate changed it to NUMBEROF(traversal_time_indices). To be strip-tested.
- R10 STRIP TESTS (all gate --all + guard, variants built mechanically from cand_final.c):
  * s1_nocasts (every (real_point3d *)/(real_vector3d *) view cast removed): 25/25 EXACT -> casts byte-inert. /W3 /Zs: plain form raises C4133 incompatible-types; cand_final has 0 TU-local warnings, so the casts are the type-correct spelling, not decoration.
  * s2_pin1 (one-statement `particle->parent_marker_index = PIN(...)`): 25/25 EXACT -> the /Od two-statement form (0x798f04: store to [ebp-0x104] then to particle+2) is byte-inert, not a match lever.
  * s3_lit4 (`index < 4`, base + /Od signed jge spelling): 25/25 EXACT -> NUMBEROF is byte-inert; candidate's NUMBEROF is contradicted by /Od (jge = signed bound) - minor authenticity nit.
  * s4_notrig (drop named sine/cosine, inline (real)cos/(real)sin in each expression): 24/1, `_get_particle_world_position [sha]` -> the trig locals ARE load-bearing, but /Od attests two homed trig results (sin first, used 3x each), so they are attested locals, not fillers.
  * p5_helpers (genuine real_math.h sine()/cosine() with non-shadowing local names): function bytes identical (25/25) BUT adds candidate-only _sine/_cosine COMDATs (16 B each) to the surplus -> NOT preferable to the candidate's (real)sin/(real)cos spelling without a provider census. Informational only.
- R11 fake_match_scan cand_final.c: 0 leads (base 0). /W3 /Zs TU-local warnings: 0 in cand_final and base.
- R12 /Od double-attestation of both defects re-checked: [ebp-0xa8] (traversal_left_vectors) referenced only by the default-loop cross_product3d push (0x799887) and the spline-3 leas; 3-marker case 1 writes time_indices[2] (index 8) twice and never [1] (0x799533, 0x799639).
- R13 git diff --check simulation (isolated scratchpad git repo seeded with the CRLF working-copy files, autocrlf=true like the project): patch applies, `git diff --check` clean; stat 3 files +106/-164.
- R14 other references: `_get_particle_world_position` appears only in config/parked.json and config/symbols.json (5061, static:true, unchanged). No glow entry in object_admission_rejections.json. build/report.json glow data 392/392 (100%), .rdata only.
- R15 FALLBACK cand_fixed.c re-gated: 24/1, `[size 2544!=1744, relocs 55!=43, sha]` (matches worker). DEFECT in the fallback: it fills the interior left vectors through point_from_parametric_line, whose body is `result->z = forward->k * t + point->y` (y-base quirk, glow.c:453). So traversal_left_vectors[1/2].k get left[0/1].j as base - a NEW wrong computation that neither January nor the /Od build contains (neither ever interpolates left vectors), and a behavioural regression versus the current parked body, which interpolates sides with the correct .k base. It also is 16 B LARGER than the current 2528 park body. The fallback is therefore not a "defined corrected body"; it must not be adopted as written (fix: interpolate left vectors with a correct z base, or keep the current park / n3a v2).
- R16 negative probe re-check: scratch/w/glow/cand_order.c (the /Od short marker-order local) = 24/1 `[sha]` - worker's refutation reproduces.

## Verdict
CONFIRMED: FUNCTIONS_EXACT_OBJECT_BLOCKED (owner gate). approve = true for the PRIMARY packet as an OWNER-RULING
packet only (production.patch must NOT be applied without the owner explicitly admitting the two preserved
uninitialised reads for this one function, brief s7). approve = false for the FALLBACK (cand_fixed.c).

Primary packet, independently verified at 931ed8dc:
- gate --all + point guard 25/0/0; object_audit PASS (36/0); surplus identical to build/base, 10/10 COMDATs identical,
  provider_link PASS (20 symbols, both orders); pdb_storage 0 disagreements; data 392/392; no rejection entry;
  fake scan 0; /W3 0 TU-local warnings; git apply --check clean; trial apply byte-identical to the candidate files;
  diff --check clean.
- Both defects triple-attested (January bytes R7, /Od R12, HCEA per n3a). All BUG-comment offsets correct. BUG
  comment format matches docs/matching_methodology.md "Original-bug policy".
- Strip tests: casts, PIN two-statement form and NUMBEROF are byte-inert; named trig locals are load-bearing but
  /Od-attested (2 calls homed and reused 3x, sin first). forward/u/v/traversal_* names are RTC/HCEX-attested.

Non-blocking notes for the integrator/owner:
1. `index < NUMBEROF(traversal_time_indices)` is byte-inert (s3) but /Od shows a SIGNED `cmp 4; jge` bound, and the
   base spelled `index < 4`; keeping `4` would be the /Od-faithful spelling (house rule 18 permits either).
2. Locals `sine`/`cosine` shadow real_math.h's `sine()`/`cosine()` inlines (legal, no warning); the /Od chain
   0x455320->0x4552f0->0x455430->ucrt sin is the shape of `sine(angle)`, so the later source's locals had other
   names. Spelling via the helpers is byte-identical for the function but adds _sine/_cosine surplus COMDATs (p5),
   so the candidate's (real)sin/(real)cos is the safer form. Renaming the locals is byte-inert.
3. The lane-A owner precedent admitted an uninit/OOB BUG "with a compile-time layout assertion". These reads are of
   uninitialised automatic arrays with no struct-layout dependency, so no layout assertion applies - flag for the
   owner in case they want one anyway.
4. BUG comment range "[ebp-0xc4..-0x94]" is end-exclusive (-0x94 is `u`); cosmetic.

Fallback cand_fixed.c: REJECT as written (R15) - propagates the point_from_parametric_line y-base z quirk into
newly invented left-vector interpolation (wrong .k base), a behaviour regression versus the current park body, and is
16 B larger (2544 vs 2528). If a zero-credit rebaseline is wanted, fix the left-vector z base first or use n3a v2.
