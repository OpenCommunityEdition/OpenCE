# `structure_visibility.obj` full reconstruction (2026-09-02)

## Result

`source/structures/structure_visibility.c` is now source-complete: all 21
January code owners have credible implementations and semantic names. The
strict per-function gate reports:

- 10 exact functions / 1,744 padded bytes / 1,693 meaningful bytes;
- 11 credible residual functions / 5,920 padded bytes;
- 0 unwritten functions.

The inherited baseline gate was 2 exact, 0 residual, and 19 unwritten. Its two
implemented owners, `_debug_pvs` and `_dequantize_byte_to_real_rectangle3d`,
totaled 320 padded bytes and 297 meaningful bytes. This wave therefore adds
eight strict functions, 1,424 padded exact bytes, and 1,396 meaningful exact
bytes while writing every previously unwritten owner.

The object is deliberately not called byte-complete. Its remaining 11
ordinary-C residuals are measured and fuzzy-parked in `config/parked.json`;
their source is retained because it is coherent, typed, and evidence-backed.

## Exact owners

| Function | Padded bytes | Meaningful bytes | Status |
| --- | ---: | ---: | --- |
| `_debug_pvs` | 32 | 18 | inherited exact |
| `_structure_visibility_find_camera` | 192 | 184 | new exact |
| `_bounding_rectangles_intersect` | 176 | 175 | new exact |
| `_structure_visibility_find_objects` | 208 | 196 | new exact |
| `_grow_clipping_rectangle_by_portal_hull` | 176 | 176 | new exact |
| `_structure_visibility_render_debug_portal_hull` | 160 | 160 | new exact |
| `_dequantize_byte_to_real_rectangle3d` | 288 | 279 | inherited exact |
| `_portal_hull_from_portal` | 128 | 126 | new exact |
| `_points_within_distance` | 112 | 109 | new exact |
| `_structure_visibility_find_clusters` | 272 | 270 | new exact |

`_structure_visibility_find_clusters` initially differed only because csplit
spelled its BSS relocation through an anonymous owner. Splitting and naming
the target BSS in `config/symbols.json` makes the ordinary strict comparison
exact; no semantic exception is used.

## Credible residuals

| Function | Target/base padded bytes | Relocations target/base | Objdiff |
| --- | ---: | ---: | ---: |
| `_structure_visibility_traverse_subclusters` | 416 / 416 | 24 / 24 | 98.42519% |
| `_structure_visibility_traverse_surface_lists` | 528 / 528 | 27 / 27 | 71.402435% |
| `_planes_intersect_rectangle` | 384 / 368 | 8 / 8 | 91.68148% |
| `_structure_visibility_build_surfaces_traverse_clusters` | 352 / 352 | 7 / 7 | 77.694916% |
| `_structure_visibility_build_surfaces_traverse_leaf` | 480 / 480 | 27 / 27 | 91.46154% |
| `_portal_hull_from_points` | 480 / 464 | 16 / 16 | 85.31645% |
| `_structure_visibility_find_mirror` | 704 / 688 | 18 / 18 | 88.52941% |
| `_structure_visibility_build_surfaces_traverse_node` | 608 / 624 | 26 / 26 | 74.99083% |
| `_structure_visibility_traverse_cluster` | 912 / 896 | 53 / 51 | 88.952896% |
| `_structure_visibility_build_surfaces` | 480 / 480 | 18 / 18 | 83.905266% |
| `_structure_visibility_compute` | 576 / 544 | 48 / 47 | 84.6474% |

The parked records use the fail-closed `unclassified` class. The codegen gaps
look like register, aggregate-lifetime, recursion, loop, and x87 scheduling
choices, but the current evidence does not prove a unique compiler mechanism.
The records intentionally make no stronger claim.

## Evidence and naming

