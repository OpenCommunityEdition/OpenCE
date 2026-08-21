# `hud.obj` initialize and update recovery

Date: 2026-08-21
Starting commit: `90475f0a`
Status: partial, `NonMatching`

## Scope

This bounded pass added ordinary typed definitions for `hud_initialize` and
`hud_update` to `source/interface/hud.c`. It preserves the four exact HUD
functions recovered by the preceding lifecycle/visibility pass and claims no
target data. The first code-producing candidate compile also reviewed
`hud_initialize_for_new_map`; that body was removed fail closed after its
assertion-string owner failed strict comparison.

## Provenance

The January PC target object is SHA-256
`4fec7af778c957b49525d56739902f8b4023534e2ea3fbd1af7686c9840f0e31`.
Its disassembly and relocations establish the allocation and assertion in
`hud_initialize`, the five subsystem calls, and the update call order followed
by the four-player loop and typed player lookup.

Clean local HCEA reference commit
`c168af2e747d3095d9a29418ae401f3a39544863` independently supplies the typed
`void(void)` bodies and subsystem semantics. Relevant source blobs are
`hud_initialize.c` at `6ae5f39cd665d8ecfbbfcf9f7e1b1eafdf1e4c96` and
`hud_update.c` at `dadf069e1e26349221ce57b9d6f43bdc2413f0c7`.
The clean Stian PC reconstruction independently corroborates the same
allocation, assertion, subsystem order, single-screen predicate, four-player
loop, `datum_get`, and sound-update call.

## Strict COFF result

| Function | Meaningful | Padded | Relocations | Normalized SHA-256 |
|---|---:|---:|---:|---|
| `_hud_initialize` | 80 | 80 | 12 | `95bdfa376bd0b73e895b725cb4afa808667fb830c9d44a30fca6e9c35c9183ab` |
| `_hud_update` | 81 | 96 | 9 | `a2a08eea21087670002fef8e53793614fdcb7c1b0e6c5ae1ae2c2bf77085798a` |
| **Total** | **161** | **176** | **21** | |

`tools.coff_compare` reports `all_equal: true` for these two bodies and the
four prior exact HUD functions: section extents, normalized bytes, and every
relocation address, type, addend, and symbolic destination match. The object
advances from `4/22` to `6/22` exact functions.

The reviewed new-map initializer had target-sized code, the same normalized
machine-code hash, and all 16 relocation addresses. Its compiler-generated
assertion literal was nevertheless different: the January target owns
`interface_get_tag_index(_interface_hud_globals)!=NONE`, while the natural
candidate spelling included spaces around `!=`. Because the relocation's
symbolic destination therefore differed, the function was withdrawn without
a retry. Its final definition and symbol are absent.

## Ownership boundary

`hud_scripted_globals`, `hud_globals`, and `player_data` remain ordinary typed
external references. The retained candidate does not add a `.bss` or `.data`
definition and claims zero storage bytes. Naturally emitted assertion and
allocation strings are compared only as required relocation owners; no
partial data credit is claimed.

## Policy and validation

The retained source uses typed subsystem calls, player access, and ordinary
control flow. It contains no assembly, volatile scheduling, optimizer
barriers, force-inlining, raw address reconstruction, union aliasing,
synthetic anchors, or signed-overflow tricks. Both new functions use house
formatting and explicit terminal returns.

The complete Halo and libcmt build graphs pass. Fresh semantic/progress output
reports zero unit errors and credits exactly two functions and 161 meaningful
code bytes. Admission reports zero candidates or revocations, all three parked
compiler ties remain valid, and all 179 tool tests pass. A clean committed
forced-object regression replay is required before integration handoff.
