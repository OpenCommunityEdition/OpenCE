# Astra 90%: actor movement avoidance evidence packet, 2026-09-20

## Result

**Zero new exact credit; zero source shapes; no candidate JSON.** The fresh raw records corroborate the already diagnosed source-form gap. They do not establish an admissible new type, scope, aggregate, or compiler-context correction. Production `source/ai/actor_moving.c`, headers, ownership, configuration, parks, and shared build files were untouched. No compiler, Ninja, commit, or push was run for this packet.

The source matches HEAD `905e5e3b41591af98c82accf3f57fc38118c2e7f`, SHA-256 `25b3d9cc2a8881669349be0b774806a8824aea4baef4e821b3bc2bcefa2da4ff`. Other workers' changes are outside this packet. `IDA_AUTOMATION_UNAVAILABLE`; the user authorized proceeding with the available evidence.

The campaign atlas classifies `_actor_move_vector_avoidance` as **written-unparked**, with 4,130 meaningful bytes and 74.86895% diagnostic similarity. This is not a park reopening. Fresh read-only COFF inspection reproduces the historical residual exactly:

| Object | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| January | 4,144 | 135 | `a8a8010c11375e82d6773df6110a10efc53469969382f39cc4d5aa0fc91449bf` |
| Current | 4,192 | 140 | `1d410a08dd97f702e2bdb8374a5187cb5f5517ace856bd3d449f9dc56116890e` |

The unit has 36 January function owners, of which 31 currently compare strict exact. The current object has 59 function owners when generated header functions are included. There is no defined `_point_from_line3d` owner. This packet does not change any of these sets.

## Read-first history

Reviewed the fresh campaign atlas and the September 14 fresh-graphs, 100K wave C, 150K w1/w3, September 15 250K w3d and next-150K n4 packets, as well as current source and recent file history. The September 15 n4 verdict supersedes the earlier tentative description of the gap as an ordinary per-site transform-inline decision.

Already tested and excluded from repetition:

- Helper `__inline`/`__forceinline`, prototype, caller-size, caller storage class, argument-kind and same-TU emission-order variants. The exact private transform body does not reproduce the January per-site behavior under those experiments.
- Scalar expansion at the perpendicular site, anonymous macro explanations, and added component temporary/parenthesis forms. These were withheld for source authenticity; the best historical `va6` still had a frame/register residual at 4,128/135.
- The clear-first avoidance-weight branch, delayed obstruction/weight initialization, short adjacent-direction index locals, split PIN/MAX expressions, and cross-product helper forms. These are existing findings, not newly discovered levers.
- Enabling the inline `point_from_line3d` body by changing the current import mechanism. Previous probes introduced a forbidden owner and the present task explicitly requires point ownership to remain intact.

Relevant historical reports are `actor_moving_obj_opus5_100k_waveC_20260914.md`, `actor_moving_obj_opus5_150k_w1_20260914.md`, `actor_moving_obj_opus5_150k_w3_20260914.md`, `actor_moving_obj_opus5_250k_w3d_20260915.md`, and `actor_moving_obj_opus5_next150_n4_20260915.md` in this directory. No old scratch candidate was compiled or proposed for admission here.

## Fresh typed and protected-local evidence

The authenticated HCEA PDB SHA-256 is `0fd11b084fd2169cafe071fd8c3e4083d6e9564f01e7964f6c26e5a03eee4dff`. Its raw DIA function record gives a static void function with parameters `long`, `const real_vector3d *`, `real_vector3d *`, and `float *`. All four already agree with the current declaration. Important locals are:

| Raw HCEA identity | Raw type | Current source correspondence | Finding |
| --- | --- | --- | --- |
| avoidance_weights | float[8] | weights[8] | Type already agrees |
| avoid_ray_result | short[2] | avoidance_types[2] | Type already agrees |
| avoid_ray_t | float[2] | avoidance_t[2] | Type already agrees |
| ray_origin, twice | real_point3d | Two loop-local ray_origin objects | Already distinct scopes |
| ray_direction, twice | real_vector3d | Two loop-local ray_direction objects | Already distinct scopes |
| avoidance_data | vector_avoidance_data | Same named aggregate | No new aggregate-layout evidence |
| turn_direction_in_vector_space | real_vector3d | velocity_direction | Same value category and size |
| movement_vector / movement_in_vector_space | real_vector3d | movement_vector / local_movement_direction | Both already vectors |
| worldspace_best_direction | real_vector3d | best_direction | Already a separate branch-local vector |

The 2020 executable's independently parsed RTC descriptor at `0x0046B610`, passed at `0x0046B5E7`, records 20 protected locals. It confirms the 32-byte weight array, 4-byte hit array, 8-byte fraction array, two 12-byte point/vector pairs, 24,648-byte avoidance aggregate, and the named approximation/vector objects. It additionally records `rotation_axis` and `cross_vector`. The latter belongs to a later-build full cross-product calculation where the January source only needs the scalar sign expression; it is not evidence to add an unused vector to January.

Neither record set contains a `perpendicular` local. Absence from RTC alone would not prove absence of an optimized scalar or vector. Here the fresh unoptimized call arguments independently show the two basis components being consumed directly; no perpendicular aggregate is materialized in that branch.

