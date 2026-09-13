# `shell_xbox.obj`: scope-table owner refresh (2026-09-08)

The full rebuilt batch preserves the authenticated `_main` SEH scope table
byte-for-byte, but the C2 compiler-local owner serial advances from `$T18273`
to `$T18284` after the accepted shared declaration updates through the seventh
reconciliation batch.

`config/semantic_data_matches.json` now names `$T18284`. The verifier still
requires the same unique 12-byte `.rdata` section, `0x40401040` flags, two
relocations, normalized SHA-256
`3414421b183188342209566fad4e0c0723070b4030630dde685ba70281e92725`,
and source-relative `_main` handler destinations. This is a fail-closed owner
refresh, not new data credit or a relaxed comparison.
