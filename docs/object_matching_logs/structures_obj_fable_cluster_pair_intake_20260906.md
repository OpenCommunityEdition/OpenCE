# Fable Structures cluster-marker pair: independent intake (2026-09-06)

## Decision

**HOLD both claimed functions; admit 0 of the reported 256 padded bytes.**

The two functions are semantically credible, have genuine target-owned public
ABIs, and form part of the real cluster-marker caller family.  The Fable exact
result is nevertheless not a clean first-natural reconstruction.  It crosses
the explicit no-retry boundary recorded on 2026-08-28 and was obtained after
measuring multiple equivalent source shapes.  No source, header, build,
configuration, or donor file was changed during this review.

The committed Fable packet is distinct from the worktree's current dirty
planar-fog addition.  Commit `f9dd42218b3325e1b3a63d7860e5309f69fd3d8e`
changes `source/structures/structures.c` only by adding the two marker bodies;
its source blob is `03fc23b10b77fefda238b6a4391c98c67e78a9c2`
(parent blob `02bd525a880e1fecc82fb03a6f94ff7b25afcf13`).
The uncommitted `structure_get_planar_fog_definition_index` addition and its
four includes/prototype are excluded from this recommendation.

## Functions and January behavior

| Function | Target ABI/linkage | Target extent | Semantics | Fable result |
| --- | --- | ---: | --- | --- |
| `structure_cluster_unmarked` | external/public, `boolean (short)` | 128 bytes, 11 relocations | assert marker session and signed index in `[0,512)`; return whether the indexed stamp differs from the current marker | byte/relocation strict, normalized SHA-256 `98b5502fb6c8cb58f48c07e07ae161eb7434b246a299c669a8eb61d33efbed6a` |
| `structure_cluster_mark` | external/public, `boolean (short)` | 128 bytes, 12 relocations | same assertions; if stamp differs, store current marker and return true, otherwise return false | byte/relocation strict, normalized SHA-256 `c426fbafcf634a63373bab944083c26aaebda5a181121a993ed0e2c0e644921a` |

January instructions independently establish the signed-short argument and
byte result.  Both functions sign-extend the input short, enforce `< 0x200`,
and return through `AL`.  The mark body compares the indexed dword at global
offset `+8 + 4*index` with the marker at `+4`, stores only on inequality, and
returns `1` only for that store.  There is no semantic defect in the Fable
bodies.

The all-target census covered 833 split objects and found seven call
relocations:

- `source/physics/collisions.obj`: two cross-TU calls to `mark`, from
  `collision_test_vector` and `collision_get_features_in_sphere`;
- `source/structures/structures.obj`: two calls from `code_00188200` and three
  from `structure_clusters_in_cone`, covering both functions.

This is a genuine public caller family, not a private unreferenced-leaf or
retention-anchor opportunity.

## Why the exact Fable forms are not admissible

The canonical 2026-08-28 ledger documents a single four-function natural-C
batch.  `structure_cluster_marker_begin` and `structure_cluster_marker_end`
were retained exact; these same two 128-byte functions were measured residual
and immediately pruned with an explicit prohibition on spelling,
declaration, control-flow, scheduling, or compiler-control retries.  That
ledger already used the HCEA and Pastudan histories plus January disassembly,
so the Fable lane supplies no new provenance that reopens the boundary.

Fable's saved files preserve a clear post-measurement chain:

- `w_structures_v2.c` introduces a temporary `boolean unmarked` local;
- `w_structures_v3.c` changes that to a redundant explicit `(boolean)` cast;
- `w_structures_v4.c` changes it to explicit true/false control flow;
- the worker report says plain `return a != b` was residual while the cast and
  `if` forms were exact;
- the report also says the equal-first `mark` form was residual and selects the
  not-equal-first form after measurement.

The final committed source is the `v3` cast form for `unmarked` and the
not-equal-first form for `mark`.  The claim that the cast is a "direct HCEA
transliteration" is incorrect: the cited HCEA reconstruction, SHA-256
`b460c6241514a7a73119c46ef543ed72cbd105d6ec708396bec92e86cfa16f19`,
contains a plain inequality return and no cast.  The cited HCEA mark body,
SHA-256
`9f82034a89de510248319e6cece6d3d8020ba1d096b8dd770bf13998925c162a`,
uses the opposite equal-first control flow.  These are later binary
reconstructions, not recovered first-party source, but they are enough to
disprove the worker report's source-provenance characterization.

Frozen Fable artifact/source hashes:

- `scratch/ng_structures.obj`:
  `5f0992ec5ce77e32df5f15ab50f562793ac975aa5c4d1d0579dcdb1e03123eec`;
- `scratch/w_structures_v2.c`:
  `5a72c290149c366e8fd5b027e26f2b5e86898f3a00adb42a5b5414158b79a1d6`;
