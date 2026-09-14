# `leaf_map.obj` Opus5 150K house-clean wave w1 (2026-09-14)

## Result

No source change is admitted to `source/structures/leaf_map.c` by this worker
(`git diff --stat` empty). The final real-file gate is unchanged from baseline:
**16 exact / 9 residual / 0 unwritten**. The `_point_from_line3d` guard passes,
all six parks match their recorded measurements, the owner census against
`build/base` shows no new or lost owner, and the fake-match scan reports zero leads.

All nine non-exact rows now have a proven mechanism and a scratch candidate that
reproduces January's normalized bytes. None of them can be landed by a worker:
each depends on a header change, a policy ruling, or a park re-baseline.

| function | January padded | blocker | scratch candidate | measured |
| --- | ---: | --- | --- | --- |
| `_leaf_face_get_vertex3d` | 176 | `real_math.h` `projection_from_vector3d` body | shadow header only | strict EXACT |
| `_leaf_map_get_leaf_bounds` | 784 | same header + radius tail shape | `scratch/workers/leaf_map/b10.c` | strict EXACT (with the header) |
| `_leaf_map_leaf_spans_polygon` | 656 | same header + `_point_from_line3d` COMDAT policy | `scratch/workers/leaf_map/s4.c` | target sha; forbidden COMDAT |
| `_intersect_planes3d` (park) | 368 | same header + representation-cast policy | `scratch/workers/leaf_map/p1.c` | strict EXACT; cast form previously rejected |
| `_leaf_map_build_portals_from_leaf` (park) | 480 | csplit relocation alias only | `scratch/workers/leaf_map/f7.c` | target sha, resolved-address identical |
| `_node_stack_read`, `_find_like_crossing`, `_leaf_map_build_leaf_face_for_leaf_on_node`, `_leaf_map_build_leaf_faces_for_leaf` (parks) | 944 | csplit relocation alias | unchanged | not claimed |

The combined scratch candidate `scratch/workers/leaf_map/pk2.c` contains all four
source bodies, compiled against the shadow header. It gates **20 exact / 5 residual**, and all five
residuals are `reloc-identity` rows with the known `_faked_xbox_command_line+4` /
`_leaf_map_globals-4` alias. Every `leaf_map.obj` code section is therefore
byte-identical to January modulo relocation naming. The policy-free subset
`pk1.c` (b10 + f7) gates **18 exact** under the shadow header and **16/9/0 with no
row change** under the current header.

## Method and measurement harness (scratch only)

The worker edited no header. `scratch/workers/leaf_map/gate_shadow.py` is `tools/campaign/gate.py`
with a leading `/I scratch/workers/leaf_map/shadow` include path. The file
`shadow/math/real_math.h` is the current header (CRLF preserved) with one change:
the `projection_from_vector3d` body is replaced by January's form. That form is the 80-byte
`decals.obj` COMDAT, SHA-256 `70d35416...`, which `decals.c` already reproduces out of line.

The header's blast radius was measured in two passes:

1. `sweep.py` compiled all 447 build units with the shadow header and compared
   per-function size, relocation count and normalized SHA-256 against
   `build/base`. 423 were identical and 23 differed, most of them because other
   workers had in-progress edits.
2. `sweep_verify.py` recompiled each differing unit from one source snapshot,
   once with the real header and once with the shadow header
   (`scratch/workers/leaf_map/sweepv/result.json`).

Only one gate row changed in any unit: `leaf_map::_leaf_face_get_vertex3d` went
from residual to EXACT. No exact function was lost. The 2026-09-04 decals and
game_engine losses do not recur, because `decals.c` now owns its own
out-of-line copy under a rename define.

The header change does alter the hashes of functions that were not exact. Four of them are parks:

- `leaf_map::_intersect_planes3d`: the inlined callee body changes.
- `rasterizer_xbox_models::__rasterizer_model_transparent_geometry_submit`: changes to `cc54c1a8a3f7041a`.
- `bitmap_group::_bitmap_group_add_bitmap`: changes to `0bd88e42b1b36f4e`, the same flip recorded by the 9633a0b4c draft.
- `bitmap_extract::_extract_plateless_cube_map`: changes to `fa197e5bc4cc7c95`.

Three changes are not parks:

- the unparked residual `rasterizer_frame_statistics::_rasterizer_frame_statistics_draw`;
- the surplus `_projection_from_vector3d` COMDAT in `structures`;
- the same surplus COMDAT in `collision_features`.

In both of those objects the surplus COMDAT becomes January's body.

## Per-function evidence

### `_leaf_face_get_vertex3d` (176)
In the base object the only differences are three inlined x87 compares from
`projection_from_vector3d` (January `test ah,1; jne`, ours `test ah,5; jnp/jp`).
The existing source is unchanged. With the shadow header the function is strict EXACT at 176
bytes, 5 relocations, `5b81d3746a420068`.

### `_leaf_map_get_leaf_bounds` (784)
With the header corrected, only the radius tail differed.

- **What January does:** it homes center.z in `[ebp+8]` and the first extent in `[ebp-0x18]`, then sums `(i*i + k*k) + j*j`.
- **What failed:** scalar `half_*` locals gave the wrong homes. `magnitude3d`, `dot_product3d`, `magnitude_squared3d` and `set_real_vector3d` over a `real_vector3d` fixed the homes but not the term order (b1, b3, b7, b9). A `set_real_point3d` corner (b11) and `vector_from_points3d` plus `magnitude_squared3d` (b12) also missed.
- **What matched:** the natural bounding-sphere statement: copy the maximum corner into a `real_point3d`, then `*radius = distance3d(center, &corner)` (b10). This gives 784 / 14 / `c1cd23ce3802f304`, strict EXACT.
- **Census note:** b10 adds `_distance3d` and `_magnitude3d` header-inline COMDATs. They belong to the same surplus class as the `_square_root`, `_dot_product3d`, `_vector_from_points3d` and `_magnitude_squared3d` COMDATs the base object already emits. January references none of them from this object.

