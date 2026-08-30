# `decals.obj` first-index exact boundary (2026-08-30)

## Outcome

The first and only production candidate makes
`_decal_get_first_decal_index` strict exact at 124 meaningful / 128 padded
bytes with all nine ordered relocations exact.  The normalized SHA-256 on both
the January target and the candidate is
`dcc7694fd91ee63d523be2b57d1cd18125b51aabb3336e18c25d88682244d616`.
All six inherited accepted Decals function packets also remain direct strict
exact.

The candidate is not retained.  The fail-closed whole-translation-unit gate
reports changed accepted evidence for `_decal_new_from_media_collision`,
`_projection_sign_from_vector3d`, `_plane3d_negate`, and
`_plane3d_distance_to_point`; changed assertion/projection `.rdata` owners;
changed `.debug$F|anonymous=2`; and a changed complete symbol-ownership set.
No adjudication or policy waiver was supplied.  Source, header, and selected
object were restored, so this commit records evidence only and claims no new
accepted function or byte.

## Freshness and provenance

- Base commit:
  `58ab7f6b90e6b6c7ef7f238362f4fdebdebb4c8e`.
- Restored `source/effects/decals.c` blob:
  `774d1477c6fba42a3a4ecb91f9335c883f584f5d`.
- Restored `source/effects/decals.h` blob:
  `316946a4cfb1fcb87a06ed7da5331968325f6d6c`.
- January split object SHA-256:
  `f7f7b96b03e0e25328fc51a1780d7673cf82d78923122c58f91e8a7517186bfa`.
- Pre-shot VC7 object SHA-256:
  `f41b31a38d00ea9ca67138c9b350ef08df097a52f1371065e490eeb6a4ce9f9b`.

All applicable Decals ledgers were read before mutation, including the plane
pair, projection-sign, A+RGB fixed-point, and attested `decal_globals` owner
records.  None names, attempts, or freezes this function.  A complete history
search for `decal_get_first_decal_index` finds only the initial symbol
inventory; no production body exists in canonical history.

The authenticated HCEA donor at commit
`c89106c4964f2df3a98ef7b4035d6750372797cd` supplies the same-name function
and the named `struct decal_globals` layout.  Its table is exactly five decal
layers by 512 structure clusters, followed by the disconnected-list head,
locked count, and permanent count.  Canonical
`source/structures/structures.h` independently defines
`MAXIMUM_CLUSTERS_PER_STRUCTURE = 512`, and HCEA's database-attested decal
layer enum defines `NUMBER_OF_DECAL_LAYERS = 5`.  The earlier Decals owner
attestation independently fixes the January relocation at aggregate BSS
`_bss_00435cb0 + 192` as `_decal_globals`.  These sources establish a named,
typed, link-closed C implementation without offset access or inferred ABI.

January disassembly independently confirms the complete behavior: keep both
signed-short arguments in `DI` and `SI`, assert the cluster range at source
line `0x3EE`, assert the layer range at source line `0x3EF`, then return
`decal_globals->first_decal_indices[layer][cluster_index]`.

## Sole natural candidate

The candidate added the authenticated decal-layer constants and full typed
`struct decal_globals`, changed only the existing file-static owner from
`void *` to that named pointer type, and inserted this target-order body:

```c
long decal_get_first_decal_index(
	short cluster_index,
	short layer)
{
	match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 1006,
		cluster_index>=0 && cluster_index<MAXIMUM_CLUSTERS_PER_STRUCTURE);
	match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 1007,
		layer>=0 && layer<NUMBER_OF_DECAL_LAYERS);

	return decal_globals->first_decal_indices[layer][cluster_index];
}
```

It also added the ordinary public prototype to `decals.h`.  The candidate is
readable typed C89 and contains no assembly, `volatile`, `register`, pragma,
intrinsic, attribute, optimizer barrier, raw address/offset access, pointer or
union pun, undefined behavior, byte forcing, synthetic anchor, object patch,
compiler exception, or comparator exception.  Exactly one normal XDK 3911
Ninja edge emitted candidate code.  No declaration, expression, constant,
source-order, flag, or compiler retry followed.

## Strict comparison

The target and candidate packet is exact at every relocation:

- `+29 DIR32` exact Decals source-path COMDAT;
- `+34 DIR32` exact cluster-range expression COMDAT;
- `+39 REL32` `_display_assert`;
- `+46 REL32` `_system_exit`;
- `+76 DIR32` the same exact source-path COMDAT;
- `+81 DIR32` exact layer-range expression COMDAT;
- `+86 REL32` `_display_assert`;
- `+93 REL32` `_system_exit`; and
- `+102 DIR32` `_decal_globals` with zero addend.

Direct complete-COFF comparison also keeps these six inherited packets exact:

- `_decals_dispose_from_old_map`;
- `_decals_dispose`;
- `_decal_new_from_media_collision`;
- `_projection_sign_from_vector3d`;
- `_plane3d_negate`; and
- `_plane3d_distance_to_point`.

The whole-TU manifest nevertheless reports only the two early dispose
functions as `still_exact`; four later accepted owners acquire changed
evidence because inserting the target-order function and its owned constants
changes section and symbol topology.  This is a representation/ownership
boundary, not a function-body residual.  Do not repeat or tune the exact body.
Reopen only as part of an explicitly reviewed whole-Decals ownership
transition that re-proves every affected function, data/debug section, and
symbol owner.

## Restored-state gates

After inverse restoration:

- the whole-TU regression manifest reports all six inherited functions
  `still_exact`, zero changed nonexact siblings, zero failures, and zero
  warnings;
- direct complete-COFF comparison reports the same six functions exact;
- `git diff --check` passes and the production source/header blobs equal the
  base blobs above; and
- protected Units `_unit_preprocess_node_orientations` remains exact at 1,920
  padded bytes, 87 ordered relocations, and normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.

The complete restored-tree gate also passes
`ninja halobetacache_build libcmt_build progress semantic_progress`.  Its
final evidence is:

- semantic audit: 470 units, 4,954 functions evaluated, 4,837 semantic exact,
  4,847 accepted exact, 131 hidden exact / 78,940 hidden code bytes, and zero
  unit errors;
- progress: 384/833 objects, 4,808/11,060 functions,
  610,370/2,198,102 meaningful code bytes, and
  1,856,050/4,176,062 data bytes;
- Halo progress: 282/468 objects, 4,641/7,574 functions, and
  597,456/1,770,166 meaningful code bytes;
- admission: zero candidates, zero contradictions, zero revocations;
- parks: 12 active, zero stale, zero invalid;
- tooling tests: 212/212 pass under `python -m pytest -q`;
- `build/report.json` SHA-256
  `fd2d950866b443b6c2d66c93c767505d8e8b25e05ebd21382fee29840d1cc466`;
  and
- `build/semantic_report.json` SHA-256
  `3c51c7e32625c13fe2f70adde7cfdcc895f7e69390c7cd5e883243e512370936`.

Only this additive ledger is retained.  No production source, header, config,
tool, semantic exception, or parked record changes, and nothing is pushed.
