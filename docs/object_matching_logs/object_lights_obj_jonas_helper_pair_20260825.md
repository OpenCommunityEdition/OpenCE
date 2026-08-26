# `object_lights.obj` Jonas helper-pair wave (2026-08-25)

## Result and fixed boundary

This bounded wave starts from clean cumulative commit
`4dc1e1a66ec5834943d3b15c304cd2356ed3b6c7` in isolated branch/worktree
`jonas/object-lights-helper-pair-one-shot-20260825`. The first and only normal
code-producing compile makes `_code_00129100` and `_code_00129120` strict
exact while preserving all seven inherited strict-exact functions.

`object_lights.obj` advances from 7/43 to 9/43 exact functions, from 411 to
459 exact meaningful code bytes, and from 448 to 512 exact padded code bytes.
The gain is exactly two functions, 48 meaningful bytes, 64 padded bytes, and
four zero-addend relocations. The object remains `NonMatching`.

Only `source/objects/object_lights.c` and this ledger are tracked changes. The
approval-gated `units.obj` shared-header package remains frozen: no Units
shared header or protected neighbor (`source/units/vehicles.c`,
`source/ai/ai_debug.c`, `source/ai/ai_script.c`, or
`source/ai/actions.c`) was touched.

## Immutable inputs and first-shot artifact

- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `object_lights.obj`: 35,275 bytes, SHA-256
  `a398a56409b9cf4555ac93df1c208baf328253fc95d2ee6349b0f9bd175d66e9`.
- Baseline commit:
  `4dc1e1a66ec5834943d3b15c304cd2356ed3b6c7`.
- Baseline source blob:
  `5a43f11a3d4c9f7b9ed0f2d41d3c2c50502c7fec`.
- Baseline generated-object SHA-256:
  `5977d2874e4fc01298b096939c5c966bd1d885c71bbe92bcee0283c09dde1388`.
- Frozen no-build baseline manifest:
  `build/audit/object_lights_helper_pair_baseline_20260825.json`, 838,817
  bytes, SHA-256
  `275ec480260cac0ffe339d1020470f42909ed308ad6378e6d807808022453c90`.
- Retained source blob / stable LF payload:
  `12362d2a85bdacf09ce7a108a2302eab7512e832` / 7,003 bytes, SHA-256
  `1dbe3bb8ab0dacc2cb91a194353b80651f429868148c1ca421b5fc0c24b2c8c0`.
- Preserved first-shot object:
  `build/audit/object_lights_helper_pair_first_shot_20260825.obj`, 3,946
  bytes, raw SHA-256
  `cef7043de74ce9763f7ffd0ebb26922b81ca2406069a7c931bf19463625d6d89`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler 13.00.9254.1 for
  80x86; `xbox/bin/vc7/CL.Exe` SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  The production edge uses the repository's ordinary `/O2 /Oy- /DDEBUG
  /Dxbox` flags and include closure.

The post-patch working file retained its frozen mixed checkout line endings
(284 CRLF plus 19 LF). `git diff --check` and Git canonicalization are clean;
the stable LF payload and Git blob above are the durable source identities.
No spelling or line-ending cleanup followed the production compile.

An earlier scaffold-only worktree named `object-lights-helper-pair-20260825`
was quarantined before any source edit after a baseline command attempted to
refresh copied full-report dependencies. It never compiled `object_lights.obj`
or any candidate. No artifact from that worktree is acceptance evidence. This
wave began afresh in the isolated one-shot worktree named above.

## Claude, executor, history, and typed provenance audit

All reachable Claude documentation, transfer tooling, historical object-lights
work, current donors, HCEA sources, and January COFF evidence were audited
before the source shape was frozen. These helpers are not described as
untried: July's 12-case transfer experiment and the surviving dirty
`C:\tmp\object-lights-lane` both emitted this pair exact. The surviving July
object has SHA-256
`1a0d58afcbdc2542244c2f23e02ecf89ec7a3d5d88cfc2b63e1518dca66f44d8`;
the separate experiment record reports `EXACT-TRANSFER`, equal normalized
hashes, and zero regressed siblings.

The production executor strategy originally refused both recipients only
after their structural and relocation-substitution checks passed, because the
then-current recipient context lacked authenticated `light_cluster_partition`
provenance. Canonical commit
`f4d45dab67330fdae8398084ac0dd07894c38a6d` later introduced the tracked typed
external declaration. The August 21 map-disconnect ownership audit proves
`_light_cluster_partition` is section zero, value zero, external, and owns no
writable storage in this translation unit.

The current hardened executor still cannot emit this pair: neither anonymous
recipient has an explicit prototype in the recipient include closure, so it
fails closed as `recipient-prototype-missing`. This retained patch is therefore
a manual evidence-backed reconstruction, not executor-generated or
executor-approved. No fabricated header or redundant local prototype was
added merely to satisfy the automation boundary.

Independent ABI and semantic evidence closes the human review gap:

- HCEA commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c` reconstructs
  `cluster_get_first_light(int *, int16_t)` and
  `cluster_get_next_light(int *)` in blobs
  `482c875c99ded0b198dbe94366d5080c8c032341` and
  `a1173db185fef9bf12b566e16d2997ce752eb897` and uses them as the typed first
  and next callbacks in `lights_preprocess_scene`.
