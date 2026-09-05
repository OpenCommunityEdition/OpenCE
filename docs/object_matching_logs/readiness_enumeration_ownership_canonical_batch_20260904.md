# Readiness, enumeration and ownership reconciliation, 2026-09-04

## Verified first batch

Starting from published canonical `56635021ee5f664a56680f7c805cafd8e76d5e71`:

- Saved Game Files now has **39 exact / 7 parked / 0 unwritten** functions.
  The final 992-byte January memory-unit enumerator is retained as natural C;
  its1008-byte candidate is an honest97.08481% fuzzy park, not exact credit.
- Endpoint Winsock now has **16 exact / 2 parked / 12 unwritten** functions.
  Listening and read-readiness use semantic flag macros and the correct word
  timeout. Their90.42857% and82.361115% parks supersede earlier exact-only
  removal; those residual searches were not repeated.
- One complete endpoint layout and the common endpoint API are owned in
  `transport_endpoint.h`, consumed by the implementation and re-exported by
  `transport_endpoint_winsock.h`. Four duplicate API declarations are removed
  from the C file. This provides real prerequisites for the set-operation
  reconstruction without incompatible local views.
- Dynamic geometry's one-byte diagnostic latch is now an owned external BSS
  definition, not COMMON. All17 functions stay strict. A separate explicit
  source-layout admission veto preserves its honest NonMatching status.
- The First Person Weapons full-cluster rejection ledger is preserved. Its
  scratch688 meaningful/704 padded exact bytes are **not integrated** because
  the complete candidate fails ordinary shared-helper linking. No source,
  metadata, or park from that candidate is imported.

This batch adds three credible implementations spanning **1,248 target padded
bytes**, but **zero new exact code bytes and zero completed objects**. It does
not claim fuzzy percentages as exact-code progress.

## Fresh accounting and data reconciliation

The verified totals are **900,020 / 2,198,102 credited meaningful code bytes**,
**6,149 / 11,060 credited functions**, **2,028,607 / 4,176,062 data bytes**,
and **391 / 833 Matching objects**. Code/functions/objects are unchanged.

The reported data increase of **5,609** is not5,609 newly reconstructed bytes:

- Saved Game Files' five newly emitted authentic literals total376 bytes:
  open-failure50, close-failure57, read-failure50, legacy-CRC-update73, and the
  UTF-16 unknown-save diagnostic146. They finish the report's entire5,608-byte
  `.rdata` section. This recredits5,232 bytes already present and strict in
  the preceding candidate.
- Dynamic geometry adds its one-byte BSS owner.

Thus **377 new physical target-owned data bytes** yield **5,609 reported data
bytes** by closing the existing whole-section accounting boundary. Independent
before/after COFF review finds no changed inherited named runtime data owner.
Saved Game Files' total6156 bytes comprise BSS540, data8, and rdata5608.
The report's raw matched-data total is2,002,313, plus the unchanged26,294 bytes
of validated semantic-data corrections, giving2,028,607. No semantic-data
exception was added for this packet.

## Verification

Full `ninja` builds every affected consumer and regenerates the target split,
objdiff report, strict semantic report, and progress. The new park records were
filled with the regenerated report percentages; all **261 parks validate**,
with zero stale or invalid entries. The first progress attempt correctly
rejected their initially missing percentages; no comparison rule was weakened.

Stable snapshots:

```text
scratch/saved-game-nonce-after-20260904.json
scratch/readiness-enumeration-after-20260904.json
```

All **6,195 / 8,245 stable strict owners** are preserved: zero regressions and
zero exact gains. Units remains **189/189 target functions**, with its existing
separate helper-ownership veto unchanged. Changed candidates pass the hard
`_point_from_line3d` emitted-symbol guard. Admission has zero new candidates,
contradictions, or revocations and four otherwise-complete rejected objects.

All **288 tests + 26 subtests pass**; only the existing optional pytest-cache
permission warning remains. The focused fake-source scanner has zero review
leads. `git diff --check` passes. Both protected bitmap source hashes are
unchanged. Independent reviews approve the saved-game semantics, the final
endpoint API/readiness delta, and the BSS/admission repair.

No active Fable worktree or assigned implementation file is inspected or
changed. Its new work remains reserved until the user announces completion.
