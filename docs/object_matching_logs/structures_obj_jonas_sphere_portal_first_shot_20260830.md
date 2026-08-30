# `structures.obj` Jonas sphere-portal first shot (2026-08-30)

This additive ledger records the sole bounded production candidate for
`_sphere_intersects_cluster_portal` in `source/structures/structures.c`. The
candidate was a strict miss, production source was restored, and no function
or object credit is claimed. January COFF is authoritative; HCEA supplies
semantic and layout provenance only.

## Scope and provenance

- Integration base: `63dc85c9435a4b6e2c77eae5a172076965edd80f`.
- Baseline/restored source blobs: `structures.c`
  `02bd525a880e1fecc82fb03a6f94ff7b25afcf13` and `structures.h`
  `3aba07a47350023a322c493d6d433c409abf9ba0`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `structures.obj` SHA-256:
  `0e0875524b3580c272bb51b1d5630540d58ce9e1e1ae6c3ef52ca5f30669a4d7`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1`;
  `CL.Exe` SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
- Flags: `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` with the unchanged XDK 3911
  and repository source include paths.
- Independent HCEA donor:
  `build/audit/refs/halocea/src/blam/physics/sphere_intersects_cluster_portal.c`
  at commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c`, Git blob
  `84db70dd205055ea691a88a17ef10986e6771c36`, and file SHA-256
  `632e01dfcca5f503834fcd22184de681b703530172e63f09deaaf2929a531223`.

Every current Structures object ledger and applicable `AGENTS.md`,
`CLAUDE.md`, tree, HCEA, January, and campaign-tooling document was read before
editing. Searches of complete Git history, all registered worktrees, retained
audit artifacts, donor caches, and Claude documentation found no earlier body,
bounded candidate, or exact artifact for this owner. HCEA is the only
authenticated natural body.

The candidate was link-closed. Its 11 calls resolve, in order, to three typed
tag-block lookups, `_global_bsp3d_get`, a fourth tag-block lookup,
`_projection_from_vector3d`, `_projection_sign_from_vector3d`,
`_project_point3d`, a fifth tag-block lookup, `_project_point3d`, and
`_convex_hull2d_test_circle`. The first ten owners are strict exact. The final
owner is present as ordinary typed C, so the production link remains closed.

## Validated baseline

Before and after this lane, `structures.obj` has 6/17 strict functions,
192/4,000 padded code bytes, and 11 unwritten functions. The target has 418
meaningful bytes, 432 padded bytes, 11 relocations, and normalized SHA-256
`f8a78ffbcb1b98195cd631cd38a79827f47005c05539ca9c9bee9ac26e5fa9d5`.

The frozen pre-candidate whole-TU manifest is
`build/audit/structures_sphere_portal_pre_20260830.json`, SHA-256
`de6ecb982966ed44eec11494614126e38340edc88ae333ed4b77c7a1afe1f848`.
The pre-candidate raw base object SHA-256 was
`3569a31c2d440e314a297d4c252c1723ba09e7b5e9dd12a2606311b16cd0cd25`.

## Sole frozen candidate

The natural typed C89 candidate followed the HCEA semantic chain while using
January's independently observed helper topology. It used a named, asserted
`structure_cluster_portal` layout; typed nested collision-BSP and plane
lookups; plane distance and bounding-sphere rejection; named projection
helpers; a short-index typed portal-vertex loop into a bounded array of
`real_point2d`; and the named convex-hull circle test. The function had an
explicit return on every path, and parameters appeared one per line.

The source body, predicates, declarations, and typed layout were frozen before
the candidate build. Candidate source/header Git blobs were
`be05daa856bd52176d27ee68807153b6f8e0d128` and
`b47a0e61c980d74415bb54d224ac31a47e72a399`.

A source-policy audit found no assembly, `volatile`, `register`, pragma,
intrinsic, compiler attribute, barrier, raw offset or address, pointer/integer
reconstruction, representation pun, undefined behavior, byte-forcing device,
object patch, or compiler/tool exception.

### Mechanical pre-emission failure

The first Ninja invocation stopped in header parsing because
`math/geometry.h` preceded `math/real_math.h`, leaving the geometry header's
real-math types undefined. It did not parse or compile the function body and
did not emit an object: the existing object remained byte-for-byte at the
pre-candidate raw SHA-256
`3569a31c2d440e314a297d4c252c1723ba09e7b5e9dd12a2606311b16cd0cd25`.

With coordinator authorization, only those two include positions were
mechanically corrected. The frozen body, predicates, declarations, and layout
were unchanged. The corrected Ninja invocation was therefore the sole
code-producing candidate compile. No follow-up spelling, predicate,
declaration, expression, or control-flow variant was compiled.

