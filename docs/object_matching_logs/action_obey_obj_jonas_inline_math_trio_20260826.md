# `action_obey.obj`: inline math wrapper trio (2026-08-26)

## Outcome

Three ordinary inline math wrappers are newly strict-exact:

- `_vector_from_points2d`;
- `_cross_product3d`;
- `_negate_vector3d`.

The wave adds three exact functions and 122 meaningful code bytes.
`action_obey.obj` advances from 6/27 to 9/27 exact functions and from 317 to
439 matched meaningful code bytes. It remains incomplete; no completion
label, semantic exception, adjudication, data credit, or comparator waiver was
added.

Source commit:

- `ef95dd016320fed49b3945c7443513b33bd005fe`
  (`Match action obey inline math wrappers`).

## Frozen evidence

The untouched translation unit was frozen at commit
`f529d037bc66823234e5e7a48bfdda08563b1ae8` in:

- `build/audit/action_obey_inline_math_baseline_20260826.json`;
- manifest SHA-256
  `6b18cb311648b730e7264cd328aaffe96cc12e534c256011ab9174adb54dca3a`.

The target functions have these authenticated January fingerprints:

| Function | Meaningful / padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_vector_from_points2d` | 29 / 32 | 0 | `5c9ee61634eac376cfc65890719accfeb6e0f5db37c55ab93b4da88bcbfe4f80` |
| `_cross_product3d` | 60 / 64 | 0 | `660da515fee7a623e6230a78b71749517ee5ad376db4126e5c8fe79414d2623a` |
| `_negate_vector3d` | 33 / 48 | 0 | `3265a51c2e64caa7a94fc32386ad62d6a774c929993624a9e83f35fbc2f2afcd` |

This was a same-name, same-ABI transfer rather than a masked-shape guess.
Current rebuilt objects already emit each exact function from the canonical
inline bodies in `source/math/real_math.h`. Representative accepted donors
include:

- `vector_from_points2d`: `real_math.obj`, `collision_bsp.obj`, and
  `collision_features.obj`;
- `cross_product3d`: `actor_type_infection.obj`, `ai_debug.obj`,
  `players.obj`, `matrix_math.obj`, `random_math.obj`, `real_math.obj`,
  `objects.obj`, `bsp3d.obj`, and `collision_bsp.obj`;
- `negate_vector3d`: `players.obj`, `real_math.obj`, `models.obj`,
  `objects.obj`, and `render_sky.obj`.

The repository header independently authenticates the ordinary C bodies,
operand ordering, return types, and parameter types. All three targets have
zero relocations, so there is no unresolved owner or addend hypothesis.

## Candidate discipline

The three inline identifiers are locally macro-renamed across the include
block, then ordinary out-of-line definitions are appended after the already
accepted `_random_range` wrapper. Appending preserves every existing function
section index. The macros only avoid duplicate C definitions; they do not
change optimization, calling convention, or emitted instructions.

The one normal VC7 candidate compile was strict-exact on the first attempt.
There was no object permutation, compiler-flag change, allowlist,
adjudication, comparator exception, object patch, intrinsic, alignment
directive, synthetic anchor, or byte-forcing expression.

## Exact comparison and frozen gate

The hardened comparator reports `all_equal: true` for the three new wrappers
and all six previously accepted functions:

- `_vector_from_points2d`;
- `_cross_product3d`;
- `_negate_vector3d`;
- `_random_range`;
- `_action_obey_flush_command_indices`;
- `_action_obey_advance_command_list`;
- `_action_obey_begin`;
- `_action_obey_end`;
- `_action_obey_update`.

For each new wrapper, target and candidate agree on every padded byte,
normalized hash, zero-relocation inventory, and function-section ownership.
The frozen gate reports exactly the three wrappers as `newly_exact`, the six
accepted siblings as `still_exact`, and `changed_nonexact: []`. Its sole
expected structural finding is the additive symbol inventory from defining
the wrappers.

## Clean committed state and replay

The clean committed manifest is:

- `build/audit/action_obey_inline_math_committed_20260826.json`;
- commit `ef95dd016320fed49b3945c7443513b33bd005fe`;
- SHA-256
  `c00d5c50ed2e757ddbdd96bbca96764cdc85e5a885fdc8466e96751f8a4581ed`.

Its pre-rebuild and post-rebuild checks return `ok: true`, no failures, no
warnings, no changed nonexact functions, and all nine functions above as
`still_exact`.

The generated object path was resolved inside this worktree. That single file
was deleted and rebuilt once from the committed source. Raw base-object hashes
were:

- before replay:
  `318e7b5cf227a3b0f6ea4258074ce0b52b10201b90bbac8835900be0020945f9`;
- after replay:
  `fe1ece76d2544a0d01b58cdd49927a0d962c616b591441ca8c48b57393e5aa63`.

VC7 debug/build records are nondeterministic, so raw object hashes are not the
equality oracle. The post-replay committed manifest is green and the direct
hardened comparison remains `all_equal: true`.

Pinned artifacts:

- target split object:
  `0e4f889a35b97c44f6ae91e4906b547bd64f2ff12b9f399e02b46f02b69633fb`;
- source file:
  `dbcc7c6d642babfa08ec5a1090661fc9fe35d50bb2f415700dd27d89350785dd`;
- source blob before/after:
  `fed1a96e20b6e9e41353ef77085488e19929f948` /
  `770920ff65e3ecddff29ba1d83b12f4b189ce89a`;
- canonical report:
  `3bab75fe187a50dd5fe76f472abe144ab33e97bca3031f67d89162f59bbd3eec`;
- semantic report:
  `8b568709e2d795289d4c420b475f6c6e0a80646925d53145ed21703cd029ee60`;
- VC7 compiler:
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`;
- hardened comparator:
  `411e41698a9491a7b69430307d03f844ee3144633e5d3038d3ffde81a8eccfb7`;
- regression gate:
  `d09b46dd2331ff7ca677cad78a66c780c4effa41a5389f0c614c9760d7632a5f`;
- matching methodology:
  `3eb52f6b2df7b94b4d732161c0f86e2becebbcf0b546824deb957882118b1b5c`.

Supporting audits:

- admission SHA-256:
  `f0aa45c22cc604fb74abc388b06dd79a27e7a0b498a5d28837a9e785680c9a36`;
- parked-frontier SHA-256:
  `cfac03aea5be607c19a6a40bcdbc1164e71ffb0179602af4bd37b3fd50e26d1d`.

## Canonical progress and gates

After regeneration:

- canonical code: 508,669/2,198,102 bytes and 4,211/11,060 functions;
- complete files: 377/833;
- Halo category: 495,755/1,770,166 code bytes and 4,044/7,574 functions;
- Halo complete files: 275/468;
- semantic audit: 470 units and 4,319 functions evaluated, 4,176 semantic
  exact, 114 hidden-exact functions / 64,705 hidden bytes, 4,237 accepted
  exact, and zero unit errors.

Repository gates:

- object admission: zero candidates and zero revocations; only the inherited
  `shell_xbox` completion-label contradiction remains;
- parked validation: three active, zero stale, zero invalid;
- tooling tests: 179/179 pass;
- Halo and libcmt dry graphs: no work;
- protected-path touches: zero;
- deleted tracked paths: zero;
- no push.

## House-rule audit

Every parameter is on its own line and each wrapper ends with an explicit
value return. The retained source is C89. It contains no assembly, `register`,
`volatile`, barrier, pragma, explicit intrinsic, attribute, raw address,
pointer/integer reconstruction, undefined behavior, synthetic anchor, or
byte-forcing form. These wrappers perform no tag or object access, so the
typed-access rules are not implicated.
