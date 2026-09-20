> Research-only: zero new exact credit; no production changes.

# Structure lens flares: authenticated marker type test, no exact gain

Baseline `7a6ae71e614638753d04d6d04acbec56c7b8f636`. One supported scratch source shape; **zero exact functions, zero meaningful/padded bytes gained, UNLANDED**. No production/header/config change, Ninja, commit, or compiler-flag sweep. The 4,331-meaningful-byte parked function remains 99.29274%.

## Result and preservation

`named-members.c` replaces only this TU's existing `structure_lens_flare_marker.direction[3]` with `char i_direction`, `j_direction`, `k_direction`, and changes the six constant-index accesses to those fields. The independently stored temporary-marker prefix is unchanged. This preserves 16-byte marker layout, char signedness, offsets C/D/E, initialization, arithmetic, access order, and all call boundaries; it adds no helper or declaration filler.

| Measurement | January | Frozen production | Named-member shape |
|---|---:|---:|---:|
| Padded function size | 4,336 | 4,336 | 4,336 |
| Relocations | 156 | 156 | 156 |
| Decoded instructions | 1,327 | 1,327 | 1,327 |
| Normalized SHA-256 prefix | `40aa8e486e52fe25` | `57d6f033f3793735` | `57d6f033f3793735` |
| Strict exact siblings | — | 3 | 3 |

The entire object preserves all **18 emitted function owners**, including all inherited shared math helpers, not merely the four report rows. No owner changed or appeared. `_point_from_line3d` remains absent, COMMON remains empty, and the three exact controls remain `_compare_temp_markers` (32/0), `_cluster_index_from_point` (96/3), and `_structure_lens_flares_place` (16/2).

All 27 inherited runtime data sections / 1,206 raw bytes and the 42-byte `.drectve` remain identical. Thus the comparison of 28 nondebug noncode sections / 1,248 bytes **includes linker directives**; those 42 bytes are not runtime data. All 17 January-owned string sections / 1,110 raw bytes match by their actual owners and strict section comparator. No data credit is added. The scratch fake scan reports zero review leads.

The hypothesis was falsifiable: changing genuine array member accesses to independently authenticated scalar member accesses might change the compiler's expression-node context without introducing synthetic dataflow. It did not change a single code owner. This rules out that representation lever in this TU at the current context; it does not establish a general compiler law.

Receipts: `named-members.json`, `named-members.gate.txt`, `named-members.patch`, `named-members.fake.txt`, and independent read-only `review.json`. `named_members.py` builds exactly this single shape; `evidence_review.py` reviews existing artifacts without compiling. All paths in this report are relative to this directory unless stated otherwise.

## Fresh primary evidence

`collect_primary.py` freshly queried HCEA `HCEX_Release.pdb`, SHA-256 `0fd11b084fd2169cafe071fd8c3e4083d6e9564f01e7964f6c26e5a03eee4dff`:

- `structure_lens_flare_marker` has `real_point3d position` at 0, signed `char i_direction/j_direction/k_direction` at C/D/E, and unsigned-char `lens_flare_index` at F. Exact record: `hcea-structure_lens_flare_marker-type.txt`.
- `build_structure_lens_flares` is only an **8-byte retail stub**, with the BSP pointer parameter, no locals, and line records 63/489/490. It cannot supply January scheduling or lexical-scope evidence. `temporary_lens_flare_marker` type lookup returned no record. Empty lookup is a limitation, not proof that the type never existed.
- The same named-field declaration already exists in `source/rasterizer/rasterizer_lights.c`. Its consuming `_rasterizer_lens_flare_submit_for_cluster` is **residual 400/18, not an exact control**. The old rasterizer w2 c4 reverse change to `char direction[3]` was byte-inert versus c1. That independent negative control and this fresh lens result agree; the donor supplies type corroboration, not an exact scheduling model.

