# `actors.obj` decision-loop exact closure (2026-09-08)

## Result

`actor_decision_loop` is now a strict section-equal match. The accepted source
changes only two local lifetimes: `action_performed` is initialized before the
loop counters, and `entry_index` is assigned after the diagnostic prefix is
formatted. Runtime behavior is unchanged, and the source remains ordinary,
coherent C without volatile/register steering, fake dependencies, inline
assembly, raw object access, or forced inlining.

The isolated `actors.obj` gate moves from 67 exact / 9 residual to 68 exact /
8 residual. The gain is 548 meaningful bytes and 560 padded bytes, with all 34
relocations and normalized code hash
`b48d5a6f9009d3a3cc05a8807305a4697defaf5294bac7b9bd077de89e087689`
equal to January. All 67 inherited exact functions remain exact.

## Audit and verification

- The campaign-wide historical source sweep found no stronger compilable
  `encounters.c` or `players.c` blob than canonical.
- `python -B -m tools.campaign.gate source/ai/actors --all
  --forbid-emitted-symbol _point_from_line3d`: 68 exact / 8 residual / 0
  unwritten; emitted-symbol guard passed.
- Full `ninja all_source progress semantic_progress`: passed.
- Rename-stable whole-tree comparison: one gain (`actor_decision_loop`, 560
  padded bytes), zero regressions across 8,245 target functions.
- `python -B -m pytest -q -p no:cacheprovider tools`: 1,140 passed, 2 skipped,
  and 26 subtests passed.
- Fake-match scan: zero findings.
- `git diff --check`: passed.

Canonical progress after the full build is 982,763 meaningful exact code bytes
and 6,492 credited functions. No semantic exception or fuzzy credit is used for
this closure.
