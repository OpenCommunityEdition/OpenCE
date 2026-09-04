# `collision_bsp.obj`: audited sphere-traversal salvage (2026-09-04)

This packet reconstructs the four private functions used by
`collision_bsp_test_sphere`. January COFF remains the sole byte, ABI, and
relocation authority. The independently recovered HCEA and Stian sources are
corroborating evidence for names, types, field meanings, and control flow.

The first version of this packet was **not admissible**. Independent review
found both a semantic error and an incomplete emitted-function audit. This
revision preserves the credible reconstruction, retracts the invalid claims,
and accepts three natural fuzzy residuals rather than manufacturing exactness.

## Scope and source quality

- Lane base: `c8d001ee2` (`Match object_damage_update`).
- Rejected packet commit: `0499df2e5` (`Reconstruct collision BSP sphere
  traversal`).
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `collision_bsp.obj` SHA-256 after semantic symbol recovery:
  `947f78953e7629eb58b9385bce671b77eb5eefb33737bd19aa2d582d7a01aae`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1`,
  SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
  under the repository's unchanged January Xbox `/O2 /Oy-` recipe.
- Audited scratch candidate:
  `scratch/collision-sphere-natural-no-new-helpers.obj`, SHA-256
  `77caf7c71c5779cb9eadf51cd4187341d606e0a699ba07a5fc2e46abc617bb7`.

The recovered private names are `add_feature`,
`collision_surface_test_sphere`, `bsp2d_test_sphere_recursive`, and
`bsp3d_test_sphere_recursive`. The recovered timing global is
`collision_bsp_usage_times`. The private declarations remain in their owning
translation unit.

The sphere traversal context has the January-authenticated `0x228` layout: a
128-entry plane stack at `0x1C`, a signed projection axis at `0x21C`, a byte
projection sign at `0x21E`, and the projected 2D center at `0x220`. The leaf
and 2D-reference records are semantic TU-private structs, avoiding a shared
header edit and its known VC7 definition-position blast radius.

## Audit corrections

The rejected candidate emitted five helper COMDATs that are absent from the
January object:

- `_point_from_line3d` (48 padded bytes)
- `_projection_from_vector3d` (80 padded bytes)
- `_projection_sign_from_vector3d` (96 padded bytes)
- `_distance_squared3d` (64 padded bytes)
- `_plane2d_distance_to_point` (32 padded bytes)

The prior ledger inspected only the named target functions and incorrectly
reported the packet clean. The audited candidate emits none of those five.
`_project_point3d` remains in the object, but it was already present in the
lane baseline and is not induced by this packet.

The prior ledger also claimed that the 3D residual consisted of three
equivalent x87 unordered-value branches. That claim is retracted. The shared
`projection_from_vector3d` body selects the wrong axis for x- and y-dominant
vectors, and different unordered branches are not generally equivalent for
NaNs. The retained named-axis conditional matches January's comparison stream
instruction-for-instruction, including its unordered/NaN behavior:

```c
if (absolute_k >= absolute_j && absolute_k >= absolute_i)
	projection = _z;
else
	projection = absolute_j >= absolute_i ? _y : _x;
