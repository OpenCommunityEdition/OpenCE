# `rasterizer_xbox_environment_fog.obj` window-owner reconciliation (2026-09-13)

## Result

The fog translation unit now consumes the shared
`struct rasterizer_window_begin_parameters` from `rasterizer.h` instead of a
second private layout with renamed fields.  The focused gate remains 24 exact,
3 residual, and 0 unwritten functions.  This cleanup claims no new exact-byte
credit.

## January evidence

Target instructions in `_rasterizer_environment_fog_screen_begin` copy 52
bytes from `global_window_parameters + 0x08` during first-time initialization.
The shared layout establishes `+0x08` as `camera`; `frustum.world_to_view`
starts at `+0x6c`.  The source therefore deliberately preserves the January
copy from `&global_window_parameters.camera` into the matrix-sized destination.
Although the behavior looks surprising, it is target-proven rather than a
matching invention.  Subsequent matrix operations and the per-frame copy use
`global_window_parameters.frustum.world_to_view`.

The discarded bitmap lookup now uses the subsystem accessor
`(void)bitmap_group_get(...)`.  The water visibility getter is declared by its
own `rasterizer_xbox_water.h` owner rather than locally in the fog consumer.

## Verification

```text
python tools/campaign/gate.py source/rasterizer/xbox/rasterizer_xbox_environment_fog --all --forbid-emitted-symbol _point_from_line3d
emitted-symbol guard passed
exact 24, residual 3, unwritten 0
```
