# `path_obstacle_avoidance.obj` valid-point recovery — 2026-08-26

## Outcome

This bounded follow-up recovers `_valid_real_point2d` strict exact at 60
meaningful / 64 padded bytes, zero relocations, and normalized SHA-256
`c7d201736ce2c6b80551e9b7a8882ecfab6f806b8a5450dae7f7667a8b9c0072`.
The four inherited exact functions remain strict.  The object therefore
advances from 4/24 to 5/24 exact functions and gains 60 meaningful bytes.  It
remains incomplete and `NonMatching`; no data or object-completion credit is
claimed.

The accepted E2 source is ordinary defined C89.  It uses typed
`real_point2d`, copies float representations through standard `memcpy`, keeps
every parameter on its own line, and ends with an explicit return expression.
It uses no assembly, union or pointer punning, undefined behavior, raw
address, volatility, barrier, pragma, register hint, force-inline control,
synthetic anchor, object-byte patch, compiler change, or comparator exception.

## Frozen evidence and novelty

The wave starts from cumulative commit
`2615f2db14c49536a588c5240c628428d77f4762`.  Evidence is preserved under
`outputs/path_obstacle_valid_point2d_evidence_20260826/`.

- January target object SHA-256:
  `773FCF3C4C50847F26051520CB99EDD537ECBC8D810752BE010D233ACC009303`.
- frozen cumulative base object SHA-256:
  `C76C904D3A7CF60FB338D429EF311EFE43A5874E46B4296F74008B26DE560581`.
- baseline source SHA-256:
  `36F65B3546A2301E661ECA16563356ABFA5922B3D2F18313E5CABC8CFAFC355E`.
- pre-wave regression manifest SHA-256:
  `7C380440CD711C7F80141E5DB230D27BCD75CEA9A6768083CA5573A49144BD80`.
- E1 source/object SHA-256:
  `23AFF9F786C850E51EDB521A360D7DB0F994321BA2F334E27BD30CC3D248AD87` /
  `663491C9587DC02B3F9EE7A76B4BC263E3AA363105858F5B496F4E6BEE44BF01`.
- accepted E2 source/object SHA-256:
  `C868AFEADC9CF53A9DB05A2D71961845A47FF18D80DDED083E5BB348EE64409B` /
  `7E399016FFBA2691B1CF6ED374F6407BDCE6B1A249B85A4895C87A2FDBDB5227`.

The current shape atlas proves the January recipient is byte-identical to the
already exact `source/game/players:_valid_real_vector2d`: 64 padded bytes,
zero relocations, and the same normalized hash.  `real_math.h` independently
provides the corresponding `valid_real_point2d` semantics and typed
signature.  The earlier complete two-leaf ledger
`path_obstacle_avoidance_obj_jonas_two_leaf_20260821.md` was read in full; it
did not emit or measure this helper.  All surviving workspace copies and
Claude/Codex ledgers were searched for a `valid_real_point2d` implementation
or include-alias experiment in this translation unit; none existed.  E1 was
therefore a genuinely new source experiment.

## E1 exact body, rejected ownership topology

E1 used the repository's established out-of-line-inline materialization
pattern: it renamed the header inline while including `real_math.h`, then
defined the public function with the natural body
`valid_real(point->x) && valid_real(point->y)`.  The pinned XDK 3911 compiler
passed `/Zs`; one ordinary compile emitted `_valid_real_point2d` strict exact
on the first shot.

E1 was not retained.  VC7 also emitted the external inline `_valid_real`
COMDAT, and placing the new body before the four accepted functions shifted
their physical section ordinals and the associated literal-owner identities.
Direct comparison still proved all four inherited bodies and relocations
exact, but the pre-wave regression gate correctly failed their frozen
ownership evidence.  E1 is preserved unchanged as evidence and received no
credit.

That regression result supplied two new facts for a separate E2 wave:

1. the target body itself was already exact; and
2. acceptance required removing the extra `_valid_real` owner and appending
   the new COMDAT after the inherited functions.

## Defined-C probe and E2

A disposable, non-production XDK probe tested representation-copy source
forms.  Direct `memcpy` produced a 48-byte scheduling variant.  Scoped early
returns and explicit shared-tail forms stayed 64 bytes but differed in branch
or Boolean return scheduling.  A sequenced short-circuit expression using two
local float copies was decisive:

```c
memcpy(&x_bits, &x_value, sizeof(x_bits));

return (x_bits & 0x7F800000) != 0x7F800000 &&
	(y_value = point->y,
	memcpy(&y_bits, &y_value, sizeof(y_bits)),
	(y_bits & 0x7F800000) != 0x7F800000);
```

