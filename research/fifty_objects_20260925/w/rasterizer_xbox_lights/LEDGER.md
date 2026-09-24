# rasterizer_xbox_lights - wave-2 worker ledger (fifty-objects lane, base 931ed8dc)

## Baseline (931ed8dc)
- gate --all (production): 13 exact / 1 residual / 0 unwritten; residual `_rasterizer_sun_glow_draw` 2352 [sha] (parked, unclassified).
- object_audit (build/base): only DIFF is `_rasterizer_sun_glow_draw`; 33 January symbols, 0 differ; surplus COMDATs
  _magnitude3d/_magnitude_squared3d/_normalize3d/_scale_vector3d/_square_root all IDENTICAL (surplus_identity) and
  provider_link PASS.

## Prior negatives read (do not repeat)
- opus_reconciliation_20260903 E01 (donor); opus5_150k_w2 (scoping solves 0x98 frame: cand_scoped.c; 20+ single-expression
  dot spellings inert k,j,i; loops); opus5_150k_w3 (V1/W1 exact via 3-statement accumulation + pointer; T1/T2/N3/N4/L1/L2/Q1/Q2
  dot_product3d regimes not exact).
- laws_w3 (opus5 worktree scratch/w3/laws_w3.md): OWNER/VERIFIER REJECTED V1/W1 under R14 (_vector_from_points3d COMDAT
  pending ruling), R15 (hand-expanded dot accumulation AND the occlusion block's hand copy of point_from_line3d),
  R16 (steering pointer alias W1). Fix named by R15: "keep ordinary helper calls (dot_product3d; point_from_line3d under a
  ruling) and still reach exact, or keep the park".
- House rules now carry the narrow Rule-6 exception: a header-inline COMDAT (incl. _point_from_line3d) may be emitted if
  byte-identical to January's selected copy + caller strictly exact + zero regressions + provider link passes.

## /Od readout (halo_cache_symbols.exe fn 0x7edb90, DX9 port "rasterizer_dx9_lights.c"; od_sun_glow.txt)
- RTC: direction(12) position(12) center(12) axes(8) eye_to_corona_vector(12) vsh_constants__screenproj(80, two scopes) bounds(16).
- Statement order: assert; vector_from_points3d(&window.camera_position, &parameters->position, &eye_to_corona_vector)
  [REAL CALL 0x42e3a0, return discarded]; normalize3d(&eye_to_corona_vector) [call, fstp st0];
  brightness = dot_product3d(&window.camera_forward, &eye_to_corona_vector) [REAL CALL 0x42dd80, ARG ORDER forward first];
  brightness = PIN((brightness - cosine(K)) / (1 - cosine(K)), 0, 1) [cosine = real call 0x455220, same local ebp-0x6c];
  viewport w/h shorts; vsh constants; direction = uncompress_int32_to_real_vector3d(...) (struct return);
  point_from_line3d(&parameters->position, &direction, parameters->definition->occlusion_radius, &position) [REAL CALL 0x42e0d0];
  rasterizer_project_billboard(&position, definition->occlusion_radius, &center, &axes);
  center.x = floor(center.x + 0.5f); center.y = floor(center.y + 0.5f) [stored back into center];
  bounds.x0 = center.x - 32; bounds.y0 = center.y - 32; bounds.x1 = center.x + 32; bounds.y1 = center.y + 32.
- dot_product3d /Od body = single expression a->i*b->i + a->j*b->j + a->k*b->k (same as real_math.h).

## Probes
### g1 (genuine /Od shape) - hypothesis: /Od attests real helper calls vector_from_points3d/normalize3d/dot_product3d(&fwd,&eye)/cosine
and point_from_line3d(&position,&direction,radius,&occlusion_point) with block-scoped eye_to_corona_vector and by-value
direction; projected_center snapped in place (/Od center.x = floor(center.x+0.5)).
- result: 2352 [sha], 672 vs 674 insns, real_regions 8 REAL (was 47 for production body).
- occlusion block (uncompress by value + genuine point_from_line3d) now BYTE-EXACT incl. radius CSE and fstp st(0).
- cos placement after the sum EXACT.
- remaining: (a) dot terms ours i(global-first),k,j vs January i,j,k all local-first; (b) screen_bounds block 0x26d-0x2c7:
  January `fild x0; fst [ebp+8]; fadd bounds.x0` (cast loaded first, kept) vs ours `fild; fstp; fld bounds.x0; fadd [ebp+8]`,
  plus one movsx/mov scheduling swap at 0x26d.
### g2 (diagnostic only; NOT landable) - V1 brightness block (R15/R16 forms) + g1 genuine point_from_line3d + by-value uncompress
- result: STRICT EXACT 14/14. Proves the occlusion block's genuine point_from_line3d call (R15 second half) is byte-exact and the
  screen_bounds residual in g1 is caused ONLY by the brightness block (IL ordinal effect, M8-class), not by the occlusion block.
