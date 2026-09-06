# `observer.obj` real update caller-cluster reconstruction (2026-09-05)

## Disposition

The complete real `observer_update` caller closure has a source-credible,
behavior-checked scratch reconstruction.  The frozen candidate moves the object
from 11 exact / 0 residual / 15 unwritten functions to 13 exact / 12 residual /
1 unwritten while preserving all 11 inherited exact owners.  The two new strict
owners are:

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `observer_apply_rotational_displacement` | 128 | 4 | `0cf86fd06942eff7a71400d65c94625c1614baac0fc5e058feb50c721dda8541` |
| `observer_rotational_displacement` | 288 | 21 | `d04926a73a36a26456a81edbcb1b585fa981ccc927f2b0c5b132d68ad48541fe` |

This is a prospective gain of 2 functions and 416 padded code bytes.  It is
not yet an admitted production delta.  The final Observer object naturally
emits SELECT_ANY `sine` and `cosine` copies while January selects NODUPLICATES
owners in Actor Combat.  A natural removal-only Actor Combat trial loses those
two currently strict owners because that partial translation unit does not yet
contain their genuine caller.  An ordinary unmodified-object link of the exact
final Observer object with the frozen current Actor Combat object now fails in
both input orders with `LNK2005` for `_sine` and `_cosine`.  Both runs then end
with `LNK1120: 46 unresolved externals`; those expected unrelated unresolved
symbols do not weaken the duplicate-definition proof.  The Observer body
therefore remains withheld until that selected-owner boundary is resolved by a
real Actor Combat caller closure or another source-authentic ownership repair.
No anchor, false export, inlining control, or source reshaping is authorized.

The twelve residuals are useful natural implementations and should be retained
as honest parks if the ownership boundary is later cleared.  No residual was
tuned after the source and behavior review.  `observer_collision_test_differential`
(64 padded bytes) remains the sole unwritten function.

## Frozen authoritative packet

The only authoritative final source/object pair is:

| Artifact | SHA-256 |
| --- | --- |
| `scratch/observer_real_caller_cluster_behavior_verified_20260905.inc` | `f22e5a642dc8dc3a795eea51491fb3cd985f5bff6297b132ec55e51f65483a50` |
| `scratch/observer-real-caller-cluster-behavior-verified-20260905.obj` | `7eb0e8d0aefa968f9abb8c7b17eca0163085012adacaebb8faa11ae8682a6911` |
| self-contained scratch edit manifest `scratch/observer_real_caller_cluster_behavior_verified_20260905.edits.json` | `04a092622697635594435f363529b43a5f02cb96fa24a26ffe8108473ce64a80` |
| production-applicable edit manifest `scratch/observer_real_caller_cluster_behavior_verified_production_20260905.edits.json` | `3c629a66c0c4c88ace8862aa5e9dbf8ba70722065062fc8122baef8e9de56581` |
| frozen pre-packet object `scratch/observer-real-caller-cluster-before-20260905.obj` | `e02ccebfa11057470d0b61573908884a54c25cfd4be3c07a404fa6a0ac4e111b` |
| frozen January object `scratch/observer-schema-target-20260905.obj` | `ca50fa09553eeb4284b01428837f0804af762728538f06c5f1c73e65a793eaad` |
| frozen current Actor Combat object `scratch/observer-actor-combat-current-base-frozen-20260905.obj` | `8879dc95e52aaa032678b40653cc7128e2f7768ac097bab70ec76e9a87fe53bd` |

The production manifest embeds the bodies directly; it does not include a
scratch file and does not introduce a caller-local public declaration.

Earlier scratch objects are diagnostic history only.  In particular,
`observer-real-caller-cluster-first-natural-20260905.obj` has SHA-256
`1bb2005433eae5a3da79d4f0d10c58e9a3896b87c23ac2d9e08eea553ad2d1ca`.
It predates the semantic corrections below and is not admissible.  The source
include used by some intermediate manifests was mutable, so the source/object
association of the 227B-named reviewed object is not independently established.
No claim is made that VC7 transformed the transient, mathematically incorrect
ABS spelling into the desired behavior.  Only the F22E source and 7EB0 object
above are final evidence.

## Real caller closure and typed ABI

The packet reconstructs fourteen genuine functions as one self-retaining
cluster:

- public `observer_update`;
- private `observer_update_command`, `observer_pass_time`, and
  `observer_postcheck`;
- private displacement, polynomial, acceleration, velocity, and position
  phases;
- private `observer_find_displacement`,
  `observer_rotational_displacement`, and
  `observer_apply_rotational_displacement`;
- private `observer_check_penetration` and
  `observer_collision_test_with_t`.

