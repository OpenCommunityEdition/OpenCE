# Geometry: Fable geosphere reconciliation (2026-09-06)

## Scope and current disposition

This packet reconstructs the coherent `geosphere_new` graph and its four
genuine private callees. It is **admitted as a coherent verified fuzzy
packet**, not an exact closure or a whole-object admission.
The ordinary build is **14 exact / 5 residual / 11 unwritten**. All 14
inherited Geometry matches and all **6,255** full-project strict matches are
preserved. There is **zero new exact code-byte credit**. The published
checkpoint remains `c2561af50c7fda9c5ea3326b35b4ba10f90d17f5`, with
916,551 meaningful exact code bytes, 6,209 credited functions and 391/833
Matching objects.

The read-only donor is `C:/halo-worktrees/fable-50k-small-families-20260904`,
tip `f9dd42218b3325e1b3a63d7860e5309f69fd3d8e`. Its first cast-bearing
source/object is retained as explicitly rejected history. No donor object
was installed in the build, and no entire donor commit was cherry-picked.

## One genuine typed-C repair

The donor normalizes a `real_point3d` through a `real_vector3d *` cast. The
established global typed-C boundary rejects that representation punning.
Root made one semantic repair: interpolate into a true `real_vector3d`
local with `set_real_vector3d`, call the genuine `normalize3d`, and copy
the three scalar components into the destination point with
`set_real_point3d`. There is no overlapping union, intermediate void cast,
raw-offset facade, hand-copied math, or source-shape variant search.

The callers reserve a distinct destination slot; its parent slots need not
have lower indices, because face subdivision can reserve its destination
before allocating edge vertices. Runtime validation must therefore execute
the whole graph from its authentic public entry rather than substitute a
made-up private ABI or assume parent-before-child index ordering.

The complete `geosphere` type and public API already belong to
`source/math/geometry.h`; no header or configuration edit is needed.
The four private prototypes and definitions stay in Geometry's C file with
semantic names. Constants, authentic 72-byte primitive vertices and
48-byte primitive triangles are TU-private. The 120 bytes remain one real
read-only section; the triangle label at offset 72 is not extra storage.

Two binary-confirmed original bugs are preserved and explicitly commented:
segment count 1 fills the allocated index buffer but fails January's strict
capacity assertion; a child allocation failure frees successful child
allocations but returns the outer owner with dangling members. These are
target-proven behavior, not a claim of safety or license to preserve an
unverified accidental match. No corrected production behavior is smuggled in.

## Actual typed measurements

These measurements are from the ordinary current output, not copied from
the stronger-looking cast-bearing donor. The chosen repair also changes
compiler-private register schedules in its dependent callers; those callers
were compiled coherently, never tuned individually.

| Function | Target / base padded bytes | Relocations | Actual fuzzy percent |
| --- | ---: | ---: | ---: |
| `geosphere_new` | 496 / 496 | 25 / 25 | 88.11176 |
| `subdivide_triangle` | 944 / 944 | 52 / 52 | 90.79672 |
| `get_face_vertex` | 400 / 400 | 18 / 18 | 75.62987 |
| `get_edge_vertex` | 416 / 400 | 17 / 17 | 61.442856 |
| `calculate_vertex` | 336 / 336 | 18 / 18 | 72.09804 |

The target scope is 2,592 padded bytes, **not 2,592 newly matched bytes**.
No particular residual mechanism is certified merely from these percentages.

## Complete inherited owner and runtime checks

The before/typed capture compares all six actual Geometry-header consumers.
All **318 inherited runtime sections** are unchanged. Only Geometry adds
reviewed owners: five intended bodies, seven natural math fallbacks,
23 assertion literals, a pooled real constant, and the primitive table.
The other five consumer object hashes are unchanged. There is no new COMMON,
BSS, `point_from_line3d` definition/reference, or unexpected undefined API;
`debug_malloc` is the sole new undefined symbol and is a genuine owner API.

The independent full-symbol audit compares all 16 inherited code owners and
16 inherited noncode owners, including bytes, relocations, flags, alignment,
storage, type, value and COMDAT selection. Typed-first and actual owners
agree completely. Adding the two source bug comments then preserves all
**355** resulting runtime sections unchanged.

All seven natural math fallbacks are byte-identical to genuine helper
owners: `set_real_point3d`, `set_real_vector3d`, `normalize3d`, `magnitude3d`,
`magnitude_squared3d`, `scale_vector3d`, and `square_root`. They receive no
target-function credit. The complete **572 base / 833 January object** scan
finds the known current ActionCharge NODUPLICATES owners for `magnitude3d`,
`normalize3d`, `scale_vector3d`, and `square_root`. The existing ActionObey
ledger already proves ordinary both-input-order LNK2005 failures for this
same four-owner set. This remains an explicit **whole-object veto**; do not
suppress helpers, patch selection metadata, or force a link to hide it.

All 23 new assertion literals match their January payload and ownership.
The real constant is compatible across the 120 definitions scanned. This
does not manufacture pooled-data credit or prove a final canonical link.

## Frozen evidence

