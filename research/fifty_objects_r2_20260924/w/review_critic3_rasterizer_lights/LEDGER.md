# review_critic3_rasterizer_lights LEDGER (adversarial review of critic_3 patch 17)

Worktree HEAD 5d3ca708. Unit source/rasterizer/rasterizer_lights claimed (review_critic3_rasterizer_lights) for verification.
Patch under review: scratch/w/critic_3/patches/17_rasterizer_lights.patch
  symbols.json line 6206 (file_offset 1509488 = 0x170870) `_lens_flare_submit_parameter_get` -> `_lens_flare_parameters_get` (static kept)
  rasterizer_lights.c: symbol-listing comment, prototype, definition, 4 call sites.

## R1 first-party name evidence (independent; own readers, body_ident.py)
- Sept-2001 raw cachebeta.map (timestamp 3bb0dabe 2001-09-25), Static symbols: rasterizer_lights.obj statics in order
  _screenshot_in_progress 0x16e440 | _lens_flare_parameters_get 0x16e470 | _lens_flare_occlusion_test_results_get 0x16e4b0 |
  (publics) _rasterizer_lights_reset_for_new_map 0x16e5a0 => sizes 0x30 / 0x40 / 0xf0, EXACTLY January's 0x170840(0x30) /
  0x170870(0x40) / 0x1708b0(0xf0) / reset 0x1709a0. Aug-2001 map (3b7b3218): _lens_flare_parameters_get 0x153ad0 (0x40) ->
  _lens_flare_occlusion_test_results_get 0x153b10. Neither map contains `lens_flare_submit_parameter_get`.
- Body identity: January cachebeta.exe file 0x170870 (0x40) vs Sept xbe VA 0x17f470: identical offsets+mnemonics+bytes except
  the 6 relocated operands (count load, 2 assert strings, display_assert, system_exit, parameters base); the assert LINE
  immediate is 0x43 in BOTH (Aug: 0x37). Sept relocated operands are consistent: string pushes differ by 0x40 = Sept map
  .rdata delta (lens_flare_index string 0x5cebc -> file string 0x5cefc), count-parameters delta 0xa000 in both builds.
- Callers (E8 scan of whole .text): January cachebeta.exe calls file 0x170870 from exactly 2 sites: 0x17100c
  (_rasterizer_lens_flare_submit +0x14c) and 0x171359 (_rasterizer_lens_flares_submit_occlusion_tests +0x79). Sept xbe calls
  0x17f470 from exactly 2 sites: map 0x16ec0c (_rasterizer_lens_flare_submit +0x14c) and 0x16ef59
  (_rasterizer_lens_flares_submit_occlusion_tests +0x79) - identical owners AND offsets. (begin_for_new_frame and the two
  draw sites inline it in both builds.) Aug: 1 E8 site (earlier code).
- January strings (cachebeta.exe): no `lens_flare_submit_parameter*`, no `*parameters_get`; `lens_flare_parameters` occurs as
  assert text (`lens_flare_parameters->light_index>=0 ...`, and the bare `lens_flare_parameters` = our line-76
  `match_assert(lens_flare_parameters)` in occlusion_test_results_get). NOTE: critic_3 calls the bare string a "data-array
  name"; it is the assert expression text (a parameter name). Supporting vocabulary only; not decisive.
- Atlas: 1749betaP (Aug) exact 0x164ad0, Sept cachebeta_xbe map 0x17f470 (= my map VA), 2002-01-14 cachebeta_xbe exact
  0x182270 all `_lens_flare_parameters_get`; January exe atlas 4cc87b45 has no row at 0x570870 (occlusion_test_results_get fuzzy).
  No atlas build names `lens_flare_submit_parameter_get` (old name is an invented descriptive name).
- HCEX.pdb (DIA2Dump -sym): neither name present (lens_flare_occlusion_test_results_get is present); nothing contradicts.
- cachebeta.pdb publics: neither name public -> file static; symbols.json keeps "static": true. Correct.

