# Rasterizer lock-operation symbols/owner reconciliation (2026-09-13)

## Result

`enum rasterizer_lock_operation` is a public rasterizer type and is now owned
in `source/rasterizer/rasterizer.h` with its complete original 0..17 value
set.  The prior four-value reconstruction and translation-unit-local numeric
stand-ins have been retired.

This is not a declaration-count matching experiment.  The complete type is
independently identified by the HCEA symbols-derived type database at:

`C:\Users\isabe\Documents\Codex\reference\halocea\src\headers\rasterizer_lock_operation.h`

That reference labels the source as the anonymous enum recorded in
`types_enum_values` and supplies all identifiers and values:

| Value | Original identifier |
| ---: | --- |
| 0 | `_rasterizer_lock_none` |
| 1 | `_rasterizer_lock_texture_changed` |
| 2 | `_rasterizer_lock_vertexbuffer_new` |
| 3 | `_rasterizer_lock_detail_objects` |
| 4 | `_rasterizer_lock_decal_update` |
| 5 | `_rasterizer_lock_decal_vertices` |
| 6 | `_rasterizer_lock_bink` |
| 7 | `_rasterizer_lock_ui` |
| 8 | `_rasterizer_lock_cinematics` |
| 9 | `_rasterizer_lock_koth` |
| 10 | `_rasterizer_lock_hud` |
| 11 | `_rasterizer_lock_flag` |
| 12 | `_rasterizer_lock_lightning` |
| 13 | `_rasterizer_lock_debug` |
| 14 | `_rasterizer_lock_text` |
| 15 | `_rasterizer_lock_contrail` |
| 16 | `_rasterizer_lock_sprite` |
| 17 | `_rasterizer_lock_bsp_switch` |

`NUMBER_OF_RASTERIZER_LOCK_OPERATIONS` follows the final value and therefore
has the source value 18.

## Caller cleanup

Every reconstructed use of `rasterizer_globals.current_lock_operation` now
uses the public named constants.  This removes:

- private duplicate enums from Bink playback, lightning, contrails, sprites,
  the King dynamic quad, rasterizer debug geometry, and Xbox decals;
- the inferred aliases `_rasterizer_lock_unlocked`,
  `_rasterizer_lock_dynamic_quad`, `_rasterizer_lock_debug_geometry`, and
  `_rasterizer_lock_structure_bsp_vertex_buffers`;
- raw values 0, 1, 2, and 3 in Xbox bitmap, transparent-geometry, and
  detail-object code; and
- a materially wrong local spelling in Xbox decals: the operation used while
  locking decal vertex data is value 5, `_rasterizer_lock_decal_vertices`,
  not value 4, `_rasterizer_lock_decal_update`.

The detail-object translation unit now includes the owning rasterizer header
and uses `struct rasterizer_globals_definition`, rather than redeclaring a
three-field prefix solely to reach `current_lock_operation`.

## January code-shape evidence

Adding only `_rasterizer_lock_flag = 11` to the former partial public enum
changed VC7's ordering of independent loads in
`rasterizer_dynamic_geometry_initialize`, making the previously exact
480-byte owner residual.  Replacing the partial declaration with the complete
symbols-authenticated enum naturally restores that function.  The focused
gate reports all **36/36** functions in
`rasterizer_xbox_draw_primitives.obj` strict exact, including the initializer.

This compiler sensitivity corroborates the complete header context but is not
the basis for inventing it: every retained name and numeric value comes from
the independent symbols build.  No filler enumerators, duplicate prototypes,
compiler barriers, pragmas, inline assembly, or TU-local ownership fiction
was introduced.

## Focused verification

PID-isolated gates were run for all 14 affected translation units with
`--forbid-emitted-symbol _point_from_line3d`.  Every guard passed.  Comparing
each gate's exact-owner names with `scratch/batch8-base.json` showed an
identical exact set in every object:

| Object | Exact owners |
| --- | ---: |
| `bink_playback.obj` | 19 |
| `render_sprite.obj` | 6 |
| `game_engine_king.obj` | 26 |
| `render_contrails.obj` | 6 |
| `cache_files_windows.obj` | 48 |
| `cinematics.obj` | 16 |
| `rasterizer_debug.obj` | 12 |
| `rasterizer_xbox_draw_primitives.obj` | 36 |
| `rasterizer_xbox_decals.obj` | 27 |
| `rasterizer_xbox_detail_objects.obj` | 20 |
| `rasterizer_xbox_hardware_bitmaps.obj` | 18 |
| `rasterizer_xbox_transparent_geometry.obj` | 21 |
| `lightning.obj` | 7 |
| `flags.obj` | 14 |

Thus the owner correction restores the initializer without losing or swapping
any previously exact function.  `flag_render_proper` remains honestly
residual and receives no exact credit.  A global configure/build was left to
the campaign orchestrator, as required by the lane's integration protocol.
