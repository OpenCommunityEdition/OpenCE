# Geometry: Opus5 150K house-clean lane, wave w3 (2026-09-14)

## Scope and provenance

- Translation unit `source/math/geometry.c`; January target `build/split/source/math/geometry.obj`.
- XDK 3911 VC7 13.00.9254 with the repository flags, unchanged. Every measurement is a whole-TU compile of a
  scratch copy through `tools/campaign/gate.py --source ... --forbid-emitted-symbol _point_from_line3d`
  (one diagnostic, p3_diagP, deliberately trips the guard and is not proposable).
- No header, config, ninja, configure, pytest or git write was used. The real file is unchanged
  (`git diff --stat -- source/math/geometry.c` is empty).
- Worker notes with every shape: `scratch/workers/w3_geometry.md`; candidates, bodies and alignments:
  `scratch/workers/w3_geometry/` (`bodies/*.txt`, `*.gate.txt`, `*.aln.txt`).
- This wave ran in three sessions; two were cut off. Their scratch trials were re-read from the gate and alndiff
  files and are recorded below with the session-4 work.
- Evidence: the January split object (bytes, relocations, frame slots, assert anchors); HCEA
  `halocea-review/src/blam/{math/convex_hull3d_expand.c, math/convex_polygon3d_clip_to_plane.c, effects/convex_hull2d.c}`
  (360 build: names and topology only); Stian `src/halo/math/geometry.c` (PC lift, topology only); laws A1-A22 and
  rejection patterns R1-R12 from `scratch/w2/laws_w2.md`.

## Baseline and final real-file state

| | Gate | Guard | Park drift | Owner census vs build/base | Fake scan |
| --- | --- | --- | --- | --- | --- |
| Baseline (HEAD 210aa8f81) | exact 24 / residual 6 / unwritten 0 | pass | - | - | - |
| Final | identical, row by row | pass | 0 / 6 | identical (124 = 124 external symbols) | 0 leads |

All six residuals are parked. **No newly exact rows. No park-reopen proposal** (no scratch candidate is strictly exact).

## Per-function outcomes

| Function (January padded) | Outcome | Best scratch candidate | Remaining difference |
| --- | --- | --- | --- |
| `_get_edge_vertex` (416) | NOT-LANDED, C2 search exhausted | w2 g5 body: 416/17 [sha] in every context | one transposed pair of parameter reloads after the calculate_vertex loop |
| `_convex_hull2d` (784) | NOT-LANDED, structural progress | `h2d_s3.c`: 784/25, 13 blocks / 26 rows (park base 35 / 122) | one `mov byte [ebp-1],0` placed before `fsub start.y`; January places it after |
| `_convex_hull3d_expand` (1808) | NOT-LANDED, policy-held form | `h3e_n4a.c`: 1808/67, 597/597 insns, 7 blocks / 14 rows, all in the closing block | `edges` homed in esi early plus a read-before-store; the form also needs an owner ruling |
| `_convex_polygon2d_clip_to_plane` (1008) | NOT-LANDED, structural progress | `p2d_s2.c`: 357/356 insns, 201 rows (park base 355/357, 248 rows) | callee-saved colouring swap, plane reload, x87 operand order, fsubr |
| `_convex_polygon3d_clip_to_plane` (1120) | NOT-LANDED; closer form owner-blocked | `p3_diagP.c` (guard fails): 406/406, 25 blocks / 46 rows | in-loop dot-term order, result reload order, fsubr |
| `_convex_hull2d_perimeter` (96) | SKIPPED-EXHAUSTED | none compiled | loop squaring schedule; the only exact form was already rejected under R3 |

### `_get_edge_vertex`

The orchestrator asked (C2) for a new search with the landed torus, hull3d_begin and geosphere bodies in place.

- The alignment of every candidate against January shows **one dependency-free transposition** at the loop exit. January reloads `subdivision_index` (`mov dx,[ebp+0x10]`) before the `edge_vertex_index` home (`mov eax,[ebp+0xc]`); ours does the reverse. The other 152 instructions, the 0xc frame and all homes are identical.
- Shapes tried, all 416 [sha] with the same transposition:
  - A: g5 without the `new_vertex` local;
  - B: `&indices[va*MAX+vb]`;
  - C: the offset ternary inlined into the sum;
  - D: function-scope `long offset`;
  - E: function-scope pointer and offset.
- A context re-gate of g5 inside this wave's best scratch bodies is still 416 [sha]:
  - `gev_ctx`: h2d_s3 + p2d_s2 + h3e_n4a;
  - `gev_ctx2`: p2d_s2.
- Classification: TU-context reload-order tie with no in-function lever.

### `_convex_hull2d`

- **A12 loop-invariant hoist order.** January emits the `current` pointer `lea esi,[edi+eax*8]` after the angle loop's `test ax,ax; jle` guard, so the pointer is written inside the loop body. Shape s2 applied this. It went from 35 blocks / 122 rows to 16 blocks / 43 rows. The zero pin, the vertex_count register, the second epilogue and the best_index slot differences all disappeared.
- **Declaration order.** January forms `best` before `start` in the nondegenerate block (same order as HCEA). Shape s3 applied this and reached 13 blocks / 26 rows.
- The only non-relocation difference left is the placement of the boolean zero store inside the y test.
- Rejected shapes:
  - s1 `hull_count++` split: inert.
  - s4 `>= || >=`: wrong encoding. It emits `test ah,1; je`, but January's `test ah,5; jp` proves the `!(a<eps)` form.
  - s5 `&points[i]`: inert.
  - s6 Bungie's `_real_epsilon > fabs(...)` idiom: inert.
  - s3b direct array indexing: inert.

### `_convex_hull3d_expand`

The session-3 salvage and session-4 shapes form a chain:

