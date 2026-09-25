# source/rasterizer/xbox/rasterizer_xbox_environment

## verdict
FUZZY_IMPROVED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\rasterizer_xbox_environment\\patches\\p1_spot_od_shape.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\rasterizer_xbox_environment\\patches\\p2_object_prereqs.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\rasterizer_xbox_environment\\cand\\cand_spot_odorder.c",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\rasterizer_xbox_environment\\cand\\cand_full.c",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\rasterizer_xbox_environment\\lab\\oracle\\forcecur.py",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\rasterizer_xbox_environment\\lab\\oracle\\forced_7_10894708.obj",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\rasterizer_xbox_environment\\lab\\m8count.py",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\rasterizer_xbox_environment\\LEDGER.md",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\rasterizer_xbox_environment\\MANIFEST.md"
]

## production_changes
Zero exact credit. (1) Proposed: scratch/w/rasterizer_xbox_environment/patches/p1_spot_od_shape.patch passes git apply --check at f6d00a8c. It replaces the parked 944 B spot body with the /Od-attested shape: `real_matrix4x3 light_matrix`, separate cross_product3d and normalize3d statements, an `inner_radius` local, `cone_scale = 1.0f/(radius - inner_radius)`, and `.l = -cone_scale * inner_radius`. Declarations follow /Od frame order (byte-inert). Measured result: 960 B / 64 relocs, sha a0b9f9f4383b30bc, still residual; the other 42 functions stay exact with an identical name set; the point_from_line3d guard passes. Because the parked function's bytes move, the integrator must update the config/parked.json entry for _rasterizer_environment_specular_spot_light_begin: base -> size 960, relocation_count 64, normalized_sha256 a0b9f9f4383b30bc33f95cd18b48b78abc7d1e23b6b93fc8f3ac56ec33eff121; re-measure objdiff. Suggested evidence text: 'Frame, size and relocations are January-exact with the /Od 0x7e4590 shape (RTC light_matrix, named half-radius local). Forcing one decision in C2's block-local round-robin allocator plus two oracle-only M8 fillers gives full January sha e0331f6b, so the residual is (a) the function's IL-symbol count, needing +2 mod 8, and (b) the normalize3d argument being a pinned operand in January but a local take in ours. Reopen only for a first-party-attested construct that produces both.' Keep class register-allocation. (2) Prepared but NOT for landing now: patches/p2_object_prereqs.patch adds `#define/#undef REAL_MATH_EXTERNAL_NORMALIZE3D` and makes the permutation index static. It keeps 42 exact and removes the surplus _normalize3d that differs from January's copy (a Rule-6 surplus). It must be applied together with in-place "static": true edits to config/symbols.json lines 5722-5727 (the 6 XDK wrappers in env's range), 5755 (spot) and 22727 (_specular_light_vertex_shader_permutation_index). Applied alone it adds an audit storage row. It also needs an emulated-split audit, which should run only once the object can be admitted.

## evidence
The /Od function 0x7e4590 was read: RTC 0x7e4968 gives light_matrix at [ebp-0x40], size 52, and HCEX has the same local. The body is a separate normalize3d statement plus a named radius*0.5f local at [ebp-0x50]. The lab od shape has 248 = 248 instructions. With 2 M8 fillers only 6 real hunks remain, all in the rotation family. Compile order (spot moved to January's slot) is inert. The TU name-count oracle (0..45 single names at 2 positions, 92 compiles) is inert. Scoping, all matrix statement orders, member-wise copies, the XDK public wrappers, extnorm, the DX9 flag (+1 M8), pointer or magnitude locals (+1 each) and an inline normalize wrapper were all measured; none move the rotation. M8 accounting was measured: scalar +1; struct member .i +2 and .j +3; named vertex_constants members count 0, flat/.n[] count 5; type structure and the helper's own locals are inert. C2's local allocator was decoded live with dbg32c (nothing on disk patched): chooser around 0x10756684, owner table 0x1088b1d8, round-robin cursor [0x1088b96c] over {eax,ecx,edx} stored at 0x1075680d, pinned operands written at 0x10756332 without moving the cursor. The TAKE trace for ours is copy1 ecx, fwd edx/edx/ecx, copy2 edx, up.k edx, then the normalize ARG takes eax, and def2/px/py/pz/vc take ecx/edx/eax/ecx/edx. COUNTERFACTUAL: /Od shape + D=2 fillers + one forced cursor store at the 7th take (the argument does not advance the cursor) is STRICT EXACT: 960/64, full normalized sha256 e0331f6b94a74f9c4b4cc2fbed6d8a0cf3d0e3b8d65c487daeac0b7a0e4ab7af equals January's. This was re-verified on the landed declaration order.

## blockers
(a) The spot row needs a genuine construct that adds +2 (mod 8) IL symbols and makes the normalize3d argument a pre-assigned (pinned) operand, or adds two vanishing local takes between the argument and the definition reload. None was found after about 30 evidence-backed shapes, so the stop rule applies. The only +1 candidate, the DX9 success flag, has no Xbox witness. (b) __rasterizer_environment_lightmap_draw: the astra intensity.c candidate is strictly exact but held by owner/reviewer standard, because the per-arm stage-0 filter blocks have no witness and Oct-2001 betaP has the same merged tail. (c) 8 storage rows need symbols.json static:true (6 wrappers, spot, permutation index). (d) The TU-view packet (17 TU-local struct views, 4 consumer-local externs) is untouched. (e) math/real_math is reserved to Codex; no real_math.h change was attempted.

## reopen_criteria
Spot: a first-party-attested source fact that (1) shifts the function's IL-symbol count by +2 mod 8 (lab/m8count.py prints c; od = 0) AND (2) turns the `&light_matrix.left` normalize3d argument into a pinned (globally allocated or fixed) operand, or adds exactly two vanishing local takes before the light->definition reload. Acceptance test: the unforced build reproduces sha e0331f6b...4ab7af. The forced counterfactual in lab/oracle/forcecur.py shows these are the only gaps. Object: owner ruling on lightmap_draw's per-arm filter blocks, plus symbols.json static edits applied with p2 under an emulated-split audit, plus the TU-view declaration packet.

## manifest
C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\rasterizer_xbox_environment\MANIFEST.md

ledger: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\rasterizer_xbox_environment\LEDGER.md
