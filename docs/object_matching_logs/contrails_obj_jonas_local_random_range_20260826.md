# `contrails.obj`: local short random-range wrapper (2026-08-26)

## Outcome

`_local_random_range` is newly strict-exact. The wave adds one exact function
and 27 meaningful code bytes. `contrails.obj` now reports 7/19 exact functions
and 253 matched meaningful code bytes. It remains incomplete; no completion
label, semantic exception, adjudication, or data credit was added.

Source commit:

- `17c6332bec41d07e162616fcff368e22a64084ad`
  (`Match contrails local random range`).

## Frozen evidence and corrected donor boundary

The untouched translation unit was frozen at commit
`620f6be1c2664974eb0c95efae49f7678ea28176` in:

- `build/audit/contrails_local_random_range_baseline_20260826.json`;
- manifest SHA-256
  `88706eab2566c8ff8b7912f3d5ad294fb777a4efe0b96e6c733bd1cb2473737d`.

January's `_local_random_range` is 27 meaningful / 32 padded bytes, with
normalized SHA-256
`7ed2e50e8e1a701a98fcceeb6a6b67e05d997f21bdb9ff5ee2e848fd1a8f409a`.
It has exactly two `IMAGE_REL_I386_REL32` relocations:

- byte 12 to `_get_global_local_random_seed_address`;
- byte 18 to `_seed_random_range`.

`docs/shape_transfer_executor_strategy.md` correctly revoked an older proposed
transfer from `source/camera/bored_camera:_real_local_random_range`: that donor
is `float(float, float)`, while this recipient is `short(short, short)`. Equal
masked bytes alone did not prove ABI compatibility, and this wave does not use
the float donor as source evidence.

Commit `7bce0f67e990e5dd75e4392cbc9e6cfa4c32f6cf` subsequently supplied genuinely
new evidence: accepted exact donor `source/ai/action_obey:_random_range` has the
same `short(short, short)` ABI, the same 27/32-byte instruction shape, the same
normalized hash, and the same call at byte 18. Only the getter relocation at
byte 12 changes from the global seed to the local seed. The authenticated
inline definition in `source/math/real_math.h` independently fixes that exact
local-getter semantic body:

```c
return seed_random_range(
	get_global_local_random_seed_address(),
	lower_bound,
	upper_bound);
```

The compatible exact donor, January relocations, and canonical inline source
together satisfy the documented reopen condition. This is not a masked-shape
guess and does not weaken the transfer executor's ABI rejection.

## Candidate discipline

The inline identifier is renamed locally across the include block and the
ordinary out-of-line wrapper is defined after the six previously accepted
functions. The macro avoids a duplicate C definition; it does not alter the
optimization profile, calling convention, or emitted instructions.

The one normal VC7 candidate compile was strict-exact on the first attempt.
There was no object permutation, compiler-flag change, allowlist, adjudication,
comparator exception, object patch, raw intrinsic call, or byte-forcing
expression.

## Exact comparison and frozen gate

The hardened comparator reports `all_equal: true` for the new wrapper and all
six previously accepted functions:

- `_local_random_range`;
- `_contrail_scale_value`;
- `_contrails_initialize_for_new_map`;
- `_contrails_dispose_from_old_map`;
- `_contrails_dispose`;
- `_contrails_disconnect_from_structure_bsp`;
- `_contrails_initialize`.

For `_local_random_range`, target and candidate agree on all 32 padded bytes,
normalized hash, relocation count, both relocation addresses/types, and both
exact destinations. The pre-commit frozen gate reports the wrapper as
`newly_exact`, all six accepted siblings as `still_exact`, and
`changed_nonexact: []`. Its sole hard finding is the expected additive symbol
inventory from defining the wrapper.

## Clean committed state and replay

The clean committed manifest is:

- `build/audit/contrails_local_random_range_committed_20260826.json`;
- commit `17c6332bec41d07e162616fcff368e22a64084ad`;
- SHA-256
  `67114da0a6ff9c153d0f5b4510d81a8d2c8c8bececf64c1d7e7023034d12dfb3`.

Its pre-rebuild and post-rebuild checks return `ok: true`, no failures, no
warnings, no changed nonexact functions, and all seven functions above as
`still_exact`.

The generated base object was deleted and rebuilt once. Raw hashes were:

- before replay:
  `1aa3a7148bdab3aa8cdd6c708b62bca8df4972c7c584e128ceaccd8a4210e445`;
- after replay:
  `ad93533bfbd219bddaa615422f9ff9c2528b73201942bdfe6f3be9984fd99c44`.

VC7 debug/build records are nondeterministic, so raw object hashes are not the
equality oracle. The post-replay committed manifest is green and the direct
hardened comparison remains `all_equal: true`.

Pinned artifacts:

- target split object:
  `4285b4c7ede8ae9a36dce1b3bf6259f3ec839df869fdf465cd5ac9c3fb3b1955`;
- source file:
  `94bf2e22d85d614bc9ad410240ede1357c7427a4b294d772e2bb3396031c1638`;
- source blob before/after:
  `9457d3ec44edfa41886fdef765e84f178c7181e9` /
  `1a24f07e4abc1d04ba26a87e2b625a8a83067505`;
- canonical report:
  `6c975969a719da0d383c9618409748c8b409424d3be8b6390ada5966228f4c27`;
- semantic report:
  `7973f1397849691f510c6e2dce774349f447408565bb295960c1de0b7c13abc1`;
- objdiff-cli:
  `090987aa22c0fe9b7d252b2b44c2c0c92c5dd3e9b5965d353060802226a13677`;
- hardened comparator:
  `411e41698a9491a7b69430307d03f844ee3144633e5d3038d3ffde81a8eccfb7`;
- regression gate:
  `d09b46dd2331ff7ca677cad78a66c780c4effa41a5389f0c614c9760d7632a5f`;
- matching methodology:
  `3eb52f6b2df7b94b4d732161c0f86e2becebbcf0b546824deb957882118b1b5c`.

## Canonical progress and gates

After regeneration:

- canonical code: 508,547/2,198,102 bytes and 4,208/11,060 functions;
- complete files: 377/833;
- Halo category: 495,633 code bytes and 4,041/7,574 functions;
- semantic audit: 470 units and 4,316 functions evaluated, 4,173 semantic
  exact, 114 hidden-exact functions / 64,705 hidden bytes, 4,234 accepted exact,
  and zero unit errors.

Repository gates:

- object admission: zero candidates and zero revocations; only the inherited
  `shell_xbox` completion-label contradiction remains;
- parked validation: three active, zero stale, zero invalid;
- tooling tests: 179/179 pass;
- Halo and libcmt dry graphs: no work;
- protected-path touches: zero;
- deleted tracked paths: zero;
- added-source banned-construct matches: zero;
- no push.

## House-rule audit

Each parameter is on its own line and the function ends with an explicit value
return. The source is C89. The retained code contains no assembly, `register`,
`volatile`, barrier, pragma, explicit intrinsic, attribute, raw address,
pointer/integer reconstruction, undefined behavior, synthetic anchor, or
byte-forcing form. This wrapper performs no tag or object access, so the typed
access rules are not implicated.
