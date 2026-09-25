# Scout frame packing: predictive mechanism, no source promotion

Baseline `012f7283230d13be8427ef7e93f73f3ef686111f`; production files unchanged. The two fixed R1 controls remain policy-held. This packet explains the **four-byte 0x98 to 0x94 frame reduction**, rather than claiming a new exact function.

The cause is a reference-count ranking threshold, not a changed storage/address-escape class. Scalar versus aggregate projection changes computed `vehicle_left` from six to eight actual machine-memory references. That crosses the unchanged seven-reference `vehicle_horizontal_forward`, reverses their order in C2's equal-size frame list, and changes which existing group the latter expands.

## Direct observations

The frame record is 0x60 bytes. These meanings are established only at the late frame-allocation stage; fields are reused by earlier passes.

| Field | Observation |
|---|---|
| +0x00 | Source descriptor pointer; descriptor +0x10 points back |
| +0x04 | Storage/analysis flags; computed-left is identical in both controls |
| +0x10 | Type/size encoding, 0x500c in both |
| +0x20 | Size, 12 in both |
| +0x24 | Final stack displacement |
| +0x30 / +0x34 | Next / previous links in size/count ordering |
| +0x38 | Accumulated stack-symbol reference weight, including address formation such as LEA (all observed additions are one) |
| +0x3c | Assigned index in the ordered frame/interference table; not a source declaration ordinal |

At construction, computed-left flags are `0x4`, descriptor words +0x20/+0x30/+0x34/+0x38/+0x54 are identical, and size is12. At pre-packing ranking, flags are `0x20204` in both. At final listing they are `0x22204`. The same symbol is joined by frame pointer **and descriptor ordinal**, avoiding arena reuse from an adjacent function.

| Symbol | Scalar count/rank | Aggregate count/rank | Scalar slot | Aggregate slot |
|---|---:|---:|---:|---:|
| applied_alpha (8B) | 8 /23 | 8 /23 | -72 | -84 |
| computed vehicle_left (12B) | 6 /34 | 8 /33 | -88 | -76 |
| vehicle_horizontal_forward (12B) | 7 /33 | 7 /34 | -76 | -76 |
| desired_velocity (12B) | 2 /36 | 2 /36 | -88 | -76 |

Full two-node interference sets for all four symbols are identical after rank-to-symbol normalization. Alpha interferes with computed-left, but not horizontal-forward. Computed-left and horizontal-forward do not interfere. This rules out a changed interference relation among these four as the cause.

### Exact source-to-memory-reference mapping

The two fixed full-TU sources differ only in the computed-left projection:

```c
vehicle_left_horizontal.i = vehicle_left.i;
vehicle_left_horizontal.j = vehicle_left.j;
/* versus held aggregate projection */
vehicle_left_horizontal = *(real_vector2d const *)&vehicle_left;
```

Scalar computed-left references: stores +349 (`fst`), +35e (`fst`), +36d (`fstp`), then later reads +58e/+59c/+5a7 (`fld`). The horizontal assignments use retained x87 values, not additional left-memory loads: total six.

Aggregate references: stores +34b/+364/+379 (`fstp`), integer reloads +34e (`mov edx,[left]`) and +369 (`mov eax,[left+4]`), then later reads +592/+5a0/+5ab (`fld`): total eight. Dynamic entry107595c5 observes exactly six/eight unit-weight additions, matching these listing references.

### C2 sequence and packing decision

- `107595c5`: add frame/reference weight. Insertion at107595f1 sorts ascending size (+0x20). Existing entries at10759641 add weight;1075964d compares predecessor count and10759652 preserves the equal-size boundary.10759657 walks backward;10735ac4 relinks the entry.
- `10759534`: iterate the resulting list;10759590 stores index at+0x3c.
- `1076ae00`: pack in that list order. `1076b3ba` scans existing groups backward, requires new size at most twice group size, and rejects interference before joining/expanding. `1076b628` grows a compatible group by the exact size difference.
- Scalar: alpha creates group9 at8B; horizontal-forward arrives before left and expands group9 to12B. Left then creates group13 at12B; desired-velocity joins13. Combined groups9+13=24B.
- Aggregate: alpha still creates group9 at8B; left arrives first and creates group13 at12B. Horizontal-forward joins13, followed by desired-velocity. Combined groups9+13=20B.
- `1076b0d4` stores the descriptor displacement; `10754e73` propagates it to frame records.

Thus the rank crossing, with unchanged interference, predicts the four-byte reduction. No forced register/memory writes were used. `c2_sort_pack.asm.txt` contains bounded static disassembly of the observed routines, with the C2 hash in its header.

## Independent prediction and negative control

`control.c` is a small independent C model, with a typed nested `pair` member, no casts, and no game source/header dependencies. Every local component is initialized before use; the contract is a valid six-element float input and ordinary external consumers. The aggregate variant changes only two scalar projection assignments into `horizontal = left.xy`.

Mechanism prediction: left count4 becomes6, crossing unchanged forward5; left/forward ranking swaps; forward joins left's12B group instead of expanding alpha's8B group; frame32B becomes28B. All predictions were observed. This is same-session model validation, not a preregistered or blind holdout: no separate timestamped prediction record is preserved. Storage words remain identical. In this toy, left's scalar weight4 consists of three stores plus one address-forming LEA for the consumer; aggregate weight6 adds two integer reloads. Forward's five consists of three stores, one LEA and one load. The field counts stack-symbol references, not solely data reads/writes.

