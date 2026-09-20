> Archived research proposal: NOT LANDED; zero exact gain. Production remains at wave3.

# Wave 4 physics evidence packet — 2026-09-20

Baseline: `7a6ae71e614638753d04d6d04acbec56c7b8f636`. Scope: `source/physics/physics.c`, specifically `_physics_update_old`. All work is scratch-only. No source/header/config/Ninja changes or commits were made. No exact credit is claimed.

## Result

One newly identified source lifetime boundary is authenticated by January and fresh raw debug instructions: **copy `object->object.angular_velocity` into `rotation_axis` after `object_translate` returns**. Current source and the earlier rejected `uo_v1.c`/`uo1.c` decoded rewrites copy it before both location lookup and translation. This is a cross-call snapshot boundary, not a declaration permutation.

The independent candidate is `physics/axis-after-translate.c`, with patch and `axis-after-translate.review.json`. It keeps the existing declarations and moves only the initialized value capture. All 13 exact controls, all other residual functions, code owners, 42 non-debug data sections, and COMMON inventory remain unchanged. `_point_from_line3d` is not emitted. The function remains residual at 5456 bytes / 118 relocations against January 5168 / 115.

A second narrow mechanism is directly authenticated: the per-point force begins by adding normal force to the already zero-initialized force, then separately adds ground friction. Current source replaces those two operations with `force = normal_force + ground_friction`. This mechanism appeared in the old broad decoded rewrite; this packet adds first-hand raw debug confirmation and an isolated measurement, not a claim that the old shape is new. The isolated correction and combined candidate are 5504 / 118. They preserve the same controls/owners but do not improve the strict match. Root may review either source correction independently; neither is an exact-match promotion.

## Primary receipts

`physics/physics-primary-review.json` contains file hashes, 19 freshly parsed RTC local records, raw PE instruction bytes, January instruction/relocation spans, ownership comparisons, and candidate checks. `physics/review_physics.py` reproduces it.

Fresh actual Ghidra ran successfully against the existing read-only `halo_cache_symbols.exe` program (`0x007be340`), producing `physics/ghidra_007be340.c`, `.asm`, `physics_source_xrefs.tsv`, and `ghidra.log`. `physics/AstraPhysicsLaunch.java` and `physics/collect.py` are the reproducible launch path. The domain object was read-only; no transaction or save was used. The PE SHA256 is `740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`.

Fresh DIA2Dump produced `physics/hcea_physics_update_old_sym.txt` and `_lines.txt`: the five-parameter signature agrees, 15 optimized typed locals remain, and 116 line records identify `d:\projects\code\hcex\sources\physics\physics.c`, MD5 `6C056038F02FCFABC73A081C66242F6A`. HCEA identifies `axis` as `union real_vector3d`; the PE RTC record independently identifies `axis`, size 12, EBP-692. The HCEA artifact is Xbox 360 PPC: the dumper's x86-like register labels must not be interpreted as x86 allocation. Its repeated stack offsets show reuse, not a full January lexical declaration order.

The later PE's compiler/build differs from January. That alone does not disprove source identity; the same-revision source claim is **unauthenticated**. January instructions independently authenticate each proposed mechanism. RTC lists protected aggregates, not every scalar or complete lexical scope; no absent RTC entry was treated as proof that a scalar did not exist.

## Axis snapshot boundary

January:

- `_object_translate` relocation at +0x104b belongs to the call at +0x104a.
- Only after that call, +0x104f/+0x1051/+0x1054 load the three angular-velocity components through EDI; +0x1057/+0x1060/+0x1066 store the local copy. The following code normalizes this copy.
- The existing baseline instead loads/snapshots components at +0x1122..+0x1147, before the location and translation calls (+0x1161 and +0x1175).
- The isolated candidate now calls translation at +0x1163 and performs the copy at +0x1168..+0x117f, preserving the target ordering.

Fresh raw PE:

- 0x7bfc72 calls thunk 0x407aa9, whose raw jump resolves to the Ghidra translation function 0x781fc0.
- 0x7bfc7a loads the object; 0x7bfc7d adds 0x48, the later build's angular-velocity offset. 0x7bfc80..0x7bfc94 copies three dwords into EBP-0x2b4/-0x2b0/-0x2ac.
- EBP-0x2b4 is exactly the fresh RTC `axis` record. 0x7bfca1 calls normalization with its address.

Every candidate path initializes the local before its first read. The translation call is an external state-changing boundary; this packet does not assume it leaves angular velocity untouched. Current reconstructed `object_translate` performs object disconnect/reconnect operations, so replacing the boundary with an optimizer-style equivalence claim would be unjustified.

## Force accumulation graph

