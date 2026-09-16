# `hud_unit.obj` — opus5 250K lane, wave w3c (2026-09-15)

## Baseline

`source/interface/hud_unit.c` at HEAD: **21 exact / 1 residual / 0 unwritten**; the
`_point_from_line3d` emitted-symbol guard passes; `scratch/parkcheck.py` reports `parks 1 drift 0`.

The single non-exact row is `_hud_render_unit_interface`, which is **PARKED**:

| | padded | relocs | normalized sha16 |
| --- | ---: | ---: | --- |
| January target | 3504 | 102 | `b82d575e…` |
| park base (real file) | 3472 | 102 | `4071a4f0…` |
| best prior body (w1 `hud_unit_candidate.c`), re-gated at this HEAD | 3504 | 102 | `[sha]` only |

Nothing was landed. `git diff --stat -- source/interface/hud_unit.c` is empty and the final real-file gate
equals the baseline.

## Duplicate prevention

Ledgers read: `hud_unit_obj_opus5_150k_w1_20260914.md`, `hud_unit_obj_renderer_initializer_wave_20260904.md`,
`hud_unit_obj_renderer_independent_review_20260904.md`, `hud_unit_obj_renderer_integration_20260904.md`,
`hud_unit_obj_renderer_precision_followup_20260904.md`, `hud_unit_obj_state_migration_wave_20260904.md`,
plus `scratch/workers/hud_unit_messaging.md`. w1's `branch_sweep` found no exact body anywhere in history
(best blob = the current 21/1/0). Neither `scratch/w2/laws_w2.md` section D nor `scratch/w3/laws_w3.md`
section D lists this function, but w1 had already spent its five allocation-level shapes
(ur9, ur12 ×2, ur13, ur14, ur15) and recorded a tie verdict.

## New evidence: the residual is ONE allocator decision

This wave's contribution is a complete mechanism decode from the January COFF alone
(`alndiff --include-equal`, 1062 target vs 1062 candidate instructions, plus a per-slot `[ebp-N]` census;
`scratch/workers/w3c_hud_unit/w1.full.txt`). Of 166 aligned hunks, 89 are pure frame-slot renumbering.

1. **Shield `draw_flags` colouring.** January frees EBX at the shield block and rematerialises `hud_unit`
   from its home: `+0x372 xor ebx,ebx` / `+0x376 mov [ebp-8],1; mov ebx,[ebp-8]` /
   `+0x380 mov edx,[ebp-0x24]; test byte [edx+0xb6],4`. Ours keeps `hud_unit` in EBX
   (`+0x386 test byte [ebx+0xb6],4`) and runs the whole `SET_FLAG` chain through `[ebp-8]`.
   The health block, where both builds reload `hud_unit` into EBX and then free it
   (`+0x660`/`+0x668`), is byte-identical in shape — so the divergence is a colouring choice at one block,
   not a source shape.
2. **`color[]` induction variable.** With EBX free, January strength-reduces `color[overcharge_index]`
   (`+0x46e lea ebx,[ebp-0xa0]`, `+0x52d mov eax,[ebx]`, `+0x61d add ebx,4`); ours addresses it as
   `[ebp+esi*4-0x9c]`. Both builds keep the same ESI counter with the same `[ebp-0xc]` home.
3. **`draw_flags` spill.** Because EBX is taken by the IV, January spills `draw_flags`
   (`+0x3ac mov [ebp-8],ebx`, `+0x601 mov edx,[ebp-8]`, `+0x630 mov ebx,[ebp-8]`); ours pushes EBX directly.
4. **Frame size.** `[ebp-8]` therefore stays live across the loop in January, so the `fast_ftol` staging
   temp needs a cell of its own (`+0x55e fst [ebp-0x44]`). That is precisely the one extra 4-byte scalar:
   the slot census shows target 35 scalar dwords (`-0x8c…-4`) against our 34 (`-0x88…-4`), the only
   census difference being target `-0x44 ×3` vs our `-0x44 ×2`. Frame `0x424` vs `0x420` follows.
5. **Downstream, not independent.** The health `min_color = max_color` value forwarding
   (`+0x72c mov eax,[ebp-0x15c]; jmp 0x761`, with the self-assigning `<= min_cutoff` arm's store elided at
   `+0x753`), the `jne`-to-body vs `je`-over-body layout of the max-cutoff test, the `xor eax,eax; mov ax,…`
   vs `mov dx,…` health `value_scale` load, and our reuse of the `hud_unit` home `[ebp-0x14]` as an x87
   temp at `+0xb6f..+0xc0a` are all consequences of the same colouring.
6. **Aggregate frame rank (separate).** January orders the aggregates
   `stack_buffer, unit_indices, unit_hud_indices, health_meter, absolute_placement, overcharge_meter, color`;
   ours moves `health_meter` to the second-deepest position. This is an independent ranking difference.

