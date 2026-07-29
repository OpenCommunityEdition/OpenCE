# `source/structures/cluster_partitions.obj` strategy ledger

This retroactive ledger preserves the evidence available from Git,
`config/parked.json`, source comments, and the current report. The original
lane predates mandatory experiment matrices; missing per-variant hashes are
called out rather than reconstructed.

## Scope and baseline

- Compiler: XDK 3911 `CL.Exe` 13.00.9254.1,
  `/O2 /Oy- /DDEBUG /Dxbox`.
- January target:
  `build/split/source/structures/cluster_partitions.obj`.
- Current inventory: 13/14 functions exact; 1,533/1,715 meaningful code bytes.
- Ordinary report: `.rdata` 500/500 bytes at 100%. Because the historical
  park entry records only the code residual, rerun strict section/ownership
  comparison before admitting the object.
- `_reference_list_copy` is the sole residual.

## Parked residual

| Function | Size T/B | Relocs T/B | Hash T/B | Measured residual | Class |
|---|---:|---:|---|---|---|
| `_reference_list_copy` | `0xC0/0xC0` | `9/9`, including addresses | `28ab903006bf625bb291ffe9b82a710eabf80400b9d1f2403b78b04429904c8b` / `dcebabeded0e38211be7325fc38d98591a621f800556829f6184e38dbacf7c03` | VC7 assigns source and destination pointer live ranges to opposite registers. | register allocation |

## Preserved experiment history

Eight legal-C source shapes were measured by the historical closeout lane.
The surviving record proves they retained equal size and relocation identity
without breaking the pointer-register mirror. The exact eight source diffs
and transient hashes were not preserved, so this ledger deliberately does not
invent them.

Do not repeat blind declaration-order or source/destination pointer-alias
spelling sweeps. Do not use `volatile`, assembly, byte patches, undefined
behavior, or compiler-flag changes.

## Reopen and disposition

Reopen only with original local-variable provenance, an independently exact
donor showing the required pointer live-range distinction, or a new legal-C
dependency control not equivalent to the historical register-allocation
sweep.

Disposition: 13/14 code is credited; historical data reporting is promising
but not a substitute for a fresh strict ownership gate. The object is **not**
strict-complete and remains `NonMatching`.
