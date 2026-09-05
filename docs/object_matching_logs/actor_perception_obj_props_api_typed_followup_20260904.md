# Actor Perception / Props primitive API correction, 2026-09-04

## Scope and decision

This follows the frozen old-Fable Props inventory in
`props_obj_old_fable_iterator_reconciliation_20260904.md`, whose canonical
baseline is `18238d9065014b67fa2a1d745140f203be6e44dd`.
All implementation here is **scratch-only**. No production C, header,
configuration, Ninja/build-directory output, commit, or push was changed.

The existing exact `actor_perception_desire_prop` contains an unauthenticated
parameter-home carrier union. Its public signature is inconsistent with the
Props caller. The correct ordinary source uses a `long` actor handle and a
separate initialized `boolean too_far`. That correction loses the old strict
match while preserving the logic. A second, bounded typed-access correction
replaces this function's actor and encounter shadow casts with existing real
shared datums and properly includes the `actor_action_class` owner header.

Final scratch result: **32 exact / 3 residual / 9 unwritten** versus the
inherited **33 exact / 2 residual / 9 unwritten**. Only
`_actor_perception_desire_prop` changes. This is an explicit source-policy
correction of a formerly exact implementation, not an accidental regression
and not new code-byte progress. No declaration/allocator search was made to
restore the parameter-home reuse.

## Authentic public ABI

Bounded DIA command, run from canonical:

```powershell
& '..\..\research\tools\DIA2Dump\x64\Release\DIA2Dump.exe' -sym actor_perception_desire_prop '..\..\research\hcea_jun2011_prototype\payload\Halo CE Anniversary (Jun 24 2011)\HCEX.pdb'
```

The HCEA record names 13 parameters: `long actor_index`,
`short desired_target_state`, `long unit_index`, `long unit_actor_index`,
four `unsigned char` flags, `short dead_ticks`, two floats,
`short required_ticks`, and `unsigned char *optional_reference`.
It contains no union argument. A public symbol for the function is present.
The later source corroboration is
`../../research-cache/halocea-full-blobs-20260830/src/blam/ai/actor_perception_desire_prop.c`.
January remains the ABI/byte authority; the later record is corroboration.

January `build/split/source/ai/actor_perception.obj` proves:

- `+0x17` reads the entire fourth parameter from `[ebp+0x14]`; it is compared
  with `NONE` and used directly as the related actor datum handle.
- `+0x3b` reads the second parameter as a word from `[ebp+0xc]`.
- Only after the fourth parameter's last handle use, `+0x42` clears its top
  stack byte `[ebp+0x17]`; the later body stores a Boolean there and returns
  that byte through the optional output pointer at `+0x21a` / `+0x21d`.
- The function returns its separate desire result in AL. This is compiler
  stack-slot reuse, not evidence for a source union in a public API.

The original TU-private union overlays `long actor_index` with an anonymous
`{ byte padding[3]; boolean too_far; }` structure. The frozen correction
removes it, changes the fourth formal to `long prop_actor_index`, and uses a
normal initialized local `boolean too_far`. The second formal is
`short desired_target_state`, retaining the independently recovered DIA name.
The proper declaration belongs in the existing
`ACTOR_PERCEPTION.C` section of `source/ai/actors.h`, not in Props and not in a
new caller-local declaration. Root owns that later header change and the
removal of Props' incompatible local declaration.

## Real shared layout and typed access

Every field used in this function is available in existing `actor_datum`,
`encounter_datum`, and `unit_datum`. No layout/header modification is needed.
The inherited view sometimes gave these slots misleading semantic names.

| January offset | Genuine owner field | Replaced shadow spelling |
|---|---|---|
| Actor `0x8` | `meta.active` | `active` |
| Actor `0x13` | `meta.dormant` | `dormant` |
| Actor `0x34` | `meta.encounter_index` | `encounter_index` |
| Actor `0x6a` | `state.mode` | `combat_status` |
| Actor `0x6e` | `state.combat_status` | `artificial_combat_status` |
| Actor `0x1cc` | `external_orders.pursuit_is_coordinator` | `corpse_interest_inhibited` |
| Actor `0x3a0` | `emotions.corpse_ignore_time` | `corpse_ignore_time` |
| Encounter `0x42` | `stand_down` | `blind` |
| Encounter `0x44` | `enemy_alive` | `stand_down` |
| Encounter `0x45` | `enemy_visible` | `enemy_target` |
| Encounter `0x58` | `corpse_ignore_time` | `corpse_ignore_time` |
| Unit `0x3cc` | `unit.time_of_death` | Already genuine typed field |

