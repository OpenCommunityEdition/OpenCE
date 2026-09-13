# Semantic accepted-ledger progress bridge (2026-09-13)

## Result

The canonical progress and object-admission paths now consume the same
fail-closed `accepted_ledger` that the multi-lane treemap already displays.
This removes an accounting split: functions proven exact by the generated
COFF semantic audit are no longer visible only in the treemap while remaining
absent from `configure.py progress` and the object-admission queue.

At the published `5d3444f79904738b984398143df262cc45157239` baseline, applying
the bridge to the current build report contributes 87 semantic-COFF false
negatives beyond ordinary objdiff and the explicit semantic-match manifest.
The resulting totals are the already accepted ledger totals:

- 1,134,830 meaningful exact code bytes;
- 6,960 accepted functions.

These are not new decompilation claims. They are existing strict COFF proofs
now represented consistently by all first-party accounting consumers.

## Fail-closed rules

`apply_semantic_accepted_ledger` admits only entries whose proof sources
include `semantic-coff`. Before credit it requires:

- an `accepted_ledger` list and an exact summary count;
- one unique `(unit, function)` identity;
- no overlap with `ordinary_rejected`;
- an existing, uniquely named function in the current objdiff report;
- a positive semantic byte count equal to the report function size; and
- a non-local function identity (compiler-local `$` continuations remain
  governed by the explicit manifest path).

Ordinary objdiff matches are skipped because they are already counted. The
explicit semantic-match verifier now marks its report function exact, so a
function proved through that independently revalidated path is also never
double-credited.

The generated report remains a Ninja output that depends on the live objdiff
report and rebuilt objects. As with the pre-existing progress command, callers
must build the report targets before asking for current progress.

## Object admission remains separate

The bridge does not mark any object complete. It lets
`tools/audit_object_admission.py` see the accepted function evidence, after
which zero function gap, zero data gap, COFF ownership, source-layout policy,
and the explicit admission-rejection manifest still govern the decision.
Thus a semantic function proof can expose an object as a review candidate but
cannot bypass data or ownership review.

## Verification

Focused regression coverage proves:

- one semantic false negative is credited once;
- an explicit semantic match is not credited twice;
- duplicate identities fail closed;
- overlap with the rejection ledger fails closed; and
- a report/ledger size disagreement fails closed.

The focused suite passed 36 tests and 26 subtests before the global rebuild.
The final regenerated Ninja graph, semantic report, progress target, and
object-admission audit all pass with the bridge active. The rename-stable
whole-tree sweep records zero regressions, and the complete `tools/` suite
passes 1,151 tests plus 26 subtests (five skipped).
