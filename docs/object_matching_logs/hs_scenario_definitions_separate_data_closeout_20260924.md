# `hs_scenario_definitions.obj` separate-data closeout (2026-09-24)

Base: `e9e62b78` on `jonas/exact-pilots`. This isolated lane uses the same
installed objdiff 3.3.1 binary as canonical; the pinned 3.6.0 release is not
silently substituted. This is one newly complete Halo-owned object, **zero new
exact code bytes**, and one verified 628-byte data section. Do not add the 628
to the raw `matched_data` metric: 3.3.1 still undercounts it and the pinned
semantic-data entry supplies whole-object admission only.

The inherited source grouped unrelated tag definitions and byte-swap tables
into five invented structures under `#pragma pack(4)` and `align(4)`, with a
fabricated `field_table_alignment` member. January's `.data` is instead
8-byte aligned. The replacement declares the typed data objects separately
and keeps their natural dependency/order. In particular, the 84-byte
`hs_global_fields` array naturally begins at +144 after the 44-byte
`hs_scripts_block` at +96, accounting for the four alignment bytes without a
source padding member. The earlier 72-byte script-field array starts at +24.
The remaining table/code/definition objects likewise occupy the pre-existing
offsets. Names of file-static arrays are descriptive, not asserted original.

COFF proof, January split versus rebuilt:

| Property | Both |
| --- | --- |
| `.data` size and flags | 628, `0xc0400040` |
| Normalized payload SHA-256 | `a1add12fc2ddd7afb3935997f9e1532b27a3887f8e85e914db954d1665547c09` |
| Relocations | 33, same addresses/types/resolved targets |
| Public definition offsets | 0, 12, 96, 228, 320, 364, 416, 596, 612 |
| `_hs_syntax_data_byte_swap` | file-static storage class 3 |

All nine public data owners retain their target storage and offsets. The sole
target function remains strict exact. The compiled object defines no surplus
external symbol, so the selected-provider check has nothing to link. Its
`source-layout-incomplete` admission rejection is retired only after the
source, section alignment, target offsets, and private function storage were
all corrected; the semantic-data entry is not a waiver for a known mismatch.

Full Ninja build, the 8,245-function stable sweep (7,596 exact, zero gained
or regressed), parks (96 active / 0 stale / 0 invalid), and admission audit
(14 candidates / 0 contradicted / 7 independently rejected / 0 revoked)
pass. `pytest tools` gives 1,157 passed / 5 skipped / 26 subtests; the
fake-match scan has zero leads; `git diff --check` passes. The object is
Matching, with no compiler-flag change, inline asm, representation cast, or
new helper COMDAT.
