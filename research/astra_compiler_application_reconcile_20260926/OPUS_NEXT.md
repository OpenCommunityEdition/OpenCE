# Opus: bounded grouped-data verifier correctness lane

The preceding source/data batch is independently reconciled. Do not repeat it.
This lane targets a concrete tooling defect and the already-prepared Q11 review,
not a new broad source campaign or an arbitrary object/byte quota.

## Start and authority

Canonical checkout:
`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\worktrees\astra-reconcile-20260920`.

Start a NEW isolated worktree/branch from its current local `jonas/exact-pilots`.
Require `7c0af5f0` (the independently reconciled production batch) to be an
ancestor and require the V1 research directory below to exist. Record the actual
40-character start commit. Do not start from the old donor or a stale remote.
Suggested branch/worktree name: `opus/q11-verifier-20260926` /
`C:\halo-worktrees\opus-q11-verifier-20260926`.

Do not edit canonical, another lane, production source/headers, symbols.json,
parks, compiler flags, objdiff version, semantic normalizations or object status.
No pushes. No original-bug, macro, cast, helper, COMMON-owner or other hold is
lifted. Root house rules and the latest specific rulings still govern.

Read first:

- `docs/campaign_house_rules.md` and `docs/matching_methodology.md`;
- `docs/object_matching_logs/compiler_application_canonical_reconciliation_20260926.md`;
- `research/compiler_application_20260925/HANDOFF_20260926.md` and OWNER_PACKET.md;
- `research/compiler_application_20260925/workers/V1/REVIEW_PACKET.md`, CARDS.md,
  patches, entries and evidence;
- `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\HALO_HOUSE_RULES_20260926.md`.

The new baseline is 1,591,710 / 1,770,166 Halo meaningful code bytes,
7,461 / 7,574 credited functions, 389 / 468 complete Halo objects and 2,588,903
credited data bytes. Whole-board strict section owners: 7,633 / 8,252. Parks
75/0/0; admission 11/0/1/0; pytest 1,161 passed, 5 skipped, 26 subtests. These
counts use different scopes; do not conflate them. Verify them on your fresh
tree with the frozen actual **objdiff 3.3.1** binary. Configure's newer download
pin is not permission to change the scorer.

## Deliverable A: grouped-entry regression-gate bug, zero credit

Authorized implementation files: `tools/regression_gate.py` and its genuine
test module(s). Research evidence may be added under a new lane directory.

`_exception_records` assumes every semantic-data entry has `entry["symbol"]`.
An existing grouped entry can therefore raise KeyError. First reproduce this
against the CURRENT production manifest with a failing regression test.

Repair the complete capture/diff path, not just the exception. Preserve each
whole entry's identity; correctly associate all members with their sections;
detect member additions/removals, changed measurements, duplicate/ambiguous
members, changed group identity and changed surplus declarations. Do not skip
groups, flatten away their identity, silently ignore unknown keys or weaken
existing single-section checks. Follow the actual production schema rather
than inventing a second one. Test all existing single/grouped entries and
adversarial mutations in both directions. Land only this zero-credit repair
in its own local commit after the full suite and unchanged credit/parks proof.

## Deliverable B: independent V1 review, initially HS only

This authorizes a reviewed implementation proposal and tests in your lane,
not production data-entry admission or new credit. Keep it separate from A.
Implementations under consideration are `tools/semantic_progress.py` and its
tests; proposed config entries belong in research, NOT the production manifest.

Re-read V1 as an adversarial reviewer. Do not accept its headline proofs or
start by chasing +57,184. Reproduce its old-entry controls and counterexamples,
then assess every one of its 14 review questions. In particular:

1. Bind the live report to the actual target, object identities and pinned
   scorer; stale or wrong-unit reports must fail.
2. Prove complete target-section coverage. Neither omitted members nor padding
   arithmetic may substitute for missing bytes. Detect duplicate/overlapping
   credit and already-credited groups.
3. Verify complete bytes, relocations including targets/addends, symbol/storage
   identity, layout/alignment and COMDAT selection. Unknown or unsupported
   shapes must fail closed.
4. Independently validate extent accounting, including tail/alignment padding;
   use the pinned scorer's implementation or discriminating tests, not a fit
   to these two objects. Separate raw data bytes from modeled padding.
5. Surplus pinning is not proof of admissibility. Prove each current selected
   provider and folding contract; preserve zero duplicate credit. Link checks
   must reject unexpected errors, not merely filter messages for one symbol.
   Use distinct byte-copy negative controls: supplying the same pathname twice
   is ignored by VC7. Expected unresolved externals are not a successful game link.
6. Exercise malformed manifests, missing/extra members, wrong symbols/flags,
   mutated bytes/relocations/providers, ambiguous ownership, stale reports and
   unsupported section names. Existing accepted entries and their credit must
   remain unchanged unless a genuine defect is found and escalated explicitly.

Prefer HS first: V1's 20 surplus literals correspond to January undefined
references. Re-prove their current providers. Keep actions separate: two of its
surplus literals come from a surplus normalize2d helper, not January references
from that object. Do not infer approval of that dependency.

The original B1/DV2 entry proposals are not approved; V1 rejects them under its
surplus contract. Its `_v1` entries are still proposals. Its board survey binds
810/833 units and refuses 23 long-name units; those are refusals, not matches.
The proposed 57,184 data bytes include 1,724 modeled padding bytes. They add
zero functions, zero meaningful code and zero completed objects. Nothing from
Q11 is credited in canonical yet.

## Working discipline and stop condition

Start with the reproducible gate bug yourself. Use at most two bounded review
workers if useful: one for model/coverage discrimination and one independent
critic for identity/provider failures. No large source-worker fanout. Stop after
two focused review/repair rounds unless a concrete new counterexample requires
a narrowly stated fix. Preserve useful negative results and do not report
bounded fuzzing as a universal soundness proof.

Do not divert into K1's supposed "last two hunks": admitted production is
8,096 bytes, frame 0x1270 and 67 differing hunks. The two-hunk body needs held
B/C/D forms. No parenthesis, declaration-count, name-count or source-shape grid.

Every relevant checkpoint: build, whole-board strict sweep, current semantic
ledger, parks, admission audit, tools pytest, warnings and diff check. Capture
actual exit codes and unfiltered logs. Missing prerequisites must be a visible
failure or explicitly reported skip, never a passing claim.

Finish with one handoff: immutable base/tip, separate commits for A and B,
tests/negative controls, affected old entries, fresh totals, explicit assumptions,
reviewer disagreements, proposed HS entry and remaining owner decisions.
Keep research records out of source commits; preserve private binary evidence
locally. Then stop for independent canonical review. Do not push.
