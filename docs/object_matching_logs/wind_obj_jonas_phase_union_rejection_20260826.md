# `wind.obj` phase-storage lifetime rejection

## Scope and result

This evidence wave re-opened only the omitted `_code_0017fbd0` wind-variance
initializer from cumulative commit
`30658aed84c48b4c2bbd2e80837991339f522e85`. The existing strict 7/9 partial
and the older Claude ledger were read first and were not edited.

The new hypothesis was rejected. No production source or matching status is
changed, and `source/scenario/wind.c` is restored to the cumulative baseline.

## New evidence and hypothesis

January disassembly proves that the target uses a `0x38`-byte frame and reuses
the first seed phase's eight-keyframe down-counter slot as the second spline
phase's advancing result pointer. The prior natural-loop candidate documented
by Claude used a `0x3C`-byte frame. This wave represented the proven phase
overlap with an ordinary C union:

- `long seed_keyframes_remaining` was used only during the seed phase;
- `real_vector3d *spline_result` was used only during the spline phase;
- no inactive union member was read;
- the HCEA three-loop spline topology and argument identities were preserved.

The candidate used C89 and no assembly, volatile shaping, pragma, barrier,
raw address, pointer/integer conversion, undefined aliasing, or byte forcing.

## Frozen one-shot measurement

The initial Ninja attempts did not start `CL.Exe`: the isolated worktree first
lacked `build.ninja`, then lacked the ignored `xbox/bin` toolchain directory.
After reproducing the build graph and attaching the pinned cumulative XDK 3911
directory, exactly one compiler invocation ran and its first object was
preserved.

| Artifact | SHA-256 |
| --- | --- |
| January target `wind.obj` | `0CE158A0182F548B70DF0627B7267447AB82861C87EF5513B9CC7F5CF73396A8` |
| cumulative baseline `wind.obj` | `916CE3983B6EF7BCB5EC89F8D654380268931DC83C9F2A73EDA4A0F79E874F46` |
| cumulative baseline `wind.c` | `1D6B91E4E865DB87D80C7F905B07CE14DD26CE0FD4F9F5B3620CA7C0B6B9DCA7` |
| Wave 1 source | `0B949E5088E0660F92CDE230966CE8CB9EC335F92E11CC8C98F566D8C6300C2F` |
| Wave 1 object | `71700994D346D9A464958B150E9ECDED4B7E80B4242FE0D53B23E06C0F47D73D` |

For `_code_0017fbd0`, target and candidate were both 384 padded bytes with
9 relocations, but relocation identities/order and normalized bytes differed:

- target normalized SHA-256:
  `1d5cd879825797c62a8f525b3f47ac10ee86b61f82da861f5ae6e6db4afa41ff`;
- candidate normalized SHA-256:
  `f556981bd031d37b3fd5a179fcea6f37808c30a70c441cd487ad09cb7a0e01fb`.

The union did not induce the desired overlap. The candidate still used a
`0x3C` frame and diverged in the seed phase: it retained the keyframe index in
`EBX`, recomputed the variance address, and placed the first `_wind_globals`
relocation at `0x1D` rather than January's pointer-induction relocation at
`0x0A`. The hardened blocker classifier returned `UNKNOWN`; it granted no
credit.

The complete frozen artifacts, disassembly evidence, classifier output, and
pre-wave regression manifest are in
`outputs/wind_closeout_evidence_20260826` outside the worktree.

## Do not repeat / reopen

Do not retry an explicit phase union or another spelling that retains the
seed keyframe index and recomputes its address. Reopen `_code_0017fbd0` only
with a source-backed topology that explains January's initial
`EBX = &wind_globals.variance[0][0]` pointer induction and the later stack-slot
overlap in the same compile.

The separate `_code_0017f750` residual remains behind the already documented
defined-C representation boundary. The old inactive-union type-pun is not an
admissible closeout technique.

## Seed-pointer and spline-local follow-up (2026-08-26)

This follow-up used the archived Wave 1 source/object, the January object, and
the later HCEA source at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`. HCEA independently preserves the
same two-phase algorithm: eight seeded samples on each of three axes, followed
by seven cyclic cubic-spline samples between each pair of seeds.

### Wave 2: loop-carried seed pointer

January begins with `EBX = &wind_globals.variance[0][0]`, copies that pointer to
`ESI` for each axis sweep, advances `ESI` by `0x300` per axis, and advances
`EBX` by `0x60` per keyframe. The rejected phase-union candidate instead kept
an integer keyframe in `EBX` and recomputed the destination address. Wave 2
changed only this seed phase to typed pointer induction:

- a loop-carried `real_vector3d *seed_result` advances by eight samples;
- a scoped `real_vector3d *axis_result` advances by 64 samples;
- the existing eight- and three-iteration down-counters are retained;
- the spline phase is byte-for-byte the prior Wave 1 source.

The candidate object is preserved as
`build/audit/wind_seed_pointer_candidate_20260826.obj`, SHA-256
`0162B29C40F77C6366058258355E3F1444F5508DE1CB9E70406672E5637C4477`.
`_code_0017fbd0` remains `384/384` padded bytes and `9/9` relocations, with
candidate normalized SHA-256
`2d347230e014cb89311ff20da2b7d20c36ff6698c0933a5abb5db08ce85d15ad`
versus target
`1d5cd879825797c62a8f525b3f47ac10ee86b61f82da861f5ae6e6db4afa41ff`.

The seed topology is confirmed. The first three relocation identities and
addresses are now exact (`0x0A`, `0x22`, `0x28`), and the seed loop from
`mov ebx,4` through its backward branch has the target instruction order and
register roles. The remaining prefix bytes are only the downstream-frame
effects: target uses a `0x38` frame and seed-counter slot `-0x10`, while this
candidate still uses `0x3C` and `-0x14` because its untouched spline phase
retains one extra dword lifetime.

### Wave 3: explicit target-named spline locals

With the seed topology frozen, Wave 3 replaced only the spline phase with
explicit long-lived keyframe, next-keyframe, cyclic-index, result-pointer,
substep, axis, and down-counter locals inferred from the January stack map.
This was rejected. It over-described the value graph, expanding the frame to
`0x44` and the function to `416/384` padded bytes. Relocations stayed `9/9`,
but the candidate normalized SHA-256 was
`d83ac4f474154dbb767ddc060be707f8798e5290de557c5172799457dd86d7d1`.
The complete object is preserved as
`build/audit/wind_target_shaped_spline_candidate_20260826.obj`, SHA-256
`DD05700DE1B18AECA9C9E1D7865120FA9DC920171D3D2FA9B7EDFB226D62E0DC`.

Do not repeat explicit one-local-per-target-stack-slot reconstruction. The
next legal reopen must keep the Wave 2 seed pointer topology and explain the
single extra spline dword through a *smaller* source value graph or a proven
phase-local overlap. Production `wind.c` and `wind.obj` were restored exactly
to the cumulative pre-wave state; no function credit or matching-status change
is claimed.
