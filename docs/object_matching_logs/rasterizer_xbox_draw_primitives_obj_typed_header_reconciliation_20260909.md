# `rasterizer_xbox_draw_primitives.obj` typed-header reconciliation

## Result

The complete ordinary-C reconstruction remains canonical, but
`rasterizer_dynamic_geometry_initialize` is now recorded as fuzzy and receives
zero exact credit. The other 35 functions in the object remain strict exact.

January and the retained initializer are both 480 padded bytes with all 35
relocation identities. The production report measures 93.59494 percent
similarity:

| owner | size | relocations | normalized SHA-256 |
| --- | ---: | ---: | --- |
| January | 480 | 35 | `8dcf639e6a56145f391857236a9079531fa32e6cf447291cd9d1a9d13b896afa` |
| canonical | 480 | 35 | `bda00785ec3b03ddfe11744b93f62d5752c743f4f57b43405e265a2304c07502` |

## Why prior exact credit was retired

The earlier 36/36 result depended on a broad rasterizer header retaining model
declarations with incomplete or weaker parameter types. The publication audit
moved those APIs to `rasterizer_models.h`, their typed subsystem interface, and
made the implementation and callers agree on `render_sort_filth` and
`model_geometry_part`. The frame-statistics definition likewise keeps three
meaningful reflection-counter fields rather than replacing them with a numeric
array solely to influence VC7.

Those source corrections change only the initializer's independent stack-load
schedule. They do not change its control flow, calls, constants, storage
destinations, size, or relocation graph. Reintroducing the old hash would mean
publishing weaker or duplicate declarations, so this function is honestly
parked instead of receiving inappropriate matching credit.

## Verification

`tools/campaign/gate.py source/rasterizer/xbox/rasterizer_xbox_draw_primitives
--all --forbid-emitted-symbol _point_from_line3d` reports 35 exact, one
residual, and no unwritten functions. The residual is registered in
`config/parked.json` as zero-credit `tu-context-optimization` evidence.
