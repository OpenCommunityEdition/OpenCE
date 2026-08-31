# Fake-match source review

`tools/fake_match_scan.py` finds unusual C/C++ constructs that deserve a
credibility or semantics review. It is a heuristic lead generator, not an
object comparator, admission gate, or proof that source is fake. A byte match
does not make nonsensical source credible, and a scanner hit does not make
ordinary source wrong.

These examples run from the repository root (the script also derives the
repository root correctly when invoked by an absolute path from elsewhere):

```text
python tools/fake_match_scan.py
python tools/fake_match_scan.py source/ai/actions.c source/game
python tools/fake_match_scan.py --format json --fail-on-findings source/ai
python tools/fake_match_scan.py --list-rules
```

The default input is the repository's `source/` directory. Explicit file and
directory inputs are de-duplicated and emitted in deterministic path/line/rule
order. Known vendored libtiff, zlib, `third_party`, and `vendor` trees are
skipped unless `--include-vendored` is supplied. Comments and string/character
literals are masked before scanning so examples and assert text do not become
findings.

Exit status is deterministic:

- `0`: the scan completed; findings may be present.
- `1`: findings are present and `--fail-on-findings` was requested.
- `2`: an input is invalid or a source file cannot be read.

Every finding should prompt a source review, not an automatic rewrite. Check
whether the construct expresses plausible original program intent, whether the
logic is meaningful for all inputs, and whether first-party source, names,
target CFG/relocations, or a credible contemporary codebase supports it. Re-run
the strict object and regression gates after any resulting edit. Exact bytes
alone are not provenance.

Semantic validity is an admission requirement even when every byte matches.
Reject code whose behavior is nonsensical, whose conditions or data flow are
meaningless, or whose source shape almost certainly would not have existed in
the original program. A coincidental exact result must be reverted or replaced
with the best honest fuzzy reconstruction and parked with its evidence.

The rule set intentionally omits several noisy shortcuts. It does not condemn
ordinary `volatile` declarations, generic representation casts, `!!value`, or
`value != value`; those can express hardware synchronization, serialization,
boolean normalization, or a NaN test. It also performs no data-flow proof, so a
clean scan cannot establish that variables are initialized or that logic is
sound.