- Current exact collideable and noncollideable wrappers in `objects.c` provide
  the identical natural C shapes. That file's blob is
  `ae382b72b36baa55d0a6f8b0c2b0d6173515a388`, introduced by
  `eb93ce635ebfa38aea18dd455f27bf79eb63e848`.
- `cluster_partitions.h` supplies the exact called API declarations. Its blob
  is `15eb66e1d45fbff16e56d3f1a8d952eae7dddd5f`.
- The transfer atlas marks both pairs safe and donor-source-resolved. January
  `_lights_preprocess_scene` is the sole target caller and owns DIR32 callback
  references at `+0x147` to `_code_00129120` and `+0x14C` to
  `_code_00129100`.
- Both January recipient symbols are external functions (type `0x20`, storage
  class 2), requiring the retained global, non-static exact names.

No Claude branch contains a hidden helper implementation or compile attempt;
the Claude residual census only records the target-only functions. Claude's
strategy and tooling establish the historical refusal and fail-closed review
boundary, not matching credit. HCEA supplies later-build typed semantics;
January COFF plus the hardened comparator remain the admission authority.

## Strict admissions

All hashes below cover relocation-normalized padded function COMDAT bytes.

| Function | Meaningful / padded | Relocs | Normalized SHA-256 | Disposition |
| --- | ---: | ---: | --- | --- |
| `_code_00129100` | `26 / 32` | 2 | `5b9b89f6962abccb3e3227ec1d46af2afc50dee9c6ee00e75b80a78924c43681` | newly exact |
| `_code_00129120` | `22 / 32` | 2 | `e9b3377d3ab4566941f1bc3f1999c5d112dad7f286444d579b0e3b8acbb885d7` | newly exact |

All relocation addends are zero:

| Function | Offset | Type | Destination |
| --- | ---: | --- | --- |
| `_code_00129100` | `+0x0C` | DIR32 | `_light_cluster_partition` |
| | `+0x11` | REL32 | `_cluster_partition_get_first_datum` |
| `_code_00129120` | `+0x08` | DIR32 | `_light_cluster_partition` |
| | `+0x0D` | REL32 | `_cluster_partition_get_next_datum` |

Direct hardened comparison also proves these seven inherited functions still
exact: `_lights_dispose`, `_lights_initialize_for_new_map`,
`_lights_dispose_from_old_map`, `_lights_enable`, `_light_delete`,
`_light_disconnect_from_map`, and
`_lights_disconnect_from_structure_bsp`.

## One-shot and source-policy record

