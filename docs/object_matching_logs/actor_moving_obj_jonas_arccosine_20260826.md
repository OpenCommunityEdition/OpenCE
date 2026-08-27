# `actor_moving.obj`: arccosine wrapper (2026-08-26)

## Outcome

`_arccosine` is newly strict-exact.  The wave adds one exact function and 12
meaningful code bytes.  `actor_moving.obj` now reports 4/36 exact functions and
141 matched meaningful code bytes.  It remains incomplete; no completion
label, semantic exception, adjudication, or data credit was added.

Source commit:

- `418b6aad052338bba4f8a40959a67974422f29e9`
  (`Match actor movement arccosine`)

## Frozen January evidence

The untouched translation unit was frozen at commit
`2cd168820d14bbe59730fa9538773b4aeffcdc1e` in:

- `build/audit/actor_moving_arccosine_baseline_20260826.json`
- manifest SHA-256
  `e68d540392935d4f5dd74cc8a9a17670bfa66cfa826b28c69f20cb63395e505a`

The January target has 12 meaningful / 16 padded bytes:

```text
55 8B EC D9 45 08 5D E9 00 00 00 00 90 90 90 90
```

Its normalized SHA-256 is
`bbe139825c1f5012bc08ffc81ed8a634feef80fe809ac3c6959eee118ef7edf5`.
The sole relocation is `IMAGE_REL_I386_REL32` at byte 8 to `__CIacos`.
The instructions are therefore fixed as a cdecl wrapper that loads its one
`real` parameter into x87 and tail-calls VC7's compiler-selected `acos`
entrypoint.

The accepted `source/camera/following_camera:_arcsine` donor has the same 16
padded instruction bytes, the same one-parameter `real` ABI, and the same
relocation position/type, with only the mathematically required destination
changed from `__CIasin` to `__CIacos`.  The authenticated inline definitions in
`source/math/real_math.h` independently state:

```c
return acos(x);
```

This is a source and ABI transfer, not a byte-shape guess.

## Source and candidate discipline

The header's inline `arccosine` identifier is renamed locally while the actor
headers are included, then the ordinary out-of-line wrapper is defined after
the three existing exact functions.  The macro only avoids a duplicate C body;
it does not change optimization, calling convention, or emitted instructions.

The first compile attempt stopped at C2084 before producing an object because
the rename was ended before `actors.h` reached `real_math.h`.  The rename scope
was corrected across all three include directives.  The first emitted VC7
candidate object then matched exactly.  There was no emitted-object source
permutation, compiler flag change, allowlist, adjudication, comparator
exception, object patch, raw intrinsic call, or byte-forcing expression.

## Exact comparison

The hardened comparator reports `all_equal: true` for the new wrapper and all
three accepted siblings:

- `_arccosine`;
- `_actor_move_keep_moving_past_destination`;
- `_actor_move_animation_busy`;
- `_actor_path_has_path`.

For `_arccosine`, target and candidate agree on all 16 padded bytes, normalized
hash, relocation count, relocation address/type, and exact destination
`__CIacos`.  The pre-commit frozen gate reports `_arccosine` as `newly_exact`,
the three siblings as `still_exact`, `changed_nonexact: []`, and only the
expected symbol-inventory change.

## Clean committed state and replay

The clean committed manifest is:

- `build/audit/actor_moving_arccosine_committed_20260826.json`
- commit `418b6aad052338bba4f8a40959a67974422f29e9`
- SHA-256
  `201277187b76bba756b4374dbceb15baede5586d9ac1d1d8b20dad0933c9dc97`

Its pre-rebuild and post-rebuild checks both return `ok: true`, no failures,
no warnings, no changed nonexact functions, and all four functions above as
`still_exact`.

The generated actor-movement object path was resolved and verified inside the
worktree.  That single generated file was deleted and rebuilt exactly once.
Raw base-object hashes were:

- before replay:
  `29b58fa57d22778482c0c82fa8958940de7a6df8906d2aeb379c8b082542f352`;
- after replay:
  `b8f8282c27ddd6b5ecc4ca3ef2d1085df5e82935946b6ca1d659ffb136044fc4`.

VC7 debug/build records are nondeterministic, so raw object hashes are not the
equality oracle.  The post-replay committed manifest is green and the direct
hardened comparison remains `all_equal: true`.

Pinned artifacts:

- target split object:
  `d693781be5186ab1f618cc7765826856e679f27a40fc94802686c338c60520fa`;
- source file:
  `95f75872aa7d0b5944dbc151c1d0149877e26ecb07ce4b3dcbc017dfa2589f5a`;
- source blob before/after:
  `6fe52f05e14e8eb304d39b0c8b0d298fa854987b` /
  `75e9d2e5b4797bc9aa9f12ad3f5ca371afb2d629`;
- canonical report:
  `0e404c72d4105426dcbea3fa3005607c326f783af9b6ba5e6f817c0b27ce4a5c`;
- semantic report:
  `6073cc662a6f47eaf77a6a32ba8fc23c8f4bb5f3878a564886684888eaf0d729`;
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
- code: 508,461/2,198,102 bytes and 4,203/11,060 functions;
- data: 1,836,762/4,176,062 bytes;
- Halo category: 4,036/7,574 functions;
- semantic audit: 470 units and 4,311 functions evaluated, 4,168 semantic
  exact, 114 hidden-exact functions / 64,705 hidden bytes, 4,229 accepted exact,
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

The single parameter is on its own line, and the function ends with an explicit
return statement.  The source is C89.  It calls ordinary `acos`; VC7 selects
`__CIacos` itself.  The retained code contains no assembly, `register`,
`volatile`, barrier, pragma, explicit intrinsic, attribute, raw address,
pointer/integer reconstruction, undefined behavior, synthetic anchor, or
byte-forcing form.
