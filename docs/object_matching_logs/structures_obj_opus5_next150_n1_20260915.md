# `structures.obj` — opus5 next-150K lane, wave n1 (2026-09-15)

Worker for `source/structures/structures.c` only, lane root
`C:\halo-worktrees\opus5-150k-house-clean-20260914`, base `ae10935da`.
**The real file is unchanged** (`git diff --stat` empty). All work is scratch.

Ledgers read first: `_jonas_cluster_marker_pair_20260828`, `_fable_cluster_pair_intake_20260906`,
`_jonas_planar_fog_definition_index_boundary_20260829`, `_jonas_sphere_portal_first_shot_20260830`,
`_opus5_150k_w2_20260914`, the structures section of `lane_reconciliation_batch5_fuzzy_ui_world_20260909`,
`structure_render_obj_jonas_fog_offset_leaf_20260821`, plus `laws_w2` and `laws_w3`.

## Baseline and final gate (identical)

`gate.py source/structures/structures --all --forbid-emitted-symbol _point_from_line3d`
= **exact 12 / residual 5 / unwritten 0**, guard passes, `parkcheck` parks 4 drift 0, `fake_match_scan` 0 leads.

Assigned rows: `_structure_get_planar_fog` (structural, parked), `_sphere_intersects_cluster_portal` (structural,
NOT parked), `_structure_cluster_unmarked` (sha-only, parked). `_render_debug_fog_planes` and
`_structure_cluster_mark` are out of scope for this wave.

## `_structure_get_planar_fog` — structural gap CLOSED and the recorded blocker REMOVED (448 / 15)

Candidate `scratch/workers/n1_structure_detail_objects_structure_visibility_etc/fg_v1.c`,
**448 / 15 / `7e308d0bf30135a5`**, 154 vs 154 instructions (target 448 / 442 / 15 / `a5aa0f690e82204d`;
parked base 416 / 14 `[size, relocs, sha]`).

Census:

- **Frame.** `sub esp,0x14`-class on both, `[ebp-1]` screen_fog and `[ebp-0x10]` offset_vector; `cluster` is spilled
  into the `fog` parameter home `[ebp+0xc]` in BOTH builds (laws_w3 A24 parameter-home reuse, already reproduced).
  No slot gap.
- **Relocations — the structural fact.** January 15, parked body 14. The missing one is a third
  `_tag_block_get_element_with_size` at +0x16d inside the fog-offset block whose result is never read (the next
  instruction is `fld [edi+4]`). January performs a fog-plane lookup there and discards it. The body also lacked the
  second `_global_structure_bsp_get` re-fetch at +0x9b: January's entry `structure` register is destroyed by the
  inlined TRUE-index call, so the cluster lookup and both fog-plane lookups run off a re-fetched pointer. Both gaps
  are **missing statements**.

Shapes (2):

| ID | Shape | Result |
| --- | --- | --- |
| fg_g1 | w2's `g1` planar-fog body re-measured on the CURRENT real file (w2's `g1.c` predates the two landed reopens and regresses them if used whole) | 448 / 15, 154/154 instructions |
| fg_v1 | fg_g1 with the unused `struct structure_fog_plane_render *fog_plane` local replaced by the **bare A17 validation call**, and `offset` changed to declare-then-assign so the call precedes the literal-zero load as January's bytes require (laws_w3 A38) | **byte-identical to fg_g1**; `CL /W3` and `/W4` emit no TU-local warning in either the candidate or the baseline |

**This removes the w2 blocker.** w2 recorded that this row "also needs an owner ruling on unused-lookup locals".
It does not: the discarded lookup is the laws_w2 **A17** class, whose admitted spelling is the bare statement
(laws_w2 section B, "Admitted … bare validation calls"; in-tree precedent `source/ai/action_converse.c:70`
`actor_get(actor_index);`). The bare form is byte-identical to the unused-local form, so no ruling is required.

Residual — the **entire** difference is one 4-instruction register-naming window in the first fog-plane lookup:

```
January: xor edx,edx / mov [esi+0x1c],1 / mov dx,[eax+2] / push 0x20 / lea eax,[ebx+0x178] / and edx,0x7fff / push edx / push eax
ours   : xor ecx,ecx / mov [esi+0x1c],1 / mov cx,[eax+2] / push 0x20 / lea edx,[ebx+0x178] / and ecx,0x7fff / push ecx / push edx
```

Same instructions, same order, same lengths: January reuses the dying `cluster` register EAX for the block address
and puts the masked index in EDX; we use ECX/EDX and leave EAX idle. The second (discarded) lookup is byte-identical
in both builds.

Checks: whole-TU gate exact 12 / residual 5 with every baseline-exact row still EXACT; guard passes; parkcheck drifts
only this park (416/14 → 448/15); owner census vs `build/base` cand-only `[]` base-only `[]`; fake scan 0 leads.

**Proposal (zero strict credit).** Re-baseline the `_structure_get_planar_fog` park to `fg_v1.c`. It closes both
structural tags, adds the two January-proven statements, uses only admitted constructs and needs no owner ruling.
**Reopen criterion** then becomes: a lever that makes VC7 reuse the dead `cluster` register for
`&structure->fog_planes` (EAX) and allocate the masked index to EDX in the first fog-plane lookup.

