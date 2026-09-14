# rasterizer_xbox_widgets.obj: Opus5 150K house-clean lane, wave w3 (2026-09-14)

Unit `source/rasterizer/xbox/rasterizer_xbox_widgets`. The real file is unchanged (`git diff --stat` empty).

## Baseline and final

| gate (guard) | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| baseline | 24 | 1 | 0 |
| final | 24 | 1 | 0 |

The only residual is parked: `__rasterizer_widget_draw_sprite2d` (528/30).

Final state of the unit:
- park drift 0
- owner census equal to base
- fake scan 0 leads

## `__rasterizer_widget_draw_sprite2d` — NOT LANDED

### Remaining difference

Exactly 2 instructions differ, at +0x12d, on the null-`texture_scale` path.

| Source | Instructions |
| --- | --- |
| January | `mov ebx,1; mov edi,ebx` (the u register receives the constant first) |
| Ours | `mov edi,1; mov ebx,edi` |

### Evidence

- Every other instruction matches.
- The functions use parameter-slot reuse for sin/cos and the products; this is already equal.
- The w2 verifier rejected the ternary-pair spelling as R4.

### New diagnostics (no candidates)

| Diagnostic | Change | Result |
| --- | --- | --- |
| wg_d1 | `texture_v` declared before `texture_u` | identical residual |
| wg_d2 | `short` u/v with `(short)` conversions | identical residual |

### Laws checked

- **A10** (conditional initializer): no stack home is involved.
- **A13**: does not apply.
- **x87 laws**: do not apply.
- **This wave's FOR-INCREMENT frequency law**: does not apply (the function has no loops).

No law predicts the order in which the constant phi is materialised, so the park criterion remains unmet.

### Reopen criterion

Unchanged: an owner ruling on the ternary pair, or a measured law for phi constant materialisation order.

## Proposals

None.
