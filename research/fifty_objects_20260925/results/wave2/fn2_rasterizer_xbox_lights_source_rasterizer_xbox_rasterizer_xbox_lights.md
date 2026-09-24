# source/rasterizer/xbox/rasterizer_xbox_lights

## verdict
FUZZY_IMPROVED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_lights\\cand_g4.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_lights\\g4_owner_gated.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_lights\\cand_g2_diagnostic_R15.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_lights\\LEDGER.md",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_lights\\od_sun_glow.txt"
]

## production_changes
NONE LANDABLE. The real file is unchanged (git diff --stat is empty). Two owner-gated artifacts are provided.
(1) scratch/w/rasterizer_xbox_lights/g4_owner_gated.patch, which passes git apply --check. The full file is scratch/w/rasterizer_xbox_lights/cand_g4.c (sha256 b85e7c0b40d1e82b...). It replaces only the rasterizer_sun_glow_draw body with the /Od-attested genuine-helper shape:
  - block-scoped `real_vector3d eye_to_corona_vector; vector_from_points3d(&global_window_parameters.camera_position, &parameters->position, &eye_to_corona_vector); normalize3d(&eye_to_corona_vector); brightness = dot_product3d(&global_window_parameters.camera_forward, &eye_to_corona_vector); brightness = PIN((brightness - (real)cos(_pi / 4.0f)) / (1.0f - (real)cos(_pi / 4.0f)), 0.0f, 1.0f);`
  - block-scoped `real_vector3d direction = uncompress_int32_to_real_vector3d(parameters->compressed_direction); point_from_line3d(&parameters->position, &direction, parameters->definition->occlusion_radius, &occlusion_point);`
  - projected_center.x/.y snapped in place, which removes the center_x/center_y, intensity and function-scope direction locals.
  It would be a zero-credit park re-baseline (base sha dd0f3291... -> f5f9e2f4a22bdb34...). It is NOT landable under Rule 6: the three new header-inline COMDATs are byte-identical to January's selected copies, but the rule needs a strictly exact caller.
(2) scratch/w/rasterizer_xbox_lights/cand_g2_diagnostic_R15.c is STRICT EXACT 14/14 and the whole-object audit passes. It is held only by R15 (details in evidence).
No symbols.json, config or header changes are needed. Optional integrator text refresh for the parked.json evidence: the recorded '0x98 versus 0xA4 frame' is stale. Production is already 0x98. The residual is only the brightness dot term/leaf order.

## evidence
BASELINE (931ed8dc): gate 13 exact / 1 residual / 0 unwritten. .rdata is 1616/1616 (report 100%). object_audit on build/base: only the function differs; 33 January symbols with 0 differences; the 5 surplus COMDATs are IDENTICAL; provider_link PASS.

/Od readout (halo_cache_symbols.exe fn 0x7edb90, a DX9 port; the full dump is in od_sun_glow.txt):
- RTC descriptors name direction, position, center, axes, eye_to_corona_vector, vsh_constants__screenproj and bounds.
- Statement order: vector_from_points3d(&camera_position, &parameters->position, &eye_to_corona_vector) as a real call with its return discarded; normalize3d(&eye); brightness = dot_product3d(&camera_forward, &eye) as a real call with forward first; brightness = PIN((brightness - cosine(K)) / (1 - cosine(K)), 0, 1) on the same local ebp-0x6c; direction = uncompress by value; point_from_line3d(&parameters->position, &direction, occlusion_radius, &position) as a real call; center.x = floor(center.x + 0.5f) in place.

PROBES (full details in LEDGER.md):
- g1 (literal /Od shape with cosine()): 8 REAL regions, down from 47 for the production body. The occlusion block (genuine point_from_line3d + by-value uncompress) is byte-exact, including the radius CSE and fstp st(0). fcos placement after the sum is exact.
- g4 (the same with (real)cos): 674/674 instructions, frame 0x98, relocation multiset identical. Only 2 REAL regions remain, both in the dot at +0x69..0x88.
  - January: fld eye.i; fmul fwd.i; fld eye.j; fmul fwd.j; faddp; fld eye.k; fmul fwd.k; faddp (order i,j,k, every product loads the local first).
  - g4: fld fwd.i; fmul eye.i; fld eye.k; fmul fwd.k; faddp; fld eye.j; fmul fwd.j; faddp (order i,k,j, the i product loads the global first).
  - surplus_cand.py: _dot_product3d, _point_from_line3d and _vector_from_points3d are all IDENTICAL to the action_charge/action_alert copies. provider_link PASS. fake scan 0 leads.
