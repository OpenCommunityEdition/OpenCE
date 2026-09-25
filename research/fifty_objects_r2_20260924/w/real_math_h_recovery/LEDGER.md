# LEDGER - January real_math.h content recovery (r2 research, LAB ONLY, read-only)

Slug: scratch/w/real_math_h_recovery/. Tree 42125a52 (clean). math/real_math RESERVED for the Codex packet:
nothing here is proposed for landing; real_math_jan.h is a NOT-landable lab header.

## Step 0 - read brief + canary lane (LEDGER F1-F9, MANIFEST, lab/window/grid/netcount/rerun_packet)
- Currency = C1 symbol records (names, params, locals, distinct string literals, if/&& labels, distinct files).
- draw (frame_statistics) = mod-64 counter of records before the .c globals block; +17 short of January (F9).

## Step 1 - January assert anchors (anchor_scan.py --tree split -> jan_anchors.txt; 833 January objects)
- Scanner: every DIR32 reloc whose target literal (defined bytes, or demangled ??_C@ name when external) ends in
  real_math.h; line = imm pushed before the file push; expr = literal pushed after it.
- COMPLETE January anchor set (10 hits, 8 distinct lines, all "..\math\real_math.h"):
  848 projection_sign_from_vector3d (decals COMDAT; inlined x2 in collision_bsp sphere/pill recursive)
  859/860 project_point3d (path_obstacles COMDAT)   879/880 project_point2d (decals COMDAT)
  1508/1509 uniform_cubic_spline (following_camera COMDAT)   1530 nonuniform_cubic_spline (glow COMDAT; expr "t>= t0 && t <= t3")
