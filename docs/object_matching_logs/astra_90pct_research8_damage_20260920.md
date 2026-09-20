> Research only: zero admission credit; no retained source/header/configuration change.

# Packet 8: damage recursive graph and genuine collision helper

**UNLANDED, zero gain.** One new, primary-backed source shape was compiled. It preserved all 32 inherited exact functions but did not close `_area_of_effect_cause_damage_to_object`. The shape reproduces the existing September 14 best result exactly and emits a new, target-absent 80-byte behavioral helper COMDAT. No second supported shape remains. Production source, shared headers, base object, configuration and parked status are unchanged; no Ninja, full tests, commits or admission changes were run by this lane.

The frozen baseline is `47d4b0d4a2e7887026d213e73ba7073bfa30ac56`, unit `source/objects/damage`. The atlas assigns the target 1,316 meaningful bytes and 1,328 padded bytes, with 95.47282% ordinary objdiff similarity. Those are a possible closure ceiling, not earned credit. The packet does not claim original January source recovery.

## Existing boundary checked first

The relevant prior records are `damage_obj_area_of_effect_cluster_20260904.md`, `damage_obj_opus5_100k_waveD_20260914.md`, `damage_obj_opus5_150k_w2_20260914.md`, `damage_obj_opus5_150k_w3_20260914.md`, the damage park entry, the packet 5 shared-context call census, and the earlier exact sibling/owner records. The prior wave D result already explained the second recursive call: address-taking locals that escape to an out-of-line call must leave scope before VC7 can convert the final self-call into the January parameter-store/backedge sequence. A1 branch scoping reproduced it. It is not a newly discovered compiler law.

The prior s1 common post-merge `PIN` also reproduced the January clamp block layout. More than 40 earlier microforms plus the later dvar/km diagnostics exhausted declaration order, operand order, branch inversions, direct and ternary clamps, double constants, quotient locals and artificial aggregate consumers. Their remaining boundary was the dead x87 radius-delta pop. That boundary remains closed here. No scalarized aggregate, filler, extra arithmetic, volatile, assembly, private header facade or compiler-flag probe was attempted.

`prepare_helper.py` additionally searched all 57 C files available beneath the two named September 14 damage scratch directories. None uses `collision_test_line`. This is the bounded evidence for the one new helper-form test; it is not a claim that every unavailable historical artifact was searched.

## Fresh supplied primary evidence

`primary.json`, `raw-proof.json`, `ghidra_0076c230.asm` and the focused raw dumps preserve the evidence. The supplied cache PE is `halo_cache_symbols.exe`, SHA-256 `740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`. Actual Ghidra opened the existing project read-only and exported the function at `0x76c230`; it closed without saving. The branch and helper conclusions were also checked directly against raw PE bytes, independently of the decompiler text.

- Child recursion is a real call at `0x76c90e`, followed by infection bookkeeping. Sibling recursion is a separate final call at `0x76c9ad`. January has the child call at `+0x4cb` but parameter stores and `jmp +0x10` at `+0x4fe..+0x505` for the sibling case. This supports ordinary recursive source and the already known scope-sensitive optimization; it does not authenticate a hand-written backedge.
- The four-ray first segment calls vector collision at `0x76c4b8`. The two center-directed tests call the same line helper at `0x76c50b` and `0x76c558`. Thunk `0x40556f` is a raw `jmp 0x432880`.
- Raw body `0x432880` builds a local vector using thunk `0x4025e0 -> 0x42e3a0`, then calls vector collision through `0x4033ff -> 0x7a78e0`. The vector helper subtracts all three point components and stores all three output components. The line helper forwards flags, point zero, the completed vector, the ignore index and the collision result without additional behavior. Its RTC descriptor at `0x432900` records one 12-byte `vector` at EBP-20. This matches the genuine existing inline in `source/physics/collisions.h`; no declaration or header modification was needed.
- AOE RTC at `0x76c9e4` records seven aggregates: three 12-byte axes, an 80-byte `collision`, 12-byte `offset` and `offset_point`, and a second 80-byte `collision`. There are no AOE-owned offset/direct helper vectors in this later build. Fresh HCEA DIA also reports seven corresponding aggregate locals, including two `collision` records. These corroborate the helper-owned temporary graph. Flattened DIA records alone do not establish exact lexical block boundaries.
- The raw falloff graph at `0x76c77f..0x76c866` computes distance and radius delta, selects the linear result or one, then applies a common clamp. It corroborates the already measured s1 form and supplies no new explanation for the January x87 pop choice.

