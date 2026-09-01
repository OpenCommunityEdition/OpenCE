# `source/scenario/wind.obj` — natural initializer reconstruction and fuzzy park

## Result

`wind_variance_initialize` is now written as coherent typed C and is parked as
an honest fuzzy reconstruction. The wind object remains **7/9 strict exact**:
the former unwritten initializer is now the second residual beside
`wind_variance_get`. No previously exact wind function changed.

The retained initializer measures:

| measurement | January | retained candidate |
| --- | ---: | ---: |
| padded `.text` | 384 | 400 |
| instructions | 130 | 137 |
| frame | `0x38` | `0x40` |
| relocations | 9 | 9 |
| normalized SHA-256 | `1d5cd879825797c62a8f525b3f47ac10ee86b61f82da861f5ae6e6db4afa41ff` | `75c381860540e026071ff2e96633e80efbc09849bb0dc828d3cc04dc2c522146` |
| objdiff fuzzy score | — | `60.99206%` |

The relocation destination multiset is the same: `wind_globals`, the global
random seed getter, the seeded direction helper, the `0.125f` literal, and the
out-of-line vector spline helper. Their late-loop offsets/order still differ
because the candidate hoists wrapped-control-point addresses while January
computes them in the axis loop.

## Source and provenance

The January target is
`build/split/source/scenario/wind.obj::_wind_variance_initialize`. Its public
storage class and descriptive name agree with the recovered September 2001
linker-map entry already recorded in the prior wind ledgers.

The later HCEA reconstruction at
`../../research-cache/halocea-cseries-20260820/src/blam/scenario/wind_variance_initialize.c`
corroborates the complete algorithm:

1. seed eight keyframes in each of the three variance axes;
2. fill the seven samples after every keyframe;
3. use the previous, current, next, and following keyframes modulo eight;
4. evaluate the uniform cubic spline at
   `control_point + sample_index * 0.125f` with unit spacing.

The Marathon source tree at
`../../_research_cache/marathon-m2-infinity-source-code` was searched for the
initializer, cyclic wind tables, and uniform-spline topology. It corroborates
only Bungie's ordinary random/math coding style; it contains no direct donor
for this routine. The HCEA source is therefore semantic evidence, not a
byte-shape donor.

The retained source uses `real`, `real_vector3d`, `short`, and `word` from
the project headers. The three wrapped keyframe indices remain `word`:
January's `movzx` instructions prove unsigned 16-bit values. A smaller
signed-`short` form emits `movsx`, so its superficially better size is not
retained.

## January shape and bounded negative

The seed loop has January's pointer induction: start at
`wind_globals.variance[0][0]`, advance `0x300` between axes, and advance
`0x60` between keyframes. The candidate reproduces that instruction topology;
only stack-home offsets move because of the larger frame.

In the spline phase January keeps wrapped numeric indices live and computes
three array addresses in the inner axis loop. The current VC7 build instead
strength-reduces those invariant expressions into three pointer homes. One
bounded ordinary-C refinement made the result/current pointers explicit in
lockstep. It moved farther away: **432 bytes**, frame `0x44`, and **10
relocations**, so it was reverted. The older phase-union carrier and explicit
one-local-per-target-slot forms remain rejected by the prior ledgers as source
lifetime fabrication, not evidence-backed Bungie code.

## Inline and object ownership

`real_math.h` currently defines both spline functions with `__inline`, while
January wind calls an unresolved external `uniform_cubic_spline_vector3d` and
owns neither spline COMDAT. Calling the header form initially emitted two
surplus text owners. `wind.c` now applies the same per-translation-unit
include rename pattern already used by `objects/widgets/glow.c`, then declares
and calls the canonical external vector spline. The unreferenced renamed
inlines are discarded.

The post-fix ownership census is:

- January text owners: **10**
- candidate text owners: **10**
- target owners missing from candidate: **0**
- candidate owners absent from target: **0**
- storage-class mismatches: **0**

This preserves January's out-of-line spline schedule without changing the
shared math header or emitting surplus COMDAT ownership.

## Verification

- `python tools/campaign/gate.py source/scenario/wind --all`:
  **7 exact, 2 residual, 0 unwritten**.
- strict COFF comparison of the seven inherited exact functions: all exact.
- `python tools/campaign/gate.py source/units/units`:
  **189 exact, 0 residual, 0 unwritten**.
- `python tools/fake_match_scan.py --format json --fail-on-findings
  source/scenario/wind.c`: zero findings.
- the complete text-owner census above: no ownership regression.
- `git diff --check`: clean apart from the repository's existing line-ending
  conversion warning.

The retained source contains no union punning, volatile access, inline
assembly, address-derived private/global names, fake dependency, dead branch,
or nonsensical control flow. Reopen only with authentic January source, a
same-compiler donor carrying the target lifetime topology, or a natural
loop/local spelling that prevents the address hoist without changing object
ownership.
