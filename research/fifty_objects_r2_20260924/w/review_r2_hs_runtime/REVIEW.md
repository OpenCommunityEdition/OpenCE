# REVIEW (round 2, adversarial): source/hs/hs_runtime, verdict FUNCTIONS_EXACT_OBJECT_BLOCKED

Worker: scratch/w/hs_runtime__render_debug_trigger_volumes/ (production.patch, production_crlf.patch, park_retire.patch).
Composed with: scratch/w/hs_runtime_storage/hs_runtime_converter_storage.patch.
Reviewer slug: scratch/w/review_r2_hs_runtime/. Every probe is in LEDGER.md. Tree: HEAD b7c1b7d9 (= f6d00a8c for source/config/tools), clean.

## Verdict

**approve = false for production.patch as submitted. The problem is fixable.**

- Land `amended/production_amended.patch` instead. Use `amended/production_amended_crlf.patch` when applying to the CRLF working tree.
- Keep the worker's `park_retire.patch` unchanged and land it in the same integration.
- The amended object is identical to the worker's candidate object: all 154 non-debug sections match, and all 488 symbols match, including the `$L` numbering. Every exactness and admission result therefore carries over.
- The verdict class is correct. `_render_debug_trigger_volumes` is EXACT. The object stays blocked only by `_hs_evaluate_begin_random`, a retired 4-ModRM-byte register tie (park private-register-convention; w1 and w3c SKIPPED-EXHAUSTED).

## Blocking finding F1: the grouping contradicts the /Od declaration order

**What the /Od frame shows.** I re-read the /Od function (od_linear 0x606c80..0x607470) and the RTC descriptor at 0x607464, which I parsed with odbuild.rtc.

The frame follows declaration order, not first-use order:
- scenario is stored first (0x606cce) but sits below volume_index.
- matrix is first used at 0x606d4f but holds the topmost slot (-0x40).

The full order is:

matrix, volume_index, scenario, volume, local_extent, world_extent, edge_index, corner, sides, side, axis, color, center, ray, result.

**Where the submitted source differs.** The submitted order is volume_index, scenario, matrix, volume, ..., corner, center, ray, result, sides, side, axis, color. It contradicts the /Od order in two places:
1. matrix is declared in the volume-loop block, after the if-block scalars.
2. center, ray and result are declared before the edge-loop locals.

**The worker's variants do not cover this.** None of them (od_order, od_order_vfirst, od_inner_only, od_outer_only, bareblock, color_*, axis_first) puts matrix before volume_index; bareblock still keeps matrix in the loop block. The MANIFEST claim that the full /Od order was measured is therefore inaccurate.

**Rule and precedent.** Brief section 9 and the wave-2 light_volumes rejection (research/fifty_objects_20260925/results/wave2/REVIEW__source_objects_widgets_light_volumes.md) both reject a grouping that contradicts the /Od declaration order when an equally exact, /Od-consistent grouping exists.

**The amended source (`var/full_od_mine.c`):**
- `real_matrix4x3 matrix;` is declared first in the if-block, before `short volume_index;`.
- center, ray and result get their own block after the edge loop:

  ```
  { real_point3d center; real_vector3d ray; struct collision_result result; ... }
  ```

  The block holds the point_from_line3d(0.5) center, the ray, the collision test and the string pair.

**Why the new block is legitimate:**
- **Escape law.** January's relocations show `_collision_test_vector` and `_render_debug_string_at_point` as REL32 out-of-line calls, so the addresses of all three locals reach out-of-line calls.
- **C89.** C89 cannot express the /Od order without a block.
- **Byte-inert.** The block does not buy the match.
- **HCEX_Release.pdb corroborates it.** Its records are matrix, local_extent, world_extent, sides[2], result, ray: matrix comes first and ray/result come after sides. HCEX.pdb has no record for this function.