| Shape | Change | Result |
| --- | --- | --- |
| x1 | A4 `plane3d_distance_to_point` helper in both surface loops | 591 vs 597 insns |
| n1 | x1 + conditional initializer | inert |
| x2diag | surface pair formed before its range test | 602 insns, 1824 bytes |
| n2 | x2diag + `edge = edges + edge_index` before assert 2098 | 597/597 |
| n3 | n2 + the same before assert 2167 | 600 insns |
| n4a | n3 + closing store `edges[new_edge_index].surface_indices[1] = edges[first_new_edge_index].surface_indices[0]` | 597/597, 14 rows in the closing block only |

Byte evidence for n3 and n4a: January 0x3b6 forms `edges + edge_index` (`movsx; shl 5; add`) before the 0x877 range test. The closing store is indexed from `edges` (`[ecx+esi+0x18]`), not through the loop's `new_edge` pointer.

Scheduling-phase shapes, all no better than n4a:
- n4b, no loop `new_edge` pointer: worse.
- n5, `first_new_edge` pointer: worse.
- n6, closing stores swapped: January's esi home but the wrong store order.
- n7, `new_vertex` pointer: identical.
- n8 / n9, `previous_new_edge_index` in the closing stores: identical (copy-propagated).

**Policy:** n2-n9 contain the pointer-before-check form that the 20260908 root rejected as abstract-C UB. It is on owner hold (w2 ledger), so nothing here is proposable even if it closes.

### `_convex_polygon2d_clip_to_plane`

- **A18 value fact (p2d_s1).** January materialises the TEST_FLAG value (`neg/sbb/neg; test eax,eax; je`) and sends the `clip_flags` NULL test straight to the clear arm. So the SET_FLAG value is `clip_flags ? TEST_FLAG(*clip_flags, point_index) : FALSE`.
  - Result: 357/357 instructions, with control flow now aligned.
  - Wave D rejected this spelling on a raw T-diff count only.
- **p2d_s2.** Passing the PIN expression directly as `point_from_line2d`'s `t` reproduces January's PIN join (`fld st(0); fmul st(3)`) and the trailing `fstp st(0)` pair. Result: 201 rows.
- **Declaration order.**
  - p2d_s3, `result_count` declared first: worse, 207 rows.
  - p2d_s3b, `result_count` before `result_clip_flags`: identical to s2.
- Left after s2:
  - callee-saved colouring: January uses edi for result_count and ebx for count / result_clip_flags; ours is the reverse;
  - esi shrink-wrap;
  - plane kept in ecx through the clip block in January;
  - count*8 CSE / point_index slot swap;
  - pre-loop x-term operand order;
  - fsubr load of `result[count-2].x`.

### `_convex_polygon3d_clip_to_plane`

- Session-3 salvage, none better than the wave D q2 baseline:
  - p3_q2, single-exit dispatch: identical to wave D q2;
  - p3_v2, `&points[i]`: identical;
  - p3_v3, function-scope `point`: identical;
  - p3_diagH, duplicate-point helper: 976/29, worse.
- **Diagnostic p3_diagP** (the guard FAILS, so it is not proposable) is the exact 3D analogue of p2d_s2: `point_from_line3d(point, &vector, PIN(...), result + result_count)`.
  - Result: 406/406, 25 blocks / 46 rows. The PIN join and the `fstp st(0)` pair now match.
  - This is strong evidence that January's 3D body used the `point_from_line3d` inline, as its 2D sibling uses `point_from_line2d`.
  - It is blocked by the lane rule: `_point_from_line3d` COMDAT forms are OUT.
- The remaining schedule items are the same family as the 2D sibling.

### `_convex_hull2d_perimeter`

- The only difference is the loop's in-place `fld st(1); fmulp st(2)` (dx² first).
- A11 (explicit-cast CSE-kill) would require a redundant `(real)` cast, which R7 rejects.
- The wave D exact form swaps operands and is rejected under R3.
- No admissible shape exists, so none was compiled.

## Rejected shapes (do not repeat)

| Function | Shapes |
| --- | --- |
| get_edge_vertex | gev_A-E; context re-gates gev_ctx, gev_ctx2 |
| convex_hull2d | s1, s3b, s4 (`>=`), s5, s6 (`eps > fabs`) |
| convex_hull3d_expand | n1, n4b, n5, n6, n7, n8, n9 |
| polygon2d | s3 (result_count first); s3b is inert |
| polygon3d | q2, v2, v3, diagH, diagP (helper-banned) |

## Reopen criteria

| Function | Reopen when | Start from |
| --- | --- | --- |
| `_get_edge_vertex` | a geometry park change makes the context flip; re-gate `bodies/gev_g5.txt` after every park change, with no more local respellings | `bodies/gev_g5.txt` |
| `_convex_hull2d` | there is evidence for the nondegenerate-predicate spelling that places the zero store after the y subtraction | `bodies/h2d_s3.txt` |
| `_convex_hull3d_expand` | the owner rules that declare+initialise pointers ahead of their index assertions are admissible (January byte order proves them) | `bodies/h3e_n4a.txt` |
| `_convex_polygon2d_clip_to_plane` | there is a lever for the count/result_count colouring and for the fsubr load | `bodies/p2d_s2.txt` |
| `_convex_polygon3d_clip_to_plane` | the owner rules on the `point_from_line3d` COMDAT, or a C form without the helper reproduces the join | `bodies/p3_diagP.txt` |
| `_convex_hull2d_perimeter` | a genuine j-first helper or source turns up | none |

For `_convex_hull3d_expand`, the next spelling to measure is HCEA's read of `closing_edge->surface_indices[0]` before the `edge_indices[0]` store.

## Orchestrator proposals

None. Nothing needs a header or config change, and no parked body closed.
