# Actor Perception emotion/situation natural-source packet

This ledger records a bounded post-reconciliation packet for
`source/ai/actor_perception`. It does not grant exact credit to either added
function and does not change matching configuration.

## Scope and provenance

- Canonical base: `5d3444f79904738b984398143df262cc45157239`.
- Natural-source donor: `99a8c5e812153bf8b590ecb9be43991bf9b8dcca`
  in `work/actor-perception-full-admission-v2`.
- Edited implementation owner: `source/ai/actor_perception.c`; the final
  source-authenticity pass keeps its PDB-authenticated defensive-crouch
  enumerators TU-local because this is their only reconstructed consumer.
- The body names, project types, typed accessors, shared `actor_datum`,
  `prop_datum`, `unit_datum`, and `actor_situation` fields, and explicit
  returns are retained. The reconstructed bodies contain no padded
  actor/prop/unit/count facade, address-derived identifier, raw byte-offset
  access, fake carrier, inline assembly, volatile object, optimizer control,
  barrier, or forced inlining.
- No shared configuration, header, or sibling source file is changed by the
  final packet.

The final naming pass used direct `HCEX_Release.pdb` evidence. It added the
PDB-authenticated defensive-crouch enumerators beside the consuming code and
included the existing actor-type and AI-communication owners. A trial in the
shared actor-definition header was rejected by the orchestrator's global
build because VC7 definition-position sensitivity invalidated the documented
`encounter_update_respawn` fuzzy evidence in an unrelated translation unit.

## Strict gate delta

The pre-edit object was captured in PID-local scratch before either body was
added. Comparing that object against the split January target with the same
hardened `coff_compare` rules gave:

| State | Exact | Residual | Unwritten | Total |
|---|---:|---:|---:|---:|
| Before | 32 | 6 | 6 | 44 |
| After | 32 | 8 | 4 | 44 |

Both previously unwritten functions now have coherent natural C bodies, but
neither is byte-and-relocation exact. The strict exact delta is therefore
**zero**, all 32 exact siblings remain exact, and this packet claims **zero
matching credit**.

The final focused gate was:

```text
python tools\campaign\gate.py source/ai/actor_perception \
  --fn _actor_emotion_update --fn _actor_situation_update \
  --out scratch\actor-perception-authentic-types-final.obj \
  --forbid-emitted-symbol _point_from_line3d

== emitted-symbol guard passed (1 forbidden names checked)
residual   1664  _actor_emotion_update  [sha]
residual   1264  _actor_situation_update  [size 1232!=1264, sha]
== exact 0  residual 2  unwritten 0  (of 2 listed)
```

The final whole-unit gate reports `32 exact / 8 residual / 4 unwritten` out
of 44.

## `actor_emotion_update`

The retained body is the strongest known natural-source frontier:

| Measurement | January target | Candidate |
|---|---:|---:|
| Padded bytes | 1664 (`0x680`) | 1664 (`0x680`) |
| Relocations | 38 | 38 |
| Normalized SHA-256 | `91da10547819f3cfcc203b3ca2294b71ced737739c2421c9e8f374ed03e64a94` | `830a20bbdf3b73c4bd94b057087d3ba914d7cd42aac52ee2a904c41085b5598a` |

Relocation address, type, and resolved-target identity are pairwise exact.
There are seven normalized byte differences, at `+0xF6`, `+0x5CA`,
`+0x5CE`, `+0x5F1`, `+0x5F2`, `+0x5F5`, and `+0x5F6`; 1657 of 1664
normalized bytes agree. The first difference is the deliberate correction of
the threat scan to the last valid `specific_threats` element instead of the
donor facade's one-byte cross-field read. The other six are branch
displacements, opcodes, and byte-register operands for the second
defensive-crouch guard. This remains a scheduling/source-authenticity
residual, not an exact match.

One additional provenance-driven natural spelling was measured: testing the
current `defensive_crouch` field before the desired `crouch` value in the
second nested transition. VC7 changed allocation across the complete function
and produced 1696 bytes with 40 relocations. It was reverted. The donor ledger
already records a 42-shape ordinary-C guard sweep, so no unsupported spelling
or codegen device was retained.