All paths below are canonical-relative, under the canonical root
`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/rasterizer-model-wrapper-alias-20260829`.

| Artifact | SHA-256 |
| --- | --- |
| `scratch/geometry-fable-geosphere-before-20260906.json` | `d89de4c54a6ca0471b68e1e44debb3f2106cc942ac4d75be62f44ff6ecc8aecb` |
| frozen January Geometry object | `9a223d12be60d64dfed8930aeb8366a33b92c82a693a341b7e10beef0682740c` |
| `scratch/geometry-fable-geosphere-typed-first-20260906.obj` | `f4815540d55abc982dadb422631c2c20df11df10dab0f357150d98fbd455b55d` |
| frozen typed actual Geometry object | `9370a04cd686f339a687f408df0452774bcc5843d7b5b577f3f4a875502be606` |
| frozen typed source | `f17d26efb18b796ceef7f9cf58d4241561be612d85770be28bfadeb38d3be45f` |
| `scratch/geometry-fable-geosphere-final-20260906.json` | `8b22a33fbc3f51a78f44dc7b63de02aa0f3e0568df74dfd36236664928d7c5fa` |
| `scratch/geometry-fable-geosphere-documented-20260906.json` | `bd6216f321d8a3c0321ebd6c2a462cd6b2ddf59837f114c3cf838cc67c33d0db` |
| `scratch/fable_geometry_geosphere_typed_owner_audit_20260906.py` | `3dcb56d054242de949c5bdef85665d6af9b46144dd2f0e35986ef562ae7d21b4` |
| `scratch/fable_geometry_geosphere_typed_owner_audit_20260906.json` | `383ce4019a386b375a96232f253d959f4a95a4c16efe7dcdd918a2b129fba892` |

Capture tool: `scratch/geometry_fable_geosphere_capture_20260906.py`.
Stable before/typed snapshots:
`scratch/geometry-fable-geosphere-stable-before-20260906.json` and
`scratch/geometry-fable-geosphere-typed-stable-20260906.json`.
Runtime-section summaries:
`scratch/geometry-fable-geosphere-typed-runtime-20260906/summary.json` and
`scratch/geometry-fable-geosphere-documented-runtime-20260906/summary.json`.
The original cast-bearing capture remains under the `cluster` phase.

## Integrated runtime, final checks and publication boundary

Root fully reviewed the independent oracle and its 68 fixture-free tests,
including same-object private linkage, real helper execution, allocation
semantics, heap/state comparison and public ABI. Root requested explicit x87
control modes rather than relying on the emulator's ambient default. The
final public replay is `scratch/geometry-geosphere-public-runtime-root-20260906.json`,
SHA-256 `e3749878e29fcc027a897e801492c5c599b9238c34b6536275180beb90abdd51`.

All **20 cases per target/typed-first/actual** pass with zero semantic failures
and zero exact runtime mismatches. Their common trace SHA-256 is
`7657a6f1bf45b06b69dc05815c1ba22377c9eb70243f08243b3763f580ad62a0`.
The cases run the real public entry, all four respective private callees and
the actual `normalize3d` code. Only genuine allocation/free/assertion/exit
APIs are stubbed. Segment counts 2, 3 and the authentic January caller's 16
are covered, plus outer/child/per-face allocation failures and the original
segment-1 capacity assertion. Both explicit x87 modes (`0x027f`, `0x037f`)
pass; neither is claimed to be the established game's default.

The production-sized result has 1,026 vertices, 2,048 triangles and 128
strips. All 1,020 normalization calls and complete seeded heap/vertex/strip
state agree exactly. No numeric tolerance was used. Twelve synthetic
captured-outcome/hash negative controls detect their mutations; they are
not input-binary mutation tests. x87 TOP balance is checked, not overstated
as proof of an empty physical register file. Freed memory stays mapped for
state inspection; allocator metadata and process teardown are outside scope.

Public oracle: `tools/audit/geometry_geosphere_runtime_differential.py`,
SHA-256 `657a72f8ba57a6d453d1220446b5a598b80df750900d1b9e702071935c69933a`.
Public tests: `tools/test_geometry_geosphere_runtime_differential.py`,
SHA-256 `47997215284600a0a7840ccb779d282372b129acfe633b35e1d2a8cc5f322f37`.
The independent intake and detailed runtime record are retained alongside
this ledger as `geometry_obj_fable_geosphere_independent_intake_20260906.md`
and `geometry_obj_fable_geosphere_runtime_20260906.md`.

Root's ordinary Ninja and full strict sweep pass: **6,255 exact, zero gains
and zero regressions**. Five honest `unclassified` parks bring the manifest
to **289 active / zero stale / zero invalid**. Admission reports zero
candidates/contradictions/revocations and the five pre-existing rejections.
The complete public suite passes **449 tests plus 26 subtests**; logs are
`scratch/geosphere-publish-ninja-root-20260906.log` and
`scratch/geosphere-publish-pytest-root-20260906.log`. The stable snapshot is
`scratch/geosphere-publish-stable-root-20260906.json`.

Exact code/data/object totals remain unchanged. Commit and both authorized
remote refs must be verified independently before calling this published.
