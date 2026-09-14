# `hud_nav_points.obj`, Opus5 150K lane wave 3 (2026-09-14)

## Outcome

Nothing landed. `source/interface/hud_nav_points.c` is unchanged (`git diff --stat` empty). Both parks stay parked.

| function | target | real file (baseline = final) | status |
| --- | --- | --- | --- |
| `_hud_render_nav_points` | 400 / 17 / `f2bb689a08030b4b` | 416 / 17 / `0df4703281637b18` | NOT-LANDED, parked |
| `_custom_render_nav_point` | 1632 / 95 / `287a20cd33864896` | 1632 / 95 / `ad77c226edfd32a6` | NOT-LANDED, parked |

Final real-file gate:

- 30 exact, 2 residual, 0 unwritten, with rows identical to the baseline;
- the `_point_from_line3d` guard passes;
- `scratch/parkcheck.py` reports 2 parks and 0 drift;
- `tools/fake_match_scan.py` reports 0 leads;
- the owner census is unchanged.

Duplicate prevention covered:

- the four pre-lane ledgers and `hud_nav_points_obj_opus5_150k_w2_20260914.md`;
- `scratch/workers/hud_nav_points.md`: w2 shapes r1-r12, p1-p8, s1-s5 and d1-d12, plus the January-wide push/load scans;
- `config/parked.json`, `scratch/w2/laws_w2.md` and the 150K handoff.

Worker notes: `scratch/workers/w3_hud_nav_points_hud_etc.md`.

## C4013 lead

The only `/W3` C4013 in the TU is `error`. It is called at line 350 (`find_nav_point`) and line 410 (`hud_activate_nav_point`). Both callers are already EXACT and both pass int constants. Neither park calls `error`, so law A7 cannot move the parks.

I did not add `#include "cseries/errors.h"`. It would gain nothing and would add declaration-count risk to 30 exact rows.

## `_hud_render_nav_points`

Detector pass over laws_w2: in the natural index form, every in-loop instruction equals January's except the register that holds -1.

- A3, A10, A14 and A17 do not apply.
- A5 was already covered by w2 r6/r7.
- The A6 frame gap (0x1c against 0x18) is a consequence of the counter spill.
- A9: the bitfield switch lowering is already identical.
- A16: the operand order is already identical.

The exact sibling in the same TU, `hud_update_nav_point_local_player`, uses a short-index `for` with `nav_point = &datum->nav_points[i]` and `continue`. That makes the index form the natural spelling here. Its residual is the w2 p1 finding: the -1 constant web takes EBX for the whole function, and the LFTR loop counter spills to the stack.

Shapes (scratch `scratch/workers/w3_hud_nav_points_hud_etc/n1..n5.c`, all reverted):

| shape | result |
| --- | --- |
| n1: index `for` in the sibling's idiom | 416/17; the only differences are registers (29 blocks) |
| n2: n1 plus a named `struct hud_nav_object_datum *object` local (sibling idiom) | identical to n1 |
| n3: n1 with a `NUMBEROF(datum->nav_points)` bound | identical |
| n4: n1 with the case source order reversed | identical: case order is inert in this switch |
| n5: n1 with `default` first | identical |

Reopen criterion (unchanged): either a natural traversal in which the loop counter outranks the -1 web with the default assert still present, or January source for the default arm. Case order, default position, bound type and the named object local are now also measured inert.

## `_custom_render_nav_point`

The residual is still one displaced `push 0` (override_scale) at +0x58A: January schedules it before the loads of `decimal_value` and `distance`. I measured the only literal spellings w2 had not tried:

- c1: `0`, an int literal converted by the prototype — identical;
- c2: `0.0` — identical;
- c3: `0.f` — identical.

Reopen criterion: unchanged from the park.

## Proposals

None.
