# Geometry: convex-hull seed and expansion reconciliation (2026-09-08)

## Result and scope

This packet restores the two public implementations required by the already
exact `convex_hull3d` caller:

- `convex_hull3d_begin` selects four affinely independent points, initializes
  the tetrahedron topology, constructs its four outward-facing planes, and
  clears the unused vertex, edge, and surface tails.
- `convex_hull3d_expand` classifies a new point, removes visible faces, finds
  and walks the horizon, splices surviving vertex fans, and constructs the new
  face fan with bounded caller-provided storage.

Both functions are complete, typed, public C implementations. They are honest
fuzzy residuals and receive zero exact credit. Geometry moves from 18 exact / 6
residual / 6 unwritten to **18 exact / 8 residual / 4 unwritten**. The packet
converts **3,296 target padded bytes** from unwritten to reviewed residual code
without changing any inherited exact function.

`source/math/geometry.h` already contained the release-PDB-authenticated
`vertex3d`, `edge3d`, and `surface3d` layouts and the correct public prototypes,
so it was not edited. The two optional polygon clipping functions remain
outside this packet. No configuration file was edited in this lane.

## Provenance and reconstruction

The January split object is authoritative for code, assertion lines, public
linkage, padded extents, and relocation ownership. Semantic source evidence is
the read-only HCEA material at:

- `scratch/_halocea-reference-20260907/src/blam/structures/convex_hull3d_begin.c`
- `scratch/_halocea-reference-20260907/src/blam/math/convex_hull3d_expand.c`

Fable commit `f9dd42218b3325e1b3a63d7860e5309f69fd3d8e`
authenticates the surrounding public caller, point test, owner types, and
prototypes, but did not contain bodies for this pair. The implementation keeps
project `real` and `boolean` types, one parameter per line, C89 block-scoped
declarations, explicit returns, and the existing public owner names.

The January assertion evidence is retained verbatim: the four pointer checks
at source lines 1710-1713 and 1973-1976, the boundary assertion at 2076, the
vertex-fan assertions at 2098-2099, and the horizon/vertex assertions at
2167-2171 and 2206-2210. The point, vertex, edge, surface, and plane accesses
agree with the owner layouts and the January offsets.

No helper was exported or force-retained. In particular, Geometry emits no
`_point_from_line3d` symbol. January has no Geometry definition or reference
for that helper; its 48-byte `IMAGE_COMDAT_SELECT_NODUPLICATES` owner belongs
elsewhere. The seed's line-distance math therefore remains naturally inlined
with typed vectors instead of creating a conflicting helper owner.

## Measured object evidence

The final isolated candidate is `scratch/geometry-safe-20260908.obj`.

| Function | Target size / relocs | Candidate size / relocs | Target normalized SHA-256 | Candidate normalized SHA-256 |
| --- | ---: | ---: | --- | --- |
| `_convex_hull3d_begin` | 1488 / 28 | 1472 / 28 | `d3e0ba2e2077ce1e167e22398d1ce4b18329c8a3301ef606b46f56456ee282bc` | `1b5824819ddffa0f161d260eec28283d4b1e3fc5a7422dee6ec8e5f5653f8804` |
| `_convex_hull3d_expand` | 1808 / 67 | 1808 / 67 | `bad17e2158737f40d52656451e9f2ce826a8b27d395f84032536dceab223fbb0` | `3b42a1bae5ecb4b878280d95cedf580f83abd8c332c193aae825d59a7ae3c5f1` |

For begin, the ordered relocation-destination sequence is equal, although
later relocation addresses move with the 16-byte extent difference. Target and
candidate contain 482 and 484 decoded instructions. The prefix and the entire
line-search body are instruction-aligned; the dominant unresolved mechanism is
a 12-byte aggregate-home difference (`0x4c` versus `0x40` stack frames), plus
the resulting plane-expression and topology scheduling. A projected-point
aggregate experiment was rejected because VC7 scalarized it more aggressively,
regressing the candidate to 1456 padded bytes and a `0x34` frame.

For expand, padded size, relocation count, and the ordered relocation-
destination sequence agree. Final root audit found that the earlier
98.264755-percent candidate formed indexed pointers before checking their
indices. That abstract-C undefined behavior was rejected. Canonical now
validates each surface or edge index before forming its pointer; the resulting
valid source is **84.64924 percent** similar because VC7 changes allocation and
relocation addresses across the horizon walk. Semantic validity takes priority
over the older superficial similarity, and no allocator-only source steering
was attempted.

## Gates and integration metadata

The required isolated gate passes compilation and the emitted-symbol guard:

```text
python -B tools/campaign/gate.py source/math/geometry --all \
  --out scratch/geometry-safe-20260908.obj \
  --forbid-emitted-symbol _point_from_line3d

exact 18, residual 8, unwritten 4 (30 listed)
```

The source-policy scan reports zero review leads:

```text
python -B tools/fake_match_scan.py --fail-on-findings source/math/geometry.c
Scanned 1 file; 0 review leads
```

The read-only parked-function audit reports 351 active, zero stale, and zero
invalid entries. No configure or Ninja command was run, as required by the lane
scope.

Root integration should retain both functions as zero-credit fuzzy parks with
class `unclassified`, using the target/candidate measurements above and the
ordinary post-integration objdiff percentages. Do not add or change symbol
ownership: both public target symbols already exist in `config/symbols.json`.
The park evidence should point to this ledger and state that begin is a
16-byte/aggregate-home scheduling residual, while expand has exact size and
relocation layout with only the measured instruction/register schedule left.
