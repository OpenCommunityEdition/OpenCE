# Data and fidelity reconciliation — 2026-09-29

Canonical base: `f3aadeab984b09739d80aacfa877cb5aea51a145`,
`jonas/exact-pilots`. Donor production inputs are pinned to
`1940c840afed190756f9f456301c99aed418bab4`, not its moving branch tip.
This reconciles the Sep-29 addendum and its still-unreconciled Sep-28
prerequisites. The user asked for reconciliation, not a new publication;
these commits are local. Donor trees, the existing README edit and unrelated
untracked research were not changed.

## Result, measured against canonical

| Measure | Before | After | Classification |
| --- | ---: | ---: | --- |
| Halo credited code / 1,770,166 | 1,596,429 | 1,597,714 | +1,285 meaningful, recovered debit |
| Halo exact functions | 7,472 / 7,574 | 7,473 / 7,573 | +1 match; separately −1 phantom denominator row |
| Halo credited data | 2,648,123 / 3,923,451 | 3,311,591 / 3,922,163 | +663,468 credit; separately −1,288 denominator |
| Complete Halo objects / 468 | 396 | 396 | No status flips |
| Stable strict owners / 8,252 | 7,645 | 7,646 | One gain, zero regressions |
| Active valid parks | 68 | 68 | No stale/invalid entries |

The only exact-function gain is
`__rasterizer_model_transparent_geometry_submit`: 1,285 meaningful / 1,296
padded bytes. K048r2 (`23c8082b`) recovers an approved B3 debit through VC7
compiler numbering. It is not a new source-body reconstruction. The diagnostic
37-name filler did not land. `_bitmap_copy` remains the unrecovered B3 debit.

Data credit is separately attributable:

| Packet | Raw | Padding | Credited |
| --- | ---: | ---: | ---: |
| Original Q10 85 records | 657,245 | 311 | 657,556 |
| Q10 EXT1 eight records | 3,400 | 56 | 3,456 |
| actions extent entry | 2,338 | 66 | 2,404 |
| main single-section entry | 52 | 0 | 52 |
| **Total increase** | **663,035** | **433** | **663,468** |

The already-credited hs 54,780 bytes and main .rdata are not claimed again.
The data percentage becomes 84.43%; code is 90.26%. All-category totals are
1,664,450 / 2,198,102 code and 3,317,905 / 4,176,062 data. Vendor code gains: zero.

## Bounded source and storage intake

- Corrected F_ALLH_R (`b008c540`), T01 (`9fb39730`), C06b (`162474a9`)
  and C07 (`4320e7db`): later-build names/types retain their evidence limits.
  The corrected 144-name packet includes eight single-PDB names, three also
  supported by /Od; five lack that second witness. No January-name claim.
- K031/K059/K060/K061 (`daaa1fdb`) stay one coherent packet. Their joint
  ballistic-park fingerprint is unchanged; no speculative refresh was made.
- K048r2 retains the shared unsigned `byte nodes[2]` and January's existing
  signed consumer view. No yield-selected filler or alternative header placement.
- P-S1 (`8b7e1d1c`) replaces hand-expanded math with the attested helpers;
  P-S2 (`03fc26b6`) retains the closer sound-cache residual and its park history.
- T1 (`c094b6dc`) restores the real `ui_widgets_active` call and the attested
  body. T1b's optional initializer was not taken.
- DA_P1 (`773f9f8b`) retains the August-attested spot body and removes the
  TU-local scale helper suppression. Spot remains non-exact; Q5 lightmap stays
  held. January's four lightmap references authenticate the selected scale
  provider under this packet's specific approval, not a generalized
  non-exact-caller exception.
- set_random_seed S (`085475b6`) takes both attested call sites together.
  Game's former NODUP copy becomes ANY; periodic_functions emits an identical
  ANY copy. The reconstructed January split's NODUP selection is not treated
  as proof that the original source used a non-inline definition.
- C05 (`c217c735`) keeps three explicit perceptions initializers and an
  implicit zero fourth. C09 (`1940c840`) removes redundant product parentheses.
  **C09 is admitted as ordinary cleanup under the user's explicit clarification.**
  History establishes reconstruction origin, not that they were inserted as
  scaffolding. Render-actor remains non-exact and earns zero credit.
- COMMON: CV-A′, CV-S, P-HS′, CV-B′, Q-B2 and CP-T B1/B2/B4/B5/B7,
  from `a548d62d`, `2bf71228`, `90f77761`, `0dcdf006`, `c7698d7f`,
  `6d1040e6`, `cfe97640`, `16a33aad`, `b6d60163`, `d23568d5`.
  These produce 52 new tentative definitions relative to canonical, with no
  initialized storage or section-byte changes from the definitions themselves.
  Ownership remains inferred under the narrow rulings; it is not certified
  January ownership. B7's literal allocation size and label renumbering remain
  disclosed. B7 and debug_sound_channels remain outside Q10 credit.
- `_scenario_paths` split name (`55829429`) and phantom jump-table row
  removal (`8220b041`) are metadata/accounting, not new source-byte matches.
