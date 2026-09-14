# `structure_visibility.obj` — Opus5 150K wave w2 park re-investigation (2026-09-14)

## Scope and baseline

- Translation unit `source/structures/structure_visibility.c`; target `build/split/source/structures/structure_visibility.obj`.
- Baseline gate (real file, guard passes): `== exact 18 residual 3 unwritten 0`.
- The eight wave-D reopens are inherited and were not touched.
- The three residuals are parked: `_portal_hull_from_points`, `_structure_visibility_build_surfaces_traverse_leaf` and
  `_structure_visibility_traverse_surface_lists`. All work was scratch-only
  (`scratch/workers/structure_visibility_path_structure_bsp_etc/`). **The real file is unchanged.**
- Ledgers read: `structure_visibility_obj_full_reconstruction_20260902.md`,
  `structure_visibility_obj_opus5_100k_waveD_20260914.md` (do-not-repeat list), the two Jonas leaf/rectangle logs and the
  100K handoff.

## Outcome

No function became strict exact, so no park-reopen is proposed. Two parks now have strictly closer scratch bodies that
carry January-proven structure. The orchestrator may re-baseline them; they earn zero credit.

### `_portal_hull_from_points`

Target is 480/16; the parked base is 464/16.

January facts:
- **Long return.** The returns are 32-bit (`mov eax,2`, `mov eax,1`, `xor eax,eax; setl al`). This is the same law that
  made the sibling `planes_intersect_rectangle` exact.
- **Positive body guard.** `fcomp 0; test ah,0x41; jne` targets the discarded epilogue placed after the whole body. That
  gives `if (facing > 0.0f) { ... return count < 3 ? discarded : succeeded; } return discarded;`. This is the same
  epilogue-placement fact as `structure_surface_index_from_point` E2.
- **Register roles.** ESI = result and EDI = source_index appear only with a `?:` pair for `source_index`/`terminator`.

| ID | Shape | Result |
| --- | --- | --- |
| P1 | `long` + `!(facing > 0)` early return | 464; discarded epilogue emitted inline |
| P2 | `long` + positive body guard, trailing discarded return | **480/16**, CFG and epilogues identical, ESI/EDI roles swapped |
| P3 | P2 + `source_index = 0` hoisted before the if | xor hoisted, roles still swapped |
| P4 | P2 + `?:` pair for source_index/terminator | **480/16**, 3 residual hunks |
| P5 | P4 + for-loop increments | identical to P4 |
| P6 | P4 + declarations scoped into the positive block | identical to P4 |
| P7 | P4 with the terminator `?:` first | worse |

P4 residual:
- the pinned `xor edi,edi` is emitted after `cmp winding,1` instead of before it;
- January loads winding into EDX for `source_index += winding`;
- January defers the x-component `fstp` past the loop compare.

Best scratch body: `scratch/workers/structure_visibility_path_structure_bsp_etc/sv_p4base.c`. The other 18 functions stay
exact, including callers `portal_hull_from_portal` and `find_mirror`.

Reopen criterion: a lever for pinned-zero materialisation before a compare, together with the register load of a
parameter used as a loop step.

### `_structure_visibility_traverse_surface_lists`

Target and base are both 528/27.

January facts:
1. The outer-call pushes (`0x100`, `[edi+4]`) precede the lightmap fetch, so the material fetch nests the lightmap fetch.
   This nested `TAG_BLOCK_GET_ELEMENT` idiom is already used in this file by `portal_hull_from_portal`.
2. `mov edx,[ebp-0x14]; mov esi,[edx+0xf8]` occurs inside the triangle-argument evaluation. The compressed vertex address
   is therefore read at the use, with no hoisted `vertex_buffer` local.
3. `mov edi,[ebx+0x48]` comes before the frustum `?:`, so the buffer pointer is initialised before the frustum.

| ID | Shape | Result |
| --- | --- | --- |
| S1 | facts 1-3 together | 528/27, 171/170 instructions, CFG and call/data sequences aligned |
| S2 | S1 + `*surface_index_buffer++` header locals | 26 relocations, eager increments (refuted again) |
| S3 | S1 + for-loop over a `surface_count` local | frame 0x20, no linear-function test replacement (refuted) |
| S4 | S1 + `&&` loop condition instead of `break` | identical to S1 |

S1 residual:
- the ESI/EDI roles are swapped (rendered_cluster and consumed versus the buffer);
- January splits the header advance (`mov ecx,[edi+8]; add edi,8 ... add edi,4`);
- January loads the v0 vertex index before the vertex address.

Best scratch body: `sv_s1base.c`. Note that the probe left one stray blank line before the `{` of the visibility test;
remove it if the body is adopted.

Reopen criterion: a natural spelling of the split header advance together with the ESI/EDI roles.

### `_structure_visibility_build_surfaces_traverse_leaf`

Target and base are both 480/27. The only difference is one parallel-move placement: January copies the rectangle result
(`push ecx; push edx; mov ebx,eax`) after both stack pushes for `planes_intersect_rectangle`; ours copies it between them.

No new shape was tried: wave D already spent L1-L9, and no brief section-9 law addresses call-result copy placement.

Reopen criterion: a measured lever for placing a call-result copy among outgoing pushes.

## Final real-file gate

`== exact 18 residual 3 unwritten 0`. Rows are identical to baseline, the `_point_from_line3d` guard passes, park drift is
0 of 3, the owner census shows no new function or data owners against `build/base`, the fake scan finds 0 leads, and
`git diff --stat` is empty.

## Proposals

No park-reopen, header or config proposal. There is an optional orchestrator decision to re-baseline the
`portal_hull_from_points` and `traverse_surface_lists` parks to P4 and S1 respectively (strictly closer, January-proven
structure, zero credit).
