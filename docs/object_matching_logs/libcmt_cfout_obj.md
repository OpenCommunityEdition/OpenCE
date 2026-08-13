# `libs/libcmt/cfout.obj` exact-match ledger

## Plain-English scope

This runtime object converts the bit fields of a C `double` into Microsoft's
internal ten-byte `_LDOUBLE` representation, then asks the CRT's decimal
output engine to produce the sign, decimal exponent, and mantissa text used
by formatted floating-point output.

The atomic admission unit owns two functions and no data.

## Provenance and preflight

- Initial campaign base: `b06c22eafa6cb574bd59bb522bf34a7ff62828b1`.
- Final revalidation base: `08f86f9b84fd45804cb1177fabd07ddaa672b9b6`.
- XDK 3911 CL `13.00.9254.1`; unchanged libcmt `/O1 /Gy` flags.
- XDK `libcmt.lib` SHA-256:
  `60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
- CodeView source path:
  `d:/xbox-finalfre/private/sdktools/crt/fpw32/conv/cfout.c`.
- Authentic Microsoft source mirror:
  `xerohour/xbox_leak_may_2020`,
  commit `fd00b4b3b2abb1ea6ef9ac64b755419741a3af00`, blob
  `c41631f9b54e45d183d5be1a7bec72face263858`, path
  `xbox trunk/xbox/private/sdktools/crt/fpw32/conv/cfout.c`.

Before any production edit, the authentic archive member was compared with
the January split object. Both owned sections were already strict-identical:

- `__dtold`: 186 bytes, zero relocations.
- `_fltout2`: 102 bytes, two relocations, to `__dtold` and `$I10_OUTPUT`.

Neither object owns `.data`, `.bss`, or `.rdata`.

## Experiment ledger

| ID | Readable source shape | Result | Disposition |
|---|---|---|---|
| P0 | Authentic Microsoft control flow with public `strcpy` declaration | `__dtold` exact; `_fltout2` 92B/3 relocations because the public header emits a call while Microsoft's private CRT header inlined it | Rejected |
| P1 | Same topology, spelling the terminating character-copy loop directly | **EXACT**: `__dtold` 186B/0 relocations and `_fltout2` 102B/2 relocations; normalized hashes equal | Accepted |

## Original-bug audit

No original defect is apparent. The zero, denormal, finite, infinity, and NaN
bit patterns follow the CRT's intended internal conversion contract. Any
behavioral modernization would be inappropriate in this exact-match layer.

## House/Berth audit

- Readable typed C, descriptive locals, and named constant.
- No assembly, `volatile`, forced inline, pragmas, UB, byte forcing, or flag
  changes.
- January bytes and symbolic relocation identities remain authoritative.
- Matching credit is withheld until the complete object and all project gates
  pass after a final canonical rebase.

## Current strict gates

- Forced build under unchanged `/O1 /Gy`: passed.
- `__dtold`: exact normalized SHA-256
  `6473f9dfd544aab6a4aab656bc5cdcfc6ba28dbc80e2fc7946b55f48853ea792`.
- `_fltout2`: exact normalized SHA-256
  `bc242540fbc99b75a8efe9fdf769c47b74c63350ea8932071e443ef7fd284c4d`.
- Complete section inventory: two code COMDATs and no owned non-code data,
  identical between January and candidate.
- Full Halo and libcmt builds: passed.
- Progress/semantic report after rebase: zero unit errors; this campaign
  checkpoint is 360/833 Matching overall and 93/212 in libcmt (including other objects
  integrated into canonical while this lane ran).
- Semantic audit: 461 units scanned, 3,587 functions evaluated, 3,503
  accepted exact, zero unit errors.
- Admission audit: zero candidates and zero revocations; only the known
  pre-existing `source/shell/shell_xbox` contradiction remains.
- Tool tests: 179/179 passed.
- `git diff --check` and prohibited-construct scan: passed.
- Clean-tree regression snapshot/check: both functions `still_exact`; no
  failures, warnings, changed non-exact siblings, or ownership warnings.
- Every decisive gate above was repeated after rebasing to the final campaign
  base.
