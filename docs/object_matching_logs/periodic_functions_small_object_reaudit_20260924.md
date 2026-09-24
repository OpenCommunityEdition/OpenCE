# `periodic_functions.obj` small-object re-audit (2026-09-24)

Base: canonical `4d1ebf179023f46efb77929fb599e514601152fb`. This re-audit
changes no production source, config, park, or matching credit.

Fresh VC7 gates have **6/7 target functions exact**. The sole residual,
`@periodic_function_build_variable_period_x_table@4`, is 256 padded bytes and
18 relocations on both sides. The current coherent C differs in the
`0x63`–`0x80` instruction schedule, not in calls, constants, or table logic.

The best source is the *later `/Od`-attested* indexed-loop version recovered in
Claude's `research/fifty_objects_20260925/w/periodic_functions/LEDGER.md` and
`production_combined.patch`. It uses `real sum = 0.0f`, a `short index`, two
`for (index = 0; index < PERIODIC_FUNCTION_TABLE_SIZE; index++)` loops,
`x_table[index] = sum` in the first, and `x_table[index] = x_table[index]/sum`
in the second. The accumulation is one expression:

```c
sum += (real_random()+1.0f)*0.25f +
    ((real)cos(8.2f*_pi*index/PERIODIC_FUNCTION_TABLE_SIZE)+1.0f)*real_random() +
    ((real)cos(10.2f*_pi*index/PERIODIC_FUNCTION_TABLE_SIZE)+1.0f)*real_random() +
    ((real)cos(14.6f*_pi*index/PERIODIC_FUNCTION_TABLE_SIZE)+1.0f)*real_random();
```

The exact sibling `_periodic_function_build_table` uses `real_random()` in its
noise arm; the `/Od` build calls that helper there, and Claude's provider-link
test passed with the extra identical `_real_random` copy. I remeasured the
builder replacement plus that noise-arm correction on this base. All six
siblings stayed exact; the builder still failed strict matching, but only one
independent machine-order choice remains:

```text
January  +0x6f  inc edx                 ours  +0x6f  fld dword ptr [ebp-4]
January  +0x70  fld dword ptr [ebp-4]   ours  +0x72  inc edx
```

The remaining integer/x87 interleaving follows from this single ready-node
choice. The Claude ledger records roughly 150 natural source/loop/declaration
and constant probes, plus TU-name and compiler-flag controls, without a
closure. Repeating those variants is not evidence-led. No fake dependency,
`volatile`, pragma, inline assembly, or hand-expanded helper was retained.

The object also has a distinct data-ownership issue: current source defines
one `periodic_functions_globals` aggregate, while the Claude data-gap packet
reconstructs the January symbols as three statics. Even if the last function
closes, that packet needs an independent ownership/data and selected-provider
link audit before marking the *whole object* Matching.

Outcome: no source change and **zero new credited bytes**. The best fuzzy
source remains a measured, unlanded candidate. Reopen the function with an
authenticated January source record or a decoded VC7 DAG scheduler rule that
explains why `inc edx` wins over the ready x87 load at this site. Then rerun
the full seven-function gate, data/owner audit, both-order provider link, full
build, stable sweep, parks, admission and tests before object admission.
