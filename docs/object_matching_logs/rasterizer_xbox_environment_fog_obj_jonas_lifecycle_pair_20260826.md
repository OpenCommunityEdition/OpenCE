# `rasterizer_xbox_environment_fog.obj`: lifecycle pair (2026-08-26)

## Outcome

Two January functions are newly strict-exact:

- `_rasterizer_environment_fog_screen_window_begin`
- `_rasterizer_environment_fog_screen_model_end`

The existing `_rasterizer_environment_fog_screen_window_end` remains exact.
The wave adds 22 meaningful code bytes and two exact functions.  The object is
not complete and no completion label or semantic exception was added.

Source commit:

- `fe60b94cb4bcc6444637401ebac421480aa2cdef`
  (`Match fog screen lifecycle leaves`)

## January evidence

The untouched translation-unit baseline was captured at commit
`92cc247cd12061ba0330eeda5d85c2d5adb59bac` in:

- `build/audit/environment_fog_leaf_baseline_20260826.json`
- manifest SHA-256
  `e6fe0f8a034d0124b95cf72c2945e806a321aa324d971b38853621c9c9d2d2f4`

Both new targets have:

- meaningful size 11, padded size 16;
- bytes `C7 05 <DIR32 owner+addend> 00 00 00 00 C3`, followed by five NOPs;
- normalized SHA-256
  `abe944925d4f3b974a0bd6e1ec5523804233ecf1d23afb41bf05be96ee3ddc59`;
- one `IMAGE_REL_I386_DIR32` relocation at byte 2.

The destinations distinguish the two otherwise identical shapes:

| Function | Exact destination |
|---|---|
| `window_begin` | defined `.bss` owner `_bss_00465ad0` + 596 |
| `model_end` | defined `.bss` owner `_bss_00465ad0` + 644 |

The existing `window_end` is a one-byte `RET` in a 16-byte padded COMDAT, with
zero relocations and normalized SHA-256
`499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca`.

## Layout proof

January's surrounding code establishes the field meanings rather than merely
their offsets:

- screen initialization allocates `0x5000` bytes and stores its pointer at
  owner + 592, then clears owner + 596;
- model submission reads owner + 596, checks it against `0x80`, increments it,
  and indexes 160-byte submission records;
- model begin stores its model pointer at owner + 644;
- model submission reads that pointer and dereferences model fields;
- model end clears owner + 644.

The retained source therefore models the proven prefix as a typed structure:

- `opaque_model_submit_parameters` at 592;
- `opaque_model_count` at 596;
- an explicit unknown span from 600 through 643;
- `model` at 644.

The external owner is explicitly zero-initialized.  This is required for VC7
to emit a defined `.bss` owner rather than a COMMON symbol, following the
documented `objects.obj` ownership precedent.

The recovered prefix is 648 bytes.  January's complete BSS owner is 661 bytes.
The missing 13-byte tail is not guessed, and no data-matching credit is
claimed.  External type sources remain hypotheses under the methodology gate.

## Cross-build and history review

The later HCEA functions are bare no-op stubs.  They contradict January's
explicit stores and are rejected as build-divergent.  Aerocatia/demon history,
the local punpckhdq snapshot, repository ledgers, and relevant Claude/AGENTS
tooling guidance supplied no stronger January body than the object evidence.

The fresh transfer atlas grouped these leaves by normalized instruction bytes
with unrelated one-store donors.  Relocation address, destination, addend, and
surrounding use—not the shape group alone—proved the semantics.

## Candidate chronology

1. The first candidate emitted both exact function bodies but left the owner
   as an undefined COMMON symbol.
2. The object exposed that ownership defect.  Explicit zero initialization was
   then applied as a documented source-level ownership correction.
3. The second candidate produced exact defined-BSS relocation identities for
   both functions.
4. Placement was reviewed against the frozen gate.  Adding the defined BSS
   necessarily renumbered the prior COMDAT/debug records; moving the new
   function could not remove that bookkeeping churn.  Direct hardened
   comparison proved `window_end` unchanged, and `changed_nonexact` remained
   empty.

No compiler flag, semantic allowlist, adjudication, comparator exception, or
object patch was used.

## Exact comparison

The hardened comparator reports `all_equal: true` for all three functions.
For each new function, target and candidate agree on:

- 16 padded bytes;
- normalized hash;
- relocation count, type, and address;
- defined `.bss` owner `_bss_00465ad0`;
- owner addend (596 or 644).

The clean committed manifest is:

- `build/audit/environment_fog_lifecycle_committed_20260826.json`
- commit `fe60b94cb4bcc6444637401ebac421480aa2cdef`
- SHA-256
  `4a45321309e462fbee6eb053f9cf3d28ef5687d067dc3c034c392470804fd061`

Its pre-rebuild and post-rebuild checks both return `ok: true`, no failures,
no warnings, no changed nonexact functions, and these `still_exact` functions:

- `_rasterizer_environment_fog_screen_model_end`
- `_rasterizer_environment_fog_screen_window_begin`
- `_rasterizer_environment_fog_screen_window_end`

The generated object was path-verified inside the worktree, deleted, and
rebuilt exactly once.  The replay base-object SHA-256 is
`8d7295627cb1c9de42f563aa6eb0ed65b263d8e96d26092f5d63d3f40f1874d2`.
Raw VC7 object hashes are not used as the semantic equality oracle because
debug/build metadata is nondeterministic; the clean manifest and hardened
section comparison remain green after replay.

Additional pinned hashes:

- target split object:
  `d12ea6f0ba18bc4e6e25921090f448b27adad35f5563477ea0cd64b6305e021c`
- source file:
  `0b3f06f40e8de70521326827a0495496d65467ea0cc8b4e715ac5c279b405a08`
- source blob before/after:
  `1aae2d5ef450fef4c5271d8b44d6f6736c406e10` /
  `41021e053b9d3aa43a4ab1e93eb90ac31bf714e9`
- report:
  `497d3cd16d551537abe464def921a35a16ea15fd3316005b6c61b81a8f13d86b`
- semantic report:
  `f02fd6aff9e543229d7dfd4ae1a7c451b0369fee49fdef0cdcb01a1178e355c0`
- VC7 compiler:
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`
- hardened comparator:
  `411e41698a9491a7b69430307d03f844ee3144633e5d3038d3ffde81a8eccfb7`
- regression gate:
  `d09b46dd2331ff7ca677cad78a66c780c4effa41a5389f0c614c9760d7632a5f`
- methodology:
  `3eb52f6b2df7b94b4d732161c0f86e2becebbcf0b546824deb957882118b1b5c`

## Canonical progress and gates

After regeneration:

- 470 units and 4,305 functions were evaluated;
- 4,162 functions were semantically exact;
- 114 hidden-exact functions account for 64,705 hidden code bytes;
- 4,223 functions were accepted exact;
- zero unit errors were reported;
- this object reports 3/27 exact functions and 23 matched meaningful bytes.

Repository gates:

- object admission: zero candidates, zero revocations, one inherited
  `shell_xbox` contradiction;
- parked validation: three active, zero stale, zero invalid;
- tooling tests: 179/179 pass;
- Halo and libcmt dry graphs: no work;
- protected-path touches: zero;
- added-source banned-construct matches: zero;
- worktree clean after the source commit.

## House-rule audit

The added functions use `void` on its own line, one parameter per line, and an
explicit final `return;`.  The source is C89.  It uses typed aggregate fields
and explicit unknown spans.  It contains no assembly, `register`, `volatile`,
barrier, pragma, intrinsic, attribute, raw address, pointer/integer
reconstruction, undefined behavior, synthetic anchor, or byte-forcing form.
