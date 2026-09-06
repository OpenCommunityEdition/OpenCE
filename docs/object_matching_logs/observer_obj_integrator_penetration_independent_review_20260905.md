# Observer integrator and penetration: independent review (2026-09-05)

## Verdict

**PASS for preserving the reviewed natural Observer caller cluster as
function-level reconstruction, subject to root's normal canonical rebuild and
owner checks.** The six functions reviewed closely here have source-credible
January semantics. This is not whole-object `Matching` approval, does not turn
the residual functions into strict matches, and grants no data credit.

The final reviewed packet is
`scratch/observer-real-caller-cluster-behavior-verified-20260905.obj`, SHA-256
`7eb0e8d0aefa968f9abb8c7b17eca0163085012adacaebb8faa11ae8682a6911`.
Its source snapshot is
`scratch/observer_real_caller_cluster_behavior_verified_20260905.inc`, SHA-256
`f22e5a642dc8dc3a795eea51491fb3cd985f5bff6297b132ec55e51f65483a50`.
The packet gate is 13 exact / 12 residual / 1 unwritten. All 11 inherited exact
owners remain exact. The newly strict functions are
`observer_apply_rotational_displacement` (128 bytes, 4 relocations) and
`observer_rotational_displacement` (288 bytes, 21 relocations).

The functions reviewed in detail remain honest residuals except for the
strict rotational function named above:

| Function | January bytes/relocs | candidate bytes/relocs | January hash | candidate hash |
| --- | ---: | ---: | --- | --- |
| `observer_update_displacements` | 80 / 6 | 80 / 6 | `30a59c7d53b2...` | `caa6fa0f9491...` |
| `observer_pass_time` | 160 / 13 | 160 / 13 | `6ecf7cfcf741...` | `0002612135b4...` |
| `observer_update_velocities` | 512 / 14 | 544 / 14 | `9af560df9a89...` | `d486a70d554e...` |
| `observer_check_penetration` | 752 / 11 | 688 / 10 | `ef9a396b122f...` | `1062b1380ec1...` |

No source spelling, declaration-lifetime, register-layout, inlining, or
control-flow variants were requested to close those residuals.

## Typed layout and ABI boundary

The reconstruction uses the genuine complete Observer records rather than an
opaque prefix. The independently checked layout has an Observer stride of
0x29C, command parameters at +0x0C, result at +0x74, current values at +0xB0,
velocities at +0xE8, accelerations at +0x120, polynomial coefficient arrays at
+0x158/+0x184/+0x1B0/+0x1DC/+0x208/+0x234, and displacements at +0x260.
Parameter flags and timers are at command +0x4C and +0x54. The authentic
parameter-real counts are 3, 3, 1, 1, 3. Root's separate 46-consumer schema
trial accepted the named command/time enums and complete union layout without
changing strict or parked runtime owners.

The private January functions use compiler-specialized register conventions.
For example, `observer_update_displacements` receives its three effective
pointers in a different EAX/ECX/stack arrangement from the candidate. Both
arrangements operate on the same typed fields and are private within the real
caller closure. This is a disclosed code-generation residual, not a public C
ABI defect and not a reason to alter the source. The external
`observer_update(real)` dispatch and its end-to-end behavior are the public
ABI authority.

## Integrator behavior

`scratch/observer_integrator_behavior_review_20260905.py` executes frozen COFF
instructions from January and the final candidate with only genuine callees
stubbed. Both objects pass the same cases:

- command freeze bit 0x20 suppresses displacement, polynomial, acceleration,
  velocity, position, and timer updates;
- the active call order is displacement, polynomial, acceleration, velocity,
  then position;
- five timers subtract `observer_globals.dtime`, retain only ordered positive
  results, and clamp zero, negative, or unordered results to +0;
- the derivative component counts are exactly 3, 3, 1, 1, 3;
- an active polynomial case at remaining time 0.5 produces 12.5625;
- an elapsed invalid command preserves the prior velocity values;
- per-parameter time-force bit 1 clears only that parameter group, while
  command force-time bit 3 clears every group, with byte counts 12/12/4/4/12;
- the ordinary elapsed valid path computes `-displacement/dtime`.

The direct field roles in `observer_update_displacements` are also checked:
current parameters and the last command feed `observer_find_displacement`, and
the output is the real +0x260 displacement record. The behavior result is
`scratch/observer_integrator_behavior_verified_candidate_20260905.json`,
SHA-256
`e4fd06e0a386c47a902ef404db8170611ec015025dccc4b39dd936058131f072`.
The packet's separate public dispatch suite passes all 100 cases.

## Penetration behavior and corrected evidence record

The final source follows the January instruction stream and the frozen HCEA
reconstruction on the substantive algorithm:

- derive location and the underwater/media policy from the focus point;
- test the direct focus-to-camera segment;
- construct the two genuine perturbation axes and try them in the order
  `-up`, `-cross`, `+up`, `+cross`;
- retain only a strictly smaller ordered collision fraction;
- bisect the best offset for ten iterations, using the observed 0.1 fraction
  continuity test; and
- blend the direct and perturbed collision fractions into the distance.

