# `hud_unit.obj` state accessor and shield-tick exact recovery (2026-09-02)

## Result

The authenticated private `get_hud_state` helper and public
`hud_tick_shield` caller are now strict exact as ordinary typed C. The focused
gate advances from **13 exact / 0 residual / 9 unwritten** to **15 exact / 0
residual / 7 unwritten** without losing an inherited exact function.

- `_get_hud_state`: 112 padded bytes, 10 ordered relocations, normalized
  SHA-256
  `923d09ad9b67550b4747520338451c77bcfddc864dbe9de7f90c510a108ec50d`;
- `_hud_tick_shield`: 64 padded bytes, 3 ordered relocations, normalized
  SHA-256
  `13c3cf982fa6906bd2a74a7be8ea087726438c98714ef0e2c46b5c2ac1930afe`.

For both functions the target and candidate sizes, normalized bytes,
relocation counts, relocation addresses, types, destinations, and ordering
are identical.

## Names and source provenance

The January symbol atlas at
`research/halo-symbol-atlas/symbols/halo_ce/4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520.jsonl`
names offset `0x4c6600` `_get_hud_state` and offset `0x4c7050`
`_hud_tick_shield`, both in `hud_unit.obj`. It also authenticates the formerly
address-named `0x4c6b80` symbol as `_hud_update_unit_local_player`; that symbol
configuration and the source symbol inventory are corrected in this packet,
although its body remains unwritten.

The independent HCEA donors are
`research/halocea/src/get_hud_state_0.c` and
`research/halocea/src/hud_tick_shield.c`. They establish the typed
`unit_hud_globals->hud_states[local_player_index]` access and the player-owned
local-HUD shield decrement. January disassembly independently fixes the two
assertions and line identities in the private accessor, the signed 16-bit
player field, the `NONE` guard, and the subtract/store sequence.

## Reopened boundary

The prior 2026-08-29 experiment proved the accessor exact but could not retain
it without a caller, and its then-current shield-tick source emitted a narrow
extension mismatch. No synthetic anchor or external-linkage substitution was
accepted. With the current canonical `struct player_datum` owner and an
ordinary declaration-plus-initialization of the semantic `short`
`local_player_index`, VC7 now emits January's `movsx` schedule naturally. The
public caller retains the correctly `static` private helper, closing both
functions together.

The production source uses the canonical `player_get` object-access macro,
`real`, typed structures and fields, and the owner `hud_unit.h` declaration.
It contains no raw object cast, raw offset, assembly, volatile/register
forcing, optimizer barrier, pragma, forced inline, fake dependency, synthetic
reference, representation pun, or nonsensical control flow.

## Validation

The focused gate reports 15 exact functions, zero residual functions, and
seven unwritten functions. The unchanged `hud.obj` consumer gate remains 19
exact / 3 documented residual / 0 unwritten, and `players.obj` remains 54
exact / 15 documented residual / 1 unwritten. The conservative fake-match
scan reports no review leads in the changed source. Full canonical build,
semantic, admission, parked-policy, and test validation is required at
integration.
