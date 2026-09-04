# `object_lights.obj` interrupted-Opus reconciliation (2026-09-03)

## Scope and provenance

This packet reconciles only the interrupted `object_lights.obj` work from the
read-only donor worktree
`C:\halo-worktrees\opus-small-families-30k-20260902` (committed donor tip
`eeedd72e5ba0fc1761519f68cad241fe606320e7`, plus its dirty
`source/objects/object_lights.c`) onto an isolated worktree based at canonical
commit `545c2bdf2e88b17afd9b23db5ba6eed6a405e095`.  The donor was never modified,
and this packet was not rebased onto or written into the advancing canonical
checkout.

The clean-base focused gate was **19 exact / 6 residual / 18 unwritten**.  The
final focused gate is **35 exact / 5 residual / 3 unwritten**.  A full rebuild
of a separate clean worktree at the exact base commit supplied the authoritative
rename-stable before map: 8,245 target functions, 5,768 exact.  The final
candidate map has 5,784 exact, for **16 exact-function gains and 5,136 padded
code bytes with zero regressions**.

## Admitted strict gains

| January section | Retained semantic name | Padded bytes |
| --- | --- | ---: |
| 2 | `_shade_scalar` | 32 |
| 3 | `_shade_vector2d` | 64 |
| 4 | `_shade_vector3d` | 96 |
| 5 | `_sample_lightmap` | 256 |
| 6 | `_sample_diffuse_texture` | 240 |
| 7 | `_should_render_lights` | 32 |
| 14 | `_light_build_cluster_array` | 96 |
| 15 | `_object_get_self_illumination` | 208 |
| 16 | `_light_particle` | 608 |
| 20 | `_brighten_real_rgb_color` | 160 |
| 31 | `_light_compute_bounding_sphere` | 240 |
| 33 | `_lights_render_diffuse` | 464 |
| 34 | `_lights_render_specular` | 336 |
| 35 | `_lights_illumination_at_point` | 720 |
| 37 | `_lights_distant_lighting_at_point` | 944 |
| 38 | `_light_reconnect_to_map` | 640 |
| **Total** | **16 functions** | **5,136** |

The three recovered names replacing address labels are not inventions.
`shade_scalar.c`, `shade_vector2d.c`, and `should_render_lights.c` in the later
HCEA source independently supply `_shade_scalar`, `_shade_vector2d`, and
`_should_render_lights`.  January COFF storage classes identify the two shade
helpers as private (class 3) and `should_render_lights` as public (class 2), so
`config/symbols.json` records that exact ownership.  Related HCEA owner files
also corroborate the retained names and behavior for light particles, bounding
spheres, diffuse/specular rendering, point illumination, distant lighting,
static preparation, texture/lightmap sampling, and map reconnection.

`light_reconnect_to_map` becomes exact only when its authentic
`light_definition_get` validation lookup is present.  January has 25
relocations and a `_tag_get` call at that position; omitting the lookup yields
only 24 relocations.  Although the returned pointer is not used later, the
lookup itself is observable validation behavior and is retained as source, not
as an optimizer carrier.

## Honest fuzzy parks and remaining unwritten work

Five complete, coherent C bodies remain parked rather than being forced:

| Function | Candidate / target bytes | Relocations | Similarity |
| --- | ---: | ---: | ---: |
| `_light_unmarked` | 80 / 96 | 8 / 8 | 77.40741% |
| `_light_mark` | 96 / 96 | 8 / 8 | 84.84849% |
| `_find_point_lights_for_object_in_cluster` | 560 / 560 | 31 / 31 | 99.3617% |
| `_build_distant_lights` | 1008 / 1024 | 78 / 78 | 96.383835% |
| `_lights_prepare_for_object_static` | 832 / 832 | 17 / 17 | 97.64286% |

The parked ledger pins target/candidate normalized hashes and the evidence
boundary for each body.  In particular, `_light_unmarked` retains the semantic
HCEA predicate `lights_globals.marker != light->marker`; the known artificial
if/return respelling is rejected even though it can shape bytes more closely.
The other gaps are ordinary VC7 branch, register, or x87 scheduling, so no
volatile state, fake dependency, pragma, forced inline, inline assembly, or
target-shaped nonsense is admitted.

Three functions remain unwritten until their real call topology can be restored:

