# `source/physics/point_physics.obj` strategy ledger

This retroactive ledger records preserved strict measurements and explicitly
marks the detail lost before per-object experiment matrices were required.

## Scope and baseline

- Compiler: XDK 3911 `CL.Exe` 13.00.9254.1,
  `/O2 /Oy- /DDEBUG /Dxbox`.
- January target: `build/split/source/physics/point_physics.obj`.
- Current code inventory: 5/6 functions exact; `_point_physics_update` is the
  only residual.
- Ordinary non-code report: `.data` 12/12 and `.rdata` 200/200 exact; an
  8-byte `.bss` needs a fresh strict ownership check before object admission.

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

Disposition: residual parked; object remains `NonMatching`.
