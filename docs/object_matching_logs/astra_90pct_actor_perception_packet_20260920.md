# Astra90% wave2: actor_perception_update source-context packet (2026-09-20)

## Result and scope

Research at isolated lane HEAD `905e5e3b`, owning only scratch candidates for `source/ai/actor_perception.c`. No production source/header/config/park was edited; no Ninja, commit or push was run. Read the September14 w1/w3 ledgers, current two actor_perception parks, fresh HCEA locals/line records, and the September20 Ghidra/RTC packet before probing. `IDA_AUTOMATION_UNAVAILABLE`; the user's authorization to proceed without IDA automation applies.

**No exact gain.** A gate-ready two-change source correction removes the complete danger-timer block-layout mismatch and complete tail index/register mismatch. `_actor_perception_update` remains **4720 bytes /170 relocations /1402 instructions**, and all35 prior strict-exact siblings remain unchanged. Only this function's code owner changes; the two parked siblings are unchanged. The final candidate is `scratch/astra-context-perception-authenticated.json`, with a compact `.diff` and standalone `-body.c` for review. Normalized SHA-256: `76d282007a76de5f8f3c00ba3158d4b76271c7a46c34ba7db7363ad8e9b927d5`.

The residual is now confined to one call-return schedule and three floating-point windows. Treat this as zero-credit structural reconstruction, not an exact admission. The decision to land the correction belongs to the orchestrator.

## Primary evidence and its limits

January target: `build/split/source/ai/actor_perception.obj`, `_actor_perception_update`. Fresh complete January/base instruction streams are saved in `scratch/astra-context-perception-{split,base}.disasm.txt`.

The authenticated cache executable is `halo_cache_symbols.exe`, SHA-256 `740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`, at the source-build research location documented in `astra_90pct_context_packet_20260920.md`. Its corresponding unoptimized function is0x00475F00, identified by assertion/source references and characteristic perception state-machine topology. Full raw Ghidra disassembly and actual decompile are in `scratch/astra-debug-evidence/ghidra_00475f00.{asm,c}`. The decompile is reconstructed C, not original source text; the raw instructions and RTC records are the primary witnesses. The executable is a2020/v140 build, and same-source-revision is unverified. Each proposed local/control distinction also corresponds narrowly to January.

Fresh HCEA PDB records are `scratch/astra-debug-evidence/hcea_actor_perception_update_{sym,lines}.txt`. They name `long interesting_orphan_index`, `short highest_prop_timer`, `prop_iterator iterator`, `boolean became_acknowledged_from_orphan`, two `actor_position_data sense_position` locals, and `ai_information_data information`. Both sense_position records share frame0xA0. Do not interpret DIA's x86 register labels for the PPC binary literally, or infer exact braces from flattened local records.

## Authenticated correction1: one interesting-orphan index

The inherited reconstruction introduces `nearest_orphan_index` initialized at function entry, writes it while selecting the nearest orphan, then copies it to a second `interesting_orphan_index` before fetching the current target prop.

The primary unoptimized body instead uses one scalar slot at EBP-0x2C throughout:

- 0x00475F60: initialize it to NONE;
- 0x00477A1C: write the current iterator index after finding a closer orphan;
- 0x00477CC0: overwrite it with NONE when the current target is an orphan;
- 0x00477D58 /0x00477D5B: load that same slot and store `actor->meta.interesting_orphan_index`.

There is no pre-tail index copy. The HCEA typed name `interesting_orphan_index` independently supplies the original identity. The corrected source keeps one local under that name and removes the invented copy. This is not an arbitrary declaration-position experiment.

A single isolated probe removes every January tail mismatch from function+0x1183 through the return, without affecting earlier differences. The reconstructed extra local had forced EDI to carry the selected index across the target-prop call; the original single local allows January's ECX phi and tail stores naturally.

## Authenticated correction2: repeated danger predicates

The inherited inner acknowledgement-timer branch tests only the uncertain-combat timer. The unoptimized body explicitly re-tests two earlier facts before that timer test:

- 0x00476063..0x0047606F: `danger_zone.hostility == 0`;
- 0x00476071..0x00476084: `!danger_zone.attached_to_us`;
- 0x00476086..0x00476096: uncertain timer is NONE or at least60.

The outer chain already implies the first two conditions and there are no intervening mutations. Their redundancy is original source topology, supported directly by the unoptimized instructions, not invented solely to alter optimization. Restoring these two predicates makes January's entire timer branch layout exact at+0xC3..+0x105. September14's polarity-only rewrite was inert; this is a distinct newly authenticated source distinction.

