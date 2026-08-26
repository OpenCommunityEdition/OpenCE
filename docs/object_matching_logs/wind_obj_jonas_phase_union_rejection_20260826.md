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
