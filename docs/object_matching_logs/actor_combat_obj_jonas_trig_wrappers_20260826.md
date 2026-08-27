# `actor_combat.obj`: trigonometry wrapper trio (2026-08-26)

## Outcome

`_sine`, `_cosine`, and `_tangent` are newly strict-exact.  The wave adds
three exact functions and 32 meaningful code bytes.  `actor_combat.obj` now
reports 9/34 exact functions and 388 matched meaningful code bytes.  It remains
incomplete; no completion label, semantic exception, adjudication, or data
credit was added.

Source commit:

- `869b1182d1f6e79e68f0b89777f6cdbac736e825`
  (`Match actor combat trigonometry wrappers`)

## Frozen January evidence

The untouched translation unit was frozen at commit
`747ce08597b192ef20cc85511ecbf9080f699f9b` in:

- `build/audit/actor_combat_trig_baseline_20260826.json`;
- manifest SHA-256
  `1b6ecb7d5a1fe3af20320a81448d5e418e22d59fc0d1a36c72bde9f906cdeff8`.

The January functions have these meaningful/padded bytes:

```text
_sine     10/16  55 8B EC D9 45 08 D9 FE 5D C3 90 90 90 90 90 90
_cosine   10/16  55 8B EC D9 45 08 D9 FF 5D C3 90 90 90 90 90 90
_tangent  12/16  55 8B EC D9 45 08 D9 F2 DD D8 5D C3 90 90 90 90
```

None has a relocation.  The instruction streams are direct one-parameter
cdecl x87 wrappers: `fsin`, `fcos`, and `fptan` followed by the required pop of
the extra x87 value.  The authenticated inline definitions in
`source/math/real_math.h` independently specify `sin(angle)`, `cos(angle)`,
and `tan(angle)`.  These are source and ABI transfers, not byte-shape guesses.

The normalized SHA-256 values are:

- `_sine`: `27de3b5b74384f112d211e1de10bc593564ddce9b3a20ceacad4896c36351c33`;
- `_cosine`: `5bc95755d7aff66043725a1d5d6814017dd7de095dd63db1a7d7ccad88b8c6e0`;
- `_tangent`: `6e4d4c0cc5108ab1adcf627d5e3590271aed85c24f438fd2f4a1073d6865e42a`.

## Candidate discipline

The header inline identifiers are renamed locally across the translation
unit's include block, then ordinary out-of-line wrappers are defined after the
six previously accepted public functions.  The macros only avoid duplicate C
definitions; they do not alter optimization, calling convention, or emitted
instructions.

The one normal VC7 candidate compile was strict-exact for all three wrappers.
There was no emitted-object permutation, compiler-flag change, allowlist,
adjudication, comparator exception, object patch, raw intrinsic call, or
byte-forcing expression.

## Exact comparison and frozen gate

The hardened comparator reports `all_equal: true` for the new trio and all six
previously accepted functions:

- `_sine`;
- `_cosine`;
- `_tangent`;
- `_actor_combat_fire_wildly`;
- `_actor_combat_disable_bursts`;
- `_actor_firing_blindly`;
- `_actor_combat_currently_firing_burst`;
- `_actor_get_weapon_definition`;
- `_actor_combat_get_firing_variant_definition`.

For each new wrapper, target and candidate agree on all padded bytes, normalized
hash, relocation count, and the empty relocation inventory.  The pre-commit
frozen gate reports exactly the three intended `newly_exact` functions, all six
accepted siblings as `still_exact`, and `changed_nonexact: []`.  Its only hard
finding is the expected additive symbol inventory caused by defining the three
new functions.

## Clean committed state and replay

The clean committed manifest is:

- `build/audit/actor_combat_trig_committed_20260826.json`;
- commit `869b1182d1f6e79e68f0b89777f6cdbac736e825`;
- SHA-256
  `7c814cff0273b44b06263b9fcdce384dbe14c1f4a34e858ee20d2ec9e369f972`.

Its pre-rebuild and post-rebuild checks both return `ok: true`, no failures,
no warnings, no changed nonexact functions, and all nine functions above as
`still_exact`.

The generated actor-combat object path was resolved inside this worktree.  That
single generated file was deleted and rebuilt exactly once.  Raw base-object
hashes were:

- before replay:
  `f9e3afb9c8e2117034371c1a224b9d332bbdf5f09d0fcb4b86ac5bc224495706`;
- after replay:
  `3dfc0f27ed5d15298f32e14f1856a663770dbe9d4296000a4075e91db9d51ed5`.

VC7 debug/build records are nondeterministic, so raw object hashes are not the
equality oracle.  The post-replay committed manifest is green and the direct
hardened comparison remains `all_equal: true`.

Pinned artifacts:

- target split object:
  `ed7cf731b085a4a3422cdc5e87ecafb1033fce0a705666d03a3b2f81618423c6`;
- source file:
  `6cfb37624200a8d6195d630a13da91282f520347e60cc0d6a05588f46e652322`;
- source blob before/after:
  `3f17972fc28fb92b8069faef86ec581d328dd87c` /
  `53d4905fea21f46164367030e76e64b40db104d1`;
- canonical report:
  `be017a0de6dbf016f5a616d8f24ca50236982994ebe13f11167689aa1ca4df3e`;
- semantic report:
  `aae0d3ada8545e52127dc53c66b05a71372cc3a7d709b730408b15e57190dfe2`;
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
- code: 508,493/2,198,102 bytes and 4,206/11,060 functions;
- data: 1,836,762/4,176,062 bytes;
- Halo category: 4,039/7,574 functions;
- semantic audit: 470 units and 4,314 functions evaluated, 4,171 semantic
  exact, 114 hidden-exact functions / 64,705 hidden bytes, 4,232 accepted exact,
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

Every parameter is on its own line and every function ends with an explicit
return statement.  The source is C89.  It calls ordinary `sin`, `cos`, and
`tan`; VC7 selects the x87 instructions itself.  The retained code contains no
assembly, `register`, `volatile`, barrier, pragma, explicit intrinsic,
attribute, raw address, pointer/integer reconstruction, undefined behavior,
synthetic anchor, or byte-forcing form.  This wave performs no tag or object
access, so the typed-access rules are not implicated.