Five 2020 first-party PE builds each supply 19 source/error-string references to one full body: tool `0xc0c8d0`, guerilla `0xd01690`, sapien `0xe95330`, halo_tag `0x8d3c40`, halo_cache `0x8a3e80`. Input hashes and references are in `primary.json`; initial function discovery was a prologue heuristic. The halo_cache body was then independently opened read-only in actual Ghidra, with successful decompilation recorded in `ghidra.log` and full listing `ghidra_008a3e80.asm`. Its hash is `740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`.

Raw RTC descriptor `0x8a5960`, reached by the epilogue call at `0x8a593c`, proves these 11 address-protected objects:

| RTC name | EBP offset | Bytes |
|---|---:|---:|
| geometry | -104 | 36 |
| plane | -144 | 16 |
| s_plane | -204 | 16 |
| t_plane | -228 | 16 |
| centroid | -248 | 12 |
| real_bounds | -272 | 16 |
| bounds | -288 | 8 |
| position | -340 | 12 |
| position_2d_test | -356 | 8 |
| point | -404 | 12 |
| offset | -424 | 12 |

`rtc.json` and `review.json` independently decode that table. It contains address-protected records, not an exhaustive lexical local list, so the absence of `s_temp` is not proof of January scope. Names alone do not justify renaming/reordering January locals to perturb allocation.

Raw instructions confirm signed byte loads at `0x8a5453`, `0x8a5471`, and `0x8a548f` from marker C/D/E. The full body's direct-call operands resolve through E9 thunks to the following operation graph: vector subtraction `0x8a4a41`, normalization `0x8a4a50`, cross product `0x8a4a6f`, two plane-from-point-and-normal calls `0x8a4a8c/0x8a4aa9`, plane distances, and in-place point-from-line operations `0x8a5173/0x8a51bc/0x8a551e`. `primary-excerpts.asm`, the full listing, and `helper-*.asm` preserve raw proof, including the helper's real normal copy and the three multiply-add component operations. Names assigned to unnamed helper addresses are semantic interpretations of those bodies, not recovered linker symbols.

This corroborates already-tested helper topology. The later /Od build uses SSE scalar arithmetic and stack-check scaffolding; it cannot authenticate VC7 x87 creation-order tie breaks. No candidate was made from Ghidra's inferred C alone.

## Prior rejection boundary and remaining blocker

Read all six canonical lens ledgers: grid topology 20260826; policy reconciliation 20260829; Fable uniform boundary 20260830; authenticated fuzzy closeout 20260901; opus5 w2 20260914; and opus5 w3c 20260915, plus the current full parked record. The September 15 ledger records 19 historical source blobs, 14 noncompiling, no superior strict result; the earlier Fable census records 903 distinct objects / 1,562 worktrees with no strict exact body. These are historical ledgers, not fresh re-executions here.

Fresh `baseline.aln.txt` and `target.asm` reproduce the same remaining 13 code-only aligned difference blocks: N1 k-store/forwarding at +A65; s-plane/normal sum ordering near +B05/+B3D/+BB0; derived alignment padding; and the integer/FP transpositions near +E28/+E6A. The full strict listing additionally displays symbol-owner annotations and shifted relocation locations; the relaxed 13-block decomposition is for explanation only and is never used for admission.

The previous near-exact point-helper path changes global compiler context while emitting forbidden `_point_from_line3d`; even that known path leaves the two integer/FP swaps. Its marker-update local bytes were already proven identical to the scalar expansion. Fresh later helper calls therefore do not justify another COMDAT-free respelling lottery. No scopes, declaration order, operand order, whole-plane storage, grid helper, compiler flag, or synthetic dependency were swept here.

The historical w2 claim that the d-only plane form is prohibited by a representation cast was explicitly superseded by w3c; that form is admitted but byte-inert by itself. It is not a new opening. Nor is strict exact mathematically proven impossible: the evidence rules out the tried mechanisms, not every possible original source context.

**Stop / reopen:** no further supported shape remains in this packet. Keep the park and current production body. Reopen for authoritative January source/local context or a genuine same-compiler donor that supplies a concrete, testable lever for both remaining integer/FP swaps and preserves ownership. Repeating the historical point-helper route additionally requires an explicit ownership-policy change; none was made or requested. No shared-header proposal is warranted by a byte-inert unlanded representation change.