The frozen candidate passed the XDK `/Zs` parse-only check. The subsequent
Ninja dry run contained exactly one
`CL build/base/source/objects/object_lights.obj` edge. That edge was invoked
once, reported exactly `[1/1] CL build\base\source\objects\object_lights.obj`,
and the output was immediately preserved under `build/audit`. No source
spelling, retry, scheduling tune, alternate compiler, flag change, or second
production compile followed.

The retained definitions are ordinary typed C using the existing typed
partition external and APIs. They have one parameter per line and direct
explicit returns. They contain no assembly, `volatile`, `register`, pragma,
intrinsic, attribute, forced inlining, barrier, raw offset or address,
pointer/integer reconstruction, cast or union pun, inactive-union access,
undefined behavior, synthetic caller or anchor, object-byte patch, comparator
exception, or alternative compiler flag.

## Whole-object ownership and regression result

The independent target-authoritative census reports 9/43 functions strict
exact, with exactly the two named gains and no inherited exact-function loss.
Its ignored artifact is
`build/audit/object_lights_helper_pair_hardened_census_20260825.json`, 38,283
bytes, SHA-256
`78ec46ed2cbcbf105ba44dc196664db1df494628ef3ad5811903b9684c058836`.
The target runtime-data census remains 2/22 strict-exact sections: there is no
new target-data gain and no inherited exact-data loss. The candidate introduces
no runtime `.data`, BSS, or COMMON owner. All 2,645 target runtime-data bytes
remain uncredited.

The ownership delta is exactly the two new code COMDAT owners and their
function symbols plus undefined references to
`_cluster_partition_get_first_datum` and
`_cluster_partition_get_next_datum`. There are no owner removals. Only expected
compiler debug metadata changes (`.debug$S`, 187 to 204 bytes); all inherited
code and runtime non-code fingerprints remain unchanged.

The generic whole-TU baseline gate deliberately returns nonzero for this
reviewed physical expansion. It reports only the expected `.debug$S` and
symbol-set changes, lists exactly the two helpers as `NEWLY_EXACT`, lists no
`changed_nonexact`, and retains all seven inherited functions in
`still_exact`. No adjudication or comparator exception is added.

## Validation and campaign progress

- Hardened semantic audit: 470 units, 4,244 functions evaluated, 4,102
  semantic exact, 114 hidden exact / 64,705 hidden code bytes, 4,163 accepted
  exact, and zero unit errors. This is exactly +2 accepted functions.
- Ordinary-only review: 36 entries, 35 structural and one rejected.
- Campaign progress: 375/833 complete objects, 4,137/11,060 exact functions,
  502,147/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes.
- Halo progress: 273/468 complete objects, 3,970/7,574 exact functions,
  489,233/1,770,166 meaningful code bytes, and
  1,829,896/3,923,451 matched data bytes.
- `object_lights.obj`: 9/43 exact functions, 459 meaningful exact code bytes,
  and no data credit.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked audit: three active, zero stale, zero invalid.
- Full `halobetacache_build` and `libcmt_build` graphs: dry run and normal
  invocation both report no work, preserving the first-shot object.
- Complete tooling suite: 179/179 tests pass.
- `git diff --check`, source-policy, changed-path, deleted-path, protected-file,
  and frozen-scope reviews pass.

The semantic, admission, and parked audit artifacts are preserved as
`build/audit/object_lights_helper_pair_semantic_20260825.json`,
`object_lights_helper_pair_admission_20260825.json`, and
`object_lights_helper_pair_parked_20260825.json`. No object-status,
configuration, semantic-ledger, parked-manifest, header, storage declaration,
protected-file, or shared-header edit is part of this wave.

## Committed-state replay

Pending clean implementation-and-ledger commit and literal delete/rebuild
replay.

## Corrected cumulative-HEAD closure

Pending integration and literal corrected-HEAD replay on
`jonas/units-integration-20260824`.

The approval-gated Units package remains frozen. This ledger does not
constitute the required exact approval phrase and does not authorize any edit
to the Units shared-header package or its protected neighbors.