HCEA frame reuse supports non-overlapping value lifetimes, but its compiland has no lexical-block records. It cannot authenticate arbitrary extra braces or a January stack-home assignment. DIA's x86-looking register spellings for this PPC build must not be interpreted as x86 register allocation. Renaming the current already-correct arrays alone would add no type evidence and was not tested.

Raw records: `scratch/astra-debug-evidence/hcea_actor_move_vector_avoidance_sym.txt` and `_lines.txt`. The line dump has 127 records with source MD5 `7337B93D8294F20972C35255330A4F8E`; it covers source lines 2175 through 2692. Lines 2642 and 2643 identify adjacent operations in the final avoidance branch. These records provide original line boundaries, not original C text or January build identity.

## Direct cross-build call evidence at the missing transform

The primary executable SHA-256 is `740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`. Its timestamp/path authenticate a 2020/v140 context; same-source-revision with January remains unverified. The correspondence of entry `0x00469CE0` is supported by its distinctive control graph, actor_moving assertion strings, and the independent RTC/HCEA local identities. Raw executable instructions are authority for this build; the Ghidra decompile is an interpretation.

Fresh raw decoding establishes:

1. `0x0046B40C..0x0046B41F` loads the zero-vector pointer and copies its three words to the rotation vector at EBP-0x1C.
2. `0x0046B422..0x0046B450` passes destination rotation, scalar `-avoidance_directions[best].k`, the left basis at EBP-0x6050, and rotation as the input accumulator. The call at `0x0046B450` targets `0x00409AB1`.
3. `0x0046B458..0x0046B47F` similarly passes rotation, `avoidance_directions[best].j`, the up basis at EBP-0x6044, and rotation. The call at `0x0046B47F` has the same target.
4. The raw thunk at `0x00409AB1` is exactly `jmp 0x0042E0D0`. The complete 92-byte helper body computes three components of `result = vector * scalar + point`, then returns result. Its four-argument ABI and component association correspond to the existing `point_from_line3d` helper. The PE itself has no recovered public name for this address, so the name is a body/ABI correspondence, not a discovered PE symbol.
5. The sharp-turn branch still calls the three-basis private transform. Thus the later unoptimized build does not express the perpendicular branch as a call to that private transform that merely happens to inline.

January's raw sequence at function offsets `+0xE73..+0xEFF` independently copies global zero and accumulates only `left * -best.k` and `up * best.j`; its sole private-transform relocation belongs to the sharp-turn branch. The current extra 12-byte perpendicular object and second private-transform call therefore remain explained. This is stronger cross-build corroboration of the existing diagnosis, not a new successful inline law.

The cross-build also confirms the already tested clear-first weight branch (`0x0046A4BB jne` into the obstructed arm), and obstruction then weight initialization immediately before the descending ray-weight loop (`0x0046A469`, `0x0046A473`). Those source forms were not re-probed.

## Concrete remaining evidence gap

A natural helper-based source expression is now supported at the operation/ABI level: zero rotation, then two scaled-add helper calls. It is not a landed-ready January reconstruction under the unchanged ownership constraints. The current TU imports `point_from_line3d` externally (`actor_moving.c:208`); calling it would retain calls that January does not contain. Enabling its existing inline definition changes the TU's helper ownership, which earlier probes already measured and which this task forbids. Copying its statements locally or inventing a macro does not resolve provenance.

Reopen only with an authenticated January include/helper mechanism that reproduces the scaled adds while preserving the point-owner guard and all exact siblings, or with primary source evidence that establishes a different actual helper/macro and its definition. January's PDB has no local, lexical, or source-line records to supply that missing text. The cross-build's matching operation does not authorize a guessed January header mechanism. A changed ownership requirement would also require orchestrator-level review and separate whole-tree validation, not a worker workaround.

Even resolving that prerequisite does not establish exactness: historical `va6` retained a 4-byte frame difference and register-allocation differences. The cause that places obstruction in BL and gives the site-2 scalar its January home remains unproved. Fresh later-build scalar homes cannot be copied to force it.

## Receipts and controls

Reproducer: `python -B scratch/astra-context-avoidance-evidence.py`. It reads the existing objects and executable and writes only its own scratch evidence. `scratch/astra-context-avoidance-evidence.json` contains the full object identities, precise RTC records, target atlas row, strict controls, source receipt, and explicit zero-shape/zero-credit disposition. It is **not** an edit JSON.

Fresh complete raw streams: `scratch/astra-context-avoidance-{base,split,2020}.disasm.txt`, `astra-context-avoidance-math-helper-0042e0d0.disasm.txt`, and `astra-context-avoidance-math-thunk-00409ab1.disasm.txt`. Actual read-only Ghidra outputs are also retained as `scratch/astra-debug-evidence/ghidra_00469ce0.{asm,c}`.

All six controls were rechecked through the unchanged strict COFF comparator and pass: `_actor_move_transform_avoidance_vector`, `_actor_move_vector_avoidance_find_direction`, `_profile_frame_get_value`, `_render_ui_widgets`, `_render_ui_widgets_postgame`, and `_encounter_test_rule`. These validate the read-only baseline; no new compiler mechanism is claimed. Because there is no source candidate, no gate, fake-match scan, or downstream admission was necessary or represented as performed.
