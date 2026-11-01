# Geometry flicker: issue 83

[Issue 83](https://github.com/chestahh/GulchCE/issues/83) reports disappearing
geometry on Windows and Linux with AMD, NVIDIA and Intel GPUs. Its attached
Blood Gulch clip shows sections of distant terrain appearing and disappearing
as the view turns. The report also names 343 Guilty Spark (`c10`).

## Fix already in the requested base

The investigation started from `origin/testing` at
`fc9c57cf3467502dc159a41ee7f54d6b01499945`. That revision already includes
OpenCE commit `521c8c8d5ab65094a31f960ff15c1191f64ef005`, "Stop the level
flashing: a portal corner on a clip line makes no NaN corner."

The corrected visibility path is:

1. `structure_visibility_traverse_cluster` in
   [structure_visibility.c](../source/structures/structure_visibility.c)
   intersects the visible screen region with a portal's outline.
2. `convex_hull2d_intersect` in [geometry.c](../source/math/geometry.c)
   repeatedly calls `convex_polygon2d_clip_to_plane`.
3. A corner on the clipping line and a preceding corner just behind it can
   produce a `0/0` intersection scale through floating-point rounding.
   Clamping does not remove a NaN (a nonnumeric floating-point result).
4. Invalid corners contaminate subsequent intersections. An empty result
   prevents traversal into the next cluster, so its surfaces are not drawn.

The upstream correction selects the current corner when that scale is NaN.
It retains normal clipping and does not disable visibility rejection. This
CPU visibility defect explains why a GPU-independent failure can look like
a graphics-driver problem. The report's appearance is consistent with this
defect; the clip alone does not prove that every reported occurrence has the
same cause.

This assignment preserves that implementation and its `port:` explanation.
It adds regression coverage and evidence rather than another renderer
workaround. No map loader, MCC adapter, shader, setting, network message or
simulation state changes. A network-version increment is not needed.

## Regression coverage

[test_polygon_clip.py](../tools/harness/tests/test_polygon_clip.py) compiles
the actual geometry functions and math helpers, without retail assets. Its
two upstream fixtures are portal outlines dumped from Sali Creek, not data
captured from the issue's video.

The additional cases check that:

- A valid intersection remains finite and nonempty when passed through
  successive portal intersections.
- Fully visible polygons and their clipping flags remain unchanged.
- Fully hidden and edge-only intersections remain rejected.
- An in-place clip preserves the correct half of the polygon and updates
  clipping flags correctly.

Negative controls remove the NaN guard only in generated temporary test
copies. Both original fixtures and the successive-portal case then fail.
The existing independent control also detects suppressed intersection output.

## Validation and limits

The nine polygon-clipping tests pass on Windows with a 32-bit clang harness.
This includes the negative controls; their expected failures establish that
the tests detect the original defect. The Windows release build uses
`--pgo=off --lto=off` and succeeds with existing Expat import/link warnings.
After final review, all 31 focused tests pass: polygon clipping, GPU
visibility readback, Custom Edition geometry bounds, and shadow setup.

Hidden-window runtime checks use Windows, an NVIDIA GeForce RTX 5070 Ti,
OpenGL 4.5 (driver 617.42), 1280x720, a 30 FPS cap, reversed depth enabled,
default optional graphics features, and scripted `look:1` camera input.
The first run enters local Blood Gulch gameplay. Its map-name console commands
do not leave the network session, so it supplies no map-transition evidence.
A separate solo run loads `c10`, then Custom Edition `deathisland`, MCC
`DigsiteMP/1`, and Xbox `bloodgulch` in the same process. Logs establish the
family transitions. MCC reports its existing rejection of the map's
`host_spawn` script at unsupported `debug_camera_load`; that limitation is
not changed here.

Both processes exit normally. The runs retain 27 and 21 periodic captures,
respectively; sampled captures show rendered scenes in all three map families.
The `c10` observation covers the opening cinematic, not later campaign play.
Periodic stills do not establish the absence of a single-frame disappearance.

These are short rendering checks, not campaign completion or a replay of
the original camera path. The map files and saves are isolated copies.
Disposable map copies and generated map caches are removed; test saves are
preserved in verified ZIP archives beside the logs and captures under the
worktree's `build/issue83/` directory. No retail assets are committed.

The final review covers the unchanged clipping fix and its visibility caller,
the added tests, and this explanation. No additional production refactoring
is justified. Tests and documentation do not modify Android or web runtime
code; full Android/web and Linux builds are not part of this validation.
The original reporters' exact camera path and their AMD/Intel/Linux systems
remain unverified. Passing the synthetic tests is not proof of flicker-free
rendering on every map or GPU.