| ID | Source shape | Meaningful T/B | Padded T/B | Relocs T/B | Normalized target | Normalized candidate | Decision |
|---|---|---:|---:|---:|---|---|---|
| E01 | Natural typed HCEA portal test with January helper topology | `418/408` | `432/416` | `11/11` | `f8a78ffbcb1b98195cd631cd38a79827f47005c05539ca9c9bee9ac26e5fa9d5` | `c3bc4abb655882765be05666723c3c4325955ff043b2eb6f399d965b08a0184b` | rejected and reverted |

The ignored evidence object is retained at
`build/audit/structures_sphere_portal_first_shot_20260830/structures.obj`;
its raw SHA-256 is
`ae872aa43849adfefe62dd0055d14052a41eec71b1ebf106cc954b5e51f240d4`.
The normalized padded comparison has 181 differing bytes.

## Relocation and disassembly boundary

All 11 relocation types, destinations, and order agree. Seven addresses agree
exactly; the four calls in the middle region are one byte later in the
candidate:

| Ordered destination | Target offset | Candidate offset |
|---|---:|---:|
| `_tag_block_get_element_with_size` | `+0x1E` | `+0x1E` |
| `_tag_block_get_element_with_size` | `+0x39` | `+0x39` |
| `_tag_block_get_element_with_size` | `+0x45` | `+0x45` |
| `_global_bsp3d_get` | `+0xC2` | `+0xC3` |
| `_tag_block_get_element_with_size` | `+0xCB` | `+0xCC` |
| `_projection_from_vector3d` | `+0xD3` | `+0xD4` |
| `_projection_sign_from_vector3d` | `+0xDD` | `+0xDE` |
| `_project_point3d` | `+0x118` | `+0x118` |
| `_tag_block_get_element_with_size` | `+0x145` | `+0x145` |
| `_project_point3d` | `+0x14E` | `+0x14E` |
| `_convex_hull2d_test_circle` | `+0x185` | `+0x185` |

The normalized prefix is exact through `+0x5C`. The first substantive
instruction-scheduling difference is at `+0x5D`: January loads the point's X
component and multiplies by the plane normal's X component, while the current
inline helper loads those commutative operands in the opposite order.

At `+0x75`, January emits a long branch to one shared false epilogue at
`+0x199`. The natural candidate emits a parity branch around a dedicated early
false epilogue. January also evaluates centroid deltas before the summed
radius; the candidate evaluates the named reach first. The two builds realign
at the first `_project_point3d` relocation, but retain different projection
stack/register ownership. At the tail, January branches to the shared false
epilogue and has an explicit true epilogue, while the candidate materializes
the final boolean with `setne`. The candidate consequently ends ten meaningful
and 16 padded bytes earlier.

## Do not repeat and reopening condition

- Do not retry this compiled typed HCEA source topology.
- Do not sweep include order, declarations, helper spelling, commutative
  expressions, early returns, predicates, or epilogue shapes around this fixed
  point.
- Do not replace named typed access with offsets, casts, aliasing, byte views,
  or any prohibited compiler steering.
- Reopen only with new authenticated provenance that independently establishes
  the point-first X expression, distance-before-reach evaluation, and the
  shared-result/shared-false epilogue topology. Those observations are
  disassembly inferences, not authorization for a tuned retry.

## Restoration and validation

The prototype, typed layout, includes, and body were inverse-reverted with
`apply_patch`. Source and header Git blobs exactly match the baseline values
above. The restored raw base object SHA-256 is
`60e4be3fee22eb7759e5ba28833e8a22e3c5143c47d7ccd5a32507f19143b90e`.
The frozen whole-TU regression reports all six inherited exact functions still
exact, with no changed nonexact or newly exact functions, failures, or
warnings.

- Aggregate Ninja gates pass: `halobetacache_build`, `libcmt_build`,
  `semantic_progress`, and `progress`.
- Semantic audit: 470 units, 4,917 functions evaluated, 4,800 semantic exact,
  131 hidden exact / 78,940 bytes, 4,810 accepted exact, one ordinary-only,
  one ordinary structural, zero ordinary rejected, and zero unit errors.
- Progress report: 384/833 objects, 4,771/11,060 functions,
  606,187/2,198,102 code bytes, and 1,856,050/4,176,062 data bytes.
- Strict global board: 277/619 objects, 4,782/8,246 functions, and
  663,079/1,922,669 padded code bytes.
- Protected Units remains 189/189 and 54,864/54,864 padded bytes. Sentinel
  `_unit_preprocess_node_orientations` remains exact at 1,920 padded bytes,
  87 relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- Python tool suite: 212/212 pass (one non-functional pytest-cache permission
  warning only).
- Admission audit: zero candidates, contradictions, or revocations.
- Parked audit: 12 active, zero stale, zero invalid.
- `build/report.json` SHA-256:
  `19402f6c01804c9d844537e23194000ab2a656cd1a76bc7d32f7c7768a3af302`.
- `build/semantic_report.json` SHA-256:
  `f1e19b7ef56c296d953cc7c7db7bba7f1f379444c230a4a4541bb3ba1b6391de`.
- `git diff --check`: pass.