The target cluster totals 9,184 padded bytes; the natural candidate totals
8,896 padded bytes.  Private January functions use compiler-specialized
register conventions, but all are retained by their genuine callers and use
complete Observer/command/derivative types.  No private helper is exported to
force emission and no cdecl/register shim is present.  The public
`observer_update(real)` contract remains unchanged.

The required owner schema was reviewed and trialed independently:

- `observer_command` exposes its genuine 14-real parameter view together with
  the named five parameter groups and the existing time flags;
- the private Observer record keeps the same 0x29C stride and genuine field
  boundaries;
- `structure_cluster.predicted_resources` is a real `tag_block` at +0x28 in a
  0x68-byte structure;
- the time enum is moved from Director to its Observer owner, not duplicated;
- the cluster's natural alignment changes from 2 to 4 because of the real
  `tag_block`; no packing directive hides that fact.

The canonical-only schema trial covered the 46-consumer include union.  It kept
all 6,217 strict functions and 281 then-valid parks, and its runtime audit found
only compiler-local label spelling changes in 11 translation units.  It did
not add an Observer body or imply body admission.  The independent schema
review is `scratch/observer_schema_independent_review_20260905.md`, SHA-256
`a05eddba55104ef97ffb026a47d4883679e14689b3025c989f900c8526e3c217`;
the runtime review is `scratch/observer_header_only_runtime_review_20260905.md`,
SHA-256
`94bbb8a5972c74a4c028b1d7bdfa2dc5b571d40bb4009d6d59d31146a32e64a2`.

## January semantic corrections to the first natural draft

The first draft was preserved before review.  Corrections were made only for
concrete behavioral or layout defects established from January instructions or
executable target behavior, never to chase registers or spelling:

1. An invalid pending command does not overwrite `last_command`; the copy is
   inside the valid-command branch.  The corresponding January assertion line
   is 0x172.
2. The fifth-order remaining-time polynomial uses `displacement*inverse_time^5`
   and `velocity*inverse_time^4` in coefficient `a`.  The frozen HCEA donor's
   lower powers were wrong except at unit duration.  All six symbolic endpoint
   identities and 48 rational tests pass after the correction.
3. Rotational displacement occupies derivative elements 8 through 10.  It is
   not written to the later `rotation_offset` field.
4. Position expiry preserves January's unordered/NaN topology: a valid command
   snaps when `!(remaining_time > 0)`, and the later delta application is
   controlled by the explicit snap state rather than a contradictory
   `remaining_time <= 0` test.
5. Postcheck validates the water-adjusted result before the final PIN clamps
   and result copies, matching the observable assertion order.
6. The penetration refinement tail retains January's unusual asymmetric
   branch:

   ```c
   if (last_miss_t < last_hit_t)
       upper = lower;
   else if (upper < 0.f)
       upper = -upper;
   ```

   An earlier review suggestion to apply unconditional ABS was retracted after
   executable target testing.
7. Timer decay uses the ordered-positive predicate so NaN clamps to positive
   zero.  Velocity expiry likewise uses `!(remaining_time > 0)`.
8. The orientation repair uses ordinary cross products and normalization,
   removing a demonstrably malformed donor expression.
9. `observer_apply_rotational_displacement` is a genuine private helper called
   by position integration.  It is neither manually expanded nor forced
   out-of-line.

The independent polynomial ledger is
`scratch/observer_polynomial_equation_review_20260905.md`, current SHA-256
`5922a4a4496a4af35d136884fc47e90eeaf3f39c042b3b80d7e166f01eb1d1c5`.
HCEA is corroborating reconstruction evidence only; January remains the sole
byte/ABI authority.

## Behavior evidence

Three saved executable checks cover the public and private behavioral
boundaries:

| Check | Result | SHA-256 |
| --- | --- | --- |
| public dispatch, 16 player masks, six time deltas, and four assertion cases | 100/100 pass | `ecf7a1a408d128241302f4d9561c9eeab507c35d03c8e83d0be26d38194fd5d0` |
| freeze/time/NaN/integrator order and derivative-count cases | target and candidate pass | `e4fd06e0a386c47a902ef404db8170611ec015025dccc4b39dd936058131f072` |
| direct, negative-refinement, and positive-refinement penetration cases | target and candidate pass | `fc46c1bf8a768417eeed5c556e2609ed6e12ef9e0fb4a6dc4ba9bc0a361a6a9e` |

The penetration check also verifies endpoints, underwater propagation,
collision-call counts and fractions, callee-saved registers, and stack
balance.  The independent final review is
`docs/object_matching_logs/observer_obj_integrator_penetration_independent_review_20260905.md`,
SHA-256
`8cc456f7abf43aad15c856d1f72d25d8e3cefc137d83964850e2fde676cccf91`.

Direct component arithmetic in penetration and postcheck was selected as the
readable algorithmic reconstruction before emission was inspected.  HCEA
independently corroborates those affine point expressions, and January emits
the corresponding scalar sequences with no `point_from_line3d` relocation.
No helper-call candidate was compiled and no source was changed in response to
the guard.  The final object emits no `_point_from_line3d`.