The candidate computes two side-effect-free scalar initializers slightly
earlier than January. Their inputs are unchanged across the intervening const
scenario queries, so this is not a semantic defect and was not used as a
source-shape tuning opportunity.

The January executable has an unusual signed-probe tail. The final source
correctly preserves it as:

```c
if (last_miss_t < last_hit_t)
	upper = lower;
else if (upper < 0.f)
	upper = -upper;
```

The frozen target and final candidate both pass three independent executable
cases: a direct collision with no better probe reduces 10 to 7.5; a negative
probe refinement retains the negative lower bound and produces
0.0048828125; and the symmetric positive probe produces 9.9951171875. The
harness additionally verifies probe endpoints, underwater flag propagation,
collision-call counts and fractions, callee-saved registers, and stack
balance. The result is
`scratch/observer_penetration_behavior_reviewed_candidates_20260905.json`,
SHA-256
`fc46c1bf8a768417eeed5c556e2609ed6e12ef9e0fb4a6dc4ba9bc0a361a6a9e`.

This paragraph explicitly retracts an earlier review inference. A first static
reading incorrectly described the tail as an unconditional absolute-value
selection, and a combined harness run stopped on the target before executing
the candidate. It therefore did **not** establish the claimed candidate
output. A prior 227B-named scratch object's association with the transient ABS
source snapshot is not independently established. No claim is made that VC7
changed ABS mathematics or that this old scratch object proves such a source
form. The only admitted evidence is the executed January behavior, the final
if/else-if source above, and the executed final 7EB0 candidate behavior.

The HCEA files under
`../../research-cache/halocea-full-blobs-20260830/src/blam/camera/` are useful
corroborating binary-reconstruction evidence; they are not original January
source authority.

## Complete-owner boundary

The final owner census preserves every inherited code and non-code owner and
reports no COMMON, lost baseline owner, or emitted `_point_from_line3d`.
Nevertheless, the candidate naturally emits four target-absent code COMDATs:

| Helper | bytes | candidate selection | frozen selected January owner |
| --- | ---: | ---: | --- |
| `_sine` | 16 | SELECT_ANY (2) | `source/ai/actor_combat`, NODUPLICATES (1) |
| `_cosine` | 16 | SELECT_ANY (2) | `source/ai/actor_combat`, NODUPLICATES (1) |
| `_valid_real_point3d` | 96 | SELECT_ANY (2) | `source/effects/particles`, NODUPLICATES (1) |
| `_vector_from_points3d` | 48 | SELECT_ANY (2) | `source/ai/action_alert`, NODUPLICATES (1) |

All four bodies match the corresponding selected-owner normalized bytes, but
byte identity alone does not prove ordinary-link compatibility. Root is
separately normalizing the genuine ActionAlert, Particles, and ActorCombat
owners; this frozen review neither assumes that later work has landed nor
waives the boundary. The Observer object must be re-censused after those owner
changes before any whole-object admission.

The candidate also emits natural SDK/math constant copies, including the three
D3D tables. Those copies and all new literals require the normal pooled-owner
audit and receive zero new data credit here. The real Observer static arrays
and counts are accepted only under their separately reviewed schema and target
boundaries. Candidate-only non-code ownership prevents treating this review as
a complete source-object closeout.

## Frozen evidence

| Artifact | SHA-256 |
| --- | --- |
| `scratch/observer-schema-target-20260905.obj` | `ca50fa09553eeb4284b01428837f0804af762728538f06c5f1c73e65a793eaad` |
| `scratch/observer-real-caller-cluster-before-20260905.obj` | `e02ccebfa11057470d0b61573908884a54c25cfd4be3c07a404fa6a0ac4e111b` |
| `scratch/observer-real-caller-cluster-behavior-verified-20260905.obj` | `7eb0e8d0aefa968f9abb8c7b17eca0163085012adacaebb8faa11ae8682a6911` |
| `scratch/observer_real_caller_cluster_behavior_verified_20260905.inc` | `f22e5a642dc8dc3a795eea51491fb3cd985f5bff6297b132ec55e51f65483a50` |
| `scratch/observer_real_caller_cluster_behavior_verified_20260905.edits.json` | `04a092622697635594435f363529b43a5f02cb96fa24a26ffe8108473ce64a80` |
| `scratch/observer_integrator_target_semantics_20260905.json` | `11aa98c82cec8ef65619ba3095551c440a58e9ba1b22b6ce6c3747af1e221cad` |
| `scratch/observer_integrator_candidate_semantics_20260905.json` | `fe0029080aba54da2851d86e5a35f96e9d514e06e3c552b92a6f77e5030c74bb` |
| `scratch/observer_penetration_target_semantics_20260905.json` | `d4b552a9de34183a16c19b8477c71e4a4fb4691fa987179f9a1776adbe34d155` |
| `scratch/observer_reviewed_owner_audit_20260905.json` | `6c6f2fbd58f68eef2d81ae7d1cb57be9e0ca7499d008bd697e79a97a78a9c0e8` |
| `scratch/observer_reviewed_math_owner_census_20260905.json` | `67d8535adcd427c268aed32c4c583e219520f37463687c795c35135d1aaad09c` |

No production source, shared header, configuration, target object, or build
output was changed by this independent review.
