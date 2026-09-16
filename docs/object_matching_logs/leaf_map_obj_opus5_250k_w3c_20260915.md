# `leaf_map.obj` Opus5 250K house-clean wave w3c (2026-09-15)

## Result

**No source change is admitted to `source/structures/leaf_map.c`** (`git diff --stat` empty).
The final real-file gate equals the baseline: **16 exact / 9 residual / 0 unwritten**, the
`_point_from_line3d` guard passes, all six parks match their recorded measurements (0 drift), the
owner census against `build/base` shows no new or lost owner, and the fake-match scan reports 0 leads.

This was a second-pass (w3) audit of a unit that wave w1 had already driven to its mechanical floor.
Every one of the nine non-exact rows already has a proven mechanism and a scratch body that
reproduces January. All nine are now terminally blocked, and both blockers are decisions taken
*after* w1, so no shape budget was spent:

1. Four rows (1,984 padded B) depend on the `real_math.h projection_from_vector3d` body correction,
   which the **2026-09-15 owner rulings declare FINAL and DECLINED** (packet P7 "stays reverted";
   +960 B here but -2,784 B on `_bitmap_copy` on top of P6). WORKER_BRIEF section 9b forbids
   re-proposing it *and* forbids submitting a body that depends on it.
2. Five rows (1,424 padded B) are the `csplit-relocation-alias` class: their code sections and
   normalized SHA-256 are already identical to January (objdiff 100%). Strict exact is unreachable
   from source (laws_w3 **A45**); they are parked and carry zero strict credit by construction.

## Baseline (gate at HEAD 1881e718b)

```
== emitted-symbol guard passed (1 forbidden names checked)
residual    160  _find_like_crossing                          [reloc-identity]
residual    368  _intersect_planes3d                          [size 400!=368, sha]
residual    176  _leaf_face_get_vertex3d                      [sha]
residual    608  _leaf_map_build_leaf_face_for_leaf_on_node   [reloc-identity]
residual    112  _leaf_map_build_leaf_faces_for_leaf          [reloc-identity]
residual    480  _leaf_map_build_portals_from_leaf            [reloc-identity]
residual    784  _leaf_map_get_leaf_bounds                    [sha]
residual    656  _leaf_map_leaf_spans_polygon                 [sha]
residual     64  _node_stack_read                             [reloc-identity]
== exact 16  residual 9  unwritten 0  (of 25 listed)
```

No UNWRITTEN rows, and no `_code_XXXXXXXX` owner remains (the 2026-09-09 private-graph
reconstruction named every private owner; `scratch/atlas/leaf_map.obj.txt` corroborates all 15
records). There is therefore no callee-first work and no EXACT-PENDING-RENAME item in this object.

Prior-art note: commit **6a473f30d** (after wave w1) already landed w1's `f7` body for
`_leaf_map_build_portals_from_leaf` and re-classified its park to `csplit-relocation-alias`. That
row is at target sha / objdiff 100% today; the w1 park-reopen proposal is consumed and must not be
re-claimed.

## Per-function outcomes

| function | padded | outcome | blocker |
| --- | ---: | --- | --- |
| `_leaf_face_get_vertex3d` | 176 | SKIPPED-EXHAUSTED | P7 header body (DECLINED) - sole diff |
| `_leaf_map_get_leaf_bounds` | 784 | SKIPPED-EXHAUSTED | P7 header body; tail already solved by w1 `b10` |
| `_leaf_map_leaf_spans_polygon` | 656 | SKIPPED-EXHAUSTED | P7 header body **and** `_point_from_line3d` COMDAT (OUT) |
| `_intersect_planes3d` (park) | 368 | SKIPPED-EXHAUSTED | P7 header body; cast half now admissible, header half is not |
| `_node_stack_read` (park) | 64 | SKIPPED-EXHAUSTED | csplit relocation alias (A45) |
| `_find_like_crossing` (park) | 160 | SKIPPED-EXHAUSTED | csplit relocation alias (A45) |
| `_leaf_map_build_leaf_face_for_leaf_on_node` (park) | 608 | SKIPPED-EXHAUSTED | csplit relocation alias (A45) |
| `_leaf_map_build_leaf_faces_for_leaf` (park) | 112 | SKIPPED-EXHAUSTED | csplit relocation alias (A45) |
| `_leaf_map_build_portals_from_leaf` (park) | 480 | SKIPPED-EXHAUSTED | csplit relocation alias (A45); body already at target sha |

