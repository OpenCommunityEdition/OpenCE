# REVIEW source/render/render_debug

approve: True

amended_patch: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_r2w2_render_debug\patches\05a_render_debug_genuine_helpers_and_vehicle_AMENDED.patch (then the unchanged C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\r2w2_render_debug\patches\06_render_debug_tentative_debug_toggles.patch; optional C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_r2w2_render_debug\patches\05b_OPTIONAL_render_debug_rotate_vector2d_view_casts.patch between them)

## per_object
[
 {
  "unit": "05_render_debug_genuine_helpers_and_vehicle.patch (as submitted)",
  "approve": true,
  "issues": "Correct, and every rule-6 exception condition holds, verified independently. It is incomplete: other /Od-attested header-helper hand expansions stay in the TU (F1), and the REAL_MATH_EXTERNAL defines it removes are left over from two of them. Land it only if 05a is declined."
 },
 {
  "unit": "05a_render_debug_genuine_helpers_and_vehicle_AMENDED.patch (reviewer)",
  "approve": true,
  "issues": "Recommended in place of 05. It is 05 plus /Od-attested point_from_line2d, arctangent and plane3d_distance_to_point calls, and 26 set_real_point3d calls. a06 is 36/36 with rows equal to production. object_audit passes. All 16 surplus COMDATs are IDENTICAL, provider_link passes in both orders, there are 0 NODUP, objdiff is identical and there are 0 fake-scan leads. The commit must list the new COMDATs: _point_from_line2d, _arctangent, _dot_product3d, _plane3d_distance_to_point and _point_from_line3d."
 },
 {
  "unit": "05b_OPTIONAL_render_debug_rotate_vector2d_view_casts.patch (reviewer)",
  "approve": true,
  "issues": "Optional; it goes after 05a. It adds rotate_vector2d with two view casts, under ruling 3: per-site /Od attestation, a byte-inert strip test, the same layout and a strict-exact result. It needs disclosure. _rotate_vector2d is identical to path_obstacles' copy and passes the link."
 },
 {
  "unit": "06_render_debug_tentative_debug_toggles.patch",
  "approve": true,
  "issues": "None. The per-symbol evidence (maps, /Od interleave, referencer census) meets the objects.c/players precedent and the 09-22 limit. The objects are byte-identical, the six become COMMON(1), and the link probes are clean in both orders."
 }
]

## checks
All checks were run independently at HEAD 434f0151. The work area is scratch/w/review_r2w2_render_debug/ and REVIEW.md there has the full log. git status is unchanged; only the pre-existing untracked research/fifty_objects_r2_20260924/ remains.

(1) Patch hashes match: 05 is 1303b91b and 06 is c8a386b4. The change lines are identical to round-1 05/06. git apply --check at HEAD gives rc 0 for 05 and 06, each alone. Applying 05 then 06 in sequence into LF and CRLF slug copies gives rc 0/0, and the result equals the worker's files/c06 (modulo EOL).

(2) Compile. rv_compile.py uses the build.ninja cflags and puts a leading /I shadow first; /showIncludes confirms the modified vehicle_datum.h is the one used. I also compiled in a full shadow tree with cwd at the tree root.
- My prod.obj equals build/base (104 sections, 0 differ).
- My c06.obj equals the worker's sweep/c06 object.
- prod vs c05: the only difference is one inserted .text 48 (_point_from_line3d).
- c05 vs c06: the only difference is six _debug_* symbols going from UNDEF to COMMON(1).

(3) gate.py row logic: c06 is 36/36 EXACT, and every size/reloc/sha row equals production.

(4) object_audit against build/split (symbols.json is unchanged, so no emulated split is needed): PASS. 63 January symbols, 0 differ, all data sections ok. The only surplus delta is +_point_from_line3d 48.

(5) PDB storage (candidate): 0 disagreements, and all six toggles are cachebeta PUBLICs.

(6) surplus_identity: 12 surplus COMDATs, 0 not identical. _point_from_line3d equals January's action_charge copy.

(7) provider_link: the full run passes on 39 rows. The --baseline run checks only _point_from_line3d against action_charge, and it passes in both orders. nodup_census: 26 definers, 0 NODUP.

(8) Link probes for 06:
- c06 with January linker_common and with base hs_globals_external, both orders: 0 LNK2005/LNK1169 and 0 unresolved toggles.
- Control (c05): 12 unresolved.

(9) Sweep: vehicle_datum.h is included only by actor_moving, physics and vehicles. All three compiled from a shadow tree are IDENTICAL to build/base.

(10) objdiff 3.3.1 mini (my own run): prod, c06 and the amended version are identical. Code 8235/9448 (34/36), data 30236/30236.

(11) /W3: the only new warning is C4244 at real_math.h(764). fake_match_scan: 0 leads.

(12) Precedents, read in the original:
- For 05: house_rules.md:33-40 (3c74fa36), handoff ruling #2 (lines 69-75), the hs_runtime landing 05255584, and the object_closure note on ruling 5.
- For 06: fb0d8145 (objects.c), players 6e3e2d35, and common_pool_ordering_20260922.md.

