# REVIEW - source/render/render_debug packet 05 + 06 (r2w2), reviewer review_r2w2_render_debug_rv1

Base: HEAD 434f0151 (tree 05255584 + ledger doc). Patches under review:
- 05 sha256 1303b91b74576ce8778cb1806408c4c860ed01f5dd4890209238153127b945c9 (verified)
- 06 sha256 c8a386b4a5b3c961aaafbc5a6b5c972075f50f75041f81591187ce2a394db4fc (verified)

## Log (appended as I go)
- Patches applied (GNU patch -p1) to HEAD blobs in R/ (c06), R05/ (c05), R0/ (prod): R == worker files/c06 modulo EOL.
- git apply --check at HEAD 434f0151: 05 rc 0, 06 rc 0 (alone). git status unchanged.
- Independent compile (rv_compile.py: build.ninja cflags, cwd root, leading /I shadow of R*/source; /showIncludes
  confirms the shadow vehicle_datum.h is the one included; prod does not include it).
  - prod.obj == build/base render_debug.obj (104 sections, 0 differ, symbol table identical) => harness faithful.
  - c06.obj == worker sweep/c06 render_debug.obj (105 sections, 0 differ, symtab identical).
  - prod vs c05: section multiset differs ONLY by one inserted .text 48 (_point_from_line3d); order otherwise equal.
  - c05 vs c06: 0 sections differ; only 6 symbols _debug_* value 0 -> 1 (COMMON size 1), section 0, storage 2.
- Rows (gate.py row logic, rv_score.py): c06 36/36 EXACT; every row (size/relocs/sha) identical to prod/base.
  vector 192/13 70cfea13919c, tick 112/1 7bf04865f13f, line_offset 128/2 b7c236b7adcb, player 272/16 a45fe6649ea6.
- object_audit c06: PASS, 63 January symbols 0 differ; only surplus delta vs base is +.text _point_from_line3d 48.
- pdb storage (candidate): 63 split symbols, 0 disagreements; the six COMMON toggles are all cachebeta PUBLIC.
- surplus identity (candidate): 12 code COMDATs, 0 not identical; _point_from_line3d == action_charge (January).
- provider_link full: PASS (39 rows); --baseline base: only _point_from_line3d vs action_charge, PASS both orders.
- nodup_census _point_from_line3d: 26 definers in build/base, 0 NODUP.
- 06 link probes (rv_link.py, temp output): c06+January linker_common (both orders) 0 dup, 0 unresolved toggles;
  c06+base hs_globals_external (both orders) 0 dup, 0 unresolved toggles; control c05+hs_globals_external: 12
  unresolved toggle lines. January linker_common records: permanent_decals 50, input 51, bsp 52, structure 53,
  player 54, camera 55 (storage 2, .bss).
- Sweep scope: only render_debug.c + units/vehicle_datum.h change; vehicle_datum.h is included by exactly
  actor_moving.c, physics.c, vehicles.c (vehicles via "vehicle_datum.h" same-dir) and no header. Shadow tree T
  (copy of source/ + libs/ with both overlays, cwd = T, build.ninja cflags): actor_moving, physics, vehicles
  IDENTICAL to build/base; render_debug(T) IDENTICAL to c06. => zero regressions in every TU whose input changed.
- /Od re-read (odbuild fn): 0x84a920 vector asserts push 0x184/0x185/0x186 then real call 0x42e0d0(point, vector,
  size, &end_point[ebp-0x14]); 0x84a510 tick: two real calls (point, tick_vector, tick_size, &point0[-0x14]) and
  (point, tick_vector, xorps(-tick_size), &point1[-0x28]); RTC shows only two 12-byte aggregates (no negative temp);
  0x848640 line_offset: (p0, *0xa31e2c global_up3d, offset, &point0), (p1, global_up3d, offset, &point1).
  0x848d00 player: vehicle = fn 0x7820d0(parent_object_index, mask 2 = vehicle), stuck = dword [vehicle+0x4dc] != 0
  (later layout, +0x64 shift: speed 0x490/slide 0x494/turn 0x498 <-> January 0x42C/0x430/0x434, stuck 0x478).
