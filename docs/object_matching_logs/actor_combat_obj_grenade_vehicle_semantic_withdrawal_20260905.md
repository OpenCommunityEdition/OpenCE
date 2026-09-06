# Actor grenade aim: vehicle-path semantic audit

## Verdict

**WITHHOLD semantic acceptance of the existing vehicle path.** Current C has
an uninitialized working-vector read whenever `actor->input.vehicle_index`
is not `NONE`. The January function also has an uninitialized-stack possibility
for some such inputs, and on its vehicle-plus-retarget path it instead consumes
the stack words previously occupied by the target point. That machine dataflow
is not sufficient evidence that an absolute world position was intended to be
a velocity direction.

There is a genuine January no-vehicle guard at AI grenade *selection*. This
audit did not prove that it remains invariant through the later release event.
Accordingly, the current owner is **semantic-uncertain, with a known source
def-use regression on the vehicle-plus-retarget machine path**, not an approved
representation-safe fuzzy match. No production edit, compilation, gate, build,
source variant, or accounting change was performed during the initial audit. Active Fable trees were
not inspected.

## Pinned current source and regression

Repository root:
`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/rasterizer-model-wrapper-alias-20260829`.

`source/ai/actor_combat.c` SHA256:
`6ca4d6545abead50bff82334f3b52a52736072ba34b624e6c2b97f66682c0b3a`.

- Function begins at line 1047; `real_vector3d aim_vector;` is line 1053.
- Lines 1069-1074 create a separate block-local typed point for retargeting.
- The only initial assignment to `aim_vector` is line 1084, inside the
  `vehicle_index == NONE` condition at line 1080.
- Lines 1120-1122 unconditionally read all three components.

