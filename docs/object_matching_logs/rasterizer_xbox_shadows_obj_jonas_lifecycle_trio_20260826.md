# `rasterizer_xbox_shadows.obj`: lifecycle trio (2026-08-26)

## Outcome

Three January functions are newly strict-exact:

- `__rasterizer_environment_shadows_begin`
- `__rasterizer_environment_shadow_model_end`
- `__rasterizer_environment_shadows_end`

The wave adds three exact functions and 29 meaningful code bytes.  The object
is not complete, and no completion label, semantic exception, adjudication,
or data credit was added.

Source commit:

- `6697d7c398478c862af0d1dbea7b903d13b67e21`
  (`Match environment shadow lifecycle leaves`)

## Frozen evidence

The untouched baseline was captured at commit
`119e535a63ad9fe3ad8e2382c01859c0105f8fcf` in:

- `build/audit/rasterizer_shadows_baseline_20260826.json`
- SHA-256
  `199b9f38a0c747d6050422c57b86015f951e2a04bee7f3b6964229cc6970c7c4`

`shadows_begin` and `shadows_end` each have 9 meaningful / 16 padded
bytes and normalized SHA-256
`f27c91ab09dc5b6f75808b4ae85f5e00e23c249e0ad6466b48ef6c5bc216fe06`.
Their January instructions are:

```text
push 4
call rasterizer_profile_begin/end
pop ecx
ret
```

Each carries one `IMAGE_REL_I386_REL32` relocation at byte 3.  The begin leaf
targets `_rasterizer_profile_begin`; the end leaf targets
`_rasterizer_profile_end`.  The named
`_rasterizer_profile_environment_shadows` enum is independently defined as 4
in `source/rasterizer/rasterizer.h`.

`shadow_model_end` has 11 meaningful / 16 padded bytes, normalized SHA-256
`abe944925d4f3b974a0bd6e1ec5523804233ecf1d23afb41bf05be96ee3ddc59`,
and one `IMAGE_REL_I386_DIR32` relocation at byte 2 to the defined `.bss`
owner `_bss_0046628c` + 68.  Its body clears that dword and returns.

January's surrounding `shadow_model_begin` stores its model-begin parameter at
owner +68 and marks a following byte active.  The later HCEA body independently
names the same cached model-parameter pointer and clears it in model-end.  HCEA
begin/end are empty stubs and are rejected as build-divergent because January
contains the two explicit profile calls.

## Source ownership

The retained source models only the proven prefix:

- 68 explicitly unknown bytes;
- a typed `rasterizer_model_begin_parameters const *model` at offset 68.

The external owner is explicitly zero-initialized so VC7 emits a defined BSS
symbol rather than COMMON storage.  The candidate owner is 72 bytes.  January's
owner is 74 bytes and uses different section alignment flags.  The final two
bytes and complete alignment provenance remain unproven, so the BSS section is
deliberately left nonmatching.

The profile helpers use the source-local `void(short)` prototypes already
authenticated by `rasterizer_xbox.h` and by the exact dynavobgeom profile
wrappers.  C identifiers beginning with `_rasterizer` naturally produce the
target double-underscore COFF names.

## One-shot result

One `/Zs` parse-only check passed.  The single object-emitting VC7 candidate
then matched all three targets without a corrective compile.

The hardened comparator reports `all_equal: true`.  It proves, per function:

- exact padded bytes and normalized hash;
- exact relocation count, type, and address;
- exact profile helper destination, or exact defined BSS owner +68;
- exact symbol decoration and COMDAT ownership.

The pre-commit frozen gate reports the three functions `newly_exact`, no prior
exact sibling, and `changed_nonexact: []`.  Its expected failures are solely
the newly introduced partial BSS, debug records, and symbol inventory; no
allowlist or adjudication hides them.

## Clean-state and replay proof

The clean committed manifest is:

- `build/audit/rasterizer_shadows_lifecycle_committed_20260826.json`
- commit `6697d7c398478c862af0d1dbea7b903d13b67e21`
- SHA-256
  `99f55e6408704717050c25bcb318d77d95162e0065bb857f76eab9e967606e18`

Both its pre-rebuild and post-rebuild checks return `ok: true`, no failures,
no warnings, no changed nonexact functions, and all three functions
`still_exact`.

The generated shadow object was path-verified inside the worktree, deleted,
and rebuilt exactly once.  Replay base-object SHA-256:

- `80f86df1bddfdd1123b99248f83ee4d00e7bddbf881c2e2dced86299152fe2da`

Raw VC7 hashes include nondeterministic debug/build metadata; the clean
manifest and hardened section comparison are the equality oracles.

Pinned artifacts:

- target split object:
  `b21253ca6dcc619abd165c3696ae838ceeb53a1c3d94cf639d08605328c29c71`
- source file:
  `4528d435f901aaeede0de3abd1cefc6f4af48cb570d3855ad8ff253bdecf20a8`
- source blob before/after:
  `fcddc47f39447579382487209c0e59b78aacb3fa` /
  `3191e8735760f473856e5846c192931d63288115`
- report:
  `6373ad4241892ff29b458b6c2d45032184f4836136f93daa9f5bbe17efffd23a`
- semantic report:
  `bd01a82b34e899a2960d3e1536c36d9f5d99ae62bb342d7d3c2ad73e101b723d`
- VC7 compiler:
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`
- hardened comparator:
  `411e41698a9491a7b69430307d03f844ee3144633e5d3038d3ffde81a8eccfb7`
- regression gate:
  `d09b46dd2331ff7ca677cad78a66c780c4effa41a5389f0c614c9760d7632a5f`

## Canonical progress and gates

After regeneration:

- 470 units and 4,308 functions were evaluated;
- 4,165 functions were semantically exact;
- 114 hidden-exact functions account for 64,705 hidden code bytes;
- 4,226 functions were accepted exact;
- zero unit errors were reported;
- this object reports 3/18 exact functions and 29 matched meaningful bytes.

Repository gates:

- object admission: zero candidates and zero revocations; only the inherited
  `shell_xbox` contradiction remains;
- parked validation: three active, zero stale, zero invalid;
- tooling tests: 179/179 pass;
- Halo and libcmt dry graphs: no work;
- protected-path touches: zero;
- added-source banned-construct matches: zero.

## House-rule audit

Every no-argument list places `void` on its own line; each parameter has its
own line; every function ends with explicit `return;`.  The source is C89 and
uses a typed pointer with an explicit unknown span.  It contains no assembly,
`register`, `volatile`, barrier, pragma, intrinsic, attribute, raw address,
pointer/integer reconstruction, undefined behavior, synthetic anchor, or
byte-forcing expression.
