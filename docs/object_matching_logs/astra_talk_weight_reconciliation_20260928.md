# Talk-weight reconciliation — 2026-09-28

Canonical base: `5a83a7234a90af7462f5996b58a995dc5d1abbce`.
Donor: `5668ca59cba0e1c3730d6b243c438006a26234da`, from
`claude-remaining-frontier-20260926`. Reconciled only the three source hunks
and the accompanying park retirement. No B3, header, symbol, scorer, data,
object-admission or Q10 change is included.

## Result

`_ai_communication_actor_talk_weight` is newly strict exact:
**904 meaningful / 912 padded bytes**, 24 relocations. Target and rebuilt
normalized SHA-256:
`9e4e417453eb75eb5f5a4d4fa433f75006e5a17ba9c8c84f3921a9d0cb108279`.

The previous parked hash was
`07a6f84aa77fc41947a0d6e66f45dd0aae00a15f97e232644cfcda13f102dbc8`,
also 912 bytes / 24 relocations. That park is retired in this same packet;
its complete historical evidence remains in the parent commit.

| Measure | Before | After |
| --- | ---: | ---: |
| Whole-board strict exact / 8,252 | 7,641 | 7,642 |
| Halo credited meaningful code / 1,770,166 | 1,592,751 | 1,593,655 |
| Halo credited functions / 7,574 | 7,468 | 7,469 |
| Halo complete objects / 468 | 394 | 394 |
| Halo credited data / 3,923,451 | 2,648,123 | 2,648,123 |
| Valid active parks | 71 | 70 |

The strict census and credited-function population are different measures.
There are **zero exact losses**, no denominator changes and no vendor gains.
`ai_communication` improves from 46/48 to 47/48; `_ai_communication_event`
remains residual and the object remains NonMatching.

## Source credibility and limits

The later first-party `/Od` function at `0x48ca30` has four separate subject
TRUE stores and three cause TRUE stores. The independent read of the supplied
executable confirms subject initialization at `0x48cc84`, cause initialization
at `0x48cc88`, and the distinct condition branches. The patch restores those
chains while keeping January's existing operations and call structure.

The `/Od` initialization stores alone do **not** uniquely establish separate
source statements: the old chained assignment can also produce those stores.
The subject-first spelling is a reconstruction supported jointly by the
compiler first-reference trace, the `/Od` local/branch evidence and January's
strict match, not recovered January source text. Known later-revision distance
helper and local-name differences were not imported.

RF-DQ's C07 strip evidence shows the subject-first introduction and subject
chain are jointly load-bearing; the cause split is byte-inert but `/Od`-attested.
The user reopened this bounded packet subject to full gates and zero losses.
No function move is used; the old R1 move remains held. There is no assembly,
new macro, decorative parenthesis, filler declaration, cast, or helper copy.

## Independent canonical checks

- Fresh baseline and final `ninja all_source progress build/report.json`: pass.
- Same 8,252 target-section identities: one gain, zero losses.
- `_ai_communication_update_speech_timers`, the historical collateral risk,
  remains strict exact at 672 padded bytes.
- Inventory hashes: only `build/base/source/ai/ai_communication.obj` changes;
  every other configured built object and every split object stays unchanged.
- Within that object, only talk-weight's resolved code section changes.
  Twenty-one compiler-internal `$L` labels renumber; their section, offset,
  type and storage are unchanged, as are every relocation's resolved identity.
- Named symbols, COMMON, undefined imports, data, section flags/alignment,
  helper bodies and the 42 pre-existing surplus sections are unchanged.
  No new provider-link exception is requested; this is not a whole-program
  executable-link claim.
- Owned-symbol audit: no storage discrepancies; only the pre-existing
  `_ai_communication_event` section remains non-exact.
- Same-path `/W3` control/candidate: 12 identical inherited warnings.
- Parks: 70 active, zero stale, zero invalid.
- Admission audit unchanged: seven review candidates, zero contradicted,
  rejected or revoked entries. No admission is inferred from this result.
- Fake-source scan: the same 26 inherited leads, none added.
- Pytest: 1,318 passed, five skipped, 100 subtests passed.
- `git diff --check`: clean. Objdiff stays 3.3.1; binary SHA-256
  `090987aa22c0fe9b7d252b2b44c2c0c92c5dd3e9b5965d353060802226a13677`.

Local reproducibility receipts, generated objects, the independent `/Od`
readout and bounded audit driver are under
`scratch/astra_talk_weight_20260928/`. Private executables and compiler assets
are not published. Existing README/research changes were preserved unstaged.

## Explicitly outside this reconciliation

B3 is not split to harvest its two gains; its losses and ownership corrections
remain a separate decision. Flags admission and name-only packets are not
included. RF-DP's Q10 review and WIP patch were inspected, not applied: its
missing-input receipt/exit handling is unfinished and Q10 remains held.
