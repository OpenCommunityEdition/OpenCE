# `real_math.obj` — opus5 150K house-clean lane, wave w1 (2026-09-14)

## Result

No function landed. `source/math/real_math.c` is unchanged (`git diff --stat` empty). The final real-file gate
is identical to the baseline, row for row:
**exact 80, residual 4, unwritten 0**.

The two structural residuals are now reduced, in scratch, to one and the same one-instruction-pair x87 load-order
tie. The two representation-type residuals have sharper owner-ruling evidence.

| Function | Target padded/relocs | Baseline candidate | Best scratch candidate | Status |
| --- | --- | --- | --- | --- |
| `_sphere_intersects_triangle3d` | 608 / 6 | 624 / 6 | 608 / 6, one 2-insn operand-order diff (`bodies/sphere_s2.c`) | NOT-LANDED |
| `_pill_intersects_triangle3d` | 832 / 12 | 848 / 12 | 832 / 12, one 2-insn operand-order diff (`bodies/pilltri_p3.c`) | NOT-LANDED |
| `_point_in_triangle3d` | 432 / 9 | 432 / 9 (20 slot bytes) | exact only with point/vector casts | OWNER-BLOCKED |
| `_pill_test_vector3d` | 448 / 9 | 464 / 9 | exact only with a vector3d->vector2d cast | OWNER-BLOCKED |

All scratch paths below are relative to `scratch/workers/real_math/`. The worker notes are in `scratch/workers/real_math.md`.

## Baseline and guards

- **Emitted-symbol guard.** The `_point_from_line3d` guard fails at baseline, and the failure is pre-existing. The candidate object emits a 48-byte `_point_from_line3d` COMDAT, together with about 30 other surplus math-helper COMDATs, all present in the unchanged file. Because the real file is untouched, no new candidate-only owner exists. The combined scratch candidate `combined_s2_p3.c` also has zero new or lost owners compared with the baseline object.
- **Parks.** The unit has none.
- **Fake-match scan.** It reports 0 leads, both on the real file and on the combined candidate.
- **Siblings.** The combined candidate leaves every sibling row unchanged.

## Ledgers read / do-not-repeat honoured

- `real_math_obj_jonas_safe_owner_restoration_20260826.md`
- `real_math_obj_jonas_global_typed_boundary_20260830.md`
- `real_math_obj_jonas_helper_restoration_exact_20260831.md`
- `real_math_projection_current_context_recheck_20260904.md`
- `legacy_small_donor_exact_reaudit_20260913.md`
- the opus5 100K handoff

The following were not repeated: the `outside[4]`/barrier form, forced-inline third-edge helpers, the direct scalar third-edge control, point/vector pointer casts in production, and a declaration-order sweep of `point_in_triangle3d`.

## `_sphere_intersects_triangle3d`

### Evidence

- **Third edge.** In the third edge, January computes the center offset `center - triangle2` on the x87 stack before `edge20` (baseline alndiff delete block @0x1c1).
- **Cross-result slots.** The second-edge cross result occupies center_offset's slots (-0x30/-0x2c). The third-edge cross result occupies edge12's slots (-0x24/-0x20). Neither ever uses edge01's slots. Each edge therefore has a distinct `cross` frame object.

### Shapes

1. **S1: move the center-offset line before `edge20` in the third edge.** This gives 608/6. The size is fixed, but slot and load-order diffs remain.
2. **S2: S1 plus a block-scoped `real_vector3d cross` per edge.** `edge20` is scoped to the third-edge block. This gives 608/6, and the only remaining difference is at 0xdf: January emits `fld [ebp-0x28]; fmul [ebp-0x14]`, while ours emits the operands swapped. This is the i-component second product `a->k*b->j` of the inline `cross_product3d(&center_offset, &edge01, &cross)`.
3. **Declaration order (S3), `boolean result = TRUE;` (S11) and the dot argument order (S8/S9)** were all inert.
4. **Edges-before-offset statement order (S4)** was worse, because the store order proves the offset comes first.
5. **Per-edge center offset variables (S5)** were worse; reusing `center_offset` matches January.
6. **`triple_product3d` (S6/S7)** was worse: the frame shrinks to 0x24.
7. **Moving `result = TRUE` after the plane test (S10)** was worse.
8. **Nesting the cross in the dot, or swapping the cross arguments with the opposite compare (sn1/sn2/sr1)** was inert or worse.

