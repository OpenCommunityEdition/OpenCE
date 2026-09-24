# `periodic_functions.obj` best-fuzzy builder rebaseline (2026-09-24)

Parent: `8188648c`, after the independently admitted three-static data and
ownership correction. This follow-up changes no header, symbol attribution, or
matching credit. The unit remains `NonMatching`.

The private `periodic_function_build_variable_period_x_table` now follows the
later first-party `/Od` build's recorded `x_table` parameter, `short index`,
indexed loops, four `real_random()` calls, and one accumulation expression.
The waveform factors use January's measured constants, naturally folded from
`8.2f`, `10.2f`, and `14.6f` times `_pi` divided by the 1024-entry table size.
This replaces our invented amplitude/cosine/destination locals and hand-expanded
random helper. The exact sibling `periodic_function_build_table` also uses the
attested `real_random()` spelling in its noise arm; this is byte-inert there.
The proposal and its negative controls are recorded in
`research/fifty_objects_20260925/w/periodic_functions/LEDGER.md` and the
2026-09-24 small-object re-audit.

The later compiler-trace note
`docs/object_matching_logs/periodic_functions_c2_probe_20260924.md` says not to
land the fuzzy candidate because it found no authentic *IL lever for exactness*.
Its new measurement independently reproduces this candidate's SHA; it does
not refute the `/Od` source evidence, identify a fake construct, or show a
regression. We interpret its no-landing sentence as a research stop on further
source-shape probing, not a binding rejection of the owner's standing policy
to preserve the best credible nonexact source in canonical. This landing is
explicitly a **zero-credit preservation**, not a reopened exact-match claim.

January and rebuilt builder: **256 padded bytes, 18 relocations**. The rebuilt
normalized SHA changes from `af8e65f5d773f0fb1e635264e54d3e63fccee99c69b65364a72be56cdca323b4`
to `c5280305149a6466d123f479355c2a15167f284c1c358403a9d5b4f20e48a54d`;
January is `802ef4da9736bcaf8a616c56d03ba3e9322b229ad4b3814c60909462d7ebacf3`.
Objdiff rises from 91.233765% to 96.623375%. The remaining difference is one
independent scheduling choice: January issues `inc edx` before the ready x87
load at +0x6f, while the rebuilt body loads first. **This is fuzzy, not exact;
zero code bytes and zero functions are credited.** No `volatile`, barrier,
pragma, artificial dependency, inline assembly, or manual helper expansion is
retained. The existing park is re-measured, not retired.

`real_random()` emits one shared-header COMDAT. It is byte-identical to
January's selected `action_charge.obj` copy (16 bytes, two relocations, normalized
SHA `50eeeb3dc58069325659d8bc0b6f2c17bb9f4fdce278b2f1a37edb64f4fcc60c`),
and the selected-provider link check passes in both object orders. The exact
`periodic_function_build_table` is a genuine caller of this helper.

Verification: full `ninja` and `ninja progress` pass; all six other functions
remain strict exact, including the sibling above. A stable-section sweep of all
8,252 functions gives 7,617 exact before and after, zero gained and zero lost.
Parks: 83 active, zero stale or invalid. Admission audit: zero contradicted,
zero revoked. Fake-match scan of the changed C file: zero leads. Pytest:
1,161 passed, five skipped, 26 subtests passed. Halo code remains
1,577,237 / 1,770,166 meaningful bytes; Halo data remains 2,587,011 bytes.

Reopen only with authenticated January source or a decoded VC7 DAG-scheduler
rule explaining the ready-node choice. Do not repeat the approximately 150
natural source-shape probes already recorded in the Claude ledger.
