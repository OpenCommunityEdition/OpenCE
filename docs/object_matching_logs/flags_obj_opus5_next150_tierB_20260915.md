# `flags.obj` — next-150K lane, final implementation wave tierB (2026-09-16)

## Outcome

Nothing landed. `source/objects/widgets/flags.c` is unchanged (`git diff --stat` is empty). No source shape was cut.

Baseline and final real-file gate (with the `_point_from_line3d` guard): **exact 14, residual 2, unwritten 0**, the
same row for row. The guard passes. `scratch/parkcheck.py` reports 1 park with 0 drift. The fake-match scan finds 0
leads.

| function | January padded / meaningful / relocs | status | result |
| --- | --- | --- | --- |
| `_flag_update` (Tier B row B2, the only function in scope) | 1184 / 1170 / 26 | parked | SKIPPED-EXHAUSTED: the width lens is refuted at HEAD; only register allocation and slot ranking remain |
| `_flag_render_proper` | 2112 / 84 relocs | residual, not parked | out of scope for this wave |

## Assignment and duplicate-prevention record

The orchestrator assigned research-campaign Tier B row B2. Its evidence was "F3 DECLARED-TYPE: January reads cell -4 as
`word`+`dword` (a `short`); ours reads it via x87/mov". The implied change was to declare the local homed at January's
-4 as `short` (A34 value-width fact). The second label on the row is F6 = TIE, a home permutation
(`scratch/research/campaign-adjudication/xstudy.txt:55`).

Read before the census:
- all 12 `flags_obj_*` ledgers, including the park ledger `flags_obj_flag_update_credible_fuzzy_park_20260903.md`, w2
  `flags_obj_opus5_150k_w2_20260914.md` and w3c `flags_obj_opus5_250k_w3c_20260915.md`;
- worker notes `scratch/workers/flags.md` and `scratch/workers/w3c_flags.md`;
- the parked.json criterion;
- laws_w2, laws_w3, and the research campaign findings (LAW I, M8, F3/D/U, H3, LAW C, LAW E, CJ4, and the section 4
  negative register and retired levers).

History checks:
- `git log --all -- flags.c` shows 17 commits.
- `branch_sweep` checked 12 blobs. None carries an exact `_flag_update`.

## Census (written before any shape)

**Shape.** Both functions have frame `sub esp,0x300`, 1184 padded bytes and 26 relocations. alndiff aligns 390 January
instructions against 393 of ours. The code is identical through 0xa5. The x87 schedule, calls, branches and exits align
1:1.

**Relocations by target.** The 26 targets are identical in symbolic multiset and order. The `__real@` rows differ only in
the known literal COMDAT representation, with the same symbolic names. The addresses from 0x2a0 on move by -1/+2
because of the three extra instructions. No call or global is missing or extra.

**Frame.** Every aggregate, array, byte cell and parameter home has the same displacement and access pattern on both
sides. Only the scalar pool differs:

| owner | January | ours | access kinds |
| --- | --- | --- | --- |
| `short row` (flags.c:521) | -4 | -8 | January: 3 16-bit memory reads + 3 dword stores/loads. Ours: the same 16-bit reads through DX + dword stores/loads |
| `fild` conversion temp | -8 | -0x10 | identical |
| `short neighbor_count` (flags.c:528) | -8 (shared) | -0xc | identical |
| `real wind_scale` (flags.c:533) + neighbor-loop countdown | -0xc | -4 | identical |
| `short column` (flags.c:507) | -0x10 | -0x14 | identical (2b:4, 4b:3) |
| `real weight_sum` / `inverse_weight` (flags.c:570-571) | -0x14 / -0x14 | -0x10 / -0xc | identical |

No cell exists on only one side. There is no dead run and no parameter home recycled on only one side.

## Re-verification of the lenses: refuted as a type defect

1. **frameslot.py at HEAD.** It still prints `cell -4: January INT data-widths [2, 4] | ours MIX data-widths [4]`. The
   byte fact reproduces, but cell -4 has **different owners** on the two sides.
   - January's -4 holds `row`.
   - Ours holds `wind_scale` and the loop countdown, which is where "MIX" comes from.
   - Per owner, the access widths are identical. January's `movsx eax,word [ebp-4]`, `add cx,word [ebp-4]` and
     `movsx eax,word [ebp-4]` match ours' `movsx ecx,dx`, `add cx,dx` and `movsx eax,dx`, all 16-bit.
   - `row` is **already declared `short`**, so the implied change is already in the source.
   - No other owner shows a width difference.
2. **f4law.py verdict** (independent cross-check) → SOURCE-WIDTH. The instances it cites are `add R16, word ptr [ebp-4]`
   vs `add R16, R16` and `movsx R32, word ptr [ebp-4]` vs `movsx R32, R16`. Both sides are 16-bit selections, and the
   operand is memory vs register for the same short `row`.

Both lenses reduce to the fact that w2 and w3c already recorded, a use-count register web:
- Ours keeps `row` in EDX from `point_physics_update` (0x215) to the latch, including `add edx,eax`, a store and a
  loop-head reload.
- January re-reads `[ebp-4]` at every use, and its latch re-enters at the row store (`jmp 0x109`).
- As a result, `neighbor_count` takes EDX in January and ESI in ours.
- The F3 scalar-pool permutation then follows at equal frame size.

## Shapes

None.
- The implied source change is already present.
- No byte fact names any other type, scope, statement or call defect.
- Requirement 3 says to stop once only register allocation, scheduling or x87 ordering remains.
- The levers that remain are all retired by the research campaign:
  - moving a slot by a one-reference gap at equal frame (F3 N10);
  - block scope for spill cells (F3 N14; w3c f1/f2 measured it negative on this function);
  - dead or filler declarations (the M8/H3 instrument, never a landing);
  - H3 numbering (no genuine declaration-census difference is evidenced).

## Classification and reopen criterion

**Classification.** Register allocation (the `row` EDX web) plus the downstream F3 scalar stack-home ranking. This is
class D, not a declared-type defect. The park is unchanged.

**Reopen criterion.** The park criterion, sharpened, applies. Reopen only on one of:
- January-side source or local evidence of an extra or removed `row` reference in the relaxation path. This is the only
  change measured to dissolve the EDX web (w2 D1/D3, diagnostics).
- A measured VC7 register-pass rule that keeps a short loop index out of an EDX web without changing its IL use count.

A width/type lens is refuted for this row.

## Proposals

None for header or config.

**Tooling note for the orchestrator.** No change is requested.
- frameslot.py's DECLARED-TYPE class compares widths by displacement, not by owner. It fires falsely on an equal-frame
  permutation when the moved owner is read through a register web on one side.
- f4law's SOURCE-WIDTH counts `R16, word [mem]` vs `R16, R16` as a width-selection difference.
- B2 should be re-tagged Tier C.

## Checks

- Final real-file gate with the guard: exact 14, residual 2, unwritten 0, identical to baseline row for row.
- `git diff --stat -- source/objects/widgets/flags.c` is empty.
- `scratch/parkcheck.py`: `OK _flag_update 1184 26`, 1 park, 0 drift.
- `tools/fake_match_scan.py source/objects/widgets/flags.c`: 0 review leads.
- Owner census: there is no source change, so the owner set is unchanged.

## Scratch

- Notes: `scratch/workers/tB_flags.md`
- Census artifacts: `scratch/workers/tB_flags/v0.c` (== real file), `v0.obj`, `aln_v0.txt`, `aln_v0_full.txt`, `T.txt`,
  `O.txt`
