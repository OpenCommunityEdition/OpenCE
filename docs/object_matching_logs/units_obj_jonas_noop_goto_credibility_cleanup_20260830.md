# Units no-op control-flow credibility cleanup (Jonas, 2026-08-30)

## Scope and disposition

The new source-credibility gate flagged an unconditional
`goto finish_dying;` immediately followed by its destination label in
`unit_update_animation`. The same block also carried an unreferenced
`start_limp_body:` label. Neither construct expressed a runtime decision,
and neither was supported by authenticated source topology. Both reconstruction
artifacts are removed.

This is a credibility cleanup, not a new exact-match claim. The retained
ordinary C falls through from the optional biped limp-body call to the shared
dying-animation tail. Earlier real branches still target `finish_dying`, so
the meaningful control-flow join remains explicit.

## Strict evidence

- January object: `build/split/source/units/units.obj`, 138,092 bytes,
  raw SHA-256
  `de70778800ba600aa7365d9fa789b2d1147baf3b67e50d2f9a70ff958c9fa757`.
- Candidate `_unit_update_animation`: exact at 1,280 padded bytes,
  64 relocations, normalized SHA-256
  `41f12865734551c376b91868f7f815bc36b7a6b575b85d6bf5446ab1688a4d5f`.
- Whole-object strict census before and after: 188 exact function owners.
  `lost_exact=[]` and `newly_exact=[]`.
- The candidate raw object is 156,123 bytes with SHA-256
  `c1c011068847e169e7f1fe83e1fa11dcbb77d2ba3da7898bf49e91253c6c869b`.
  Its container differs from the prior worktree object because removing two
  source lines changes debug metadata; runtime function acceptance is
  unchanged.

The conservative scanner now reports zero review leads for
`source/units/units.c`. This result is only a lexical check; the semantic
reason for accepting the edit is the natural fall-through plus the strict
whole-object evidence above.

## Verification

- XDK 3911 rebuilt `build/base/source/units/units.obj` through the generated
  Ninja edge.
- `tools/coff_compare.py` reports `_unit_update_animation` equal.
- `tools/audit/compare_object_exact_sets_20260827.py` reports 188/188
  retained exact owners, no losses, and no new credit.
- `python tools/fake_match_scan.py --format json source/units/units.c`
  reports zero findings.
- `git diff --check` passes.

Reopen this cleanup only if authenticated original source proves that the
otherwise inert label or jump was intentionally present. Exact bytes alone
are not such proof.
