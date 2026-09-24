# owner_queue/names_xbox_hs_scen - LEDGER (wave 5, base cdc8ebd3, verification only; nothing lands)

Items: (1) rasterizer_xbox .bss split with 9 descriptive names; (2) hs_scenario_definitions data split with 9 names.

## Step 0 - setup
- Read WORKER_BRIEF.md (sections 0, 9, 10). Tree at cdc8ebd3, git status clean at start.
- Slug created: scratch/w/owner_queue/names_xbox_hs_scen/

## Step 1 - rasterizer_xbox packet rebase (source: scratch/w/rasterizer_xbox_audit/patches, wave-2 base 931ed8dc)
- git diff 931ed8dc..cdc8ebd3: rasterizer_xbox.c and rasterizer_xbox_internal.h UNCHANGED; config.json and symbols.json changed elsewhere.
- All 5 wave-2 patches (4 main + followup_rasterizer_xbox_debug) pass `git apply --check` at cdc8ebd3 as-is (symbols.json hunks 4-6 at +1/+1/+2 line offsets).
- Mini tree rx/tree/ = working-tree copies (CRLF; `git show` blobs are LF and do not take the CRLF patches) + patch -p1 --binary: no rejects.
- Post-image rasterizer_xbox.c == worker cand_L.c (cmp); header == worker hdr/rasterizer_xbox_internal.h (cmp).
- Regenerated zero-offset patches at cdc8ebd3: rx/patches_cdc8ebd3/01..04 (diff -u, a/ b/ labels); each and all together `git apply --check` OK.

## Step 2 - rasterizer_xbox measurements at cdc8ebd3
- csplit control (config/ copy) -> rx/split_ctl == build/split (833/833 identical: build/split is current).
- csplit patched config (rx/config_P = config/ + patched symbols.json/config.json) -> rx/split_P: 833 compared, ONLY source/rasterizer/xbox/rasterizer_xbox.obj differs.
- qgate.py (= reviewer rgate.py: gate.py + --split-root/--tmpdir) cand (patched .c) + patched header via shadow tmpdir vs split_P, --forbid-emitted-symbol _point_from_line3d: 95/95 EXACT, guard passed. '#error' shadow control: C1189 SHADOW_HEADER_USED (shadow header proven used). -> rx/gate_cand_vs_splitP.txt
- qaudit.py (object_audit + AUDIT_SPLIT_ROOT) cand vs split_P: PASS; 188 sections ok (.bss 2230 align 4/4 nibble, blend .rdata 108 align 3/3, .data 6), 216 January symbols 0 differ. Surplus: 3 code COMDATs (_dot_product3d, _plane3d_distance_to_point, _plane3d_from_point_and_normal), 3 SDK tables, 9 literals, .drectve. Production vs build/split: FAIL(22) (unchanged from wave 2).
- provider_link cand: PASS 15/15 (both orders). -> rx/provider_link_cand.txt
- pdb storage (candidate vs split_P): 216 split symbols, 0 disagreements. Production: 25 disagreements.
- HEADER BLAST RADIUS (rx/blast.py; 7 includers; patched / unpatched header at the same shadow position; '#error' control hit for all 7):
  xbox_texture_cache 25/0, rasterizer 131/0, rasterizer_transparent_geometry 19/0, active_camouflage 12/0, environment 42/2, xbox transparent_geometry 22/0: every section identical.
  **rasterizer_xbox_models 12/2: ONE section differs - the PARKED __rasterizer_model_draw: 5168/348 both, normalized sha 89b0d7ea (unpatched header == build/base == parked.json base sha) -> cbfa8585 (patched header).** NEW since wave 2 (reviewer measured identical at 931ed8dc). Cause: models.c lost `#include "interface/hud_draw.h"` in 466698b8/cdc8ebd3, so the one-prototype removal now lands on a declaration-count tie in this fuzzy owner. No per-function status changes (still residual); strict credit unaffected; but a parked function's bytes move => parked.json base measurement for __rasterizer_model_draw must be re-baselined in the same landing (else the park check fails).
- objdiff 3.3.1 (rx/od_rx.py own project; frozen build/tools/objdiff-cli.exe 3.3.1): prod_rx == cand_rx: data 9048/9048 (.bss 2230/.data 6/.rdata 6812 all 100), code 91/95 raw with the same 4 pre-existing under-credits (get_target 91.111115, set_target 96.837944, set_target_as_texture 93.98148, set_stencil_mode 96.65072 - semantic_matches entry). models: __rasterizer_model_draw 95.08453 (orig header, == build/base == parked.json) -> 95.07629 (patched header); every other models measure identical.
- Park check (rx/park_check.py calls tools.parked_functions.validate_parked_functions on a models-only manifest/report):
  unpatched header + current parked.json: active 2 / stale 0; patched header + current parked.json: STALE 1 (__rasterizer_model_draw base sha + objdiff_percent);
  patched header + re-baselined parked.json (rx/patches_cdc8ebd3/05_parked.json.patch): active 2 / stale 0.