The original toy packing history was inferred from final slots plus the measured sorter. The later generic-CLI smoke runs now directly capture both toy histories: scalar forward joins alpha's group while left creates another; aggregate forward joins left's group while alpha remains separate. `generic_cli_smoke.json` and the generic-CLI test preserve this direct confirmation.

Negative control: make only the toy `use2` declaration const-correct (`pair const *`). This leaves the memory-use shape unchanged. Counts, ordering, slots, normalized function bytes and relocation records are exactly unchanged from the aggregate control. The normalized function SHA256 is `5e6cd74c0902bf9c9ec3df433506227b4db07b090337b8c643cd784e262c6e0e` (192B padded, five relocations). These are compiler-mechanism controls, not proposed game edits.

## Reproduce

From repository root, using Python with `capstone` and `pefile` installed:

```powershell
python research/astra_breakthroughs_20260925_r2/scout/frame_trace.py
python research/astra_breakthroughs_20260925_r2/scout/construction_trace.py
python research/astra_breakthroughs_20260925_r2/scout/watch_frame.py
python research/astra_breakthroughs_20260925_r2/scout/priority_trace.py
python research/astra_breakthroughs_20260925_r2/scout/packing_trace.py
python research/astra_breakthroughs_20260925_r2/scout/independent_control.py
python research/astra_breakthroughs_20260925_r2/scout/export_results.py
python research/astra_breakthroughs_20260925_r2/scout/test_mechanism.py
```

Fixed sources reconstruct in memory from checked-in R1 patches and the unchanged production source, with strict source hashes. No donor worktree or old scratch source is needed. The pinned debugger binary location, compiler hashes, and dependency hashes are in `input_manifest.json`; the debugger must exist at its documented location. Each experiment gets a separate copied debugger executable/configuration under `scratch/astra_scout_r2_20260925/`. The only bridge change is an inclusive two-marker gate range; construction precedes the usual backend marker, so a one-marker gate would capture the wrong function's construction.

Every observed object is replayed with the exact stock CL command recorded in that experiment's `stock_check.json`. Full byte equality is required after masking only COFF timestamp bytes4..7. Eighteen core receipts plus two generic-CLI smoke receipts are embedded in `measurements.json`, including all independent controls. Binaries, full raw logs and listings stay in scratch. Raw-log hashes are preserved in `trace_hashes.json`; packet observations and control patches are text only. Watchpoints become stale after arena release; only the record lifetime through its final displacement is evidence. The debugger prints one hexadecimal digit for a hit, so creation hit36 appears as4 in its DWP message; configuration still arms on decimal36.

### Reuse for another frame-gap function

The observation-only CLI has one source/unit/function/output interface. It captures memory-reference additions, size/count order, packing choices and final named slots in one gated run, followed by exact-command stock replay. It does not expose FORCE, flag overrides, source transformations, or promotion. Output is restricted to repository `scratch/`; choose a unique private directory per concurrent run. An exclusive `.frame_trace.lock` rejects simultaneous use of the same directory; after a crash, inspect whether the process is still active before manually clearing a stale lock. Source files inside the output directory are rejected. Duplicate frame identities, noncontiguous ranks, or an addition-sum/count disagreement fail closed instead of publishing a potentially mis-gated report.

```powershell
python research/astra_breakthroughs_20260925_r2/scout/trace_frame.py --source scratch/astra_scout_r2_20260925/control_scalar.c --unit source/units/vehicles --function _frame_control --output scratch/astra_scout_r2_20260925/generic_control_scalar
```

`--unit` selects the existing unit's canonical flags from build.ninja. `--function` is the exact COFF name. Results go to `frame_report.json`, with source/compiler/debugger/log hashes, stock receipt, count sums, ordered frame records, group choices and final displacements. Repeat the example with `control_aggregate.c` and `generic_control_aggregate` for the second smoke cell. `generic_cli_smoke.json` preserves both successful independent-control smoke runs. This generic interface does not trace early construction or decode arbitrary interference graphs; use the specialized scripts only when those extra questions are needed. Counts are frame-allocation-stage stack-symbol reference weights, including LEA, not a count of source identifier spellings or solely data reads/writes.

## Holds and next falsifiable work

No change to production/source/header/compiler configuration; no helper expansion, exactness credit, or promotion. Existing view and aggregate restrictions remain unchanged. The held aggregate scout still has its previously documented +628 `fld`/`fmul` operand-order residual; this packet does not solve that separate arithmetic operand-identity issue.

The predictive mechanism can evaluate a **source-evidenced** proposal: count actual computed-left memory references, compare with horizontal-forward's seven, then observe whether equal-size order crosses and interference stays fixed. It does not justify adding artificial reads, arbitrary casts, or perturbing declarations to force a count. A future admissible source shape needs independent source evidence and all existing gates. Prior R1 Ghidra /Od export remains the source-topology evidence; no live Ghidra endpoint was available for this pass, and the existing IDA Free startup blocker remains. The new backend claim rests on bounded pinned-C2 disassembly and timestamp-only stock-checked live traces.