January +0xb3d..+0xb6d reads and adds existing `force` components at offsets 0x118/0x11c/0x120 to `normal_force` 0x84/0x88/0x8c, storing force. +0xb73..+0xba3 then independently adds ground friction 0x90/0x94/0x98. Four further in-place force additions follow before the torque cross product. Current source starts with a single addition of normal and ground force and therefore has only five additions.

Fresh raw PE 0x7bf74d..0x7bf7a6 contains the two ordered three-argument in-place adds. The first call at 0x7bf773 pushes `(force, normal_force, force)`; the second at 0x7bf7a1 pushes `(force, ground_friction, force)`. Both use thunk 0x406339 -> 0x44e8e0; raw helper instructions confirm component-wise addition. This corroborates the decompiler without relying on its inferred argument names.

`memset(mass_points, 0, sizeof(*mass_points)*count)` precedes the loop in both source and target; restoring the read of existing force introduces no uninitialized read. The operations remain floating-point additions with the target's intermediate stores; they are not replaced by a mathematical associativity argument.

## Three bounded scratch measurements

| Shape | Size / relocs | Normalized SHA256 | Exact controls | Credit |
|---|---:|---|---:|---:|
| January | 5168 / 115 | `bee483283d8e9c3aa3c752e1a6097b5322604fc5c9379bf60528256d2dab08ed` | — | — |
| Baseline | 5456 / 118 | `8e46f2b6ccf73fb4cbebd7ec52451a52d7b8b86030de04bfd467a592126ddcb9` | 13 | 0 |
| Axis after translation | 5456 / 118 | `af8463f9ebda2c6b7c5ee556616333421fdea39d2eb062e58eb0692e211762aa` | 13 | 0 |
| Separate force additions | 5504 / 118 | `dac2b2aa56cb48929431106cc54aba62b382e4bf838b9c3c3b51f75b96ab62c9` | 13 | 0 |
| Combined | 5504 / 118 | `6a50206dcd7fc7e9c120ee41c8d3f6bbd7378a6aa7ab7be6e90a6fde6420bc38` | 13 | 0 |

All gates: 13 exact / 4 residual / 0 unwritten. Every candidate changes only `_physics_update_old`; no code owner is added or removed, no point owner is emitted, all 42 non-debug data sections compare equal, and COMMON remains empty. Full candidate source/object paths, SHA256 hashes, exact-control lists, data comparisons and gate files are in individual review JSONs.

`physics/*.aln.txt` uses the repository's unchanged alignment tool with stack offsets, relocation-site addresses and full fail-closed identities retained. January has 1621 instructions; baseline 1696, axis-only 1697, force-only 1698, combined 1699. Full alignment equal counts are 1035/1045/1043/1054 respectively; changed hunks are 310/311/308/309. These are diagnostics, never admission evidence.

`physics/baseline-relocations.json` retains full relocation records. A separately labelled symbolic-sequence locator finds three surplus zero-literal references and no missing external call identity; identical repeated literals make exact alignment-site attribution ambiguous. That locator strips section-owner differences for shared symbols and is **not** the strict normalized comparison. No stack-erased or index-zipped diagnostic is used for admission.

## History and remaining evidence gap

Read before trying shapes: the 100K, 150K w1 and w3 physics ledgers; their original worker notes and `uo_v1.c` / `w3_physics/c/uo1.c`; parks; Sept19 commit `37d51331` real_math.h correction and `real_math_obj_large_closeout_evidence_pass_20260919.md`.

The old broad rewrite reached 5184 / 115 with the now-landed conjunction-form axes predicate. Its dozens of source changes, static point surrogate, known x87 operand ties and speculative local inventories are not imported. Named water/air scale experiments, broad fused-point replacements, collision flag placement sweeps, invisible scalar local counts and header shadow permutations are exhausted and were not repeated.

The current source comment claiming 5280 / 114 and out-of-line axes validation is stale: Sept19's authentic header correction makes the actual baseline 5456 / 118 and fully inlines axes validation. Root should update that comment when integrating any reviewed correction. No current header defect was found here.

The first remaining source-relevant mismatch is already known: January calls `matrix4x3_from_point_and_vectors` before setting the four force/acceleration vectors; current source uses declaration initializers before that call. Fresh PE 0x7be400 followed by four setter calls at 0x7be438/461/48a/4b3 corroborates the prior decoded rewrite. That shape has already been exhausted; it was not retested. Numerous existing fused vector-operation and arithmetic-expression mismatches remain later. Fresh typed locals/RTC do not supply the complete missing scalar/local lifetime inventory needed to authenticate arbitrary declaration order, nor a new compiler law that closes the residual.

Stop after three shapes. No fourth/fifth shape is justified by new primary evidence in this packet. The cross-call aggregate snapshot timing is a reusable diagnostic distinction; it is not a general license to move aggregate declarations or copies for stack allocation.
