# `structure_visibility.obj` — opus5 next-150K lane, wave n1 (2026-09-15)

Worker for `source/structures/structure_visibility.c` only, lane root
`C:\halo-worktrees\opus5-150k-house-clean-20260914`, base `ae10935da`.
**The real file is unchanged** (`git diff --stat` empty); all three residuals are parked, so all work is scratch.

Ledgers read first: `_full_reconstruction_20260902`, `_opus5_100k_waveD_20260914` (do-not-repeat table),
`_opus5_150k_w2_20260914`, the two Jonas leaf/rectangle logs, plus `laws_w2` and `laws_w3`.

## Baseline and final gate (identical)

`gate.py ... --all --forbid-emitted-symbol _point_from_line3d` = **exact 18 / residual 3 / unwritten 0**,
guard passes, `parkcheck` parks 3 drift 0, `fake_match_scan` 0 leads.

| Function | January | Real file (parked) |
| --- | --- | --- |
| `_portal_hull_from_points` | 480 / 468 / 16 / `5d359f8240cd82db` | 464 / 16 `[size, sha]` |
| `_structure_visibility_traverse_surface_lists` | 528 / 521 / 27 | 528 / 27 `[sha]` |
| `_structure_visibility_build_surfaces_traverse_leaf` | 480 / 27 | out of scope (do-not-repeat) |

## NEW EVIDENCE: two January assert literals prove a local name and two expression texts

`tinfo --data` sections 35 and 36 of the January object own:

```
??_C@_07KLBPNJKH@ooz?$DO0?4f?$AA@                  = "ooz>0.f"                    (8 bytes)
??_C@_0BL@FCCKLIAE@result?9?$DOvertex_count?$CB?$DNNONE?$AA@ = "result->vertex_count!=NONE" (27 bytes)
```

The parked body emits `"one_over_z > 0.0f"` (18) and `"result->vertex_count != NONE"` (29). Because `match_assert`
stringizes `#expr`, these are direct source evidence: January's local is named **`ooz`** and both assert expressions
are written with **no spaces** and `0.f`, not `0.0f`. The space-free style already has in-tree precedent
(`source/structures/leaf_map.c:971`). A full `.rdata` string census of `build/base` vs `build/split` for this object
shows these are the **only** two mismatched literals in the whole TU.

Second defect found in the same body: it declares `struct structure_bsp *structure = global_structure_bsp_get();` and
discards it with `(void)structure;`. January calls the function and never reads EAX, which laws_w2 **A17** spells as
the bare validation-call statement (admitted in laws_w2 section B; in-tree precedent `source/ai/action_converse.c:70`).
An unused local is prohibited outright by the brief's house-rule list.

## `_portal_hull_from_points` — strictly closer body, park re-baseline proposed

Census before any shape:

- **Frame.** January `sub esp,0xc08` = `real_point3d view_space[MAXIMUM_PORTAL_HULL_VERTICES]` (0xc00) + `[ebp-4]`
  terminator + `[ebp-8]` ooz. Ours identical; no slot added or missing.
- **Relocations.** 16 = 16, same order and (after the fix) same addresses: `_global_structure_bsp_get` (result
  DISCARDED), `__real@3fb99999a0000000`, `__real@00000000` ×2, `__real@bf800000`, `_matrix4x3_transform_point`,
  `_screen_plane`, `_convex_polygon3d_clip_to_plane`, the file literal ×2, `_display_assert` ×2, `_system_exit` ×2 and
  the two assert-expression literals. Nothing missing on either side.
- **Structural cause of 464 ≠ 480** (w2, reproduced): January's returns are 32-bit (`mov eax,2`, `mov eax,1`,
  `xor eax,eax; setl al`) ⇒ `long` return; and the discarded epilogue sits after the whole body ⇒
  `if (facing > 0.0f) { ... } return discarded;`, not an early `if (facing <= 0.0f) return discarded;`.

Shapes (2 this wave; w2 spent P1–P7):

| ID | Shape | Result |
| --- | --- | --- |
| V1 | w2 P4 + bare `global_structure_bsp_get();` + `ooz` + both January assert texts | **480 / 16**, 3 differing slots; the A17 change is byte-inert, the assert changes remove two candidate-only `.rdata` owners and reproduce January's two literal owners exactly |
| V2 | V1 with the `?:` pair replaced by `source_index = 0;` + an asymmetric if/else | reproduces January's hoisted `xor edi,edi` before `cmp word [ebp+0x10],1` exactly (laws_w2 A38/A42), but the whole ESI/EDI web swaps (≈12 slots). Strictly worse |

V1 = `scratch/workers/n1_structure_detail_objects_structure_visibility_etc/pv_v1.c`, 480/16/`436f25638df8c7a2`.
Whole-TU gate exact 18 / residual 3 (rows identical to baseline); guard passes; parkcheck drift only on this park;
owner census vs base: cand-only = January's two literals, base-only = the two wrong ones they replace;
`fake_match_scan` 0 leads.

Residual, 3 slots, all allocator/scheduler:

1. `xor edi,edi` one block early in January (V2 shows a source lever exists but it flips the register web);
2. January `mov edx,[ebp+0x10]; … add edi,edx` vs our `add edi,[ebp+0x10]` (addressing mode, −2 bytes);
3. January defers `fstp [eax+4]` past `inc ebx; cmp di,[ebp-4]`.

**Proposal (zero strict credit).** Re-baseline the park to `pv_v1.c`: strictly closer under R11 (the `[size]` tag is
gone), January-proven structure, it replaces two candidate-only `.rdata` owners with January's exact ones, and it
removes a house-rule violation from the parked body.

**Reopen criterion.** A lever that keeps the `?:` pair's ESI/EDI web while materialising the zero before the winding
compare (or fixes the web under the hoisted-initializer form). The x87 store deferral and the parameter-load
addressing mode have no known lever.

## `_structure_visibility_traverse_surface_lists` — hypothesis refuted, w2 S1 stands

Census: frame `sub esp,0x1c` on both, slots `[ebp-4]` consumed count, `[ebp-8]` rendered_cluster_index, `[ebp-0xc]`
frustum — identical. Relocations 27 = 27, identical multiset and order. Not structural.

Remaining January facts not reproduced by S1: (a) rendered_cluster in ESI and the surface-index buffer in EDI (ours
swapped; ≈13 of the differing slots), (b) the 12-byte header advance split as `mov ecx,[edi+8]; add edi,8 … add edi,4`,
(c) the v0 vertex index loaded before the vertex address.

Shape tried (1): `tsl_v1.c` reads the three header words as `*surface_index_buffer++` in a **well-defined** spelling
(two named index locals first, so there is no double modification between sequence points) while keeping S1's nested
`TAG_BLOCK_GET_ELEMENT`. **REFUTED**: VC7 emits eager per-read `add esi,4` increments, 178 instructions and 26
relocations. Together with w2's S2 this refutes the post-increment explanation of fact (b) from both spellings; the
8/4 split is a compiler induction-variable artifact, not a source form.

**Reopen criterion.** A lever that assigns the rendered_cluster web to ESI and the buffer web to EDI. Note for the
next taker: January's first ESI definition is `mov esi,eax` immediately after `_rendered_cluster_get` (+0x49) — the
rendered_cluster web is defined first and wins ESI, while our build gives that same first-defined web EDI, so the
lever is allocator priority, not definition order.

## Proposals

1. Park re-baseline `_portal_hull_from_points` → `pv_v1.c` (zero credit; fixes two data owners and one house-rule
   violation).
2. No header, config or symbols.json packet.