Commit `1999496142a3994b5e9c9f2fd22bd954f42b4bf7` ("Clean up actor grenade
target typing") specifically removed the three assignments to
`aim_vector.n[0..2]` in the prop-target branch and replaced them with the
separate typed point. It correctly removed a point/vector pointer cast, but
did not preserve the subsequent working-vector dataflow. `git show` of that
commit proves the exact hunk; this is not an inference from a fuzzy score.

The 2026-09-02 small-family ledger claims its then-current point/vector reuse
eliminated the older uninitialized rendering. That was already incomplete for
vehicle inputs without a retarget branch. The 2026-09-04 dirty-cleanup ledger's
blanket claim of no undefined behavior is not supported by the retained C.
Neither ledger is a substitute for a release-time precondition proof.

## Complete January function review

Target: `build/split/source/ai/actor_combat.obj`, SHA256
`6030e4891bf64d0cb0c41028a6c816ef07c4442f6c573ac70f37fb1e049ca45b`.
`_actor_aim_grenade` is the public function at file offset `0x111B0`, 544
padded bytes, 22 relocations, normalized SHA256
`e0f88f129bff43bffd1418d177c79d4849c02f0e71e1acf1cf188c16030193fb`.
All instructions from offset 0 through the return at `0x21E` were inspected.

The working output words are `[ebp-0x24]`, `[ebp-0x20]`, `[ebp-0x1C]`.

| January offsets | Def-use fact |
| --- | --- |
| `0x19..0x2C` | If `grenade_current_prop_index` at actor+`0x6B4` is `NONE`, skip the entire prop block. |
| `0x3B..0x5D` | Read signed prop state; states 2..3 supply target unit; states outside 0..1 enter retarget block. |
| `0x5F..0x82` | Copy prop body position at prop+`0xBC`; add 0.2 to Z; store all three working words. |
| `0x85` | Call the real retarget helper with the address of those words. |
| `0x8B..0x90` | Call the real reaim helper using actor handle and origin, not a pointer to the working local. |
| `0x95..0xA1` | Compare actor+`0x158` (vehicle index) to `NONE`; non-NONE jumps directly to `0x1F8`. |
| `0xA7..0xC5` | Only on-foot: copy actor+`0x6BC..0x6C4` current aim into all three working words and copy XY to a separate normalized temporary. |
| `0xC8..0x1F5` | Optional on-foot facing clamp/rotation, normal validation, and replacement of the working words. |
| `0x1F8..0x215` | Multiply the three working words by actor+`0x6C8` aim speed and write output, with no further vehicle check or initialization. |

Thus January has three distinct input classes:

- On-foot: working vector is initialized from the refreshed actor aim;
  target-state selection does not affect this initialization.
- Vehicle plus retarget branch: output consumes the earlier point-shaped
  words, even if retarget validation itself rejects updating the actor target.
- Vehicle without retarget branch: this function has written none of those
  three local words before output reads them.

The retarget and reaim helper calls are not hidden initializers of an arbitrary
unpassed C local. Reproducing stack-slot contents using casts, unions, raw
memory, or position-as-velocity assignments would be a machine-shape workaround,
not a justified semantic repair.

## Genuine callers and authenticated selection guard

`config/relocs.json` SHA256
`a148bf95e1da1be704e42a10ae08c7e6100b677a61d572b05f00e0fb03a87a2a`
has exactly one incoming reference to `_actor_aim_grenade`:
`_unit_throw_grenade_release + 0x7B` (relocation operand offset), source file
offset `0x19AA6B`. This is the call at instruction offset `0x7A`.

`build/split/source/units/units.obj` SHA256:
`de70778800ba600aa7365d9fa789b2d1147baf3b67e50d2f9a70ff958c9fa757`.
The complete January entry-to-call path checks only:

1. grenade throw state at unit+`0x23D` equals in-hand (2),
2. grenade object at unit+`0x244` is present,
3. actor index at unit+`0x1A4` is present.

It detaches the grenade, obtains its origin, and calls actor aim. It does not
check the actor vehicle field or unit parent before this call. Existing
canonical `_unit_throw_grenade_release` is strictly identical to January;
normalized SHA256
`3938e1501fc288b40037125c290f98765dcdc08609482d5181fc758d630fb659`.

January release has four incoming call sites:

- `_unit_animation_set_state + 0xBA`;
- `_unit_update_animation + 0x1FB`;
- `_code_001a0cf0 + 0x395` (damage/death animation path);
- `_unit_update + 0xDB9`.

There **is** an authentic earlier guard. In January Actions,
`_actor_action_test_grenade + 0x4F` compares actor+`0x158` to `NONE`;
the non-NONE path `0x58..0x69` records debug decision zero and returns FALSE.
This existing canonical owner is strict, normalized SHA256
`7bd493a9cf4e7118d48a9bbe6762e77fb025e22ea92c9bb892f04ede6724efdf`.

The strict `actor_action_consider_grenade` calls try-to-throw with
`known_trajectory=TRUE` immediately after successful test. The strict
try-to-throw re-runs the test when `known_trajectory=FALSE`. This authenticates
ordinary AI *initiation* on foot. The `actor_combat_update` January call at
`+0x27B` then forwards the throw order; the strict 48-byte
`actor_unit_control_throw_grenade` only ORs the throw control flag and has no
vehicle check.

January Actions object SHA256:
`624bb4506adff68e9888a20e44042f28ec7f7e4fdc33115c64c25beb1270b5b5`.
January Actors object SHA256:
`e07c0f6375faf94da001d8d594aeae65fc57c1597cadf74323f3b1038c7c8e95`.

## Precise remaining lifecycle gap

The selection guard is not yet a release-time invariant:

- Strict `unit_throw_grenade_begin` disallows entering/exiting-seat animation
  states but has no general parent/vehicle check.
- Strict `unit_can_enter_seat` checks self-entry, seat occupancy and player
  enemy occupancy; it does not reject an in-hand grenade or a busy animation.
- Strict `unit_enter_seat` attaches the unit to its parent before its optional
  entering-seat animation, then reports AI vehicle entry. Its direct body
  neither tests in-hand grenade state nor calls grenade release.
- Strict `unit_update` performs premature release when in-hand and animation
  no longer equals throw-grenade. The actor input sampler derives vehicle index
  from its parent vehicle.

These observations identify a plausible transition question, not an executed
gameplay counterexample. The ordering of actor input sampling, forced/scripted
seat entry, animation change, and release remains unproven. Indirect side
effects of entry helpers and content/animation restrictions also need a
bounded lifecycle proof before labeling such states valid or impossible.
No active Fable AI-script source was inspected to fill that gap.

## Safe handoff, not a candidate patch

Keep the already parked owner explicitly semantic-uncertain and withhold a
"representation-safe / complete" claim. Preserve the independent strict
ActorCombat gains; this audit offers no reason to revoke them. The current
grenade aim owner was already residual, so this diagnosis grants no exact
bytes and does not itself change exact-byte totals.

Do not copy the HCEA lift's zero initialization: it is a later reconstruction
convenience without January support. Do not unconditionally use current aim
and call that byte-equivalent: it changes the target's vehicle-plus-retarget
machine behavior. Do not restore the point/vector cast or equivalent explicit
position-as-velocity carrier.

If a complete release-time `vehicle_index == NONE` invariant is authenticated,
a natural initialization from the post-reaim current aim can be considered
under that valid domain, with honest off-domain documentation and a fresh
owner-local fuzzy park. Until then, no production correction is justified by
this evidence. A deliberate gameplay bug fix outside January equivalence
requires an explicit policy decision rather than an opportunistic match.

The next proof should trace the real in-hand grenade lifecycle through seat
entry and actor input refresh, not compile source variants. Any eventual
single-function correction must preserve all inherited strict owners, report
its actual residual change, and retain the no-new-point COMDAT boundary.

## Root accounting disposition

Root read the complete audit and its separate accounting review. The live
`unclassified` park is withdrawn from `config/parked.json`; an approved park
requires credible semantics, and the validator's measurement checks alone
cannot supply that approval. No unsupported status/class was invented and
no validator or matcher was weakened. The C body is unchanged pending a real
lifecycle proof; removing it could also affect genuine private helper owners.

The withdrawn measurement was target 544 padded bytes / 22 relocations versus
base 576 / 22, base normalized SHA256
`e9bba5b05f536e0aafd56e6fa491a8560347116525e58787e26f295b0049a264`,
at 82.03977%. Its 543 meaningful target bytes had no exact credit. The old
park evidence in commit `616569bb1` is superseded, including its stale claim
that the separately re-audited grenade-definition helper was not retained.
Both the September 2 small-family and September 4 dirty-cleanup ledgers now
have dated corrections instead of rewriting their historical measurements.

Strict park validation now reports 282 active, zero stale and zero invalid
records. Exact accounting remains 907,050 code bytes, 6,174 functions,
2,031,312 data bytes and 391/833 Matching objects. All 288 tooling tests plus
26 subtests pass. This withdrawal removes no exact gain.

Removing a park also removes its canonical per-function veto on external
treemap exact claims. Root checked the live all-lanes view immediately before
and after withdrawal: this function remains 82.03977%, canonical/best/union
exact all false, no contributors and no semantic-accepted variant. Only
`policyParked` changes true to false. No external completion claim is exposed
by this refresh. Future lane reconciliation must explicitly revisit this
semantic issue; the current tools have no dedicated persistent rejected-
function policy, and this result does not imply all residuals are approved.
