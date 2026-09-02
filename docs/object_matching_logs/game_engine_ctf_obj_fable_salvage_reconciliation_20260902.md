# `game_engine_ctf.obj` Fable salvage and reconciliation (2026-09-02)

## Scope and immutable inputs

- Frozen canonical base:
  `fca0dd5d7103a7cb331106c61a86dc26e8c84356`.
- Preserved Fable snapshot: `d0b198b92`.
- Snapshot-only integration commit:
  `9c8e78e2ae8e26dce16c4d456fcd6bffe25a7c7c`.
- Translation unit: `source/game/game_engine_ctf.c`.
- Target authority: the January 2002
  `build/split/source/game/game_engine_ctf.obj`, SHA-256
  `7dad89f40b3c4206575064e98337ec7019fc243e2022e31f6585a4d6018b1baa`.
- Compiler authority: XDK 3911 `CL.Exe` 13.00.9254.1 with the generated
  `/O2 /Oy- /DDEBUG /Dxbox` flags.

All work was isolated from canonical.  No shared configure, full Ninja build,
push, or unrelated source edit was performed.

## Evidence read before reconciliation

- Repository methodology, acceleration, campaign-throughput, fake-match, and
  object-ledger documentation, including the prior CTF public-leaf and Fable
  worker ledgers.
- The preserved Fable worker log, target disassembly, and the complete bounded
  weapon-update variant matrices in the original Fable scratch tree.
- January target bytes, relocation topology, assertion strings and source-line
  anchors.
- HaloCEA's independently reconstructed CTF functions and data definitions.
  They authenticate callback/helper names and behavior but are not treated as
  byte authority.  In particular, the later CTF player-update source preserves
  the otherwise suspicious discarded `game_engine_get_variant()` call, and
  the later weapon-update source corroborates the unsigned elapsed-time test
  and the full objective-weapon behavior.
- The HCEX/PDB `_ctf_engine` global label and the January table layout.
- Bungie's released Marathon 2/Infinity source repository was reviewed as the
  requested naming/style precedent.  It confirms the older Bungie C/cseries
  lineage and multiplayer CTF identifiers, but supplies no interchangeable
  Halo CTF body; no code was copied from it.

## Frozen baseline and retained Fable gain

The canonical source gate was frozen before importing Fable:

| state | exact | residual | unwritten | strict exact padded code |
| --- | ---: | ---: | ---: | ---: |
| canonical `fca0dd5d7` | 17 | 0 | 22 | 480 / 5,328 |
| raw Fable snapshot | 38 | 1 | 0 | 4,752 / 5,328 |
| reconciled source | 38 | 1 | 0 | 4,752 / 5,328 |

The strict gain is **21 functions**, **4,272 padded code bytes**, and **4,115
meaningful code bytes**.  The reconciled exact set contains 4,364 meaningful
bytes versus the baseline's 249.  The remaining function has 566 meaningful
bytes in a 576-byte padded section.

An address-normalized sweep maps every old `_code_<address>` baseline owner to
its semantic symbol at the same January image address.  All 17 accepted
baseline functions retain the same target padded size, normalized hash, and
relocation count, and all 17 remain strictly exact in the candidate.  This is
the regression oracle; semantic renaming is not mistaken for symbol loss.

## Semantic names and linkage

Every CTF address label was replaced in both source and `symbols.json`.
Names come from the January engine-table slots, HaloCEA names, and the
authenticated behavior documented in the earlier ledger.  Table callbacks
and same-TU helpers are `static`, matching the established
`game_engine_slayer.c` ownership model.  The unreferenced
`ctf_state_message_update_warning` stub remains externally linked: XDK C2
eliminates an unreferenced static definition, which would falsely remove the
January 16-byte function.

The formerly anonymous 48-byte BSS owner is now the semantic TU-private
`ctf_globals`.  This linkage is supported by its only-local reference graph
and the absence of a public PDB label, unlike the PDB-authenticated exported
`ctf_engine` table.

## Prototype ownership and the C2 header hazard

The Fable worker temporarily declared five borrowed APIs in the consuming
`.c`.  Their declarations now live in narrow owner headers:

- `source/game/game_engine_runtime.h` owns `game_show_score_team`,
  `game_show_score_you_ally_enemy`, and `game_engine_flag_reset` from
  `GAME_ENGINE.C`;
- `source/game/players_runtime.h` owns
  `player_get_starting_location_count` and
  `player_get_starting_location` from `PLAYERS.C`.

This is deliberate rather than cosmetic.  A frozen A/B sweep compiled all 47
report-resolvable direct consumers of broad `game_engine.h`/`players.h`.
Putting the declarations in those broad headers changed XDK C2 definition
context and regressed the 1,920-byte
`units::_unit_preprocess_node_orientations` from exact.  Those edits were
reverted; both broad headers are byte-identical to the base.  Narrow
owner-runtime headers follow the repository's existing
`player_control_runtime.h` precedent and affect only CTF.

## Exact data ownership and semantic accounting

The snapshot omitted both runtime data owners.  The reconciled source now
defines them naturally:

