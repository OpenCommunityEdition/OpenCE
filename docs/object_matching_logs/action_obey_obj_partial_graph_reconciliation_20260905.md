# ActionObey partial callback graph reconciliation

Date: 2026-09-05. Parent: `c32247e7651d680dcd133d72b2fbf4e8009cc124`.

## Accepted result

The verified partial graph advances ActionObey from **10/27 to20/27 strict
exact target functions**, **485 to2,436 meaningful code bytes**, and
**528 to2,560 padded bytes**. New credit is **1,951 meaningful /2,032 padded
bytes and10 functions**. Seven target functions remain unwritten; this is not
a completed object.

Canonical totals after the independently checked accounting repair are
**912,298 /2,198,102 meaningful exact code bytes**, **6,197 /11,060 credited
functions**, **2,061,020 /4,176,062 matched data bytes**, and **391/833 Matching
objects**. The separate stable owner sweep advances6,233 to**6,243 /8,245**,
with zero inherited exact losses. All298 tests plus26 subtests pass.
There are284 valid fuzzy parks, zero stale/invalid entries, and no revoked
object admissions.

| New target function | Meaningful bytes | Padded bytes |
| --- | ---: | ---: |
| action_obey_command_end |549|560|
| action_obey_individual_setup |110|112|
| action_obey_individual_flush_command_indices |49|64|
| action_obey_individual_begin |71|80|
| action_obey_individual_advance |20|32|
| action_obey_individual_end |72|80|
| action_obey_individuals_iterate |290|304|
| action_obey_command_list_setup |383|384|
| action_obey_directmovement_update_facing |336|336|
| action_obey_individual_update |71|80|

## Genuine source and private caller closure

Only `source/ai/action_obey.c` changes implementation. No shared header
from the failed command-owner trial is retained. Public prototypes remain
in the existing ACTION_OBEY owner section of `actions.h`; all nine newly
implemented private functions are static with semantic names. The matching
nine `config/symbols.json` records are edited in place, preserving the
23,225-record order and every other record.

The public setup routine and five existing public wrappers retain the real
iterator/callback graph. The end callback retains command_end; the update
callback retains directmovement_update_facing. No dummy caller, emission
anchor, forced inline, private extern ABI, or consumer-local owner declaration
is introduced. The excluded private begin/perform pair and its individual
perform caller are not declared as fake externs.

The source consumes the already accepted full swarm/obey/unit/biped records,
typed datum/object/tag access, project boolean/real types and flag macros,
named command cases, original assertions and PC-only loop diagnostics.
Parameters/prototypes retain the house format and explicit returns.
The structure-cache leaf is unchanged.

The real math owner supplies cross_product3d and negate_vector3d through the
new facing caller. Their two include-time aliases and manual duplicate
bodies are removed atomically. Both existing target bodies remain strict
exact. Their selection metadata changes from manually emitted NODUPLICATES
to natural header SELECT_ANY; that ownership difference is disclosed, not
mistaken for identical object metadata. The other two inherited manual
aliases, random_range and vector_from_points2d, are unchanged pending their
genuine private-command callers. This partial packet does not endorse them
as final source topology.

## Provenance adjudication

Fable's preserved donor is
`C:/halo-worktrees/fable-50k-small-families-20260904`, commit
`f9dd42218b3325e1b3a63d7860e5309f69fd3d8e`; it remains read-only.

These are authenticated reconstructions, **not a claim that all Fable bodies
were first-shot natural matches**. Its `scratch/reports/action_obey.md`
records an earlier early-return setup shape changed to the ordinary single
success-result form, a command_end case ordering correction to January's
physical case-block order, and a signed-short swarm flag correction.
The signed field is now independently PDB-proven. The retained success-result
control flow is meaningful, follows the engine's common single-result
style, and matches January's state-store/dataflow. The command case order
corresponds to the actual January layout without changing behavior or adding
nonsensical control flow. No spelling, lifetime, declaration-order, flags,
assembly, or source-shape search was conducted during this integration.
This adjudication does not reopen exhausted tuning recipes elsewhere.

## Accounting repair, not a relaxed match

Ordinary objdiff reports command_end at85.26786% and the facing helper
at96.77419%, even though complete code and ordered relocation checks pass.
Two narrow semantic ledger entries restore their549+336 meaningful bytes:

- command_end:560 bytes,24 relocations, normalized SHA-256
  `f59f0185a672ce6409fe3d3c3a439e517483593f6ffc9bf5eff4b18dd4420a61`;
- facing:336 bytes,17 relocations, normalized SHA-256
  `17b28fabb3ad4366074689d2f9be1364ccf07cdc0c7a185dbd4dff25b00e3f97`.

The discrepancy is compiler-local switch-label representation. Every build
must still pass the unchanged strict comparator; no target bytes, branch
destinations, relocation identities, or comparison policy were altered.
Without these ledger entries the report showed911,413 bytes and6,195
functions. The additional885 bytes are part of this same1,951-byte packet,
not another reconstruction gain.