- `scratch/w_structures_v3.c`:
  `712627382375da35d222f8033ba296dcba9d75267eb1f9bb2330ebb99b161053`;
- `scratch/w_structures_v4.c`:
  `0970819ab6196e7ca0c88742f17509a329dedc361ab6305e7cab2401a0959af5`;
- Fable worker report:
  `c9b6a56ea4d2753f3fda54cc0c6b14bb78828f5b931961d39bb6548be0a7c872`.

No new source body should be compiled from this chain.  If an owner explicitly
reopens the exhausted family in the future, the source must be selected from a
single natural semantic authority before compilation; the exact Fable cast
and branch-orientation history must not be used as that first source.

## Object and non-code ownership

The saved candidate adds exactly two code owners and one 65-byte assertion
literal relative to the current canonical base.  It loses no code or non-code
owner and emits no helper code.  It preserves the inherited 2056-byte COMMON
owner `_bss_004c1100`; that raw-name owner is existing debt, not new storage or
evidence of proper ownership.

The new literal is the bound expression
`cluster_index>=0 && cluster_index<MAXIMUM_CLUSTERS_PER_STRUCTURE`.  Its
candidate section is `.rdata`, 65 bytes, alignment shift 4, selection 2,
external storage, zero relocations, with normalized SHA-256
`4a7aa93c8e45f18e6d3263734df296f61549e36aa8135b7344bf5ae4405d448c`.
The 833-object target pool selects the same owner in `effects/decals.obj`; the
572-object canonical pool has compatible same-byte selection-2 owners in
`effects/decals.obj` and `structures/structure_visibility.obj`.  It is pooled
data with zero new data credit, not a collision blocker.

The target global section is `.bss`, 2056 bytes (`0x808`), alignment shift 4,
external storage in split metadata.  All 20 January relocations to it are from
the same Structures TU and use only the proven fields:

| Offset | Field | Type |
| ---: | --- | --- |
| `+0` | `initialized` | `boolean` |
| `+1` | `cluster_marker_initialized` | `boolean` |
| `+2` | unnamed alignment | 2 bytes |
| `+4` | `cluster_marker` | `long` |
| `+8` | `cluster_magic_numbers[512]` | `long[512]` |

January assertion strings authenticate the spelling `structure_globals`.
The HCEX PDB independently records `structure_globals` as `File Static` and
`struct <unnamed-tag>` (`DataKind 5`, section 5, offset `0x2A6CB8`).  That is
later analogous linkage evidence, not a claim that January PDB metadata
directly typed this owner.  PDB SHA-256:
`f55cfe957da8079a62a26a6753f1ddbb700b067a66de0bd5db5aac182d985ff1`.

Accordingly, a future coherent owner cleanup should replace the raw COMMON
definition/macro with a TU-private semantic `structure_globals` object and a
matching static symbol-metadata rename at January file offset 4,985,088.  It
must receive a full dependency/build audit and must not be folded into the
present exhausted-body claim.

## Required owning-header boundary if ever reopened

`source/structures/structures.h` is the real API owner.  It should own the
entire existing public marker family, not just the two new functions and not
caller-local prototypes:

```c
void structure_cluster_marker_begin(void);
boolean structure_cluster_unmarked(short cluster_index);
boolean structure_cluster_mark(short cluster_index);
void structure_cluster_marker_end(void);
```

Direct current consumers of `structures.h` are ten C translation units plus
`source/render/render.h`: Actors, Decals, Game, ObjectLights, GameState,
GameSound, ClusterPartitions, StructureDetailObjects,
StructureRuntimeDecals, and Structures itself.  Because `render.h` is a
transitive include owner, any future header trial must capture the actual full
compiler dependency closure rather than treating that direct list as the
complete blast radius.

## Reproducibility

Independent COFF census:

- `scratch/fable_structures_cluster_pair_intake_20260906.py`, SHA-256
  `5ae316e76d0167965985af9d8b5c35c1973695af5ecaa3a695901812abf3e5e2`;
- `scratch/fable_structures_cluster_pair_intake_20260906.json`, SHA-256
  `580e1e8328dfbfad5d0e4654587a7930aff4bb0737a53336a351d298b3e0dd58`.

Frozen objects used by that audit:

- January target:
  `0e0875524b3580c272bb51b1d5630540d58ce9e1e1ae6c3ef52ca5f30669a4d7`;
- canonical base:
  `80dc76a1bd449ce855753065ad4996cbabfc018306ac90f3d783f0c31bd662f8`;
- Fable saved candidate:
  `5f0992ec5ce77e32df5f15ab50f562793ac975aa5c4d1d0579dcdb1e03123eec`.

Canonical prior-boundary ledger SHA-256:
`7ba02393f87c35f5bfb748ea67f6fea5e39c9e3fdd2856758d7e8c8faab91349`.