Four previously accepted independent branch controls remain strict exact in the current build: `_profile_frame_get_value`, `_render_ui_widgets`, `_render_ui_widgets_postgame`, `_encounter_test_rule`. These support preserving independently authenticated repeated source tests/stores; they do not authorize guessing redundant conditions elsewhere.

## Five bounded compiler shapes

All shapes were gated through unchanged flags with the point-from-line emitted-symbol guard and full TU sibling listing. No sixth shape was attempted.

| Shape | Target-function result | Symmetric differing instruction rows | Exact siblings |
| --- | --- | ---: | ---: |
| Baseline (reference) |4720/170 residual|62|35|
|1. Separate authentic sense_position scopes/names|byte-and-relocation identical to baseline|62|35|
|2. One interesting_orphan_index|tail exact|32|35|
|3. Restore two original inner danger tests|timer block exact|56|35|
|4. Genuine distance_squared2d call on 3D prefixes|target function byte-and-relocation identical to baseline|62|35|
|5. Combine shapes2+3 only|4720/170 residual|26|35|

The row-count metric uses full alignment of relocation-normalized instructions, discarding relocation annotation text and normalizing branch destination numbers, then sums both sides of non-equal spans. It is a diagnostic count, not strict-byte coverage or proof. Full exact metadata is `scratch/astra-context-perception-shape-results.json`.

The scope hypothesis had good independent local evidence: RTC gives separate56-byte `sense_position` records at EBP-0x94 and EBP-0x554, and HCEA names them identically with one reused frame slot. Moving the first local into the existing non-dormant block and retaining the second in its existing orphan arm is byte-inert. It is excluded from the proposed patch.

The genuine helper probe is also independently supported: call0x00476E77 goes to thunk0x00405CF9, which jumps to0x00443EA0. That function has RTC vector `v` size8 and calls the two-point vector construction then vector magnitude square. It operates on the x/y prefixes of the two3D positions, matching January semantics and existing admitted2D-prefix cast precedent. Direct `distance_squared2d((real_point2d const *)&last, (real_point2d const *)&body)` does not fix the x87 order and adds header COMDATs `_distance_squared2d` and `_vector_from_points2d`. It is excluded from the proposed patch; no helper/header change is recommended.

## Preservation audit

Final candidate:35 exact /9 residual /0 unwritten of44 listed. `_point_from_line3d` emitted-symbol guard passes. Fake scan:zero review leads. All35 existing exact functions are section-and-relocation identical to baseline. All57 other function sections, including residuals and both parks, are unchanged.

Function-owner inventory is58/58, with no additions or drops. Raw symbol-owner count is179/179; three compiler-generated `$L` label names are renumbered after removing the source local. No meaningful owner is added/dropped, and there is no changed noncode section. Only `_actor_perception_update` changes among function owners. Raw census and four strict controls are `scratch/astra-context-perception-review.json`.

## Concrete remaining blockers

1. **Call-return schedule at+0x6A3:** January adds8 to ESP, then moves EAX to EDI; current code moves EAX to EDI first. The function/local types already agree with the available source evidence. A declaration split or type change solely to move these instructions is not justified.
2. **Squared2D result at+0x96A..+0x970:** four x87 stack-register choices square the x/y components in the opposite order. The proper header helper compiles identically to the inherited macro, confirming a compiler/helper scheduling residual rather than demonstrating a new source spelling.
3. **Two inlined desire-prop argument bindings:**+0x998..+0x9BE and+0xA81..+0xAA1 interleave the squared-distance x87 temporary with integer argument/actor-data loads differently. The unoptimized debug body confirms the same13 arguments, their types, and the squared-distance expression; it does not establish a different valid statement boundary. September14's argument-binding experiments remain exhausted.

Reopen for a new authenticated helper definition, original January call-site/type context, or a natural same-compiler donor that explains these specific remaining instructions. Do not repeat local permutations, scope changes, generic flag pressure, or scalar/helper spellings already shown inert. Until such evidence arrives, keep strict credit at zero.

## Reproduction artifacts

`scratch/astra-context-perception-{sense-scope,one-index,danger-tests,real-distance2d,authenticated}.{json,obj,gate.txt,aln.txt}`; matching standalone bodies; final `.diff`/`.fake-scan.txt`; base/split disassemblies; `perception-distance2d-primary.disasm.txt`; shape-results and preservation-review JSON. Source and global build state remain unchanged by this worker.