- 06 evidence re-verified: January referencers of all six = {hs_globals_external, render_debug} only; no base
  definer. Board census (hs_pair_census_rv.txt): 242 pool records, 74 {hs_globals_external, X}; canonical defines
  15, all in X (units 8, objects 5, structure_visibility 2), 0 outside X. Maps: 2001-08-15 beta five toggles
  `<common>`, 2001-09-25 beta six `<common>` + _render_debug_structure_decals; 2001-09-25 retail cache.map: 0
  render_debug.obj rows, six toggles absent, hs_globals_external carries the debug_* name literals, _debug_objects /
  _render <common> present. /Od: toggles at 0xf0afb0-b3,b6,b7 interleave with render_debug cache vars b4,b8,bc,c0.
  Players COMMON precedent 6e3e2d35 (canonical) and objects.c fb0d8145 confirmed.
- objdiff 3.3.1 mini (mine): prod / c06 / l2d identical: code 8235/9448, 34/36, data 30236/30236.
- /W3: only new warning c06 vs prod is real_math.h(764) C4244 (arctangent inline body visible).

## FINDING F1 - the packet leaves most /Od-attested header-helper hand expansions in the TU (rule 6)
/Od helper census of the render_debug range (od_census_calls.txt, od_census_wide.txt) vs candidate c06:
- render_debug_vector2d (/Od 0x84a760, asserts 269-272): REAL CALL point_from_line2d(point, vector, size,
  &end_point) (0x444020); candidate hand-expands `end_point.x = point->x + size*vector->i; ...`.
- render_debug_camera (/Od 0x847660): arctangent(forward.j, forward.i) (0x47a380) * 57.29578; candidate writes
  `(real)atan2(...)` - the hand expansion the REAL_MATH_EXTERNAL_ARCTANGENT define fossil points at.
- render_debug_bsp (/Od 0x847210 region): plane3d_distance_to_point(plane, &render.camera.position [push
  0xecfe0c]) (0x56d580 = dot_product3d(point,&plane->n) - plane->d); candidate hand-expands the dot product
  through an invented `point` local absent from /Od - fossils REAL_MATH_EXTERNAL_{DOT_PRODUCT3D,
  PLANE3D_DISTANCE_TO_POINT}. So the "dead defines" are dead only because the calls were hand-expanded.
- set_real_point3d (0x4ac810) real calls: box_outline 8 (/Od 0x846de0), box 8 (0x846860), box2d_outline 4
  (0x846510), build_pill_points 6 (0x8452f0); candidate hand-expands all 26 as component stores.
- build_circle_points (/Od 0x8450c0): rotate_vector2d(&points[index], sine, cosine, &points[index+1]) (0x4c9a90);
  candidate hand-expands via set_real_point2d(... cos*x - sin*y, sin*x + cos*y) (needs point2d->vector2d view).
Labs (never land; all under this slug), each whole-object compiled with the shadow header:
- L2D (+pfl2d): 36/36, rows == c06; _point_from_line2d IDENTICAL to January action_vehicle; link PASS; 0 NODUP/7.
- L3 (+20 set_real_point3d in box fns): object byte-IDENTICAL to L2D.
- L4 (+arctangent, +plane3d_distance_to_point, `point` local removed): 36/36 rows == c06; new _arctangent (==
  actor_perception), _dot_product3d (== action_charge), _plane3d_distance_to_point (== decals) all IDENTICAL;
  provider link PASS both orders; NODUP 0; object_audit PASS.
- L5 (+6 set_real_point3d in build_pill_points): object byte-IDENTICAL to L4.
- L6 (+rotate_vector2d with 2 point2d->vector2d view casts): 36/36; _rotate_vector2d == path_obstacles, PASS;
  strip test (casts removed): byte-IDENTICAL, only C4133 x2 => casts byte-inert (ruling-3 class, needs disclosure).

## Amended packet (reviewer-built, verified)
- patches/05a_render_debug_genuine_helpers_and_vehicle_AMENDED.patch (sha256 794a3669fbbe78c9...): the whole of
  05 (same change lines) PLUS the /Od-attested helper calls of F1 without casts:
  point_from_line2d (vector2d), arctangent (camera), plane3d_distance_to_point (bsp; the invented `point` local
  goes, /Od pushes &render.camera.position directly), set_real_point3d x26 (box_outline 8, box 8, box2d_outline 4,
  build_pill_points 6). Argument order per /Od pushes (checked per call).
