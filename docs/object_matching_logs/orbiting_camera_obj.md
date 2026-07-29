# `source/camera/orbiting_camera.obj` strategy ledger

This is a retroactive evidence ledger for a lane that predates the mandatory
per-object experiment matrix.

## Scope and baseline

- Compiler: XDK 3911 `CL.Exe` 13.00.9254.1,
  `/O2 /Oy- /DDEBUG /Dxbox`.
- January target: `build/split/source/camera/orbiting_camera.obj`.
- Current inventory: `_orbiting_camera_new` exact;
  `_orbiting_camera_update` residual (1/2 functions).
- Ordinary report: `.rdata` 60/60 bytes at 100%. Revalidate its strict
  section ownership before object admission.

## Parked residual

| Function | Size T/B | Relocs T/B | Hash T/B | Measured residual | Class |
|---|---:|---:|---|---|---|
| `_orbiting_camera_update` | `0x470/0x470` | `50/50` identities | `908d34ea2543c87642a508552b21907f5aa137815b6abd711edf4816ff17188c` / `b413e48816f598e702d4f838d2921f98928534b4b6b8a94a25eefe9008ce0a11` | One nine-byte scheduling window interleaves the final zero-vector store with depth, field-of-view, and timer loads; instruction addresses realign afterward. | instruction scheduling |

## Preserved experiment history

Five grounded assignment-order shapes were built. The untouched source
remained best at ordinary 99.51899%; none changed the residual without
regressing surrounding scheduling. The original lane did not retain the five
individual source diffs/hashes, so they cannot be listed more precisely.

Do not repeat generic assignment/declaration ordering. Barriers, `volatile`
anchors, assembly, undefined behavior, byte forcing, and flag changes are
prohibited.

## Reopen and disposition

Reopen only with original statement/local provenance, an exact donor with the
same store/load dependency graph, or a newly demonstrated legal-C scheduler
control that preserves 50/50 relocation identity and the exact sibling.

Disposition: residual parked; object remains `NonMatching`.