```

This is ordinary typed geometry, not a byte-match-only branch. Direct
projected-point and projection-sign expressions are retained because both the
January instruction stream and the independent HCEA lift expand this work in
the caller, while January contains no corresponding helper sections. The
embedded `real_math.h` assertion string and line further authenticate the
projection-sign logic. No inline attribute, pragma, compiler barrier, fake
dependency, representation cast, or assembly is used to force that schedule.

The surface walker now uses a `byte` breakable index, matching the owning
`collision_surface` field. The unsigned 0..255 domain is preserved while
ordinary C integral promotion reproduces January's signed range comparison
and logical right shift. The `BIT_VECTOR_TEST_FLAG` cast is the established
boundary for the public byte-packed breakable-surface buffer and is also used
by the owning breakable-surface subsystem. The loop sentinel remains
`surface->first_edge_index` rather than a synthetic cached local, matching the
January re-load topology. Its squared-distance calculation uses ordinary
`delta_x`, `delta_y`, and `delta_z` locals rather than a decompiler-shaped
expression or an emitted helper.

## Results

| Function | Meaningful | Padded | Relocations | Result |
|---|---:|---:|---:|---|
| `_add_feature` | 44 | 48 | 0 | strict exact |
| `_bsp2d_test_sphere_recursive` | 145 | 160 | 3 | natural fuzzy, 98.064514% |
| `_collision_surface_test_sphere` | 875 | target 880 / candidate 848 | 13 | natural fuzzy, 86.4106% |
| `_bsp3d_test_sphere_recursive` | 792 | 800 | 22 | natural fuzzy, 99.08602% |

The admissible strict gain is therefore one function, 44 meaningful code
bytes, and 48 padded bytes. The focused object advances from 16 to 17 strict
functions. Together with the inherited
`collision_surface_find_closest_point2d` residual, the current frontier is 17
exact, four residual, and nine unwritten functions.

### `bsp2d_test_sphere_recursive`

Target and candidate both have 77 instructions, 160 padded bytes, and the
same three ordered relocation identities at the same addresses. The entire
aligned residual is one commutative multiply operand load: January loads
`center2d->x` and multiplies by `plane->n.i`; VC7 loads `plane->n.i` and
multiplies by `center2d->x` for the retained natural direct expression. The
shared helper call makes the body strict exact but emits the non-target
`_plane2d_distance_to_point` COMDAT, so it is rejected.

### `collision_surface_test_sphere`

The body preserves all three independently corroborated phases: collect
vertices inside the sphere, collect intersecting edges, then accept the
projected polygon interior when no boundary feature was hit. It lazily skips
destroyed breakable surfaces and naturally expands the exact duplicate-
suppressing `add_feature` helper. Target and candidate retain the same 13
ordered relocation identities. January's vertex-distance reduction uses
packed SSE, whereas the natural direct scalar expression emits x87; this
changes the stack frame and downstream register/lifetime schedule. No local
SSE transcription or shared-header rewrite is retained without authoritative
source evidence.

### `bsp3d_test_sphere_recursive`

Target and candidate both occupy 800 padded bytes and have the same 22
ordered relocation identities. The candidate has 290 instructions versus
January's 287. After correcting the projection semantics, the aligned
differences are a commutative y/z plane-distance evaluation order and the
natural `boolean` materialization (`mov al` / `xor al`) versus January's
32-bit form (`mov eax` / `xor eax`). The latter moves the final two call
relocations three bytes earlier and leaves three trailing alignment bytes.
The conservative park class is therefore `unclassified`, not merely
instruction scheduling.

## Focused validation

- Final focused gate: 17 exact, four residual, nine unwritten; all 16
  inherited exact functions remain exact.
- `_add_feature`: target/candidate 48 padded bytes, zero relocations, normalized
  SHA-256
  `e18e07cfbea5714a01b37f116c6a286787bf7e1428f3e5799981770700be1112`.
- `_bsp2d_test_sphere_recursive`: target/candidate 160 padded bytes and three
  relocations; normalized SHA-256 respectively
  `3d3e2ebc2475a44e6fc46a61a195af74e7bfe2b9123b724beb96f1fb3dc9c8f1`
  and
  `bbd48caaee511fea2e988c180bd90ee0e2b3ea970e1b12ca9e147a10ddeb5e15`.
- `_collision_surface_test_sphere`: target 880 / candidate 848 padded bytes,
  both with 13 relocations; normalized SHA-256 respectively
  `d8ecf6e6e8e9fdc80c42b6c515fbb1b84a20be1adeb2938ded8793f43495079d`
  and
  `b8b40ff369f5c4845872f78a79bf92531f06a252973eacbb4e122b3ac3a439fb`.
- `_bsp3d_test_sphere_recursive`: target/candidate 800 padded bytes and 22
  relocations; normalized SHA-256 respectively
  `35304d9cd3dbf061092fc383fa53e18c78003d1c12cb9cddce7cf9b6e69381ed`
  and
  `f0b2e6723519920dcc3ca4060725b75b2b3fc3cd032e52595af3a6a9669ea008`.
- Full emitted-function inventory confirms that all five packet-induced
  non-target helpers listed above are absent.

The canonical integrator then ran the full `all_source`, `all_split`, report,
and semantic-report build successfully. The reconciled snapshot is preserved
at `scratch/collision-sphere-reconciled-after.json` and reports 6,135 exact
functions out of 8,245. A stable per-function comparison against the pre-packet
snapshot records exactly one gained function and 48 gained padded bytes with
zero regressions. Protected `units.obj` remains 189/189 exact, the fake-match
scan reports zero leads, and the actual rebuilt COFF inventory confirms all
five rejected helper emissions are absent. No stale success claim from the
rejected candidate is carried forward.

## Canonical admission

The original isolated packet and its correction were applied together, without
committing the rejected intermediate source to canonical history. The full
canonical build passes. Snapshot
`scratch/after-collision-sphere-canonical-20260904.json` records 6,149 exact
owners out of 8,245; comparison against the admitted music checkpoint confirms
one gain / 48 padded bytes / zero regressions. The canonical progress report
contains 889,898 meaningful matched code bytes and 6,101 credited functions.
Semantic reporting has zero unit errors. All 238 parks validate and all 284
tool tests pass. The new complete-symbol guard explicitly passes all five
forbidden helper names for the actual canonical collision candidate.

`units.obj` retains its 189 target-exact functions. A separate audit has now
identified an inherited candidate-only `point_from_line3d` definition in that
unit; function-match credit must not be confused with whole-object approval.
This collision packet neither introduces nor resolves that independent issue.