- `_lights_preprocess_scene` (2,400 padded target bytes);
- private `_render_debug_light` (272 bytes);
- private `_light_get_bounding_sphere` (224 bytes).

The last two were reported as exact by the interrupted donor only because they
were changed to external linkage solely to force emission.  January COFF and
`config/symbols.json` both identify them as private.  This reconciliation
restores `static`, rejects those two false gains, and leaves their HCEA-backed
bodies as honest groundwork for the unwritten real caller.

## Header-position and ownership reconciliation

The first donor replay placed recovered declarations into seven broad owner
headers.  A full sweep caught definition-position regressions in unrelated
consumers, including `bitmap_utilities.obj`, `decals.obj`, rasterizer water,
and the protected `units.obj` function `_unit_preprocess_node_orientations`.
This is the known VC7/C2 definition-position sensitivity, not a semantic type
dependency.

Every broad header was restored byte-for-byte.  The required owner declarations
now live in narrow subsystem interfaces:

- `source/bitmaps/bitmap_group_lookup.h`;
- `source/bitmaps/bitmaps_sampling.h`;
- `source/objects/object_lights_rendering.h`;
- `source/rasterizer/rasterizer_environment.h`;
- `source/rasterizer/rasterizer_geometry_environment.h`;
- `source/structures/structure_render_lights.h`;
- `source/structures/structure_vector_tests.h`.

`render.c` and `render_objects.c` consume the object-light owner interface
instead of redeclaring foreign functions/globals locally.  Typed light, light
definition, lens-flare definition, shader-environment, object-definition, and
object-datum access is expressed through owner macros.  Source uses campaign
`real`, word, enum, and flag abstractions, explicit returns, one parameter per
line, and the recovered private/public linkage.  The retained static-light
source preserves January's `scale_vector3d` inline schedule, and the final COFF
contains no `point_from_line3d` symbol or COMDAT.

## Validation

- focused `gate.py --all`: 35 exact / 5 residual / 3 unwritten;
- clean-base versus final rename-stable whole-tree sweep: 16 gains, 5,136
  padded bytes, zero regressions across 8,245 target functions;
- protected `units.obj` exacts restored after narrow-header reconciliation;
- private full `ninja all_source progress semantic_progress`: pass;
- semantic report: 6,120 evaluated, 5,793 semantic exact, 158 hidden exact,
  5,810 accepted, zero unit errors;
- progress report: 5,745 / 11,060 code functions and 813,630 / 2,198,102 code
  bytes accepted on this isolated base; 181 parked entries validated;
- target/candidate COFF ownership check: `_shade_scalar` and
  `_shade_vector2d` class 3, `_should_render_lights` class 2; rejected private
  helpers are not spuriously emitted; no `point_from_line3d` symbol;
- `python tools/fake_match_scan.py --fail-on-findings` over all ten changed C
  and header files: zero review leads;
- `python -m pytest -q -p no:cacheprovider`: 261 passed;
- `config/parked.json` and `config/symbols.json`: valid JSON;
- `git diff --check`: pass.

## Current canonical integration audit

The packet was replayed after the rasterizer-core, structure-render, and
two-leaf encounters integrations.  The newer canonical
`structure_bsp_definitions.h` already owns the exact six-byte
`struct structure_surface`, so the now-redundant translation-unit copy was
removed.  The two light-stage declarations were merged into canonical's
existing narrow `rasterizer_environment.h` owner interface.  No broad header
or semantic source workaround was introduced.

Against `scratch/after-encounters-leaves-20260903.json`, the combined canonical
rebuild independently confirms the same 16 gains and 5,136 padded bytes with
zero regressions across all 8,245 functions.  The focused result remains
35 exact / 5 residual / 3 unwritten.  The full build reports 5,861 semantic
exact and 5,878 accepted functions with zero unit errors; progress reports
829,961 / 2,198,102 meaningful code bytes and 5,810 / 11,060 code functions.
The merged manifest validates 186 active parks with zero stale or invalid
entries, admission remains 0 candidates / 0 contradicted, the ten-file
fake-match scan has zero review leads, and the tooling suite passes 261/261.
The protected bitmap working files retain their pre-integration SHA-256 hashes.

The packet is deliberately not a claim that `object_lights.obj` is complete.
It is the largest admissible, house-rule-compliant subset recovered from the
interrupted donor, with the five honest fuzzy bodies parked and the three real
unwritten functions explicitly exposed for a later source-authenticated wave.
