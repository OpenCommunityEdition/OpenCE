# `periodic_functions.obj` inline-math donor rejection — 2026-08-30

## Result

`source/math/periodic_functions.obj` remains **2/7 strict exact**.  One fresh,
same-compiler, ordinary-C donor premise was tested across the five residual
owners.  It did not change the compiler's math-lowering decision and produced
no exact owner, so the complete packet was inverse-restored.  No production
source, data, matching metadata, compiler flags, or ownership credit is
retained.

The restored gate is:

```text
residual    256  _code_000fa050  [size 304!=256, relocs 20!=18, sha]
residual    304  _code_000fa150  [size 288!=304, relocs 18!=25, sha]
residual    688  _code_000fa280  [size 864!=688, relocs 48!=46, sha]
residual    304  _periodic_function_evaluate  [size 368!=304, relocs 19!=20, sha]
residual    288  _transition_function_evaluate  [size 320!=288, relocs 16!=17, sha]
== exact 2  residual 5  unwritten 0  (of 7 listed)
```

The inherited exact initialize and dispose owners remain exact.

## Evidence and census

The campaign methodology, acceleration playbook, tooling README, repository
instructions, and the complete 2026-08-27 semantic-foundation ledger were read
before emission.  HaloCEA, Stian, Pastudan, punpckhdq, Claude, and Fable were
used only as hypothesis sources; January COFF remained authoritative.

The all-ref history contains four committed `periodic_functions.c` blobs and
no post-foundation implementation.  A filesystem census across the Codex
workspace and `C:/halo-worktrees` found **1,566 source copies with seven
physical SHA-256 hashes**.  The three dedicated historical periodic-function
trees preserve six target/candidate objects with four raw hashes.  None is a
strict completion, and no unmeasured exact artifact was found.  Current HCEA
still corroborates the retained waveform semantics and lexical case-local
topology, but it adds no January compiler-control evidence.

The fresh evidence was in newly available strict-exact owners compiled with the
same XDK 3911 VC7 flags:

- `_accelerate_to_velocity` uses a direct ordinary-C `fmod` expression and
  emits target-identical `__CIfmod`;
- `_linear_to_non_linear_alpha` uses an ordinary-C `pow` expression and emits
  target-identical `__CIpow`;
- `_sine` and `_cosine` use ordinary-C standard-library calls and emit
  target-identical x87 `fsin` and `fcos`.

This changed the previous premise that only forbidden explicit intrinsics or a
flag change could select January's math lowering.  It justified exactly one
new candidate packet.

## Sole candidate

The packet transferred only those donor-supported source surfaces:

- removed explicit argument/result casts from every `fmod` call;
- used the existing readable `power`, `sine`, and `cosine` real-math wrappers
  at the corresponding standard-library call sites;
- retained all target-attested constants, waveform mapping, control flow,
  data ownership, and function boundaries.

The packet was ordinary typed C89.  It added no asm, `volatile`, `register`,
pragma, explicit intrinsic, attribute, barrier, raw address, offset access,
type pun, undefined behavior, forced inline, byte shaping, or compiler/config
change.  XDK `/Zs` passed before the single code-producing compile.

Candidate object SHA-256:
`6ae7f3f230ca26f5c37266712c114d50602c77a76c81ba28e734db828a725380`.
Candidate source packet SHA-256:
`2270bf57a818e5550b435b4a8dd4a493cbe65b559830934695e41ffb21d36274`.

| Owner | January padded/relocs/hash | Candidate padded/relocs/hash |
| --- | --- | --- |
| `_code_000fa050` | 256/18/`802ef4da...` | 304/20/`624ceb20...` |
| `_code_000fa150` | 304/25/`60a30a46...` | 288/18/`5f0b8843...` |
| `_code_000fa280` | 688/46/`b77d7e7c...` | 848/48/`3511aa3f...` |
| `_periodic_function_evaluate` | 304/20/`eca198fa...` | 368/19/`4485d8dc...` |
| `_transition_function_evaluate` | 288/17/`a82ce883...` | 320/16/`2f09327b...` |

The wrappers were byte-neutral in four owners: VC7 still selected `_fmod`,
`_pow`, `_cos`, and `_sin`.  `_code_000fa280` shortened by 16 padded bytes only
because the implicit-promotion ternary removed a conversion/spill sequence; it
remained 160 bytes too large with two surplus relocations.  This is not a
partial owner gain.

## Restoration and boundary

The candidate was rejected immediately and inverse-restored.  The production
source is the exact inherited Git blob
`da606b820601e79a0af603446d90295402b49cb2`; the post-restore compile returns
to 2 exact / 5 residual / 0 unwritten.

Do not repeat standard-call versus `sine`/`cosine`/`power` wrappers, explicit
versus implicit promotions, or the HCEA case-local spelling solely to request
intrinsic lowering.  In this TU those ordinary-C forms are now measured.

Reopen only with January source/PDB compiler-control provenance, or a
same-compiler exact donor whose *surrounding function topology* (not merely the
call spelling) explains why VC7 chooses `__CIfmod`, `__CIpow`, `fcos`, and
`fsin` inside a loop/switch body without a pragma, intrinsic, or flag change.