## `_pill_intersects_triangle3d`

### Evidence

- **`outside` storage.** January stores `outside` in its own frame slot `[ebp-1]`, which gives a 0x40 frame, and homes `t` in triangle0's parameter slot. The baseline instead placed `outside` in a parameter home.
- **Third-edge miss path.** January falls through to the shared return-TRUE block after a `je return_false`; the baseline emitted `setne`. The third-edge miss path therefore does not return directly.

### Shapes

1. **P1: `boolean outside = FALSE;`** (declaration plus initialization, house rule 17). This gives a 0x40 frame, and every slot and home becomes identical to January.
2. **P2: P1 plus `outside = TRUE;` on the third-edge miss**, the same as edges 1 and 2. This gives 832/12, and the only remaining difference is at 0x64: January emits `fld [ebp-0x14]; fmul [ebp-0x24]`. This is the same i-component `a->k*b->j` pattern, here in `cross_product3d(&edge01, &edge12, &normal)`.
3. **P3: P2 with the `goto return_true` label replaced by a plain `return TRUE;`.** The result is identical to P2, so P3 is the clean body to keep.

## The shared load-order tie (lab, `lab/`)

The same January choice appears in both functions. For an all-memory inline `cross_product3d`, January loads `a->k` before `b->j` in the i-component. VC7 as we drive it loads `b->j`.

**Inert:**

- inline `cross_product3d` text order (k/j/i, i/j/k, direct stores, swapped products)
- 1–8 extra TU prototypes
- 1–16 extra struct declarations
- 1–8 extra file statics
- local variable names
- the plane-test and dot spellings
- code after edge 1

**Not a lever:** a hand-expanded cross with direct field access does flip this product. It also perturbs about ten other x87 schedules (624 bytes), and it is manual inlining anyway.

**Conclusion:** we have no legal source control. Per the brief, the ≤5-shape budget is spent.

## `_point_in_triangle3d` (owner-blocked; new bisection evidence)

- **pf1: Fable `21f4fc550` body.** It uses vector helpers, casts to `real_point3d const *` for projection, and calls `cross_product2d` on `(real_vector2d const *)` projected points. It is EXACT; this is the control.
- **pf2: pf1 with the 2D crosses hand-written.** This returns to the baseline 20-byte slot residual.
- **pf3: the current typed body plus `cross_product2d` with casts.** The slots become exact, but the hand-written 3D normal changes two x87 load orders.

**Conclusion:** January used both `cross_product3d` on `real_vector3d` edges and `cross_product2d` on the projected `real_point2d` results. Any exact spelling therefore needs point/vector pointer conversions at the helper arguments.

## `_pill_test_vector3d` (owner-blocked; no new shapes)

January writes normal->i/j through the output pointer and calls `_fast_normalize2d` on it. The only exact form is `fast_normalize2d((real_vector2d *)normal)`, which the 20260830 typed-boundary audit already closed.

## Orchestrator proposals

1. **Owner ruling (no header text).** Should point2d/vector2d and point3d/vector3d pointer conversions at math-helper call sites be admissible when January's bytes prove the helper call?
   - Evidence: the pf1/pf2/pf3 bisection above, plus the pill_test_vector3d alndiff.
   - Consumer: `real_math.c`.
   - Unlocks: 880 padded bytes (432 + 448).

## Reopen criteria

- **sphere / pill-triangle.** Reopen with a measured, legal control of the i-component `a->k*b->j` load order in an all-memory inline `cross_product3d`, for example a law derived from another consumer. Start from `bodies/sphere_s2.c` and `bodies/pilltri_p3.c` (combined full file `combined_s2_p3.c`, which has zero sibling or owner drift). Both functions (1,440 padded bytes) should close together.
- **point_in_triangle3d / pill_test_vector3d.** Reopen on the owner ruling above.
