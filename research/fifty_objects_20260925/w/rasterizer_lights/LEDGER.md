# rasterizer_lights worker ledger (wave 4, base 16542e46)

## Baseline
- gate --all on unmodified source (base.c): 10 exact / 3 residual / 0 unwritten.
  residual: _rasterizer_lens_flare_submit_for_cluster [sha] 400, _rasterizer_lens_flares_draw [sha] 2240,
  _rasterizer_lights_reset_for_new_map [sha] 48.
- Prior ledgers read: rasterizer_lights_obj_{frame_start,corona_rotation,opus5_150k_w2,opus5_250k_w3c};
  lane C handoff 5.4 (overrun proof, owner-admitted on proof) and lane C canonical reconciliation
  (overrun EXCLUDED: "COFF places a different array immediately after the overwritten one").
- Recorded negatives NOT to repeat:
  * submit_for_cluster c1..c5 (direct per-component stmts, (real) casts, set_real_vector3d w/ casts,
    char direction[3], direction/up at outer block scope) and p1 inline-visible point_from_line3d.
  * draw: swapping real decls inert; separate distance_fade local -> frame 0xb8; single
    3-factor light_brightness expr -> x87 phi lost; inlined point_from_line3d for reflection point
    regresses; A35 leaf-age lever predicts ours.
  * reset: every exact spelling (0x7722) is a 34-byte overrun (owner-gated, authentic bug class).

## Evidence gathered (before probes)
- .bss layout: January results2@0, results@262176, params@292640, count@333600, error@333604.
  Ours (base): results@0, results2@30464 (rest equal). objdiff .bss 100% (size-inferred), but
  object_audit fails on the two offsets + 6 storage rows (symbols.json lacks "static":true for
  the 5 .bss statics and _lens_flare_evaluate_corona_rotation_function; none are cachebeta publics).
- HCEX compiland (hcex_rl_compiland.txt): file statics local_lens_flare_parameters[0x400],
  local_lens_flare_occlusion_test_results[0x380], local_lens_flare_occlusion_test_results2[2][0x10008],
  local_lens_flare_count; the error latch is a STATIC LOCAL `unsigned char warned` of
  rasterizer_lens_flare_submit. HCEX draw locals: direction, eye_to_corona_vector, corona_axis,
  scale_functions[4], corona_position, scale (real_vector2d), point, tint_color, animation_color.
  submit_for_cluster locals: parameters, up, direction. occlusion_tests: direction, point.
- /Od (0x82f710) submit_for_cluster: statement order == ours; set_real_vector3d is a real call
  with i,j,k products; an otherwise-unused 4-byte local at [ebp-0x58] is initialised with 1/127
  (0x3c010204) between `parameters` and `direction` in frame order; multiplications use the
  literal directly (C++ const-folding) => later source has a const real local for 1/127.
- s1 (real const direction_scale local between parameters and direction, /Od-attested const 1/127 local): byte-identical to baseline for submit_for_cluster (INERT).

## draw probes (oracle + genuine)
- d1 (house-rule fix: genuine /Od-attested `point_from_line3d(&position, &mirror, reflection->offset, &point)`
  replacing the hand expansion; rule 6 / wave-3 law (g) require it): 20 REAL regions (worse; +0x202 fixed but
  many dot-product term orders flip). Oracle: d1 + >=4 dead locals == base bytes exactly (25834daf).
- base + D dead `long` locals at fn top, D=0..15: byte-identical to base (insensitive); D=16..48 other states;
  January never reached. d1 + D: D=0 455b, 1..3 7da9, 4..9 == base.
- combo k1 on d1 (/Od-attested inventory: B brightness_upper local, R radius_upper local, S separate
  second-loop counter, C cosine delta local): pure COUNT behaviour (any 1..3 -> 7da9, all 4 -> base 2583).
  None reaches January. => the 3 draw sites are NOT a function-top local-count residual.