## `_sphere_intersects_cluster_portal` — structural gap closed WITHOUT the forbidden COMDAT; exact form still blocked

This is the only non-parked residual in the group. Target 432 / 418 / 11 / `f8a78ffbcb1b9819`; real file 416 / 11
`[size, sha]`.

Census: frame identical (`projected_vertices[…]` at `[ebp-0x414]`, `projected_position` `[ebp-0xc]`,
`projected_position2d` `[ebp-8]`, parameter homes reused for the projection/sign temps); relocations 11 = 11 in the
same order. Structural cause of 416 ≠ 432: the shipped body returns early twice, whereas January has ONE shared false
epilogue at the bottom (`jne 0x199` from the first test) with the true return nested under all three tests, and its
`projection_sign` local is a `short` (`movzx ax,al` at +0xed), not a `boolean`.

Shapes (2):

| ID | Shape | Result |
| --- | --- | --- |
| sph_v1 | current file + single-exit/nested-guard restructure, nested collision-plane argument, `short projection_sign`, hand-written component rows | **432 / 11, `[sha]` only — the size gap closes without `_point_from_line3d`**; 181 vs 182 instructions |
| sph_v2 | sph_v1 + `real negative_plane_distance = -plane_distance;` for the three component rows | **432 / 11, 182 vs 182 instructions**; January's `fchs` + `fld st(0); fmul; fadd` block is now EXACT (our `fmul; fsubr` folding of the unary minus is gone) |

`sph_v2` residual, 3 clusters: (A) +0x5d `fld [edi]; fmul [eax]` vs ours reversed — the inlined
`plane3d_distance_to_point` x-term operand age; (B) +0x92…+0xa0 the inlined `distance_squared3d` term order
(5 instructions moved); (C) three 1-slot integer/x87 interleaves (`movzx ax,al`, `push eax`, `lea eax,[ebp-0xc]`),
which is the SAME 3-slot residual w2's `v5` reached *with* the helper.

(A) and (B) are the laws_w3 **A35(4)** effect: the hand expansion ages the normal/position fields and flips the two
*earlier* inlined products. They are exactly what `point_from_line3d` buys, and nothing else measured buys them.
Under the standing FINAL ruling ("no `_point_from_line3d`-dependent reconstruction anywhere") strict exact is
unreachable for this function in this lane.

Checks on `sph_v2`: whole-TU gate exact 12 / residual 5 with every baseline-exact row still EXACT; guard passes;
parkcheck parks 4 drift 0; owner census cand-only `[]` base-only `[]`; fake scan 0 leads.

**Not landed on purpose.** The row is not parked and its body is complete, so brief section 8 forbids a worker from
landing a non-exact body. `sph_v2.c` is nevertheless structurally correct where the shipped body is byte-provably
wrong; taking it is an orchestrator decision (zero strict credit, and it would need a park entry). Default
recommendation: leave the real file untouched.

**Reopen criterion.** Either the `_point_from_line3d` ruling is lifted (then w2's `v5.c` is 6 ops away and only the
3-slot interleave remains), or an A35 lever ages `plane_normal`/`position` the way a genuine inline call does.

## `_structure_cluster_unmarked` — SKIPPED-EXHAUSTED (standing owner hold, no new evidence)

Zero shapes spent, by design. The 2026-08-28 ledger records an explicit no-retry boundary for this topology; the
2026-09-06 Fable intake HOLDS both marker functions and rejects the cast / result-temp / explicit-if chain as
post-measurement selection; laws_w2 **R1** rejected w2's single-exit re-argument of the same body; laws_w3 **R13**
makes those holds binding on later waves.

Free census (`build/base` vs `build/split`, no compile): 47 target vs 45 candidate instructions; relocations 11 = 11,
identical targets and order; no frame. The entire residual is the result materialisation:

```
January: mov ecx,[_structure_globals+8+eax*4] ; cmp ecx,[_structure_globals+4] ; setne al
ours   : mov edx,[…+eax*4] ; mov eax,[…+4] ; xor ecx,ecx ; cmp edx,eax ; setne cl ; mov al,cl
```

The three source forms that remove the temporary are precisely the held ones.

New negative evidence: a full `.rdata` string census of `build/base` vs `build/split` for `structures.obj` shows
**zero** mismatched assert texts (every ours-only string is a select-any COMDAT January references from another
object). The assert-literal lever that unblocked `_portal_hull_from_points` in `structure_visibility.obj` does not
exist here.

**Reopen criterion.** An owner ruling admitting one of the held forms, or a genuinely new source authority (not a
measurement-driven selection) that independently produces January's direct `setne al` without a temporary.

## Proposals

1. Park re-baseline `_structure_get_planar_fog` → `fg_v1.c` (448/15; closes both structural tags; **removes** the
   recorded "unused-lookup local" ruling requirement).
2. Orchestrator decision only: whether `sph_v2.c` (432/11, structurally correct, zero credit) should replace the
   non-parked `_sphere_intersects_cluster_portal` body. Worker recommendation: no.
3. No header, config or symbols.json packet.
