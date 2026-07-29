# Exact-shape transfer candidate executor

## Scope and invariants

This branch productionizes only candidate generation and review-packet emission.
The executor is deliberately unable to apply a patch, invoke a compiler, update a
matching ledger, edit configuration, commit, merge, or integrate.

The repository is read-only from the executor's point of view. Output must be a
new directory outside the repository. A successful run emits an unapplied patch,
a standalone candidate fragment, and review metadata. A refusal emits no
candidate.

The input atlas is evidence to recheck, not an authority for text replacement.
The executor derives the ordered substitution events from the donor and
recipient January target COFF relocations and requires the atlas events to match
exactly. It refuses:

- unsafe or incomplete proof records;
- stubs with a meaningful size of 16 bytes or less;
- unequal padded/meaningful sizes, normalized bytes, relocation count/order/type,
  storage class, or prologue;
- non-injective substitutions;
- relocation addend changes, non-symbol substitutions, and constant symbols;
- donor source that is missing, ambiguous, recursive, or not a real definition;
- missing, ambiguous, or incompatible recipient prototypes/inline definitions,
  including return type and every parameter type;
- stale/wrong-callee atlas events;
- a transformed symbol that is not present exactly as often as the donor target
  relocates it;
- missing recipient declarations unless a single unambiguous declaration can be
  copied verbatim from a repository header (the packet pins that header hash);
  an optional manifest can pin the same operation explicitly; and
- a recipient function already defined in source.

Text generation is token substitution only. It changes the donor function name
and the relocation-proven symbol identifiers; it cannot synthesize a body,
change constants, reorder statements, or retry a refusal with heuristics.

## Acceptance boundary

`CANDIDATE_GENERATED_NOT_ACCEPTED` is the strongest possible executor result.
Exact acceptance remains exclusively:

1. the hardened cross-name `tools.coff_compare.section_infos_equal` comparison;
2. a zero-regression whole-translation-unit manifest comparison; and
3. unchanged whole-object data/BSS/ownership fingerprints.

Those gates must be run in a disposable build worktree by a human review lane.
The executor neither runs them nor converts their future results into matching
credit.

## Operator usage

Invoke the tool from a clean campaign worktree. `--target-root` is the directory
whose children mirror the atlas unit names and contain the January target
objects. `--output-dir` must be a new directory outside the repository.

```powershell
python tools\transfer_executor.py `
  --repo-root C:\path\to\campaign-worktree `
  --atlas C:\path\to\transfer_atlas_v2.json `
  --target-root C:\path\to\build\split `
  --base-root C:\path\to\untouched\build\base `
  --recipient-unit source/objects/widgets/flags `
  --recipient-function _flag_delete `
  --donor-unit source/objects/widgets/antenna `
  --donor-function _antenna_delete `
  --output-dir C:\path\outside\repo\flag-delete-review
