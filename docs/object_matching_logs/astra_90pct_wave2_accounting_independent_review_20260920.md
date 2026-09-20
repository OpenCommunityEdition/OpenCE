# Wave2 accounting independent review

**PASS: 320 checks, zero discrepancies.** The completed checkpoint contains six
cumulative accepted functions, 8,314 meaningful bytes and 8,352 padded bytes.
Wave2 contributes only `_extract_sequence`, 720 meaningful/padded bytes.
Halo is 1,439,906 / 1,770,166 meaningful bytes (81.34299%); the remaining gap
to 1,593,150 is **153,244**, so the 90% goal remains incomplete.

The script `scratch/astra-alias-wave2-accounting-review.py` and its JSON output
independently compare the schema3 atlas, summary and manifest with Git objects
at `905e5e3b41591af98c82accf3f57fc38118c2e7f`, the accepted ledger, canonical
report rows, and actual target/candidate COFF objects. No Ninja, compilation,
production edit, configuration edit, or commit was performed. Fresh read-only
park and admission checks write only separate `astra-alias-wave2-accounting-*`
scratch evidence.

Stage0 summary and all frozen measurement fields in the original 293 rows are
unchanged from that commit. Wave1 summary, every `wave1_after` row, all five
old admission records, and the complete one-wide relocation proof are also
unchanged. The manifest's `after`, atlas `latest_summary`, and wave2 summary
agree exactly. Current source hashes agree with `wave2_after`. The saved final
semantic report equals `build/semantic_report.json`.

## Recomputed physical accounting

Padded totals below directly iterate the accepted ledger, resolve its target
symbols and deduplicate `(unit, section)` only for offset-zero owners. This
does not use the narrower `function_sections()` helper, which misses five
valid libcmt sections with offset-zero aliases. Nine accepted embedded records
remain canonical entries but add no parent-section padding a second time.

| Snapshot | Halo entries | Halo meaningful | Halo physical sections / padded | Overall entries | Overall meaningful | Overall physical sections / padded |
|---|---:|---:|---:|---:|---:|---:|
| Stage0 | 7281 | 1431592 | 7272 / 1489642 | 7548 | 1452824 | 7539 / 1511627 |
| Wave1 | 7286 | 1439186 | 7277 / 1497274 | 7553 | 1460418 | 7544 / 1519259 |
| Wave2 | 7287 | 1439906 | 7278 / 1497994 | 7554 | 1461138 | 7545 / 1519979 |

All six saved target/candidate admission fingerprints equal the actual current
objects. Five compare strictly under symbolic relocation identities. One-wide
still has its documented symbolic difference; a fresh full resolved comparison
proves all 70 destinations equal and resolved. Its previous proof record is
preserved byte-for-byte as JSON content. No new alias exception is involved.

Stable snapshots independently show five cumulative gains / 7,200 padded
bytes, versus six ledger gains / 8,352 padded bytes. The difference is exactly
one-wide's 1,152 padded bytes. Wave2 stable gain is one / 720. Neither accepted
ledger nor stable snapshots lose an inherited entry. The wave2 starting stable
snapshot equals the frozen wave1 snapshot.

## Residual and zero-credit accounting

The current canonical report minus the accepted ledger yields 287 rows and
330,260 meaningful bytes: 283 section-owner rows and four embedded records.

| Class | Rows | Meaningful | Physical padded |
|---|---:|---:|---:|
| Written, unparked | 108 | 181364 | 182560 |
| Parked | 167 | 144503 | 145808 |
| Unwritten | 7 | 3733 | 3784 |
| Embedded records | 4 | 436 | 0 |
| Semantic veto | 1 | 224 | 224 |

The three changed AI residuals are absent from the accepted ledger: ai_debug
24,576, perception 4,720, and looking 4,714 meaningful bytes, totaling 34,010
at zero credit. These are target function extents, not gained coverage. Wave1's
separate historical fuzzy extents are not added to the wave2 figure.

Fresh park validation exactly matches the checkpoint: 185 active, zero stale,
zero invalid. Fresh admission audit also exactly matches: 19 candidates, zero
contradicted, eight rejected, zero revoked. These audit candidate counts are
not counts of newly admitted whole objects. The two cumulative object statuses
remain Action Alert and bitmap extraction; their whole-object code/data extents
do not create duplicate function credit. Bitmap's 4,496 data bytes use canonical
report alignment, as established by the independent extraction ownership review.

The current HANDOFF's six gains, 153,244 remaining bytes, stable/ledger
distinction, 34,010 zero-credit extents, object admissions and test receipt
(1,152 passed, five skipped, 26 subtests passed) agree with these records.
Tests and full build were reviewed from the completed checkpoint logs; this
accounting review did not rerun them. The newly documented read-only avoidance
packet supplies no source shape or credit and does not alter this checkpoint.
