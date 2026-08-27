# `decals.obj`: A+RGB packing fixed point (2026-08-27)

## Outcome

`_real_a_rgb_color_to_pixel32` was reconstructed from the January target,
HCEA's independently recovered two-argument ABI and channel semantics, and
the Stian source topology. Two evidence-separated full-translation-unit
measurements were strict nonmatches, so all production source and compiler
configuration changes were reverted exactly. `decals.obj` remains
`NonMatching`; no function, data, ownership, semantic, or completion credit
is claimed.

## Frozen evidence

- clean starting commit: `5173181710bd31930902fe013e8b0bf5978c9a7e`;
- target object SHA-256:
  `7dc63419b2c0e0abe896ecbe05b42e05f9024ba77a6f17961a603e6fadf916bc`;
- pre-wave source SHA-256:
  `a0e5d94e3e99a6d7a0577bee5546af2ca089cf787df1f1ea9af6f4eb9277dd5a`;
- pre-wave configuration SHA-256:
  `9461278386852cddc0a79d822343554e3fa93087ed0adcafea69d42d7e3fde02`;
- baseline object:
  `build/audit/decals_real_a_rgb_color_baseline_20260827.obj`;
- first candidate:
  `build/audit/decals_real_a_rgb_color_first_shot_20260827.obj`;
- lifetime candidate:
  `build/audit/decals_real_a_rgb_color_scale_lifetime_20260827.obj`.

The target is 260 meaningful / 272 padded bytes, with 14 relocations and
normalized SHA-256
`50c2f830176fb14c624b5177983c19aba8c82b5e07eecf7a205b4149afe1e917`.
It asserts alpha range and `valid_real_rgb_color`, then loads alpha, red,
green, blue, and one stack-local `255.0f` into the x87 stack. It performs four
signed dword `FISTP` conversions in blue/green/red/alpha order and assembles
ARGB in one reusable stack slot. There is no `+0.5f` rounding term.

## Measurements

The first readable defined-C implementation used the target assertions,
typed `real_rgb_color` access, explicit signed conversions, unsigned packing,
and target-observable `/QIfist`. It measured 256 padded bytes, 18 relocations,
and normalized SHA-256
`03e67ecbebdc621420e8d2d0368b1d8a7c880fc382a7e6c1a9bf07475fff2ffb`.
VC7 emitted four relocatable `255.0f` loads and four qword conversion
temporaries. January instead initializes one stack-local scale before the
assertions and uses dword conversions.

That strict result supplied the second hypothesis: declare one initialized
scale plus distinct signed channel locals before unsigned packing. The source
passed `/Zs`, and the normal XDK 3911 edge was compiled once. VC7 completely
propagated those locals and reproduced the first candidate byte-for-byte after
relocation normalization: again 256 padded bytes, 18 relocations, and the same
normalized SHA. Its raw object SHA-256 is
`736c48a6cf7c254722a75673506a33e40d90d3f892a0ba0565c89f851f823e1f`.

A repository-wide January shape census found no other function with the
target normalized hash, so there is no exact source donor to transfer. HCEA's
unsigned-byte locals describe later Xbox semantics but not this January x86
lowering. Stian's `+0.5f` form is contradicted directly by January code.

## Fixed-point boundary

Do not repeat direct constants, one named scale, four named signed locals,
scope/declaration spelling, unsigned channel locals, or the Stian rounding
term. The obvious single signed-expression spelling may induce the January
dword conversion tree, but shifting a positive signed `long` into bit 31 is
undefined behavior and is prohibited by campaign rules. Optimizer-control
devices, pointer/union punning, barriers, assembly, intrinsics, byte forcing,
and compiler substitution are likewise inadmissible.

Reopen only with an authentic January source record, a new exact same-shape
donor, or a readable defined-C topology that independently explains why VC7
must retain the scale and signed conversions. All tracked production files
were restored byte-for-byte; only this Jonas-owned evidence ledger is retained.