**What is unchanged.** volume stays in the loop with its initialiser (rule 17). The /Od `volume = NULL` is a dead store with a one-line assert-anchor drift (0x214 against January's 0x213), so it is not reproduced. Placing volume in the loop does not violate the declaration ORDER.

**Fallback.** If the owner rejects the block, use `amended/production_matrix_only.patch` (and its `_crlf` variant). It is also EXACT and gives the identical object. It fixes the matrix contradiction, which needs no new structure.

## Independently re-run (all clean except F1)

- **Patches.** `git apply --check` is OK at HEAD for production(_crlf).patch, park_retire(_crlf).patch, the storage patch and my 4 amended/fallback patches.
  - GNU patch (LF) and `git apply --directory` (CRLF copies) both reproduce the worker's cand.c and compose file byte for byte.
  - Either patch order gives an identical result (AB == BA), with no mixed line endings.
- **Gate (--all).** base 64/2/0; worker alone 65/1/0; worker composed with storage 65/1/0; amended alone and amended composed 65/1/0. The only residual in each is `_hs_evaluate_begin_random` [sha].
- **Per-section sweep over 127 January-owned sections.** gain 1 (`_render_debug_trigger_volumes`), regression 0, changed 1. My base.obj equals build/base production (changed 0).
- **object_audit.**
  - Worker alone: FAIL(13), made up of begin_random plus the 12 pre-existing converter storage rows.
  - Composed (worker or amended): FAIL(1), begin_random only. All 129 January symbols match, and the 61 data sections are ok.
  - The only new surplus is +.text `_point_from_line3d` (48 B). No 1.0/-1.0 literal is emitted: the calls with t = +-1 fold.
- **Storage against PDB publics.** pdb_storage logic on the candidate objects: base/worker have 12 disagreements (the converters); composed/amended have 0. `_point_from_line3d` is public in cachebeta (publics line 6312) and is emitted as an external SELECT_ANY COMDAT.
- **Surplus identity.** 5 candidate-only code COMDATs, 0 different. `_point_from_line3d` is 48/0/9b763841f8519177 == January's action_charge copy.
- **Provider link.** `--baseline` gives `_point_from_line3d` vs action_charge PASS in both orders. The full run on the composed and amended objects PASSes. On the worker object alone the full run FAILs only the 12 converter rows (no January provider), which the storage patch fixes.
- **nodup_census.** 24 definers, 0 NODUP.
- **Ruling-5 class (house rules, rule-6 exception).** The helper is a genuine real_math.h `__inline`, byte-identical, with a strict-exact caller. It is not hand-expanded; production's hand expansion is removed. There is no header or config change.
- **/Od attestation of every changed construct.**
  - Arm 1 has 4 calls to 0x42e0d0 (corner[0] = pos + world_extent*1.0, then the 3-call tail); arm 2 copies corner[0] and then makes the same 3 calls. The center call adds 1 more. Every argument push matches.
  - sides[2] is zero-initialised (24 B) per edge iteration, and side is computed before axis.
  - In the else block, color is copied from blue and its alpha set to 0.15.
  - The string calls are an if/else pair (yellow 0xa1c244 / white 0xa1c22c).
- **Strip tests (lab).**
  - Ternary string call: RESIDUAL.
  - Tail of 3 point_from_line3d calls hoisted below the if/else: RESIDUAL.
  - Both duplicated constructs are therefore load-bearing and /Od-attested, not decoration.
- **Names.** matrix, local_extent, world_extent, sides, result and ray are attested by HCEX_Release; corner, center and color come from the /Od RTC. The scalars keep production's descriptive names.
- **Warnings.** `/Zs /W3` gives the same warning set as HEAD. A sanity lab with an unused local proves C4101 is detected.
  - `gate.py --cflag /W3 /WX` is NOT a valid warning test here: the sanity lab still compiled under it.
- **Data.** tinfo --data shows base and amended identical except for .debug$S. There is no data change.
- **park_retire.patch.** Entries go from 86 to 85; only {hs_runtime, _render_debug_trigger_volumes} is removed, and the JSON is valid.
- **Independent convergence.** My amended source is textually identical to the amended file produced by the parallel function-level reviewer (review_r2_hs_runtime__render_debug_trigger_volumes).

## Integrator steps

1. Apply `amended/production_amended.patch` (LF via git apply; `_crlf` for the working tree) together with `park_retire.patch`. A moved parked body fails PROGRESS. The storage patch composes in either order.
2. Run the full ninja plus the stable sweep. Ruling 5 requires the full ownership/build sweep with zero regressions.
3. math/real_math is RESERVED for the Codex cross_product2d packet. After it lands, re-gate hs_runtime (declaration-count effects) and re-run surplus identity and provider_link for `_point_from_line3d`.
4. hs_runtime stays NonMatching because of the retired begin_random tie. Credit: 1 function, 1,088 padded bytes, strict. The COMDAT surplus gets zero credit.