- Inert spellings:
  - g5/g6: dot argument order.
  - g7: eye vector scope.
  - n1: nested normalize3d(vector_from_points3d()).
  - r1: /Od-attested `/16.0f`.
  - sc: SUCCEEDED().
  - q1/q2: helper-return pointer + dot_product3d.
  - tusweep: 1..40 file-scope declarations.
- Worse spellings: n4/n5 (dot inside PIN hoists fcos) and va (named center_x/center_y, 46 REAL).

DIAGNOSTIC g2: V1's brightness block (helper-return pointer + 3-statement accumulation) + genuine point_from_line3d + by-value uncompress gives STRICT EXACT 14/14. On the g2 obj:
- object_audit PASS;
- 7 surplus COMDATs, all IDENTICAL;
- provider_link PASS;
- pdb_storage 0 disagreements.
This proves the only unresolved January fact is the brightness dot.

NEW MECHANISM EVIDENCE (lab only):
- Corpus scan (scan_ijk.py) finds exact in-tree genuine inline dot_product3d sites that January emits i,j,k with the local loaded first: _leaf_map_leaf_spans_polygon and _item_align_to_normal_and_point.
- Truncation bisect: the order of this dot changes with LATER code, and at L=241 it becomes i,j,k.
- Dead-read oracle: `real oracle = global_window_parameters.camera_forward.k;` in the loop or at the function end turns the dot into i,j,k (only the i operand stays wrong).
- Dead-local oracle: 4..9 initialized dead locals flip the i operand to local-first.
- The two oracle keys interact.
=> January's order is reachable from the genuine helper. It is keyed by function-wide IL content that is invisible in bytes (a hash/ordinal effect), so the hand accumulation is not proven to be January's source.

## blockers
The strict residual is a dependency-free x87 order in the one commutative brightness dot (term order and i-product operand order). It is driven by function-wide IL content that does not show in January's bytes, and no first-party evidence names that content. The stop rule fired after more than 10 evidence-based shapes.
Landing is owner-gated:
(a) cand_g4 emits identical header-inline COMDATs, but its caller is not strictly exact (Rule 6).
(b) cand_g2 is exact but uses the R15-rejected 3-statement dot accumulation. R14 is now covered by the Rule-6 exception (identical copies, provider link PASS). R16's fix text allows a helper-return pointer. The occlusion half of R15 is resolved by the genuine point_from_line3d call.

## reopen_criteria
(1) The owner admits the g2 brightness block (accumulation through the vector_from_points3d return pointer). The object then becomes complete immediately: land cand_g2_diagnostic_R15.c, retire the park, and run a full ninja plus the stable sweep for zero regressions.
(2) The owner allows identical header-inline COMDATs with a non-exact caller. Then land cand_g4.c as a zero-credit park re-baseline to f5f9e2f4a22bdb34.... This removes a hand-expanded point_from_line3d, which production violates today, and a named-field dot.
(3) A C2 decode of x87 commutative leaf/term ranking (the function-wide IL memory-reference hash) identifies which construct gives i,j,k with every product loading the local first on top of cand_g4, AND that construct is first-party attested. Lab leads: a later reference to global_window_parameters.camera_forward fields, and +4..9 IL locals before the dot.
Do not repeat: g1/g4/g5/g6/g7/n1/n4/n5/va/r1/r2/sc/q1/q2 or the oracle sweeps.

## task notes
- The tree was not modified.
- The worker directory scratch/w/rasterizer_xbox_lights/ holds:
  - mk.py: splice, gate and alndiff harness;
  - sweep.py, osearch.py, esearch.py and tusweep.py: oracle sweeps, lab only;
  - bisect.py: truncation bisect;
  - micro/: a micro-TU lab;
  - scan_ijk.py: January corpus scan of x87 dot term orders;
  - surplus_cand.py: candidate-object COMDAT identity check;
  - ref/: V1.c and W1.c copied from the opus5-150k worktree.
- Owner question to put forward. With the genuine /Od-attested helpers, the function is exact except for the order of one commutative dot. The only exact source form found is the w3 accumulation (g2). Lab evidence suggests January used dot_product3d, with its order set by IL content that does not show in the bytes. The owner should choose between admitting g2 (object complete) and allowing g4 as a genuine fuzzy re-baseline (needs a Rule-6 caller-exact waiver), or keeping the current park.
- The parked.json evidence text is stale: it still says the frame is 0xA4 against January's 0x98, but production is already 0x98.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\rasterizer_xbox_lights\LEDGER.md
