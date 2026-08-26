# `ai.obj` Jonas dispose one-shot wave (2026-08-26)

## Result and fixed boundary

This bounded wave starts from clean cumulative commit
`66c5e5f298f17454a2dda8d41d086e7d1e7d3acf` in isolated branch/worktree
`jonas/ai-dispose-one-shot-20260826`. The first and only normal
code-producing compile of the frozen candidate makes `_ai_dispose` strict
exact while preserving all twelve inherited strict-exact functions.

`ai.obj` advances from 12/46 to 13/46 exact functions, from 943 to 983 exact
meaningful code bytes, and from 1,040 to 1,088 exact padded code bytes. The
gain is exactly one function, 40 meaningful bytes, 48 padded bytes, and eight
zero-addend REL32 relocations. The object remains `NonMatching`.

Only `source/ai/ai.c` and this ledger are tracked changes. The approval-gated
`units.obj` shared-interface package remains frozen. No Units shared header or
protected neighbor (`source/units/vehicles.c`, `source/ai/ai_debug.c`,
`source/ai/ai_script.c`, or `source/ai/actions.c`) was edited.

## Immutable inputs and first-shot artifact

- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `ai.obj`: SHA-256
  `5b207c27308c3e78b7adf5bfc14f65c7cf8cc99aeba15b845270eb572265ced2`.
- Baseline commit:
  `66c5e5f298f17454a2dda8d41d086e7d1e7d3acf`.
- Baseline source blob:
  `0cac8b1e755a27bd3f32894904be1e437c4b9cb7`.
- Fresh untouched baseline generated-object SHA-256:
  `60c78752a9e73a1de4d950ceecb521f1c7e33d362f290917199fe9208c2fd013`.
- Frozen no-build baseline manifest:
  `build/audit/ai_dispose_baseline_20260826.json`, SHA-256
  `38621920b602e1d337aebbff7c896852675125313fbf1d76159a6b0157ee5ff5`.
- Retained source blob / working payload:
  `f9d573a7f1607ab733f213c65d29574f8da6014f` / 16,197 bytes, SHA-256
  `720b81752363588226ea2c1081e796b0722c5f5fcd317f019d4aed67d64c6aa4`.
