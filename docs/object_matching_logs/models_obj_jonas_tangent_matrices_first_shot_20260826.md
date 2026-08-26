# `models.obj` tangent-matrix first-shot recovery — 2026-08-26

## Outcome

This bounded follow-up recovers `_model_build_tangent_matrices` from the
January Xbox target on the first and only code-producing candidate compile.
The function is strict exact at 109 meaningful bytes / 112 padded bytes with
both relocations and normalized SHA-256
`ace1ed2977ba261e3284e16f3b166315afd0f0eeae10780f8e6c30f6f241eb0c`.

All six inherited exact functions remain strict.  Consequently
`source/models/models.obj` advances from **6/12 to 7/12 exact functions**, and
the campaign gains **109 code bytes and one function**.  The object remains
incomplete; no data or object-completion credit is claimed.

## Frozen evidence

The wave starts from cumulative commit
`3990f91f6ebf9dfef922b191a86001285cf7b467`.

- January target object SHA-256:
  `2F888351B09FB71C9042E11FF32A88975B7676A9D966D9A0ECA6A82609EB6C65`
- frozen cumulative baseline object SHA-256:
  `C964579FFC6103F91A4A340DD8E33C304908AFF6C63191D60191B2548E410CF9`
- baseline source SHA-256:
  `522871ACA4CB91B46F357ADE3F40919D8C8270871477BA17D6ABCE605BDB2116`
- baseline header SHA-256:
  `2126C0C35AACDCE94C52C17BBC2F01E080A311B996A852ECCE14685BC75AFAC2`
- first candidate object SHA-256:
  `D222250A289477FD7D951A9B26D302A91B07ABF6BEEA39798DECBCEBB01E8427`
- first candidate source SHA-256:
  `A1EF68F5FE649077BB3758C6D024C4DB561EAEF680BA30762FB2F738EE990591`
- first candidate header SHA-256:
  `2FA3D7A39B71205DA653B9BBA4CA7CBBC397ABE13575F803038E9B0D84F8C768`

All artifacts and regression manifests are preserved under
`outputs/models_tangent_evidence_20260826/`.

## Reconstruction and layout proof

January disassembly proves an otherwise no-op nested geometry/part walk.  The
outer typed block access passes element size `0x30`; the inner typed block
access passes `0x68`.  The first call's result is used at `+0x24` as the
geometry's `parts` tag block.  These two calls are the function's only
relocations, both `REL32` references to
`_tag_block_get_element_with_size`, at offsets 37 and 69.

The later HCEA donor
`research-cache/halocea-cseries-20260820/src/blam/models/model_build_tangent_matrices.c`
confirms the no-op nested-loop semantics, but its later
`gbxmodel_geometry_part` is `0x84` bytes and uses direct address arithmetic.
Neither later-build detail was imported.  Production source instead uses
file-private opaque January layouts: a `0x30` geometry with the proven
`parts` field at `+0x24`, and a `0x68` part.  Unproven fields remain
`reserved`; no speculative shared-header layout is introduced.

Both loops use `short` indices, matching January's `movsx` from `AX`/`DI`.
Typed `TAG_BLOCK_GET_ELEMENT` calls preserve the original access boundary and
naturally retain the otherwise unused inner access.  The retained C89 source
uses one parameter per line and an explicit final `return;`, with no assembly,
register forcing, pragmas, volatility, raw addresses, pointer/integer tricks,
or undefined behavior.

## Strict and regression evidence

The hardened comparator proves the new function's full padded bytes and both
ordered relocations exact.  It also proves these inherited functions remain
strict: `_model_interpolate_node_orientations`,
`_model_get_node_orientations`, `_model_get_default_inverse_matrix`,
`_model_find_node`, `_code_00113710`, and `_model_get_marker_by_name`.

The normalized pre-wave regression manifest reports exactly
`_model_build_tangent_matrices` as `NEWLY_EXACT`, five inherited functions as
`still_exact`, and `changed_nonexact: []`.  `_code_00113710` moves a physical
section ordinal because the new COMDAT precedes it in the incomplete source;
direct comparison proves its 16 padded bytes unchanged.  Remaining findings
are limited to compiler debug records and the expected expanded symbol set.
No runtime data owner changes, adjudication, semantic exception, park entry,
object-status change, or comparator relaxation are present.

## Campaign validation

The isolated progress gate reports:

- semantic audit: 470 units, 4,298 functions evaluated, 4,215 accepted exact,
  and zero unit errors;
- all-code progress: 508,050 / 2,198,102 bytes and 4,189 / 11,060 functions;
- Halo code progress: 495,136 / 1,770,166 bytes and 4,022 / 7,574 functions;
- data remains 1,836,756 / 4,176,062 bytes overall and
  1,831,564 / 3,923,451 Halo bytes;
- file completion remains 376 / 833 overall and 274 / 468 Halo files.

The five remaining target functions are `_code_00112db0`,
`_model_get_node_matrices`, `_model_node_matrices_from_orientations`,
`_model_find_marker`, and `_render_model`.

## Integration replay

The isolated recovery is committed as
`a073514cda4cb78cbc317e4fbd10ced460c45f03` and was cherry-picked into the
cumulative campaign as `6b2294301a99b3ed053a098b983df5d25a61af4f`.

The accepted cumulative state is frozen in
`outputs/models_tangent_evidence_20260826/regression-post-admission-cumulative.json`.
Its immediate replay reports all seven accepted `models.obj` functions as
`still_exact`, with no newly exact functions, changed nonexact functions,
failures, or warnings.  No protected Units source was touched and nothing was
pushed.