- => NEW REQUIRED CHANGE for apply-readiness at cdc8ebd3: 05_parked.json.patch (re-baseline __rasterizer_model_draw base sha 89b0d7ea.. -> cbfa8585.., objdiff 95.08453 -> 95.07629, evidence sentence appended). No source change. Not a new source form.
- Other rasterizer_xbox parks: none. rasterizer_xbox_environment park unaffected (all 90 sections identical). semantic_matches `_rasterizer_set_stencil_mode` still strict-exact vs split_P.

## Step 3 - hs_scenario_definitions packet rebase (source: scratch/w/hs_scenario_definitions/deliver, wave-3 base 954eebd2)
- 6e3e2d35 landed the reviewer fallback F1+F2 (static byte_swap_script_syntax_data, prototype param `size`, symbols.json line 3249 rename + static). Since 954eebd2 only hs_scenario_definitions.c (rename hunks) and hs.c (839 statics + helper) changed among hs files; hs_scenario_definitions.h, hs.h, tag_groups.h, byte_swapping.h, data.h unchanged.
- At cdc8ebd3: worker A, B, C2 FAIL git apply --check (rename already landed); C1 and D apply.
- Rebase: the worker's final post-image cand1.c (== deliver/hs_scenario_definitions.c) already spells the landed rename identically (static prototype in the prototypes block with `long size`, private-code section) -> A01 = diff(tree, CRLF(cand1.c)); the diff contains ONLY the data split + hs.h include + pragma/aggregate removal (no function-name churn). C03 = diff(tree, CRLF(cand2.c)). C01 header = deliver with_limits.h (CRLF), == tree header + HCEX enum only. C02 hs.c = tree hs.c minus the TU-local #define (hs.c CRLF, 0 bare LF).
- symbols.json A02: ONLY the 9 inserted static rows (worker B hunk 2, in place; offsets verified unused); the rename hunk dropped (landed).
- D split into A03 (config.json 282 NonMatching->Matching) and A04 (remove the hs_scenario_definitions rejection entry); JSON parses.
- git apply --check at cdc8ebd3: each of A01-A04, C01-C03 OK; A01-A04 together OK; C01-C03+A02-A04 together OK; rx 01-05 + hs A01-A04 together OK.

