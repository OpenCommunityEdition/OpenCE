# REVIEW source/hs/hs_runtime

approve: False

amended_patch: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_r2_hs_runtime\amended\production_amended.patch (LF, apply with git apply against the index). CRLF working-tree variant: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_r2_hs_runtime\amended\production_amended_crlf.patch. Land it with the worker's C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\hs_runtime__render_debug_trigger_volumes\park_retire.patch (unchanged). Fallback: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_r2_hs_runtime\amended\production_matrix_only.patch (plus its _crlf variant).

## per_object
[
 {
  "unit": "source/hs/hs_runtime",
  "approve": false,
  "issues": "F1: the submitted grouping contradicts the /Od declaration order. matrix is declared in the loop block after volume_index/scenario, and center/ray/result are declared before sides/side/axis/color. Fixed by amended/production_amended.patch: matrix first in the if-block, plus a { center; ray; result; } block after the edge loop, which the escape law permits. The amended patch gives an identical object and passes every check: gate 65/1/0; object_audit FAIL(1), begin_random only, when composed with the storage patch; surplus IDENTICAL; provider_link PASS both orders; 0 regressions. park_retire.patch is approved as is. The object stays blocked by the retired _hs_evaluate_begin_random tie."
 }
]

## checks
Environment: HEAD b7c1b7d9 (source, config and tools equal f6d00a8c); tree clean before and after the review. All evidence is in C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_r2_hs_runtime\ (LEDGER.md, REVIEW.md).

Patch application:
- git apply --check is OK at HEAD for production.patch, production_crlf.patch, park_retire.patch, park_retire_crlf.patch, the storage patch and my 4 amended/fallback patches.
- GNU patch on the LF HEAD blob and git apply --directory on CRLF copies both rebuild the worker's cand.c and compose_mine_first file byte for byte.
- The worker and storage patches give identical results in either order (AB==BA). No mixed line endings.

gate.py --all (only residual in every candidate is _hs_evaluate_begin_random [sha]):
- base: 64/2/0
- worker patch alone: 65/1/0
- worker + storage: 65/1/0
- amended alone and amended + storage: 65/1/0
- _render_debug_trigger_volumes is EXACT 1088 in all candidates.

Per-section sweep over the 127 January-owned sections:
- gain 1, regression 0, changed 1.
- My base.obj is identical to build/base production.

object_audit:
- worker alone: FAIL(13) = begin_random + 12 pre-existing converter storage rows.
- composed (worker or amended): FAIL(1), begin_random only; 129/129 symbols match; all 61 data sections ok.
- only new surplus: .text _point_from_line3d.

pdb_storage logic against cachebeta publics:
- composed: 0 disagreements.
- _point_from_line3d is public (line 6312) and is emitted as an external SELECT_ANY COMDAT.

surplus_identity logic on the candidate objects:
- 5/5 IDENTICAL.
- _point_from_line3d is 48/0/9b763841f8519177, equal to January's action_charge copy.

provider_link:
- --baseline: PASS in both orders.
- full run on the composed/amended objects: PASS.
- full run on the worker object alone fails only the 12 converter rows (no January provider).
- nodup_census: 24 definers, 0 NODUP.

tinfo --data: base and amended are identical apart from .debug$S. No data change.

/Zs /W3: the warning set equals HEAD's. A sanity lab with an unused local proves C4101 is detected; gate --cflag /W3 /WX turned out not to be a valid warning test.

park_retire: parked entries go from 86 to 85; only the trigger_volumes entry is removed; the JSON is valid.

/Od and PDB evidence:
- Re-read with od_linear 0x606c80..0x607470; parsed the RTC descriptor at 0x607464.
- Floats: 1.0, -1.0, 0.5, 0.15, 0.95.
- 8 point_from_line3d calls, and every argument push matches the candidate.
- HCEX_Release.pdb locals via DIA2Dump: matrix, local_extent, world_extent, sides[2], result, ray. HCEX.pdb has no record for this function.

Strip tests (lab):
- ternary string call: RESIDUAL
- single hoisted tail: RESIDUAL
- Both duplicated constructs are load-bearing and attested by /Od.

Other checks:
- Re-checked the placement of matrix in each of the worker's 8 order variants.
- January relocations confirm that collision_test_vector and render_debug_string_at_point are out-of-line calls (the escape law applies).
- begin_random retirement confirmed from its park and the w1/w3c ledgers.
- Claimed hs_runtime while building the amended patch, then released it.

## issues
F1 (blocking, fixable): the declaration grouping in production.patch contradicts the /Od declaration order.

What the /Od frame shows:
- The frame follows declaration order, not first-use order:
  - scenario is stored first but sits below volume_index;
  - matrix is used first late but holds the topmost slot, -0x40.
- /Od order: matrix, volume_index, scenario, volume, local_extent, world_extent, edge_index, corner, sides, side, axis, color, center, ray, result.

Where the submitted source differs:
1. It declares matrix in the volume-loop block, after `short volume_index; struct scenario *scenario`.
2. It declares center, ray and result before the edge-loop locals.

Why this blocks:
- None of the worker's 8 order variants puts matrix before volume_index; bareblock also keeps matrix in the loop. So the MANIFEST claim that the full /Od order was measured is inaccurate.
- Brief section 9 and the wave-2 light_volumes precedent reject a grouping that contradicts the /Od order when an equally exact, /Od-consistent grouping exists.

Amended patch:
- Declares `real_matrix4x3 matrix;` first in the if-block.
- Puts center, ray and result in their own block after the edge loop. The escape law permits this: their addresses reach the REL32 out-of-line calls _collision_test_vector and _render_debug_string_at_point. C89 needs the block, and the block is byte-inert. HCEX_Release corroborates it: matrix comes first there, and ray/result come after sides.
- Result: 65/1/0, and the object is identical to the worker's candidate (154/154 non-debug sections, 488/488 symbols, including $L numbering). Every exactness and admission result carries over.
- My amended source is textually identical to the parallel function reviewer's amended file.
- Fallback if the owner rejects the block: amended/production_matrix_only.patch (also exact, identical object).

Everything else verifies, so the verdict class FUNCTIONS_EXACT_OBJECT_BLOCKED is correct:
- The function is strictly exact.
- The object is blocked only by the retired begin_random tie. The 12 storage rows are fixed by the separate storage patch, which composes in either order.
- The ruling-5 COMDAT conditions hold at unit level.
- park_retire.patch is approved unchanged.

Integrator steps:
- Land the amended patch together with park_retire.patch.
- Run the full ninja build and the stable sweep; ruling 5 requires the full build sweep.
- After the reserved Codex real_math packet lands, re-gate hs_runtime and re-run surplus identity and provider_link.
- The object stays NonMatching. Credit: 1 function, 1,088 padded bytes. The COMDAT gets zero credit.
