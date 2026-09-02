# `game_engine_oddball.obj` canonical admission (2026-09-02)

## Verdict

The reconciled Oddball translation unit is complete and is admitted as
`Matching`. Its natural source produces all 38 target functions exactly under
the hardened COFF comparator and owns all 736 target data bytes. No source
change, matching-only symbol trick, or artificial string declaration is
needed.

## Reviewed objdiff false negatives

Ordinary objdiff reports 37/38 functions and leaves the 136-byte
`_oddball_engine` table uncredited. Both misses have the same benign cause:
the January split object references the pooled `"oddball"` literal as an
undefined symbol, while VC7 emits the identical select-any string COMDAT in
the rebuilt Oddball object. The January selected definition is owned by
`game_engine.obj`.

The hardened comparator proves:

- `_oddball_engine_display_score`: 736 padded bytes, 56 relocations, normalized
  SHA-256
  `6ad4051ea66ca25449fb855b7cb0076efef3e056e80fa1a6566d7dc5fc2a2e38`.
- `_oddball_engine`: 136 bytes, 27 `DIR32` relocations, normalized SHA-256
  `ffe1ef0aa827017276299f3a497655019d91f94374129c31634061500e5ae4b9`.
- The table payload, relocation offsets, relocation types, symbolic targets,
  and addends are identical. Only which object presents the select-any string
  definition differs.

The fail-closed records in `config/semantic_matches.json` and
`config/semantic_data_matches.json` encode these exact measurements. This is
the same reviewed ownership pattern already used for the Slayer and CTF
engine tables.

## House-rule boundary

The source retains the ordinary initializer `.name = "oddball"`. Forcing an
external decorated literal owner would be fake matching and is explicitly
rejected. No inline schedule, function body, type, prototype, or data layout
was changed for admission, and no `point_from_line3d` COMDAT is emitted.

## Verification requirement

Admission is conditioned on a regenerated split, full `ninja all_source
progress semantic_progress`, the complete tooling test suite, the admission
audit, and an address-stable exact-set regression sweep reporting zero lost
functions.