## Shapes tried (2)

Metric: `scratch/workers/w3c_hud_unit/score.py` — normalized structural diff with registers, `[ebp-N]`
displacements and immediates masked (block count / instruction count). For reference the current park base
(`build/base`) scores 127 blocks / 308 instructions.

| shape | change and law | result |
| --- | --- | --- |
| w1 | `scratch/workers/hud_unit_messaging/hud_unit_candidate.c`, re-gated at this HEAD | 3504 / 102, **41 blocks / 56 insns** |
| v1 | ACCESSOR-BINDING law (brief §9): bind `struct meter_hud_element_definition *shield_meter = &hud_definition->shield_meter.meter;`, then `overcharge_meter = *shield_meter;` and `first_meter ? shield_meter : &overcharge_meter`. Motivated by January's non-destructive `+0x422 lea esi,[ecx+0xf4]` against our destructive `add eax,0xf4`. | **worse**: 45 blocks / 65 insns (1061 insns). The bound pointer hoists above the `value_scale` read and rewrites it as `movsx esi,word [esi+0x13e]`, displacing three more slots. Reverted. |
| v2 | Frame-object RANKING probe: restore the real file's `long unit_hud_index` read, dropping `unit_hud_indices` from 5 refs to 4 so that `health_meter` (7 refs / 104 B) would outrank both arrays (4 / 72) under a refs/size rule. | **hypothesis refuted**: frame stays `sub esp,0x420` and `health_meter` stays at `[ebp-0x220]`; score 43 / 58. VC7's aggregate frame rank in this function is not refs/size. Reverted. |

## Shapes deliberately not retried

- **Plain `<` for the health max-cutoff.** Refuted by the bytes: January `+0x727 test ah,1` tests C0 alone,
  which is `!(a >= b)`; an ordered `<` compiles to `test ah,5`. The 2026-09-04 independent review had already
  ruled the `<` spelling a semantic (NaN) regression. The remaining branch-polarity difference is a
  consequence of the join-store forwarding, not of the predicate.
- **Hand pointer walk over `color[]`.** R16 / hand-strength-reduction class; only three uses, each of which
  could name the array, and the units-lane law "strength-reduction beats hand-written pointers" applies.
- **A10 / A38 initializer permutations on the shield `draw_flags`.** w1 ur12/ur15 measured the if/assign,
  `SET_FLAG` and `FLAG(...)`-initializer forms (38/50 and 3536 B).
- **A1 static linkage.** `unit_hud_globals` and the function-level `overcharge_count` are already `static`;
  the 2026-09-15 static-linkage census (brief §9b) closes the lever generally.
- **A24 parameter-home offer.** The only parameter is `player` at `[ebp+8]`, referenced ×16 on both sides
  with no temporary packed into its home — the detector does not fire.
- **A23 / A28 / A40.** No helper changes inline state here (every call is out of line in both builds); the
  only `MAX` was already replaced by w1's byte-proven `shield_fading ? last : shield` pair.

## Checks

- Real file untouched: `git diff --stat -- source/interface/hud_unit.c` empty, `git status --porcelain` clean.
- Final real-file gate: `== exact 21 residual 1 unwritten 0`; `_point_from_line3d` guard passed.
- `python -B scratch/parkcheck.py source/interface/hud_unit build/base/source/interface/hud_unit.obj`:
  `OK _hud_render_unit_interface 3472 102`, `parks 1 drift 0`.
- `python -B tools/fake_match_scan.py source/interface/hud_unit.c`: 0 review leads (same as HEAD).
- Owner census: not applicable — no candidate was landed.

## Proposals

None. No header, config or owner-ruling packet is required: the blocker is a compiler colouring decision,
not a declaration, linkage or policy question.

## Reopen criterion (supersedes the w1 criterion)

Reopen `_hud_render_unit_interface` when a source-level lever is found that makes VC7

1. stop keeping `hud_unit` in EBX across the shield `draw_flags` chain — i.e. reproduce January's
   `+0x380 mov edx,[ebp-0x24]` rematerialisation. That single change yields the `color[]` pointer IV, the
   `draw_flags` spill, the extra `fast_ftol` cell and frame `0x424` as a cascade; and
2. ranks `health_meter` between `unit_hud_indices` and `absolute_placement` in the frame
   (refs/size is refuted as the ranking key — see v2).

A C2 allocator decode (memory note `c2-pogo-attribution`) is the natural next evidence source; no further
source-arrangement sweep is warranted.

**Optional, zero credit (orchestrator decision only).** The park base could be re-measured to
`scratch/workers/hud_unit_messaging/hud_unit_candidate.c` (3504 / 102 — January's exact size and relocation
count; 41/56 structurally against the current base's 127/308). w1 offered this and it was not taken; R11
applies to a strictly-closer body that changes size and is not tied to a reopen, so it is recorded here as
an option rather than proposed.
