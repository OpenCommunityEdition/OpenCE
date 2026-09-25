# REVIEW (round 2): source/hs/hs_runtime :: _render_debug_trigger_volumes (1088 B)

Worker verdict: FUNCTIONS_EXACT_OBJECT_BLOCKED. Worker slug: scratch/w/hs_runtime__render_debug_trigger_volumes/.
Reviewer slug: scratch/w/review_r2_hs_runtime__render_debug_trigger_volumes/ (the ':' is sanitized). Full probe log: LEDGER.md.

## Verdict

**approve = false for production.patch as submitted. It is fixable: apply the amended patch.**

- Amended patch, LF: `amended/production_amended.patch`. CRLF working-tree variant: `amended/production_amended_crlf.patch`.
- Retire the park in the same integration with the worker's `park_retire.patch` (or `park_retire_crlf.patch`). It is approved unchanged.
- The amended object is identical to the worker's candidate: every non-debug section, every relocation and the symbol table match. All of the worker's exactness and admission results therefore carry over unchanged.

## Blocking finding F1: the block grouping contradicts the /Od declaration order

**What the /Od frame shows.** In the /Od frame (od_tv.txt, RTC descriptor 0x607464), `matrix` sits at -0x40. It is the first user slot, above volume_index -0x48, scenario -0x4c and volume -0x50. The complete order is:

matrix, volume_index, scenario, volume, local_extent, world_extent, edge_index, corner, sides, side, axis, color, center, ray, result.

**What the submitted patch does:**
- It declares `matrix` first in the volume-loop block. That is textually after the outer `short volume_index; struct scenario *scenario`.
- It declares center, ray and result at the top of the loop block, before the edge-loop locals.

**The worker's variants.** None of the seven order variants puts matrix before volume_index. I checked fn_od_order.txt, fn_od_outer_only.txt and fn_bareblock.txt. So the MANIFEST/LEDGER claim that "the full /Od frame order" was measured is inaccurate.

**Precedent.** The wave-2 light_volumes review (research/fifty_objects_20260925/results/wave2/REVIEW__source_objects_widgets_light_volumes.md) rejected the same situation: a grouping that contradicts the /Od declaration order, when an equally exact /Od-consistent grouping exists. Brief section 9 records the same rule.

**Reviewer probes.** Each is a full-TU gate with `--all`, and each result object is identical to cand.obj in every non-debug section:

| probe | change from the candidate | result |
| --- | --- | --- |
| rv_matrix_outer.c | `real_matrix4x3 matrix;` moved to the top of the if-block, before volume_index | EXACT, 65/1/0 |
| rv_full_od.c | rv_matrix_outer plus `{ real_point3d center; real_vector3d ray; struct collision_result result; ... }` after the edge loop, giving the complete /Od order | EXACT, 65/1/0 |

**Why the block is legitimate:**
- Escape law: the addresses of center, ray and result all reach out-of-line calls. January's relocations show `_collision_test_vector` and `_render_debug_string_at_point`. So a scope of their own is permitted.
- Strip test: the block is byte-inert, so it is not buying the match.
- New independent corroboration from HCEX_Release.pdb, found by a raw CodeView scan (hcex_locals.py):
  - The records are matrix, local_extent, world_extent, sides, result, ray, with no S_BLOCK32 records.
  - S_COMPILE2 for RELEASE_CACHE\halo\hs_runtime.obj gives language C on VC 16, so declarations must be C89 (declarations only at the start of a block).
  - The record order is not first-use order, because result comes before ray.
  - Both later builds therefore put ray and result after `sides`. In January, sides is zero-initialised once per edge iteration, so it lives in the edge-loop block. A C89 source can only declare ray and result after it inside a block that follows the loop.
  - This is corroboration, not proof.
- Fallback: if the owner judges the block invented, `fallback/production_matrix_only.patch` (= rv_matrix_outer) is also EXACT and gives the identical object. It fixes the matrix contradiction, which is expressible in C89 without new structure, and leaves only the center/ray/result deviation, which C89 forces without a block.

**What stays as the worker wrote it:** volume declared in the loop with its initializer (rule 17). /Od's outer `volume = NULL` is a dead store, and the /Od assert anchor has drifted by one line (0x214 against January's 0x213). rv_volume_outer is EXACT as well; it was kept for information only.

