# Encounters platoon-rule pair: independent new-owner review

## Scope and result

PASS for the new literal, real-constant, private-function, and code-owned
switch-table ownership of the first natural production candidate. This is a
bounded read-only COFF/image audit, not a full link, a build, a gate, a runtime
test, or a claim that the residual test-rule function is byte-exact. No source
variant or production edit was made by this audit. Active Fable trees were not
inspected; existing canonical objects were read only for the 16 selected data
symbols.

The source-level review is
`scratch/encounters_platoon_rule_pair_source_review_20260905.md`, SHA256
`c00dfd2ba93ac2a0332d5148f16a6a6789c63fc61127af71e963fb7357402f12`.
It verifies ordinary signed C, full datum handles, authentic enum cases and
debug strings, genuine existing caller ownership, and no tuning constructs.

## Frozen inputs and reproducible report

All paths are relative to the canonical repository
`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/rasterizer-model-wrapper-alias-20260829`.

- Schema-after baseline:
  `scratch/encounters-platoon-schema-after-20260905/source/ai/encounters.obj`,
  SHA256 `abe55d24ee301a9cb2e94eb9093005f487e55d732f3f5ca1117b8be052ba03d3`.
- Actual canonical first candidate:
  `scratch/encounters-platoon-pair-canonical-first-20260905.obj`, SHA256
  `44b52909171e2ed014c4bfc68f9dd1283a8db4b3f1cf17f92c5009e4f774a8f8`.
- January target: `build/split/source/ai/encounters.obj`, SHA256
  `7edc1863eb2c08f7f2ad612e3cfa7b246c0eb9a3b5907fd08c2c17ecd9a056a3`.
- Original image: `cachebeta.exe`, SHA256
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- Original symbol map: `config/symbols.json`, SHA256
  `53c34ecbaad3167430dbccd0b225c3066be4dc3e0d9b975cae4d346e19444394`.
- Script: `scratch/encounters_platoon_pair_new_owner_audit_20260905.py`, SHA256
  `52a6e9bfc40bbb4a52444f38468401744ed183bf330d5265dec3ac289125cb39`.
- Final report:
  `scratch/encounters-platoon-pair-canonical-new-owner-audit-20260905.json`,
  SHA256 `e0d1e0d0581f65044b64b7b54143f57883fbcf496a60675ce501eb5e2dbaefc5`.

Reproduce from the repository root:

```text
python -B scratch/encounters_platoon_pair_new_owner_audit_20260905.py --candidate scratch/encounters-platoon-pair-canonical-first-20260905.obj --output scratch/encounters-platoon-pair-canonical-new-owner-audit-20260905.json
```

The report records exact selected-owner payloads, flags, symbol metadata,
section auxiliaries, relocations, and object hashes. Its canonical census is
a snapshot of the 572 existing `build/base/**/*.obj` files at audit time.

## New private function owners

Exactly two new code owners exist relative to the schema-after baseline:
`_encounter_update_platoons` and `_encounter_test_rule`. Both have storage class
3 (static), function type 32, value zero, `.text` flags `0x60501020`, and COMDAT
selection 1 (NODUPLICATES), matching their genuine January private ownership.
There is no removed previously defined named runtime owner. The sections are
320 and 656 bytes, respectively; this padded extent is not meaningful-code
credit. Compiler checksums and non-associative section-number auxiliary fields
are recorded separately from runtime identity.

## All 16 new data owners are authentic and merge-compatible

Thirteen exact diagnostic strings and three compiler real constants are added.
Their entire raw payloads match the original image at the symbol-map file
offsets, not merely a text substring or an assumed later-build spelling.

Every candidate data definition is external storage class 2, type 0, value 0,
`.rdata` flags `0x40301040`, with no relocations and SELECT_ANY selection 2.
Scanning all 833 January split objects and requiring actual defined COFF
symbols finds:

- All 13 strings physically owned by January `source/ai/encounters.obj`.
- `__real@3e800000` (0.25) and `__real@3f000000` (0.5) physically owned by
  January `source/ai/action_alert.obj`.
- `__real@3f400000` (0.75) physically owned by January `source/ai/ai_debug.obj`.

All physical target owners have identical payloads and flags and the same
external/value-zero/no-relocations/SELECT_ANY metadata. Their exact object
paths and hashes are in the report. January Encounters refers to the three
pooled constants without physically defining them; natural candidate emission
of compatible SELECT_ANY definitions is not evidence of an invented global.

The separate complete selected-symbol census of all 572 existing canonical
built objects finds 93 definitions: 13 strings (one each), 16 definitions of
0.25, 60 of 0.5, and 4 of 0.75. Counts include the current Encounters object.
All 93 have identical payloads/flags and compatible SELECT_ANY metadata.
There are zero conflicting NODUPLICATES definitions or other incompatible
selected owners. This proves the bounded merge-compatibility condition; it
does not claim a whole-program link was run.

## Internal switch tables remain under their real code owner

The adjacent table data was partitioned at actual code dispatch relocation
targets, not by treating a contiguous relocation run as one table.

| Test-rule owner | First switch | Diagnostic switch | Internal / external relocations |
| --- | --- | --- | --- |
| January | `0x244`, 10 entries, selectors 0 through 9 | `0x26C`, 9 entries, selectors 1 through 9 | 21 / 25 |
| Candidate | `0x244`, 9 entries, selectors 1 through 9 | `0x268`, 9 entries, selectors 1 through 9 | 20 / 25 |

All table entries are internal DIR32 relocations into real instruction
boundaries in the same test-rule section. The two code-to-table dispatch
references plus the table entries account for every internal relocation.
There is no externalized table owner or unaccounted internal edge.

January's first table explicitly includes selector zero at its false/default
block `0x106`; the candidate decrements the selector, bounds-checks 0 through
8, and routes original selector zero to its false/default block `0x107`.
The other nine first-table destinations and all nine diagnostic destinations
are exactly one byte later in the candidate. Thus the one-relocation count
difference is an omitted redundant never/default table slot, not a missing
rule implementation. The script independently records both dispatch contexts
and every destination. This is a natural residual, not permission to tune
the switch or reorder declarations.

## Acceptance boundary

This ownership review found no blocker. Root owns all-section/global strict
preservation, full build/test results, meaningful-code accounting, residual
classification, and publication. The separate runtime review is
`docs/object_matching_logs/encounters_obj_platoon_rule_pair_runtime_review_20260905.md`.
Nothing here upgrades `_encounter_test_rule` to exact, weakens the house rules,
or authorizes source-shape variants.
