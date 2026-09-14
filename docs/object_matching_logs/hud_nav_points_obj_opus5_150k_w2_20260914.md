# `hud_nav_points.obj` park re-investigation, Opus5 150K lane wave 2 (2026-09-14)

## Outcome

Nothing landed. `source/interface/hud_nav_points.c` is unchanged (`git diff --stat` empty). Both parks stay parked.

| function | target | real file (baseline = final) | status |
| --- | --- | --- | --- |
| `_hud_render_nav_points` | 400 / 17 / `f2bb689a08030b4b` | 416 / 17 / `0df4703281637b18` | NOT-LANDED, still parked |
| `_custom_render_nav_point` | 1632 / 95 / `287a20cd33864896` | 1632 / 95 / `ad77c226edfd32a6` | NOT-LANDED, still parked |

Final real-file gate:
- 30 exact, 2 residual, 0 unwritten;
- rows identical to baseline;
- `_point_from_line3d` guard passes;
- `scratch/parkcheck.py`: 2 parks, 0 drift;
- `tools/fake_match_scan.py`: 0 leads;
- no owner change, because the file is unchanged.

Duplicate prevention covered:
- the four earlier object ledgers, the `parked.json` entries and the 100K handoff;
- `git log --all`: 33 commits, none with an exact body;
- `branch_sweep`: 24 blobs, and the best is the current file (30/2/0).

## `_hud_render_nav_points`

The target and the real file differ in only one place: the loop cursor. January uses `lea esi,[eax+8]` (cursor at `reference_index`), with EBX as the down-counter and ESI holding -1 before the loop. The real file's pointer do-while keeps ESI at the datum base. The instruction lengths that follow then need a 16-byte alignment jump.

Candidate shapes (scratch only, all reverted):

| shape | result |
| --- | --- |
| r1: short-index `for` with `nav_point = &datum->nav_points[i]` | Gets the +8 cursor, but -1 is pinned in EBX for the whole function, the counter is spilled to `[ebp+8]` and the frame is 0x1c. 416 bytes. This repeats the prior-lane family. |
| r2: long index | same as r1 |
| r3: index do-while | same as r1 |
| r5: if/else instead of `continue` | same as r1 |
| r6/r7: direct `datum->nav_points[i].field` | same as r1 |
| r11/r12: outer-scope pointer; position/radius/goal hoisted (scoping lever) | same as r1 |
| r8: end-pointer `for` | +8 bias and in-loop -1 immediates, but the trip count is not folded (guard plus 0xaaaaaaab division). 432 bytes. |
| r9/r10: Marathon dual-IV `for (i = 0, p = base; ...; i++, p++)` | identical to the real file (base cursor) |

New mechanism evidence (diagnostics, not admissible source):
- **p1.** Take r1 and delete only the default-case `match_assert` (its `system_exit(-1)` push). The allocation becomes exactly January's: ESI holds -1 before the loop and then the +8 cursor, EBX is the counter, and the in-loop -1s are immediates.
  - So in index form the whole residual is the weight of the -1 constant web against the LFTR down-counter. January's counter outranked -1 while the assert was present.
  - The bitfield `type==NONE` test and `type = NONE` store do not contribute (p7, p8).
- **p3–p5.** VC7 moves a user pointer IV to +8 only when the pointer is never dereferenced at +0. Removing both `nav_index` reads moves the cursor to +8; removing either one alone does not.

Reopen criterion: a natural typed traversal in which the loop counter outranks the -1 constant web with the default assert present. Or, January source for the default arm.

## `_custom_render_nav_point`

The residual is still the 8 bytes at +0x58A. January pushes the constant `override_scale` 0.0f before loading the inline `fast_ftol` result and `distance`. VC7 loads both first.

Candidates, all reverted:
- s1: decimal expression entirely in the argument list;
- s3: block-local `decimal_modulus`;
- s4: HCEA-style separate meters local (also permutes stack slots);
- s5: named `real decimal_part` with `(short)fast_ftol(decimal_part)` as the argument.

Diagnostics d1–d12 changed the argument kinds:
- float constant 1.0f;
- other int constants, `NONE`/`FALSE`;
- a constant decimal;
- `fast_ftol_C` for the decimal;
- a parenthesised whole argument (PAREN law);
- a real variable override;
- an x87 `(real)short` override.

None of these puts a constant push ahead of the loads. The x87 variant interleaves the loads inside its own push sequence.

A scan of every January split object (`scratch/workers/hud_nav_points/fistp_scan.py`, `pushload_scan.py`) found:
- `push imm; mov r32,[inline-ftol result]` occurs nowhere else in January;
- the only other push-constant / load / push-constant / push-register instance is a cross-jumped tail in `_TIFFReadDirectory`.

There is no same-compiler donor. The reopen criterion is unchanged from the park.

## Proposals

None: no header, config or park-reopen proposal.
