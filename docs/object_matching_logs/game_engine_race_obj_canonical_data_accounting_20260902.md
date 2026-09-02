# `game_engine_race.obj` canonical data accounting (2026-09-02)

## Verdict

Race remains correctly `NonMatching`: 34 of 36 functions are exact under the
hardened gate, `_race_engine_get_score` is honestly parked, and
`_race_engine_display_score` is unwritten. Its 136-byte `_race_engine`
callback table is nevertheless exact and should receive data credit.

## Reviewed ownership difference

The January split object references the pooled `"race"` literal as an
undefined symbol, while VC7 emits the identical select-any string COMDAT in
the rebuilt Race object. This is a COFF ownership presentation difference,
not a table mismatch. The hardened comparator proves identical normalized
bytes, all 25 relocation offsets and types, symbolic destinations, and
addends. The table's normalized SHA-256 is
`97a4c632a978acafd20933da4504d8ec3fdf3c5afd2f78934bced57dc8f746f4`.

`config/semantic_data_matches.json` records the evidence with
`allow_incomplete_unit: true`, matching the established CTF policy. This
recovers exact data only and cannot mark Race complete or grant code credit.
No source was changed, and forcing an external decorated string owner would
violate the anti-fake-match rule.
