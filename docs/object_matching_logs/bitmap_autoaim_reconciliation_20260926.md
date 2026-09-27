# Bitmap allocation and autoaim-pill reconciliation

Canonical baseline: `d946958ec1739200364f003b281f7310ddadfb32`.
Donor: `claude/remaining-frontier-20260926`, source commits
`8202ac1355b7732a948c9887223b4066ff47f148` and
`a448c5954dddbe12057e6883fa8d9bb4de9cf07e`, research read at `9857c608`.
Only these two source changes and their two park removals are reconciled.
Donor RF-X/RF-Y work remains active and untouched. No object admission or push.
Canonical source/park commit: `a6d8572861f7c5c54a0dfca4ba84bde5547dce00`.
Backup ref `backup/astra-before-bitmap-autoaim-20260926` retains the baseline.

## Fresh canonical gains

| Function | Meaningful | Padded | Relocations |
|---|---:|---:|---:|
| `_rasterizer_bitmap_new` | 388 | 400 | 26 |
| `_biped_get_autoaim_pill` | 326 | 336 | 9 |
| Total | 714 | 736 | |

Halo ledger **1,595,220 -> 1,595,934 / 1,770,166 (90.16%)**;
functions **7,465 -> 7,467 / 7,574**. Whole-board strict owner sweep
**7,637 -> 7,639 / 8,252**, exactly these two gains, zero losses.
Meaningful extents and the ledger agree for this batch; previous ledger
padding caveats remain unchanged. Data stays **2,588,903**; objects **389/468**.

The bitmap unit reaches **19/19 strict-exact functions**. The admission tool
adds it as a candidate, **11 -> 12**, not an automatically completed object.
RF-Y's separate storage/ownership/link/object audit is not presumed complete.
No `Matching` status is changed. Bipeds has **44 exact / 7 residual** functions.

## Source review and limits of the evidence

Both changes are ordinary, semantics-preserving C source repairs with no
new helper, ABI, type, cast, compiler control, macro or original-bug exception.
Independent reviewers read the raw later `/Od` executable as data, SHA-256
`740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`.
Cross-build code is corroboration, not uniquely authenticated January C.

### Bitmap allocation

The existing switch is only re-indented. The device arm checks the hardware
pointer and clears it on failure; the no-device arm only clears it. The common
tail reports failure and returns `success`, which already starts TRUE. This
preserves the no-device success behavior and failure diagnostics while replacing
the early return. No upstream `D3DCALL` macro or redundant TRUE store is added.

Raw `/Od` at `0x7ea7d0`: hardware check `+0x2e4`, FALSE store `+0x2ea`,
failure clear `+0x2ee..+0x2f9`, no-device clear `+0x30e`, common result load
`+0x318`, sole return `+0x33b`. It has different format-validity/per-case logic
and **no January error() tail**; those differences are not imported.
January's final conditional jump at `+0x159` targets `+0x171`; hardware clear
is at `+0x162`, error call `+0x169`, no-device copy `+0x177`.

Placement of the authentic error call after the join remains an inference from
January's code and natural control flow. Neither later `/Od` nor retail decides
that placement. The single exit and shared error placement close only together;
B6/B7 failed controls are preserved. This is not an inert filler statement or
invented branch, and requires no broadening of the decorative-source holds.

### Biped autoaim

One width assignment after the outer if/else becomes one at each outer-arm end.
Every execution still performs one identical load/store after the same work.
The first remains shared by the two inner arms; the earlier Lane B `aa1`
control instead placed stores in all three innermost arms and did not match.

Raw `/Od` at `0x8bffb0`: spherical jump `+0xd7` enters the shared width block
`+0x10c..+0x118`; `+0x11a` skips to the epilogue; the physics arm has its own
width block `+0x176..+0x182`. January independently corroborates the expected
value/pointer registers at spherical `+0x9a/+0xa0/+0xa5`, nonspherical
`+0xe4/+0xde/+0xeb`, and physics `+0x138/+0x110/+0x13f`.

**Qualification to donor claims:** `/Od` does not universally guarantee that
statements are never merged or duplicated. Here the observed first-party CFG,
natural source topology and stock-compiler match support the repair; original
January braces/statements are not uniquely proven. Earlier-build equality is
stability of the normalized instruction stream, not proof of unchanged source.
V01's failed global-allocation prediction is retained. V05 captures the shared
block after allocation, but later cloning is inferred from final stock-equal
output, not directly witnessed at the cloning operation.

The existing scheduling parks permit a natural compatible-compiler context
explaining their residuals. Both are retired after fresh verification. No
separate per-function owner rejection was found; other biped, duplicate-branch,
UB and macro holds are unchanged.

## Verification and preservation

- Full Ninja before/after and fresh affected-TU compiles, followed by independent
  baseline/candidate `/W3` compilation. All non-debug sections reproduce.
- Full comparison of **46 bitmap sections / 206 biped sections**: only each
  target text section changes. Same section counts/flags/storage; no added or
  removed code/data/COMMON owners or helpers. Existing surplus copies do not
  change; no new provider-link exception is required. Complete relocation and
  padded-section identity is verified, not just the function score.
- Warnings: bitmap **14 -> 14**, bipeds **15 -> 15**, no new warnings.
- Pytest before/after: **1,161 passed, 5 skipped, 26 subtests passed**.
- Parks **74 -> 72**, zero stale/invalid. Admission: **12 candidates / 0
  contradicted / 1 rejected / 0 revoked**. Same **26** fake-scan findings by
  path/rule/snippet. Source/config whitespace checks pass.
- No header, compiler flag, scorer, normalization, symbols.json, production test
  or object-status changes. Pre-existing README and seven untracked research
  directories are preserved and excluded from this batch.
- Fresh receipts are in `scratch/reconcile_bitmap_autoaim_20260926/`.
  `research/astra_bitmap_autoaim_reconcile_20260926/verify.py` reproduces the
  section/storage/warning/receipt checks; `RESULTS.json` pins inputs and outputs.
  Selected donor reports, controls and checkpoint R4 are preserved under
  `donor/`. Their pre-commit checkpoint head is not substituted for our fresh
  canonical measurements. Private binary/compiler/SDK assets stay local.
