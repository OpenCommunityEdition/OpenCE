# `path_obstacles.obj`: remaining unwritten packet (2026-09-12)

## Outcome

This packet reconstructs the two previously unwritten January functions in
`source/ai/path_obstacles.c`. Both are strict exact matches, adding **1,228
meaningful / 1,248 padded code bytes** with no loss among the eleven inherited
exact owners. The isolated whole-TU gate advances from 11 exact / 3 residual /
2 unwritten to 13 exact / 3 residual / 0 unwritten.

| Function | Meaningful | Padded | Relocs | Normalized SHA-256 | Result |
| --- | ---: | ---: | ---: | --- | --- |
| `_obstacles_get_discs_in_sphere` | 792 | 800 | 25 | `b55841a0cc894ffd5b0cf4433f9ba4584cfdf78da40ab929fbbc7f8bf31bb33c` | strict exact |
| `_render_debug_obstacles` | 436 | 448 | 20 | `af8ec56d35ffb5aab41744673824d2b83acabfc36e8516fcf929e9acf7df49e0` | strict exact |

## Reconstruction evidence

January's split object is authoritative for code, relocations, constants,
assertion text and source lines. Stian's reconstruction in
`research-cache/stian-halo-full-history-20260828/src/halo/structures/structures.c`
and its debug-render map in `src/halo/ai/path_smoothing.c` supplied an
independent control-flow map. The HCEA reconstruction at
`research-cache/halocea-full-blobs-20260830/src/blam/objects/obstacles_get_discs_in_sphere.c`
provided later-build semantic corroboration only.

The June 2011 HCEA PDB independently confirms the public
`obstacles_get_discs_in_sphere` signature and the original local names
`object_indices`, `world_matrix`, and `transformed_center`. The recovered body
uses the project's typed object, device, tag-block, object-definition,
collision-model, matrix, and collision APIs. The dynamic machine-to-device
view goes through the local `path_obstacles_device_datum_from_object` accessor
macro, following the existing `*_datum_from_object` convention, rather than
leaving an implementation cast at the use site.

The source preserves the actual behavior visible in January: candidate-object
filtering, machine open-state handling, object and marker sphere tests, node or
world transforms, directional vertical rejection, weighted vertical distance,
and optional moving-biped discs. Debug rendering constructs its point and
downward vector with the real-math helpers, raycasts against front-facing
structure surfaces, and renders either projected circles or fallback spheres.

## Color-table ownership

January contains an external 2,048-byte `.data` owner at `_data_002b7578`.
The reconstructed `real_argb_color global_obstacle_colors[MAXIMUM_DISC_COUNT]`
has identical size, zero relocations, and byte-for-byte identical contents:

`427b5b578a7c799308e129048e8b95e869ca33d6eb3adabd18adad9e61558948`

The descriptive identifier `global_obstacle_colors` is a semantic
project-convention inference, not a claim of recovered PDB spelling. The
isolated intake gate initially used
`--alias global_obstacle_colors=data_002b7578`. The root integration then
renamed the target owner in `config/symbols.json`; the ordinary canonical gate
now consumes the semantic source spelling directly.

## Verification

One PID-safe grouped gate compiled the complete translation unit:

```text
python tools/campaign/gate.py source/ai/path_obstacles --all \
  --forbid-emitted-symbol _point_from_line3d \
  --out scratch/path_obstacles_packet_final.obj
```

It reports 13 exact / 3 residual / 0 unwritten and passes the forbidden-COMDAT
guard. The only residual functions are the same three inherited owners:
`circle_tangents`, `obstacles_disc_neighborhood`, and
`obstacles_test_circle`. `tools/fake_match_scan.py` reports zero review leads,
and `git diff --check` passes apart from the checkout's line-ending notice.
The root integration subsequently passed the full Ninja build and stable
repository sweep with zero regressions; aggregate publication evidence is in
the sixth 10K batch ledger.