### g3 (diagnostic) - g1 brightness + hand-expanded occlusion (by-value uncompress): same 8 REAL regions as g1 -> occlusion form inert.
### g4 - g1 with `(real)cos(_pi / 4.0f)` instead of `cosine(_pi / 4.0f)` (hypothesis: cosine() inline adds 2 IL locals per call AFTER the dot; M8 count key shifts the later screen_bounds fadd leaves)
- result: 674/674 insns, 2 REAL regions: screen_bounds block now EXACT. Only the dot remains: ours i(global-first),k,j; Jan i,j,k local-first.
### g5/g6 - dot_product3d argument order swapped (&eye,&fwd) on g1/g4: identical code to g1/g4 (argument order INERT here).
### ORACLE i1..i8 (diagnostic only, never landable): K initialized-unused `long oracle_k = k;` at the head of the brightness block on g4
- K=1..3: identical to g4. K>=4 (4..8 all same): i-term flips to LOCAL-FIRST (January operand order); all 3 products now
  local-first; remaining: term order ours i,k,j vs January i,j,k. => the i-term operand order is an IL-count key (M8 class):
  January has >=4 more IL locals before/at the dot than g4, OR a different construct. Unused declarations (o1..o7) are inert.
### ORACLE c4..c12 (diagnostic) - same oracle on g1 (cosine()): screen_bounds stays broken for K=4..12; K=4..8 i-term local-first, K=12 flips back.
  => screen_bounds flip is caused by cosine()'s inline locals themselves, not a plain count shift; (real)cos is the working spelling.
### g7 - g4 with eye_to_corona_vector at FUNCTION scope: identical result to g4 (frame still 0x98; scope inert). Keep block form (either is fine).
### ORACLE sweep (sweep.py; diagnostic only) - dead `long oracle_k = k;` block at 4 positions (function top / block top / after
vector_from_points3d / after normalize3d) x K. Position INERT; i-term local-first exactly for K in [4,9], global-first for K=0..3 and
10..24; K>=28 breaks other regions. TERM ORDER stays i,k,j for every K and position => term order is structural, not a count key.
### n1 (normalize3d(vector_from_points3d(...)) nested) == g4 (inert). n4/n5 (dot inside PIN, no separate statement): cos hoisted first (A35(3) confirmed), worse.
### g4 census: surplus COMDATs _dot_product3d/_point_from_line3d/_vector_from_points3d (+existing 5) ALL IDENTICAL to January's
selected copies (surplus_cand.py) and provider_link PASS. Rule-6 exception needs the CALLER strictly exact -> g4 is not landable fuzzy.
### Corpus scan (scan_ijk.py over build/split): 3-term x87 dot sequences: kji 256, jki 72, ijk 46, ikj 30, kij 26, jik 20.
- EXACT in-tree inline-dot_product3d examples with January's LOCAL-FIRST i,j,k: _leaf_map_leaf_spans_polygon (plane3d_distance_to_point
  -> dot_product3d((real_vector3d *)point, &plane->n) - plane->d) and _item_align_to_normal_and_point (dot_product3d(&marker.matrix.up,
  normal) + 1.f). So VC7 CAN emit i,j,k local-first from the genuine inline helper; the ordering is context-driven, not proof of a hand accumulation.
### ORACLE tusweep (diagnostic): 1..40 dummy file-scope `extern long` declarations before the function: dot INERT for all K.
  => TU name-count (H3) is not the key; switching global_window_parameters to the genuine rasterizer.h struct would not move the dot.
### micro lab (micro/m1.c, diagnostic): same brightness block alone in a tiny TU -> canonical k,j,i GLOBAL-first; items-like
dot(&eye, normal)+1 -> k,j,i local-first. Context (rest of the function) drives the order; oracle position-invariance
(top vs between vector_from_points3d and dot) shows the leaves are created lazily at/after the dot, not at the stores.
### LAB bisection (bisect.py; truncating the body after line L, brightness sunk to an extern; diagnostic only)
- L=34: k,j,i mixed; L=59: k,j,i; L=75: k,j,i ALL LOCAL-FIRST; L=154: k,j,i; L=258: i(L),k(G),j(G); L=327: i(G),k(L),j(L) (= g4).
  => LATER code (named locals / IL after the dot) changes the dot's leaf order: the key is a function-wide IL ordinal set,
  so an IL-invisible difference anywhere in the body (named locals, temps) can be January's missing source fact.