## Function measurements

The natural residual measurements are frozen as follows:

| Function | January bytes/relocs | Candidate bytes/relocs | Disposition |
| --- | ---: | ---: | --- |
| `observer_check_penetration` | 752 / 11 | 688 / 10 | park |
| `observer_collision_test_with_t` | 224 / 15 | 208 / 15 | park |
| `observer_find_displacement` | 112 / 1 | 176 / 1 | park |
| `observer_pass_time` | 160 / 13 | 160 / 13 | park |
| `observer_postcheck` | 2112 / 104 | 1968 / 107 | park |
| `observer_update` | 304 / 24 | 304 / 24 | relocation/literal identity residual; park |
| `observer_update_accelerations` | 464 / 13 | 528 / 13 | park |
| `observer_update_command` | 1040 / 33 | 1056 / 35 | park |
| `observer_update_displacements` | 80 / 6 | 80 / 6 | park |
| `observer_update_polynomial` | 1440 / 48 | 1488 / 46 | park |
| `observer_update_positions` | 1568 / 40 | 1280 / 28 | park |
| `observer_update_velocities` | 512 / 14 | 544 / 14 | park |

Park percentages must be generated from the actual canonical production object
after integration.  This scratch review does not propose configuration edits
or pre-admit any fuzzy function.

## Complete code-owner boundary

The saved census reports baseline 23 code / 14 non-code / 0 COMMON owners and
candidate 41 code / 57 non-code / 0 COMMON owners.  Every inherited code and
non-code owner is unchanged and none is lost.  The 18 new code owners are the
fourteen intended functions plus four naturally emitted inline-header bodies:

| Helper | Candidate bytes/selection | January selected owner | Current disposition |
| --- | --- | --- | --- |
| `sine` | 16 / SELECT_ANY | Actor Combat / NODUPLICATES | unresolved; natural owner cleanup loses two Actor Combat stricts |
| `cosine` | 16 / SELECT_ANY | Actor Combat / NODUPLICATES | unresolved; same boundary |
| `valid_real_point3d` | 96 / SELECT_ANY | Particles / NODUPLICATES | natural selected-owner cleanup passed focused and full regression checks |
| `vector_from_points3d` | 48 / SELECT_ANY | Action Alert / NODUPLICATES | natural selected-owner cleanup passed focused and full regression checks |

Each candidate helper body is byte-identical to its selected January owner,
but byte identity does not by itself establish link compatibility.  The final
math-owner census scans all 833 target and 572 canonical objects with zero
parse failures.  Its JSON is
`scratch/observer_reviewed_math_owner_census_20260905.json`, SHA-256
`67d8535adcd427c268aed32c4c583e219520f37463687c795c35135d1aaad09c`.
The full frozen owner audit is
`scratch/observer_reviewed_owner_audit_20260905.json`, SHA-256
`6c6f2fbd58f68eef2d81ae7d1cb57be9e0ca7499d008bd697e79a97a78a9c0e8`.

Action Alert and Particles cleanup removes genuine duplicate non-inline
definitions while preserving their inherited function packets and resolving
their tested pair-link conflicts.  That does not resolve the separate Actor
Combat sine/cosine selected-owner boundary.  Observer integration must be
re-censused against the final normalized owner set and pass the ordinary link
checks; this ledger grants no whole-object approval.

The final 7EB0 Observer object was also linked with the actual normalized
Action Alert and Particles objects as one three-object aggregate in both
orders.  The frozen normalized inputs are
`scratch/math-owner-schema-actual-action_alert-20260905.obj`, SHA-256
`c15f370e118de1a78c8cbcab7e3d4cd07f255d2c50af6c4b81e7c2cf391b46da`,
and `scratch/math-owner-schema-actual-particles-20260905.obj`, SHA-256
`bd089e80873d936e39ac8adb641fa146162756652c77a97a45f71ff6e868ef45`.
Neither Observer-first nor owners-first reports any `LNK2005`; both proceed to
the expected `LNK1120: 70 unresolved externals` and produce no output.  This is
a bounded duplicate-owner PASS for the resolved `vector_from_points3d` and
`valid_real_point3d` owners, not a successful whole-program link.

| Aggregate-link evidence | SHA-256 |
| --- | --- |
| `scratch/observer_normalized_math_owners_link_audit_20260905.py` | `5b6de520506d3bcbabb497ce9a6aa7d1de02d063c1fa61e75f503b54f576cb65` |
| `scratch/observer_normalized_math_owners_link_audit_20260905.json` | `94318758c42a1aa37c1e0ea32dcf172989310161f1a19f876339acea4652fb4d` |

