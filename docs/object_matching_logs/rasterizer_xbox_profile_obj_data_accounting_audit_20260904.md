# Xbox Profile initialized-data accounting audit (2026-09-04)

## Result and scope

The existing Opus reconciliation already reconstructed this data correctly.
This packet changes no implementation, schema, compiler flags, target bytes,
or comparator. It adds **132 credited data bytes**, not code bytes, through
the existing dynamically checked semantic-data manifest. The object remains
`NonMatching`; no completion is claimed.

The prior source/evidence record is
`rasterizer_xbox_profile_obj_opus_reconciliation_20260903.md`.
Root and an independent reviewer repeated the complete section and owner
comparison against the current production object, rather than relying on
that historical report. The active Fable lane was not inspected or changed.

## Complete initialized-section proof

The raw report has 2,234 target data bytes and 2,102 matched data bytes.
The 1,122-byte BSS and 980-byte read-only data are already fully credited.
The sole remaining gap is the complete 132-byte initialized section.

Both target and production have exactly one such `.data` section, with flags
`0xC0400040`, identical raw payload, and 29 DIR32 relocations. Its normalized
SHA-256 is
`b577924a924477a95be3c0a539812cce4cae4eaf93489d580a909a02169d0cf9`.

| Owner | Offset | Type | Storage |
| --- | ---: | ---: | ---: |
| `_rasterizer_profile_performance_counter_frequency` | 0 | 0 | 3 (static) |
| `_rasterizer_profile_globals` | 8 | 0 | 3 (static) |

Both layouts agree exactly. Source uses a real eight-byte `LARGE_INTEGER`
initialized to one and a separate naturally aligned 124-byte state object.
There is no packed aggregate, opaque replacement, or representation cast.
Symbol-table order differs but does not change either owner's identity.

The hardened direct comparator and an independent image-address-resolved
comparison both pass. The only raw relocation-presentation differences are
the `clear` and `water` string references at section offsets `0x10` and
`0x5C`: the target uses undefined image symbols, while the candidate contains
identical local select-any strings. Resolved destinations, relocation types,
addresses, addends, and normalized bytes agree.

The manifest uses the true offset-zero frequency owner as its anchor and
`allow_incomplete_unit: true`. The existing verifier must re-prove all bytes
and resolved relocations and must find that 132 bytes cover the entire
reported data gap. The stored size, relocation count, and hash are checked
on every report generation. No matching rule is relaxed.

## Whole-object veto

All 16 target functions are exact, but the production object also contains
three stock SDK select-any table owners absent from the January split:

- `_D3DTEXTUREDIRECTENCODE`: 16 bytes;
- `_D3DSIMPLERENDERSTATEENCODE`: 328 bytes;
- `_D3DPRIMITIVETOVERTEXCOUNT`: 88 bytes.

These 432 candidate-only bytes receive no credit. Ordinary final-link
ownership/coalescing has not been established. The inherited caller-local
debug-options view is a separate shared-layout boundary. An explicit
`object_admission_rejections.json` entry records these facts so that fixing
the data accounting cannot silently turn a zero-gap report into whole-object
approval. Authentic shared ownership and complete ordinary link evidence
are required to reopen this veto; no fake header, forced helper, metadata
selection change, or invented discard pass is acceptable.

## Verification

Full Ninja/report generation passes the dynamic 132-byte proof. Credited data
advances from 2,028,615 to **2,028,747** bytes without any source change here.
Matching objects remain **391/833**. Admission reports zero candidates,
contradictions and revocations, with five fully-reported explicit rejections.
The combined Profile code packet adds two strict functions independently;
the complete stable sweep has **zero regressions**. All 267 parks validate
and all 288 tests plus 26 subtests pass. The audit output is
`scratch/profile-frame-tick-admission-20260904.json`; the combined verification
record is `profile_obj_frame_tick_start_reconciliation_20260904.md`.