- The biped-discard and endpoint-poll park text corrections preserve the holds
  and measurements. They do not admit the held exact variants.

Canonical's virtual-keyboard helper/call and static symbol row, both ordered
physics water-depth predicates, and the two canonical physics park histories
are preserved. The unrelated donor libtiff self-alias macros are excluded.
The source intake therefore is not a blind donor-tree replacement.

## Data verifier and accounting boundary

The reviewed actions surplus-helper rule (`a448602a`) and entry (`4b9fc69d`)
require January's own import and a unique authenticated provider, identical
helper/constant contents, valid relocation addends, select-any storage, no
weak/alternate aliases and the retained guard. Only January-owned data is
credited, not surplus helper code/constants. Main's 52-byte entry is `aa8129c0`.

Q10 production is `9ab48dc8` with EXT1 `4551c4e9` and wording `a3d84118`.
The four LF-normalized pins are unchanged: verifier, COFF reader, provenance
ledger and XDK library manifest. The complete mutation catalogue/harness is
preserved, rather than silently letting missing-fixture tests skip.

Required scope, verbatim: "This lifts Q10 only for those 85 records and the 8
records of allowlist extension EXT1 under this verification contract. It does
not certify January ownership, complete pool layout, or a matching executable
link."

Q10 credits 93 records: 660,645 raw + 367 padding = 661,012. By tag:
64 inferred records / 11,884 bytes; 29 probable records / 649,128 bytes.
`_render`, tagged probable, contributes 643,744 bytes (97.4%). Its later-build
type drift remains disclosed; this is bounded COMMON storage credit, not
proof of the whole record's January source type or initialization behavior.
24 owner-unresolved records, 94 FAIL records, the two unlisted PASS records,
27 vendor records and two linker records remain uncredited by Q10.
`source/linker_common` stays MISSING.

The denominator packet (`38f82c59`) reassigns 29 non-Halo pool records:
libcmt +880, xapilib +308, dsound +4, linker +96. Halo decreases 1,288;
the all-category denominator stays 4,176,062. This is accounting only.

Limitations are intentionally not silently repaired:

- `binkxbox.lib` and `dsstrmh.lib` are not in the authenticated XDK extract.
- The current entry expects `C:/tmp/xdk3911_extract/XDK/xbox/lib`; all 38
  manifest libraries were freshly size/hash verified. Another machine needs
  those private prerequisites or a separately reviewed path adaptation.
- The hook trusts the freshly invoked pinned verifier. A substituted runner
  can still replay a compatible stale PASS against different built objects;
  direct receipt-to-current-object binding is the held RF-EZ hardening work.
  The ordinary pipeline deletes prior outputs and reruns the verifier.
- Pins do not secure a compromised Python/import environment, unpinned hook
  or generated outputs. Provenance is a reviewed trust root, not rediscovered
  by the verifier. The frozen allowlist is enforced by its tests and review.

## Independent verification

- Source-only checkpoint, complete tooling checkpoint and a clean rebuild
  after `ninja -t clean -r cl`: same single strict gain and zero exact losses.
  Cold/current sections and COMMON inventories agree in all 467 Halo objects.
- Baseline/current section census: only six units change sections (ai_debug,
  xbox_sound_cache, ui_widget, periodic_functions, xbox_environment,
  xbox_models). Game additionally changes set_random_seed selection metadata.
- All 56 surplus helper/literal/table copies in sound/environment/game/periodic,
  plus game's owned set_random_seed, match January and current providers.
  38 provider pairs / 76 orders: expected unresolved-only exits, no duplicate
  or unexpected diagnostics; no images emitted. Periodic object audit PASS;
  other differences are precisely their documented residuals.
- Whole-board diagnostic link: unresolved names 471 -> 419, exactly the 52
  new COMMON definitions; no new unresolved or duplicate names. One inherited
  duplicate remains. Both attempts fail; this is not a playable-build claim.
- Full `/W3 /Zs`: 4,067 -> 4,071. Only P-S2's one long-to-short conversion
  and T1's three bounded 0..3 conversions are added. No warning-silencing casts.
- Source and data policy reviews found no additional blocker under the
  approved scopes. RF-EV's independent COFF reader/extent model passes 39/39
  checks against a freshly executed original-85 control and the final93 entry.
  The control varied only the entry path, not verifier code or runner; the
  production files were never temporarily rewritten for that experiment.
- Final tools suite: 1,663 passed, five skipped, 159 subtests passed. Parks:
  68 valid, zero stale/invalid. Admission: seven candidates, zero contradicted,
  rejected or revoked. Fake scan: unchanged 26 leads. `git diff --check` clean.
- Objdiff remains 3.3.1, SHA-256
  `090987aa22c0fe9b7d252b2b44c2c0c92c5dd3e9b5965d353060802226a13677`.

Receipts, scripts, baseline objects, full diagnostics and independent reviews
are retained in `scratch/astra_data_reconcile_20260929/`; a compact receipt
index accompanies this ledger. Private executables, objects, PDBs and SDK
libraries are not committed. The 51-set, paren class P1/P2/C07/U, BW-P3,
s7, biped A1, K067 and receipt-binding hardening remain research only.
