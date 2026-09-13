# `players.obj` Fable exact-packet reconciliation — 2026-09-13

## Result

The Fable donor at `b2fa98b2a0a056bf259369f9beb25d6880656e78`
contains twelve sections absent from the current accepted-exact ledger.  Its
prebuilt report identifies **3,553 meaningful / 3,632 padded exact code
bytes**.  None are admitted: the packet's exact output depends on code-generation
controls, representation punning, duplicated helper bodies, raw offsets,
numeric enum/flag values, or an ABI-fiction stub.  The current coherent source
therefore remains in place and receives **zero new exact functions and zero
new exact bytes**.

The focused current gate remains **54 exact, 15 residual, and one unwritten of
70 target functions**.  The emitted-symbol guard confirms that the candidate
does not define `_point_from_line3d`.

| Donor section | Meaningful / padded bytes | Reconciliation decision |
|---|---:|---|
| `_player_examine_nearby_device` | 162 / 176 | Reject the volatile reinterpretation of the sphere radius and the donor's non-static definition.  Keep the direct `real` field access and January-atlas private linkage. |
| `_player_examine_nearby_item` | 793 / 800 | Reject the volatile reinterpretation of `weapon_count` and numeric powerup/action values.  Keep the typed enum constants, access helpers, and ordinary scalar lifetime. |
| `_player_examine_nearby_objects` | 233 / 240 | The donor and current function bodies are text-identical.  January and the donor allocate the object-index pointer in `EDI`; the current TU uses `ESI`.  The donor closure is a downstream C2-state effect of unsafe surrounding functions, not an independently transplantable source correction. |
| `_player_examine_nearby_unit` | 1 / 16 | Reject `void player_examine_nearby_unit(void) { return; }`.  Stian and Pastudan callers establish a two-handle boolean ABI, while January's one-byte `ret` leaves the boolean return undefined.  Keep the authenticated declaration and no fabricated body. |
| `_player_examine_nearby_vehicle` | 435 / 448 | Reject raw `byte *` offsets, a duplicate `vehicle_get`, and numeric seat/action/flag values.  Keep the semantically named vehicle projection, enum constants, and ordinary access path. |
| `_player_handle_powerup` | 240 / 240 | Reject manual copies of `player_powerup_on` and `player_powerup_additional`, the unsupported `long` powerup carrier, and numeric enum values.  Keep calls to the real private helpers. |
| `_player_handle_powerup_minor` | 159 / 160 | Reject the copied `player_powerup_on` implementation and unsupported widened carrier.  Keep the named helper call and typed powerup index. |
| `_player_set_action_result` | 238 / 240 | Reject forced `__declspec(noinline)` and two hand-written inline distance helpers whose expression order exists only to steer x87 code generation.  Keep the named `distance3d` calls and action-result enum. |
| `_player_teleport_on_bsp_switch` | 320 / 320 | Reject the volatile boolean plus the nonsensical `TRUE` then `FALSE` double store.  Keep the direct negated trigger result. |
| `_player_update_powerups` | 109 / 112 | Reject `_ReadWriteBarrier` and the copied `player_powerup_off` implementation.  Keep the ordinary private-helper call. |
| `_players_reconnect_to_structure_bsp` | 729 / 736 | Reject volatile reinterpretations of `leaf->cluster_index` and `adjustment_or_width`.  Keep the typed structure and `real` field reads. |
| `_unit_should_autopick_weapon` | 134 / 144 | Reject the volatile weapon-definition pointer used only to alter register allocation.  Keep the typed definition pointer and named flag macro. |

## Evidence boundary

This audit read the complete current/donor Players ledgers, the donor source,
its prebuilt object/report, the relevant source/config history, and the HCEA,
Stian, and Pastudan semantic lifts.  Those independent sources support the
gameplay topology of the current candidates, but they do not authenticate the
donor's volatile aliases, barriers, forced inlining decisions, raw offsets,
or false prototype.  In particular, the current and donor
`player_examine_nearby_objects` bodies compare line-for-line equal; its donor
hash cannot be claimed without also accepting unrelated unsafe TU state.

No production source, header, configuration, park entry, compiler flag, or
16-player-sensitive player index/bit width was changed.  Reopen an owner only
for first-party January-equivalent source, line/scope records, or an ordinary
legal-C form supported independently of its matching output.

## Focused validation

```text
python -B tools/campaign/gate.py source/game/players --all \
  --forbid-emitted-symbol _point_from_line3d
```

Result: emitted-symbol guard passed; `54 exact / 15 residual / 1 unwritten`.
The existing 54 exact owners remain exact.
