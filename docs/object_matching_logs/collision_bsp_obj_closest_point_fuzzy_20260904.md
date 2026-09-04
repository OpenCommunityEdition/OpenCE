# `collision_bsp.obj` closest-point fuzzy reconstruction (2026-09-04)

This additive ledger records the accepted natural reconstruction and bounded
fuzzy closeout of `_collision_surface_find_closest_point2d` in
`source/physics/collision_bsp.c`. January COFF remains the byte authority;
HCEA is independent semantic corroboration only.

## Scope and evidence

- Lane base: `08a8c8d1d` (`Match render_debug_object_damage`).
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `collision_bsp.obj` SHA-256:
  `ebed4b7ca91a0f2e3cd73a5f286c6d3cdcc33cf2127996b1d3f3afe56b2cf879`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` under
  the repository's unchanged January Xbox `/O2 /Oy-` recipe.
- Independent HCEA donor:
  `_research_cache/halocea/src/blam/physics/collision_surface_find_closest_point2d.c`,
  SHA-256
  `d2d8771abba6f0f60f101bcc4866ede1429e0fe589c7da68e27865885f1464da`.
- Earlier bounded investigation:
  `docs/object_matching_logs/collision_bsp_obj_jonas_closest_point_first_shot_20260829.md`.

The earlier investigation rejected and restored a natural HCEA-shaped body,
but explicitly authorized one later reopening if authenticated evidence
supported the opposite `dot < 0.0f` predicate. January does: its x87 status
test routes unordered values through the length arm and matches the opposite
orientation. This lane took that authorized reopening and retained the result.

## Retained source

The body uses the repository's named collision BSP, surface, edge, vertex,
point, vector, `boolean`, and `real` types. It:

1. fetches the surface and walks its circular, consistently oriented edge
   chain through typed `TAG_BLOCK_GET_ELEMENT` access;
2. projects each endpoint with `project_point3d`;
3. classifies whether the query lies outside each edge and whether its
   projection lies before, within, or after that segment;
4. returns the projected edge point or a shared corner when that is closest;
5. handles the last-to-first wrap corner; and
6. returns the input point with `TRUE` when it lies inside every edge.

The owner-header declaration is corrected from `real_point2d *` to `boolean`.
That type agrees with both return paths and with the existing `bipeds.c`
caller, which uses the output argument and intentionally ignores the boolean.

Source/header Git blobs are
`10c64ea8993711e19e4f920256c39d341e3abcb2` and
`a965d39062112b2000bdde3982f5673e2b43d068`. The final raw candidate object
SHA-256 is
`7c94196bfe50a6c752d2bb425e84656d413012807f18f7fd9d0ebbc6de8d66cd`.

## Measurement and residual boundary

| Measure | January | Candidate |
|---|---:|---:|
| Meaningful bytes | 555 | 555 |
| Padded bytes | 560 | 560 |
| Relocations | 11 | 11 |
| Normalized SHA-256 | `c61b255f98ff4e2265d41eed79757db722c37a9d62db50b20142d839be363d6a` | `044384f284562918f8e9709fa15d8ce1b435047fac8d46ddb75bfc2779d2ab43` |
| objdiff similarity | 100% | 98.81739% |

All ordered relocation types and symbolic destinations agree. The first eight
relocation addresses also agree; the final three appear four bytes earlier in
the candidate. The aligned instruction comparison confines the remaining
distance to commutative x87 operand-load choices in the dot/projection math and
one destructive multiply/store choice in the naturally inlined second
coordinate of `point_from_line2d`. The candidate has 237 instructions versus
January's 235.

Bounded source-level probes covered initial flag lifetime, nested versus
flattened branch form, point/edge-vector assignment order, dot-product operand
order, equivalent manual arithmetic, and the scope of `length_squared`.
Variants either worsened size/control flow or converged on this same natural
fixed point. Manual per-call helper expansion and compiler steering were not
retained because they would be weaker source and could disturb the protected
January inline schedule.

## Validation and disposition

- Focused scratch compilation: 16 exact, one residual, 13 unwritten; all 16
  inherited exact functions remain exact.
- `collision_surface_find_closest_point2d`: 555 meaningful / 560 padded bytes,
  11 matching relocation identities, 98.81739% objdiff.
- Full `build/report.json` generation succeeds with the corrected header and
  compiles every includer, including `bipeds.obj` and protected `units.obj`.
- The source uses no raw offsets, anonymous address names, representation
  puns, volatile/register forcing, barriers, pragmas, assembly, fake
  dependencies, explicit inlining, undefined behavior, or nonsensical
  branches.

The function is therefore retained as a coherent fuzzy reconstruction and
parked as `instruction-scheduling`. It claims no strict matched-byte or exact
function credit. Reopen only for authoritative January source/local records or
a natural same-compiler helper/TU context that resolves the remaining x87
schedule without collateral changes.