The named fields are independently corroborated by the pinned HCEA body.
The exact byte offsets are proved by January instructions and by a separate
scratch-only compile-time `offsetof`/type-width check:
`scratch/actor-perception-owner-layout-check-20260904.c`. It compiles under
the current normal XDK edge. Its lack of implementation is intentional; it
is a type-proof artifact, never a candidate or accounting input.

The final source uses existing `actor_get`, `encounter_get`, and `unit_get`
typed macros directly. It includes `actions.h` and removes the duplicate
caller-local `actor_action_class` declaration. Its threshold now uses the
owning `_action_class_passive` enum, and orphan state bounds use the existing
Props enum. January's floating comparisons and branch logic are untouched.
No stack storage cast, aliasing view, assembly, forced inline, volatility,
padding, synthetic helper, or source-order control remains in this function.

### Enum domain cross-check

The parameter name `desired_target_state` does not alone establish the
`actor_target_type` priority enum: that enum and the Props state enum happen
to have orphan entries at 4 and 5 in opposite inspected/uninspected order.
The pinned `actor_perception_update.c:311` and `:335` call this routine with
`_prop_state_uninspected_orphan` while deciding whether to call
`prop_orphan_transition` or change the prop to becoming-unacknowledged.
`actor_perception_refresh.c` and `prop_status_refresh.c` pass `NONE`;
`actor_perception_refresh_test_object.c` passes zero for a newly observed
object. This supports the **requested Prop state** domain retained here.
The original exact enum spelling is not claimed to survive in the PDB:
DIA declares only `short`.

January has no same-TU relocation to the standalone desire-prop function;
those calls are folded into callers. In January `actor_perception_update`,
the folded desire test at `+0x989..+0xa1a` precedes the real
`prop_orphan_transition` call at `+0xa40`, then the replacement is installed
by `actor_switch_props` and the original prop state becomes zero at `+0xa5b`.
This is the same prop-state transition context, not an actor target-priority
assignment. There is no reason to relabel the comparison as actor-target
priority solely to match the word "target" in the formal's name.

The mode `< 3` and combat-status `>= 4` thresholds correspond to genuine
existing `_actor_mode_combat` and `_actor_combat_status_certain` constants,
but their definitions currently live privately in `source/ai/actors.c:322`
and `:389`. This packet does not duplicate them locally or move those enums
without owner authorization; root may separately move the genuine complete
domains into the owning header and verify the resulting header consumers.

The earlier scalar-only and final typed implementations have **identical
function section information**. This independently checks that the real
owner fields above preserve all accessed offsets, types, and ordinary logic.
Other functions and their inherited views are untouched; this is not a
whole-file source-credibility or Matching approval.

## Frozen artifacts and reproduction

| Artifact | SHA-256 |
|---|---|
| January `build/split/source/ai/actor_perception.obj` | `fbbba614819baf2d042c4b0fa0180a93c1ad4507788579a3c2bb8774ee2997da` |
| `scratch/actor-perception-owner-baseline-20260904.obj` | `04bd295eeab08ad75001bb3649e380a939a7aadea8d3fd36c5aa26270e904473` |
| `scratch/actor-perception-owner-natural-proposal-20260904.c` | `737396b6e7f10bc013c33509af431e6bbd9ba09edb0ad2295061a416feed4413` |
| `scratch/actor-perception-owner-natural-proposal-20260904.obj` | `563348cb3fd5709f0f52e8683fcce0b0b10edded7cae725d1e33a6c7771c06e8` |
| **Final** `scratch/actor-perception-owner-typed-proposal-20260904.c` | `a3706b1756bf503d01d86d6aa934bdee346cd9a5cd13a7393fb515c4a0c916a3` |
| **Final** `scratch/actor-perception-owner-typed-proposal-20260904.obj` | `ce72d2e437b5e2433227a07b7a07e022a299134d7c79f2a4c7c6d1213fecae43` |
| `scratch/actor-perception-owner-layout-check-20260904.c` | `ee531743212350d76515945e401041021e45ec7466efa30895faed4208717fe9` |

