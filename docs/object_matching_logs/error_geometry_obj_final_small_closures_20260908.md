# `error_geometry.obj` final small closures (2026-09-08)

## Result

Four previously non-exact functions are now strict byte-and-relocation matches.
No fuzzy function is counted.

| Function | Meaningful bytes | Padded bytes |
| --- | ---: | ---: |
| `_error_geometry_polygon_list` | `735` | `736` |
| `_error_geometry_polygon_mesh__textured_with_no_import_scale` | `576` | `576` |
| `_error_geometry_bounded_triangle` | `606` | `608` |
| `_error_geometry_set_transform` | `1,008` | `1,008` |
| **New exact credit** | **`2,925`** | **`2,928`** |

The focused object gate advances from `13 exact / 0 residual / 4 unwritten`
at canonical commit `4ebe0ff6ec889aa7ae5657086afb51b175694aff` to
`17 exact / 0 residual / 0 unwritten`.

## Evidence and source form

The polygon-list implementation is the complete three-pass January mesh
writer reconstructed from the target instruction and relocation stream and
cross-checked against the annotated HCEA decompile in
`research-cache/stian-halo-current-20260827/src/halo/main/main.c`. It preserves
the single running point index, per-polygon colors, triangle-fan indices,
exact VRML strings, transform call, and flush. Expressing the two related
inner-loop increments together in the natural `for` increment expression
reproduces January's scheduling without a fake dependency or compiler trick.

The textured mesh and bounded-triangle bodies came from the preserved dirty
tail of the Fable small-family lane. Their semantics are supported by the
target calls, strings, constants, and sibling Error Geometry routines. Moving
the genuine point-count assignment to its first use and ordering the two
independent bounds-color member assignments as January did closes the last
ordinary scheduling differences.

`error_geometry_set_transform` is not handwritten expansion or guessed
logic. It uses the existing owning `match_assert_valid_real_matrix4x3` macro,
whose authentic diagnostic sequence generates the complete 1,008-byte
January body, followed by the typed matrix copy. Its public prototype is in
`source/tool/error_geometry.h`.

All bodies use project math types and named globals, contain explicit returns,
avoid address-named identifiers and inline assembly, and preserve the January
inline schedule without emitting `_point_from_line3d`.

## Focused validation

- `python tools/campaign/gate.py source/tool/error_geometry --all
  --forbid-emitted-symbol _point_from_line3d`: `17 exact / 0 residual /
  0 unwritten`; emitted-symbol guard passes.
- The four exact functions match normalized instructions, ordered relocation
  identities, meaningful size, and padded size.
- Repository-wide stable regression and consumer checks are performed by the
  batch orchestrator before publication.
