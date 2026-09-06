# Fable Geometry geosphere integrated-runtime ledger (2026-09-06)

## Verdict

The typed natural five-function geosphere graph is runtime-equivalent to the frozen January target for every modeled case. The independently compiled first object and frozen Ninja actual object both produce the same exact canonical trace SHA-256 as January:

`7657a6f1bf45b06b69dc05815c1ba22377c9eb70243f08243b3763f580ad62a0`

- January target: 20/20 semantic cases pass.
- Typed first candidate: 0 semantic failures, 0 exact target mismatches.
- Frozen actual: 0 semantic failures, 0 exact target mismatches.
- Exact heap, vertex, strip, normalize-input, public-call, return/stack, nonvolatile-register, direction-flag, caller-frame, and x87 control/TOP checks all pass.
- No numeric tolerance or fallback was used. The diagnostic ULP/absolute-delta path remained unused.
- Twelve synthetic captured-outcome/hash negative controls all detect their mutations. They do not mutate input binaries.

This is semantic evidence for residual functions, not a claim that their COFF sections became exact.

## Runtime boundary

Only the genuine public `_geosphere_new(short)` entry is invoked. The object-local relocation graph executes all four private functions from the same respective object:

1. `_subdivide_triangle`
2. `_get_face_vertex`
3. `_get_edge_vertex`
4. `_calculate_vertex`

No private function is exposed through an invented callable ABI. `_calculate_vertex` calls and executes the real `_normalize3d` COFF body. The other authenticated emitted math COMDATs (`_square_root`, `_scale_vector3d`, `_magnitude_squared3d`, `_magnitude3d`) are materialized but correctly receive zero calls from this graph.

The only modeled external APIs are `_debug_malloc`, `_debug_free`, `_display_assert`, and noreturn `_system_exit`, with their real cdecl argument order. All observed allocator clear arguments are zero; the tool explicitly rejects a nonzero clear request instead of silently treating it as an ordinary uninitialized allocation.

## Frozen inputs

| Input | SHA-256 |
|---|---|
| January target `scratch/geometry-fable-geosphere-before-20260906/files/build/split/source/math/geometry.obj` | `9a223d12be60d64dfed8930aeb8366a33b92c82a693a341b7e10beef0682740c` |
| Typed first `scratch/geometry-fable-geosphere-typed-first-20260906.obj` | `f4815540d55abc982dadb422631c2c20df11df10dab0f357150d98fbd455b55d` |
| Frozen actual `scratch/geometry-fable-geosphere-final-20260906/source/math/geometry.obj` | `9370a04cd686f339a687f408df0452774bcc5843d7b5b577f3f4a875502be606` |
| Frozen typed source | `f17d26efb18b796ceef7f9cf58d4241561be612d85770be28bfadeb38d3be45f` |
| Frozen owner header | `04f7b7b7d027c0621d93410abd856c9cb42b6268164ad8db4a78647d91c5411f` |
| Frozen symbols config | `c2224cc0088411edbe1501013e7f4536c2e0c45db7bbe5a545f8e137e740ed5a` |
| Before manifest | `d89de4c54a6ca0471b68e1e44debb3f2106cc942ac4d75be62f44ff6ecc8aecb` |
| Final manifest | `8b22a33fbc3f51a78f44dc7b63de02aa0f3e0568df74dfd36236664928d7c5fa` |
| January ordinary `_normalize3d` owner `build/split/source/ai/action_charge.obj` | `76c4c0b62850ffaa42fe5027c3e68b2cc42f7640c1156342c9a9cc4d1125afea` |
| January ordinary `_magnitude_squared3d` owner `build/split/source/ai/action_alert.obj` | `351ff0de9ddfea9b3c03a9038abab75b0c4b48290443b33e09c33105b41ab61a` |
| January production caller `build/split/source/math/random_math.obj` | `1ce32f14ed504b4e14cf80f712f5e9cf3810425c72982da9d5cd5eb744395d57` |

The frozen source guard proves the typed `real_vector3d vertex` local, semantic `set_real_vector3d`, real `normalize3d(&vertex)`, semantic `set_real_point3d` copyout, and absence of point/vector representation-pointer casts in the function body.

