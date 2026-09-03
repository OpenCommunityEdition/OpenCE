# `game_engine_race.obj` display-score exact closeout (2026-09-03)

## Result

`race_engine_display_score` is reconstructed as ordinary typed C and is a
strict January match:

- padded size: `1152 / 1152` bytes;
- relocation count: `81 / 81`;
- normalized SHA-256:
  `079f7fd734d64b17d2c680bf3ba5470290326d7d3ceae77d1b9ed9a24ea454a7`;
- differing normalized bytes: `0`.

The object gate moved from `34 exact / 1 residual / 1 unwritten` to
`35 exact / 1 residual / 0 unwritten`. Exact function bytes increased from
`4,080` to `5,232` (`+1,152`). The pre-existing 224-byte
`race_engine_get_score` residual is unchanged.

## Evidence and source shape

The implementation was recovered from the January COFF function body and
relocations, January message IDs and structure offsets, the existing exact
game-engine siblings, and the local HCEA reconstruction at:

`research-cache/halocea-cseries-20260820/src/blam/game/race_engine_display_score.c`

HCEA's authenticated multiplayer string indices supply the semantic names for
indices `0xA7` through `0xB1`. January differs from the Xbox donor by one in
the race-extension message IDs, so the current January enum values remain the
authority.

The target's two dispatch tables are reproduced naturally by resolving the
message player only for message forms that consume one, then dispatching the
format operation. This preserves the fixed-string path's ability to run
without a message player. Case ordering follows the target's semantic block
layout and lets MSVC perform its ordinary tail merging. No byte-forcing source
is present.

The function uses:

- `player_get` and `struct player_datum` rather than raw datum casts;
- `real` and `TICKS_PER_SECOND` for lap-time conversion;
- named race-message and multiplayer-text enum constants;
- the existing owner-header prototype in `game_engine_race.h`;
- explicit terminal `return result;` and the repository's vertical parameter
  style.

There is no inline assembly, `volatile` or `register` steering, pragma,
forced-inline annotation, raw structure offset, artificial dependency, or
undefined behavior.

## Validation

```text
python -m tools.campaign.gate source/game/game_engine_race --all
== exact 35  residual 1  unwritten 0  (of 36 listed)

python -m tools.campaign.gate source/units/units --all
== exact 189  residual 0  unwritten 0  (of 189 listed)

python tools/fake_match_scan.py --format json --fail-on-findings source/game/game_engine_race.c
findings: []

ninja build/base/source/game/game_engine_race.obj build/base/source/units/units.obj
passed

python -m pytest -q
261 passed
```
