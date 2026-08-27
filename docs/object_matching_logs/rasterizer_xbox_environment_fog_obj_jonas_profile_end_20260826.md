# `rasterizer_xbox_environment_fog.obj`: profile-end leaf (2026-08-26)

## Outcome

`__rasterizer_environment_fog_end` is newly strict-exact.  The wave adds one
exact function and 9 meaningful code bytes.  The object now reports 4/27 exact
functions and 32 matched meaningful code bytes; it is not complete, and no
completion label, semantic exception, adjudication, or data credit was added.

Source commit:

- `22be28cb02e4e31b518ba986453393147fd2b63d`
  (`Match environment fog profile end`)

## Frozen January evidence

The untouched translation unit was frozen at commit
`c9b53b3a21161af4a6e59724f28bece3d14557b1` in:

- `build/audit/environment_fog_profile_end_baseline_20260826.json`
- manifest SHA-256
  `51ac16cd1e6d84c12658e4c87aac2ff4d546e7c1ecfb9daeab5a8ee76a584885`

The January target function has 9 meaningful / 16 padded bytes:

```text
6A 11 E8 00 00 00 00 59 C3 90 90 90 90 90 90 90
```

Its normalized SHA-256 is
`907d0f397dafd5d70dd91fc564c1f1f03bcf8705da718226ddb452d03d78f88b`.
The sole relocation is an `IMAGE_REL_I386_REL32` at byte 3 to the external
symbol `_rasterizer_profile_end`.  Thus the instructions are independently
fixed as:

```text
push 17
call rasterizer_profile_end
pop ecx
ret
```

The named rasterizer profile enumeration in
`source/rasterizer/rasterizer.h` independently places
`_rasterizer_profile_environment_fog` at value 17.  Existing exact profile
wrappers in the Xbox rasterizer translation units authenticate the local
`void(short)` prototype and VC7 calling shape.  The public rasterizer layer
also calls this private Xbox fog-end entry point.  The January object remains
the authority; cross-build and repository-history review supplied no stronger
contradictory body.

## Candidate discipline and exact comparison

After the evidence freeze, the source added the named enum constant, the
authenticated prototype, and one ordinary C89 wrapper.  Exactly one normal
object-emitting VC7 candidate compile was performed.  It matched on that first
compile; no corrective compile, source permutation, compiler flag change,
allowlist, exception, object patch, or byte-forcing expression was used.

The hardened comparator reports `all_equal: true` for the new leaf and the
three already accepted siblings:

- `__rasterizer_environment_fog_end`;
- `_rasterizer_environment_fog_screen_window_end`;
- `_rasterizer_environment_fog_screen_window_begin`;
- `_rasterizer_environment_fog_screen_model_end`.

For the new leaf, target and candidate agree on all 16 padded bytes, the
normalized hash, relocation count, relocation type and address, and the exact
external destination `_rasterizer_profile_end`.  The pre-commit frozen gate
reports only the new leaf as `newly_exact`, all three prior leaves as
`still_exact`, and `changed_nonexact: []`.  Its expected failures are solely
the new function's debug record and symbol inventory.

## Clean committed state and replay

The clean committed manifest is:

- `build/audit/environment_fog_profile_end_committed_20260826.json`
- commit `22be28cb02e4e31b518ba986453393147fd2b63d`
- SHA-256
  `a671f5c3aa76d60463c7f5595be271423d81c3b14e4fc865377b5972a05b3787`

Its pre-rebuild and post-rebuild checks both return `ok: true`, no failures,
no warnings, no changed nonexact functions, and all four functions above as
`still_exact`.

The generated object path was resolved and verified inside the worktree, then
that single file was deleted and rebuilt exactly once.  Raw base-object hashes
were:

- before replay:
  `ee5de58873be309f1b18017c408d1b4da190da71cfb5b5f755079bd1f98d3fc0`;
- after replay:
  `6d5b67bd8ed0226f7c239da1d5a2dd78a7d2eab0f786015125b985020f98665c`.

VC7 debug/build records are nondeterministic, so raw object hashes are not the
equality oracle.  The post-replay committed manifest is green and the direct
hardened comparison remains `all_equal: true`.

Pinned artifacts:

- target split object:
  `d12ea6f0ba18bc4e6e25921090f448b27adad35f5563477ea0cd64b6305e021c`;
- source file:
  `dd0042d2b46c033a75d37a66a850932effcd2dafaa085e741cf0353c39a77b3f`;
- source blob before/after:
  `41021e053b9d3aa43a4ab1e93eb90ac31bf714e9` /
  `c0ca51cd26839d2bdf0b3f99767e692b6a5e57fb`;
- canonical report:
  `c74ad97b23ccc39382b3fce7d7a55b581f6f84956111542217b3a04fdfc98dd6`;
- semantic report:
  `a364596048598147d0dc892201a4c7a58c42e77d14166fbbcdb76baf9ccc07fe`;
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
- code: 508,436/2,198,102 bytes and 4,201/11,060 functions;
- data: 1,836,762/4,176,062 bytes;
- Halo category: 4,034/7,574 functions;
- semantic audit: 470 units and 4,309 functions evaluated, 4,166 semantic
  exact, 114 hidden-exact functions / 64,705 hidden bytes, 4,227 accepted exact,
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

The no-argument list places `void` on its own line, the prototype parameter is
on its own line, and the void function ends with an explicit `return;`.  The
source is C89.  It contains no assembly, `register`, `volatile`, barrier,
pragma, intrinsic, attribute, raw address, pointer/integer reconstruction,
undefined behavior, synthetic anchor, or byte-forcing form.
