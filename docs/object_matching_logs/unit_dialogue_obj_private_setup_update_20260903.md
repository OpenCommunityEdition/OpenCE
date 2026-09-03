# `unit_dialogue.obj` private setup and update packet (2026-09-03)

## Result

The focused gate advances `source/units/unit_dialogue` from **3 exact / 0
residual / 11 unwritten** to **5 exact / 2 residual / 7 unwritten**. Two newly
emitted private functions are strict matches:

| function | padded bytes | relocations | normalized SHA-256 |
|---|---:|---:|---|
| `_unit_find_dialogue_variant` | 224 | 9 | `7d5876679b6d39eadde3903bf919d5beda45f1c3ce4a88704705bfadb12732fd` |
| `_unit_dialogue_setup` | 96 | 5 | `70e30ade567d6adcc4b530db9bc3c8b5e9c6afb4c9f2cdc455725e672e59105e` |

That is **+320 strict padded code bytes** and two functions. The three prior
exact speech-priority/is-speaking functions remain exact.

## Evidence and names

January target disassembly, assertion strings, PDB symbol records, and the
independent HaloCEA applied-type reconstruction agree on the authentic private
names `unit_find_dialogue_variant` and `unit_dialogue_setup`, plus the global
`sequential_counter`. `config/symbols.json` replaces the corresponding
`code_...` and `bss_...` placeholders. The dialogue tag is obtained through the
typed `dialogue_definition_get` macro, unit/object access remains typed, and
the public determine-variant declaration is owned by the existing closest
associated header, `units.h`, rather than a foreign C file or a new one-use
header.

## Honest fuzzy work

`unit_dialogue_determine_variant` is complete at 176/176 padded bytes and 7/7
relocations (88.95522% objdiff). Its residual is the final independent
load/increment/store schedule. `unit_dialogue_update` is complete at 768/768
padded bytes and 15/15 relocations (98.4058% objdiff); remaining differences are
branch/instruction scheduling. The retained update includes January's unusual
second decrement of `damage_minor_timer`, independently corroborated by
HaloCEA, rather than replacing original behavior with a plausible-looking
repair. Both functions are recorded in `config/parked.json`.

## Validation

- Focused `gate.py --all`: 5 exact / 2 residual / 7 unwritten.
- Protected `source/units/units`: **189 exact / 0 residual / 0 unwritten**.
- The required declarations in `ai_communication.h` were checked across all
  eight dependent translation units: every exact/non-exact function identity
  is preserved. They alter only the normalized stream of the already-parked
  `ai_communication_update_speech_timers`, which remains 672/672 bytes with
  43/43 relocations and is re-attested at 97.67839% objdiff.
- Exact helpers retain matching size, relocation identity, and normalized hash.
- No inline assembly, barriers, volatile/register forcing, raw offsets, fake
  dependencies, forced inlining, or nonsensical match-only logic was admitted.

The gate used only PID-scoped scratch objects. No canonical checkout or remote
was modified by this lane.