- finer bisect L=156..257: term order varies (k,j,i / i,k,j / j,k,i ...) and at L=241 the dot is emitted i,j,k (j global-first).
  => January's i,j,k TERM ORDER IS REACHABLE from the genuine inline dot_product3d in this function; it depends on the IL of
  LATER code (hash/ordinal effect), so the R15 hand accumulation is not the only i,j,k form. Dead-long oracles never move term
  order (only the i-term operand), so the key is IL expression/node content, not the local count.
### r1 - /Od-attested `r = ((real)pass / 16.0f) * 80.0f - 4.0f` (0x940194=16.0): byte-identical to g4 (folds to *0.0625), dot unchanged (inert).
### r2 - `/ SUN_GLOW_RAY_COUNT` int divisor: worse (code changes); rejected.
### LAB expression-oracle (esearch.py; dead `{ real oracle = EXPR; }` blocks; diagnostic only, never landable)
- A dead read of `global_window_parameters.camera_forward.k` in the ray loop or at the function END reorders the dot to
  i,j,k (i global-first, j/k local-first). Dead reads of camera_forward.i / parameters->position.x / a product elsewhere give
  k,j,i or j,i,k variants. => the dot's leaf/term order is keyed by the function-wide set of IL memory references to the
  global_window_parameters fields (hash-order class), not by statement position.
### cb0..cb10 (diagnostic): count oracle + end dead-read oracle interact (cb0-2: i,j,k i-global-first; cb3-9: i-local-first but
i,k,j). Two oracle keys are coupled; the landscape is hash-order-like. No evidence source exists for IL-invisible content.
### sc - SUCCEEDED(...) spelling for the 11 loop checks: inert. q1/q2 - genuine helper-return pointer + dot_product3d: == g4 (inert).
### va - named center_x/center_y (production style): 46 REAL (frame/slots) -> in-place snapping (/Od-attested) is right.

## STOP RULE FIRED (>= 10 evidence-based shapes; residual is a dependency-free x87 order in one commutative dot)

## Final state
- Production (real file) UNCHANGED (git diff --stat empty). Real-file gate 13/1/0; data 1616/1616 (report 100%).
- Best credible genuine candidate: cand_g4.c (sha256 b85e7c0b...; fn normalized sha f5f9e2f4a22bdb34...), 2352/174, 674/674 insns,
  frame 0x98, reloc multiset identical, 2 REAL regions / 3 reloc rows, all inside the brightness dot (+0x69..+0x88):
    January: fld eye.i; fmul fwd.i; fld eye.j; fmul fwd.j; faddp; fld eye.k; fmul fwd.k; faddp   (i,j,k; all local-first)
    g4     : fld fwd.i; fmul eye.i; fld eye.k; fmul fwd.k; faddp; fld eye.j; fmul fwd.j; faddp   (i,k,j; i global-first)
  Everything else (occlusion via genuine point_from_line3d, fcos after the sum, screen_bounds, loop) is byte-exact.
  Source = /Od 0x7edb90 statement shape: vector_from_points3d / normalize3d / brightness = dot_product3d(&fwd,&eye) /
  brightness = PIN(...) / direction = uncompress(...) by value / point_from_line3d(...) / projected_center snapped in place.
  Deviation from /Od: (real)cos instead of cosine() (g1 with cosine(): +6 REAL regions in screen_bounds; disclosed).
  Census: +_dot_product3d/_point_from_line3d/_vector_from_points3d COMDATs, all IDENTICAL to January's selected copies,
  provider_link PASS, fake scan 0 leads. NOT LANDABLE: Rule-6 exception requires a strictly exact caller.
- Diagnostic exact: cand_g2_diagnostic_R15.c = g4 but with the w3 V1 brightness block (vector_from_points3d return pointer +
  3-statement `cosine +=` accumulation). STRICT EXACT 14/14, object_audit PASS, surplus 7 COMDATs IDENTICAL, provider_link PASS,
  pdb_storage 0 disagreements, data 100%. Blocked ONLY by R15 (hand accumulation of dot_product3d); R14 is now covered by the
  Rule-6 exception (identical copies + exact caller + provider link), R16 fix text allows the helper-return pointer, and the
  occlusion half of R15 is resolved (genuine point_from_line3d).

## Reopen criteria
1. Owner ruling admitting the g2 brightness block (accumulation) -> object complete immediately (needs full ninja sweep).
2. Owner ruling allowing identical header-inline COMDATs with a non-exact caller -> land cand_g4.c as a park re-baseline
   (dd0f3291... -> f5f9e2f4..., zero credit) replacing the hand-expanded point_from_line3d / named-field dot.
3. A C2 decode of the x87 commutative leaf/term ordering (function-wide IL memory-reference hash) that names which
   IL-invisible January construct (a later reference to global_window_parameters fields or extra IL expressions) yields
   i,j,k local-first; the lab shows the order is reachable (bisect L=241; end dead-read of camera_forward.k gives i,j,k).
