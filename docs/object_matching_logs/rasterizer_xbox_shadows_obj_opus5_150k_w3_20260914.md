# rasterizer_xbox_shadows.obj: Opus5 150K house-clean lane, wave w3 (2026-09-14)

Unit `source/rasterizer/xbox/rasterizer_xbox_shadows`. The real file is unchanged, and `git diff --stat` is empty.

## Baseline and final

| gate (guard) | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| baseline | 16 | 2 | 0 |
| final | 16 | 2 | 0 |

At baseline, `__rasterizer_environment_shadow_draw` is already exact; wave w2 reopened it. Both residuals are parked:
- `__rasterizer_environment_shadow_begin`: 944/73 [sha]
- `__rasterizer_environment_shadow_model_draw`: 688/48 [sha]

Final checks:
- Park drift is 0.
- The owner census matches base.
- The fake scan reports 0 leads.

## `__rasterizer_environment_shadow_begin`: SKIPPED-EXHAUSTED (no new law)

**Re-read against the January stream:**
- Five constant stores (row 2 and `row3[0]`) sink before the x87 block.
- Each row scales `forward` and `left` by `inverse_radius` through `fld st(0); fmul`.
- The negated plane distance is emitted with k,j,i term order and the position component loaded first in all six terms, then `fmul st(1); fchs`.

**Best w2 scratch bodies**, each leaving one 2-instruction transposition:

| body | argument order | transposed term |
| --- | --- | --- |
| `b2.c` | `dot_product3d(vector, position)` | row1 x term |
| `p_pv.c` | `scale_vector3d` + `dot_product3d(position, vector)` | row0 x term |

**Laws applied as detectors:**
- A22 dot canonicalisation: satisfied.
- A22 younger operand: it cannot explain both rows. The mismatching term moves with value-number order, not with a spelling.
- A4 helper routing: already applied.
- R3 forbids choosing operand order to steer.

w2 spent 5 shapes plus diagnostics, so no new shape was compiled. The reopen criterion is unchanged (w2 ledger).

## `__rasterizer_environment_shadow_model_draw`: SKIPPED-EXHAUSTED (no new law)

**Remaining difference** (w2 `m1.c`): January loads `detail_map_scale` into EAX before `fld detail_map_v_scale; fmul detail_map_scale`; ours does the `fld` first.

**Laws checked:**
- A20 store order: the stores already match.
- The x87 operand-order law: w2 found operand order inert (m8).
- A11 cast: not applicable.

No law predicts an integer-load-before-x87 placement, so no new shape was compiled. The reopen criterion is unchanged.

## Proposals

None.