```powershell
python tools/campaign/gate.py source/ai/actor_perception --out scratch/actor-perception-owner-baseline-20260904.obj --all --forbid-emitted-symbol _point_from_line3d
python tools/campaign/gate.py source/ai/actor_perception --source scratch/actor-perception-owner-typed-proposal-20260904.c --out scratch/actor-perception-owner-typed-proposal-20260904.obj --all --forbid-emitted-symbol _point_from_line3d
python tools/campaign/gate.py source/ai/actor_perception --source scratch/actor-perception-owner-layout-check-20260904.c --out scratch/actor-perception-owner-layout-check-20260904.obj --fn _actor_perception_desire_prop --forbid-emitted-symbol _point_from_line3d
python tools/campaign/alndiff.py source/ai/actor_perception _actor_perception_desire_prop --ours-object scratch/actor-perception-owner-typed-proposal-20260904.obj
```

All emitted-symbol guards pass. January and inherited exact desire-prop:
549 meaningful / 560 padded bytes, 16 relocations, normalized SHA-256
`f9729e40ecdc2cbd04fcb50024e74431499a7f69d9d071ca13b8cf6eea3def13`.
Both natural corrections: 552 meaningful / 560 padded bytes, 16 relocations,
normalized SHA-256
`607cc3f2234cd8cdf3e3e0c0c6d3c52de8748c67c11ee3e1a32865c98e6bfa85`.

The ordinary local allocates a four-byte stack slot and uses `[ebp-1]`
instead of reusing `[ebp+0x17]`. The prologue/epilogue and instruction/relocation
offsets shift accordingly. All 16 relocation destinations remain the same;
no byte-match claim is made. This is an evidence-backed register/stack
allocation residual, not a reason to reintroduce the carrier union.

## Full candidate ownership and collateral verification

Baseline and final each emit 45 named code owners. The 35 target-owned
implemented functions are 32 unchanged exacts, the corrected desire-prop
residual, and two unchanged inherited residuals. The target's nine unwritten
owners remain absent. The **only** changed code section is desire-prop.

All ten inherited candidate-only helper owners are unchanged:
`_square_root` 16, `_vector_from_points3d` 48, `_scale_vector3d` 48,
`_magnitude_squared3d` 48, `_magnitude3d` 48, `_normalize3d` 96,
`_distance_squared3d` 64, `_distance3d` 64, `_dot_product3d` 32,
and `_prop_acknowledged` 32 bytes. There is no new helper or altered linkage.
These inherited extras are disclosed, not waived into a whole-object proof.

All 60 named non-code owners retain their exact bytes, normalized hashes,
and relocation information. None is added or removed. No COMMON allocation
appears. The two unchanged residuals are `_actor_emotion_unopposable_retreat`
and `_actor_perception_refresh_test_object`; there are no collateral losses
among the other 32 strict functions.

## Admission and accounting handoff

Root should add the authentic primitive declaration to the existing owner,
remove Props' conflicting local declaration, apply this bounded owner
correction, then remeasure the frozen Props iterator candidate in the final
shared-header context. The full dependent sweep is still required; this
scratch gate cannot establish that unrelated header consumers stay exact.

If both scratch outcomes survive integration, Props recovers 544 meaningful
exact bytes while this deliberate source-policy correction revokes 549
meaningful exact bytes: **net -5 meaningful / -16 padded strict bytes and
no net strict function-count gain** for the combined API/caller packet.
That accounting is honest; source integrity takes precedence over retaining
the unsupported historical exact. The reconstructed owner remains useful
ordinary C and may be parked without exact credit.

Suggested park class is `register-allocation`, with the target/base sizes,
relocation counts and normalized hashes above. Obtain the actual
`fuzzy_match_percent` from the final regenerated report before writing the
manifest; no percentage has been invented or inferred from instruction
counts here. Reopen only for authentic source/compiler evidence, not a
parameter-home facade, pointer alias, declaration permutation, or optimizer
control. No whole-object completion is recommended for either unit.
