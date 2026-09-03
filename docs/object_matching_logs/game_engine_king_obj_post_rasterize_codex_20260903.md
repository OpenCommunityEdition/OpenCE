# `game_engine_king.obj` post-rasterize reconstruction (2026-09-03)

## Result

This lane reconstructs the sole unwritten function in
`source/game/game_engine_king`, `_king_engine_post_rasterize`, as complete,
ordinary typed C. The focused gate moves the unit from `26` exact, `2`
residual, and `1` unwritten function to `26` exact, `3` residual, and `0`
unwritten functions. All 26 inherited exact functions remain exact.

This is deliberately not an exact-byte gain claim. The January function is
`916 / 928` meaningful / padded bytes with 23 relocations. The retained
candidate is 896 padded bytes with 22 relocations and reaches `88.0717%`
objdiff similarity. It is therefore recorded as an honest `unclassified`
fuzzy park rather than described as complete at the object-code level.

## Reconstructed behavior

The function obtains the King hill shader from the first typed multiplayer
information block in the scenario game globals. A first closed-polygon pass
sums the 3D length of every hill edge. From that perimeter it derives the
rounded segment count and the texture-repeat scale found in January. A second
pass builds a four-vertex vertical quad for each edge, extending 0.8 world
units above the hill outline. It calculates the face normal with
`vector_from_points3d`, `cross_product3d`, and `normalize3d`, advances the
horizontal texture coordinate by physical edge length, and submits each face
through `render_dynamic_quad` with the scenario-owned shader.

The retained source uses `real`, `real_point3d`, `real_vector3d`, and
`model_vertex_uncompressed`; scenario tag-block access goes through
`TAG_BLOCK_GET_ELEMENT`. The public declaration remains in the existing
owner header, `game/game_engine_king.h`. Every parameter is on its own line,
the no-argument list carries `void` on its own line, and the function has an
explicit terminal `return;`. The function introduces no raw address name,
manual object/tag cast, numeric enum surrogate, forced inline/noinline,
volatile or register forcing, barrier, pragma, assembly, fake dependency, or
`point_from_line3d` emission.

## Evidence and bounded experiments

The January split object is the byte and ownership authority. Target
disassembly establishes both passes, the 0.8/0.2/1.0 constants, the normal
calculation, and the dynamic-quad call. The independent later-Xbox HaloCEA
reconstruction corroborates the behavior and name, pinned at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`, path
`src/blam/game/king_engine_post_rasterize.c`, blob
`5ee668b4803d3665b097cac021628e63092c97b2`. Its raw decompiler-shaped
zero-difference algebra was not copied; the retained expressions state the
actual perimeter and texture-coordinate math directly.

Alignment-aware comparison shows that the remaining difference is broad
register and x87 scheduling, not missing game logic. January retains the hill
point count in EBX, current point in ESI, and next index in EDI, then
recomputes the next point after `csmemset`. C2's natural lowering spills the
count and preserves the next point across that call, which also removes one
global relocation. The fail-closed classifier returns `UNKNOWN`.

Bounded natural-source tests covered pointer and indexed loop forms, scoped
and shared loop locals, several declaration orders, direct and temporary
next-point expressions, target-like vertex assignment orders, and equivalent
accumulated-distance expressions. None recovered the January allocation and
x87 schedule. One declaration move grew the candidate only by retaining a
useless zero store, so it was rejected as fake progress. Natural source
levers are exhausted for this pass; the residual should be reopened only for
authoritative January source/local records or a natural compatible-compiler
donor.

## Measurements

| Metric | Before | After |
| --- | ---: | ---: |
| Exact functions | 26 / 29 | 26 / 29 |
| Residual functions | 2 | 3 |
| Unwritten functions | 1 | 0 |
| `_king_engine_post_rasterize` padded bytes | absent / 928 | 896 / 928 |
| `_king_engine_post_rasterize` relocations | absent / 23 | 22 / 23 |
| `_king_engine_post_rasterize` objdiff | n/a | 88.0717% |
| Unit strict matched meaningful bytes | 2658 / 4865 | 2658 / 4865 |
| Unit objdiff `.text` similarity | 79.12534% | 95.70781% |

Normalized hashes for the function are
`a08e67ac8eb6f17666d65a2fcc7add06e4cf523448a667875e0f5beffcfc1b71`
for January and
`9f13a4c24615b66a6ac337d302424c38c17f0f73e9623e172a81da359c4b6bd5`
for the retained candidate.

## Verification

- focused campaign gate: `26` exact, `3` residual, `0` unwritten;
- all 26 inherited exact King functions retained;
- translation-unit compile: pass, with only the pre-existing authenticated
  C4700 warning in `find_next_hill`;
- fail-closed blocker classifier: `UNKNOWN`;
- parked-function manifest validation: pass;
- fake-match scan over the King source/header: zero findings;
- Python verification suite: pass.

The integrator should still run the canonical full build and address-keyed
full exact-set regression sweep after cherry-picking this isolated commit.