(13) 06 evidence, re-derived:
- The referencers of the six are exactly {hs_globals_external, render_debug}.
- Census: 74 January COMMON records have that {hs_globals_external, X} shape. Canonical defines 15 of them, all in X, none outside X.
- 2001 maps:
  - the 08-15 beta lists 5 toggles as `<common>`;
  - the 09-25 beta lists 6 `<common>` plus render_debug_structure_decals;
  - the 09-25 retail map has no render_debug.obj and no toggles, although it still has the hs_globals_external name literals and other `<common>` records.
- /Od: the toggles at 0xf0afb0-b7 are interleaved with render_debug's cache variables at b4/b8/bc/c0.

(14) /Od re-read with odbuild:
- 0x84a920, 0x84a510 and 0x848640 make real point_from_line3d calls with exactly the patch's arguments.
- The asserts sit at 388-390.
- In tick, the RTC shows only two 12-byte aggregates.
- In player, the vehicle comes from 0x7820d0 with mask 2, and "stuck" is a dword test at the end of the vehicle datum.

(15) New /Od helper census of the render_debug range. It found further real helper calls that the candidate still writes out by hand (finding F1 below), confirmed with labs L2D-L7.

(16) Amended 05a with 06 (a06):
- 36/36, and the rows equal production.
- Compiling from the shadow tree gives the same object.
- object_audit PASS. PDB 0.
- Surplus: 16 COMDATs, all IDENTICAL. The new ones are _arctangent (matches actor_perception), _dot_product3d (action_charge), _plane3d_distance_to_point (decals) and _point_from_line2d (action_vehicle).
- provider_link: full run PASS on 43 rows, and the new-only run passes both orders. 0 NODUP.
- Link probes for 06 pass. objdiff is identical. /W3 matches c06.
- git apply --check at HEAD: 05a, 05b and 06 all rc 0. Applying 05a -> [05b] -> 06 in sequence into LF and CRLF copies gives rc 0 and matches the lab sources.
- 05b: the strip test (casts removed) is byte-identical.

Claim: taken as review_r2w2_render_debug_rv1 and released.

## issues
APPROVE, zero credit. Both 05 and 06, as submitted, are correct and meet their precedents.

For 05, every condition of the rule-6 exception in house_rules:33-40 and ruling #2 holds, and I verified each one independently:
- it uses the genuine real_math.h __inline;
- the COMDAT is byte-identical to January's action_charge copy;
- the callers are strictly exact;
- only render_debug.obj changes, and the three vehicle_datum.h includers are identical;
- the link passes in both orders.

The vehicle view and typed accessor are sound: the /Od build uses mask 2, and `stuck` corresponds to the stuck_mass_point_flags offset.

For 06, the evidence is per symbol, not adjacency:
- the referencers are exactly hs_globals_external and render_debug;
- the 2001 betas list the toggles as `<common>`, and the retail map drops them exactly when render_debug.obj is absent;
- the /Od build interleaves them with render_debug's own variables;
- 15 of 15 canonical definitions in the same {hs_globals_external, X} shape are in X, with no counterexample;
- it has the same form as objects.c and players (6e3e2d35).

FINDING F1 (rule 6; the packet is incomplete, not wrong). The /Od helper census shows that c06 still writes out by hand calls that /Od makes for real:
- point_from_line2d in render_debug_vector2d (/Od 0x84a760);
- arctangent in render_debug_camera (0x847660), written as `(real)atan2(...)`;
- plane3d_distance_to_point(plane, &render.camera.position) in render_debug_bsp. It is written out through an invented `point` local that /Od does not have.
- set_real_point3d at 26 sites: box_outline 8, box 8, box2d_outline 4, build_pill_points 6.
- rotate_vector2d in build_circle_points (0x8450c0).

The MANIFEST calls the three REAL_MATH_EXTERNAL defines "dead" and removes them. In fact they are left over from the arctangent and plane3d_distance_to_point/dot_product3d hand expansions. The round-1 review did no /Od helper census for render_debug, and treated the same class as a blocker in model_animations and leaf_map.

FIX (verified): scratch/w/review_r2w2_render_debug/patches/05a_render_debug_genuine_helpers_and_vehicle_AMENDED.patch (sha256 794a3669...).
- It contains all of 05, plus every no-cast /Od call above, with argument order checked against the /Od pushes.
- a06 (05a + 06) is 36/36 with rows equal to production, and object_audit passes.
- New COMDATs, each byte-identical to January's selected copy and link-clean in both orders, with 0 NODUP: _point_from_line2d, _arctangent, _dot_product3d, _plane3d_distance_to_point (plus _point_from_line3d from 05).
- Recommended landing: 05a instead of 05, then 06, applied one at a time from the repo root.

OPTIONAL: patches/05b_OPTIONAL_render_debug_rotate_vector2d_view_casts.patch (sha256 7e56eaec...) goes after 05a.
- It adds two point2d->vector2d view casts, which fall under ruling 3: /Od passes &points[index] and &points[index+1] straight in, the strip test is byte-identical, the layouts are identical and the result is 36/36. _rotate_vector2d is identical to path_obstacles' copy and passes the link.
- It needs a commit/ledger disclosure.

Commit disclosure for 05a should list every newly emitted COMDAT.

Advisory (not changed):
- /Od spells `top_circle_point->y + height_magnitude`; this is byte-inert (lab L7).
- The /Od RTC name of render_debug_player's text buffer is `string`.
- vehicle_datum.h's header comment is stale.
- 5 C4013 remain.
- The object stays NonMatching until owner Q2d (D0/D1) and patch 08.
