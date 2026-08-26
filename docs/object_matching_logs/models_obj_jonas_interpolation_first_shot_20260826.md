# `models.obj` interpolation first-shot recovery — 2026-08-26

## Outcome

This bounded wave recovers `_model_interpolate_node_orientations` from the
January Xbox target on the first and only code-producing candidate compile.
The function is strict exact at 265 meaningful bytes / 272 padded bytes with
all ten relocations and normalized SHA-256
`3692490ae6c7ae275eb2b9b24ec49ebd6a1f41919e84f80fe30c493283637fc7`.

All five inherited exact functions remain strict.  Therefore
`source/models/models.obj` advances from **5/12 to 6/12 exact functions**, and
the campaign gains **265 code bytes and one function**.  The object remains
incomplete and no data or object-completion credit is claimed.

## Frozen baseline and evidence

The wave starts from cumulative commit
`b9dd796092201d3af456a3bf5dba717490ed253b`.

- January target object SHA-256:
  `2F888351B09FB71C9042E11FF32A88975B7676A9D966D9A0ECA6A82609EB6C65`
- frozen cumulative baseline object SHA-256:
  `B02311121CE447D4615187C5CCE86B9727B292146934E9BCCD27EB6560DAD744`
- baseline source SHA-256:
  `9E2CB3693323EAF7F3EB4F094E551B66504FAA15983C3A1477ED872A5E414276`
- baseline header SHA-256:
  `B8910FFEA725EC671901A133F1A469DD07792BF568F9FF1B80D4936E993A466A`
- first candidate object SHA-256:
  `80A88FA5380DFA0CFD1868EB9449E954CBAFD950D4712BA7A1BBE3AB8FF24A30`
- first candidate source SHA-256:
  `032F1C4A70F469A28700247F88D89EA1926A1461914735DD66B1235985B8ABA5`
- first candidate header SHA-256:
  `6BA7259067C2262EEF84C02C086058D671A1C7407B937BC2CEC4399D74D10EE9`

The target, cumulative baseline, preserved first candidate, source/header
copies, and pre-wave regression manifests are retained under
`outputs/models_interpolation_evidence_20260826/`.

## Provenance and source reconstruction

The primary source-shape donor is the January sibling
`source/models/model_animations.c::interpolate_node_orientations`.  Its
fraction calculation, two assertions, scale blend, quaternion call,
translation blends, and final return already reproduce the same January
compiler patterns.  The target `models.obj` disassembly proves that this
routine substitutes a `struct model const *` first parameter and obtains its
loop bound from `model->nodes.count`.

The later-build semantic donor
`research-cache/halocea-cseries-20260820/src/blam/models/model_interpolate_node_orientations.c`
independently confirms the five-parameter contract and, importantly, the
in-place target-orientation parameter that its decompiler initially lost.
That later source is hypothesis evidence only; January bytes, assertion line
numbers 579/580, ABI, padding, and relocations remain authoritative.

The campaign methodology, exact-match acceleration playbook, project Claude
and agent guidance, and the Stian reconstruction/tooling guidance were
reviewed before emission.  The retained code is C89, formats every parameter
on its own line, and ends the void function with an explicit `return;`.  It
adds no assembly, register forcing, pragmas, volatility, barriers, raw
addresses, pointer/integer tricks, or undefined behavior.  The body performs
no tag or object lookup, so no typed-access macro boundary is implicated.

## Strict function evidence

| Function | Meaningful bytes | Padded bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---:|---|
| `_model_interpolate_node_orientations` | 265 | 272 | 10 | `3692490ae6c7ae275eb2b9b24ec49ebd6a1f41919e84f80fe30c493283637fc7` |
| `_model_get_node_orientations` | 132 | 144 | 1 | `12edab8ab04cee1e331d29d7d09622f5b1b620707304f3905237647738bd7349` |
| `_model_get_default_inverse_matrix` | 36 | 48 | 1 | `b98a4d3e531aa2338d5a99a8e00e36dd64d029986656295a0e4ba2d469048d86` |
| `_model_find_node` | 101 | 112 | 3 | `2b247d6fe3fde5d709c9741530db95c540c0a32e70eac2865a4cbde6de46a944` |
| `_code_00113710` | 1 | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_model_get_marker_by_name` | 458 | 464 | 18 | `84d29b166c5eeebeb037934451b1991b28738f14f3a12a8791ceba2735b5d9e1` |

The new function's ordered relocations are the `1.0f` constant, two assertion
source/condition literals, two `display_assert` calls, two `system_exit`
calls, the reused models source-path literal, and the quaternion interpolation
call.  Address, type, destination, and addend agree with the target for every
entry.

## Regression and ownership boundary

The normalized pre-wave manifest differs from the candidate exactly as
expected for inserting a target-owned function before five existing COMDATs:

- `_model_interpolate_node_orientations` is the sole `NEWLY_EXACT` function;
- `changed_nonexact` is empty;
- the five inherited accepted-function records move physical section ordinals,
  but direct hardened comparison proves their complete padded bytes and
  relocation semantics unchanged;
- the target-owned `frame_count>0`, `frame_index<frame_count`, and `1.0f`
  COMDATs enter the candidate, while the shared models source-path literal is
  reused;
- compiler debug records and the COFF symbol inventory expand accordingly.

The generic manifest deliberately fingerprints those physical ordinals and
therefore returns nonzero for review.  No adjudication, semantic exception,
park entry, object-status change, or comparator relaxation is added.  A clean
post-commit snapshot and replay are required to close the accepted enlarged
surface.

## Campaign validation

The isolated full progress gate reports:

- semantic audit: 470 units, 4,297 functions evaluated, 4,214 accepted exact,
  and zero unit errors;
- all-code progress: 507,941 / 2,198,102 bytes and 4,188 / 11,060 functions;
- Halo code progress: 495,027 / 1,770,166 bytes and 4,021 / 7,574 functions;
- data remains 1,836,756 / 4,176,062 bytes overall and
  1,831,564 / 3,923,451 Halo bytes;
- file completion remains 376 / 833 overall and 274 / 468 Halo files.

The remaining six target functions are `_code_00112db0`,
`_model_get_node_matrices`, `_model_node_matrices_from_orientations`,
`_model_find_marker`, `_render_model`, and `_model_build_tangent_matrices`.
They require separate evidence-led waves; this first-shot recovery does not
speculate about them.

## Integrated committed-state replay

The isolated source recovery was committed as
`1c7991beeec35abb910e6123e1aa2b40a8cd2eb5` and integrated into the cumulative
branch as `79b1672813477f362c8f4dc13012a7f7f2bffb88`.

The cumulative XDK rebuild reproduces the 265-byte / 272-byte function and all
ten relocations exactly.  Its clean accepted-state manifest is
`outputs/models_interpolation_evidence_20260826/regression-post-admission-cumulative.json`.
Immediate same-path replay reports all six accepted functions under
`still_exact`, with zero failures, warnings, newly exact functions, or changed
nonexact functions.  No push was performed.
