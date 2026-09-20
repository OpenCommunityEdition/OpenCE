# Wave3 independent checkpoint audit

**PASS: 353 checks, no discrepancies.** Wave3 gains zero exact functions,
meaningful bytes, or padded bytes. The accepted ledger is identical to frozen
Wave2 commit `1ec9171e757e8aab5038f26ec9797744da947c52`: six cumulative
admissions, 8,314 meaningful / 8,352 padded gained bytes. Halo remains
1,439,906 / 1,770,166 (81.34299%), with **153,244 meaningful bytes remaining**
to the 90% threshold. No moving-function change is counted.

Reproduction and detailed results are `scratch/astra-wave3/accounting-review.py`
and `.json`. This audit reads existing Git records, sources, snapshots and
production objects. It runs no compiler, Ninja, tests, or source/config edits.

## Frozen records and accounting

Schema4 preserves the Stage0 summary and all original measurements, Wave1
summary and per-row state, and Wave2 `wave2_summary`/`wave2_after` and per-row
state exactly as committed. All 293 original atlas rows remain, including
accepted rows. All six manifest admission records, including the complete
resolved one-wide proof, are unchanged. Current `latest_summary`, manifest
`after`, and Wave3 summary agree. Current row source hashes agree with the
production source. The saved semantic report equals `build/semantic_report.json`.

Direct accepted-ledger iteration and unique offset-zero target-section
deduplication give:

| Scope | Canonical accepted entries | Meaningful bytes | Physical sections | Padded bytes |
|---|---:|---:|---:|---:|
| Halo | 7287 | 1439906 | 7278 | 1497994 |
| Overall | 7554 | 1461138 | 7545 | 1519979 |

Nine accepted embedded records do not add their parent padding again. Current
unaccepted inventory remains 287 rows / 330,260 meaningful bytes. Earlier
whole-object statuses and park retirements are unchanged and add no duplicate
function credit. Stable Wave3 gain/loss is 0/0; cumulative stable gain remains
five functions / 7,200 padded bytes with zero losses. One-wide's separately
proved alias accounts for the existing stable/accepted-ledger distinction.

## Source/header and actual production-object boundary

The audit resolves literal include paths relative to each source/header and
the source root across C/C++ and header suffixes. It finds exactly five C
consumers and no intervening header consumers. The qualified includes in main,
HS globals and bipeds, and the sibling `collision_debug.h` includes in the
two collision files, resolve to the same real owner header. The first draft
of this review's inventory missed the two sibling spellings; that review-script
error was corrected before release and never indicated a source omission.

Each saved pre-change consumer source equals its frozen Git source. Header-only
objects and actual production objects were independently compared with those
saved pre-change objects, including normalized code, named ownership, runtime
data, COMMON definitions, and complete section auxiliary records.

| Consumer | Code owners | Strict controls preserved | Runtime data sections | Production code changes |
|---|---:|---:|---:|---|
| main | 96 | 90 | 101 | none |
| collision_debug | 1 | 0 | 31 | none |
| collision_usage | 14 | 13 | 53 | none |
| hs_globals_external | 0 | 0 | 445 | none |
| bipeds | 82 | 42 | 111 | physics only |

The header-only comparison preserves all **145 exact controls** and every
consumer code section. Actual production preserves those same controls and all
runtime data/COMMON owners. Only `_biped_update_physics` changes; its production
fingerprint equals the independently reviewed candidate. Moving remains
byte/relocation-identical to the frozen baseline. No consumer emits a point
helper. The whole-tree owner/data/COMMON census matches committed Wave2.

HS's typed member/scalar addresses preserve the full tables and count, which
still strictly match January. `_hs_external_global_definitions` and
`_hs_external_globals` name one shared section; they are not separate byte
credit. Header declarations create no storage. The three existing scalar
definitions and linker-common owners remain intact.

## Zero-credit and checkpoint receipts

Only biped physics is a changed fuzzy function in Wave3: its 5,366-byte target
meaningful extent receives zero credit. Candidate 5,376 padded / 124 relocations
still differs in normalized SHA. Moving's withdrawn scratch proposal is not
landed or counted. Cumulative changed fuzzy target extents are 10,962 + 34,010
+ 5,366 = 50,338, all at zero credit; these are function extents, not new
coverage or amounts of newly written source.

Checkpoint receipts agree: tests **1,152 passed, five skipped, 26 subtests
passed**; parks **185 active / zero stale / zero invalid**; admission audit
**19 candidates / zero contradicted / eight rejected / zero revoked**; fake
scan **22 global findings / zero changed-source findings**. The committed
Wave2 park and admission JSONs equal the current receipts. Full-build and
stable/semantic logs show no failures. No tests or full build were repeated
for this accounting audit.

The updated HANDOFF and main research atlas clearly label Wave3 as zero gain,
retain earlier wave sections as historical records, identify physics as the
sole changed biped body, preserve the source-revision uncertainty, and leave
the 90% goal unfinished. No discrepancy requiring production or record changes
was found.
