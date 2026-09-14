# Geometry: Opus5 150K house-clean lane, wave w2 (2026-09-14)

## Scope and provenance

- Translation unit `source/math/geometry.c`; January target `build/split/source/math/geometry.obj`.
- XDK 3911 VC7 13.00.9254 with the repository flags, unchanged. Every measurement is a whole-TU compile through
  `tools/campaign/gate.py --source` on a scratch copy.
- No header, config, ninja, configure or pytest was used.
- A csplit verification of one proposed `symbols.json` label ran against a scratch copy of `config/`, with output in scratch only. It is described below.
- Worker notes, with every shape and measurement: `scratch/workers/geometry.md`. Candidates: `scratch/workers/geometry/`.
- Evidence used:
  - the January split object: bytes, relocations, frame-slot census and assert anchors;
  - HCEA `halocea-review/src/blam/{networking/build_torus.c, math/get_edge_vertex.c, math/geosphere_new.c, structures/convex_hull3d_begin.c, math/convex_hull3d_expand.c}` (360 build: names and topology only);
  - Bungie `random_math.c:183`, for the point-to-vector cast idiom already admitted in `geometry.c` and `random_math.c`;
  - the prior Geometry ledgers and the 100K wave D verifier verdicts (`opus5-100k-consolidated-20260914/scratch/waveD-verify-results.json`).

## Baseline and final real-file state

Baseline and final real-file gates are identical:

- `== exact 21  residual 9  unwritten 0  (of 30 listed)`
- the `_point_from_line3d` guard passes
- all 9 residuals are parked

The real file is unchanged: `git diff --stat -- source/math/geometry.c` is empty, and all changes are park reopens, which workers may not land.

Final checks: park drift 0/9, owner census vs `build/base` clean, fake scan 0 leads. **No newly exact rows in the real file.**

## Park-reopen proposals

### Primary candidate

`scratch/workers/geometry/primary_reopen.c` (sha256 `faf3f4f3...a95a5`) is the current real file plus three reopened bodies.

- Gate: 23 exact / 7 residual. All 21 baseline exact rows are unchanged.
- The guard passes. Only the three reopened parks drift.
- The owner census vs `build/base` is identical, with no new code or data owner. Fake scan 0.
- With the symbols.json label below, `tools/coff_compare.py` against a re-split target shows 24 equal functions.

| Function | Target size / relocs / sha16 | Old park base | Result |
| --- | --- | --- | --- |
| `_build_torus` | 768 / 18 / `6460f3c33d8f750a` | 752 / 18 | STRICT EXACT |
| `_convex_hull3d_begin` | 1488 / 28 / `d3e0ba2e2077ce1e` | 1472 / 28 | STRICT EXACT |
| `_geosphere_new` | 496 / 25 / `89b49ad03b434d62` | 496 / 25 | code identical; exact once the label is added |

#### `_build_torus`

The 100K wave D form was rejected only by the owner census: `add_vectors3d` emitted a new `_add_vectors3d` COMDAT. The new body keeps that verified structure and replaces the helper with a componentwise translation: `points->x += radial.i; points->y += radial.j; points->z += radial.k;`.

What the structure consists of:
- the per-ring `real_point3d cylinder_point`;
- the in-place `rotate_vector_about_axis((real_vector3d *)points, ...)`;
- short counters (HCEA `int16_t`);
- `cosine`/`sine` evaluated at their use sites.

January proves the componentwise add after the call. It emits `fld radial.i; fadd [esi]` for x and y, and an integer self-copy of z (`radial.k` folds to 0).

Two alternatives fail:
- `set_real_point3d(points, radial.i + points->x, ...)` moves z through the x87 and is not exact.
- The componentwise form also avoids a second point-to-vector cast.

The park's "typed rotated_point local" premise is refuted by January's 0x4c frame.

#### `_convex_hull3d_begin`

The January frame census supplies the evidence the park asked for, "a local that owns the 12-byte hole":
- In the line-distance loop, `offset.k` is homed at `[ebp-0x34]`.
- `[ebp-0x3c]` and `[ebp-0x38]` are never referenced, so `offset.i` and `offset.j` were scalarised.
- The subtraction result's i component goes to a different home at `[ebp-0x48]`.
- So January subtracts into a third vector: `subtract_vectors3d(&offset, &projection, &perpendicular)`, then `magnitude_squared3d(&perpendicular)`.

Two further natural forms complete the closure:
- **Plane loop.** It is the standard inline `plane3d_distance_to_point(&plane, points + point_index)`. January loads the plane member first and walks x, y, z. The `_plane3d_distance_to_point` COMDAT already exists in the base object.
- **Topology.** The assignments are written per element in struct declaration order. Within each constant group VC7 emits, January's store addresses ascend. The old order was HCEA's decompiler (360 compiler) order.

Shapes k1, k2 and k3 close the function in three steps.

#### `_geosphere_new`

January zeroes `edx` before the NONE fill, keeps the vertex cursor as a live short IV while the source pointer is strength-reduced, and homes it after the loop. That is `short vertex_index = 0;` plus a separate loop index, with `result->vertices[vertex_index++] = geosphere_primitive_vertices[index];`. HCEA uses the same cursor, which is then passed to `subdivide_triangle`.

