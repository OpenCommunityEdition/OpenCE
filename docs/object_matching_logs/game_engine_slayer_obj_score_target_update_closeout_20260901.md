# `game_engine_slayer.obj` target/update closeout (2026-09-01)

## Result

The natural-C Slayer packet advances `source/game/game_engine_slayer` from
18 exact / 0 residual / 9 unwritten functions to 25 exact / 1 credible fuzzy
park / 1 unwritten function. The seven newly strict functions contribute
1,376 padded code bytes.

| Function | Padded bytes | Result |
| --- | ---: | --- |
| `_target_is_eligible` | 80 | strict exact |
| `_update_speed_for_score` | 272 | strict exact |
| `_slayer_engine_adjust_score` | 64 | strict exact |
| `_find_next_target` | 400 | strict exact |
| `_slayer_engine_player_killed_player` | 144 | strict exact |
| `_slayer_test_flag` | 16 | strict exact |
| `_slayer_player_update` | 400 | strict exact |

`_slayer_get_score_header_string` is retained as a natural 80-byte candidate
against January's 96-byte function. It preserves all six relocation identities
and both semantic branches. January shrink-wraps ESI separately in each branch;
the available VC7 compile hoists one save and merges the common return. The
ordinary early-return and branch-local lifetime controls were exhausted without
manufacturing an optimizer carrier, so the function is recorded in
`config/parked.json` as a `tu-context-optimization` fixed point.

The remaining 640-byte `_code_000a4300` display routine is intentionally left
unwritten pending an authenticated semantic name and coherent reconstruction.

## Source and naming evidence

The function and field names in this packet are authenticated by the later
Halo CE Anniversary source/PDB topology. The `player_datum` fields formerly
named `unknown88` and `unknown_d1` are now `multiplayer_special` and
`quit_out_of_game`; the same semantic rename was applied to their existing
users without changing those translation units' exact-function counts.

Public Slayer declarations live in the dedicated owner header
`source/game/game_engine_slayer.h`. Adding those declarations to the broad
`game_engine.h` changed VC7 definition position and reopened `units.obj`; the
dedicated header preserves the January schedule and keeps `units.obj` at
189/189. Private helpers remain `static` in the Slayer translation unit.

## Verification

- `tools/campaign/gate.py source/game/game_engine_slayer --all`: 25 exact,
  1 residual, 1 unwritten.
- `tools/campaign/gate.py source/units/units --all`: 189 exact, 0 residual,
  0 unwritten.
- Neighbor gates remain unchanged: Players 52 exact / 17 residual / 1
  unwritten; Game Engine 166 exact / 8 residual / 6 unwritten.
- Full Ninja build, semantic report, and progress report pass.
- Strict campaign board moves from 710,595 to 711,971 padded bytes and from
  4,999 to 5,006 exact functions, with no unrelated exact-function loss.
- No inline assembly, volatile optimizer control, fake dependency, address-name
  placeholder, or `point_from_line3d` COMDAT is introduced.
