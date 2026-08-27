# `periodic_functions.obj`: target-attested waveform foundation (2026-08-27)

## Outcome

`source/math/periodic_functions.obj` remains honestly `NonMatching` at 2/7
strict-exact functions. The retained wave adds no exact-byte, semantic-manifest,
parked, admission, or completion credit. It does preserve both inherited exact
functions and raises objdiff fuzzy similarity from 47.24609% to 51.66195% while
correcting target-attested waveform behavior and constants.

The baseline and retained objects are:

- `build/audit/periodic_functions_baseline_20260827.obj`, raw SHA-256
  `d74bdde52c51fa69a3e38c1d1d629eaa3004e7150360cfbc47a2f70b2915e54`;
- `build/audit/periodic_functions_semantic_foundation_20260827.obj`, raw
  SHA-256
  `260c45dae6dd2fa6b78b446d8f4cc4b4d4001d5f57e823d39ada05836afc2253`.

The source moves from Git blob
`9c0932caa7be4f0841cd43a7c363dd0291588e3f` to
`da606b820601e79a0af603446d90295402b49cb2`.

## Evidence

January build 2342 is authoritative. Its split object has raw SHA-256
`ef0d4254184e2305d43891dd5cf3a521e592588debcefd3c2fe3675ab001f154`.
The target switch table and disassembly prove that periodic function types 4
and 5 take the triangularizing branch after `fmod`, while types 6 and 7 return
the sawtooth remainder directly. The previous C source had those behaviors
reversed. The target also independently fixes the following single-precision
constants:

- table phase step: `0.027343748f` (`0x3cdfffff`);
- variable-period cosine: `0.044792242f` (`0x3d377812`);
- low-frequency cosine: `0.025157286f` (`0x3cce16a7`);
- wander coefficient: `0.89759791f` (`0x3f65c8fa`).

The public HCEA reconstruction corroborates the same function-type mapping,
constants, table normalization rule, and evaluator behavior:

- <https://github.com/surreptitiousresearch/halocea/blob/main/src/blam/math/periodic_function_build_table.c>;
- <https://github.com/surreptitiousresearch/halocea/blob/main/src/blam/math/periodic_function_build_variable_period_x_table.c>;
- <https://github.com/surreptitiousresearch/halocea/blob/main/src/blam/math/periodic_function_evaluate.c>;
- <https://github.com/surreptitiousresearch/halocea/blob/main/src/blam/math/transition_function_evaluate.c>.

The independently reconstructed build-2276 source in
`research-cache/stian-halo-current-20260827` corroborates the evaluator control
flow, table-pointer lifetime, function mask, interpolation ordering, and
initialization fallback. Its `CLAUDE.md`, `AGENTS.md`, and associated tooling
documentation were used as donor guidance only: preserve C89, ABI, field
widths, control-flow shape, and explicit unknowns; measure the rebuilt object;
and do not treat behavior-oriented scoring as byte-match acceptance.

The user-supplied Baboon, demon, nimbus, halopc-restored, HaloCEA,
`punpckhdq/halo`, and `pastudan/halo` public trees were queried at their current
default branches. Only HaloCEA exposed complete periodic-function bodies.
`pastudan/halo` supplied an Xbox table-golden harness, which corroborates the
12-periodic/6-transition table layout but is not byte-match evidence:
<https://github.com/pastudan/halo/blob/main/tools/verify/test_periodic_tables.py>.

## Strict comparison and fixed point

| Function | Target bytes/relocs | Baseline bytes/relocs | Retained bytes/relocs | Strict |
| --- | ---: | ---: | ---: | --- |
| `_periodic_functions_dispose` | 112/8 | 112/8 | 112/8 | exact |
| `_periodic_function_evaluate` | 304/20 | 336/18 | 368/19 | non-exact |
| `_transition_function_evaluate` | 288/17 | 336/17 | 320/16 | non-exact |
| `_code_000fa050` | 256/18 | 304/20 | 304/20 | non-exact |
| `_code_000fa150` | 304/25 | 288/18 | 288/18 | non-exact |
| `_code_000fa280` | 688/46 | 848/47 | 864/48 | non-exact |
| `_periodic_functions_initialize` | 192/17 | 192/17 | 192/17 | exact |

The exact functions retain normalized SHA-256 values
`09f94698f1709bc4480b5131f6d69ee770b376da2f558dea97876e2f1c6c2651`
and
`dd0e4caee119b1cdbd7398ed568e471d96ea1a5048f82105140b81057243f154`.

The five residual bodies share a precise compiler boundary. January uses
compiler-generated `__CIfmod` and `__CIpow` helpers and inline x87 `fcos`/`fsin`.
The pinned production compile emits `_fmod`, `_pow`, `_cos`, and `_sin` calls
from ordinary C. Explicit intrinsic requests, pragmas, flag changes, assembly,
compiler substitution, or object patching are forbidden. No codegen search
followed the sole frozen candidate compile. Reopen these bodies only with
original source provenance or a legal ordinary-C spelling independently shown
to produce the January math lowering.

## Gates

- `halobetacache_build`, `libcmt_build`, report generation, and semantic audit:
  pass;
- semantic audit: 470 units, 4,398 functions evaluated, 4,253 semantic exact,
  4,313 accepted exact, zero unit errors;
- ordinary configured headline remains 377/833 complete objects,
  4,199/11,060 functions, and 460,573/2,198,102 matched code bytes;
- Halo headline remains 275/468 complete objects, 4,032/7,574 functions, and
  447,659/1,770,166 matched code bytes;
- object admission: 0 candidates, 0 contradictions, 0 revocations;
- parked validation: 3 active, 0 stale, 0 invalid;
- tooling tests: 179/179 pass, with only the known unwritable pytest-cache
  warning;
- `git diff --check`: pass;
- no protected source, matching label, semantic ledger, comparator, admission,
  parked manifest, compiler setting, or project configuration changed; no push.

## House-rule audit

The retained source is C89. Each parameter remains on its own line and every
function retains an explicit terminal return. This translation unit performs
no tag or object datum access. It adds no assembly, `register`, `volatile`,
barrier, pragma, explicit intrinsic, attribute, raw address, pointer/integer
reconstruction, pointer or union pun, synthetic anchor, byte forcing, object
patch, compiler substitution, or matching exception.
