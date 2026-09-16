# `lra_cache.obj` — opus5 next-150K lane, wave n2 (2026-09-15)

Lane `C:\halo-worktrees\opus5-150k-house-clean-20260914`, branch
`opus/next-150k-house-clean-20260915`, baseline `ae10935da`.
Production source was NOT modified. `git diff --stat -- source/memory/lra_cache.c` is empty.

## State

| measurement | baseline | final |
| --- | --- | --- |
| gate | exact 13 residual 1 unwritten 0 | exact 13 residual 1 unwritten 0 |
| `_lra_allocate` | 368 padded / 4 relocs / `bd671ca359d22c92` | unchanged |
| target | 400 padded / 388 meaningful / 4 relocs / `93beee39f5b39866` | — |

Park `_lra_allocate` [register-allocation]: OK, drift 0.

## What this wave adds: a complete census, and one decisive negative

Prior ledgers (2026-08-26 policy fixed point, 2026-08-29 global census, 2026-08-30 credible fuzzy park)
describe the residual as a "loop-carried zero" and record E1-E50 plus N1-N3/A1. This wave measured the
gap block by block for the first time.

**The `[size 368!=400]` tag is not a frame gap and not a missing statement.**
`sub esp,0x14` on both sides; the five slots are the same five variables; the relocation census is
4/4 with identical targets and order. The three middle cells are only ROTATED:

| slot | January | ours |
| --- | --- | --- |
| `[ebp-0x04]` | first_deleted_block | write_offset |
| `[ebp-0x08]` | write_offset | last_block |
| `[ebp-0x0c]` | last_block | first_deleted_block |
| `[ebp-0x10]` | result | result |
| `[ebp-0x14]` | number_of_passes | number_of_passes |

All 32 bytes are in six blocks, five of which follow from one fact:

1. (+13 B) January tail-DUPLICATES `first_deleted_block = NULL; number_of_passes = 0;` into both arms of
   the `last_block` test, with a fresh `xor eax,eax` per arm (0x4d-0x52 and 0x59-0x5e). Ours merges them
   at the join and spends the entry zero from ESI (0x51/0x54).
2. (-7 B for January) that EAX zero is still live at the loop head, so `write_offset = 0` is
   `mov [ebp-8],eax` (0x7f); ours has no live zero and emits `mov [ebp-4],0` plus a reload.
3. (+6 B) January's two `continue` edges are `jmp 0x166` (rel32) to the bottom `while (!result)` test;
   ours are jump-threaded to the loop head (`jmp 0x57`, rel8).
4. (+10 B) January keeps a third epilogue for the `break`; ours cross-jumps it into the shared bottom
   test, re-using ZF from `test cx,cx`.
5. entry rotation (January's arms `jmp 0x66`, skipping the head reload) and the early return
   (`mov eax,esi` vs `xor eax,eax`) follow from 1 and 2.

## Shapes (3, all scratch `--source`, siblings 13/13 EXACT in each)

| id | shape | evidence | result |
| --- | --- | --- | --- |
| S1 | ternary -> explicit `if (last_block) next_block = last_block->next; else next_block = NULL;` | law A25 (JOIN-STORE, w3-dated, postdates every lra ledger) | 368/4, **byte-identical** |
| S2 | early exit `return NULL;` -> `return result;` | January 0x17c is `mov eax,esi`, a copy of the result zero | 368/4, **byte-identical** |
| S3 | the two initialisations written in BOTH arms (diagnostic; R2-class, never intended to land) | direct test of fact 1 | 368/4 and WORSE: VC7 HOISTED the common stores above the branch (0x48/0x4b) and rotated the slot roles |

S3 is the decisive result. A25 predicts exactly what was observed: a store at the end of both arms is
merged and hoisted; a store after the join stays at the join. **No source placement of those two stores
can duplicate them into the arms.** The only form that ever reached 400 bytes
(`while ((_ReadWriteBarrier(), !result))`, 2026-08-30 ledger) forced the duplication from outside the
source language. The family is closed from the source side, not merely unexhausted.

`branch_sweep` (9 blobs) confirms no donor: the best historical row is `exact 4 residual 8 unwritten 2`,
and the only `sizedelta 0` blob is the banned barrier form at `exact 1 residual 7 unwritten 6`.

## Disposition

`NonMatching`, 13/14, park retained, no credit claimed.

Reopen criterion — criteria 1-3 of the 2026-08-30 ledger stand unchanged. Criterion 4 is NARROWED:
a candidate must act on the ZERO'S LIVE RANGE (make VC7 materialise the initialisation zero separately
in each arm and keep it live into the loop head), not on statement position; statement position is now
measured inert in both directions. Do not repeat S1, S2 or S3.
