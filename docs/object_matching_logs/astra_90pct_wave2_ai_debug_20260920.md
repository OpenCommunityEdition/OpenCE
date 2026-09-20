# AI debug: two source graphs recovered, strict residual retained

Baseline is `905e5e3b41591af98c82accf3f57fc38118c2e7f`. Work was confined to scratch candidates and this report. No production source, headers, build configuration, comparison metadata, or Ninja state was changed by this worker. Four evidence-based shapes were compiled; the fifth allowance was not used.

The review candidate is `scratch/astra-ai-debug-five-point-vision.c`, with an exact source patch in `scratch/astra-ai-debug-five-point-vision.patch` and independent-comparison inputs in `scratch/astra-ai-debug-five-point-vision-review.json`. It restores a separately stored activation-marker vertex and the vision-cone zero-vector/accumulation graph. Both mechanisms have direct January machine-code evidence corroborated by fresh Ghidra extraction. It remains **zero credit**, subject to parent review: 57 exact functions and three residuals; `_ai_debug_render_actor` now has January's 24,976-byte padded extent and 1,905 relocations, but its normalized SHA is different.

## Evidence authority and prior hypotheses

Read `ai_debug_obj.md`, including its initial frame-recovery claims and its later vision-cone/register-allocation notes, plus `ai_debug_obj_opus5_150k_w2_20260914.md` and `ai_debug_obj_opus5_next150_n2_20260915.md`. Direct search found no ai_debug entry in `config/parked.json` or `config/semantic_matches.json`. Already exhausted local-declaration permutations, generic short scopes, point-arithmetic spelling changes, string-table ordering, and speculative register-pressure edits were not repeated.

January COFF is the admission authority. The same session's actual Ghidra execution produced `scratch/astra-debug-evidence/ghidra_0049c470.asm`, its decompile, and 94 protected RTC local records in `ghidra_rtc_locals.tsv`. The analyzed PE is the 2020 symbol executable documented in `astra_90pct_debug_packet_20260920.md`. Its same-source-revision claim is unauthenticated; a different compiler/build timestamp alone does not disprove identical source. RTC protected aggregate names and byte extents do not supply complete scalar declaration order or lexical block boundaries. HCEA lacks this ai_debug function, and January's stripped compiland provides no local/type/line records.

The fresh January disassembly used here is `scratch/astra-ai-debug-jan.disasm.txt`. It was generated directly from the carved target with the existing COFF resolver. All offsets below are relative to `_ai_debug_render_actor` unless prefixed with an eight-digit PE address.

## The fifth marker point is present in January

The old ledger's initial claim that January has no fifth point is contradicted by its instructions. At `+0x1140..+0x115a`, January reads base point components from `ebp-0x18`, `ebp-0x14`, and `ebp-0x10` and copies them into the distinct 12-byte aggregate at `ebp-0x114..ebp-0x10c`. Its address is passed at `+0x11b7` and `+0x1264` for the two marker edges incident on the bottom vertex.

Those same three stack slots later receive `" ack "`, `"u/orph"`, and `"i/orph"` pointer relocations at `+0x5984`, `+0x598e`, and `+0x5998`. This is disjoint lifetime reuse. The later string table cannot establish that the earlier copied point was absent. Matching the overall frame size after deleting the point was therefore insufficient evidence for that deletion.

Fresh RTC and actual /Od instructions recover the five-point graph in the combined-PVS branch:

| Name | RTC ordinal / frame offset | Role and direct evidence |
|---|---|---|
| `p0` | 19 / -776 | Copy of `base_point`, bottom vertex; PE `0049dbf9..0049dc17` |
| `p2` | 21 / -816 | `base_point + up * 0.2`; helper call `0049dc3f` |
| `mid_point` | 18 / -756 | `base_point + up * 0.1`; helper call `0049dc69` |
| `p1` | 20 / -796 | `mid_point + offset_vector * 0.1`; helper call `0049dc94` |
| `p3` | 22 / -836 | `mid_point - offset_vector * 0.1`; helper call `0049dcbf` |

All five RTC extents are 12 bytes. The four line calls at `0049dcde`, `0049dcfd`, `0049dd1a`, and `0049dd3b` traverse `p0->p1->p2->p3->p0`. This distinguishes the actual center named `mid_point` from the bottom copy: the old reconstruction had used the name `mid_point` for the bottom copy.

The selected patch restores that copy and the authenticated vertex graph while retaining the current branch's arithmetic expressions and existing point-helper call. It does not introduce unrelated helper calls or force an allocator result. A separate scratch variant spelling all four computations as point-helper calls was measured and excluded from the selected patch.

## The missing symbolic relocation identifies the vision-cone graph

After the five-point correction, the only net missing symbolic destination in the relocation census was `_global_zero_vector3d`. January loads it at `+0x55d0` (relocation `+0x55d2`), reads its three components at `+0x55dc`, `+0x55fc`, and `+0x55ff`, and stores them into the indexed direction vector at `+0x55f2`, `+0x5602`, and `+0x5605`. The following x87 instructions repeatedly add basis contributions to the stored vector.

This is corroborated independently by actual /Od code. RTC ordinal 86 is `direction_vector`, 48 bytes at frame offset -4984, and ordinal 89 is `headspace_vector`, 12 bytes at -5056. At `004a3648..004a365a`, /Od copies all three components of the global zero vector into `direction_vector[side_index][ring_index]`. It then calls the same point-from-line implementation in place at `004a36b0`, `004a370c`, and `004a3768`, in this order:

