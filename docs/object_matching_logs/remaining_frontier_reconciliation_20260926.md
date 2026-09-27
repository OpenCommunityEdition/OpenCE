# Remaining-frontier two-function reconciliation

Canonical baseline: `8cda1f91f3a037f52ac2eed551936b03df6796c1`.
Donor: `claude/remaining-frontier-20260926`, initially read at research-only
`b62f74c1` with these two uncommitted source changes. During independent review,
Opus completed its clean-build checkpoint and committed them as
`9ed00bf758887d22ee67f9d1e8add23f317b2fc1` and
`9f0b3af9413e6dc5c1be28786d22f0ea7df6c119`; its evidence tip is
`7d1b038edcd80864dddee8089da0a6851a3a2318`.
The two canonical source files equal those donor files. The donor was read-only
throughout reconciliation. No push was requested or performed.
Canonical source commits: `9c9118d1` (actor path) and `81a11ff9` (scenario help).

## Admitted source and fresh results

| Function | Non-tail-padding extent | Padded extent | Relocations |
|---|---:|---:|---:|
| `_actor_path_refresh` | 1,428 | 1,440 | 52 |
| `_display_scenario_help` | 596 | 608 | 50 |
| Total | 2,024 | 2,048 | |

The path change recovers a live decision flag, not a dummy initialization.
An independent read of the first-party later `/Od` executable confirms the TRUE
store before the destination test, conditional overwrite, and use guarding the
path rebuild (`0x46cbc1` through `0x46cc2b`). `build_path` is a descriptive name,
not a claim of recovered scalar debug names. Short-circuit behavior is preserved.
The unused-local negative control from the donor did not close the residual.

The UI change restores the null-and-type assertion matching the diagnostic's
contract. It is redundant with the preceding search but independently attested
at `/Od` `0x65fe16` through `0x65fe2c`; short-circuiting protects the dereference.
January's file, line 2438 and message remain unchanged. The later build's line
2720 and different scenario-name lookup are disclosed, not imported. These
observations corroborate plausible source; they do not uniquely prove January's
source text. No new exception, helper, macro, cast or hold reversal is involved.

### Five-byte reporting discrepancy (not additional reconstructed code)

The unchanged objdiff 3.3.1 report gives `_actor_path_refresh` size **1,433**,
which the existing accepted ledger imports as `code_bytes`. The actual target
section has twelve trailing `0x90` bytes at `0x594..0x59f`; its non-tail-padding
extent is **1,428**. A linear instruction decode runs through its jump-table
data and into five of those padding bytes. This batch does not modify the
scorer/comparator or claim those five bytes as meaningful source progress.

Accordingly the official frozen ledger rises **2,029**, from **1,591,710 to
1,593,739 / 1,770,166 (90.03%)**, while the actual new non-tail-padding extent
reported above is **2,024**. Both full padded sections, including their tables
and padding, are strictly identical under the existing hardened comparator.

## Independent canonical gates

- Fresh full Ninja before/after; the two changed TUs recompiled in canonical.
  Opus's separately retained R1 checkpoint also completed its full clean build.
- Whole-board strict sweep: **7,633 -> 7,635 / 8,252**, precisely the two gains,
  **zero exact losses**. Halo ledger functions **7,461 -> 7,463 / 7,574**.
- Full per-section comparison of both TUs (151 and 372 sections): only each
  target function changes, excluding compiler debug records. Local `$L` labels
  renumber; their resolved relocation destinations are checked. No code/data/
  COMMON owner, external/static storage contract, section flag or helper changes.
- Fresh diagnostic `/W3` compiles of baseline and candidate: actor_moving
  **12 -> 12**, ui_widget **17 -> 17**, **zero new warnings**.
- Tools pytest before/after: **1,161 passed, 5 skipped, 26 subtests passed**.
- Parks **75 active / 0 stale / 0 invalid**. Neither target was parked; no park
  entry is removed. Admission **11 candidates / 0 contradicted / 1 rejected /
  0 revoked**; fake scan **26 inherited leads**, unchanged.
- Halo objects remain **389 / 468**; data remains **2,588,903**. Neither object
  is newly complete. No provider-link exception or fresh surplus helper exists;
  the prior providers and their code/relocations remain unchanged.
- Production source, verification script and this ledger pass whitespace checks.
  Raw copied donor evidence retains its original formatting, including the
  context-prefix whitespace required by its two unified-diff patches (excluded
  from the source whitespace claim). No header, config,
  scorer, production test or build flag is changed. All existing holds remain.

## Reproduction and preservation

`research/astra_frontier_reconcile_20260926/verify.py` checks full sections,
storage and `/W3` diagnostics against the saved pre-edit objects. Local receipts
are in `scratch/reconcile_frontier_20260926/`; compact results and selected donor
records are archived under `research/astra_frontier_reconcile_20260926/`.
The donor's other research and held candidates remain in its own worktree,
untouched. Canonical's seven inherited untracked research directories remain.
No game executable, PDB, SDK/compiler binary or object file is added to Git.
