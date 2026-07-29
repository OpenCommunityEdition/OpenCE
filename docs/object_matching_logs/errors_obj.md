# `source/cseries/errors.obj` strategy ledger

This retroactive ledger records preserved evidence for work performed before
the mandatory per-object experiment format.

## Scope and baseline

- Compiler: XDK 3911 `CL.Exe` 13.00.9254.1,
  `/O2 /Oy- /DDEBUG /Dxbox`.
- January target: `build/split/source/cseries/errors.obj`.
- Current inventory: 9/10 functions exact; `_error` is the only residual.
- Target-owned `.bss`, `.data`, and `.rdata`: 501/501 bytes exact.

## Parked residual

| Function | Size T/B | Relocs T/B | Hash T/B | Measured residual | Class |
|---|---:|---:|---|---|---|
| `_error` | `0x270/0x270` | `50/50` identities | `61bf8aad11f19fd443093f2eba0bae2db27d90af0bc156ec78a530246ae47fd7` / `09377b04504056b8f2278b92bf682d5bd8be32a924e9e87c53cddd954f11ce53` | Instruction-identical through `+0x111`; the tail uses a consistent EBX/ESI/newline-spill allocation split. | TU-context optimization |

## Preserved experiment history

Five grounded families covered declaration order, local scope, `register`
hints, and volatility. Each either emitted the unchanged baseline or
regressed relocation addresses. The historical lane did not preserve
per-variant hashes or exact source diffs.

`volatile` is prohibited under current policy even though it appeared in the
historical search. Do not repeat declaration/scope/register-hint sweeps without
new evidence; never use assembly, byte patches, undefined behavior, or flag
changes.

## Reopen and disposition

Reopen only with original local-variable provenance, an exact donor explaining
the newline spill and EBX/ESI split, or a new legal-C frame/lifetime control
that preserves the exact first `0x111` bytes and all 50 relocations.

Disposition: 9/10 code and all target-owned data exact; object remains
`NonMatching`.
