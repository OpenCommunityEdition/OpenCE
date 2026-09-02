# Main/Game Engine header reconciliation (2026-09-01)

## Combined-lane regression

The individually verified Main Loop and Slayer packets interacted through
`source/game/player_control.h`. Adding the public `player_control_update`
prototype to that broad header kept `_main_loop` exact in isolation, and the
Slayer packet's semantic player-field names kept Game Engine exact in
isolation. Combined, VC7 reversed two independent epilogue reloads in
`_populate_statistic_buffer`, reopening that 560-byte function.

The strict verdict sweep exposed the interaction as one regression despite
all twelve intended functions matching. No source-level behavior changed.

## Resolution

`player_control_update` now lives in the narrow owner header
`source/game/player_control_runtime.h`. `main.c` and `player_control.c` include
that owner header; broad consumers such as `game_engine.c` retain their prior
`player_control.h` declaration environment. This follows the campaign's
established narrow-owner-header remedy for C2 definition-position sensitivity
without a local use-site prototype or a fake source dependency.

Focused gates after the split:

- `_main_loop`: strict exact, 1,648 padded bytes.
- `_populate_statistic_buffer`: restored strict exact, 560 padded bytes.
- `player_control.obj`: 41 exact / 0 residual / 8 unwritten, unchanged.
- `units.obj`: 189 exact / 0 residual / 0 unwritten.

The combined full-board verdict must therefore retain all twelve intended
gains with zero regressions before admission or push.