January's split COFF remains the byte and ABI authority. The reconstruction
used its complete instruction streams, relocations, padding, assertion/error
strings, static-data shapes, and call graph. HCEA's Xbox 360 structure
visibility family supplied independent semantic behavior and the names of all
private helpers except the debug hull renderer. The reconstructed PC-family
source and cross-build correspondence were secondary corroboration only; no
later-platform instruction sequence displaced January evidence.

Recovered private mappings are:

| January address | Recovered owner |
| --- | --- |
| `00185F00` | `structure_visibility_traverse_subclusters` |
| `001860A0` | `structure_visibility_traverse_surface_lists` |
| `001862B0` | `bounding_rectangles_intersect` |
| `00186360` | `planes_intersect_rectangle` |
| `001865B0` | `grow_clipping_rectangle_by_portal_hull` |
| `00186660` | `structure_visibility_render_debug_portal_hull` |
| `00186820` | `structure_visibility_build_surfaces_traverse_clusters` |
| `00186980` | `structure_visibility_build_surfaces_traverse_leaf` |
| `00186B60` | `portal_hull_from_points` |
| `00186D40` | `portal_hull_from_portal` |
| `00186DC0` | `points_within_distance` |
| `001870F0` | `structure_visibility_build_surfaces_traverse_node` |
| `00187350` | `structure_visibility_traverse_cluster` |
| `001878C0` | `structure_visibility_find_clusters` |

The name `structure_visibility_render_debug_portal_hull` is an explicitly
documented inference, not an HCEA-authenticated symbol. Its unique caller,
argument roles, exact projection math, line-loop behavior, and use under the
`debug_portals` condition make the name descriptive and subsystem-consistent.
The other mappings are corroborated by HCEA/cross-build names.

The target symbol manifest now also names:

- `screen_plane` at `002A1AF4`;
- `render_structure_visibility_portal_traversal` at `00313AF0`;
- `render_structure_visibility_subcluster_traversal` at `003140E8`;
- `render_structure_visibility_surface_traversal` at `003146E0`;
- `warned_about_missing_subclusters` at `004C0CF0`;
- `structure_visibility_globals` at `004C0CF8`.

All recovered private functions and private data/BSS owners are marked static
in `config/symbols.json`. `structure_sphere_threshold` retains its observed
external ownership.

## Global ownership

January places `structures_use_pvs_for_vs` and `debug_portals` as adjacent
one-byte external common symbols in `source/linker_common.obj`, at image
addresses `004ED521` and `004ED522`. They are read and written by this
subsystem, and `debug_pvs` sets both. The source now gives them real tentative
definitions in `structure_visibility.c`, in January address order, and exposes
typed `extern boolean` declarations from `structure_visibility.h`.

The resulting COFF symbols have external storage, undefined/common section
number zero, and value one (the common allocation size), which is the correct
VC7 representation. The function gate is unchanged. The generated HS external
global table still refers to the same external symbol names, so its relocation
destinations remain stable.

## The dequantization inline schedule

The earlier exact first shot used an include-time macro rename to hide
`real_math.h`'s external `dequantize_byte_to_real` prototype from a same-named
private inline helper. That workaround is not retained.

The scalar helper is now the honest TU-private
`structure_visibility_dequantize_byte_to_real`. The rectangle function calls
that semantic name six times in source order, VC7 still inlines all six copies,
and `_dequantize_byte_to_real_rectangle3d` remains strictly exact at 288 padded
bytes with its six literal relocations. VC7 also emits an unreferenced 48-byte
private select-any COMDAT for the inline helper. No relocation in the object
targets that copy. An XDK `Link.Exe /OPT:REF /INCREMENTAL:NO` diagnostic link,
rooted only at `_dequantize_byte_to_real_rectangle3d` and supplied with inert
definitions for the compiland's otherwise unresolved imports, emitted exactly
`0x120` bytes of `.text`: the target rectangle function. Its map contains no
`_structure_visibility_dequantize_byte_to_real`, proving that the ordinary
linker discards the unreferenced select-any owner. This is the compiler's normal
inline representation, not a target function claim. `__forceinline` was also
tested, but it neither suppressed the owner nor satisfied the fake-match scan,
so the source retains the ordinary, semantic `__inline` spelling. The final
object contains no `point_from_line3d` symbol/COMDAT.

