# `main.obj` natural state and render packet (2026-09-09)

## Result

Against published base `7a1d0d91d74c316f8e8b27a2308d3187da429a39`,
the focused Main gate advances from 77 exact / 7 residual / 11 unwritten to
81 exact / 9 residual / 5 unwritten. All inherited exact owners remain exact.

| Owner | Meaningful | Padded |
| --- | ---: | ---: |
| `_halt_and_catch_fire` | 658 | 672 |
| `_main_pregame_render` | 302 | 304 |
| `_main_rasterizer_throttle` | 514 | 528 |
| `_main_vertical_blank_interrupt_handler` | 184 | 192 |
| **Total** | **1,658** | **1,696** |

Meaningful credit remains conditional on the final stable canonical sweep.

## Source and ownership

The packet restores the authenticated 0x620-byte private Main state and the
packed 0x38B-byte screenshot/framerate state. Target-observable fields now
have semantic names and compile-time offset assertions; no address-derived
source identifier or raw offset access is used. The candidate Main BSS has the
target's 2,659-byte extent.

The strict bodies are natural C corroborated by January disassembly and the
historical exact Main source at `fd0de0a58` / `ef64130c8`. Required
cross-translation-unit contracts were restored at their owners:

- `rasterizer_transparent_geometry_draw(boolean)` in the
  `RASTERIZER_TRANSPARENT_GEOMETRY.C` section of
  `source/rasterizer/rasterizer.h`;
- `render_camera_get_adjusted_field_of_view_tangent(real)` in
  `source/render/render_cameras.h`;
- the two-argument `render_frame_pregame(window, bitmap)` declaration and
  definition in `source/render/render.h` and `render.c`; and
- the already-defined idle/lapsed profiler APIs in
  `source/cseries/profile.h`.

The rasterizer global schema now names the target-proven 0x20 D3D flip count,
0x28 live frame/vblank pair, 0x30 previous pair, and 0x3E/0x40 throttle
controls while preserving the existing 64-bit `vertical_blank_index` alias
used by exact callers. Its initializer preserves every byte at its original
offset. The complete `rasterizer.obj` function gate remains 131/131 exact,
and `render.obj` remains 13/13 exact after the owner corrections.

## Honest fuzzy work

Two complete natural bodies are retained at zero exact credit:

- `main_frame_rate_debug`: target 538 meaningful / 544 padded bytes;
  candidate 528 padded bytes with 51 of 52 target relocations and
  approximately 94.67 percent similarity.
- `main_update_time`: target and candidate are both 1,440 padded bytes;
  candidate has 118 relocations versus 117. The former
  `_ReadWriteBarrier` experiment is not retained.

These bodies contain no volatile/codegen pressure, dead anchors, raw-address
access, or fake branches. Their final report-derived measurements are entered
in `config/parked.json`, and neither receives exact credit.

The remaining unwritten owners are `main_framerate_render`,
`main_game_render`, `screenshot_render`, `set_window_camera_values`, and
the rejected null-write `main_crash`. They receive no credit.

## Focused verification

- Main all-owner gate: 81 exact / 9 residual / 5 unwritten.
- Forbidden emitted `_point_from_line3d` guard: pass.
- Scoped fake-match scan across Main and the owner-header packet: zero leads.
- `render.obj`: 13 exact / 0 residual / 0 unwritten.
- `rasterizer.obj`: 131 exact / 0 residual / 0 unwritten.
- `git diff --check`: pass.

The orchestrator still owns the full build, complete 8,245-owner stable diff,
park validation, dependent-header sweep, test suite, and publication verdict.