## Evidence re-measured at HEAD

`alndiff` against `build/base/source/structures/leaf_map.obj` (not w1's shadow-header numbers):

- `_leaf_face_get_vertex3d`: 85 vs 85 instructions; the **only** differences are three compare pairs
  from the inlined `projection_from_vector3d` - target `test ah,1 / jne` at 0x3f, 0x48, 0x60 against
  our `test ah,5 / jnp|jp`.
- `_leaf_map_get_leaf_bounds`: 297 vs 297; the same three compares (0x187, 0x190, 0x1a8) plus the
  radius tail (January homes `center.z` in `[ebp+8]` and the first extent in `[ebp-0x18]`, and sums
  `(i*i + k*k) + j*j`). w1's `b10` - copy the maximum corner into a `real_point3d` and take
  `*radius = distance3d(center, &corner)` - closes the tail and is strict EXACT with the corrected
  header; under the current header it is row-neutral (16/9/0) and therefore not landable
  (brief section 6: an already-written residual may land only if it becomes strict EXACT).
- `_leaf_map_leaf_spans_polygon`: 256 vs 256; the same three compares (0x7f, 0x88, 0xa9) plus one
  3-instruction `fld [ebp-0x24]; fmul [esi+8]; faddp st(1)` group that January emits two
  instructions later (A35/A22 leaf age). w1's `s4` closes it with the natural inline
  `point_from_line3d`, which is OUT by lane rule; every hand expansion of that helper is R15.
- `_intersect_planes3d`: 141 vs 152; `sub esp,0x10` vs `sub esp,0x24`, the same three compares
  (0x1a, 0x23, 0x3b), and the displaced `push edi`. January passes `&projected_plane.n` and
  `&result->n` straight into `project_point3d`; our typed copies cost 0x14 of frame.

`real_math.h:967` at HEAD reads `if (k < j || k < i) { return j < i; } else { return _z; }`;
January's body is the three-return form, byte-identical to the out-of-line `decals.obj`
`_projection_from_vector3d` COMDAT (sha `70d35416...`). Every January call site in this object
inlines it, so no spelling inside `leaf_map.c` can reach those compares.

## Law detector pass (w2 A1-A22, w3 A23-A45)

Only the detectors that fired are recorded.

- **A7 (missing prototype).** FIRES: `scratch/gate4/c4013.txt` reports
  `structures\leaf_map.c(400) : warning C4013: 'render_debug_line' undefined`. The genuine owner is
  `source/render/render_debug.h:30`; `leaf_map.c` includes `render/render_debug_geometry.h` but not
  `render/render_debug.h`. **Measured** (`scratch/workers/w3c_leaf_map/v1.c`, the real file plus that
  one include, CRLF preserved): the gate is row-for-row identical to baseline (16/9/0, guard passes),
  a per-record comparison of `v1.obj` against `build/base/source/structures/leaf_map.obj` shows
  **40 records and 0 differences** (size, relocation count and normalized sha), and parkcheck reports
  0 drift. It is therefore fully byte-inert, its five call sites (lines 400/407/412 in
  `_render_debug_leaf_portal`, 658/664 in `_render_debug_leaf_faces`) are inside rows that are
  already EXACT, and a declaration-count change cannot alter an inlined header body's compare
  direction. Per laws_w3 **E18** and brief section 8, a byte-inert hygiene include in a unit where
  nothing lands gets no landable, so it is NOT applied; it is offered to the orchestrator below.
- **A1 (static linkage).** Fires weakly and is refuted. `_leaf_map_globals` has no PDB public
  (`pdb_symbols.json` carries only leaf_map string literals and public code labels), so it is
  plausibly a January static; but the HEAD static census measured zero row changes across 17 units,
  a storage class cannot change a `-4` relocation addend, and **A45** records that a duplicated
  `"static": true` name makes `image_symbol_addresses` drop the name and *breaks* the resolved-equal
  admission the five alias parks depend on. Not proposed.
- **A35 / A22 (leaf age, dot canonicalisation).** Explains the single non-header hunk in
  `_leaf_map_leaf_spans_polygon`; its only measured closure is the OUT helper. No admissible shape.
- **A24 (parameter-home offer).** Checked against `_leaf_map_get_leaf_bounds`: January's `[ebp+8]` /
  `[ebp-0x18]` homes are already reproduced by `b10`; the residual is the header, not a home.
- **A3/A29 (single exit), A23, A25-A28, A30-A34, A36-A44.** No detector fires. Every non-exact row
  already has January's exact instruction count (85/85, 297/297, 256/256); there is no
  inline/out-of-line helper disagreement, no arm-store merge, no `&&`/`||` topology gap, no ternary
  polarity gap, no short MAX/MIN phi, no per-TU `#define helper helper_inline` to remove, no switch,
  no `= {0}` site, and no redundant-store or volatile question in this object.

## Rejected / not-attempted shapes and why

- Re-proposing the `real_math.h projection_from_vector3d` correction: forbidden (owner ruling
  2026-09-15, WORKER_BRIEF 9b).
- Landing `b10`, `s4` or `p1` bodies in the real file: each is EXACT only under the declined header,
  so each would be an **R17** conditional landing on a prerequisite that will not land, and `s4`
  additionally fails the `_point_from_line3d` guard.
- A TU-local re-spelling of the axis selection to dodge the header body: hand-expanded helper,
  **R15** / house rule 13, and a fake match by the owner gate.
- Any source change to the five alias rows: they are byte-identical to January; a change can only
  lose that.

## Proposals

1. **Hygiene (0 B, orchestrator's call).** `source/structures/leaf_map.c`: add
   `#include "render/render_debug.h"` immediately before the existing
   `#include "render/render_debug_geometry.h"` (line 143). It fixes the only C4013 in this unit and
   is measured byte-inert (40/40 records identical). Not landed by this worker because nothing else
   lands here (E18).
2. **Carried forward, unchanged, for whoever owns the P7 decision.** If the owner ever reverses P7,
   this object yields 1,984 padded B with bodies that already exist:
   `_leaf_face_get_vertex3d` 176 (no `.c` change at all), `_leaf_map_get_leaf_bounds` 784
   (`scratch/workers/leaf_map/b10.c`), `_intersect_planes3d` 368 (`p1.c`; its representation-cast
   half is now covered by the w2/w3 "admitted" list for point/vector casts), and
   `_leaf_map_leaf_spans_polygon` 656 only if `_point_from_line3d` also becomes permissible
   (`s4.c`). This is recorded, not proposed.

## Reopen criteria

- `_leaf_face_get_vertex3d`, `_leaf_map_get_leaf_bounds`, `_intersect_planes3d`,
  `_leaf_map_leaf_spans_polygon`: the P7 `projection_from_vector3d` ruling is reversed (or a
  compensating packet removes its `_bitmap_copy` cost); for `_leaf_map_leaf_spans_polygon`, also the
  `_point_from_line3d` COMDAT policy.
- The five `csplit-relocation-alias` rows: a csplit / `symbols.json` change that attributes an
  inter-symbol padding address (4982956, the `-4` bias of `leaf_map_globals.node_stack[count-1]`) to
  the following owner instead of `_faked_xbox_command_line+4`. Nothing a worker can do; the
  resolved-address validator already admits them at zero credit.

## Validation

- `python -B tools/campaign/gate.py source/structures/leaf_map --all --forbid-emitted-symbol _point_from_line3d`
  -> `exact 16 residual 9 unwritten 0`, guard passed (identical to baseline).
- `python -B scratch/parkcheck.py source/structures/leaf_map build/base/source/structures/leaf_map.obj`
  -> `parks 6 drift 0`.
- Owner census: 40 records in the candidate object, identical to `build/base` (no new candidate-only
  code/data/BSS/COMMON owner).
- `python -B tools/fake_match_scan.py source/structures/leaf_map.c` -> 0 review leads.
- `git diff --stat -- source/structures/leaf_map.c` -> empty.
- Worker notes: `scratch/workers/w3c_leaf_map.md`; probe: `scratch/workers/w3c_leaf_map/v1.c`.
