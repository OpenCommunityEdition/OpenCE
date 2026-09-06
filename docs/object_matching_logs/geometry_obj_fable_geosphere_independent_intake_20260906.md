# Geometry geosphere residual independent intake (2026-09-06)

## Scope and disposition

This is a read-only review of the five committed Fable bodies in
`source/math/geometry.c`:

| Function | January padded / meaningful / relocations | Fable padded / meaningful / relocations | Ordinary report |
| --- | ---: | ---: | ---: |
| `_calculate_vertex` | 336 / 321 / 18 | 336 / 321 / 18 | 96.56863% |
| `_get_face_vertex` | 400 / 385 / 18 | 400 / 391 / 18 | 82.48052% |
| `_get_edge_vertex` | 416 / 402 / 17 | 400 / 386 / 17 | 65.39286% |
| `_geosphere_new` | 496 / 494 / 25 | 496 / 481 / 25 | 88.11176% |
| `_subdivide_triangle` | 944 / 939 / 52 | 944 / 935 / 52 | 93.459015% |

The bodies are a coherent real caller closure and are credible semantic
reconstructions, but the first source is **withheld as written**.
`_calculate_vertex` contains a point/vector representation-pointer cast that is
expressly outside the campaign's typed-C policy.  Every other reviewed function
reaches it, so none of the five is a separable admissible packet.  In addition,
the natural translation unit emits inline math COMDATs whose whole-object
linkage is not clean.

After this review, root selected one defined-C semantic repair before any
follow-up compile: form the same three components in a true `real_vector3d`
local, normalize that local, and copy its three components to the destination
with `set_real_point3d`.  This is not evidence for January's original source and
must not be used as an exactness control.  It is nevertheless semantics-
preserving on the complete real caller graph: the destination is a newly
allocated slot distinct from both parents (a face path may allocate a parent
after reserving the new slot), with no intervening observer or side-effectful
API.  It preserves the same float
expressions, normalization, zero/NaN outcome and final 12 bytes.  Only
out-of-contract aliasing of the new slot with a parent could distinguish the
store timing.  Therefore a single frozen compile of that already-selected
repair may support honest function-level fuzzy retention if its graph and all
new owners pass.  It does not waive the whole-object helper veto and is not
permission for further source variants.

## Provenance and first-natural history

The donor is the clean committed state of
`C:/halo-worktrees/fable-50k-small-families-20260904`:

- branch `fable/50k-small-families-20260904`;
- tip `f9dd42218b3325e1b3a63d7860e5309f69fd3d8e`;
- committed `geometry.c` blob
  `b38145bf4f140d73e7503700e2a16e46d4d01acb`;
- committed `geometry.h` blob
  `744a68e77443205a67fed08a527539794630d4d8`;
- final donor object SHA-256
  `ff9f9582d7d31cb72d7711d4098a2b228d9bd06e79427da8abbc3597724dbdf6`.

Neither Geometry file is dirty in the exhausted Fable worktree.  The five
function sections and the primitive table are byte-, relocation-, and
metadata-identical across the saved first-chain objects:

- `scratch/w_geometry.obj`, SHA-256
  `459ed94a5b585084411ff1dd27120f6a5105cf6dc35855f4eb315698ebca7993`;
- `scratch/ng_geometry.obj`, SHA-256
  `0f64a31af7a6970e5f6a8a519d0d35330673c642cec90fb6434ba2e4781e5077`;
- final donor base object above.

The saved `w_geometry_tv` and perimeter experiments do not contain this graph.
No geosphere source-shape retry or post-measurement residual tuning was found.
The HCEA files under
`research-cache/halocea-full-blobs-20260830/src/blam/math/` are later binary
reconstructions, not original-source authority.  Their relevant SHA-256 values
are:

- `calculate_vertex.c`:
  `5ee730fc4efef7a50fb314933f327c9d84fcb021567b2f942149e4a3b2f0ad5a`;
- `get_edge_vertex.c`:
  `8a2002731db24788df7e9e1aa424fee5c3d6166d4dd7eab5e236c54d6c48b5ad`;
- `get_face_vertex.c`:
  `d0088e0f01f438df773733b3acf568fe6d73eb955c0494526dbadc05b3e07997`;
- `subdivide_triangle.c`:
  `149f342e9edcd32f7541a0a4be46249851ab5efa80109b31c93548ebe116977e`;
- `geosphere_new.c`:
  `368ea592ce18e5a74a8e2fdc43670e2359814d5db9fdf5049fbb7c5da379666c`.