After that change the instruction stream is identical (alndiff shows 0 differences). The only difference left is relocation naming: January references `.rdata:_geosphere_primitive_vertices+74` because `config/symbols.json` has no interior label at the triangle table.

### Alternative: `_get_edge_vertex`

Candidate: `scratch/workers/geometry/edge_vertex_reopen.c`. It gives 416 / 17 / `ebafcbd056870e28`, strict EXACT. With the hull3d_begin and geosphere_new reopens it is still exact (`gev_h3b.c`).

Evidence:
- **Single-exit law.** Every January exit shares one epilogue: `return v2` falls through with v2 already in eax.
- **Int-width offset.** January's `movsx; sub ecx,edx` is a full-width subtraction, so `offset` is a `long`. A short offset narrows it to `sub di,dx` (measured). HCEA's binary note agrees.
- **Short result.** A `short vertex` result takes the truncated sum.

Body: `vertex = *edge_vertex_index + offset - 1;` with `offset = reversed ? segment_count - subdivision_index : subdivision_index;`, and one `return vertex;`.

**This reopen conflicts with the torus reopen.** With the torus body in place, `_get_edge_vertex` differs only by two transposed, dependency-free parameter reloads after its loop.
- Bisection: the torus short-counter change alone flips it. Unused-local diagnostics do not restore it, so this is not a declaration-count effect.
- It is a TU-context scheduling tie in a function that follows six still-residual bodies in file order.
- The two forms tried, g6 (if/else) and g7 (HCEA `edge_index` local), are not exact in either context.

The primary candidate therefore takes the larger torus (768 bytes) and leaves `_get_edge_vertex` parked. Its park base is unchanged by the primary candidate.

## Orchestrator proposal (config)

`config/symbols.json`: insert one line in place, directly after the `_geosphere_primitive_vertices` line (line 17617, CRLF):

```
{ "file_offset": 2597632, "flags": 0, "name": "_geosphere_primitive_triangles", "static": true },
```

Why the label is correct:
- 2597632 = 2597560 + 72 = 6 × sizeof(real_point3d).
- Target bytes from +72 are the 8×3 short triangle table.
- January code references +74, which is `triangles[0][1]`.
- The table is TU-private static data, and the name matches the source and HCEA.

Verification:
- `build/tools/csplit.exe` was run with `-p scratch/workers/geometry/splitcfg/config` for `source\math\geometry.obj`.
- The control run with the real `config/` reproduces `build/split/source/math/geometry.obj` byte for byte.
- With the label, the `.rdata` section has the same two owners as our object.
- `_geosphere_new` becomes equal; the base object's 21 equal functions are unchanged.

Consumers: geometry.obj split only; this is a csplit-only regen. No source or header consumer is affected.

## Not landed

| Function | Class | Reason / reopen criterion |
| --- | --- | --- |
| `_convex_hull3d_expand` | owner-blocked lead | x1 uses the `plane3d_distance_to_point` helper in both surface loops (not exact). A scratch-only diagnostic declared and initialised the surface pointers before index validation; January computes `imul 0x1c; add` before its range tests. That made the first ~195 instructions identical but still 1824 vs 1808. It is the pointer-before-check form the 20260908 root rejected as abstract-C UB. Reopen only after an owner ruling on that form, then with evidence for the vertex-fan and horizon schedule. |
| `_convex_polygon3d_clip_to_plane` | x87 schedule | Mismatches: in-loop plane-distance term schedule (z,x,y with mixed operand loads), PIN join `fld st(0)`, fsubr in the duplicate test. A `point_from_line3d` diagnostic is identical to base and trips the guard. No new law applies. |
| `_convex_polygon2d_clip_to_plane` | register / x87 schedule | Mismatches: plane pointer kept in ecx, `previous_point` homed, `in_front` memory-only, materialised TEST_FLAG value, PIN join duplicate. The wave D shapes were already spent. |
| `_convex_hull2d` | zero-register pin | Already single-exit. January returns `[ebp-8]` on the non-2D path, while ours pins zero in esi. No source lever. |
| `_convex_hull2d_perimeter` | exhausted | The history blob `b38145bf4f` holds the already-rejected hand-inlined `j*j + i*i` loop. The helper forms per_s1 and per_s2 give the base schedule. Reopen only with a genuine j-first helper or source. |

## Rejected shapes (do not repeat)

**Torus:**
- t0 (`add_vectors3d` census veto)
- t1 (`set_real_point3d` sums)
- tor_A / tor_C (short counters only, or ring cosine locals)

**get_edge_vertex:**
- g1: short offset
- g2: reassigning `subdivision_index`
- g3: single exit with two assignments
- g6: if/else offset in the torus context
- g7: HCEA `edge_index` local

**geosphere_new:** none; n1 and n2 are both code-identical.

**hull3d_begin:** none. k1 and k2 are intermediate steps.

**expand:** the x2diag UB form is diagnostic only.
