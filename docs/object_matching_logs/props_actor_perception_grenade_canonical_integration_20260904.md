# Props, Actor Perception and grenade helper: canonical reconciliation

## Frozen result, 2026-09-04

Starting canonical/GitHub publication is `0ac295264659381fe8d3bb234588e01483f5ffde`.
This packet selectively admits two missed older-lane improvements and corrects
one unsupported historical exact implementation. The new active Fable 50k
lane and its assigned objects remain excluded.

| Change | Meaningful exact bytes | Padded exact bytes |
| --- | ---: | ---: |
| Old Fable `prop_new_unacknowledged` iterator recovery | +544 | +544 |
| Old Opus `actor_get_grenade_definition` recovery | +56 | +64 |
| Remove unsupported `actor_perception_desire_prop` carrier-union exact claim | -549 | -560 |
| **Net** | **+51** | **+48** |

Verified production totals are **898,738 / 2,198,102 meaningful credited code
bytes**, **6,142 / 11,060 credited functions**, **2,022,998 data bytes**, and
**391 / 833 Matching objects**. No whole-object completion is claimed.

The stable sweep is **6,188 / 8,245 exact owners**. It explicitly reports two
gains and one loss: Actor Perception target section 6. That loss is the
deliberate policy correction below, not hidden, excluded from the snapshot,
or relabeled as a zero-regression result. All other inherited strict owners
remain strict. No collateral exact regression is accepted.

## Props and the credible public API

The old Fable iterator form is a genuine missed source lead. It uses the
existing typed prop iterator instead of the hand-expanded traversal, preserves
the same replacement selection and datum reuse, and calls the private
`prop_add` with the authenticated actor/unit/prop source order. The correct
API context retains its strict 544-byte match, including all 21 ordered
relocations. Props is now **14 exact / 3 residual / 0 unwritten**.

The former caller-local `actor_perception_desire_prop` declaration disagreed
with its definition in two parameter types. Root moves the authentic
13-parameter declaration into the existing Actor Perception section of
`actors.h`, with a `short desired_target_state` and plain `long` actor handle,
then removes the conflicting declaration from Props. There is no dedicated
Actor Perception header, so this follows the closest-owner-header rule.

The old exact callee encoded compiler stack-slot reuse as a public carrier
union. January first consumes a long actor handle, then reuses its dead
parameter-home top byte for a boolean; that is not evidence of a source union.
The corrected implementation uses an initialized separate `boolean too_far`.
It also replaces all actor/encounter shadow casts in this function with the
existing genuine shared datums, fixes misleading field names through actual
owner fields, and includes `actions.h` for `actor_action_class` rather than
declaring that API locally. HCEA DIA, caller transitions and January accesses
corroborate the primitive types and Props-state domain. No header layout or
enum-placement changes are made to influence code generation.

Actor Perception is now **32 exact / 3 residual / 9 unwritten**. Its corrected
callee remains 560 padded bytes / 16 relocations, at **97.791664%** similarity;
the explicit ordinary local uses a local stack slot rather than the old
argument home. All other 32 exact functions and the two inherited residuals
remain unchanged. The corrected body is retained as a measured honest park,
not an exact implementation. This is a bounded function/API repair, not an
approval of every inherited shadow view elsewhere in that translation unit.

## Actor Combat owner recovery

The old ruling that the grenade-definition abstraction had no January owner
was wrong: file offset 63776 has a genuine 64-byte target section. The
restored private helper uses existing typed game-global/tag access and is
naturally retained by the two real grenade callers. Its semantic name
`actor_get_grenade_definition` is corroborated by pinned HCEA, while January
remains the byte authority. The target mapping is atomically marked static.
Actor Combat advances to **19 exact / 6 residual / 9 unwritten**.

The natural caller change also improves `actor_combat_reaim_grenade` from
98.509674% to **99.86452%**. Its previous park was correctly flagged stale by
the validator; root did not suppress that failure. Independent aligned review
confirms the changed cleanup/move schedule and remaining frame/stack-slot and
velocity-product scheduling differences. Target and candidate remain 448
padded bytes, 16 relocations and 162 instructions. The park is explicitly
remeasured, conservatively unclassified, and receives no exact credit. No
spelling, declaration-order, compiler-option or forced-inline search is used.

## Ownership and verification

All three changed translation units pass the complete forbidden-symbol guard:
no `_point_from_line3d` code definition is emitted. Props retains its existing
extra vector helper; Actor Perception retains ten existing math/prop helpers;
Actor Combat retains its four existing math helpers. None is introduced by
this packet or waived into a whole-object proof. Named runtime non-code owners
are unchanged; the net data-byte gain is zero.

The full generated build passes after the proper owner-header rebuild and
park remeasurement. Units remains **189 exact / 0 residual / 0 unwritten**;
its separate inherited candidate-only helper veto remains in force. All
**256 parks validate**, with zero stale or invalid entries. Admission reports
zero candidates, contradictions or revocations, and four otherwise-complete
reviewed rejections. The changed-source fake-match scan has zero leads.
**288 tests and 26 subtests pass**, with only the existing optional pytest-cache
permission warning. Both protected bitmap hashes are unchanged.

```text
ninja
python -m tools.campaign.stable_verdicts snapshot scratch/props-perception-grenade-final-after-20260904.json
python -m tools.campaign.stable_verdicts diff scratch/pas-current-replay-after-20260904.json scratch/props-perception-grenade-final-after-20260904.json
python -m tools.parked_functions --output scratch/props-perception-grenade-final-parks-20260904.json
python tools/audit_object_admission.py --output scratch/props-perception-grenade-final-admission-20260904.json
python tools/fake_match_scan.py source/ai/props.c source/ai/actor_perception.c source/ai/actor_combat.c source/ai/actors.h
python -m pytest -q tools
python configure.py progress
git diff --check
```

The stable diff intentionally exits nonzero for the documented policy
withdrawal; its full output must show no other loss. Per-unit gate artifacts
are `scratch/props-final-owner-gate-20260904.obj`,
`scratch/perception-final-owner-gate-20260904.obj`, and
`scratch/grenade-final-owner-gate-20260904.obj`. Proprietary/scratch artifacts
are not committed. The source and independent ABI/owner evidence are preserved
in `props_obj_old_fable_iterator_reconciliation_20260904.md`,
`actor_perception_obj_props_api_typed_followup_20260904.md`, and
`actor_combat_obj_grenade_definition_opus_reaudit_20260904.md`.

Physical production source SHA-256 values:

- Props: `69811b5a64bcfb06779f154e5368f148cf831155f147555a8ac0abe607147a2c`.
- Actor Perception: `719121b37059205ef073a29a7c2bc518b4a2cbeb604b0eb469e122036f2ced71`.
- Actor Combat: `6ca4d6545abead50bff82334f3b52a52736072ba34b624e6c2b97f66682c0b3a`.
- `actors.h`: `04a1a641eeccb4514c699057dad8601395fcd4037096b80170c1b0ecfbcf4a06`.
