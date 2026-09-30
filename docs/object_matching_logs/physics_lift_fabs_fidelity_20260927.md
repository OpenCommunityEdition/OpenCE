# Physics lift: four source-backed absolute-value corrections

Baseline: `a65968488575d1adcd96da2211a10b342363e09a`.
Scope: four `ABS` -> `fabs` tokens in `source/physics/physics.c`, plus honest
park measurements. No new exact function, meaningful code byte, data byte or
complete object is claimed. Physics remains NonMatching, 14/17 functions exact.

## Source evidence

January directly clears the sign of the forward/velocity dot product before
the continuous x87 lift product at these function-relative offsets:

| Function | Water lift | Air lift |
|---|---:|---:|
| `_physics_compute_new` | `+0x629` | `+0x748` |
| `_physics_update_old` | `+0x7ad` | `+0x8d5` |

The three dot-product terms, powered-point lift ratios, and subsequent up-vector
force identify these operations independently of the candidate's byte score.
Two independent reviewers decoded the January COFF windows. The target object's
SHA-256 is `2d7273c20036601c6ee6a87a032a12e38d598840f7614cecba225910fe11d46c`.

The supplied later `/Od` image corroborates the four absolute-value calls at
`0x7ba765`, `0x7ba920`, `0x7bf020`, and `0x7bf231`. Image SHA-256:
`740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`.
Its adapter ultimately calls `ucrtbased.dll!fabs`, but accepts and returns float
and has additional stored locals. Those later rounding boundaries and locals
are **not** imported into January. The target operation is proven; the exact
original C spelling is inferred, not recovered source text.

The existing `ABS(x)` macro compares `x >= 0` then selects `x` or `-x`; this is
not interchangeable with `fabs` for negative zero or NaNs. Ordinary C `fabs`
returns double, with the final conversion at the existing `real lift` assignment.
The correction therefore makes no all-input equivalence claim. The target's
direct sign clear, not a compiler-state coincidence, justifies the change.

Every factor, operand order, local, scope, helper, header and compiler flag is
unchanged. Production keeps its 2,197 CRLF line endings. Earlier larger physics
packets already contained this arithmetic lead but coupled it to held helper
expansions; none of those expansions or other held forms is admitted here.

## Bounded measurements and rejected control

All sizes below are padded bytes; the second number is relocation count.

| Probe | `_physics_compute_new` | `_physics_update_old` |
|---|---:|---:|
| Frozen baseline | 3120 / 51 | 5456 / 118 |
| New: water only | 3040 / 50 | unchanged |
| New: air only | 3056 / 50 | unchanged |
| New: both | 2992 / 49 | unchanged |
| Old: both | unchanged | 5376 / 116 |
| Four-site packet | 2992 / 49 | 5376 / 116 |
| January | 2944 / 49 | 5168 / 115 |

The two independent function results compose exactly. Removing 208 excess
padded bytes is structural progress, **not 208 bytes of matching credit**.
Both normalized hashes remain different from January:

- new: `acd0df16915ce7cf9b6bc0abfff5cb62bbe5339285eaf796989568ab4db08d3e`;
- old: `1dec5f5b73451138ca09709ccf7baf12925f2688a27416417f7251f8c96a685d`.

Configured stock compile: zero warnings before and after. Diagnostic `/W3`:
12 inherited warnings become 16. The four additions are C4244 double-to-real
conversions at the existing lift initializers. Independent source/ABI review
accepts these understood numeric conversions under the warning-disclosure rule;
there are no new implicit declarations, ABI conflicts, or warning suppressions.

One preregistered final-product `(real)` cast control removed the four warnings
but changed both residual sections. It was rejected because it was not
byte-inert. No cast-placement search, pragma, new macro, or changed flag followed.

## Verification and credit

The production object independently equals the frozen uncast candidate in all
non-debug sections and symbol/storage records. Against the baseline, only the
two named residual code sections change. All 83 non-debug sections and 87 named
definitions retain their inventories; every other code/data section, relocation,
surplus copy, undefined reference and COMMON record is unchanged. No new helper
or provider is emitted. This is not a new whole-program link claim.

Final gates:

- `ninja all_source progress build/report.json`: pass.
- Stable whole-board sweep: 8,252 rows, 7,641 exact; zero gains and zero losses.
- Physics: all 14 inherited exact functions preserved, three residuals remain.
- Parks: 72 active, zero stale, zero invalid. `physics_compute_new` history is
  retained and its measurements updated; `physics_update_old` is newly recorded
  as an unclassified fuzzy residual, not a closure or a new implementation.
- Admission: 12 candidates, zero contradicted, one rejected, zero revoked;
  unchanged. Fake-match scan: the same 26 inherited findings.
- Tests: 1,161 passed, five skipped, 26 subtests passed.
- Scoped whitespace check passes; unrelated README and research edits preserved.
- Halo ledger unchanged: 1,598,242 / 1,770,166 meaningful code bytes,
  7,469 / 7,574 credited functions, 390 / 468 complete objects,
  2,588,903 / 3,923,451 credited data bytes. Scorer remains 3.3.1.

Reopen the residuals on independently supported source, precision or inline
evidence, not declaration/VN filler, arbitrary operand permutations or held
helper copies. All original-bug, helper, cast and source-shape holds stand.

## Local reproducibility records

Private binary inputs and scratch objects are not part of this commit. Local
records under `scratch/astra_frontier_sources_20260927/` retain:

- `math/PLAN.md`, `EXTENSION_PLAN.md`, `CAST_CONTROL_PLAN.md`, the complete
  probe sources/objects/logs and `REPORT.md`;
- `physics_review/REVIEW.md`, independent primary disassemblies and audit;
- `physics_packet_review/REPORT.md` and `POST_INTEGRATION.md`, frozen seals and
  full section/storage comparisons;
- `before.json`, `after.json`, full-build, pytest, admission, park and fake-scan
  outputs; `final_verify.py` and its final receipt.

The minimal uncast patch SHA-256 is
`72d7a51b2563b461da52442139d5a478fe50945a04f12b2d6ae4650b929ee97a`.
