# `observer.obj` opus5 150K wave w2 park re-investigation (2026-09-14)

## Scope and provenance

- Translation unit `source/camera/observer.c`; target `build/split/source/camera/observer.obj` (January 2002 Xbox
  debug build, VC7 13.00.9254).
- Targets: the two remaining parks, `_observer_update_command` and `_observer_update_positions`. Both are locked in
  `config/parked.json`, so all experiments were scratch copies (`scratch/workers/observer/`). The real file is
  unchanged (`git diff --stat -- source/camera/observer.c` is empty).
- Prior ledgers read in full: `observer_obj_opus5_100k_waveD_20260914.md` (with its worker notes and edit manifests in
  the 100K worktree), `observer_obj_replacement_exact_packet_20260912.md`,
  `observer_obj_real_update_caller_cluster_20260905.md`. `branch_sweep`: no branch blob is better than the current
  file.
- Evidence: January COFF (instructions, relocations), Ghidra 12.1.2 decompilation
  (`scratch/ghidra/out/observer.obj.decomp.c`), January map atlas (`scratch/atlas/observer.obj.txt`; both functions
  have exact records), HCEA `blam/camera/observer_update_command.c` / `observer_update_positions.c` and
  `headers/observer_parameter.h`, and the Stian build-2276 lift of `observer.c`.

## Baseline and final gate

`gate.py source/camera/observer --all --forbid-emitted-symbol _point_from_line3d`:

- Baseline: exact 24 / residual 2 / unwritten 0; guard pass.
- Final real-file gate: identical, row by row (`scratch/workers/observer/final.obj`).
- `scratch/parkcheck.py`: parks 2, drift 0.
- Fake scan: 0 leads.
- Owner census: unchanged, since the real file is byte-identical to HEAD.

No newly exact functions.

## `_observer_update_command`: not landed (exhausted)

| | padded | relocs |
| --- | ---: | ---: |
| January | 1040 | 33 |
| parked body | 1056 | 35 |
| wave D best (`ecmd1`), re-measured on the current file | 1040 | 33 (sha only, 34 differing instructions) |

**Mechanism.** January's loop keeps four loop variables:
- `edx` = the pending timer pointer (live across the assert, spilled to `[ebp-8]`);
- `edi` = the last-timer pointer;
- `esi` = the down-counter;
- `[ebp-4]` = the flags pointer.

VC7 on the candidate merges the timer induction variable into the last-timer pointer (`esi = timer0 - last0`,
addressing `[esi+edi]`) and homes the counter instead. The Ghidra body (decomp lines 656-677) shows the same three
pointer induction variables plus the counter.

**Precedent examined.** January's own `observer_find_displacement` keeps three same-step pointer induction variables
unmerged, but all three are live-out (the second loop uses them). `update_command` has no natural live-out use.

**Shapes this wave.**
- c1, the do-while loop form (the `find_displacement` idiom): inert, 34 differing instructions.

Wave D already spent the indexed/pointer permutations, `const`, and declaration/increment order.

**Classification.** Induction-variable merge / register tie after the structure was fixed.

**Reopen criterion.** A January-side reason for VC7 to keep the timer induction variable separate: a genuine live-out
use, a different loop form, or a same-compiler donor.

## `_observer_update_positions`: not landed (improved scratch body)

| | padded | relocs | differing instructions |
| --- | ---: | ---: | ---: |
| January | 1568 | 40 | - |
| parked body | 1280 | 28 | - |
| wave D best (`epos6`) | 1568 | 40 | 31 |
| **w2 best `scratch/workers/observer/pos_a3.c`** (in `a3.c`) | 1568 | 40 | **18** |

### New January-byte evidence (both applied in `pos_a3.c`)

1. **Pointer initialisation schedule.**
   - `lea ebx,[ebp-0x84]` (delta) is emitted after the velocity store `mov [ebp-0x2c],esi`.
   - So `real *delta = displacement.n;` is declared after `velocity`.
   - Measured: delta declared first puts the `lea` before that store; declared last puts it after all four stores; declared after `velocity` is EXACT.
   - The Stian lift lists the same order.
2. **Advance schedule.** In the pointer-advance block, `velocity += count` precedes `delta += count`. This made the advance block EXACT.

### Measured shapes

| shape | result |
| --- | --- |
| p1: delta declared last | 31 |
| p1vel: delta declared after velocity | 29 |
| a1/a2: target first, velocity early | 32 / 36 |
| a3: p1vel + velocity before delta | **18** |
| t1/t2: header `valid_real_vector3d_axes2` with pointer locals, with or without a polar loop | 1216/36; the helper stays out of line |
| t3-t7: polar-group loop tail with pointer locals or casts (the HCEA 360 topology) | 22-24 |

In t3-t7 the loop form fixes the dot-product sum order but flips the product operand ranks.

Probes only, not shapes:
- a TU-local one-expression axes helper: inlines, 20;
- `(real_vector3d *)position + 1`: inert.

### Remaining 18 instructions

These are x87 reassociation and operand-rank choices:
1. The inlined `normalize3d` magnitude inside the inlined rotational apply: January `k,i,j` (the common VC7 z,x,y idiom); ours `k,j,i`.
2. The axes-check dot product: January `k,i,j`; ours `k,j,i`.
3. The j/k components of the first `cross_product3d(up, forward, &left)`: January loads the forward field first; ours loads up first.

The t3 loop is the only spelling that changed a sum order. It has no January-side proof (no loop is visible, which is
consistent with VC7 unrolling a one-trip `for`), it needs the literal 8, and it is not closer. It is not proposed.

**Classification.** x87 reassociation/value-age tie after the structure was fixed.

**Reopen criterion.** A measured VC7 rule for the three-term sum rank (why January keeps z,x,y where this TU yields
z,y,x) together with the cross-product operand rank. Start from `pos_a3.c`, which is strictly closer than the parked
body and than wave D's `epos6`.

## Do-not-repeat (this wave)

- **update_command:** the do-while loop form.
- **positions:** delta declaration permutations (settled: after `velocity`); advance-order permutations (settled:
  velocity before delta); the header axes2 helper with pointer locals (stays out of line); polar-loop tails with
  pointer locals, reversed declarations, inline casts or `forward + 1`; `(real_vector3d *)position + 1` in the apply
  call.

## Proposals

- No park-reopen, header or config proposals.
- Park-update information only: `pos_a3.c` is a better-evidenced scratch baseline for the positions park. It is not
  exact and must not be credited.