```

The donor selector is optional only when the recipient selector is unique. The
tool refuses an ambiguous selection. Missing declarations are copied only when
one exact declaration exists in repository headers. An operator may instead
pass `--declaration-manifest`; each entry must include `symbol`, verbatim `text`,
repository-relative header `source`, and the SHA-256 of that header. Extra,
stale, non-verbatim, or non-header declarations are refused.

A successful output directory contains:

- `candidate.c`: standalone generated fragment;
- `candidate.patch`: unapplied unified diff;
- `review_packet.json`: hash-pinned inputs, derived events, generation checks,
  and all acceptance gates marked `NOT_RUN_REQUIRED`;
- `frozen_tu_baseline.json`: the complete untouched recipient target/base
  function, symbol, non-code section, and ownership fingerprints plus the
  pre-edit strict-exact sibling set; and
- `REVIEW.md`: human handoff and mandatory gate list.

## Implementation and validation log

- 2026-07-29: observed the clean canonical campaign integration HEAD at
  `5b61655c5e40f6964d8dd37d2db4364b0a82c07e`.
- 2026-07-29: created isolated branch `codex/shape-transfer-executor` and
  worktree `work/shape-transfer-executor` from that commit.
- 2026-07-29: reviewed the fail-closed executor spec, minimum transformation
  language, duplicate atlas, donor/recipient map, 12-case experiment driver,
  atlas generator, and later production blueprint.
- 2026-07-29: synthetic executor suite passed 22/22. The lexically first test
  was the requested zero-substitution, non-rasterizer class. Covered successful
  no-substitution and callee/data substitutions; unchanged constant
  relocations; explicit and automatic hash-pinned declarations; immutable and
  atomic packet emission; frozen pre-edit TU/object manifests; changed-input
  detection; review lints for donor-domain parameter names and potentially
  redundant generated declarations; and refusals for wrong callees/stale atlas
  events, non-injective maps, relocation/statement-order changes, stubs,
  ambiguous sources/pairs, missing declarations/baselines/prototypes,
  incompatible return/parameter ABIs, pre-existing recipient bodies, and
  source/target occurrence mismatches.
- 2026-07-29: the complete `tools/test_*.py` suite passed 167/167.

## Archived-artifact validation

Validation used `transfer_atlas_v2.json` and the archived January target objects
under `fable-transfer-exp/build/split`. Generation read source only from this
clean `5b61655c5` worktree.

The real zero-substitution non-rasterizer pair was tested first:

- `ai/actions:_set_real_vector3d <- ai/ai_debug:_set_real_point3d`:
  **REFUSED (`donor-source-missing`)**. The older
  `donor_source_resolved` flag was a regex false-positive: `ai_debug.c`
  references `set_real_point3d` but does not define it. The implementation is
  inline in another header. Header-emission donor resolution is intentionally a
  later resolver upgrade, not a heuristic in this executor.

The five previously exact experiment pairs then produced:

| Recipient | Result | Detail |
|---|---|---|
| `objects/widgets/flags:_flag_delete` | `CANDIDATE_GENERATED_NOT_ACCEPTED` | Ordered target relocations derived `_antenna_data -> _flag_data`; unique `datum_delete` prototype copied verbatim from `source/memory/data.h` and header hash pinned. |
| `ai/ai:_ai_dispose` | REFUSED | No explicit `ai_debug_dispose` declaration in recipient context or a unique repository header. |
| `effects/effects:_effects_dispose` | REFUSED | No explicit `effect_data` declaration/definition in recipient context or a unique repository header. |
| `objects/object_lights:_code_00129100` | REFUSED | No explicit `light_cluster_partition` declaration/definition in recipient context or a unique repository header. |
| `objects/object_lights:_code_00129120` | REFUSED | Same missing `light_cluster_partition` evidence. |

All four missing-declaration refusals occurred after the target-object structural
and relocation-derived substitution checks passed. The executor did not recreate
the experiment driver's hand-written declarations because the production
blueprint requires real-header provenance.

Independent human review of the first `flag_delete` packet made two source-only
cleanups that relocation evidence cannot prove: `antenna_index` became
`flag_index`, and the redundant local `datum_delete` prototype was removed.
After that cleanup the candidate passed strict comparison at 32/32 bytes and
2/2 relocations; all four pre-existing strict-exact siblings remained exact;
both existing `.rdata` COMDAT payloads/sizes remained byte-identical; and
`_flag_data` remained the same 4-byte COMMON symbol. This is a valid
zero-regression transfer validation, not production admission: `flags.obj`
remains incomplete at 5/16 functions, no production source was changed here,
and no matching credit was granted. The executor now emits explicit
`REVIEW_REQUIRED` lints for both classes of cleanup instead of attempting them.

An early next-packet attempt for
`effects/contrails:_local_random_range <-
camera/bored_camera:_real_local_random_range` exposed a serious false-positive
class and is **revoked**. Relocations suggested only
`_real_seed_random_range -> _seed_random_range`, but the donor is
`float(float, float)` while `source/math/real_math.h` already defines the
recipient inline as `short(short, short)`. Their wrapper bytes happen to share a
shape because both argument types consume dword stack slots and each
tail-returns its own callee's native ABI. Applying the packet in an isolated
review worktree failed CL with C2371 at `contrails.c(162)`, pointing to the
conflicting inline definition at `real_math.h(1344)`.

The executor now requires a unique compatible recipient declaration or inline
definition from the actual recipient include closure, with identical canonical
return and parameter types, before source transformation. Missing, ambiguous,
function-pointer/array-unsupported, or incompatible evidence is terminal. A
synthetic regression test reproduces the real/short conflict, and the real pair
now refuses as `recipient-prototype-incompatible` without creating output.

Re-scanning all 21 atlas entries marked `donor_source_resolved` under this
hardened rule produced zero new candidates. Therefore the current automated
candidate batch is empty. The independently validated `flag_delete` transfer is
the sole human-reviewed result, but it is still not admitted and the hardened
generator would now refuse it until a recipient prototype is sourced. The other
proven experiment pairs and the zero-substitution pair stay closed until
declaration, prototype, or donor-source provenance is improved without
heuristics.

No production source, matching status, configuration, or ledger changed during
artifact validation.
