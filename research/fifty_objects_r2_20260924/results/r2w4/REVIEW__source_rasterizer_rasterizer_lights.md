# REVIEW source/rasterizer/rasterizer_lights

approve: True

amended_patch: None

## checks
All checks were run independently at 5d3ca708 (git status is clean, tracked files are untouched and the claim is released). Evidence is in C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_critic3_rasterizer_lights\ (LEDGER.md, MANIFEST.md, body_ident.py, objcmp.py, splitcmp.py, gate_*.txt, battery_*.txt, objdiff/).

(1) Name evidence, read with my own readers:
- Sept-2001 cachebeta.map (3bb0dabe), Static symbols section: the statics run _screenshot_in_progress 0x16e440, _lens_flare_parameters_get 0x16e470 (0x40), _lens_flare_occlusion_test_results_get 0x16e4b0 (0xf0), then reset at 0x16e5a0. That is the same order and sizes as January's 0x170840/0x170870/0x1708b0/0x1709a0.
- Aug-2001 map: the same name at 0x40. Neither map contains the old name.
- Body: January cachebeta.exe file 0x170870 is byte-identical to Sept xbe VA 0x17f470 apart from 6 relocated operands. The assert line immediate is 0x43 (67) in both builds (Aug has 0x37). The Sept string pushes differ by exactly the .rdata delta shown in the map.
- Callers: a whole-.text E8 scan finds exactly 2 call sites in each build, with the same owner functions at the same offsets (_rasterizer_lens_flare_submit +0x14c, _rasterizer_lens_flares_submit_occlusion_tests +0x79).
- January strings: nothing names either identifier.
- Atlas: Aug exact, Sept map (0x17f470) and 2002-01-14 xbe exact all give `_lens_flare_parameters_get`.
- HCEX.pdb (DIA2Dump): neither name is present.
- cachebeta.pdb publics: neither name is public, so the function is file static and `"static": true` is correct.

(2) References: git grep over every tracked file. The old name appears only in symbols.json:6206 and in rasterizer_lights.c (the listing comment, prototype, definition and 5 call sites), and the patch covers all of them. It does not appear in parked.json, the semantic files, symbol_ownership, the rejection list, tools or tests. The only other mentions are historical prose in docs and round-1 research patches. The new name collides with nothing.

(3) git apply --check is clean. Applying all critic_3 patches in sequence to HEAD copies gives the same rasterizer_lights.c and the same symbols.json rows as applying patch 17 alone.

(4) Split emulation: csplit with a control config reproduces build/split exactly (833/833). With the patched symbols.json only rasterizer_lights.obj changes, and my objcmp reports it IDENTICAL once the single name mapping is applied. As a sanity check, the unmapped compare does flag differences.

(5) Gate: all 13 January functions. Production against build/split is 10 exact / 3 residual (submit_for_cluster, draw, reset, all [sha]). The candidate against the emulated split gives the same 13 rows apart from the rename. Negative control: the .c change alone against the old split drops to 7/5/1.

(6) objcmp: candidate vs production and vs build/base are IDENTICAL, covering all non-.debug$S section bytes, relocations and symbols with the name mapped.

(7) Section-4 battery through the with_split redirect gives the same result for production and candidate:
- object_audit FAIL(3): only the 3 residuals that were already there; 41/41 January symbols ok.
- pdb_storage: 0 disagreements.
- surplus_identity: 11 COMDATs, 0 not identical.
- provider_link: PASS in both orders.

(8) Data: tinfo --data on the emulated split matches build/split (156 lines, identical).

(9) objdiff 3.3.1 (sha1 3130e428), one-unit reports for prod, cand and base: all identical at 1996/5244 code, 9/13 functions, 335089/335089 data and 99.06512 overall; the renamed function is 100.0.

(10) House rules: the change is an identifier only, replacing an invented descriptive name with an authentic first-party one. Nothing new was added, so the strip test and the /Od declaration-order check have nothing to cover. The prototype stays in the owning .c.

## issues
Nothing blocks this. Approved as-is with no amendment. It earns zero credit, is byte-inert, and no object completes because the 3 residuals remain.

Notes:
(a) Inaccurate wording in the packet evidence: critic_3 calls January's bare "lens_flare_parameters" string a "data-array name". It is actually the assert text of match_assert(lens_flare_parameters) in lens_flare_occlusion_test_results_get (a parameter name), so it only supports the vocabulary. The name rests on the Sept/Aug maps, the byte-identical body with the same assert line 67, and the callers at identical offsets. All of these I confirmed independently.
(b) For the integrator: symbols.json and rasterizer_lights.c must land in the same commit, followed by a csplit regen. Without the regen, submit and occlusion_tests become [reloc-identity] and the renamed function shows UNWRITTEN.
(c) Coordination: the round-1 P2 draw packet (research/fifty_objects_20260925/w/rasterizer_lights/P2_draw_*.patch, FUZZY_IMPROVED, not landed) edits the same call at line 1145 using the old name. If it is ever landed after patch 17, it needs a trivial rebase.
(d) The two historical docs ledgers that mention the old name in prose (rasterizer_lights_obj_opus5_150k_w2_20260914.md:27 and rasterizer_lights_obj_opus5_250k_w3c_20260915.md:105) should stay as they are.
