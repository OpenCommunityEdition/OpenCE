# `hud_messaging.obj` objective getter exact recovery (2026-09-02)

## Result

Starting from fuzzy-objective checkpoint `ad22b5502`, this packet recovers
`_hud_messaging_get_objective` byte-for-byte and advances the focused gate from
**28 exact / 1 residual / 7 unwritten** to **29 exact / 1 residual / 6
unwritten**.  The function contributes **184 meaningful / 192 padded bytes**,
with 14 exact relocation identities and normalized SHA-256
`7aa9744b58515da513e7d55d5c6647fb5339c1a2671cd5e8a84a440906721984`.
All 28 inherited exact functions and the parked setter measurement remain
stable.

## Reconstruction and provenance

January proves the early null result, scenario HUD-message tag lookup, active
objective reload after the tag call, two-byte element lookup, count/type
assertions, and wide-character byte offsets passed to
`tag_data_get_pointer`.  The independent HCEA Xbox reconstruction confirms
the objective-message and text-data relationship, while the Stian/Pastudan PC
lifts preserve the assertion topology.  Aerocatia/demon authenticates the
named message and element layouts and `_hud_message_type_text` enum constant.

The retained ordinary C uses `HUD_MESSAGE_TEXT_DEFINITION_GET`,
`TAG_BLOCK_GET_ELEMENT`, `sizeof(wchar_t)`, and the established cseries
`match_vassert` macro with January's attested source path, lines `0x2A2` and
`0x2A3`, and diagnostic strings.  The C predicates retain conventional spaces
around equality operators while the explicit diagnostics preserve January's
independently observed no-space text.  These assertions are semantic
invariants, not inert codegen controls.  The public `wchar_t *` declaration lives in the owning
`interface/hud_messaging.h`.  No raw address/offset, address-derived name,
volatile or register forcing, barrier, assembly, forced inline, fake
dependency, or nonsensical operation is present.

## Validation

- Focused object gate: `29 exact / 1 residual / 6 unwritten`.
- Direct disassembly is instruction-for-instruction identical through all 66
  decoded instructions; section size, normalized bytes, and all relocation
  identities/addresses match.
- Header-consumer gates remain stable:
  - `hs.obj`: `445 exact / 3 residual / 0 unwritten`;
  - `hud.obj`: `19 / 3 / 0`;
  - `players.obj`: `54 / 15 / 1`;
  - `game_state.obj`: `24 / 0 / 0`;
  - `game_engine.obj`: `166 / 8 / 6`.
- The existing `_scripted_hud_set_objective` park remains measurement-stable.
- `compare_messages` remains dead-stripped and unclaimed.
