# `actions.obj` authenticated private names (2026-08-31)

## Scope

This is an identifier/linkage correction only. It changes no function body,
control flow, type, declaration lifetime, or prototype location:

- `_code_0000b140` -> private `actor_action_allowed_to_enter_vehicle`;
- `_code_0000bb40` -> private `actor_pursuit_consider_nearby_actor`.

Both declarations and definitions remain `static` in `source/ai/actions.c`.
Neither private helper belongs in `actions.h`.

## Provenance

Original Halo CE retail x86 linker maps recover the decorated names
`@actor_action_allowed_to_enter_vehicle@8` and
`@actor_pursuit_consider_nearby_actor@12`. Exact address propagation places
them at January file offsets `45376` and `47936`. Independent HCEA PDB/atlas
evidence and the lifted HCEA function roles corroborate both identities and
signatures. Marathon has no corresponding AI action helpers and is not cited
as name evidence.

The January split and candidate objects both give the helpers COFF storage
class 3 after regenerating from `config/symbols.json`.

## Frozen measurements and gates

Before the rename, both placeholder-owned functions were already strict
exact:

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `actor_action_allowed_to_enter_vehicle` | 64 | 3 | `50e1c57094a30fc679ca298b1a9ad9039b2f290477e48708446fc3d812eb5cb3` |
| `actor_pursuit_consider_nearby_actor` | 144 | 5 | `fdf1750cc00af2c32e2e3ff63c750dc9c71de9154e9e1ed5ceb55dcb754e8e66` |

The full `actions.obj` gate must remain 57 exact / 6 residual / 0 unwritten.
The renamed calls must preserve exact
`actor_pursuit_find_nearby_actors` (496 bytes, 12 relocations, SHA-256
`1313144e528893c95bc64a944d535541e0586bd180c200e4bfaf96584d314c5d`).
The inherited `actor_action_handle_vehicle_entry` residual remains 960 bytes
with 22 relocations; its two helper relocation spellings change in lockstep
only.

This narrow packet deliberately excludes the broad Fable Wave 4 rename set.