1. `looking_vector * headspace_vector.i`;
2. `looking_left_vector * headspace_vector.j`;
3. `looking_up_vector * headspace_vector.k`.

The baseline instead directly assigned three summed products. That removed the global-zero copy and the stored intermediate accumulation graph. The selected fourth shape restores the initialized vector and the three in-place calls. The casts only adapt the existing `real_vector3d` storage to the existing `point_from_line3d` API. Every in-place call reads initialized storage. No helper, type, alias, header, or global was added.

The old ledger's discussion around line 913 described a historical source version that already had the global-zero load. Its conclusion about a load moving across a loop head cannot apply to the current baseline, where the relocation is absent entirely.

`scratch/astra-ai-debug-relocation-audit.json` supplies sequence alignment of symbolic relocations with neighboring anchors. It excludes internal branch/table offsets for this specific source-operation diagnostic; the strict gate still checks the complete section. Reordered symbolic loads remain visible and are not counted as source omissions merely because SequenceMatcher emits an insertion/deletion pair.

## Results and ownership

All shapes were independently based on the baseline except the fourth, which combines the accepted evidence from the five-point shape with the newly authenticated vision graph. MAX and the alternative marker helper spelling are excluded from the review patch.

| Shape | Actor padded bytes | Relocations | Frame | Strict unit result |
|---|---:|---:|---:|---|
| January | 24976 | 1905 | 0x810 | target |
| Baseline | 24848 | 1903 | 0x810 | 57 exact / 3 residual |
| 1: blockage MAX expression | 24832 | 1903 | 0x810 | 57 / 3; rejected |
| 2: five-point marker | 24912 | 1904 | 0x81c | 57 / 3 |
| 3: five-point marker with all helper calls | 24864 | 1904 | 0x81c | 57 / 3; excluded |
| 4: five-point marker plus vision accumulation | 24976 | 1905 | 0x81c | 57 / 3; source correction for review |

The MAX hypothesis came from fresh /Od `0049c909..0049c938`, where two signed shorts merge into a temporary before assignment. January's original corresponding local block was already equivalent apart from spills. Its measured global effect did not justify retaining it.

The selected candidate normalized SHA is `62c77e81f1dd7d33fc379ad770a951c9fd64b10265094c2ab503b5e183fd5f57`; January is `c236780ab71e93cb5f2a2270894caa3b17f2242c8a7009d8277d67ce35434bd7`.

The candidate changes only `_ai_debug_render_actor` among all baseline code owners. Added and removed owner sets are empty. All 57 exact siblings remain exact; the other residual owner sections are also identical to baseline. The inherited `_point_from_line3d` owner remains exactly 48 bytes/zero relocations with SHA `9b763841f8519177ac2d3fd471226972b9fa92cf1e9061e9f92359077dc82741`. COMMON inventories, noncode symbol definitions, and all production data section information are unchanged. Of 424 noncode sections, only `.debug$S` changes: five PID digits in the S_OBJNAME path (`_gate_46800.obj` versus `_gate_34536.obj`). The review JSON preserves its complete bytes and differing byte offsets 138..142.

## Remaining boundary and reusable mapping

Unmodified `tools/campaign/alndiff.py` yields 6,869 target instructions versus 6,829 candidate instructions, including decoded table data. It aligns 4,050 normalized instructions as equal and emits 1,474 changed blocks containing 2,819 target and 2,779 candidate instructions. These counts retain frame offsets and relocation annotations; relocation-site offsets and external-versus-defined rendering inflate them. They are diagnostic counts, not a match percentage. The first actual difference is `sub esp,0x810` versus `sub esp,0x81c` at `+3`. The candidate's first table begins at 24,588 versus January's 24,576 despite equal padded section extent.

The first remaining nontrivial register-lifetime region is the avoidance-ray color block around `+0x59d`; January keeps the color in a register where the candidate spills it. A known x87 lifetime difference remains at `+0x1046`: January retains a multiplied component with `fst`, while the candidate uses `fstp` and reloads. Neither observation authenticates a new declaration, scalar width, or lexical scope. No fifth shape was tried.

`scratch/astra-ai-debug-rtc-observed-ranges.json` and `.tsv` map all 94 RTC records to first/last observed **direct EBP-relative references**, with full matched instructions in JSON. They are lower-bound observation ranges, not complete lifetimes: indexed accesses, pointer propagation, callee uses, and lexical scope are not reconstructed. In particular, the later build's three `tempbuf` records are 80 bytes, whereas current source has three 72-byte buffers. No independent January extent evidence was established, so those array sizes were left untouched. A matching frame size or later-build extent alone is insufficient.

Reproduction commands are in `scratch/astra-ai-debug-shapes.py`, `astra-ai-debug-five-point-review.py`, `astra-ai-debug-relocation-audit.py`, and `astra-ai-debug-final-alignment.py`. The review gate is:

```text
python -B tools/campaign/gate.py source/ai/ai_debug --source scratch/astra-ai-debug-five-point-vision.c --all --out scratch/astra-ai-debug-five-point-vision.obj
```

No exact credit is requested for these corrections. They remove two source-graph omissions using January plus direct debug evidence and correct the historical frame inference without pretending that the remaining allocation/scheduling differences are solved.