- patches/05b_OPTIONAL_render_debug_rotate_vector2d_view_casts.patch (sha256 7e56eaecc37fff54...): build_circle_points
  rotate_vector2d with two point2d->vector2d view casts (ruling-3 class: /Od passes &points[index] /
  &points[index+1] straight in; strip test byte-identical; identical {real,real} layout; strict exact). Needs
  ledger/commit disclosure; optional.
- Apply: `git apply --check` at HEAD 434f0151: 05a rc 0, 05b rc 0, 06 rc 0. Sequential 05a -> [05b] -> 06 into
  LF and CRLF copies: all rc 0, LF == CRLF modulo EOL; 05a+06 == lab L5, 05a+05b+06 == lab L6 (modulo EOL).
- a06 = 05a+06 compiled (shadow header; also shadow tree T with cwd = tree root: identical):
  36/36 EXACT, every row == production; object_audit PASS (63/0; surplus delta vs base: +_arctangent 16,
  +_dot_product3d 32, +_plane3d_distance_to_point 48, +_point_from_line2d 48, +_point_from_line3d 48);
  PDB 0 disagreements; surplus 16 COMDATs 0 not identical (arctangent==actor_perception, dot_product3d==
  action_charge, plane3d_distance_to_point==decals, point_from_line2d==action_vehicle, point_from_line3d==
  action_charge); provider_link full PASS (43 rows) and new-only PASS both orders; NODUP 0 for all five
  (14/73/20/7/26 SELECT_ANY definers); 06 link probes (linker_common, hs_globals_external, both orders) 0 dup,
  0 unresolved toggles; objdiff 3.3.1 mini == production (8235/9448, 34/36, data 30236/30236); /W3 == c06
  (only real_math.h(764) C4244 new vs prod); fake_match_scan 0 leads (prod, c06, a06, b06).
- Sweep: 05a changes only render_debug.c beyond 05's vehicle_datum.h line; vehicle_datum.h includers
  (actor_moving, physics, vehicles) IDENTICAL to build/base in shadow tree T.
- Advisory (not changed): /Od build_pill_points spells `top_circle_point->y + height_magnitude` (candidate:
  reversed operands; lab L7 shows byte-inert); render_debug_player's /Od text buffer RTC name is `string`
  (candidate `text`); vehicle_datum.h header comment stale; 5 C4013 implicit declarations remain.

## Precedent audit
- 05 / 05a: docs/campaign_house_rules.md:33-40 (3c74fa36) + handoff ruling #2 (lines 69-75) read in the original.
  Conditions: genuine shared-header __inline (real_math.h, untouched) YES; byte-identical to January selected copy
  YES (every new COMDAT); caller strictly exact YES (36/36, rows == prod); full ownership/build sweep zero
  regressions YES (only render_debug.obj differs; includers identical; worker 612-TU sweep agrees); selected-
  provider link YES both orders; no hand expansion YES for point_from_line3d (05) and for every /Od-attested
  helper (05a). Same class landed in canonical: hs_runtime 05255584, leaf_map/scale_vector3d lanes, path.c.
- 06: objects.c fb0d8145 (bare tentatives), units/structure_visibility (Matching), players 6e3e2d35 (COMMON
  tentatives, lighter evidence) read; docs/common_pool_ordering_20260922.md limit (no adjacency-only ownership;
  uninitialised form; class check) satisfied by per-symbol evidence (maps, /Od interleave, referencer census).

## VERDICT
APPROVE (zero credit). 05 and 06 as submitted are correct and meet their precedents; however 05 leaves the
TU's other /Od-attested header-helper hand expansions (F1), and it deletes the REAL_MATH_EXTERNAL defines as
"dead" although they are the fossils of two of those expansions. Recommended landing: 05a (in place of 05),
then 06; 05b optional with ruling-3 disclosure. Object stays NonMatching pending owner Q2d (+ patch 08).
- Pruned shadow tree T (reproduce: copy source/ + libs/ into scratch/w/review_r2w2_render_debug/T, overlay GA_LF files, python rv_tcompile.py T <units>).
