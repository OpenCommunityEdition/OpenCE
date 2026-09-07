# Frame statistics: one January counter owner across twelve consumers

Baseline: `883e7c06211e758d5b1683706567d3787bbefabb`.

## Scope

Replace twelve incompatible local declarations of the same
`rasterizer_frame_statistics` global with a complete, shared
`struct rasterizer_frame_statistics_globals` in its genuine associated
`source/rasterizer/rasterizer_frame_statistics.h`. The header declares the
external object; this packet does **not** invent a storage definition or move
linker-common ownership into an arbitrary C file.

The named fields are the counters already used by reconstructed writers.
Unreviewed interiors remain explicit reserved byte spans inside the complete
record. No FPS/draw body, new helper, initializer, compiler option, symbol-map
entry or completion label is added. This is a prerequisite to the pending
Opus rasterizer reconstruction, with **zero new exact-byte credit**.

FrameStatistics remains six strict functions, two residuals and two unwritten
functions. Its 4,176-byte draw is not admitted by this owner migration; the
[prior draw-source hold](rasterizer_frame_statistics_obj_opus_draw_source_admission_hold_20260906.md)
still applies to the donor's source-order experiments and artificial predecessor
dependency. A future natural fuzzy implementation must use the real owners.

## January storage and consumer evidence

A read-only census of all **833 January target objects** finds this symbol
in thirteen COFF symbol tables. Twelve are undefined external references;
the sole defined owner is `build/split/source/linker_common.obj`, section 58,
`.bss`, **368 bytes / 0x170**, external storage class 2. The later HCEA
28-byte FPS-only record is not a compatible January layout.

The affected implementation files are:

- `source/rasterizer/rasterizer_frame_statistics.c`
- `source/rasterizer/rasterizer_debug.c`
- `source/rasterizer/rasterizer_lights.c`
- `source/rasterizer/xbox/rasterizer_xbox.c`
- `source/rasterizer/xbox/rasterizer_xbox_decals.c`
- `source/rasterizer/xbox/rasterizer_xbox_draw_primitives.c`
- `source/rasterizer/xbox/rasterizer_xbox_dynavobgeom.c`
- `source/rasterizer/xbox/rasterizer_xbox_environment.c`
- `source/rasterizer/xbox/rasterizer_xbox_environment_fog.c`
- `source/rasterizer/xbox/rasterizer_xbox_models.c`
- `source/rasterizer/xbox/rasterizer_xbox_shadows.c`
- `source/rasterizer/xbox/rasterizer_xbox_vertex_shaders_runtime.c`

Each old definition's named fields/offsets are preserved. Six files already
included the owning header; the remaining six now include it in their normal
header section. Existing field-offset assertions refer to the shared type.
The complete pre-edit Ninja dependency database reports exactly six valid
transitive consumers of that header: FrameStatistics, Xbox, Shadows,
Environment, EnvironmentFog and DynamicGeometry. Together with the six new
direct includers, this is the twelve-object rebuild set. No active Fable or
Opus implementation file or donor worktree is modified.

## Counter meanings and the signedness boundary

The existing Xbox provider names the `+0x150/+0x154` fields as signed-long
vertex-shader constant byte counters. Models previously exposed the same
storage as unsigned `skinning_work/lighting_work`. It now uses the provider's
names and types. Its existing local snapshots remain `unsigned long`, so
conversion and the mixed-type subtraction retain modulo-32-bit arithmetic;
its two accumulated fields remain unsigned. No signed-overflow probe or
new arithmetic expression is introduced.

| Offset | Shared name | Writer evidence / meaning |
| --- | --- | --- |
| `+0x130/+0x134` | `dynamic_vertex_count`, `dynamic_vertex_buffer_count` | A successful dynamic-vertex lock adds its count and increments the buffer count. The second field is not a maximum capacity. |
| `+0x138/+0x13C` | `dynamic_triangle_count`, `dynamic_triangle_buffer_count` | The corresponding dynamic-triangle lock performs the same two operations. |
| `+0x148/+0x14C` | `dynamic_light_count`, `lens_flare_count` | January LightSubmit and LensFlareSubmit increment these DWORD fields under statistics mode 2. |
| `+0x150/+0x154` | `vertex_shader_skinning_constant_bytes`, `vertex_shader_lighting_constant_bytes` | The actual Xbox skinning/lighting providers accumulate constant-upload byte counts. |
| `+0x160/+0x164` | `model_skinning_constant_bytes`, `model_lighting_constant_bytes` | ModelBegin accumulates the differences around those provider calls, not numbers of skinned/lit objects. |

