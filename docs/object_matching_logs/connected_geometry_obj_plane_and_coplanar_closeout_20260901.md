# `connected_geometry.obj`: plane closure and coplanar park (2026-09-01)

## Outcome

This pass moved `source/tool/connected_geometry.obj` from 5/10 to 6/10
strict-exact functions without regressing any of the 8,245 configured
functions:

| Function | Before | After | Padded bytes | Relocations |
| --- | --- | --- | ---: | ---: |
| `_plane3d_from_points` | unwritten | strict exact | 176 | 2 |
| `_triangle_coplanar` | unwritten | fuzzy parked | 384 | 11 |

The campaign board moved from 710,595 to 710,771 strict matched bytes and
from 4,999 to 5,000 strict functions.  The predicate is deliberately not
included in either strict total.  The unit now has six strict functions, one
measured instruction-scheduling park, and three unwritten functions.

## Evidence and source ownership

The readable Stian reconstruction maps `FUN_001037b0` to
`_plane3d_from_points`, `FUN_00103a00` to `_triangle_coplanar`, and the
surrounding helpers to the same January compiland.  The independent HCEA file
`src/blam/structures/plane3d_from_points.c` corroborates the important
degenerate-plane behavior: normalize the cross product and return `NULL`
when it has zero magnitude.  January does the same; the old header helper
incorrectly returned the output plane on that path.

The production TU now includes its natural owner header instead of repeating
private float-based point, plane, edge, triangle, and geometry definitions.
The source uses `real_point3d`, `real_plane3d`, and the shared connected-
geometry records.  The public `connected_geometry_add_triangle` declaration
remains in `tool/connected_geometry.h`; the math helper remains defined at its
natural shared location in `math/real_math.h`.  The corrected inline helper
preserves the existing January inline schedule in every other includer while
letting VC7 emit the required COMDAT owner in `connected_geometry.obj`.

The final object contains `_plane3d_from_points`, `_triangle_coplanar`, and
the expected vector-helper COMDATs, but it does **not** contain a
`_point_from_line3d` COMDAT.

## Strict closure: `_plane3d_from_points`

The retained function is ordinary typed C:

- form two vectors with `vector_from_points3d`;
- cross them into `plane->n`;
- normalize the normal;
- on success, set `plane->d` with `dot_product3d` and return `plane`;
- on degeneracy, set `plane->d` to zero and return `NULL`.

The success-first conditional is not codegen theater: it expresses the
January/HCEA return contract directly.  Its emitted 176-byte function has two
relocations and is normalized-byte identical to January.

## Fuzzy park: `_triangle_coplanar`

The retained predicate resolves the three triangle points through typed
dynamic-array records, checks each point against the group plane, constructs
the triangle plane in January's point order, and accepts only like-facing
triangles.  It preserves January's observed `y + z + x` scalar accumulation
order instead of routing through a helper whose inline schedule differs.

Target and candidate are structurally identical:

- padded size: 384 / 384;
- relocations: 11 / 11, address-for-address with identical destinations;
- decoded instructions: 151 / 151;
- target normalized SHA-256:
  `02acd868e062aee29df8e260c3fbd62221e3f2ced5e5f43573a1a288e6129d54`;
- candidate normalized SHA-256:
  `4a01cf6dc779006e22869160f05e1640de65f3f9559f720f03f2ab716e7eff99`;
- objdiff similarity: 99.87755%.

The only remaining difference is one dependency-free x87 operand choice at
target offsets `+0x11D/+0x11F`.  January loads `point2.x` and multiplies by
`plane.n.i`; VC7 loads `plane.n.i` and multiplies by `point2.x`.  Sequential
accumulation closed the later facing-dot schedule exactly.  Bounded controls
that did not close this final pair were reversing the x operands, reversing
the final product operands, explicit final grouping, `k+i+j` grouping,
sequential point2 accumulation, and a named point-product temporary.  The
natural version is recorded in `config/parked.json`; no artificial alias,
volatile access, fake dependency, inline assembly, or inert expression was
retained.

## Remaining atomic add-triangle cluster

The following three authenticated private owners remain unwritten:

| Function | Padded bytes |
| --- | ---: |
| `_connected_geometry_find_or_add_vertex` | 192 |
| `_connected_geometry_find_or_add_edge` | 240 |
| `_connected_geometry_add_triangle` | 416 |

They remain one private-ABI cluster.  The previously documented January edge
packet reads an indeterminate automatic orientation byte if the dynamic-array
edge count is negative.  Initializing the byte or adding a guard produces
lawful C but adds machine behavior absent from January; reproducing the read
would violate the no-undefined-behavior rule.  No new source or local-record
evidence removed that boundary, so the cluster was not partially integrated.
The reopen criteria in
`connected_geometry_obj_jonas_add_triangle_cluster_no_ub_boundary_20260830.md`
remain unchanged.

## Verification

- isolated gate: `exact 6, residual 1, unwritten 3`;
- full 241-step Ninja rebuild: pass;
- semantic report: 5,013 semantic exact, 5,024 accepted exact, zero unit
  errors;
- before/after 8,245-function sweep: one gain, zero regressions;
- parked audit: 64 active, zero stale, zero invalid;
- protected `units.obj`: 189/189 strict exact;
- protected `structure_lens_flares.obj`: 3 exact plus its unchanged measured
  park;
- fake-match scan of the touched source/header set: zero review leads;
- tooling tests: 255 passed using a lane-local pytest temporary directory;
- `point_from_line3d` COMDAT audit: absent from the candidate object.
