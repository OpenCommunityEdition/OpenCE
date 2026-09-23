# Lane C canonical reconciliation (2026-09-22)

Canonical base: `a01115aaf586e6e7386d873ce935e27c41e84e85` on
`jonas/exact-pilots`. Donor: `claude/lane-c-graphics-ui-20260920` at
`a5fd4a51aa76ad418c322c1d94d19b5d0c76f884` (60 local commits). This is
an audited net integration, **not** admission of all 60 donor commits.

## Verified result

- Stable function sweep versus `scratch/post-lanes-ab.json`: 8,245 functions,
  7,470 -> 7,493 exact; 23 gained, **zero lost**; 17,056 newly exact padded
  code bytes. `tools/campaign/stable_verdicts.py` identified every row.
- Halo meaningful code ledger: 1,479,452 -> 1,498,567 / 1,770,166.
  This ledger also reflects the comparator upgrade, so its 19,115-byte delta
  must not be represented as 19,115 newly reconstructed function bytes.
- Halo credited data: 2,378,791 -> 2,642,490 / 3,923,451. The large delta
  includes the objdiff v3.3.1 -> v3.6.0 accounting correction, not just new
  source. `hs` adds a separately verified 54,780-byte data closure.
- Full 833-unit build passed. Pytest: 1,152 passed, 5 skipped, 26 subtests.
  Parks: 151 active, zero stale/invalid. Admission: zero contradicted or
  revoked. `linker_common` remains partial and receives **zero** credit.
- The explicitly allowed `_point_from_line3d` copy in
  `rasterizer_lights.obj` is byte-identical to January's folded survivor in
  `action_charge.obj`; January's rasterizer-lights object imports it. The
  runtime test checks both conditions.

## Reconciliation exclusions and corrections

- Excluded the 116 tentative COMMON definitions: pool adjacency cannot prove
  individual TU ownership. An existing owner test refuted the claimed
  `rasterizer_frame_statistics.c` owner. The later `hs_syntax_data`,
  `ai_globals`, and `rasterizer_model_cortana_hack` moves were likewise not
  carried forward. The pool-ordering document now states this limitation;
  partial-base diagnostics remain research only.
- Excluded held `main_crash` null-write source, `_fast_ftol_C` pragma/asm,
  the two-pass `_real_argb_color_to_pixel32` asm, and the lens-flare reset's
  34-byte overrun. The latter was unsafe in the rebuilt object: COFF places a
  different array immediately after the overwritten one, so the donor's
  claimed adjacency assertion did not pin its actual consequence.
- Excluded the address-named, unreferenced cheats global from symbol credit;
  marked `cheats.obj` nonmatching instead of presenting 3,201 unproven data
  bytes as complete. Removed an invented bitmap-drawing global that regressed
  the already-exact 2,784-byte `_bitmap_copy`.
- Removed a duplicate `king_engine` definition introduced by the merge.
  Preserved the canonical AI communication and infection bodies where they
  were already better. Moved consumer-local declarations for
  `pin_normal_to_cone3d` and `object_list_from_ai_reference` to their owning
  headers. Disclosed the January-attested interface locals and unrecovered
  numeric stall labels in source.
- Rebased `_ai_test_ballistic_line_of_fire` as a **fuzzy** park after shared
  declaration context changed its hash. No exact credit was assigned.

The donor's earlier handoff and admission manifest remain available only as
archival, lane-local evidence; they were incomplete at the final 60-commit
tip. The source/build/stable-sweep result above is the canonical claim.