### `_leaf_map_leaf_spans_polygon` (656)
With the header corrected, only the x87 term order of `distance0` differs:
January uses x,y,z and ours uses x,z,y.

- **Forms that failed:** reusing `distance0` in the intersection (s1, s2) lost 4 bytes of frame. Scoping the locals to the crossing block (s3) and nesting `vector_from_points3d` (s5) had no effect.
- **What matched:** January's natural inline `point_from_line3d(&point0, &delta, intersection_t, &intersection)` (s4) reproduces the target SHA `b616b3553ac28596`.
- **Why it is blocked:** the object then emits the forbidden 48-byte `_point_from_line3d` COMDAT. This joins the owner-ruling list with weapons, decals and actor_moving.

### `_intersect_planes3d` (park, 368)
January's frame is `sub esp,0x10`, which holds only the projected plane.
`project_point3d` receives `&projected_plane.n` and `result` directly. The
previously rejected cast form (p1) reproduces this exactly under the corrected header:

```c
project_point3d((real_point3d *)&projected_plane.n, projection,
	projection_sign_from_vector3d(&reference_plane->n, projection),
	(real_point2d *)&result->n);
```

No C spelling without a cast can pass these addresses, because the unions have identical layouts but share no member.
Current rules prohibit the cast, so this is not proposed as a park reopen. It is recorded for an owner ruling.

### `_leaf_map_build_portals_from_leaf` (park, 480)
**New lever: the inline frequency law.**

- **Size gap:** the 48-byte gap is exactly January's inlined `map_leaf_find_face_on_node`. Our call sat two if-levels deep inside the two-trip loop, so it stayed out of line. Probes confirmed this: f1 and f5 stay at 432 bytes, while f2, f3 and f4 inline and reach 480.
- **Statement order:** in January's block layout, the find's found-tail comes before the plane-on-stack check. The `else if` therefore follows the descend block.
- **Sign tests:** January tests `first_traversal_node` with `jns`. This is the file's sign-bit designator idiom, already exact in `find_like_crossing`, `leaf_map_build_leaf_faces` and `leaf_map_build_portals`.

The f7 body reproduces normalized SHA `d50628a74d0b5faa`. Only one relocation differs, at +0x6a:
`_faked_xbox_command_line+4` against `_leaf_map_globals-4`. Both resolve to
image address 4982956 through `config/symbols.json`. The body is:

```c
if (ancestor_node_index == NONE && child_index == 0 && !(first_traversal_node & LONG_MIN))
	continue;
if (descend_from_this_node)
{
	short face_index = map_leaf_find_face_on_node(leaf_map,
		TAG_BLOCK_GET_ELEMENT(&leaf_map->leaves, index_from_node(leaf_index), struct map_leaf),
		node_index);
	if (face_index == NONE)
		continue;
}
else if (ancestor_node_index != NONE && plane_on_stack && side == child_index)
	continue;
```

Here `descend_from_this_node` uses `(first_traversal_node & LONG_MIN) != 0`.

Admission checks for f7:

- whole-TU gate is 16/9/0 with no sibling change;
- the guard passes;
- the only park drift is this park;
- the owner census is clean;
- the fake-match scan reports 0 leads.

## Proposals

1. **Header fix** in `source/math/real_math.h`, `projection_from_vector3d` (lines 974-981). Replace the current body with:
   ```c
    if (k >= j && k >= i)
    {
        return _z;
    }
    else if (j >= i)
    {
        return _y;
    }
    else
    {
        return _x;
    }
   ```
   It currently reads `if (k < j || k < i) { return j < i; } else { return _z; }`.
   - **Consumers:** 188 files name `real_math.h` directly; all 446 C build units include it transitively. The direct callers are `decals.c` (own copy), `real_math.c`, `collision_features.c`, `leaf_map.c` and `structures.c`.
   - **Measured effect:** +176 strict bytes, 0 exact losses, and the four park drifts listed above.
   - **Paired source change:** land b10's `leaf_map_get_leaf_bounds` tail in the same packet for a further +784.
2. **Owner ruling on the `_point_from_line3d` COMDAT:** `leaf_map_leaf_spans_polygon` is 656 bytes exact with the inline call (s4), once proposal 1 lands.
3. **Owner ruling on layout-identical representation casts** where January proves the address is passed directly: `intersect_planes3d`, 368 bytes (p1), once proposal 1 lands.
4. **Park re-baseline** of `_leaf_map_build_portals_from_leaf` from unclassified to csplit-relocation-alias, using the f7 body. It receives zero strict credit.

## Reopen criteria

- **`leaf_face_get_vertex3d`, `get_leaf_bounds`:** the header correction is admitted.
- **`leaf_map_leaf_spans_polygon`:** the header correction lands and the `_point_from_line3d` policy allows it, or the build stops emitting surplus COMDATs.
- **`intersect_planes3d`:** the header correction lands and the owner admits the cast.
- **`build_portals_from_leaf` and the four alias parks:** the csplit/`symbols.json` boundary naming is fixed.