## Caller graph, linkage, and ABI

The target and donor agree on function storage and COMDAT policy: the four
helpers are static/storage 3 and `_geosphere_new` is public/storage 2; every
code section is selection 1, alignment shift 5, flags `0x60501020`.

The complete genuine graph is:

```
random_math_initialize
  -> geosphere_new
       -> subdivide_triangle (once for each of eight primitive faces)
            -> get_face_vertex (four sites)
                 -> get_edge_vertex (five sites)
                      -> calculate_vertex
                 -> calculate_vertex
```

A full 833-object January relocation census found the only external caller at
`source/math/random_math.obj::_random_math_initialize` (new at relative call
offset 31, dispose at 180).  All private calls are internal to Geometry.  The
private register assignments are compiler-private and need not be promoted to
public ABI.  The public owner is already correct in `geometry.h`:
`struct geosphere *geosphere_new(short segment_count)` together with
`geosphere_dispose`; the established structure is 20 bytes.  No shared-header
edit, public shim, local extern, or fake export is required.

Because `geosphere_new` genuinely depends on every private helper and the
primitive tables, admitting only the public constructor would be an artificial
emission boundary.  The five bodies must stand or remain held together.

## Primitive tables

The candidate and January target have one byte-identical 120-byte non-COMDAT
`.rdata` section: alignment shift 4, flags `0x40400040`, selection 0, no
relocations, static root `_geosphere_primitive_vertices`, SHA-256
`2f4de0a86fa0fa9c9f232df3c7d9efaa716ce8179eb9d165cf6e47eebb030cc6`.

Decoded values are exactly:

- vertices: `(0,0,1)`, `(0,1,0)`, `(1,0,0)`, `(0,-1,0)`, `(-1,0,0)`,
  `(0,0,-1)`;
- triangles: `(0,1,2)`, `(0,2,3)`, `(0,3,4)`, `(0,4,1)`, `(5,1,4)`,
  `(5,4,3)`, `(5,3,2)`, `(5,2,1)`.

The donor naturally emits static interior symbol
`_geosphere_primitive_triangles` at section offset 72.  January's split config
does not name the interior symbol; the physical table nevertheless matches.
If the packet is ever reopened, a source-authenticated static interior label at
target file offset 2597632 is reasonable metadata, but it must be described as
reconstructed source ownership rather than recovered January PDB proof.

## January semantics and original-behavior cautions

The full January disassembly supports the ordinary typed algorithm:

- `_calculate_vertex` linearly interpolates parent point components and
  normalizes the newly written 3-float value;
- `_get_edge_vertex` canonicalizes endpoint order, handles endpoint requests,
  and memoizes the seven possible interior positions in an 8-by-8 short cache;
- `_get_face_vertex` maps boundary requests through the edge helper and creates
  one interior vertex by interpolating between two edge vertices;
- `_subdivide_triangle` allocates `(segment_count+1)^2` shorts, initializes them
  to `NONE`, builds rows and triangle strips, and frees the temporary map;
- `_geosphere_new` allocates the 20-byte owner, its vertex/strip storage and
  128-byte edge cache, copies the primitive tables, and subdivides all eight
  faces.

The following are target-authenticated behaviors and edge conditions.  The two
binary-and-layout-confirmed defects called out below need explicit
original-behavior comments rather than silent fixes; the loose assertions and
allocation behavior should remain documented in the ledger without implying
that every one is a separately proven reachable bug:

1. generated/parent vertex assertions use `index <= vertex_count`; this is the
   exact January predicate, and no stricter bound should be substituted;
2. a face-map allocation failure is silent, skips that face, and is exposed
   later only by the outer final assertions;
3. **Original bug:** `segment_count == 1` reaches the final strip-capacity
   equality but the
   target asserts strict `<`; zero, negative, and sufficiently large counts
   are not guarded before short arithmetic/allocation-size formation;
4. **Original bug:** if a child allocation in `_geosphere_new` fails, already
   allocated members
   are freed but the owner is not freed and the freed member pointers are not
   cleared; the function returns a non-null, partially initialized owner with
   dangling member state.

These are not permission to add a clamp, cleanup repair, nulling store, or
different assertion.

## Typed-source veto

Current `real_math.h` defines distinct unions:

```
real_point3d  { real n[3]; { real x, y, z; }; { real u, v, w; }; }
real_vector3d { real n[3]; { real i, j, k; }; }
```

They have the same 12-byte representation but neither contains the other as a
typed subobject.  The donor/HCEA body calls:

```
normalize3d((real_vector3d *)&sphere->vertices[new_vertex]);
```

January's instructions prove that normalization occurs in place at the point's
address.  They do not prove that C may reinterpret a `real_point3d` object as a
`real_vector3d` object.  The campaign has already adjudicated this exact class:

- `real_math_obj_jonas_global_typed_boundary_20260830.md` rejects historical
  exact bodies that require point/vector representation-pointer casts and says
  that an overlapping union view only moves the same pun into the type;
- `bsp3d_obj.md` B46 replaced point/vector casts with typed component math and
  explicitly bars their reintroduction;
- `structure_lens_flares_obj_jonas_policy_reconciled_20260829.md` likewise
  requires typed component macros rather than representation casts.

Therefore the first body is not admissible even as a credible fuzzy park.  Do
not bless the cast because HCEA repeats it, add an overlay member, cast through
`void *`, or treat matching same-address ABI as more important than defined C.
The one root-selected vector-local/point-copy repair described above is a legal
semantic correction supported by the full caller topology and by the existing
Real Math policy precedent for a true typed local plus scalar copy-out.  Audit
its first frozen object once; do not search its spelling, declaration order, or
helper-emission shape.

## Naturally emitted math owners and whole-object boundary

The unmodified donor Geometry object emits these selection-2 COMDATs as a
natural consequence of current `real_math.h`:

| Owner | Size / relocations | Candidate SHA-256 |
| --- | ---: | --- |
| `_set_real_point3d` | 32 / 0 | `8bcddd57839f98d10c8d64854238ba75564306cc9ef4d8055ea3eccd128911a0` |
| `_normalize3d` | 96 / 3 | `67fbd33f204d0c93cce135fcfd37d4ce5b443642edc6ad99230846b91c893e5b` |
| `_magnitude3d` | 48 / 0 | `114f10e7ee4543446daaaafc859a4ec729ba6c8c74c38825d05bd0f00a42352c` |
| `_magnitude_squared3d` | 48 / 0 | `0d04579096eeab3a1c45f87774e1eb20ce4856dbac219f624a855e8c9ae99725` |
| `_scale_vector3d` | 48 / 0 | `6436919fbdc80b5d5fe76d9c92fa146e5ac7d3b4637c9b75aaa62401f0c54a3b` |
| `_square_root` | 16 / 0 | `0717923dacb4356a94553a32f0ba8a314904fae6d7d0bfc42ecc4af6f10415b7` |

There is no `_point_from_line3d`.  Other helper definitions in the full Fable
Geometry object belong to unrelated bodies and must not be charged to this
five-function graph.

All six bodies and the three scalar constants have byte-identical real owners
elsewhere.  That does not make the whole object safe: current
`source/ai/action_charge.obj` (SHA-256
`f4d34dac012b9a5a3fe63857f97d12ee67222f2e339d2482760d29458b57d0ef`)
owns byte-identical selection-1/NODUP copies of `_magnitude3d`, `_normalize3d`,
`_scale_vector3d`, and `_square_root`.  The existing ActionObey ownership ledger
already proves ordinary unmodified both-input-order links fail with LNK2005 for
that exact four-owner set.  `_magnitude_squared3d` and `_set_real_point3d` have
compatible current selection-2 owners, but they do not cure the four conflicts.

Thus the natural helper emission is genuine rather than filler, while
whole-object admission remains barred.  Do not suppress it by manual math,
forced inline, metadata patching, `/FORCE`, or source anchoring.  Even if the
typed cast is resolved later, function-level progress must carry zero credit
for these helper copies and an explicit whole-object veto until the real
ActionCharge owner boundary changes.

## Typed-first canonical owner audit

Root's single defined-C repair produced these frozen artifacts:

- typed first object, SHA-256
  `f4815540d55abc982dadb422631c2c20df11df10dab0f357150d98fbd455b55d`;
- actual frozen object, SHA-256
  `9370a04cd686f339a687f408df0452774bcc5843d7b5b577f3f4a875502be606`;
- frozen source, SHA-256
  `f17d26efb18b796ceef7f9cf58d4241561be612d85770be28bfadeb38d3be45f`;
- final capture manifest, SHA-256
  `8b22a33fbc3f51a78f44dc7b63de02aa0f3e0568df74dfd36236664928d7c5fa`.

The typed-first and actual objects are identical for every genuine code and
noncode owner.  Relative to the frozen pre-packet Geometry object (SHA-256
`c3ab4ebdedeb476cc64bc4a0e8db747c05616fe4d7e121e75ee5b8863725171c`):

