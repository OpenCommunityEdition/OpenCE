# TransportEndpointWinsock error-string final independent review (2026-09-05)

## Scope and verdict

This is a read-only review of the canonical production packet after the
semantic global rename and full regeneration.  I did not edit source,
configuration or existing ledgers, and did not run a gate, configure or
Ninja build.

**PASS for retaining `winsock_error_to_string` as credible fuzzy source and
for retaining the semantic `transport_endpoint_globals` name.**  The packet
must not receive exact-code credit, data credit or whole-object admission.
The existing unclassified park at **49.78903%** is the correct accounting
disposition.

## Frozen inputs

- production source SHA-256:
  `c821c9932a6bbb53465779309b295496dd95e2aa2580650c282b9be048e05e70`;
- regenerated January object SHA-256:
  `8a19a22ca0afd5683af2ed79d6cb429722b7a92079ea8f82c6328261b3dfaae1`;
- frozen receive baseline object SHA-256:
  `bac37bef57141777b4e8754866a57af18958f26626f466ea3b9e3995ced72a17`;
- current production object SHA-256:
  `d631a798a80d185b32f03a781d9a394bedf0dfd866a52401d75cf0e8d13813a3`;
- complete owner audit:
  `scratch/transport_error_string_canonical_owner_audit_20260905.json`,
  SHA-256
  `1d90f2fa57f08e29edad2510daa68f350cc805c2b7b57bc6ba4e981fb4b79ee7`;
- complete runtime-record audit:
  `scratch/transport_error_string_canonical_runtime_audit_20260905.json`,
  SHA-256
  `d352dd4525d56d6815b2a8ae4fd9b6f9068b0312435eee69f0851e05ca598c40`.

## Complete runtime-owner transition

The runtime audit reports exactly **87 added owners**: the one newly written
function plus its 86 string literals.  It reports zero lost owners, no COMMON
before or after, and no unexplained inherited change.  The added-owner set is
identical to the union of `_winsock_error_to_string` and the 86 new owners in
the independent non-code census.

Nine inherited function records differ.  Four are compiler-local label
renumbering only:

- `_read_endpoint`;
- `_read_from_endpoint`;
- `_write_endpoint`;
- `_write_to_endpoint`.

The remaining five functions contain eight pre-existing calls to the public
`_winsock_error_to_string` symbol:

| caller | relocation offsets |
| --- | --- |
| `_accept_endpoint` | 176 |
| `_create_endpoint_socket` | 71, 115, 203, 286 |
| `_disconnect_endpoint` | 108 |
| `_get_endpoint_address` | 329 |
| `_listen_endpoint` | 128 |

For every one of these eight records, address, relocation type20 (REL32),
addend0, resolved destination, symbolic name, storage class2, symbol type32
and symbol value0 are unchanged.  The sole difference is
`target_symbol.section_id`: it moves from `null` to
`.text|owners=_winsock_error_to_string` because the same symbol changes from
undefined to genuinely defined in this translation unit.  This is a bounded
ownership transition, not caller code drift.

The lower-level census independently shows all **22 inherited code owners**
and all **20 inherited non-code owners** preserve their sections and owner
properties.  No inherited owner is lost or changed.

## New function and physical data

January and production both measure the new function at 1,568 bytes and 177
relocations with relocation-normalized SHA-256
`35e5bf685b7053c6bf425c6f771467a3359cebda61937162f15f708d9b21ae60`.
It is nevertheless not strict exact: ten physical relocation destinations
differ (five early string bodies and five compensating switch-table entries).
The ordinary production report therefore correctly measures 49.78903%, and
the park does not disguise the residual as exact.

All **86 newly emitted literal owners**, totaling **1,363 raw bytes**, have a
corresponding January owner with equal content, relocation information,
offset/storage/type, section flags, alignment and COMDAT selection.  This is
real physical source progress, but it does **not** create report credit:

- the unit still reports only its existing 524-byte BSS as matched data;
- the target's 2,040-byte `.rdata` remains a non-exact merged section
  (ordinary similarity 96.298256%);
- `config/semantic_data_matches.json` has no entry for this unit;
- applying the unchanged semantic-data manifest to the regenerated report
  yields the same campaign total, **2,030,868 matched data bytes**, as the
  frozen receive baseline.

Thus the accounting delta is **+0 matched data bytes**, not +1,363.  The six
inherited candidate-only data owners (three assertions and three SDK tables)
also remain present, and five January data owners are still absent, so no
whole-object data claim follows from the new literals.

## Stable-status check and disposition

The receive-baseline and current 8,245-owner snapshots have identical key
sets.  Their sole value change is this function's status from unwritten to
residual; its size and normalized hash are identical in both snapshots.
There is therefore no strict-owner gain or regression: the stable count stays
at 6,217.

The packet correctly contributes 1,568 meaningful bytes of reconstructed
fuzzy source while adding **zero exact code bytes**, **zero credited
functions**, and **zero matched-data bytes**.  Seven January code owners and
five January data owners remain missing, and the inherited extra data owners
remain an admission caveat.  Keep the object NonMatching and retain the
existing park.  No `_point_from_line3d` or other new helper/COMMON owner is
present.