- Preserved first-shot object:
  `build/audit/ai_dispose_first_shot_20260826.obj`, raw SHA-256
  `b88dc203ce624e2aaebd8d8bf78203019817018946f080f3d377deda9a0df79e`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler 13.00.9254.1 for
  80x86; `xbox/bin/vc7/CL.Exe` SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  The production edge uses the repository's ordinary `/O2 /Oy- /DDEBUG
  /Dxbox` flags and include closure.

The copied generated scaffold initially had a stale generator command record.
Before any source edit, one baseline-only Ninja invocation regenerated
`build.ninja` and rebuilt the untouched `ai.obj`; that object is the frozen
baseline above. The graph then reported no work for `ai.obj`. This support
compile predates the candidate and is not acceptance evidence.

## Claude, executor, experiment, and semantic provenance

All reachable Claude documentation, current transfer tooling, the archived
shape-transfer experiment, HCEA source, current exact donors and callees, and
January COFF evidence were audited before the source shape was frozen. This
body is not described as historically untried.

The archived experiment driver at
`C:/halo-worktrees/fable-transfer-exp/exp_driver.py` has Git-style blob
`5c1138337f7f79939fc71c002bf185f9df083993` and SHA-256
`e23d294bb978eaace3b1149eeaaf743e30d1e2f0b267a30be86abf26507af8a7`.
Its natural eight-call candidate produced `EXACT-TRANSFER`, equal 48-byte
normalized shapes, equal eight-relocation inventories, and no regressed
siblings. The preserved experiment-results JSON has SHA-256
`b8b66c8fa2f0f5e1a9d499272cad23ff6c0d474d8bf1ce4b954eb48fe2e84b86`.

The current hardened executor strategy, SHA-256
`072ee7b2b3a2900e401bec7f69bff475f57639456d9c6270d2ea51189aaefa83`,
records that `ai_dispose` passed its structural and relocation-substitution
checks but was refused because `ai_debug_dispose` had no unique declaration
in a repository header. The executor therefore did not generate or approve
this production patch. The retained source closes that manual review boundary
with ordinary local typed declarations matching repository definitions; it
does not add a declaration to the protected implementation or change a shared
header merely to satisfy the generator.

Independent semantic evidence fixes the call sequence:

- HCEA is pinned at `c168af2e747d3095d9a29418ae401f3a39544863`.
  Its `src/ai_dispose.c` blob
  `3628d284afc9965c945ee46722f260ddc9f152b0` independently attests the
  first six calls and their order.
- January target relocations add `ai_profile_dispose` and
  `ai_debug_dispose` as calls seven and eight. All eight destination symbols
  are external type-`0x20` functions with zero relocation addends.
- Current `source/shell/shell.c` blob
  `93da16bdd6ac9b12b91b09bc8ea8930f078bad5e` supplies the readable accepted
  eight-disposal donor shape.
- Direct hardened comparison proves all eight current callees strict exact.
  The first seven are 16-byte zero-relocation returns with normalized SHA-256
  `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca`.
  `_ai_debug_dispose` is 80 padded bytes with eight relocations and normalized
  SHA-256
  `0357c3db56a7ae52077642e9261d6fa36c298a09df31a192fd2adf87fea74866`.

`actors.h`, `props.h`, and their include closure already declare
`actors_dispose`, `props_dispose`, and `paths_dispose`. The five local
prototypes added to `ai.c` cover `ai_communication_dispose`,
`ai_script_dispose`, `encounters_dispose`, `ai_profile_dispose`, and
`ai_debug_dispose`; each exactly matches its typed repository definition.

## Strict admission

Hardened comparison covers relocation-normalized padded function COMDAT bytes:

| Function | Meaningful / padded | Relocs | Normalized SHA-256 | Disposition |
| --- | ---: | ---: | --- | --- |
| `_ai_dispose` | `40 / 48` | 8 | `7579c4adfd589894c9532699fb833fb56e1eae85732fee895baec0dbbfa0ecb9` | newly exact |

All relocations are zero-addend REL32 entries:

| Offset | Destination |
| ---: | --- |
| `+0x01` | `_ai_communication_dispose` |
| `+0x06` | `_ai_script_dispose` |
| `+0x0B` | `_encounters_dispose` |
| `+0x10` | `_props_dispose` |
| `+0x15` | `_actors_dispose` |
| `+0x1A` | `_paths_dispose` |
| `+0x1F` | `_ai_profile_dispose` |
| `+0x24` | `_ai_debug_dispose` |

The same direct comparison proves these twelve inherited functions remain
strict exact: `_ai_dispose_from_old_map`, `_ai_place`,
`_ai_globals_ai_active`, `_ai_globals_dialogue_triggers_enabled`,
`_ai_globals_grenades_enabled`, `_ai_erase`, `_ai_try_vehicle_eviction`,
`_ai_update_team_status`, `_ai_handle_enter_vehicle`,
`_ai_handle_exit_vehicle`, `_ai_enemies_can_see_player`, and
`_ai_enemies_attacking_player`.

## One-shot and source-policy record

The frozen candidate passed the XDK `/Zs` parse-only check. The subsequent
Ninja dry run contained exactly one
`CL build/base/source/ai/ai.obj` edge. That edge was invoked once, reported
exactly `[1/1] CL build\base\source\ai\ai.obj`, and the output was immediately
preserved under `build/audit`. No source spelling, declaration, ordering,
scheduling tune, alternate compiler, flag change, or second candidate compile
followed.

The retained definition is ordinary typed C with eight direct calls and an
explicit return. It has `void` on its own parameter line. It contains no
assembly, `volatile`, `register`, pragma, intrinsic, attribute, forced
inlining, barrier, raw offset or address, pointer/integer reconstruction, cast
or union pun, inactive-union access, undefined behavior, synthetic caller or
anchor, object-byte patch, comparator exception, or alternate compiler flag.

## Whole-object ownership and regression result

The target-authoritative census reports 13/46 functions strict exact, 983
meaningful exact code bytes, and 1,088 padded exact code bytes. Its artifact is
`build/audit/ai_dispose_hardened_census_20260826.json`, SHA-256
`e44e0efeee51362ac6cdc9a5fde50d8c53d9bb1306f00665555074d91a631fd1`.
It records all twelve inherited functions stable, no changed nonexact
function, and both candidate runtime `.rdata` COMDATs byte/relocation stable.
The candidate still receives zero of the target's 3,324 runtime-data bytes.

The ownership delta is exactly one new external code owner `_ai_dispose` plus
undefined typed references to its eight callees. There is no owner removal and
no new runtime `.data`, BSS, COMMON, or `.rdata` owner.

The generic whole-TU regression gate deliberately returns nonzero for this
physical expansion. It identifies `_ai_dispose` as the sole `NEWLY_EXACT` item
and reports `changed_nonexact: []`. Because the new COMDAT precedes the
existing public definitions, object-local section ordinals move by two; the
generic manifest includes those ordinals and therefore labels the twelve
accepted records, five anonymous `.debug$F` sections, `.debug$S`, and the two
string COMDAT records changed. Their stable normalized hashes, sizes,
relocations, identities, and ownership are unchanged, as recorded by the
hardened census and direct comparator. No adjudication or comparator exception
was added.

## Validation and campaign progress

- Full `halobetacache_build` and `libcmt_build` graphs pass. The candidate
  `ai.obj` was already current and was not rebuilt by either full invocation;
  its live raw SHA-256 remains identical to the preserved first-shot object.
- Hardened semantic audit: 470 units, 4,245 functions evaluated, 4,103
  semantic exact, 114 hidden exact / 64,705 hidden code bytes, 4,164 accepted
  exact, and zero unit errors. This is exactly +1 accepted function.
- Ordinary-only review: 36 entries, 35 structural and one rejected.
- Campaign progress: 375/833 complete objects, 4,138/11,060 exact functions,
  502,187/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes.
- Halo progress: 273/468 complete objects, 3,971/7,574 exact functions,
  489,273/1,770,166 meaningful code bytes, and
  1,829,896/3,923,451 matched data bytes.
- `ai.obj`: 13/46 exact functions, 983 meaningful and 1,088 padded exact code
  bytes, with no data credit.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked audit: three active, zero stale, zero invalid.
- Complete tooling suite: 179/179 tests pass.

The named report, semantic, admission, and parked artifacts are preserved as
`build/audit/ai_dispose_report_20260826.json`,
`ai_dispose_semantic_20260826.json`, `ai_dispose_admission_20260826.json`, and
`ai_dispose_parked_20260826.json`. No object status, configuration,
semantic-ledger, parked-manifest, header, storage declaration, protected file,
or shared-interface edit is part of this wave.

## Committed-state replay

Implementation-and-ledger commit
`c2ed180a119e2a6e89ed465634f335ce33421208` was clean before replay. Its
committed source blob is `f9d573a7f1607ab733f213c65d29574f8da6014f`.
The clean committed manifest is
`build/audit/ai_dispose_committed_20260826.json`, SHA-256
`fa73db0b391abaaf2b98112a8d04ecd3c58c7cd7afc408f67eece2cebfd40f38`.

The resolved generated object path was verified inside the isolated worktree,
then `build/base/source/ai/ai.obj` was literally deleted. The Ninja dry run
exposed exactly one expected compiler edge, and the normal rebuild reported
exactly `[1/1] CL build\base\source\ai\ai.obj`. The rebuilt object was
preserved as `build/audit/ai_dispose_committed_replay_20260826.obj`, 6,113
bytes, raw SHA-256
`2dcf1d284f78321ad032c3302a3b9cb9c7b6cb8f8db5ccfabd6c65942446f525`.

The replay and first-shot raw objects differ at exactly COFF header offsets
`+0x4` and `+0x5`, compile-timestamp bytes. Every modeled section, symbol,
meaningful and padded byte, and relocation is identical. Direct hardened
comparison again reports all thirteen functions exact. The committed manifest
check has no failures or warnings, no newly exact or changed-nonexact entries,
and places all thirteen functions in `still_exact`; its replay JSON has
SHA-256
`807f6be6d0bb319b129cabdaeb45972bcccd1e57b433eeaa1d0417cf0f75e468`.

Replay semantic, admission, and parked audits reproduce the accepted results
exactly. Their preserved SHA-256 values are, respectively,
`42369eab1e0249d62307f972f7f0db85ae9d1fba68cf1c1bbc1ea86fddc2ac01`,
`f0aa45c22cc604fb74abc388b06dd79a27e7a0b498a5d28837a9e785680c9a36`,
and `cfac03aea5be607c19a6a40bcdbc1164e71ffb0179602af4bd37b3fd50e26d1d`.
The `ai.obj`, complete Halo, and complete libcmt dry runs all report no work
after replay.

No push is performed.