The source-authenticity pass removes a misleading shared local that used the
integer `3` simultaneously as actor mode, combat status, and threat level.
The final source names those independent domains as `_actor_mode_combat`,
`_actor_combat_status_definite`, and
`_actor_threat_visible_aiming_at_me`. It uses the named actor-threat ladder,
actor-definition flag bits, and PDB-authenticated `_defensive_crouch_*`
enumerators in the switch. It reads the highest priority only from the
PDB-authenticated `specific_threats[9]` field and reads the hide-behind-shield
condition from the separately named `cumulative_threats[9]` field. The final
`ai_communication_event` argument uses the owned pointer prototype and
`NULL`, not an integer boolean. The compiled frontier retains equal size and
relocation count while recording the additional bounds-authentic immediate
difference described above.

Direct `DIA2Dump -sym actor_emotion_update` evidence supplies the local names
`attack_vector`, `friend_attack_vector`, `vector_to_line_of_fire`,
`movement_direction`, and `future_point`; the final source uses those names.
The PDB also reports `hasInlineAssembly: false` and `WasInlined: false`.

### `point_from_line3d` ownership

The TU defines `REAL_MATH_EXTERNAL_POINT_FROM_LINE3D` immediately before its
existing include block and undefines it immediately afterward. This keeps the
math call external without changing `real_math.h`.

Both target and candidate contain exactly one `_point_from_line3d` symbol-table
entry, and it is undefined external (`section 0`, value zero, external
storage). Both contain exactly one reference:

```text
_actor_emotion_update +0x3B5, relocation type 20,
target = [symbol, _point_from_line3d, addend 0]
```

No candidate code definition or COMDAT with that name is emitted. The complete
candidate passes `--forbid-emitted-symbol _point_from_line3d`.

## `actor_situation_update`

The natural donor body restores the acknowledged-prop walk, enemy threat
census, friend area/visibility/proximity census, best-target selection,
target-weight refresh, and the two final situation-status updates. The final
source expresses those operations through the repository's shared datums
rather than donor-era padded views.

The initial donor view called actor-state offset `0x6C` `combat_status` and
then compared it with raw value 10. The authenticated `actor_datum` layout
proves that offset is `state.action`; the final source names it `action` and
compares it with `_actor_action_charge`. The `prop_datum` PDB likewise
authenticates the used `unit_index`, `state`, `visibility`, `line_of_sight`,
`target_weight`, `enemy`, `currently_damaging_me`, `unreachable_ticks`,
`distance`, `quantized_facing`, `dead`, `fighting`, `shooting`, and
`dangerous_vehicle_driver` fields. Shared `unit_datum` supplies
`unit.actor_index` and `unit.player_index`. Shared `actor_situation` supplies
the exact PDB member names `known_enemies`, `visible_reachable_enemies`,
`specific_threats`, `cumulative_threats`, and every area/visible/close friend
counter and per-type array. The padded actor, prop, unit, count, and emotion
priority facades and their offset assertions were removed once no longer
used. Actor types, perception levels, line-of-sight results, actions, and
threat levels use named constants rather than unexplained integers. These
corrections do not claim or create an exact match.

| Measurement | January target | Candidate |
|---|---:|---:|
| Padded bytes | 1264 (`0x4F0`) | 1232 (`0x4D0`) |
| Relocations | 28 | 28 |
| Normalized SHA-256 | `7c1d092d9e1619958622643b7803eb4f59c81a2e9bc94b7964f56f24e92757f6` | `3986f549578398a0a736ab6229cce62342abd2748bab5fc4ad4aa0f1043a8385` |

The normalized comparison has 987 differing bytes beginning at `+0x0C`; 277
of the 1232 overlapping bytes agree. Relocation counts match but pairwise
identity does not because the differing control-flow layout moves 23 of the
28 sites. This remains the known topology/layout residual and receives no
exact credit.

## Disposition

Keep both coherent fuzzy implementations in canonical source under the house
rule that useful natural reconstructions should not remain stranded in donor
trees. Neither function may be marked Matching from this evidence. Reopen
`actor_emotion_update` only with January-local source/provenance or a genuine
defined-C scheduling control; reopen `actor_situation_update` only with source
or layout evidence that predicts its missing topology. Do not use register
forcing, raw-offset facades, volatile/barriers, fake helpers, or an emitted
`point_from_line3d` body.

The final source passes the whole-unit forbidden-symbol gate with all 32
previously exact siblings preserved and passes
`tools/fake_match_scan.py --fail-on-findings` with zero findings. This is a
source-authentic zero-credit park.
