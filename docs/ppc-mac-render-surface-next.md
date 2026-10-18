# Native surface lookup needed by object updates

Intel's current native `objects_update` run gets through weapons and unit
dialogue, then reaches
`unit_update -> lights_illumination_at_point -> structure_test_vector ->
structure_render_surface_from_point_and_leaf`. The assertion for surface
reference index 2168 is expected with the current world view: it validates
leaf ranges against the native surface-reference count, but leaves
`structure.surface_references` empty. `structure_render_surface_from_point_and_leaf`
indexes that block for every collision hit before it can decide whether a
matching lightmapped surface exists.

## Minimum real data batch

Extend `engine_world`'s scoped native BSP view with these connected records:

1. Decode the structure surface-reference block (8-byte rows: surface index,
   BSP3D node index) and populate `structure.surface_references`. The count is
   already read from the BSP root at `+0xec`; use its paired address field and
   validate every leaf's existing `[first, first + count)` range against the
   count. Each non-`NONE` surface index must be in the surface block, and each
   non-`NONE` node index must be in the converted collision BSP3D nodes.
2. Decode surface rows (6 bytes, three little-endian `uint16_t` vertex
   indices) into `structure.surfaces`. The existing geometry reader locates
   this block at root `+0xf8` and preflights all triangle indices against each
   material's vertex count. Keep that validation in the conversion path.
3. Populate native lightmap rows and their nested material blocks. The
   existing validator/geometry reader locates lightmaps at root `+0x104`,
   with 32-byte disk rows, and each row's material block at `+0x14`. Native
   `structure_lightmap` and `structure_material` objects need engine-owned
   blocks. Witness each native size and field offset before decoding; matching
   strides alone do not make little-endian disk pointers or scalars native.
4. For each material, retain its first surface and surface count, vertex type,
   vertex count, and a native position array reachable through
   `compressed_vertex_data.address`. The lookup macro in `structures.c` reads
   an `environment_vertex_compressed` position from this field, and
   `environment_vertex_compressed_get_point` consumes its first 12 bytes.
   The retail geometry decoder already validates the 56-byte disk environment
   vertices and triangle indices and emits their positions, but its callback
   currently omits the material's first-surface index and lightmap/material
   indices. Expose those metadata fields or pair the callback with a bounded
   disk-row walk; do not infer association from shader handles alone.

This stays within the actual structures and collision BSP and does not require
GPU drawing. The same batch must decode and publish the genuine
`structure.lightmap_group` reference and its bitmap assets before this path is
claimed complete. Preserve `NONE` only when the map itself has no lightmap
group; do not clear a real reference to bypass sampling or accept synthetic
black lighting. A zero-initialized index 0 is also not a valid substitute.
Confirm native field offsets and vertex attributes with compile-time witnesses
before implementing this research plan.

## Validation before publishing the view

- Bound block counts and checked `count * stride` arithmetic before resolving
  spans; reject nonempty blocks with null, misaligned, or out-of-BSP ranges.
- Check all leaf reference ranges, surface-reference indices, collision node
  indices, lightmap/material nested ranges, and each material's surface range.
- Require each referenced surface vertex index to be within that material's
  vertex count. The retail geometry preflight already checks every triangle
  index before its first callback; preserve that all-or-nothing behavior.
- Require finite vertex positions before copying to engine memory. The
  existing geometry reader checks all vertex floats and vector limits.
- Copy into owned native allocations and free them with the scoped world;
  source cache pointers cannot survive cache unmapping.
- Decode and load the real lightmap tag and bitmap resources before invoking
  illumination. Preserve source lightmap bitmap indices so
  `structure_test_vector` can distinguish lit from unlit surfaces.

The next likely boundary after this path is object light lookup by cluster.
`lights_illumination_at_point` calls `find_point_lights_for_object_in_cluster`
after the surface lookup; with an empty light partition it should return no
lights, but its marker globals and partition lifecycle must already be
initialized by the real object/light startup path. This is separate from the
surface records above and should be diagnosed from the next run rather than
filled with synthetic data.

Evidence: `source/structures/structures.c` (surface lookup and vector test),
`source/structures/structure_bsp_definitions.c` (surface-to-material search),
`source/objects/object_lights.c` (illumination query),
`source/structures/structure_bsp_definitions.h` (native surface/lightmap/material
types), `port/linux/game/cache_file_formats.c` (validated disk strides and
vertex layout), `port/mac/retail_geometry.c` (bounded geometry decoding), and
`port/mac/engine_world.c` (currently decoded leaves/clusters but empty surface
reference, surface, and lightmap blocks).
