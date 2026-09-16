# `structure_detail_objects.obj` — opus5 next-150K lane, wave n1 (2026-09-15)

Worker for `source/structures/structure_detail_objects.c` only, lane root
`C:\halo-worktrees\opus5-150k-house-clean-20260914`, branch
`opus/next-150k-house-clean-20260915`, base `ae10935da`. VC7 13.00.9254, unit flags unchanged.
**The real file is unchanged** (`git diff --stat` empty); both non-exact rows are parked, so all work is scratch.

Ledgers read first: `_jonas_wave`, `_jonas_dot_product4d_20260827`, `_jonas_initialize_for_new_map_20260827`,
`_small_family_completion_20260901`, `_opus5_150k_w2_20260914`, plus `scratch/w2/laws_w2.md` (R6, C2) and
`scratch/w3/laws_w3.md`.

## Baseline and final gate (identical)

`gate.py source/structures/structure_detail_objects --all --forbid-emitted-symbol _point_from_line3d`
= **exact 13 / residual 2 / unwritten 0**, guard passes, `parkcheck` parks 2 drift 0.

| Function | January padded / relocs / sha16 | Real file (parked) |
| --- | --- | --- |
| `_structure_render_detail_objects` | 1136 / 31 / `dbf237f0819e54a6` | 1136 / 31, `[sha]` |
| `_structure_detail_objects_initialize` | 80 / 3 / `99e687328f033302` | 80 / 3, `[sha]` |

## `_structure_render_detail_objects` — PARK-REOPEN PROPOSED (strict EXACT in scratch, 1,136 B)

Candidate: `scratch/workers/n1_structure_detail_objects_structure_visibility_etc/sdo_reopen.c`
(byte copy of the current real file with only this body replaced; CRLF preserved).

- Whole-TU gate: **exact 14 / residual 1 / unwritten 0**; all 13 baseline-exact rows stay EXACT; guard passes.
- `scratch/parkcheck.py`: the `initialize` park is OK; only the reopened body drifts, and it drifts **to the target
  hash** `dbf237f0819e54a6`.
- Owner census vs `build/base`: cand-only `[]`, base-only `[]` (vs `build/split` only the pre-existing literal and
  `_fast_ftol` select-any set, identical to base).
- `fake_match_scan`: 0 leads.

**Why this is admissible now.** Wave w2 measured this exact body and the verifier REJECTED it under laws_w2 **R6**
(hand-written compiler output: `layer_cell_counts[0] = 0; ... memset(&layer_cell_counts[1], ...)`) while recording the
concrete fix in laws_w2 **C2**: *"`_structure_render_detail_objects` 1136: `verify/.../t6.c` is already exact;
resubmit."* The candidate is exactly that file. `diff cand_sdo.c t6.c` is a single hunk:

```
-	short layer_cell_counts[32];
-	unsigned long visible_layer_flags;
-	layer_cell_counts[0] = 0;
-	visible_layer_flags = 0;
-	memset(&layer_cell_counts[1], 0, sizeof(layer_cell_counts) - sizeof(layer_cell_counts[0]));
+	unsigned long visible_layer_flags = 0;
+	short layer_cell_counts[32] = { 0 };
```

i.e. the natural `= {0}` initializer of laws_w2 **A21** in place of the compiler expansion. The only other change I
made is collapsing one stray double blank line (byte-inert, laws_w3 A41).

The structural argument for the rest of the body is unchanged from the w2 ledger (the park's premise
"x87/frame/pointer-lifetime scheduling" was wrong; the gap was a frame-size gap caused by four byte-proven
source-shape facts: single-statement aggregate initializers, the separate view fetch, fresh byte conversions for the
x/y offsets, and the store/call order of the cell header). The pre-existing credibility disclosures (unchecked NULL
`detail_object_data`, the `key.z` restore, the `-1`/`++` bounds pair) are all already in the parked body.

## `_structure_detail_objects_initialize` — NOT LANDED (stack-adjust scheduling tie)

Census (`alndiff`, 25 target vs 25 candidate instructions, one transposition):

```
T 0x2f  add esp,0xc                          O 0x2f  mov [_debug_detail_objects+4],eax
T 0x32  mov [_debug_detail_objects+4],eax     O 0x34  add esp,0xc
```

Everything else is byte-identical. Neither build allocates a frame (no `push ebp`, no `sub esp,N`), so there is no
frame census to write; the relocation census is identical in multiset, order and address apart from those two
instructions (`.rdata "structure detail objects"`, `_game_state_malloc`, `.bss _debug_detail_objects+4`). The row is
therefore **not structural**: only the flush point of the deferred cdecl `add esp,0xc` differs.

The source statement order is byte-forced and already correct (three component stores, then the global publish, then
the `l` store). VC7 cannot sink or hoist the global store across stores through the malloc'd pointer, so no statement
permutation can move the pop between the `k` store and the publish.

Shape tried this wave (1; w2 spent i1–i5):

| ID | Shape | Result |
| --- | --- | --- |
| i6 | laws_w3 A33 lvalue topology: publish through the global on the last store (`detail_object_global_runtime_data->default_z_reference_vector.l = 0.0f;`) | 80/3, **byte-identical** to the parked body — VC7 forwards the just-stored pointer |

No second shape was spent: every remaining device is prohibited (a second pointer alias to the same storage = R16; a
representation cast to route the vector through `set_real_vector3d`; statement permutations refuted above).

**Reopen criterion** (unchanged from w2, restated with the census): a construct, demonstrated in any exact
VC7 13.00.9254 object in this tree, that flushes a pending cdecl `add esp,N` BEFORE a `mov [global],reg` store that is
itself followed by another store through `reg`. A byte-pattern scan over `build/split` for that sequence is the
cheapest way to find a donor; it is a cross-object sweep and belongs to the orchestrator, not to a per-function shape
budget.

## Proposals

1. **park-reopen** `_structure_render_detail_objects` from `sdo_reopen.c` (1,136 B, strict exact, 0 regressions).
2. No header, config or symbols.json packet is needed. The w2 names-only follow-up (`detail_object_cell_coord` tag
   rename from the atlas mangled records) can ride at unpark time.
