# Path obstacle avoidance admission, 2026-09-27

Canonical baseline: `a567739bfc8e363e846669095f1dbdd542d902a0` (the preceding
collisions static-local repair; code-identical to `ef79aeda`). Reconciles only
the source-comment portion of donor `cb25c573e477d1bf92555b965a1b076a8ed094a3`
and the status row from `6d9f88550e6e0aa8bd91fe0369229df9d91dcafa`.

## Authority and scope

The 2026-09-27 Q-A3-1 ruling admits this object with the two names
`debug_path` and `debug_obstacles` explicitly disclosed as descriptive, not
recovered. It requires January address/layout ownership evidence independent
of those labels. This is not a general waiver for unnamed globals.

The twelve obsolete address-based names in the source's symbol-list comment
now use the real identifiers already in production and `symbols.json`.
The BSS comment and definition comment disclose the two inferred names.
No executable source, shared header, symbol configuration, compiler flag,
scorer, ABI or helper emission changes. The older held A3 code-shape patch is
not imported. Only this object's status becomes `Matching`.

## Independent current-object evidence

- All 24 functions are strict exact; all 49 January-owned sections and 52
  symbols pass bytes, relocations, sizes, alignment, flags, offsets and storage.
- The 0x2142-byte BSS is verified without comparing private identifier text:
  static objects at +0 and +0x1538, public flags at +0x2140 and +0x2141;
  all 15 referencing code relocations resolve identically. The two copy sizes
  and multi-function uses corroborate types and file scope. A label-renaming
  control still passes; an altered-addend and unrelated-object control fail.
- The source changes compile to identical nondebug sections and symbol
  records. Only scratch output-path debug metadata differs. Warning multisets
  are unchanged: 12 at /W3 and 51 at /W4.
- All 21 surplus code/data COMDATs (15 code, six data) match both January's
  selected and current providers. No COMMON. Five provider pairs pass all ten
  duplicate-definition probes in both orders. These unresolved-only probes
  are bounded link-collision evidence, not a complete executable link.
- January's synthetic split models `_obstacles_get_disc` and
  `_valid_real_point2d` with selection 1, while our inherited identical shared
  inline copies have selection ANY (2). No selection field changed in this
  packet. Provider identity and duplicate tests are recorded, not inferred
  merely from equal function bytes.

The unchanged `error_heap` local real/long union was explicitly retained by
the 2026-09-08 owner reconciliation and cited as an existing accepted form in
the 2026-09-15 wind ruling. January directly reads the same spilled real as
integer bits and as a floating value for the hexadecimal/decimal diagnostic.
This packet neither introduces the union nor uses Q-A3-1 to waive an unrelated
hold. The later A3 confirmation question and the existing provenance are
recorded in the independent report. Its six existing argument views likewise
remain under their recorded per-site attestation and byte-inert controls.

## Full canonical gate

- `ninja all_source progress build/report.json` passes, objdiff 3.3.1 unchanged.
- 8,252 stable owner rows / 7,641 exact: zero gains, zero losses.
- Parks: 71 active, zero stale or invalid.
- Admission: 12 candidates, zero contradicted, one rejected, zero revoked.
- Fake-match scan: unchanged 26 inherited leads.
- Tests: 1,161 passed, five skipped, 26 subtests. Diff whitespace check clean.
- Halo complete objects: **389 -> 390 / 468**.
- Halo meaningful code unchanged: **1,598,242 / 1,770,166**, 7,469 accepted
  functions. Halo credited data unchanged at **2,588,903**. No new byte credit.

Focused receipts: `scratch/astra_object_closeout_20260927/path_obstacle_avoidance/`.
Full gates: `scratch/astra_object_closeout_20260927/gates/path_*`.
The independent report includes exact owner-ruling citations, the preserved
failed harness control, all input/provider hashes and final integration checks.

No donor worktree was changed. Inherited README edits and unrelated research
remain outside this commit. Publication is separate from this local admission.
