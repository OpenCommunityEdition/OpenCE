# Astra90% wave2: actor_looking source-context packet (2026-09-20)

## Result and ownership

Research owned `source/ai/actor_looking.c` only, through scratch candidates. The inherited unit has14 strict exact functions and two residuals. Five evidence-based shapes were tested; no production source, header, config, park, shared build or commit was changed by this worker. No Ninja or push was run. `IDA_AUTOMATION_UNAVAILABLE`; the user authorized continuing with available primary evidence.

**Zero new strict credit.** The final candidate corrects a recovered private parameter/array type and three independently authenticated source-order/control facts. `_actor_look_update` becomes4704 padded bytes versus January4720 (4714 meaningful), with159 relocations on each side. The full-section disassembler reports1439 instructions on each side, including embedded switch-table data interpreted as instructions; equal counts do not imply an exact function. The normalized SHA-256 remains different: candidate `4bc222ca36d696b3a8bfebfb48c402e06df583ac82cac6fbd3f79d305adc9f7d`.

All14 existing exact siblings are unchanged; `_actor_look_idle_find_prop` is unchanged. The candidate is suitable for an orchestrator review as an authentic zero-credit correction, not for exact admission. It is `scratch/astra-context-looking-authenticated.json`, with a review `.diff` and full scratch `.c`.

## Read-first evidence and provenance

Read the September14 waveC,150K w1/w3 reports and September19 `actor_looking_obj_evidence_closeout_20260919.md`. The latter already exhaustively searched139 unique focused candidates,12 history blobs and eight checkouts. It rejected equality recomputation because source records did not authenticate it. That specific gap is now closed by fresh unoptimized instructions; the broader allocation barrier is not.

January: `build/split/source/ai/actor_looking.obj`. The January PDB contributes a public name/extent but no function-local/type/scope records. Current complete target/base/candidate disassemblies are `scratch/astra-context-looking-{split,base,authenticated}.disasm.txt`.

Fresh primary cross-build function:0x0045F5D0 in the authenticated `halo_cache_symbols.exe`, SHA-256 `740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`. Source/assertion strings and full look-update control graph identify it. Read-only Ghidra disassembly/decompile: `scratch/astra-debug-evidence/ghidra_0045f5d0.{asm,c}`. The executable is2020/v140, so same-source-revision is unverified. Decompiled C is not recovered original text; raw instructions, RTC records and narrow January correspondence support the claims below.

The HCEA2011 PDB is authenticated at SHA-256 `0fd11b084fd2169cafe071fd8c3e4083d6e9564f01e7964f6c26e5a03eee4dff`. Fresh raw DIA records for the relevant private helper and two additional callers are saved in this worker's scratch files. PPC register spellings rendered as x86 names by DIA are not used as register-placement evidence.

## Newly authenticated private type closure

The inherited reconstruction treats look-angle limits as `real_vector2d looking_bounds`, passed to a `real_vector2d const *cone_limits` private parameter. Fresh raw PDB records establish the actual identities:

- `actor_look_valid_look_vector`: parameter **float *look_delta_deviations**;
- `actor_look_update`: local **float[2] look_delta_deviations**;
- `actor_looking_test_validity`: local **float[2] look_delta_deviations**;
- `actor_look_idle_find_prop`: local **float[2] look_delta_deviations**.

These are direct PDB type records, not the cached reverse-C file's inferred vector type. The actual raw helper record also confirms one leading float and three real_vector3d pointer parameters; there is no phantom leading integer argument.

The candidate changes the TU-private declaration and definition to `real *look_delta_deviations`, its threshold access to `look_delta_deviations[side]`, and all three local/caller sites to `real look_delta_deviations[2]`, indexed at0/1 and passed directly. No header or ABI convention changes, casts, local facades or extra helper owners are introduced. All accesses remain within the two-element array. The isolated type-closure gate keeps the helper and all14 exact siblings exact; only update's instruction scheduling changes slightly.

Raw dumps: `scratch/astra-context-looking-valid-look-hcea-sym.txt`, `scratch/astra-context-looking-actor_looking_test_validity-hcea-sym.txt`, and `scratch/astra-context-looking-actor_look_idle_find_prop-hcea-sym.txt`. Update's existing full raw record is `scratch/astra-debug-evidence/hcea_actor_look_update_sym.txt`.

## Repeated equality and statement-order witnesses

1. **Independent override comparisons.** At0x00460241, the unoptimized body sets `succeeded` FALSE. It computes `secondary_priority == 8` at0x00460248..0x00460267 and stores `change_facing`. It then computes the identical equality again at0x0046026D..0x0046028C into `face_exactly`. The canonical body had initialized `face_exactly` by copying `change_facing` and initialized `succeeded` afterwards. Restoring the two independent comparisons and the initial FALSE store is directly authenticated, despite the value-equivalent simplification. This addresses the September19 rejection's precise missing-evidence premise.