| owner | size | relocations | normalized SHA-256 | result |
| --- | ---: | ---: | --- | --- |
| `_ctf_engine` | 136 | 28 | `45c41730d038fd3f985f7ccfd8a7e04aa02e2db549ac941386e7cd04c1e1d8c1` | strict exact |
| `_ctf_globals` | 48 | 0 | `17b0761f87b081d5cf10757ccc89f12be355c70e2e29df288b65b30710dcbcd1` | strict exact |

This adds **184 exact data bytes**.  Ordinary objdiff already sees the BSS and
all 504 target read-only bytes as exact.  It leaves the 136-byte table partly
uncredited only because csplit sees the `"ctf"` relocation as an undefined
image symbol while MSVC owns the same select-any string COMDAT locally.
`semantic_data_matches.json` now records the independently resolved exact
table, explicitly allowed for this incomplete/fuzzy-parked unit.  Targeted
semantic application closes the data report from 552/688 to **688/688**.

Ordinary objdiff similarly reports `_ctf_engine_display_score` at 92.391304%
because of switch-table/local-label ownership.  Hardened COFF comparison
proves all 928 padded bytes and all 78 semantic relocations exact, so the
function is recorded in `semantic_matches.json`.  A one-unit semantic audit
reports 38 strict exact functions, one hidden exact, zero ordinary-only
functions, and zero unit errors.

## Honest remaining residual

`_ctf_engine_weapon_update` is source-complete but fuzzy-parked:

| measurement | January | candidate |
| --- | ---: | ---: |
| padded size | 576 | 576 |
| meaningful size | 566 | 566 |
| relocations | 40 | 40 |
| normalized SHA-256 | `0c8e2649895a583167f4b4c8d53d9d29c7e4c0edbad2488e717d4517ce66d20b` | `ca4f3e8d94db6ebd18708ad5263816b8e2542424d9fcbbe35cbd33701e1f2a56` |
| objdiff | 100% target | 77.044945% |

Aligned Capstone diff shows a coherent whole-function codegen difference,
not missing behavior: January keeps the weapon index in EDI while VC7 chooses
EBX, uses `shr`/`test` for one named flag macro where the candidate uses
`test ah`, computes the opposite team earlier, and chooses the opposite
fallthrough for the carrier goal-marker blocks.  The fail-closed blocker
classifier returns `UNKNOWN`; the park is therefore `unclassified` rather
than an invented register or scheduling recipe.

The preserved Fable search already measured block-local team variants W3/W4,
connected-flag local W5, goal-polarity W6, manual-bitshift Vw2, and their
combinations.  None closed the residual.  The manual bit replacement was also
inferior to the house-rule-compliant `TEST_FLAG` spelling.  Reopen only with
authoritative January source/local records or a natural compatible-compiler
donor that explains the entire schedule.

## House-rule review

- No `code_<address>` or `bss_<address>` placeholder remains in the CTF
  source or its symbol range.
- Typed `player_get`, `unit_get`, `weapon_get`/`weapon_try_and_get`,
  `TAG_BLOCK_GET_ELEMENT`, cseries flag macros, `real` scalars, and named enum
  constants are used.
- Unicode tag lookup uses `UNICODE_STRING_LIST_TAG`, not a raw fourcc.
- No-argument declarations put `void` on its own line; parameters are one per
  line; every function has an explicit terminal return.
- There is no forced inline/noinline, manual optimizer dependency, volatile or
  register forcing, pragma, inline assembly, raw-address access,
  representation pun, undefined-behavior carrier, or nonsensical branch.
- The January inline schedule remains intact.  The raw Fable and reconciled
  objects both contain the same four ordinary discardable math-helper
  COMDATs; neither emits `point_from_line3d`.
- `fake_match_scan.py` reports zero review leads on the CTF source and the two
  owner-runtime headers.

## Verification

- Final isolated gate: **38 exact / 1 residual / 0 unwritten** of 39.
- `_ctf_engine` and `_ctf_globals`: hardened comparison `all_equal: true`.
- Frozen address-normalized regression sweep: **17/17 still exact**, zero
  loss.
- Broad-header A/B audit found the C2 regression described above; final broad
  headers hash exactly to the base.
- `/W3 /WX /wd4146 /wd4244` isolated compile: pass with the same 38/1/0 gate.
- Fake-match scan: 3 files, zero findings.
- Targeted comparator, park, classifier, regression, fake-scan, and admission
  tool tests: **121 passed**.
- Full repository Python suite: **258 passed**.
- Targeted park validation: one active CTF park, zero stale, zero invalid.
- Targeted object-admission audit: zero candidates, contradictions, rejections,
  or revoked credits; the object correctly remains incomplete.
- JSON parse and `git diff --check`: pass.
- No `point_from_line3d` symbol is emitted.

The object remains `NonMatching` because fuzzy parks do not grant completion
credit.  It is nevertheless source-complete, has no unwritten function, and
owns all target runtime data.  Integrate the snapshot commit first and the
reconciliation commit second; do not mark the object complete until the one
parked function becomes strict exact or campaign policy explicitly changes.
