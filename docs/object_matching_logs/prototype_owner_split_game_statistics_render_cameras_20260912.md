# Prototype-owner split: game statistics and render cameras (2026-09-12)

## Game-statistics interface boundary

`game_statistics_record_damage` is the sole statistics API consumed outside
the public `game.h` umbrella.  It is now declared by the narrow owning header
`source/game/game_statistics_internal.h`, included by its defining
`game_statistics.c` translation unit and its only consumer, `damage.c`.

`game_statistics_start` and `game_statistics_record_kill` remain in `game.h`.
That file is the closest associated public header and preserves the declaration
topology seen by the January build.  `game_statistics_stop` has no consumer, so
it remains definition-only; its target external linkage is unchanged.

A broad `game_statistics.h` consolidation was tested and rejected.  Merely
removing the two existing declarations from `game.h` and exposing all four
functions through the broad header reproducibly reopened two previously exact
Race functions:

- `_race_touch_flag`: 336 padded bytes, relocation count 28 instead of 30.
- `_race_engine_player_update`: 160 padded bytes, normalized-byte mismatch.

Reverting only that broad declaration topology restored Race to 35 exact, one
pre-existing residual.  The retained narrow split therefore resolves the real
cross-TU damage bridge without trading away 496 exact padded bytes or using a
conditional header/code-generation trick.

Focused retained-state gates:

- `game_statistics.obj`: 3 exact, 1 pre-existing residual.
- `damage.obj`: 31 exact, 3 pre-existing residuals.
- `game_engine.obj`: 169 exact, 11 pre-existing residuals.
- `game_engine_race.obj`: 35 exact, 1 pre-existing residual.

## Render-cameras interface boundary

`render_sprite.c` now includes the existing narrow owner
`render_cameras_internal.h`.  Its consumer-local duplicates for
`render_frustum_cube_view_fraction` and `render_camera_screen_to_view` were
removed, and both declarations are owned by that internal header.

All nine consumers of `render_cameras_internal.h` were enumerated and gated.
Their exact/residual sets remain unchanged.  In particular,
`render_sprite.obj` remains 6 exact / 4 pre-existing residual and
`render_cameras.obj` remains 11 exact / 4 pre-existing residual / 6 unwritten.
Both focused gates also pass the forbidden `_point_from_line3d` emitted-symbol
guard.

No new exact-match credit is claimed by this ownership-only cleanup.
