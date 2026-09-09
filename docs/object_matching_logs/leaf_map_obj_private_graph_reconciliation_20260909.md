# Leaf Map private graph reconciliation (2026-09-09)

## Result

The interrupted Opus Leaf Map work was selectively reconstructed on top of
published canonical commit `b74f7a2d7`.  The donor translation unit was not
copied wholesale: it duplicated a public structure, carried declarations in
the wrong owner, emitted a forbidden `point_from_line3d` helper, and used two
representation-pointer casts in plane projection.

The admitted packet adds eight strict function owners:

| owner | meaningful bytes | padded bytes |
| --- | ---: | ---: |
| `_node_stack_push` | 69 | 80 |
| `_node_stack_pop` | 64 | 64 |
| `_map_leaf_find_face_on_node` | 55 | 64 |
| `_leaf_map_build_portal_from_leaves` | 818 | 832 |
| `_normalize_three_dee` | 122 | 128 |
| `_leaf_map_build_portals` | 231 | 240 |
| `_leaf_map_build_leaf_faces` | 215 | 224 |
| `_leaf_map_initialize_from_bsp` | 292 | 304 |
| **total** | **1,866** | **1,936** |

The rename-stable whole-tree sweep records no exact regression.  Leaf Map is
now `16 exact / 9 residual / 0 unwritten` among its 25 listed function owners.
This is function progress, not whole-object completion: January's `.data` is
2,512 bytes versus canonical's 2,044, and January's `.bss` is 1,034 bytes
versus canonical's 1,032.

## Evidence and names

Read-only source donor:

- worktree `C:\halo-worktrees\opus-small-families-30k-20260902`
- tip `eeedd72e5ba0fc1761519f68cad241fe606320e7`
- frozen donor object `scratch/w6_leaf_map.obj`

The Halo CE same-build symbol atlas authenticates these former address labels:

- `_find_like_crossing`
- `_map_leaf_find_face_on_node`
- `_leaf_map_build_portal_from_leaves`
- `_normalize_three_dee`
- `_intersect_planes3d`
- `_leaf_map_build_portals_from_leaf`
- `_leaf_map_build_leaf_face_for_leaf_on_node`
- `_leaf_map_build_portals`
- `_leaf_map_build_leaf_faces_for_leaf`
- `_leaf_map_build_leaf_faces`

January assertion strings independently authenticate `leaf_map_globals`,
`MAXIMUM_NODE_STACK_COUNT`, `index_from_node`, and the source path/line
topology.  The first three stack-helper names are descriptive private names
consistent with their operations; no `code_XXXXXXXX` owner remains in this
object.  The profile record and polygon seed are now named
`leaf_map_initialize_section` and `global_leaf_face_polygon`, and the anonymous
BSS aggregate is named `leaf_map_globals`.

The public initializer prototype now lives in `structures/leaf_map.h`.
`convex_polygon2d_clip_to_plane` and `convex_hull2d_perimeter` declarations now
live in their actual owner, `math/geometry.h`, instead of the Leaf Map
translation unit.  All C includers of both changed headers were compiled by
focused gates and the complete rename-stable sweep.

## Zero-credit retained work

Four natural functions have identical normalized code, sizes, relocation
counts, relocation offsets, and resolved relocation destinations, but csplit
spells one address through the preceding symbol:

| owner | padded bytes | differing relocation | resolved January address |
| --- | ---: | --- | ---: |
| `_node_stack_read` | 64 | target `_faked_xbox_command_line+4`; ours `_leaf_map_globals-4` | 4,982,956 |
| `_find_like_crossing` | 160 | same | 4,982,956 |
| `_leaf_map_build_leaf_face_for_leaf_on_node` | 608 | same | 4,982,956 |
| `_leaf_map_build_leaf_faces_for_leaf` | 112 | same | 4,982,956 |

These are recorded as `csplit-relocation-alias` parks.  They receive **zero
strict-exact credit** and are accepted only by the resolved-address validator;
no source was distorted to force a different symbolic spelling.

Two complete semantic reconstructions also remain honest fuzzy parks:

| owner | January / canonical padded | relocations | objdiff |
| --- | ---: | ---: | ---: |
| `_intersect_planes3d` | 368 / 400 | 15 / 15 | 83.8% |
| `_leaf_map_build_portals_from_leaf` | 480 / 432 | 17 / 17 | 63.948864% |

`intersect_planes3d` uses typed temporary points instead of the donor's
vector/point pointer casts.  The recursive portal traversal keeps the natural
signed node-designator comparisons; the donor worktree's dirty sign-bit
rewrites were rejected.  Both functions receive zero exact credit.

## Validation

- PID-isolated Leaf Map gate: `16 exact / 9 residual / 0 unwritten`.
- Forbidden-owner gate: no emitted `_point_from_line3d`.
- Focused gates passed for every direct Geometry and Leaf Map header consumer.
- `resolved_address_scan.py` proved all four alias destinations above.
- Rename-stable whole-tree comparison: eight Leaf Map gains and zero losses.
- Park metadata was refreshed for two pre-existing fuzzy bodies whose
  translation-unit allocation changed after the properly owned Geometry
  declarations were introduced; neither receives exact credit.