- all 16 inherited code owners and all 16 inherited noncode owners retain
  strict bytes, relocations, flags, alignment, storage, type, value and COMDAT
  selection; no inherited owner is lost;
- the only intended substantive code additions are the five geosphere bodies;
- seven natural inline fallbacks are also emitted: the previous six plus
  `_set_real_vector3d` (32/0, selection 2, the same body bytes as
  `_set_real_point3d`);
- there is no COMMON, BSS, `_point_from_line3d`, or other unexpected code;
- `_debug_malloc` is the sole newly undefined symbol and is the genuine
  allocation API used by the bodies, not a local facade;
- all 23 new assertion-string COMDATs are strict matches to their January
  owners, including flags/alignment/storage/selection, and the pooled `1.0f`
  constant is byte- and owner-compatible across all 120 definitions found;
- the primitive table remains the one exact 120-byte section.  The source's
  `_geosphere_primitive_triangles` is only an interior static symbol on that
  section, not a second allocation or candidate-only payload.

The repair naturally changes `_calculate_vertex` and the four private/public
callers that inherit its compiler-private register schedule; `_geosphere_new`
itself remains byte-identical to the cast-bearing first candidate.  That is a
coherent private-ABI recompilation effect, not evidence to retune any caller.
The repaired five remain residual, at the same 14 exact / 5 residual / 11
unwritten object frontier reported by the root gate.

A fresh bounded pool scan covered all 833 January split and 572 current base
objects.  Every copy of each of the seven math helpers is byte-identical.
Selection differences are exactly the known ownership boundaries:

- `_magnitude3d`, `_normalize3d`, `_scale_vector3d`, and `_square_root` each
  retain the real current ActionCharge selection-1 owner, so the previously
  proven both-order LNK2005 boundary still decisively bars whole-object
  admission;
- `_magnitude_squared3d`, `_set_real_point3d`, and `_set_real_vector3d` have a
  January selection-1 copy but only compatible selection-2 current base
  owners.  They add no second current canonical conflict and do not weaken the
  four-owner veto above.

The other five `geometry.h` consumers captured before and after the compile
(`game_engine_king`, `breakable_surfaces`, `leaf_map`,
`structure_lens_flares`, and `structure_visibility`) are raw-object hash
identical.  No header-consumer collateral is present.

## Reproducible evidence

- `scratch/fable_geometry_geosphere_residual_intake_20260906.py`, SHA-256
  `d10d08b82db754d58d8e48be05bb1ed1ec48ad8a6cbeef7fa37064c792afc123`;
- `scratch/fable_geometry_geosphere_residual_intake_20260906.json`, SHA-256
  `84929d7dab995093e9e79d6301be35962583585fd79d5f73b32d907bbb89a49f`;
- `scratch/fable_geometry_geosphere_helper_owner_census_20260906.py`, SHA-256
  `62a643f24c923b923b7f32fc3f5272ceaf8146a9c5ddd9bbad578ce03fd05092`;
- `scratch/fable_geometry_geosphere_helper_owner_census_20260906.json`, SHA-256
  `b2bcd310c861f73835d0bb55b2ee559c4402082263e4f59aa43163343f035165`.
- `scratch/fable_geometry_geosphere_typed_owner_audit_20260906.py`, SHA-256
  `3dcb56d054242de949c5bdef85665d6af9b46144dd2f0e35986ef562ae7d21b4`;
- `scratch/fable_geometry_geosphere_typed_owner_audit_20260906.json`, SHA-256
  `383ce4019a386b375a96232f253d959f4a95a4c16efe7dcdd918a2b129fba892`.

The intake script compares full symbols, code/data section properties, bytes,
relocations, saved-candidate equality, private callers, and the 833-object
external-reference census.  The helper census compares full hardened/raw
owner properties across the current canonical base and January split trees.

## Actionable result

- Preserve the frozen first-natural cast-bearing object and source as explicit
  rejected history.
- The already-selected true-vector-local/scalar-copy repair has passed the
  complete frozen owner audit.  Subject to the separately owned runtime proof,
  the five dependent bodies are eligible as one coherent function-level fuzzy
  packet; the first cast-bearing artifact remains inadmissible.
- Make no header edit: the public structure and new/dispose API are already
  owned correctly.
- After that one semantic repair, make no further source variant, helper
  suppression, metadata exception, or compiler exception.
- Retain the documented original bugs, exact primitive table, zero helper
  credit, and explicit whole-object helper veto in any function-level
  admission.
