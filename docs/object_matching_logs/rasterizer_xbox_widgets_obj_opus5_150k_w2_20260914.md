# `rasterizer_xbox_widgets.obj`: opus5 150K lane, wave w2 (2026-09-14)

## Baseline and outcome

Unit `source/rasterizer/xbox/rasterizer_xbox_widgets`, lane base `12f7375d4`.
Baseline gate (with the `_point_from_line3d` guard): **22 exact / 3 residual / 0 unwritten**.
Parks: `__rasterizer_widget_begin` (1,152) and `__rasterizer_widget_draw_sprite2d` (528), drift 0.

Final real-file gate: **23 exact / 2 residual / 0 unwritten**, guard passed, park drift 0,
no candidate-only owner versus `build/base`, fake-match scan 0 leads.

| Function | Padded | Relocs | Result |
| --- | ---: | ---: | --- |
| `__rasterizer_widget_submit_occlusion_test` | 640 | 36 | **EXACT, landed** (sha `7176593c07173a51`) |
| `__rasterizer_widget_begin` | 1,152 | 87 | parked; exact scratch candidate, park-reopen proposed |
| `__rasterizer_widget_draw_sprite2d` | 528 | 30 | parked; exact scratch candidate, owner ruling requested |

The 22 baseline exact rows are unchanged row by row. Only the orchestrator's full build counts.

## `__rasterizer_widget_submit_occlusion_test` (landed)

The previous body used three `return` statements and called `error()` inside the failure branch. The
x87 clamp/floor sequences already matched. The remaining gap was the register home of `pixel_count`
(stack slot versus January's `EDI`), an eager `xor esi,esi`, and a merged `add esp,0x10` where January
cleans the two diagnostic calls separately.

January evidence:

- At +0x1a3..+0x1b5 January emits `test edi,edi; jge; [pop edi; xor eax,eax; pop ebx; ret]; jle ret_edi`.
  Projection failure jumps into the same `xor eax,eax` block. That is one result variable whose
  constant-0 inputs come from the projection failure and from a `< 0 -> 0` clamp, followed by the
  `> 0` draw test, with no zero materialised before the projection call.
- At +0x24f/+0x25e the `rasterizer_error` and `error` cleanups are separate. This is the only such
  pair in the rasterizer objects (scans in `scratch/workers/rasterizer_xbox_widgets/scan_err.py` and
  `scan_sep.py`), so `error()` sits after a different join than the failure branch.
- `rasterizer_xbox.c` `_rasterizer_frame_end` and `_rasterizer_present` (both exact) use the house tail:
  `boolean success = TRUE;` at function scope, a failure branch that sets `FALSE` and calls
  `rasterizer_error` with the stringified call, then `if (!success) error(_error_silent, "### ERROR <fn> failed");`
  immediately before the single return.
- HCEA's later-build decompilation clamps `rectangle_area < 0 ? 0 : rectangle_area`. This corroborates
  the semantics only.

Landed source: single exit with `pixel_count = MAX(0, (x1 - x0) * (y1 - y0))`, `pixel_count = 0` in the
projection-failure `else`, `pixel_count = 1` when occlusion is disabled, and the function-scope
`success` flag checked after the whole if/else chain. Behaviour is unchanged for every input: disabled
returns 1, failed projection or a negative or zero area returns 0, and a positive area draws and
returns the area.

Shapes (scratch only): (o1) single exit with an in-block success idiom kept the stack home; (o2) a zero
in the projection `else` gave 624 bytes; (o3) the full S_OK check chain folded to o1 bytes; (o4) success
at enabled scope matched o1; (o5) the `MAX(0, area)` clamp was correct except for the merged cleanup.
Twelve lab tail spellings, compiled as extra functions in lab TUs, showed that only a function-scope
flag checked after the chain keeps the cleanups separate. Three inner branch spellings are
byte-identical: `if (result >= 0) {TRUE} else {...}`, the same with `&& success`, and `if (result < 0)`.
The non-redundant `if (result < 0)` form was landed.

Laws applied: the SINGLE-EXIT law and the `MIN`/`MAX` macro phi. New measured detail: an
`if (!success) error()` test inside the draw block is threaded and its call cleanups merge. The same
test at function scope, after the if/else chain, is a real join and keeps them separate.

## `__rasterizer_widget_begin` (park-reopen proposed)

Candidate: `scratch/workers/rasterizer_xbox_widgets/park_begin_row.c`. It is the current real file with
only the begin store order changed.

- The occlusion arm of the parked body is already byte-identical. Only the sprite arm differs: January
  completes all four viewport-derived x87 stores before `push 5`, while the zeros-first body
  interleaves them with the `SetVertexShaderConstant` argument pushes.
- The park's corroborating premise is refuted. The zeros-first order came from
  `halocea-review/.../dx9/_rasterizer_widget_begin.c`, which is Hex-Rays output: its header cites the
  Hex-Rays diagnostic "local variable allocation has failed", and 92 HCEA files carry it. Its statement
  order is therefore the emitted store order of a different compiler. January's own bytes place the 16
  constant stores first in both arms regardless of source order.
- Four probes: row-major fill in both arms is EXACT; dynamic-first in both arms is EXACT; row-major in
  the sprite arm only is EXACT, because the occlusion arm is insensitive; zeros-first with `[4][3]` last
  is residual. The current order is proven wrong by the sprite arm. The natural `[row][column]` fill
  reproduces both arms.
- Candidate gate: 24/1 with only this row changed; the sprite2d park does not drift; census clean;
  fake scan 0.
- History blobs `85a2d84926`/`bc6411cfe8` contain the same row-major order. Their raw pixel-shader
  numbers and shift extraction were not ported.

## `__rasterizer_widget_draw_sprite2d` (owner ruling)

Candidate: `scratch/workers/rasterizer_xbox_widgets/park_sprite2d_ternary.c`. It is the current real
file with only the texture ternary pair.

Facts established:

- In the same function, the axis and scale null paths store right-to-left, as `a = b = 1.0f` does. The
  texture null path assigns `u` (`EBX`) first.
- Rewriting the scale block as ternaries breaks both sprite2d and sprite3d. January's scale blocks
  really are if/else chains, and the ternary does not make any spelling match.
- No if/else texture spelling reproduces January. The current chain, `u = 1; v = 1;` and `v = u = 1`
  swap the u/v registers, and donor variants C/D/G/H/I also failed.

The ternary pair is the only tested spelling consistent with January. No law authenticates it, so the
park criterion is **not** independently met; this is recorded for an owner ruling.

## Rejected and do-not-repeat

- Occlusion: a folded S_OK check chain does not change the bytes; an in-block success flag merges the
  cleanups.
- Begin: the "zeros last except `[4][3]`" order does not match; do not cite HCEA decompilation order
  as source order.
- Sprite2d: if/else texture spellings (chain, separate statements, reversed chain) and scale-block
  ternaries.

## Reopen criteria

- Begin: orchestrator acceptance of the premise refutation and sensitivity evidence above.
- Sprite2d: an authenticated source or compiler law explaining the u-first register order, or an
  owner ruling on the ternary pair.