- Ours: 997 / 1014-1015 / 1035-1036 / 1550-1551; ours has NO 1530 assert (junk body). Offsets ours-Jan:
  +149 / +155 / +156 / +42. Spans: 848->859 Jan 11 ours 17 (ours has 7-line #ifdef EXTERNAL_PROJECT_POINT3D block ->
  Jan 1 line MORE than ours minus wrapper); 859->879 Jan 20 ours 21 (+1 = our #endif); 880->1508 Jan 628 ours 514
  (Jan +114; ours carries 59 lines of campaign #ifdef REAL_MATH_EXTERNAL_* wrappers in this span, so January had
  ~173 more lines of genuine text there); 1508->1530 Jan 22 (ours: nonuniform assert absent).

## Step 2 - later-build anchors (/Od halo_cache_symbols.exe via odbuild.py str "real_math.h"; HCEX.pdb -lsrc)
- /Od file literal "D:\P4\Halo1\source\math/real_math.h": projection_sign 871 (0x56d950), project_point3d 882/883
  (0x4c9600), project_point2d 902/903 (0x56d680), uniform_cubic_spline 1549/1550 (0x52b110),
  nonuniform_cubic_spline 1571 (0x79cad0). => later = January +23 before Jan 848, spans 848->859 (11), 859->879 (20),
  1508->1530 (22) IDENTICAL to January; span 880->1508 later +18 (646 vs 628).
- HCEX.pdb (DIA2Dump -lsrc d:\projects\code\hcex\sources\math\real_math.h, MD5 DE0F004C...; hcex_lines.py ->
  hcex_helpers.tsv): only 13 out-of-line helper copies carry real_math.h line records (PPC /O2 inlines the rest):
  normalize2d 753-766, projection_from_vector3d 860-868, project_point2d 901-913, normalize3d 1003-1016,
  fast_normalize3d 1020-1029 (!), distance_squared3d 1050-1054, plane2d_from_points 1253-1267,
  uniform_cubic_spline_vector3d 1597-1601, nonuniform_cubic_spline_vector3d 1614-1618, valid_realcmp 1623-1630,
  valid_real_normal3d 1679-1681, valid_real_vector3d_axes3 1712-1714, dequantize_byte_to_real_rectangle3d 1734-1743.
  project_point2d 901-913 brackets the /Od 902/903 assert pair => HCEX header == /Od header revision in this range.

## Step 3 - HCEX_Release.pdb (RELEASE_CACHE) line records: 96 out-of-line real_math.h helpers (hcexrel_helpers.tsv)
- Same header MD5 DE0F004C as HCEX.pdb; release flavour keeps nearly every helper out of line => near-complete ORDER and
  `{` line map of the 2011 real_math.h (entry record = the `{` line; verified on sine/cosine spacing vs ours).
- HCEX == /Od + 3 lines at all five /Od anchors (projection_sign `{`873 -> assert 874 = /Od 871+3; project_point3d
  `{`884 -> 885/886 = 882/883+3; project_point2d `{`901, locals 902-903, asserts 905/906 = 902/903+3; uniform `{`1551
  -> 1552 = 1549+3; nonuniform `{`1573 -> 1574 = 1571+3). So Jan->HCEX: +26 before 848, +18 more in 880..1508.

## Step 4 - January-attested helper inventory + ORDER (jan_inlines.py -> jan_inlines.txt; cachebeta publics x contribs.json)
- 93 distinct real_math.h-family helper names are emitted out of line somewhere in January (cachebeta publics = January
  names; rva == file_offset). VC7 emits the not-yet-emitted inline callees immediately BEFORE the first function that
  needs them, as a batch in HEADER DEFINITION ORDER (decals batch of 8, action_charge batch of 13 ... all monotone with
  ours and HCEX). Batches are January ORDER constraints; no batch contradicts our order.
- January helpers ABSENT from our real_math.h (all January copies EXACT in our build as consumer-.c bodies):
  vector_intersect_plane2d (geometry batch [plane2d_distance_to_point, vector_intersect_plane2d, vector_intersect_plane3d];
  ours = plain def in geometry.c), valid_real_vector2d (players batch [random_direction3d, valid_real_vector2d]; ours =
  plain def in players.c), valid_real_normal2d (actor_looking; ours = .c __inline + actor_looking.h proto),
  valid_real_plane3d (matrix_math #33; ours = matrix_math.c def + real_math.h proto; canary packet vp).
- fast_normalize2d/3d, cross_product_magnitude3d, dequantize_byte_to_real: January real_math.obj #24-#27 in .c definition
  order between pill_intersects_rectangle3d and signed_angle_between_vectors2d => .c functions in January (as ours).
  HCEX has them as header inlines (770 / 1020 / 1107 / 1726) => LATER additions (explain HCEX +14/+24 lines).
- nodup_census (nodup_helpers.txt): 16 January helpers live as NODUP .c copies in our build.

## Step 5 - prototype inventory (math_publics.py -> math_publics.txt)
- Every one of our 119 real_math.h prototypes names a January public. January publics of real_math/matrix_math/
  random_math with NO prototype in our real_math.h: vector_intersects_triangle2d, vector_intersects_triangle3d,
  pill_intersects_rectangle2d, line_from_planes3d (real_math_planes.h), pin_normal_to_cone3d (real_math_cones.h),
  matrix3x3_determinant, matrix3x3_inverse, matrix3x3_from_axis_and_angle, matrix3x3_multiply (matrix_math.h),
  matrix3x3_rotation_to_quaternion (vehicles.c calls it with NO declaration: C4013), get_random_seed, random_seed_debug_log
  (random_math.h), get_number_suitable_for_initializing_random_seed, random_math_initialize, random_math_dispose (real_math.c
  calls them undeclared), seed_random_orientation.  = 16.
- HCEX include graph (DIA2Dump -sf): 2011 math headers are ONLY geometry.h, integer_math.h, periodic_functions.h,
  real_math.h, vector_tree.h. matrix_math.h, random_math.h, real_math_planes.h, real_math_cones.h,
  real_math_declarations.h do not exist (campaign-invented; each also costs +1 file record in every TU including it).

## Step 6 - lab harness + record calibration (jan.py edit catalogue, measure.py, probe.py; frame_statistics draw counter)
- Controls: base net 0; rlr +1, vp +1, spl1 0, spl3 -11 (== canary lane).
- CALIBRATION (probe.py, prototypes/defs at the MATRIX_MATH prototype anchor): prototype `f(void)` = 2, n NAMED params
  = 1 + 2n (3,5,7,9,11,13 for n=1..6), n UNNAMED params = 1 + n. Definition = prototype + 1: void 3, n params 2 + 2n.
  `return a && b` = +0; `if` = +1. => a named parameter costs 2 records (not 1).
- New edits (frame_statistics TU, true values; netcount mod-64 unwrapped with the calibrated model):
  vip2 +8, vrv2 +4, vrn2 +4, vcol +8, prm +53 (meas -11), pmm +31, prnd +18, noext 0.
  Combos: JAN bodies (spl1,spl3,rlr,vp,vip2,vrv2,vrn2) = +7 (meas 7); + prm,pmm,prnd = +109 (meas -19);
  + vcol = +117 (meas -11). All consistent with the additive model.
- ACCOUNTING vs canary F9 (ours carries +17 known surplus in this TU yet == January mod 64 => January has +17 mod 64 of
  content we lack): the three January-attested missing helper bodies vip2+vrv2+vrn2 = +16 => residual -1 (mod 64).
  Adding the 16 January-public prototypes (+102) would leave -27/+37: NOT consistent unless other content compensates.

## Step 7 - CORRECTION of the canary F9 accounting + /Od locals
- vt (the 6 campaign verify typedefs in rasterizer_frame_statistics.c) measures -17 records, NOT -6 (typedef name +
  offsetof/?: expansion records). swcw -2 (redeclaration costs params only: 2/param).
  => ours carries +17 (typedefs) +11 (spline junk) +2 (stack_walk dup) -2 (rlr,vp missing) = +28 known surplus in the
  draw TU; draw exact => January has +28 (mod 64) records of content we lack there (not +17).
- JAN bodies (spl1,spl3,rlr,vp,vip2,vrv2,vrn2) + vt + swcw = net -12 measured (measure_tree.py) => residual +12 (mod 64).
- /Od (later build) named-local census of the random wrappers (odbuild fn): real_random 0x42e1c0, random 0x58c100,
  real_random_range 0x42f360, random_direction3d, *_vector_in_cone3d: NO local slot. real_local_random 0x573ae0,
  real_local_random_range 0x521410 (ours has it), local_random 0x551170 (word slot, RTC-initialised), local_random_range
  0x566650 (word slot), random_range 0x43e850 (word slot): named-local shape. NB modern /Od also spills SSE float
  returns through an RTC-initialised [ebp-4] temp (dot_product2d 0x42dd30), so only CALL-result spills are evidence.
- lrl / lrrl / rrl (named local in local_random / local_random_range / random_range): +1 each; whole-board sweeps
  (sweep_S_*.txt): January COMDAT copies (_local_random debug_memory, _local_random_range contrails, _random_range
  action_obey) stay EXACT = byte-inert => LATER tier only, cannot be double-attested by January bytes.

## Step 8 - HCEX body spans vs ours (body_spans.txt) + /Od float-temp caveat
- Many trivial helpers are 4 lines LONGER in HCEX (dot_product2d/3d, magnitude_squared2d/3d, magnitude2d/3d, cross_product2d,
  valid_realcmp ...) while January's copies of the same helpers carry NO assert (anchor scan: only 8 January lines) =>
  HCEX added no-code lines (asserts compiled out in RELEASE) AFTER January. HCEX line spans are therefore NOT evidence of
  extra January locals; only the /Od integer call-result spill is (random_range/local_random/local_random_range).
  vector_intersect_plane3d HCEX body 12 lines (code only at the return) = later asserts; January copy has none.
- /Od: modern MSVC spills SSE<->x87 conversions through RTC-initialised [ebp-N] temps (reciprocal_square_root 0x58a4c0,
  point_to_line_distance3d? 0x42e150 two slots) => float-valued "locals" in /Od are NOT evidence; integer ones are.
- Whole-board grid g1 (grid_g1.txt): P0 (JAN bodies+companions) drawnet +7, +_collision_move_point 4752 B, lost 5;
  P1 (+swcw,vt) -12; P2 (+lrl,lrrl,rrl) -9; P3 (+vcol) -1 (lost 7); P4 (P1+16 prototypes) +26 (lost 8). None zero-loss.

## Step 9 - deliverables
- real_math_jan.h (mk_jan_header.py): ours + JAN tier (spl1, spl3, rlr, vp, vip2, vrv2, vrn2); LATER (lrl, lrrl, rrl)
  gated by REAL_MATH_JAN_LATER; INF (vcol, prm, pmm, prnd) gated by REAL_MATH_JAN_INFERRED. Compiles in all 4 modes;
  frame_statistics net +7 / +10 / -11(+117) / -8(+120) (measured on the final file).
- tu_net.py (header-context counter: unit's #include + EXTERNAL defines prepended to frame_statistics.c): JAN +7 in
  FS/geometry/players/aim_assist/physics/game/particles/actor_combat, +6 actor_looking/units (redeclaration), +8/+9
  collisions/bipeds with noext (EXTERNAL proto->def). tu_net_results.txt.
- grid_g2: R0 (JAN+noext) gains move_point, loses 7 incl. biped_update_airborne/jumping (noext codegen) -> noext NOT in
  the lab header default. No zero-loss combination found.
- january_real_math_map.md written (anchor map, 98-row inventory, 34 January batches, prototypes, calibration, F9
  correction, per-TU deltas, sweeps, open items, consumer sweep list).
- Cleanup: large DIA dumps deleted (regenerate: DIA2Dump -sf / -t / -lsrc; commands in hcex_lines.py docstring).