2. **Enabled-block initialization order.** Raw0x0045F89C..0x0045F8C6 stores facing_free, aiming_freeTRUE, looking_free from can_look, then facing_optional. It clears facing_locked and looking_locked at0x0045F8EA/0x0045F8EE before loading the two cosine values at0x0045F8F2..0x0045F90D. The corrected source follows that observed ordering. It fixes the earlier load-order mismatch at January+0x186. It does not guess alternate declaration positions.

3. **Clear the free/optional states before installing primary facing.** Raw0x0045FE76/0x0045FE7A clears those two states, then0x0045FE9F..0x0045FEB6 copies primary_vector, then performs the exact-facing merge. January's previous primary-binary packet independently recorded the same order. The candidate moves the existing two assignments ahead of the copy/merge; no extra stores are added.

Other later-build facts were deliberately not copied. The debug body contains a separate local with writes but no recovered uses, additional field/flag behavior, and a third initial TRUE flag. Those facts do not establish a necessary January variable or a valid new dependency. The source patch introduces none of them. The existing source scalar/vector/scope families from previous campaigns were not swept again.

## Bounded probe results

| Shape | Update padded bytes | Full-section instructions | Symmetric differing instruction rows | Existing exact siblings |
| --- | ---: | ---: | ---: | ---: |
| Baseline reference |4688|1425|432|14|
|1. Independent override comparisons; succeeded initialized first|4704|1439|424|14|
|2. Authentic free/locked/cosine statement order|4688|1425|422|14|
|3. Genuine float-array/private float-pointer type closure|4688|1425|430|14|
|4. Primary free/optional clears before copy/merge|4688|1425|432|14|
|5. Combine the four authenticated corrections|4704|1439|412|14|

Instruction-row scores align the entire relocation-normalized stream, discard relocation annotation text, normalize branch destination numbers, and sum both sides of non-equal spans. This is a diagnostic measure, not byte coverage or an admission rule. The modest score change must not be presented as4720 exact bytes. The primary-clear shape has the same score but a different normalized section; it is retained for source-order evidence, not to inflate a numeric score.

All five gates use unchanged compiler flags and the `_point_from_line3d` emitted-symbol guard. No sixth shape was tested. Full metadata is `scratch/astra-context-looking-shape-results.json`.

## Preservation and controls

Final whole-TU gate:14 exact /2 residual /0 unwritten. All32 other function sections are byte-and-relocation identical to baseline. Function-owner inventory33/33; raw defined symbol-owner inventory123/123, no added or dropped names. Four compiler-generated `$L` labels refer into the changed update section; they are not new ownership. No noncode section changes. Fake scan:zero review leads. Guard passes.

The same four independent branch controls were checked afresh and remain strict exact: `_profile_frame_get_value`, `_render_ui_widgets`, `_render_ui_widgets_postgame`, `_encounter_test_rule`. Preservation audit and exact-control metadata: `scratch/astra-context-looking-review.json`.

## Remaining concrete blocker

The first substantive residual now begins at January+0x1B1: facing_locked uses EBP-8 in January and EBP-7 in the candidate; aiming_cosine and primary_priority use opposite4-byte homes. January saves the secondary-direction aim-valid result to EBP-7 at+0x56B and keeps the desired-facing pointer in EBX. The candidate retains the result in BL and reloads/reassigns the pointer, driving the remaining register/stack cascade through secondary selection and stationary facing. The body and relocation count are close, but the gap is still16 padded bytes and strict normalized bytes disagree.

RTC's nine protected aggregates and HCEA's typed locals authenticate useful names and array types; they do not disclose a January address-taking use or source lifetime for `secondary_aim_valid`. Neither binary mandates inventing a pointer local, volatile flag, dead variable or redundant use to force that flag into memory. Further progress requires a specific additional original use/lifetime/type distinction or a measured compiler law that predicts this allocation. Repeating already searched scope/declaration/flag spellings is not justified.

The raw initial can_lookTRUE and later otherwise-unused flag are recorded only as cross-build observations, not an invitation to pressure the allocator. The two stationary2D-copy families were already tested in previous campaigns and are not reopened by having new names alone.

## Reproduction and disposition

Artifacts: `scratch/astra-context-looking-{independent-facing,initial-stores,typed-deviations,primary-clear,authenticated}.{json,obj,gate.txt,aln.txt}`, individual body/full-source files, final `.diff`, `.fake-scan.txt`, raw HCEA records, and preservation/shape JSON. No exact credit is claimed. The final patch is available for orchestrator review as a coherent, typed, source-authenticated zero-credit improvement.
