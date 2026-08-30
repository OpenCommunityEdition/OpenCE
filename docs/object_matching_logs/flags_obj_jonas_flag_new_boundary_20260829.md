# `flags.obj` `_flag_new` one-shot boundary (Jonas, 2026-08-29)

## Result

This bounded wave started from canonical commit
`c4af544228c14b198bcdbf9397118434f1b37099`. The sole frozen production
candidate for `_flag_new` was compiled once and rejected. No production source,
header, configuration, semantic-match, parked-function, or tool change is
retained.

| Measurement | January target | Frozen candidate |
| --- | ---: | ---: |
| Meaningful / padded code bytes | 371 / 384 | 380 / 384 |
| Ordered relocations | 12 | 12 |
| Normalized SHA-256 | `90715ff5d28478273c1f1ed96124b151c41c6ec6c3f7f483ba34c506a1bfacee` | `fa6797dc34f9f9f937d684eba02690f6904a6f81ed8432012c711eafd380f82d` |
| Return instructions | 2 | 3 |

All twelve relocation identities and addends agree in order. Eleven addresses
also agree exactly. The only relocation-address difference is the entry call to
`_global_scenario_get`, at target `+10` versus candidate `+13`. The remaining
addresses are `+36`, `+43`, `+49`, `+72`, `+79`, `+200`, `+206`, `+232`,
`+291`, `+335`, and `+342`, with matching destinations `_tag_get`,
`_flag_data`, `_datum_new`, `_flag_data`, `_datum_get`,
`_flag_datum_get_vertex`, `_global_origin3d`, `_global_zero_vector3d`,
`_flag_datum_get_cell`, `_flag_set_attachment_shape`, and
`_flag_set_trailing_shape`.

The target and candidate are byte-identical from section offsets `0x1E`
through `0x169`, a 332-byte core. The measured divergence is confined to the
entry schedule and return topology. January saves EBX/ESI/EDI and immediately
calls `_global_scenario_get`, then materializes `NONE` in EAX. The candidate
materializes `NONE` in ESI before that call. At the tail, January shares the
definition-rejection path with its two-return layout; the nested candidate
emits a third return. The fail-closed blocker classifier therefore records
`control-flow-return-shape`, recipe R5, with proven confidence. This result is
not exact and receives no code credit.

## Provenance and prior-attempt census

- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `flags.obj` SHA-256:
  `8bf8e0959be8366ab347ccbefbf24a037d9639ec1c57f779c1b9923d91c47c5d`.
- All five pre-existing Flags ledgers were read in full. None mentions,
  compiles, measures, or claims `_flag_new`.
- All 1,517 registered worktrees were inspected for a production
  `long flag_new(...)` definition; none contains one. All refs expose only the
  seven known `flags.c` blobs, and `git log --all -S flag_new` reaches only the
  initial symbol-list comment. Searches of the campaign research, review,
  artifact, and Claude-documentation caches found target symbol records and the
  authenticated donor, but no prior candidate or compile.
- The independent HCEA donor is
  `surreptitiousresearch/halocea` commit
  `570c83fd9c365dad6f2a3e7041705d5b84c7847c`, file blob
  `3a14785140691048ad0b7e64690b6a4179ce1bbe`, payload SHA-256
  `1b1d51045024c4f718b77c914d7646ca1a2eb386eb5fd9937e6b56d1e5ccb4d`.
  It independently authenticates the datum allocation, bounds and blue-shader
  rejection, typed vertex/cell initialization, and final attachment/trailing
  shape calls. January COFF and disassembly remain the byte, field-offset,
  call-order, and ABI authority.

## Frozen source boundary

The one compiled candidate used the existing typed flag datum/accessors and a
target-authenticated typed definition prefix through `shader_blue`. Its natural
C89 body called `global_scenario_get`, rejected `NONE`, allocated through
`datum_new`, initialized each typed vertex from `global_origin3d` and
`global_zero_vector3d`, initialized each interior typed cell, and called both
shape helpers. It used a nested single-final-return layout; that exact shape is
now exhausted and must not be retried.

The candidate contained no assembly, volatile/register scheduling device,
pragma, intrinsic, attribute, optimizer barrier, raw address or byte-offset
access, pointer/integer reconstruction, pointer or union pun, undefined
behavior, byte forcing, object patch, or tool exception. It was fully removed
with an inverse patch. Rebuilding restores `_flag_new` to `UNWRITTEN` and keeps
all ten inherited owners strict exact.

The HCEA donor's explicit early-return topology was not compiled in this wave.
A future wave may reopen only with explicit campaign authorization treating
that independently sourced topology as a distinct frozen candidate; it must
not tune or respell the rejected nested shape.

## Retained regression boundary

After the inverse patch and clean rebuild, `flags.obj` remains 10/16 strict
functions and 1,040/5,888 padded code bytes. `_flags_initialize`,
`_flags_initialize_for_new_map`, `_flags_dispose_from_old_map`,
`_flags_dispose`, `_flag_delete`, `_flags_update`, `_flag_render`,
`_flag_datum_get_vertex`, `_flag_datum_get_cell`, and
`_flag_set_trailing_shape` remain byte- and relocation-exact. No Units,
Vehicles, Matrix Math, AI Debug, or other active-lane source was touched.

Final repository gates pass:

- Complete `halobetacache_build` and `libcmt_build`: pass.
- Strict campaign board: 277/619 objects, 4,751/8,246 functions, and
  659,559/1,922,669 padded code bytes; Flags is unchanged at 10/16.
- Full report/semantic/progress: pass. The semantic audit evaluates 4,885
  functions across 470 units, accepts 4,779 exact, and reports zero unit
  errors. Progress remains 383/833 linked files, 602,843/2,198,102 meaningful
  code bytes, 4,740/11,060 functions, and 1,856,018/4,176,062 data bytes.
- Protected `_unit_preprocess_node_orientations`: strict exact at 1,920 padded
  bytes, 87 relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- Object admission: zero candidates, contradictions, and revocations.
- Parked validation: 12 active, zero stale, zero invalid.
- Pytest: 205 passed; `git diff --check`: pass.

The committed scope is this additive ledger only. Nothing is pushed.