The Opus draw draft's `dynamic_*_maximum`, `skinned_object_count` and
`lit_object_count` names therefore must not be transplanted uncritically.
January's compact display labels alone do not override its actual writers.
Likewise, the existing largest-transparent-triangle field is not authenticated
as a count of sorted triangles. These observations change the next draw
reconstruction; they do not authorize altering its target output.

Most member spellings are existing descriptive reconstruction names, not
new claims of original Bungie identifiers. This packet does not certify the
remaining local debug-options, model-parameter or private-global facades in
these files. They are separate owner-reconciliation work.

## Whole-tree verification

The first coherent shared-owner build succeeds. The complete stable sweep
retains **6,328 / 8,245 strict functions**, with zero gains and zero losses.
The frozen root audit checks all **572 rebuilt objects and 833 target objects**:

- No runtime section, padded/normalized byte, ordered relocation, symbol
  offset/type/storage, COMDAT selection, COMMON or undefined API changes.
- Five rebuilt objects differ beyond the COFF timestamp, solely in compiler
  debug metadata: Xbox (189 runtime sections), Decals (55), DrawPrimitives
  (110), DynamicGeometry (45), and VertexShadersRuntime (25).
- All other rebuilt objects and all target objects are byte-identical after
  timestamp normalization. No `point_from_line3d` owner is introduced.
- Both ordinary and semantic reports are identical as parsed JSON.

The bounded normalizations exclude CodeView records, COFF timestamps and
compiler-local serial names only. This is not a whole-engine link test or
proof of the unwritten draw/FPS algorithms.

Permanent coverage is in `tools/test_rasterizer_frame_statistics_owner.py`
and `tools/fixtures/rasterizer_frame_statistics_owner.c`. The ten focused
tests compile the actual owning header with VC7, compare every member offset
and extent against an independent fixed table, verify signedness, inspect the
sole January owner and twelve rebuilt imports, and execute the unsigned Models
delta/accumulation contract across wrap boundaries. Shifted-layout and reversed
signedness negative controls fail as intended. The warning pragma is confined
to the test fixture's type check; no production pragma is introduced.

The draft harness initially made incorrect assumptions about csplit's BSS raw
pointer, the Models function boundary, shared `.text` relocations and VC7's
warning level. These were test-harness errors, not production regressions.
The corrected root replay passes **10 / 10 focused tests** and the complete
suite passes **1,009 tests, 26 subtests, with two skips**. Final strict comparison
still reports zero gains and zero regressions. Park and admission inventories
remain unchanged: 297 active parks, zero stale/invalid; zero admission
candidates/contradictions, five inherited rejections and zero revocations.

Final review removed two orphaned comment lines left behind by the Xbox local
structure removal. A fresh build and frozen whole-tree audit after that
comment-only cleanup reproduce the same runtime and report results above.

## Frozen evidence

| Artifact | SHA-256 |
| --- | --- |
| `scratch/frame-statistics-owner-before-20260906.json` | `eb3234b26daf6bd09a16139a0fef6dc4c0f2a61b936356ec2b75f69cd59452f8` |
| `scratch/frame-statistics-owner-trial-20260906.json` | `7b68a86a01b8a10a2717e6023bab33dccfff7b8e2f5d26b75aa863f6c502251b` |
| `scratch/frame-statistics-owner-final-20260906.json` | `bc48e5669debb93a0e73024c4deddcdda473d0f741cd7103c19d44b9c12d9820` |
| `scratch/frame_statistics_owner_final_audit_20260906.py` | `dd0a6dd007f230b3cb2cd66f71a58851c883f956da8b86ec640981104a9d3a3b` |
| `scratch/frame-statistics-owner-final-audit-20260906.json` | `f2452513a1acd3e86bff471f5f3db13fc4dd519863bbb2c4f16624c6b9dc717b` |
| `tools/test_rasterizer_frame_statistics_owner.py` | `27fe02aee97f75b149f4b805cc2dfef238f171257e53406fb3e09c36854abf9b` |
| `tools/fixtures/rasterizer_frame_statistics_owner.c` | `9e618114639a64ed1c46ab9366db3f766409c70c5b1daf9ebc6fa57ec6a30b3e` |

The scoped patch generator is
`scratch/frame_statistics_owner_patch_20260906.py`; it emits a patch only,
which root applies through `apply_patch`. The independent frozen audit is
`scratch/frame_statistics_owner_audit_20260906.py`, with its result at
`scratch/frame-statistics-owner-audit-20260906.json`. The before/after stable
snapshots are `scratch/frame-statistics-owner-{before,after,final}-stable-20260906.json`.

Canonical adjusted totals remain **930,074 meaningful exact code bytes,
6,282 credited functions and 391 / 833 Matching objects**. No donor headline,
fuzzy percentage or shared-type cleanup is counted as a new completed object.
