# `collisions.obj`: vector-exit first shot (2026-08-29)

## Result

The first and only production candidate for `_collision_test_vector_exit` is
not strict exact and receives no implementation credit. It reproduced the
400-byte padded envelope, all six relocation types and symbolic destinations
in order, the complete 0x448-byte typed stack layout, and the donor's control
flow. VC7 scheduled one initialized object-index local earlier than January,
then selected a different register/load order throughout the success packet.
The body is two meaningful bytes longer and its later relocation addresses
drift. No declaration, assignment, helper, predicate, scope, or placement
variant was compiled.

The source body and prototype were removed by the exact inverse patch. The
baseline object was rebuilt and its frozen regression manifest is clean. This
additive record is the wave's only intended tracked result; no production
source, header, configuration, exception, parked record, or protected Units
file survives changed.

## Prior-work and provenance census

No current object-specific Collisions ledger exists. The complete current
object-log index, the Claude near-complete object-log tree, path-limited all-ref
history, registered worktrees, and the authenticated public-source caches were
searched before editing. Ref-reachable history contains the function name only
in the initial address inventory and never contains a definition. No registered
campaign worktree contains a body. The only natural semantic donor found is the
HCEA reconstruction.

HCEA commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c`,
`src/blam/physics/collision_test_vector_exit.c`, is the source/layout donor.
Its payload SHA-256 is
`1650562117091fdc45792b21598bc51b58668685605f46487c56630c7cb9a2d4`.
It supplies the reverse-ray topology, nested collision-model tests, transformed
and optionally negated exit plane, result-field mapping, and final forward-ray
point calculation. January COFF remains the ABI, size, instruction, constant,
relocation, and acceptance authority.

January independently tightens the donor into this repository's natural typed
helper topology. It calls:

- exact `_collision_model_instance_new` (112 padded bytes, four relocations);
- exact `_collision_model_test_vector` (384 padded bytes, 12 relocations);
- exact `_matrix4x3_transform_plane` (128 padded bytes, no relocations);
- the existing `plane3d_negate` inline COMDAT; and
- exact in-TU `_collision_model_get_material_type`.

All owners and complete structure layouts already exist in the production
include closure. The lane therefore added no fabricated declaration, raw data
view, address, or offset.

## Frozen typed candidate

The candidate used a `boolean found`, initialized the output type to `NONE` and
fraction to `REAL_MAX`, and entered only for an object collision. Inside that
block it declared, in natural donor order, a reverse origin, reverse vector,
typed object index, `collision_model_instance`, and
`collision_model_test_vector_result`. It built the reverse ray with named
`x/y/z` and `i/j/k` fields, called the two collision-model helpers, transformed
and conditionally negated the plane, obtained the material through
`collision_model_get_material_type`, copied every named result field, and used
the existing inline `point_from_line3d` on success.

The public prototype used the existing `collision_result`, `real_point3d`, and
`real_vector3d` types. The body was readable C89 with declarations at block
starts, one parameter per line, and an explicit return. It used no assembly,
`volatile`, `register`, pragma, intrinsic, attribute, forced inline, optimizer
barrier, raw address or byte offset, pointer/integer reconstruction, cast or
union pun, inactive-union access, undefined behavior, synthetic anchor, byte
forcing, object patch, compiler/flag change, or comparator exception. A Ninja
dry run exposed one production edge; exactly that edge emitted the candidate
once.

## Strict measurement

| Measurement | January | First shot |
| --- | ---: | ---: |
| Meaningful / padded bytes | 395 / 400 | 397 / 400 |
| Relocations | 6 | 6 |
| Normalized SHA-256 | `26cf5d43237cdfad7ff884fed4ca826816b4ad726b2bb3f2800528f309d9df17` | `56150b1ff3800f1d377b643d85fa96c01e8fea1881e5c741bbd44834e5fe8e96` |

The preserved ignored first-shot object is
`build/audit/collisions_vector_exit_first_shot_20260829/collisions.obj`, raw
SHA-256
`a629d2d0604da687e8cbe3399024cb4984315fc6b7da28a7d1b86065fed266ab`.
The candidate source/header Git blobs were
`3e17cb2e952f2c90ce1af6d2af019623ffd126da` and
`c0ce5006ba19314056539c810bdbe7026e2ea174`; their byte SHA-256 values were
`677312b2a6faafe87aa964993ad813aaf8541263d3c7997e1f09e5169fbaec31`
and `4a124d539676108d5f9c46ee77eb4591fa9c93893ed9eaf035ad7c5e7068a12f`.

The ordered relocation comparison is:

| Symbolic destination | January | First shot |
| --- | ---: | ---: |
| `_collision_model_instance_new` | `+0x69` | `+0x69` |
| `_collision_model_test_vector` | `+0x8E` | `+0x8E` |
| `__real@3f800000` | `+0xA6` | `+0x9F` |
| `_matrix4x3_transform_plane` | `+0xCB` | `+0xCD` |
| `_plane3d_negate` | `+0xE2` | `+0xE4` |
| `_collision_model_get_material_type` | `+0xF5` | `+0xF7` |

The first branch displacement differs because the candidate's success path is
two bytes longer. The first substantive instruction-order boundary is at
`+0x2E`: January saves EDI and then loads the vector pointer before loading the
previous collision's object index at `+0x34`; the candidate loads that object
index at `+0x2E`, then saves EDI and loads the vector pointer. The candidate's
block-start initialized declaration made the object-index value available
earlier. Later, January sign-extends the hit node before loading the `1.0f`
constant, while the candidate loads the constant first. Those choices shift
the four later relocations and the success-field register schedule.

HCEA's C99 donor declares and initializes the object index after constructing
both reverse vectors. A C89 declaration without initialization followed by a
later assignment is therefore a plausible evidence-backed reopening direction,
but it was not compiled: doing so after measurement would be a second source
candidate. Do not repeat the block-start initialized-local form or sweep
assignment/declaration/register spellings. Reopen only as a separately
authorized one-shot supported by original-source or same-compiler provenance.

## Whole-TU boundary and restoration

The candidate manifest also reported the expected external-section ownership
transition: all eight inherited accepted full fingerprints, five runtime
`.rdata` owners, and the symbol set moved when the new external COMDAT was
inserted. `_collision_test_vector_exit` remained changed/nonexact and no newly
exact owner appeared. No adjudication or ownership exception was added.

After inverse restoration, `source/physics/collisions.c` and `.h` are again Git
blobs `f004d17a1ff024dbcea70ab26c1abcf3dafde42f` and
`15e26b230b092d5f5a2d36559a3f4f0ed9a03988`. The frozen manifest reports all
eight inherited accepted functions `still_exact`, `changed_nonexact: []`, no
data failure, and no warning. The immutable January object SHA-256 is
`2d80a02a21bf86d54a4dce4e3309759f1f167936e3a761c978f586a3ab6f9ab8`.
The restored source/header byte SHA-256 values are
`1f7edc9c0c887e8b15977c374a6263abd6441da6153a89dc8dbadea3bc7f7ef3`
and `887e0ad04577191bb3ddc39dad7cfc10f263deac39295cdcc954faf2ac6d67c2`;
the phase-specific restored object SHA-256 is
`0fc22d0d7417fab2ae8572abd38ef2c87b3af3b5ec9d2d65d7d45eb6ce2c5a56`.

## Final gates

- Full `halobetacache_build`, `libcmt_build`, `semantic_progress`, and
  `progress`: pass.
- Strict Collisions gate: eight exact, zero residual, 12 unwritten; the target
  remains unwritten at 400 padded bytes.
- Strict board: 277/619 complete objects, 4,782/8,246 functions, and
  663,079/1,922,669 padded code bytes.
- Semantic audit: 470 units, 4,917 functions evaluated, 4,800 semantic exact,
  131 hidden exact / 78,940 hidden bytes, 4,810 accepted exact, and zero unit
  errors or ordinary rejections.
- Campaign progress: 384/833 objects, 4,771/11,060 functions,
  606,187/2,198,102 meaningful code bytes, and
  1,856,050/4,176,062 data bytes.
- Admission: zero candidates, contradictions, and revocations.
- Park validation: 12 active, zero stale, and zero invalid.
- Protected Units gate: 189/189 functions exact. The sentinel
  `_unit_preprocess_node_orientations` remains strict exact at 1,920 padded
  bytes, 87 relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- Complete tool suite: 212/212 passed; the sole warning is the sandbox's denied
  optional `.pytest_cache` creation.
- `build/report.json` SHA-256:
  `19402f6c01804c9d844537e23194000ab2a656cd1a76bc7d32f7c7768a3af302`.
- `build/semantic_report.json` SHA-256:
  `f1e19b7ef56c296d953cc7c7db7bba7f1f379444c230a4a4541bb3ba1b6391de`.
- Frozen restored regression check and `git diff --check`: pass.

No production candidate was retained and no push was performed.
