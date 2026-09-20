> Research-only: zero new exact credit; no production changes.

# Wave 5 shared source-context study

**Subsequent test disposition:** the single proposed coherent sound type test was run by the root and failed its first falsifier. The exact owner changes `mov esi,dword [ebp+0x10]` to `mov si,word [ebp+0x10]`, shifting both call relocations. Both source files were restored and no consumer was probed. See `scratch/astra-wave5/sound-type/independent-review.md` and `.json`. The proposal below is preserved as pre-test reasoning; it is now rejected, not pending or landable.

**Result:** no supported general header, type, or call-boundary law explains the high-match pool. One narrow, falsifiable owner-type test is identified below. It concerns 1,564 meaningful bytes in two sound callers, predicts at most one 205-byte complete closure, and has not been compiled. **No exact gain or coverage progress is claimed.** All wave 4 candidates remain research-only.

Scope: read-only study of current January/base objects, current atlas, wave 3 accepted ledger, earlier research/rejections, and fresh HCEA raw parameter records. No production source/header/config edits, Ninja, commits, new compiler shapes, declaration sweeps, flag experiments, or ABI-incompatible local prototypes were used. Worktree baseline is `7a6ae71e614638753d04d6d04acbec56c7b8f636`.

## Current high-match pool, recomputed

`inventory.py` subtracts every `(unit,function)` in `scratch/astra-wave3/semantic-after.json` from the atlas and uses `wave3_after.objdiff_percent`. The result is **106 unresolved rows / 137,665 meaningful bytes at >=95%**. Thus it does not accidentally recount accepted Alert/Obey rows or use the old 142 KB estimate as current gain.

Fresh direct-call relocation census against current `build/base` and original `build/split`:

| Group | Functions | Meaningful bytes |
| --- | ---: | ---: |
| Identical relocated direct-call multiset | 104 | 131,184 |
| Missing known point/subtract helper calls | 1 | 5,165 |
| Extra recursive-call emission | 1 | 1,316 |
| Total | 106 | 137,665 |

There were zero read/disassembly errors. Each counted call is an actual decoded CALL with an attached i386 REL32 relocation; complete identities/addends are retained. This is a call-boundary detector, not a strict equality test. Indirect calls, call order/arguments, and allocation can still differ. Equal multisets alone do not prove correct source, but they reject a broad *missing/extra helper-call* explanation for 95.3% of these bytes.

The two exceptions are named and already explained by narrower evidence:

- `rasterizer_xbox_models::__rasterizer_model_draw`, **5,165 bytes**, lacks `_subtract_vectors3d` and `_point_from_line3d`. This is the existing point owner/header veto. Prior COMDAT census, exact copies, forced-inline rejection, and the repeated out-of-line trial are not new reopening evidence. No probe is proposed.
- `damage::_area_of_effect_cause_damage_to_object`, **1,316 bytes**, emits two recursive internal calls instead of one; every external call count agrees. This is not a shared header helper mismatch. No new flow shape is proposed.

## Prior comprehensive rejections respected

Read the current `astra_90pct_rejected_hypotheses_20260920.md`, both September 16 compiler-law ledgers and the superseding `opus5_next150_research_campaign_20260916.md`, the September 19 real-math/point ownership evidence, current context packet, and relevant sound, HS runtime, collision BSP and models histories. The following remain excluded:

- The historical 82 KB inline family was mostly call-consistent. The new current census strengthens that limitation; it does not resurrect the stale cached detector board.
- First-copy COMDAT selection is not a universal law. Existing point copies do not authorize adding another emitted owner.
- Missing January locals/headers cannot be recovered by relabelling later flattened records as original scopes or declaration order.
- Generic 8/12-byte copy schedules, declaration numbering, bare scopes, unused locals, or argument spelling are not source reconstruction.
- The short-header/long-definition sound mismatch previously proposed is not admissible and is not the test below.
- Commutative/x87 scheduling belongs to the parallel alias worker's study. No operand-order analysis or probe was duplicated here.

## Private parameter/source-order hypothesis: no shared effect established

The wave 4 decal evidence showed an authentic private source order `projection, geometry` even though January's optimized ABI puts geometry in EAX. Its complete caller/callee correction was byte-and-relocation inert across every owner. Therefore physical register promotion is not evidence that a source parameter came first, and correcting a later-authenticated source order is not itself a coverage lever.

Fresh HCEA records here corroborate the existing parameter kinds and order of:

| Residual | Meaningful bytes | Fresh record versus current source |
| --- | ---: | --- |
| `_bsp3d_test_sphere_recursive` | 792 | context pointer, long index; void result |
| `_bsp3d_test_pill_recursive` | 1,491 | context pointer, long index; boolean result |
| `_hs_evaluate_begin_random` | 519 | short function index, long thread index, boolean initialize |

These **2,802 bytes** have no newly identified parameter-width/order defect. The BSP context type names differ between revisions, which does not by itself prove a layout or source-context mismatch. The HS residual is already bounded to four register-choice bytes after the same two sign extensions and same parameter-home stores; the fresh signature does not reopen that shape.