## R2 references (git grep over all tracked files at 5d3ca708)
- Old name occurs in: config/symbols.json:6206, source/rasterizer/rasterizer_lights.c (listing comment, prototype, definition,
  5 call sites incl. begin_for_new_frame) - ALL covered by the patch (cand.c has 0 remaining occurrences, 8 new).
- Not in config/parked.json (draw park keys on _rasterizer_lens_flares_draw; its sha/size/relocs are name-free), not in
  semantic_matches.json / semantic_data_matches.json / symbol_ownership.json / object_admission_rejections.json, not in tools/
  or tests. Remaining mentions are historical prose (docs/object_matching_logs/rasterizer_lights_obj_opus5_150k_w2_20260914.md:27,
  ..._250k_w3c_20260915.md:105) and round-1 research patches (research/fifty_objects_20260925/w/rasterizer_lights/*) - leave.
- Coordination: round-1 P2 draw packet (FUZZY_IMPROVED, not landed; parked.json still base 25834daf) edits the same line 1145
  (`lens_flare_submit_parameter_get(lens_flare_index)` -> `(sun_lens_flare_index)`); if landed after 17 it needs a trivial
  rebase to the new name.
- New name collides with nothing (no other definition/declaration/macro/symbols.json row).

## R3 measurements (slug-local; prod.c = git show HEAD, cand.c = GNU patch of patch 17 onto it)
- git apply --check (worktree, read-only): clean. Sequential application of all critic_3 patches onto HEAD copies: the
  rasterizer_lights.c result == cand.c and symbols.json rows 6200-6212 == single-patch result (no interaction).
- csplit (build/tools/csplit.exe) control with config copy: 833/833 byte-identical to build/split. Candidate config (patched
  symbols.json only): exactly 1 object differs (rasterizer_lights.obj); objcmp ctl vs cand = IDENTICAL with the single name
  map (sanity: unmapped compare flags the 2 .text reloc sets + 1 symbol row, so the compare is sensitive).
- Gate: prod vs build/split 10 exact / 3 residual (submit_for_cluster, draw, reset [sha]); cand vs emulated split: the SAME
  13 rows modulo the rename (10/3). Negative control: cand vs the OLD split = 7/5/1 (renamed fn UNWRITTEN, submit and
  occlusion_tests [reloc-identity]) => source and symbols.json MUST land together + csplit regen.
- objcmp prod.obj vs cand.obj (all non-.debug$S sections bytes+relocs+symbols, name mapped): IDENTICAL; build/base obj vs
  prod.obj IDENTICAL; build/base vs cand IDENTICAL (mapped).
- Battery (with_split redirect; tools unmodified): object_audit FAIL(3) both sides = the 3 pre-existing residual .text
  sections; 41 January symbols, 0 differ; pdb_storage 0 disagreements both; surplus_identity 11 COMDATs, 0 not identical
  both; provider_link PASS (25 surplus, both orders) both. Outputs identical modulo the rename (battery_prod/cand.txt).
- Data: tinfo --data on emulated split == build/split (156 lines identical).
- objdiff 3.3.1 (sha1 3130e428) one-unit reports: prod/ctl, cand/split_cand, base/build-split all 1996/5244 code, 9/13 fns,
  335089/335089 data, 99.06512 - identical; renamed fn 100.0 under the new name.

## R4 house rules / strip test
- Only an identifier changes (plus listing comment). Authentic first-party name (Sept+Aug 2001 maps, same lineage, identical
  body incl. assert line 67) replaces an invented descriptive one. No new construct, scope, declaration, cast or directive:
  strip test and /Od declaration-order checks are vacuous. Prototype stays in the owning .c (file static).

## R5 verdict
APPROVE as-is (no amendment). Zero credit, byte-inert. Integrator: land symbols.json + .c together, then csplit regen.
