# source/rasterizer/xbox/rasterizer_xbox_models::__rasterizer_model_draw

## verdict
FUZZY_IMPROVED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_models\\production.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_models\\best_candidate.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_models\\parked.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_models\\config\\parked.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_models\\owner_F1_draw_lab_scaffold.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_models\\LEDGER.md"
]

## production_changes
Apply C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\rasterizer_xbox_models\production.patch (one file, source/rasterizer/xbox/rasterizer_xbox_models.c, no header touched; git apply --check clean; applying it to a copy reproduces best_candidate.c (g6) byte for byte). For draw it moves perpendicular, parallel, vertex_constants[3] and specular_constants[2][4] out of function scope into one block. The block opens after shader_get_and_verify_type, at the reflection-colour computation, and closes after the two-sided pass. Declaration order follows the /Od RTC order. Apply it together with parked.patch (config/parked.json; a full edited copy is in scratch/w/rasterizer_xbox_models/config/parked.json), which re-baselines this park: base 5168/348, normalized_sha256 771bf2e799fe91d357ec0dc2e56d91cb8f05be67aad0fc11ea49ffeafc002f26, objdiff 95.71134, with an evidence note appended. Zero exact credit.

## evidence
Baseline: gate 12 exact / 2 residual. Draw is [relocs 348!=350, sha], 56 real regions, 95.08453% (park value reproduced exactly with objdiff-cli 3.3.1 on a private mini-project). The relocation multiset differs only by January's out-of-line _subtract_vectors3d and _point_from_line3d calls at 0x5d2 and 0x605.

Source shape, three independent sources:
- January bytes, via /Od 0x7f6710 (RTC gradient_color [ebp-0x84]), 0x7f704f..0x7f70cd: subtract_vectors3d(&upper,&lower,&gradient_color), then point_from_line3d(&lower,&gradient_color,periodic_function_evaluate(..),&self_illumination_color).
- January slot sharing: gradient_color shares with cc1 and seed shares with vertex_shader_work. By the escape-scope law these locals sit in a block that closes before those uses; /Od declares them in the later build's guarded block.
- HCEX: real_rgb_color is a union with no vector member, so the casts are needed.

Finding F1 (lab):
- The /Od form alone (c1) still inlines subtract_vectors3d (5184/349).
- The site sits exactly one inline-gating step above subtract's cost: one extra if puts the call out of line.
- VC7 counts a conditional region BEFORE folding it. `if (TRUE)`, `if (sizeof..)`, `if (shader_model)` and `if (shader)` produce no code, yet make draw [sha]-only: 5168, 350 relocations, frame 0xa4, every January slot including gradient [ebp-0x48].
- The same trick around the camera vector_from_points3d breaks it. So January's invisible region starts after the camera-distance statement and encloses the self-illumination block.
- Both later lineages guard exactly that section: /Od `if (ps>=1.4 || detail_mask==none)`, and the third-party HCEX decompile (corroboration only) `if (!detail_mask)`. January's bytes have no test there.

Other decoded fixes:
- Plasma sub-structure: /Od 0x7f035c `add eax,0x28` into a named pointer. This makes 0xba exact but flips submit's name-count tie.
- Constant-array block: CALL-RESULT ALIAS SCOPE law; January stores the constants early at +0xa88 and +0x12bd.
- The lab scaffold c8 (owner_F1_draw_lab_scaffold.c) reaches 99.57182% with 10 real regions, all H3/M8 ties: 0x1da edi reload (+1..3 pre-function declarations), 0x5f9 fstp order, 0xa17 perpendicular.alpha reassociation, and 0xdf9/0xf6d fog x87 first-term order.

Measured inert: fog operand order (byte-identical), fog declaration order, gradient as real_vector3d, a camo `return;`, extra inline expansions, block position, the C4013 prototype, and dead/unused-local oracles.

Landable result (best_candidate.c):
- 12/14 exact kept; tags unchanged; frame and emitted-symbol set identical to production.
- 95.08453 -> 95.71134%, 56 -> 55 real regions.
- fake_match_scan 0 leads; /W3 warning set unchanged.

## blockers
F1: January's zero-code conditional region around the self-illumination block cannot be recovered from bytes. Every tested form is an invented condition until there is first-party evidence or an owner ruling.

The /Od-attested rgb->vector/point casts for the subtract_vectors3d/point_from_line3d calls are byte-inert. They are still owner-gated while the caller is not strictly exact.

In every bare-block form the plasma sub-structure edit flips the transparent-submit name-count tie. The oracle shows +1..+4 declarations before submit restores it.

H3/M8 ties remain even with the scaffold.

## reopen_criteria
Reopen if the owner rules on, or first-party evidence names, the January form of the conditional region around the self-illumination block. Measured-equivalent candidates: `if (TRUE)`, `if (shader_model)`, `if (shader)`. That ruling should come together with admitting the /Od-attested casts on the self-illumination colours.

Then apply the plasma sub-structure (/Od 0x7f035c) and close the H3/M8 ties listed in owner_F1_draw_lab_scaffold.c. Re-gate transparent submit after every draw edit; it is a name-count canary.

## task notes
Neither function became exact.

Key new mechanism, F1: VC7's per-site inline gating counts an if-region before it folds the condition. `if (TRUE)` or a null test of an already-dereferenced pointer produces no code, yet drops the site's budget by one step. This explains January's out-of-line _subtract_vectors3d in draw, with no visible test in the bytes. With such a region plus the /Od-attested calls, the block scopes and the plasma sub-structure, draw reaches [sha]-only at 99.57%: size, relocations, frame and every slot are exact. That is an owner question (the January condition is unrecoverable), not a landing.

Also measured: plane3d_distance_to_point inlines at any depth when its argument is a constant global address, and stays out of line when the argument is a pointer local. That argument-shape effect contradicts the Lane C law that argument shape is inert, and it explains the draw/env asymmetry.

The transparent-submit function (already exact) is a fragile canary: plasma and many bare-block draw edits flip its tie. The integrator must run the full sweep after applying. The production patch keeps submit and all other 11 exact functions exact against both the current and the regenerated split.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\rasterizer_xbox_models\LEDGER.md has every probe, recorded as hypothesis, evidence, change and result: c1..c9, d1/d3, f1..f3, g1..g6, lab L_*, oracle o/u/f/s series, env e_* and L_h_*. It also has the evidence pass (E1-E6), the objdiff mini-project measurements, the csplit/audit verification, the do-not-repeat list and the reopen criteria.

No tracked file was modified; git status is clean. One stray redirect to /LEDGER.md was attempted before a shell variable was set. It failed with permission denied and nothing was created (verified).

Deliverables are in scratch/w/rasterizer_xbox_models/:
- production.patch (.c only; draw 95.08->95.71, env 95.87->96.39; 12/14 exact kept);
- parked.patch and config/parked.json (both parks re-baselined);
- symbols.patch and config/symbols.json (5 wrapper storage rows; csplit changes only this object);
- best_candidate.c (g6);
- owner_F1_draw_lab_scaffold.c (not landable).
