# REVIEW source/hs/hs_runtime::_render_debug_trigger_volumes (1088 B)

approve: False

amended_patch: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_r2_hs_runtime__render_debug_trigger_volumes\amended\production_amended.patch

## per_object
[
 {
  "unit": "source/hs/hs_runtime",
  "approve": false,
  "issues": "Function-level landing: reject production.patch as submitted and land amended/production_amended.patch (or production_amended_crlf.patch) with the worker's park_retire.patch. Reason: the submitted grouping declares matrix after volume_index/scenario and center/ray/result before the edge-loop locals, which contradicts the /Od declaration order. The amended order is matrix, volume_index, scenario, volume, local_extent, world_extent, edge_index, corner, sides, side, axis, color, center, ray, result, with an escape-law block for center/ray/result. It is EXACT and gives an identical object. Fallback without the block: fallback/production_matrix_only.patch, also identical. Object admission stays blocked by the _hs_evaluate_begin_random tie; storage is handled by the separate converter patch, which composes cleanly (object_audit FAIL(1), begin_random only)."
 }
]

## checks
I re-ran every check independently. Evidence is in C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_r2_hs_runtime__render_debug_trigger_volumes\ (REVIEW.md, LEDGER.md).
(1) Patches:
- `git apply --check` is clean at worktree HEAD b7c1b7d9 (source/config/tools equal to f6d00a8c) for production.patch, production_crlf.patch, park_retire.patch and park_retire_crlf.patch.
- GNU patch on the f6d00a8c blob reproduces cand.c byte for byte.
- The CRLF patch needs `patch --binary` under GNU patch; git apply is unaffected.
(2) gate.py --all on the applied file: 65 exact / 1 residual (_hs_evaluate_begin_random [sha]) / 0 unwritten. _render_debug_trigger_volumes is EXACT at 1088. The unmodified baseline gives 64/2/0. My baseline object equals build/base, and my candidate object equals the worker's cand.obj except for .debug$S.
(3) object_audit: FAIL(13), all pre-existing (begin_random plus the 12 converter storage rows); the baseline is FAIL(14). The only new surplus is +.text _point_from_line3d, 48 B.
(4) Surplus: _point_from_line3d is new and IDENTICAL to January's selected copy in action_charge.obj. The other 4 surplus COMDATs are IDENTICAL too.
- provider_link --baseline: PASS in both orders.
- nodup_census: 24 definers, 0 NODUP.
- COMDAT selection is ANY with external storage.
- cachebeta publics line 6312 lists _point_from_line3d, so the storage agrees.
(5) Owner ruling 5 (all-inlined header-inline class) and the exception at docs/campaign_house_rules.md:33-40: all five conditions hold. The full build sweep is left to the integrator. The opt-in --forbid-emitted-symbol guard flags the COMDAT, as expected; no test or build step enforces it for hs_runtime.
(6) /Od re-read (od_linear 0x606c80..0x607462, RTC descriptor 0x607464):
- Both side arms call point_from_line3d (0x42e0d0) three times.
- The constants are 1.0 / -1.0 / 0.5.
- sides={0} zero-inits 24 B per edge iteration.
- side is computed before axis.
- color is copied, then its alpha is set.
- The two render_debug_string_at_point calls are under an if/else.
- /Od frame order: matrix, volume_index, scenario, volume, local_extent, world_extent, edge_index, corner, sides, side, axis, color, center, ray, result.
(7) Strip tests:
- single_tail re-gated: RESIDUAL (the duplicated tail is load-bearing).
- New rv_ternary (the if/else string pair back to production's ternary): RESIDUAL, 25 alndiff hunks. The if/else is both load-bearing and /Od-attested.
(8) HCEX_Release.pdb, raw CodeView scan:
- S_REGREL32 records are matrix, local_extent, world_extent, sides, result, ray, with no S_BLOCK32.
- S_COMPILE2 shows language C on VC16.
(9) Reviewer probes, each a full-TU gate:
- rv_matrix_outer: EXACT.
- rv_full_od: EXACT.
- rv_volume_outer: EXACT.
- The objects from rv_matrix_outer and rv_full_od are identical to cand.obj in every non-debug section, relocation and symbol.
(10) The amended file (CRLF) gates 65/1/0 EXACT, and /W3 /WX is clean.
(11) Composition with hs_runtime_converter_storage.patch:
- Result is the same in either order.
- Gate 65/1/0.
- object_audit FAIL(1), begin_random only.
- provider_link PASS on all surplus.
- The composed object is identical to the worker's compose.obj.
(12) park_retire applied to a copy: parked entries go from 86 to 85. Only {hs_runtime, _render_debug_trigger_volumes} is removed, everything else is unchanged, and the JSON is valid.
(13) No header, symbols.json or config status change. The tracked tree is still clean.

## issues
Rejected as submitted because the block grouping contradicts the /Od declaration order. It is fixable, and the fix gives an identical object. This is the same finding class as the wave-2 light_volumes rejection, and brief section 9 records the same rule.

F1: in the /Od frame, `matrix` is at -0x40, the first user slot, above volume_index -0x48, scenario -0x4c and volume -0x50. The submitted patch declares matrix first in the volume-loop block, which is textually after the outer `short volume_index; struct scenario *scenario`. It also declares center, ray and result at the top of the loop block, before the edge-loop locals sides/side/axis/color; /Od places them after color.
- None of the worker's 7 order variants puts matrix before volume_index, so the MANIFEST claim that the full /Od frame order was measured is inaccurate.
- Reviewer variant rv_full_od moves `real_matrix4x3 matrix;` to the top of the if-block and wraps the tail in `{ real_point3d center; real_vector3d ray; struct collision_result result; ... }` after the edge loop. It follows the complete /Od order and is EXACT (65/1/0), giving an object identical to cand.obj.
- The block is justified by the escape law: all three addresses reach out-of-line calls (collision_test_vector and render_debug_string_at_point). It is byte-inert, so it passes the strip test.
- HCEX_Release.pdb corroborates it: a C-language VC16 compiland (C89 declarations) with records matrix, local_extent, world_extent, sides, result, ray. That order is not first-use order, and it puts ray/result after the per-edge `sides`.
- Fallback if the owner judges the block invented: fallback/production_matrix_only.patch (rv_matrix_outer) is also EXACT with an identical object. It fixes only the matrix contradiction.

Everything else checks out and needs no change:
- exactness, 34 relocations, sha 5789a872;
- the /Od-attested duplicated arm tail and if/else string pair, both load-bearing under strip tests;
- HCEX/RTC names;
- the ruling-5 _point_from_line3d COMDAT: IDENTICAL, provider link PASS in both orders, 0 NODUP, PDB-public external;
- zero regressions in the unit;
- park retirement;
- composition with the storage patch.

Minor inaccuracy: the worker's claim that GNU patch reproduces the file from the CRLF working tree only holds with `patch --binary`. git apply works fine.

APPLY INSTEAD of production.patch:
- LF: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_r2_hs_runtime__render_debug_trigger_volumes\amended\production_amended.patch
- CRLF variant: amended\production_amended_crlf.patch

Apply it together with the worker's park_retire.patch (approved unchanged). Then run a full ninja and the stable sweep, and re-run surplus/provider_link after the Codex real_math packet lands. hs_runtime stays NonMatching because of the retired begin_random tie.