Fresh HCEA symbol and line records are in `hcea-sym.txt` and `hcea-lines.txt`. PDB SHA-256 is `0fd11b084fd2169cafe071fd8c3e4083d6e9564f01e7964f6c26e5a03eee4dff`; the named function has RVA `0x16b4680`, extent `0x634`, and source MD5 `1581c5bb15f0a4ede255f96a98702d92`. The independent reviewer additionally queried `collision_test_line`: its five parameter types, local `real_vector3d vector`, and inline attributes agree with the genuine header; line records identify `physics/collisions.h:178..184`, MD5 `8e973bf002e27a508e76340f72c1b8b1`. Those exports and independent raw/object checks are in `../damage-independent/`. The cache helper's exact symbol name is inferred from the raw operation/ABI correspondence and this independent named API, not recovered from a matching cache PDB. January, the cache PE and HCEA differ in compiler and layout; their same-source-revision identity is unverified. Later static linkage was not imported into the January C owner.

## Single source shape and result

`helper.c` is built from frozen `baseline.c` by `prepare_helper.py`. Only the target body changes. The new mechanism replaces the two manual `vector_from_points3d` plus `collision_test_vector` expansions with the existing canonical `collision_test_line`, deleting the now unneeded helper vectors. Previously tested A1 branch-local aggregate lifetimes and s1 common post-merge `PIN` are held as known controls, corroborated by the new raw graph. They are not counted as new hypotheses or extra compiled shapes.

| Object | Padded bytes | Relocations | Normalized SHA-256 prefix |
| --- | ---: | ---: | --- |
| January target | 1,328 | 51 | `d3f929d4ce73b3cd` |
| Current production baseline | 1,312 | 52 | `46952a116c873e6f` |
| New helper shape | 1,312 | 51 | `93606db11e3fdb8f` |
| September 14 best scratch | 1,312 | 51 | `93606db11e3fdb8f` |

The new function is strictly equal, including relocation identities and offsets, to the archived `park_rebase_candidate.obj` function. Its call relocation multiset equals January, with one internal child call, but full strict equality still fails. January at `+0x42e` emits `fxch st(1); fstp st(0)`; the candidate emits `fstp st(1)`. This two-byte difference shifts the rest of the function and jump table across the section padding boundary. Full unfiltered alignment is retained in `helper-alignment.txt`; its symbol-versus-defined-data presentation differences are not new alias hypotheses.

`helper-review.json` records the complete frozen comparison:

- Gate: 32 exact, 2 residual, 0 unwritten; every inherited exact sibling remains exact. `object_damage_body` is unchanged, and AOE is the only changed inherited code owner.
- Owner census: January 34, baseline 42, candidate 43. The sole added owner is `_collision_test_line`, 80 bytes / 1 relocation, SHA-256 `ca7acee0998d390837fc21d0a7a58241afcb65a4195116dfc3fb511af05e9c12`. No candidate code relocation references that helper; both calls inline. It is a real existing header implementation, but a behavioral target-absent emission nonetheless. No ownership exception or whole-object admission is proposed.
- All 38 runtime non-code sections / 1,364 bytes match the frozen baseline. All three COMMON definitions / 84 bytes match. No inherited code owner is lost. `_point_from_line3d` is not emitted. Baseline and candidate fake scans both have zero findings.
- Candidate operations initialize the helper vector before use and preserve the existing result and branch semantics. No new undefined behavior or uninitialized read is introduced. No ownership, alias, normalization or comparator rule was changed.

The lane stops after one source compile. The genuine helper hypothesis is now a negative control: authentic helper ownership and the correct recursive graph are insufficient to select the January x87 instruction sequence. Reopen only with a new independently authenticated real source mechanism for that dead-value pop, or original January local/type/compiler evidence that directly distinguishes the remaining choice. Do not repeat A1/s1, inline/manual helper substitution, the exhausted scalar spellings, or target-byte rewriting. Gain remains **0 functions / 0 meaningful bytes / 0 padded bytes**.

## Reproduction

Run from the worktree root. Preparation, raw review and object review write only this scratch packet. The gate writes its normal PID temporary source/object under `scratch/` and preserves the base object.

```text
python scratch/astra-wave8/damage/verify_primary.py
python scratch/astra-wave8/damage/prepare_helper.py
python tools/campaign/gate.py source/objects/damage --source scratch/astra-wave8/damage/helper.c --all --out scratch/astra-wave8/damage/helper.obj --forbid-emitted-symbol _point_from_line3d
python scratch/astra-wave8/damage/review_helper.py
```

The recorded invocation is in `helper.gate.txt`; frozen source/object and January object are included. These commands document reproducibility, not an instruction to rerun an exhausted source shape without new evidence.