The aggregate script uses the same ordinary linker policy as the failing
Actor Combat proof and preserves both complete diagnostic logs.

The exact Actor Combat pair boundary is no longer inferential.  The durable
link script and full output record are:

| Artifact | SHA-256 |
| --- | --- |
| `scratch/observer_actor_combat_sine_cosine_link_audit_20260905.py` | `f6a0203d37be7feb65f6c50f44a003fb0ee2df96d2314739435d9201a922d3f1` |
| `scratch/observer_actor_combat_sine_cosine_link_audit_20260905.json` | `b6ccf86eccfbddc39354af0f7032ea5b6110ab564d392e332a86c7f388c63f83` |

Both runs use ordinary XDK VC7 `Link.Exe /DLL /NOENTRY /NODEFAULTLIB` with
the two unmodified objects and no `/FORCE`, `/IGNORE`, synthetic stubs, object
patches, or metadata patches.  Results are:

| Input order | `LNK2005` names | unresolved diagnostic lines | Final result |
| --- | --- | ---: | --- |
| Observer, Actor Combat | `_cosine`, `_sine` | 8 `LNK2001` + 47 `LNK2019` | `LNK1120: 46 unresolved externals`, no output |
| Actor Combat, Observer | `_cosine`, `_sine` | 5 `LNK2001` + 50 `LNK2019` | `LNK1120: 46 unresolved externals`, no output |

The raw diagnostic-line counts exceed the unique unresolved count because one
unresolved symbol can be referenced more than once.  The JSON preserves both
complete linker logs and the exact commands.

## Static data boundaries and deferred ownership

January's 22-byte writable contribution is fully understood and byte-equal to
the candidate:

| Offset | Size | Semantic owner | Values |
| ---: | ---: | --- | --- |
| 0 | 10 | `observer_parameter_real_counts[5]` | 3, 3, 1, 1, 6 |
| 10 | 2 | natural alignment gap | zero |
| 12 | 10 | `observer_parameter_derivative_real_counts[5]` | 3, 3, 1, 1, 3 |

January use sites prove the two table roles and boundaries.  A later HCEA
applied-symbol database independently gives the same semantic names and
relative addresses.  The names are corroborated; HCEA does not prove January
linkage.  Static linkage remains a same-translation-unit-use inference.

January's adjacent 28-byte read-only contribution has three distinct semantic
boundaries:

| Offset | Size | Semantic owner | Candidate status |
| ---: | ---: | --- | --- |
| 0 | 20 | `observer_maximum_accelerations[5]` | exact byte prefix and genuine static array |
| 20 | 4 | `seconds_per_tick` | target-only and unreferenced by this cluster |
| 24 | 4 | `sine_region_angle` | exact pooled literal bytes; not a distinct candidate owner |

The current scratch source spells the last value as the readable
`OBSERVER_SINE_REGION_ANGLE` constant.  January bytes plus the later symbol
database support the separate `sine_region_angle` boundary, but they do not
justify fabricating a 28-byte aggregate or restoring an unused
`seconds_per_tick` merely to reproduce contribution shape.  Root explicitly
deferred those semantic data-owner decisions.  Consequently this packet claims
no data credit and no whole-object data match.  The data audit is
`scratch/observer_reviewed_data_owner_audit_20260905.json`, SHA-256
`ff97868b0cd9d37ae718d1e43f75afa9cb0cadf7322470767b8177b23eb18c4f`.

The other new non-code owners are natural assertions, literals, math constants,
and three inherited XDK tables.  They require the ordinary pooled-owner and
runtime-link audit before any object admission; none receives credit here.

## Integration checklist

Before this packet may enter canonical source, root should require all of the
following in one controlled integration:

1. Resolve the Actor Combat NODUPLICATES versus Observer SELECT_ANY sine/cosine
   boundary through a genuine owner/caller reconstruction or demonstrated safe
   ownership topology.  Do not force helper emission or disappearance.
2. Apply the already validated owner-schema hunks and the self-contained
   production body manifest only; do not copy scratch replacement headers.
3. Regenerate authentic private symbol metadata atomically and gate without
   diagnostic aliases.
4. Preserve the 11 inherited strict Observer owners, all current global strict
   owners, and the current park set.
5. Re-run the public dispatch, integrator, and penetration behavior suites on
   the actual production object.
6. Re-run complete code/non-code/COMMON and ordinary-link audits, with the hard
   `_point_from_line3d` emitted-symbol guard.
7. Measure the twelve parks from the production object.  Leave the 64-byte
   differential helper unwritten unless a genuine caller/source closure later
   retains it.
8. Keep the 22-byte count-table evidence and 28-byte read-only boundaries as
   explicit data accounting; do not claim an aggregate or whole-object match.

No production Observer body, data declaration, target metadata, park record,
build output, commit, or GitHub ref was changed by this reconstruction lane.