## Whole-object boundary

The candidate naturally emits five math functions absent from this
ActionObey target: square_root16, scale_vector3d48, magnitude_squared3d48,
magnitude3d48 and normalize3d96 padded bytes. All are SELECT_ANY. Only
normalize3d has incoming candidate relocations, from the facing function at
167 and217; the other four are header emission. The full572-base/833-target
owner census finds byte/relocation-identical available implementations for
each helper. **None receives ActionObey target credit.**

Code equality is not link compatibility. Ordinary frozen actual base/base
links with ActionCharge fail in both input orders with LNK2005 for
square_root, scale_vector3d, magnitude3d and normalize3d: ActionCharge owns
NODUPLICATES copies. Both links also retain37 unrelated unresolved externals
and produce no final binary. No FORCE, IGNORE, stub, object patch, selection
patch, or synthetic discard pass was used. Full Ninja success means
compilation/reporting, not successful final-image linkage.

An explicit ActionObey whole-object rejection records this ownership boundary
and the remaining inherited alias topology. Target-function progress remains
valid separately. Reopening requires authentic source/owner evidence and
ordinary compatible canonical linkage, not manual expansion or forced
inlining to hide helper emission.

## Verification evidence

All paths below are relative to canonical:
`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/rasterizer-model-wrapper-alias-20260829`.

- Frozen `scratch/action-obey-partial-graph-before-20260905.json` and
  `*-after-20260905.json` include source, target/base objects, symbol manifests,
  and hashes of all572 base objects. Only ActionObey changes; all571 other
  complete base-object hashes are identical.
- Whole frozen base hashes: before
  `5335c76a7e9833303b69e3a2d3a8e71fb73ae34e2b83aac4d22642a11c6659d6`;
  after `f339f7776b58f5d044015dc363a709ac649878c918a61591a6e21a71ebedb4f9`.
  Renamed January target:
  `13a4e47de2c79a3122033539284d148cae052f5f008f084a3d15862dda844368`.
- `scratch/action-obey-partial-graph-stable-after-20260905.json`:
  ten gains/2,032 padded bytes, zero losses.
- Gate:20 exact, zero residual, seven unwritten; no emitted
  `_point_from_line3d`. No candidate code/data/BSS/COMMON owner is silently
  excluded from the complete-object inventory.
- Independent object audit root replay:
  `scratch/action-obey-partial-graph-independent-object-root-20260905.json`,
  SHA-256 `321305aa1268865dfc91d598ab827ab24de37a3e9e1bfcd99880a7e3eb71c104`.
  It verifies exact positional symbol edits, all sections and all new owners.
- Math-owner census root replay:
  `scratch/action-obey-partial-graph-math-comdat-root-20260905.json`,
  SHA-256 `6f236108159648eccef28a68c9c07e221abeedf76c268a30c093c971aaf586b6`.
- Ordinary links:
  `scratch/action-obey-partial-graph-ordinary-links-20260905.json`,
  SHA-256 `8e0a4d74b64caae7d1867b47a1d5e50cfdf1993f260746760190eaf7eefd0ce5`.
- Public verification tool:
  `tools/audit/action_obey_callbacks_runtime_differential.py`,
  SHA-256 `1caf0250cc996639dd792145894cfbd381fc61546c380f4d625e7b6c18833697`.
  Its target-first cases cover setup9, flush44, advance256, in target,
  actual and isolated objects. Real six-argument callback ABI, complete
  modeled0x24/0x58 state, API arguments/order, caller stack frame, ESP,
  nonvolatile registers and DF all pass. Setup's authentic null-reference
  assertion/noreturn exit is covered. Tag indices/handles stay within the
  documented valid model; this is not a whole-game execution claim.
- `tools/test_action_obey_callbacks_runtime_differential.py` adds ten
  domain/negative-control tests. Full `tools` suite:298+26 pass.
- Final accounted build:
  `scratch/action-obey-partial-graph-accounted-build-20260905.log`.
  Parks/admission reports have no stale park or revocation.
- Units remains189/189 exact. Both protected bitmap source hashes are
  unchanged.

Reproduction of the bounded public oracle:

```text
python -B tools/audit/action_obey_callbacks_runtime_differential.py --actual build/base/source/ai/action_obey.obj
```

The tool requires its documented frozen target and retained HCEA donor
evidence (or explicit paths), and refuses changed section/hash evidence.

## Remaining reconciliation

The coherent larger command-owner trial was rejected after a Units
regression and fully restored; see
`action_obey_obj_command_owner_trial_rejected_20260905.md`.
Private begin/perform source proposals and corrected PDB prerequisite evidence
remain scratch-only, uncompiled, and uncredited. Seven target functions,
remaining data, helper ownership, and two inherited aliases still prevent
ActionObey completion. Fable Bitmaps and other lane packets remain separate
pending work; historical lane totals are not added to canonical.