## Section identity and graph closure

| Function | January bytes/relocations | Typed bytes/relocations | Typed normalized SHA-256 |
|---|---:|---:|---|
| `_geosphere_new` | 496 / 25 | 496 / 25 | `56760ccae276f6ff7f6f188448a6f19fbffa288c993cd71d4dd9cad5c9b1c978` |
| `_subdivide_triangle` | 944 / 52 | 944 / 52 | `0ea8faff29e129b55a134432a0e602b69d5f7ad1beeee4b15f02a5e1403599ed` |
| `_get_face_vertex` | 400 / 18 | 400 / 18 | `10460c1e30f697342132688a0b347c3cf155248e73da04199d5d482d22c26c63` |
| `_get_edge_vertex` | 416 / 17 | 400 / 17 | `240b22a49f13a88b486b99416a7f162513a3c76a9e4de22ee5445d83f3cc7ded` |
| `_calculate_vertex` | 336 / 18 | 336 / 18 | `26b9afec42c4b82dbb45631840c95109ba1e2ee7bcbe5597afc9d45570e89783` |

Every graph-call relocation was checked by name and count. The target obtains its real exact `_normalize3d` implementation from the pinned ordinary January owner; the typed candidate/actual execute the identical helper emitted in their own object. The helper is 96 bytes, 3 relocations, normalized SHA-256 `67fbd33f204d0c93cce135fcfd37d4ce5b443642edc6ad99230846b91c893e5b`.

## Cases and observed original behavior

Each of these ten cases runs under explicit x87 control words `0x027f` and `0x037f` (20 total). Both values are disclosed test modes; the oracle does **not** claim either is the game's established ambient default. The control word is written explicitly and must be preserved; the FPSW TOP field must return to zero at the terminal boundary. TOP balance alone is not claimed to prove an empty physical x87 register file.

- Valid segment counts 2 and 3.
- Production segment count 16. Frozen January `_random_math_initialize` has one `_geosphere_new` REL32 and its instruction stream pushes immediate `0x10` before that call.
- Outer allocation failure.
- Each child failure: vertices, triangle-strip indices, and subdivision table.
- First per-face allocation failure at segment 2 and segment 3.
- Segment-1 original capacity assertion.

The segment-16 result has 1,026 vertices, 2,048 triangles, 128 strips, and 1,020 real `_normalize3d` calls. Exact hashes are:

- heap: `44043d89b5e805f41a73c01728dc6a3fa1ec5a9eccdbebdf45f67bfa9b430726`
- vertices: `e7ab8ac99dba99ca4dcb351bee1fe931d041dfc86c374b445b59b828d95f67f1`
- used strip stream: `3f1e50bd61936bbbdaa18bee45ab17d22237d43ae27f00299047320a8c71e900`
- normalize inputs: `1d1a310e05d1d7e1834892a02287b28b03b35f4471a0dc10ba6da305b837e584`

The failure cases deliberately preserve January's defects rather than papering them over:

- Child allocation failure returns the non-null outer allocation after freeing any successful child allocations; the corresponding non-null child fields are not cleared and therefore dangle.
- First-face allocation failure at segment 2 silently returns an incomplete sphere with 14 strips (rather than 16) because all 18 shared-edge vertices are eventually generated by the other seven faces.
- First-face allocation failure at segment 3 exits through the original line-99 `vertex_index == result->vertex_count` assertion after producing 21 strips and 31 calculated vertices.
- Segment 1 reaches the original line-98 strict capacity assertion after 8 strips because the used-short index equals, rather than remains below, the 32-short capacity.

These observations are not safety guarantees or endorsed contracts.

## Deliverables and verification

- Public oracle: `tools/audit/geometry_geosphere_runtime_differential.py`
- Public fixture-free tests: `tools/test_geometry_geosphere_runtime_differential.py`
- Preserved scratch oracle: `scratch/geometry_fable_geosphere_runtime_differential_20260906.py`
- Focused pytest: **68 passed**.
- Public oracle replay: PASS, 20 cases, target/candidate/actual trace hashes identical, zero semantic failures, zero exact mismatches.

No production C/header/config/build input was edited or compiled by this audit.