## Independently verified (all clean)

- **Patches.** At worktree HEAD b7c1b7d9 (source/config/tools equal to f6d00a8c), `git apply --check` is clean for the worker's four patches and for my amended LF, amended CRLF and fallback patches.
  - GNU patch rebuilds cand.c from the HEAD blob byte for byte, and rebuilds amended/hs_runtime.c (LF) and hs_runtime_crlf.c (CRLF).
  - Note: both CRLF patches need `patch --binary` under GNU patch, which otherwise reports "different line endings". `git apply` is unaffected.
- **Gate.** The applied file gives 65 exact / 1 residual / 0 unwritten. The only residual is `_hs_evaluate_begin_random` [sha]; `_render_debug_trigger_volumes` is EXACT 1088. The baseline is 64/2/0. My baseline object equals build/base; my candidate object equals the worker's cand.obj.
- **object_audit.** FAIL(13), all pre-existing: begin_random plus the 12 converter storage rows; the baseline is FAIL(14). The only new surplus is +.text `_point_from_line3d` (48 B).
- **Surplus identity.** `_point_from_line3d` is new and IDENTICAL to January's selected copy (action_charge.obj). The 4 older surplus COMDATs are also IDENTICAL.
- **Provider link, --baseline.** PASS in both orders.
- **nodup_census.** 24 definers, 0 NODUP.
- **COMDAT storage.** Selection ANY and external storage. cachebeta publics line 6312 lists `_point_from_line3d`, so storage agrees.
- **Owner ruling 5 (2026-09-21, all-inlined header-inline class).** Every condition holds: a genuine real_math.h `__inline`, byte-identical, strict-exact caller, zero unit regressions, and no hand expansion. The patch actually removes production's hand-expanded arithmetic. docs/campaign_house_rules.md lines 33-40 carry this exception.
  - The opt-in `--forbid-emitted-symbol _point_from_line3d` guard flags the COMDAT, as expected for this class. No test or build step enforces it for hs_runtime.
- **/Od attestation, re-read from od_linear 0x606c80..0x607462:**
  - Both side arms call fn 0x42e0d0 (point_from_line3d) three times with (corner, sides, t, corner). The arms are duplicated in the source text.
  - Floats: 0x93dd5c = 1.0, 0x9401b4 = -1.0 and 0x93dd50 = 0.5 (center).
  - `sides = {0}` zero-inits 24 B per edge iteration.
  - side is computed before axis.
  - In the else arm, color is copied from blue and its alpha is set.
  - render_debug_string_at_point is called twice under if/else, with yellow 0xa1c244 and white 0xa1c22c.
- **Strip tests:**
  - single_tail.c, re-gated: RESIDUAL. The duplicated tail is load-bearing.
  - rv_ternary.c, a new test that replaces the if/else string pair with the production ternary: RESIDUAL (25 alndiff hunks). The if/else is load-bearing and /Od-attested.
  - The helper, `sides[2]` and name choices are HCEX- or RTC-attested. Names are inert (c1 and c2).
- **Names.** matrix, local_extent, world_extent, sides, result and ray are HCEX-attested; corner, color and center come from the RTC descriptor. The scalars keep production's descriptive names.
- **/W3 /WX.** Clean on the amended file.
- **Composition with scratch/w/hs_runtime_storage/hs_runtime_converter_storage.patch:**
  - The result is order-independent.
  - Gate 65/1/0.
  - object_audit FAIL(1), begin_random only.
  - provider_link PASS on all surplus.
  - The composed object is identical to the worker's compose.obj.
- **park_retire.patch.** Parked entries go from 86 to 85. Only {source/hs/hs_runtime, _render_debug_trigger_volumes} is removed; the other entries and the begin_random park are unchanged. The JSON is valid.
- **Headers, config, symbols.** There are no header, symbols.json or config status changes. The math/real_math reservation is untouched.

## Integrator steps

1. Apply `amended/production_amended.patch` (LF, git apply) and `park_retire.patch` together. A moved parked body fails PROGRESS, so the park must go with the source change.
2. Run a full ninja and the stable sweep. Ruling 5 requires the full ownership/build sweep.
3. After the reserved Codex real_math packet lands, re-run surplus identity and provider_link for hs_runtime.
4. The object stays NonMatching because of the retired `_hs_evaluate_begin_random` tie.