- L1 (lab) `light_brightness = occlusion_fraction*light_brightness`: byte-identical to base (operand order INERT).
- Oracle: dead locals inserted after `position` decl (D=0..9): byte-identical (insensitive).
- Oracle: TU names before draw (extern long, K=0..40) and prototypes at file top (K=1..33): draw and
  submit_for_cluster byte-identical in every K (H3/TU name-count keys INERT for this TU's residuals).
- k2 combos on d1 with T (/Od 0x8308e9: set_texture result stored in a named 1-byte local [ebp-0x105]):
  d1+T+B+R+S == 25834daf (the CURRENT PARK HASH exactly) with every added local /Od-attested and the
  genuine point_from_line3d call => a house-rule-compliant respelling of the park body at unchanged hash
  (zero credit; removes the rule-6 hand expansion). Candidate e1.c. Any 1-3 of them -> 7da9 (19 REAL).

## submit_for_cluster probes
- Oracle: D=0..17 dead locals at fn top: byte-identical (insensitive). TU names (see above): insensitive.
- x3 (lab) C4204 aggregate initializer `real_vector3d direction= {i*K, j*K, k*K}`: == c1 (loads not hoisted).
- x4 (lab) set_real_vector3d(&direction, i, j, k) + scale_vector3d(&direction, 1/127): loads hoisted but
  filds right-to-left (k, j, i) - not January.
- Characterisation (new): January = per-component fild/fmul/fstp in i,j,k order WITH all three char loads
  hoisted before the first store (arg-temp evaluation + per-component sink). Ours (set_real_vector3d) =
  right-to-left evaluation (k,j,i) kept on the x87 stack; January's own hs_runtime
  render_debug_trigger_volumes set_real_vector3d(&extents, x1-x0, ...) is right-to-left too, so the
  January submit_for_cluster source is NOT the plain set_real_vector3d-of-products form, yet the /Od
  (later) build calls set_real_vector3d with the products. No attested form reproduces it.
- .bss layout lab: bss1 (results '= {0}', results2 uninit) and bss2 (both '= {0}', results2 declared first) both give January's layout (results2@0, results@262176); bss3 (both init, results first) does not. Code rows unchanged (10 exact). bss1 preferred: keeps HCEX declaration order (results, results2).
- bss4 (lab): HCEX static local 'static boolean warned = FALSE;' in rasterizer_lens_flare_submit replaces local_lens_flare_error_printed: same .bss offset 333604, VC7 name ?warned@?1??rasterizer_lens_flare_submit@@9@9; submit row becomes [reloc-identity] until symbols.json renames _local_lens_flare_error_printed -> that name (+static).
- L2/L3 (/Od 0x82fb92 attests the light variable is initialised to 1.0f at block entry): 'real light_brightness = 1.0f;' FIXES site +0x202 (5 REAL left: +0x31c and the +0x738 interleave). Declaration position (L2 in place, L3 moved after position) same bytes 39103012.
- S1 (lab) camera_offset declared in loop body before direction: REVERTS the L2 site-1 fix (== base 25834daf). S2 (direction declared then assigned by statement): == L2. Dead-local oracle on L2 at fn top / loop body / window block (D=0..9): all inert.
- O1/O2 (lab): /Od declaration order for the visible block (scale_functions, position, light=1.0, mirror, camera_offset, depth, occlusion, corona_rot, screen_rot, cos_scale, cos_offset, reflection_index, light_scale), with/without position init: == L2 bytes (declaration order inert).
- CORRECTION: S1 accidentally edited rasterizer_lens_flare_submit (first `camera_offset` decl in file); the
  properly scoped S1b/S3/S4/S5 (camera_offset in loop body before/after direction, in window block;
  direction at function scope) are all == L2 bytes. Scope/declaration placement INERT for site 2.
  Side finding: S1's edit of a DIFFERENT function reverted draw's site 1 -> cross-function coupling.
- Oracle: unused `real_vector3d lab_agg_K;` (K=1..9) in the visible block: inert.
- A1 (lab) `-dot_product3d(&camera_offset, &direction)` at site 2: byte-identical (A44 canonicalisation; arg order INERT).
- draw STOP for site 2 (+0x31c direction.i vs camera_offset.i) and site 3 (+0x738 set_texture arg interleave):
  inert under dead locals (3 positions), unused aggregates, TU names, declaration order, scope moves,
  arg order. Laws A35/M8 do not yield an attested lever.
- Candidate e2 = d1 (genuine point_from_line3d) + light_brightness = 1.0f (/Od 0x82fb92) + /Od-attested
  locals B (brightness_upper_bound), R (radius_upper_bound), S (separate second-loop counter),
  T (set_texture result byte local /Od 0x8308e9) == 39103012 (5 REAL; +0x202 closed). Without the four
  locals the genuine-call form is 19 REAL (80ff/b3fe). k3 table: any 4 of {T,B,R,S,C} -> 39103012.
- csplit regen (build/tools/csplit.exe -i cachebeta.exe -p slug/config -o slug/split) with slug symbols.json (6 lines: static on evaluate_corona + 5 .bss rows; _local_lens_flare_error_printed -> ?warned@?1??rasterizer_lens_flare_submit@@9@9): rc 0, 833 objs, ONLY rasterizer_lights.obj differs from build/split.

## Candidates assembled (all compiled with gate.py; regen split = slug csplit with slug symbols.json)
- stg.c = base + storage/layout packet (results '= {0}', HCEX static local `warned`): vs regen split 10/3 (no loss).
- fin.c = stg + draw e2 edits: vs regen split 10 exact / 3 residual; draw 39103012d0bd42ef (5 REAL, +0x202 closed).
- fin_owner.c = fin + owner-gated reset overrun (BUG comment): vs regen split 11 exact / 2 residual;
  audit_r (AUDIT_SPLIT_ROOT=slug/split): 41 January symbols, 0 differ; only draw + submit_for_cluster DIFF.
- Surplus sections of fin_owner.obj vs build/base production obj: identical set and hashes except the two
  intended functions (no new COMDAT, provider-link status unchanged).
- objdiff 3.3.1 one-unit reports (slug od_proj/*; prod reproduces build/report.json exactly):
  prod unit 99.06512 / draw 99.38321; fin unit 99.06641 / draw 99.38626 (fuzzy improvement), data 335089/335089
  vs regen split; fin_owner unit 99.067245, reset 100, matched code 1996 -> 2044, 10 matched fns.
- NOTE: Windows case-insensitive FS: draw labs S1..S5 overwrote files s1..s3 (submit_for_cluster const-local lab); s1's recorded result was measured before the overwrite. Const-local labs re-made as kc_*.c.
- kc_const / kc_var / kc_outer (const or non-const 1/127 local in loop body or outer block, used by all three args): submit_for_cluster byte-identical to baseline (INERT). submit_for_cluster STOP.
- q1 (/Od 0x82fcad..0x82fd8b ?: form: light_brightness = far>0 ? PIN(...) : 1.0f, plus the /Od init) == 39103012 (same as if/else) -> adopted (the /Od stores through a ?: temp, attesting the conditional-expression form). q2 (init + if without else) = 66d0ccad (different; rejected).

## FINAL (wave 4)
Files: production.patch (P1 storage/layout + P2 draw + symbols.json + parked.json re-baseline; git apply --check
clean), P1_storage_layout.patch, P1_symbols_json.patch, P2_draw_on_P1.patch, P2_draw_standalone.patch (draw only,
applies on the current tree), P2_parked_json_rebaseline.patch, owner_gated_reset.patch (on top of P1+P2),
config/symbols.json + config/parked.json (edited full copies), candidates stg.c / fin2.c / fin_owner2.c / e3.c.
- P1 (zero credit, admission prerequisite): results '= {0}' (January .bss order results2@0, results@262176 is
  impossible with both uninitialised under the HCEX names; VC7 .bss law measured bss1/bss2/bss3), HCEX static local
  `warned` (rasterizer_lens_flare_submit), symbols.json: static on 5 .bss rows + _lens_flare_evaluate_corona_rotation_function,
  rename _local_lens_flare_error_printed -> ?warned@?1??rasterizer_lens_flare_submit@@9@9. csplit regen changes only
  rasterizer_lights.obj; vs regen split all 41 January symbols match (object_audit), 0 PDB-storage disagreements,
  code rows unchanged (10 exact), data 335089/335089 (objdiff one-unit).
- P2 (fuzzy improvement, zero credit): draw 25834daf -> 39103012 (6 -> 5 REAL; +0x202 closed), objdiff 99.38321 ->
  99.38626; rule-6 hand expansion removed. Park re-baseline in P2_parked_json_rebaseline.patch.
- Owner-gated: reset 0x7722 overrun with BUG comment -> EXACT (48). With P1's January layout the overrun hits
  exactly January's neighbour (local_lens_flare_parameters[0] bytes 0..33); this removes the premise of the
  2026-09-22 canonical exclusion (different neighbour in the rebuilt object). fin_owner2: 11 exact, 41/41 symbols.
- Remaining blockers: submit_for_cluster (x87 evaluation order of the direction decode, no attested lever) and
  draw sites +0x31c / +0x738. fake_match_scan 0 leads; /W3 warnings unchanged.
