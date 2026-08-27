# `action_obey.obj`: random-range wrapper (2026-08-26)

## Outcome

`_random_range` is newly strict-exact.  The wave adds one exact function and
27 meaningful code bytes.  `action_obey.obj` now reports 6/27 exact functions
and 317 matched meaningful code bytes.  It remains incomplete; no completion
label, semantic exception, adjudication, or data credit was added.

Source commit:

- `7bce0f67e990e5dd75e4392cbc9e6cfa4c32f6cf`
  (`Match action obey random range`)

## Frozen evidence and donors

The untouched translation unit was frozen at commit
`3f2e6a76cb9cf575954a57e1bf3813cd312b0025` in:

- `build/audit/action_obey_random_range_baseline_20260826.json`;
- manifest SHA-256
  `cc41ccbacdd5ab85bced427218def879a69dab6df5cc8f13fe9a051070f670de`.

January's `_random_range` is 27 meaningful / 32 padded bytes, with normalized
SHA-256
`7ed2e50e8e1a701a98fcceeb6a6b67e05d997f21bdb9ff5ee2e848fd1a8f409a`.
It has exactly two `IMAGE_REL_I386_REL32` relocations:

- byte 12 to `_get_global_random_seed_address`;
- byte 18 to `_seed_random_range`.

The repository shape atlas independently finds two exact 32-byte, two-call
donors with identical relocation-masked instruction bytes:

- `source/ai/action_alert:_real_random_range`;
- `source/camera/bored_camera:_real_local_random_range`.

The authenticated inline definition in `source/math/real_math.h` fixes the
recipient's short-returning semantics and exact destinations:

```c
return seed_random_range(
	get_global_random_seed_address(),
	lower_bound,
	upper_bound);
```

Thus the transfer is jointly proven by January's bytes and relocations, two
accepted ABI-shape donors, and the canonical repository source body.  It is
not a masked-byte guess.

## Candidate discipline

The header inline identifier is renamed locally across the include block, then
the ordinary out-of-line wrapper is defined after all five previously accepted
functions.  The macro only avoids a duplicate C definition; it does not alter
optimization, calling convention, or emitted instructions.

The one normal VC7 candidate compile was strict-exact.  There was no
emitted-object permutation, compiler-flag change, allowlist, adjudication,
comparator exception, object patch, raw intrinsic call, or byte-forcing
expression.

## Exact comparison and frozen gate

The hardened comparator reports `all_equal: true` for the new wrapper and all
five previously accepted functions:

- `_random_range`;
- `_action_obey_flush_command_indices`;
- `_action_obey_advance_command_list`;
- `_action_obey_begin`;
- `_action_obey_end`;
- `_action_obey_update`.

For `_random_range`, target and candidate agree on all 32 padded bytes,
normalized hash, relocation count, both relocation addresses/types, and both
exact destinations.  The pre-commit frozen gate reports `_random_range` as
`newly_exact`, all five accepted siblings as `still_exact`, and
`changed_nonexact: []`.  Its sole hard finding is the expected additive symbol
inventory caused by defining the new function.

## Clean committed state and replay

The clean committed manifest is:

- `build/audit/action_obey_random_range_committed_20260826.json`;
- commit `7bce0f67e990e5dd75e4392cbc9e6cfa4c32f6cf`;
- SHA-256
  `506b7f5ff0129697a6a59056c2bf4783c36f9ccce32de4ea2346b4fa3fa60ed0`.

Its pre-rebuild and post-rebuild checks both return `ok: true`, no failures,
no warnings, no changed nonexact functions, and all six functions above as
`still_exact`.

The generated object path was resolved inside this worktree.  That single
generated file was deleted and rebuilt exactly once.  Raw base-object hashes
were:

- before replay:
  `8d320430f41df2e30729f3f419bca7c16091972b06fef62273d4b6c4bfb630c0`;
- after replay:
  `09f54824c750107064118aabebf38a202c03c236cd83ba5e423c4816a4fffdf3`.

VC7 debug/build records are nondeterministic, so raw object hashes are not the
equality oracle.  The post-replay committed manifest is green and the direct
hardened comparison remains `all_equal: true`.

Pinned artifacts:

- target split object:
  `0e4f889a35b97c44f6ae91e4906b547bd64f2ff12b9f399e02b46f02b69633fb`;
- source file:
  `2794ea8b908210ea9bd5a94693ede143c95db1850bac1803ddb33f8269148f83`;
- source blob before/after:
  `e0bb4361e9e4a4bf353e5cbf58037fee9b9e0169` /
  `fed1a96e20b6e9e41353ef77085488e19929f948`;
- canonical report:
  `28e97215980d2cb6afb13e04cffda0b6ed2d893155847215138c2925250ea822`;
- semantic report:
  `4e06bd4620272dbf7d9eebcf36e682271599afae808d7536152cc834031dbc7a`;
- VC7 compiler:
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`;
- hardened comparator:
  `411e41698a9491a7b69430307d03f844ee3144633e5d3038d3ffde81a8eccfb7`;
- regression gate:
  `d09b46dd2331ff7ca677cad78a66c780c4effa41a5389f0c614c9760d7632a5f`;
- matching methodology:
  `3eb52f6b2df7b94b4d732161c0f86e2becebbcf0b546824deb957882118b1b5c`.

## Canonical progress and gates

After regeneration:

- all categories: 23.13% matched and 14.21% linked, 377/833 complete files;
- code: 508,520/2,198,102 bytes and 4,207/11,060 functions;
- data: 1,836,762/4,176,062 bytes;
- Halo category: 4,040/7,574 functions;
- semantic audit: 470 units and 4,315 functions evaluated, 4,172 semantic
  exact, 114 hidden-exact functions / 64,705 hidden bytes, 4,233 accepted exact,
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

Each parameter is on its own line and the function ends with an explicit
return statement.  The source is C89.  The retained code contains no assembly,
`register`, `volatile`, barrier, pragma, explicit intrinsic, attribute, raw
address, pointer/integer reconstruction, undefined behavior, synthetic anchor,
or byte-forcing form.  This wrapper performs no tag or object access, so the
typed-access rules are not implicated.
