# Claude data/ownership closeout: canonical reconciliation (2026-09-24)

Claude's clean `claude/data-closeout-20260924` lane at `b3e9f9af` was
reconciled onto local canonical `jonas/exact-pilots` at `c661a334`, after the
separate ten-Halo-object batch. Its five commits were cherry-picked in order.
The only content merge was the append to `config/semantic_data_matches.json`:
the existing and donor entries are disjoint and both are retained. The donor's
`items.c` helper repair was already byte-identical to canonical's, so it was
not applied twice. Neither lane's credited data was counted twice.

## Accepted result under canonical's installed objdiff 3.3.1

| Measure | Before donor | After donor | Delta |
| --- | ---: | ---: | ---: |
| Complete objects, all | 469/833 | 479/833 | +10 |
| Complete Halo objects | 351/468 | 361/468 | +10 |
| Credited data, all | 2,568,800 | 2,578,710 | +9,910 |
| Credited Halo data | 2,562,486 | 2,572,396 | +9,910 |
| Credited Halo meaningful code | 1,549,953/1,770,166 | 1,549,953/1,770,166 | 0 |
| Stable strict exact functions | 7,585/8,245 | 7,585/8,245 | 0, no regression |

The ten objects and each section's source evidence are in
`claude_data_ownership_closeout_20260924.md`. Their whole-object admissions
include private-symbol ownership and selected-provider checks, not data-byte
equality alone. The lane's `items.c` repair was already present in the earlier
canonical object-closure batch.

## Three retained holds

`objects`, `hs_scenario_definitions`, and `hs_globals_external` remain
`NonMatching`. The first has five helper-provider link conflicts; the second
has a genuine 8-versus-4-byte data-alignment difference and fabricated source
aggregates; the third represents 442 separately named January statics with an
invented array. Their exact functions and data evidence remain available,
without whole-object credit. Explicit entries in
`config/object_admission_rejections.json` record each reopen criterion.
Correcting the three premature `Matching` labels removes all inherited
admission contradictions; it does not change the credited code or data totals.

## Scorer version boundary

The installed canonical `build/tools/objdiff-cli.exe` is 3.3.1, despite
`configure.py` naming 3.6.0. This work does **not** silently replace that
binary. `build.ninja` passes the existing binary via `--objdiff`, so merely
rerunning Ninja does not download the pinned version. Independent read-only
side-by-side reports on the same canonical COFFs show that 3.6.0 changes raw
data results for 47 Halo objects and one libcmt object, and raw code/fuzzy
results for eight D3D8 objects. The strict accepted function identities are
unchanged, but dashboard totals are not directly comparable. A future scorer
rollout must regenerate `objdiff.json` and reports together and repeat the
stable, park, admission and progress gates. The three held objects stay
`NonMatching` under either scorer; a fresh 3.6.0 probe confirmed this.

## Verification

After integration, full Ninja builds all 833 units. The 8,245-function stable
sweep reports zero gains and zero regressions. Parks: 103 active, zero stale or
invalid. The admission audit reports zero contradictions and zero revoked
rows. The modified source files have zero fake-match review leads.
`python -m pytest -q tools` passes 1,154 tests, five skips and 26 subtests.
The ten accepted objects are to be checked in the refreshed local treemap;
the three holds must remain uncredited there. No GitHub push is made by this
integration: the standing push threshold is 10,000 newly exact code bytes,
and this lane adds zero.