## Step 4 - hs_scenario_definitions measurements at cdc8ebd3
- csplit config_H (config/ + A02 symbols + A03 config): 833 compared, ONLY source/hs/hs_scenario_definitions.obj differs. Combined config_RH (item 1 + item 2): ONLY rasterizer_xbox.obj + hs_scenario_definitions.obj differ, each byte-identical to its per-item split.
- qgate vs split_H: candA (A01 post-image) EXACT 224 _byte_swap_script_syntax_data 1/1; candC (C03 + C01 header via shadow; '#error' control hit) EXACT 1/1; _point_from_line3d guard passed. (Production source vs split_H also EXACT 1/1: this function's .data relocations resolve by section+offset.)
- qaudit vs split_H: candA PASS and candC PASS: 23 sections ok incl .data 628/628 align 4/4 (January 8-byte), 40 January symbols 0 differ, surplus .drectve only. Production vs build/split: FAIL(1) (.data align 4/3 = the recorded rejection).
- pdb storage (vs split_H): candA 40/0, candC 40/0. None of the 9 names is a cachebeta public (all 9 grep 0) => static is right.
- provider_link: candA/candC no surplus external definitions: PASS.
- objdiff 3.3.1 (hs/od_hs.py): prod vs build/split data 484/1112 (.data 92.773094) == build/report.json; candA and candC vs split_H: 1112/1112 (.data/.rdata/.text 100) NATIVE (no semantic_data_matches entry needed). Names-load-bearing test: candA vs build/split (split WITHOUT the 9 rows) .data 48.931007 / data 484/1112 -> the symbols.json rows are required for the +628 data credit.
- C1 consumer sweep at cdc8ebd3 (hs/sweep.py; hs.c changed heavily in 6e3e2d35 so re-measured): hs 447/1 (1410 sections, 0 differ, 0 symbol diffs), hs_compile 61/3 (222, 0, 0), hs_runtime 64/2 (172 sections 0 differ; 64 internal $L label names shift = declared-name count, values/sections identical); per-function rows identical mod vs orig vs production; '#error' control hit for all 3. objdiff per-function maps mod == orig == build/base for all three (hs/od_sweep.txt). => C1+C02 byte-inert and park-inert.
- /W3 /Zs (w3.py): production {C4146:1}; candA/candC {C4090:2, C4146:1} (lines 96/103: hs.h `char const *hs_*_names[]` into tag_groups.h `char **names`) - byte-inert, disclose (unchanged from wave 3).
- fake_match_scan: 0 leads on all post-images (rx .c, hs A .c, hs C .c/.h).
- audit_object_admission (current tree): rasterizer_xbox already a candidate (function_gap 0, data_gap 0); hs_scenario_definitions not listed (data_gap 628) - the packet closes that gap.

## Step 5 - rasterizer_xbox extra measurements
- Names-load-bearing test (rx/config_nobss = item-1 symbols.json with the 23 .bss rows reverted to the one aggregate row): cand vs split_nobss gate 86/95 (9 reloc-identity: __rasterizer_dispose, __rasterizer_initialize, _rasterizer_get_target, _rasterizer_preinitialize__fill_you_up_with_the_devils_cock, _rasterizer_set_model_skinning, _rasterizer_set_target, _rasterizer_set_target_as_texture, _rasterizer_set_texture, _rasterizer_set_texture_non_blocking), object_audit FAIL(15). => the symbols.json rows are required; the source split alone would REGRESS 9 exact functions.
- /W3 /Zs: production and candidate identical {C4090:1, C4146:1, C4244:15}.
- Independent follow-up (rasterizer_xbox_debug 8 static wrapper rows): applies at cdc8ebd3; csplit radius rasterizer_xbox_debug.obj only; gate 18/18; object_audit (production obj vs that split) FAIL(8) -> PASS.
- Name evidence re-check: HCEX rasterizer_dx9 compiland has enum _rasterizer_target_render_d3d/_render_primary/_render_secondary and Static Local `union point2d dimensions` in the three set_texture functions; /Od build strings use `global_render_targets[_rasterizer_target_render_primary]`; /Od "node_matrix" hits are model-animation strings (fn 0x6fd580), not this object. No first-party `node_matrix_constants`, `*_copy`, `bitmap_dimensions*`.

## Step 6 - packet assembly
- patches/1_rasterizer_xbox/01-05, patches/2_hs_scenario_definitions/A01-A04 + C01-C03, patches/independent/, patches/APPLY_ORDER.txt; symbols/ (3 in-place edited symbols.json copies).
- git apply --check at cdc8ebd3: every patch alone; item1 + item2A + independent together; item1 + item2C together: all OK.
- Combined post-images reproduced outside the repo (git apply in the session scratchpad, not a repo) and cmp-identical to rx/tree, hs/treeA, hs/treeC, hs/config_RH.
- git status at cdc8ebd3: clean (0 lines) after all steps.

## Step 7 - HEAD moved during the run
- HEAD advanced cdc8ebd3 -> eb33baec ("Ledger: batch 5 ... tallies at cdc8ebd3"), integrator commit touching ONLY docs/object_matching_logs/claude_fifty_objects_20260925.md; git status clean. No source/config/tools/build change => every measurement above stands. All packet patches re-checked: `git apply --check` clean at eb33baec too (item1 + item2A + independent; item1 + item2C).

## Step 8 - name attestation re-check (January split rasterizer_xbox.obj printable strings, word-boundary)
- Attested bare in January strings (14): d3d, global_d3d_surface_render_primary_z, global_d3d_{texture,surface}_render_secondary, global_d3d_{texture,surface}_shadow_{primary,secondary}, global_d3d_{texture,surface}_sun_glow_{primary,secondary}, global_d3d_{texture,surface}_water (+ d3d_palette, 7 strings). `rasterizer_xbox_d3d_globals` appears in NO string.
- 0 hits (the 9 held names): node_matrix_constants, bitmap_dimensions_non_blocking, bitmap_dimensions, global_d3d_texture_render_primary, global_d3d_surface_render_primary, global_d3d_texture_render_secondary_z, global_d3d_surface_render_secondary_z, global_d3d_texture_render_primary_copy, global_d3d_surface_render_primary_copy. CONFIRMED (matches wave-2 review).
- hs: HCEX globals dump re-grep: 10 `*_block_fields` statics, 23 distinct `*_bs_definition`; tag_reference_definition names are only public `global_<group>_reference`; no hs tag-definition data. /Od build: no hs_scripts_block / data_array_header / _bs_definition / _block_fields strings. CONFIRMED (matches wave-3 review).
- Wrote MEASURED.md, RULING.md.
- Final: git status clean at eb33baec; deliverables: patches/ (APPLY_ORDER.txt), symbols/, MEASURED.md, RULING.md.