It emitted all 64 target bytes, no relocations, target hash `c7d201...`, and
no helper function.  The final probe source/object SHA-256 values are
`3073B591D4128B4E74A1FF538FDEFF8E99FA2CBEEEB11A7FF498A58EF8C3E0ED` and
`2B024A3E05720675E99DDB9AC1D85BC9804548B6381DFFA7D19F517347F1BAAD`.

E2 applies exactly that proved body, undefining the repository's `memcpy`
compatibility macro locally so `<string.h>` supplies the standard operation.
There are no later `memcpy` uses in the current partial translation unit.  It
appends the new function after the inherited bodies, preserving every prior
runtime section ordinal.  A second `/Zs` pass succeeded and the separate E2
ordinary compile reproduced the probe's strict target identity.

## Strict and regression evidence

The hardened comparator proves these five functions strict:

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_valid_real_point2d` | 64 | 0 | `c7d201736ce2c6b80551e9b7a8882ecfab6f806b8a5450dae7f7667a8b9c0072` |
| `_path_get_step` | 80 | 4 | `ae2df3005e2c2fb9bb7cff93e5e556aa251e7849df4c31272f2f5ed524ed15b5` |
| `_heap_parent_index` | 64 | 4 | `d881211c22ac87e87eea84ff27ee5a869b529d6deb342183bb588083d59e7eff` |
| `_heap_left_index` | 16 | 0 | `917a9fedd32bb5126c2d04bdaf9fbf6fdcbc3aa75902d1106f38219e073fca73` |
| `_heap_right_index` | 16 | 0 | `6a4798b5ee49d1972065decde68d52bfbfe40fa010e23ee6aab43167988be886` |

The E2 regression comparison reports only `_valid_real_point2d` as
`newly_exact` and all four inherited functions as `still_exact`, with no
changed nonexact function.  All inherited runtime non-code sections and
literal-owner identities are unchanged.  Its two expected pre-admission
findings are the expanded compiler symbol inventory (the new exact COMDAT and
standard `__fltused` marker) and `.debug$S`; neither carries runtime data
credit.  No adjudication or comparator relaxation is added.  Clean
post-admission replay on the cumulative branch is required before final
integration validation.

## Integration and committed-state validation

The isolated implementation is commit
`5f46fa48f30b3b93bef63214ddd43e1b46b4bcd4`.  It was cherry-picked into the
authoritative cumulative branch as
`f8e3800861693e7d7f33264c00bae523c2afdfb0`.  The committed source blob is
`697bb107d31d90911ed95e1b072b03cef968c761`.

The normal cumulative Ninja edge rebuilt only
`build/base/source/ai/path_obstacle_avoidance.obj`.  Its phase-local raw
SHA-256 is
`76E339138D47427296428EFF67EC4FC57E5E8B7E0D22DB8648E81D5EA855A383`.
Direct hardened comparison again proves `_valid_real_point2d` and all four
inherited functions strict, including every padded byte and ordered
relocation.

Fresh objdiff and strict semantic reports complete without unit errors.  The
semantic report evaluates 4,299 functions and accepts 4,217 exact functions.
Current campaign progress is:

- all code: 508,297 / 2,198,102 meaningful bytes and 4,191 / 11,060
  functions;
- Halo code: 495,383 / 1,770,166 meaningful bytes and 4,024 / 7,574
  functions;
- all data: 1,836,756 / 4,176,062 bytes;
- Halo data: 1,831,564 / 3,923,451 bytes;
- file completion: 377 / 833 overall and 275 / 468 Halo.

Relative to the immediately preceding cumulative report, this recovery adds
60 meaningful code bytes and one function.  The additional 187 bytes, one
function, and one complete object visible relative to the older turn-opening
report are the separately integrated `cseries.obj` closeout, not part of this
wave.

The full `halobetacache_build` and `libcmt_build` gates report no work pending.
Object admission reports zero candidates or revocations; the sole
`shell_xbox` completion-label contradiction is pre-existing.  Parked-function
validation remains 3 active, zero stale, and zero invalid.  The complete
tooling suite passes 179/179 tests.  Source-policy scans and `git diff --check`
are clean, and no protected Units source was touched.

The post-admission cumulative regression manifest is preserved at
`outputs/path_obstacle_valid_point2d_evidence_20260826/regression-post-admission-cumulative.json`,
SHA-256
`36152977639A301CC6A573173660C69525EE08484F53C806A0644C5416A859BD`.
Its immediate replay reports all five accepted functions as `still_exact`,
with no newly exact or changed nonexact functions, failures, or warnings.