## One supported narrow type test: coherent sound owner

Fresh `DIA2Dump -sym sound_definition_find_pitch_range_by_pitch` identifies a named third parameter **`short old_range_index`** in the HCEA procedure record at RVA `0x17f8ac0`. The current true owner header and definition both use `long pitch_range_index`; every read in the definition is explicitly truncated to short.

January supplies complementary evidence:

- `_sound_set_definition_end` at `+0x3c/+0x3e` emits `xor eax,eax; mov ax,[sound+0x8e]` before the API call at `+0x55`.
- `_update_channel_for_looping_sound` carries the same pitch-range call-width distinction. Historical measured short-header probes removed this shared hunk.
- The original callee loads a dword from its third argument slot into ESI at `+0x9`, then tests SI, sign-extends SI, or returns AX from SI. No observed upper bits are part of its source computation. The 32-bit load does **not independently prove a long source type**; it also does not prove a short declaration.

The important distinction from the rejected historical trial is **coherent owner type**. The prior waveC experiment explicitly compiled a short prototype against the existing long definition and proposed only the header change. That mismatch remains rejected. The history reviewed here does not record a test changing the real declaration and definition to short together, keeping all body/call expressions unchanged.

Affected residuals are **`_sound_set_definition_end` 205 meaningful bytes (91.884%)** and **`_update_channel_for_looping_sound` 1,359 bytes (99.198%)**, total **1,564**. Only the latter is part of the >=95 census. This is not a 1,564-byte closure forecast: looping has an independent private-call zero-extension difference described below.

**The single falsifiable test proposal, not run:** in an isolated header/source shadow, change only the third parameter from long to short in `sound_definitions.h` and the true `sound_definitions.c` definition together. Keep all casts, call expressions, body control flow and other declarations unchanged. First compile the real owner. If the currently exact 253-byte `_sound_definition_find_pitch_range_by_pitch` changes at all, stop; do not tune its body or try an incompatible prototype. If it remains strict exact, compile the genuine consumers and test the prediction that `_sound_set_definition_end` becomes strict exact. Require a full owner/data/COMMON audit and all inherited exact controls before any admission. No source patch or production recommendation exists yet.

The HCEA type is from a later PPC build, not recovered January source. Its corroboration plus the original caller/callee width behavior makes this a falsifiable small test, not an authenticated global type migration. If either the exact owner or complete consumer controls reject it, the correct result is no supported change.

## Exact counterexamples and required controls

Fresh current strict comparisons pass for the following relevant functions:

| Control | Meaningful bytes | What it controls |
| --- | ---: | --- |
| `_sound_definition_find_pitch_range_by_pitch` | 253 | Real owner; must stay exact under a coherent type correction |
| `_update_potentially_audible_looping_sound` | 472 | Existing exact API caller passing the `NONE` sentinel |
| `_sound_new_impulse` | 970 | Another exact API caller; rules out an indiscriminate caller rewrite |
| `_channel_get_state` | 234 | Genuine short private parameter with current exact callee |
| `_refresh_sounds` | 543 | Exact caller of the same private short channel API |

Fresh HCEA independently confirms `static short channel_get_state(short)`, matching the current declaration and definition. Nevertheless January's looping-sound call has `xor eax,eax; mov ax,[sound+0x8c]`, while the current caller lacks that XOR. Thus **a short parameter is not sufficient to explain every zero-extension difference**. Narrowing the pitch API cannot be advertised as a universal law or assumed to close looping's remaining 1,359 bytes.

The real `sound_definitions.h` has **18 direct C consumers and zero header consumers**, freshly enumerated; therefore no additional transitive C consumer arises through a header include. The current objects contain **656 strict exact function controls** across these units (not claimed as post-change preservation). `sound-context.json` lists every consumer and exact function. A shared-header test must preserve all of them, full noncode relocations/owners and COMMON, as well as the unchanged point-owner guard. Header declaration-count effects are not permission to accept unrelated drift.

## Deliverables and disposition

- `inventory.py`, `inventory.json`, `inventory.stdout.txt`: current high-match census and per-function direct-call identities.
- `collect_types.py`, `hcea_types.json`, `hcea_types.stdout.txt`, seven raw HCEA procedure dumps.
- `sound_context.py`, `sound-context.json`, `sound-context.stdout.txt`: owner/caller disassemblies, exact counterexamples, all 18 header consumers and 656 current strict controls, and the single conditional test.
- `*-base.asm`, `*-target.asm`: January/current source call/type evidence for all seven selected sound functions.

`IDA_AUTOMATION_UNAVAILABLE` remains recorded; the user authorized available primary tooling. No inferred original source revision, lexical scope, or complete local inventory is claimed. The generic remaining allocation/scheduling gaps are still per-function unknown context; they have not been converted into a transferable compiler law. **Zero shapes compiled and zero new exact bytes.**
