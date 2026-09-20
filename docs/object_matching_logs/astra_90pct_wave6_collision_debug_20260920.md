> Research evidence only; no source change or exact credit from this packet. Its measurements precede the separate vertex admission.

# Collision-debug packet 6 — closed without a new shape

At HEAD `f3daaff82197c861a9a6899f76b701969a2151a3`, `_collision_debug_render` remains residual. No new source shape, candidate, production/header/config change, Ninja run, or commit was made. Fresh evidence does not resolve the source and owner distinctions left by the earlier campaigns. Exact credit: **zero**.

## Fresh baseline and complete comparison

The frozen `baseline.c` is byte-identical to production. `baseline.obj` was scratch-compiled with the normal gate and `_point_from_line3d` emission forbidden. The gate passes that guard and reports:

| Measure | January | Current |
|---|---:|---:|
| Padded section bytes | 4,192 | 4,112 |
| Relocations | 278 | 275 |
| Instructions | 1,227 | 1,201 |
| Frame immediate | `0xb528` | `0xb52c` |

The priority atlas counts 4,182 meaningful target bytes; the gate measures the full padded section. Full normalized byte/relocation identity fails. The full aligned diagnostic retains operands and relocation identities: 478 aligned equal instructions and 223 changed hunks. This is a locator, not an admission score. The first raw code difference is the frame immediate at `+0x3`; saved-register allocation also diverges around `+0x8b`.

`baseline-comparison.json` records all function and noncode section identities. `evidence.json` records all 41 defined, named code/data owners, the sole function owner, all 31 non-debug noncode sections with full relocation identities, no COMMON, and no point helper. There are no inherited exact function siblings in this C unit. With no candidate or source mutation there is no changed owner/control set to admit; this is a baseline census, not a candidate PASS claim.

## History consumed before considering probes

The September 14 100K and 150K-w1 logs, September 15 250K-w3c log, August fixed-point/public-donor history, current context packet, and September 16 worker result were read. Relevant repository logs are `docs/object_matching_logs/collision_debug_obj_opus5_{100k_20260914,150k_w1_20260914,250k_w3c_20260915}.md`. The most recent bounded worker record is `C:/halo-worktrees/opus5-throughput-recovery-20260916/scratch/workers/w6_collision_debug.jsonl` (70 attempts).

Those investigations already covered genuine short row/column loops, separating the two loop-index scopes, copy-then-scale, phantom-point helper/caching shapes, surface sign tests, clipped-motion topology, assertion spelling, spray bounds, and three-array versus aggregate spray storage. The best historical source reached 4,192 bytes/278 relocations, but still had array relocation identity, frame-cell, and x87 differences. Equal size/count was never exactness. Its asymmetric camera point setter and `real t` local were not established as the January source. No repetition of those shapes is justified here.

## Fresh primary evidence and its limits

Actual headless Ghidra opened the existing project **read-only** and freshly exported `ghidra_007af800.asm` and `.c`. The source-string xrefs independently identify `physics\collision_debug.c` at this function. `collect.py`, Java launcher, arguments and logs preserve reproduction. Exact-name DIA symbol and line queries for the function and each of the three proposed spray-array names all returned zero-byte results, with successful exit codes. Empty results establish no missing archive or lexical record; no broader symbol extraction was attempted. `IDA_AUTOMATION_UNAVAILABLE` remains the approved limitation.

`evidence.py` independently rereads the supplied PE, follows helper thunks, disassembles the critical raw intervals, and reparses the RTC descriptor at `0x7b1ee4`, referenced from the epilogue at `0x7b1ec2`. All 26 name/offset/extent records agree with the earlier RTC census. The supplied PE SHA-256 is `740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`.

* At `0x7b0613..0x7b0651`, both camera position and forward are copied through direct integer loads/stores. There is no point-setter call at that site. This supplies no fresh primary reason for the historical asymmetric point-setter shape. It does not prove whether the source used aggregate or component assignments.
* At `0x7b065e..0x7b06c5`, the local point and vector are copied first, then the vector is scaled in place. The raw call thunk `0x404d77` resolves to `0x42e220`; its body verifies the three-component scale operation. This corroborates an already tested mechanism, not a new probe.
* The phantom hit at `0x7b0780` calls thunk `0x409ab1`, resolving to the actual `point_from_line3d` body at `0x42e0d0`. Operand setup is point, vector, collision `t`, output point. The historical direct-helper candidate was already worse against January and introduced the forbidden owner. This later call is insufficient reason to repeat it or manually expand another helper.
* RTC identifies `textstring` as 2,048 bytes, at EBP-47,568; the raw formatting call also passes `0x800`. January calls formatting with `0x800` at `+0x914`, but the output address at `+0x90e` is EBP-`0x920`, while its earlier vector-result address at `+0x343` is EBP-`0x538`. The 1,000-byte separation plus the vector result's 1,048-byte extent fills exactly 2,048 bytes. The 44,040-byte features region ends at EBP-`0x920`. These offsets admit a 1,000-byte text array followed by a distinct vector result, or a 2,048-byte text array sharing storage with an expired vector result. Optimized stack offsets do not choose the lexical declaration. Neither the current overflow comment nor the later RTC record independently proves the January text-array extent; no resize or manufactured lifetime scope was probed.

The 2020 build/compiler identity prevents treating it as the January build. It alone does not disprove a same-source-revision claim. All use here is limited to directly observed operations, named aggregate extents and first-party paths; January instructions remain decisive. RTC contains protected aggregates, not a full scalar declaration/lifetime ordering.

## Exact next action / reopen criterion

There is no new admissible mechanism. Do not replay the old short-loop, scalar-order, helper, `t` carrier, asymmetric copy, or array-spelling sweeps. Reopen only with January-side helper/lifetime evidence that resolves the remaining frame/x87 differences, or a shared compiler/source rule that predicts those differences and survives exact controls. The three spray-region addends support separate storage regions but provide no authentic source names, and changing their synthetic configuration identities alone cannot close the body. No foreign owner-header proposal follows from this packet.

Reproduction from repository root:

```powershell
python tools/campaign/gate.py source/physics/collision_debug --source scratch/astra-wave6/collision-debug/baseline.c --all --forbid-emitted-symbol _point_from_line3d --out scratch/astra-wave6/collision-debug/baseline.obj
python scratch/astra-wave6/collision-debug/compare.py
python scratch/astra-wave6/collision-debug/collect.py
python scratch/astra-wave6/collision-debug/evidence.py
```

The collection uses the existing Ghidra project read-only; generated settings/classes/cache live only within this packet directory. No candidate JSON was produced because no source change qualified.
