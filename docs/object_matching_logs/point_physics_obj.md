# `source/physics/point_physics.obj` strategy ledger

This retroactive ledger records preserved strict measurements and explicitly
marks the detail lost before per-object experiment matrices were required.

## Scope and baseline

- Compiler: XDK 3911 `CL.Exe` 13.00.9254.1,
  `/O2 /Oy- /DDEBUG /Dxbox`.
- January target: `build/split/source/physics/point_physics.obj`.
- Historical baseline: 5/6 functions exact; `_point_physics_update` was the
  only residual.
- Current verified inventory: 6/6 functions exact, 1,822/1,822 meaningful
  code bytes, and 220/220 data bytes. `.text`, `.rdata`, `.data`, and `.bss`
  all report 100% after a clean XDK 3911 rebuild.

## Parked residual

| Function | Size T/B | Relocs T/B | Hash T/B | Measured residual | Class |
|---|---:|---:|---|---|---|
| `_point_physics_update` | `0x570/0x570` | `68/68` identities | `5aca2728fa2d89a716057b07ccbcc1e00cbf126644391dcf1a534707f5cba88c` / `93750650472a8e7cfc7dff5fd05b16a5d0b61d4ba611b324a6d1612ed82dbeb4` | First divergence is target `MOV ECX,EAX` versus candidate three-byte `MOVZX` from CL, shifting later relocations by one; additional commutative x87 operand-order differences remain. | TU-context optimization |

## Preserved experiment history

Five grounded low-byte/current-flags source shapes were built. The untouched
source remained best at ordinary 99.39632%. Attempts to force the byte form
introduced extra loads/spills and regressed the function. Individual
transient hashes and source diffs were not retained by the historical lane.

Do not repeat generic casts, low-byte snapshots, current-flags aliases, or
declaration-order variants without new evidence. Do not use `volatile`,
assembly, byte patches, undefined behavior, or flag changes.

## Reopen and disposition

Reopen only with a legal-C use/lifetime shape proven to select the target
two-byte move while retaining the 68-relocation graph, or with original
local/source provenance. Recheck the 8-byte `.bss` under strict ownership
rules before any object admission.

## Upstream resolution (2026-08-07)

The completed object in `punpckhdq/halo` commit `76a2766930a98f946d669c6db3565c95301d11c4`
was rebuilt in a pristine detached worktree before any integration. All six
functions passed the hardened comparator, including relocation identity.
The upstream `.rdata` (200 bytes), `.data` (12 bytes), and `.bss` (8 bytes)
also matched the January object.

The commit was then applied to the current campaign in an isolated integration
branch. Conflicts were resolved without discarding unrelated current work:

- the January-proven 0x40-byte `point_physics_definition` layout replaced the
  incomplete 0x34-byte HCEX-derived layout;
- the anonymous 8-byte `point_physics_globals` block was replaced by the two
  target-owned four-byte globals;
- the collision-user enum was restored so the source names user 13 instead of
  retaining a magic number;
- a typed `point_physics_definition_get()` macro and size assertion were added
  to retain the project house rules.

The consolidated Halo build completed successfully. The final strict function
measurements are 6/6 exact, with `_point_physics_update` at 0x570 bytes and
68/68 relocations. The old `MOVZX`/x87 residual was therefore a type/source-
provenance problem, not a permanent compiler tie.

Disposition: strict object complete; eligible for canonical admission after
the whole-tree regression manifest check.