## Public declaration ownership

This wave adds `source/structures/structure_visibility.h`, which owns the
public visibility APIs and `render_mirror`. Ad-hoc declarations and the
duplicate mirror layout were removed from `render.c`; `hs.c` now includes the
owner header for `debug_pvs`.

Existing public functions implemented in `geometry.c` are declared by
`math/geometry.h`:

- `convex_polygon3d_clip_to_plane`;
- `convex_hull2d_intersect`.

Existing public camera/frustum functions implemented by `render_cameras.c`
are declared by `render_cameras_internal.h`, including projection bounds,
frustum visibility tests, clipped/full frustum bounds, and camera mirroring.
TU-private layouts and helper declarations remain in
`structure_visibility.c`.

## Header blast gates

Every direct C includer was enumerated before admission. Pristine-versus-
patched per-function gates were identical for all consumers:

| Consumer | Result |
| --- | --- |
| `source/math/geometry` | 12 exact, 18 unwritten; unchanged |
| `source/structures/structure_lens_flares` | 3 exact, 1 pre-existing residual; unchanged |
| `source/physics/breakable_surfaces` | 11 exact, 1 pre-existing residual; unchanged |
| `source/game/game_engine_king` | 26 exact, 2 pre-existing residual, 1 unwritten; unchanged |
| `source/interface/hud_nav_points` | 30 exact, 2 pre-existing residual; unchanged |
| `source/render/render_particles` | 2 exact, 1 pre-existing residual; unchanged |
| `source/render/render` | 13/13 exact; unchanged |
| `source/hs/hs` | 445 exact, 3 pre-existing residual; unchanged |
| `source/units/units` | 189/189 exact; unchanged |

The final post-definition Units gate also reports 189 exact, zero residual,
and zero unwritten.

## House-rule audit

- Public declarations live in their owner headers; private layouts/functions
  remain in the owning C file.
- Typed tag-block and object access uses project macros rather than repeated
  raw `tag_get`/`object_get` casts.
- Real values use project `real` and real geometry types.
- Bit-vector operations use cseries macros.
- Functions use one parameter per line and explicit returns.
- No anonymous `code_`, `data_`, `rdata_`, or `bss_` owner remains in this
  compiland's source inventory or target symbol metadata.
- No volatile/register forcing, compiler barrier, pragma, inline assembly,
  raw byte emission, fake dependency, or inert matching local is retained.
- The only `goto` reproduces the legitimate cached-leaf path in
  `structure_visibility_find_camera`; it is not a next-label fake-match shape.

## Verification

- Strict visibility gate: 10 exact, 11 residual, 0 unwritten.
- `config/parked.json`: all 11 residuals validated active; repository total
  139 active, 0 stale, 0 invalid at admission.
- Full 551-step Ninja build/report/progress: pass, zero unit errors.
- Halo campaign progress at this branch: 721,235 / 1,770,166 exact code
  bytes and 5,195 / 7,574 exact functions.
- Units gate: 189/189 exact.
- Fake-match scan over all six changed source/header consumers: 0 findings.
- Python test suite: 261 passed.
- `git diff --check`: pass.
- Final COFF symbol scan: no `point_from_line3d` symbol. The expected private
  select-any scalar-helper COMDAT is unreferenced; an XDK `/OPT:REF` link probe
  emitted only the exact 288-byte rectangle function and discarded the helper.

The remaining work is exact-codegen closure of the 11 measured residuals,
not missing behavior. Reopen them only when authoritative January source/local
records or a natural same-compiler donor provides new evidence.
