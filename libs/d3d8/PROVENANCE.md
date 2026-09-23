# RXDK source-reference provenance — D3D8 20k checkpoint

The source reconstruction consulted and adapted Team Resurgent's RXDK-Libs
(2026), under GPL-3.0-or-later, pinned at:

    29e018674a3443f9df5b5cb4478bb9c54d8a2a3a

Inspected paths in that repository, all under `libs/libd3d8/se/`:
`pixeljar.cpp`, `pixeljar.hpp`, `state.cpp`, `resource.cpp`, `vshader.cpp`,
`pshader.cpp`, `enum.cpp`, and `d3dbase.cpp`.

Repository source URL:
https://github.com/Team-Resurgent/RXDK-Libs/tree/29e018674a3443f9df5b5cb4478bb9c54d8a2a3a/libs/libd3d8/se

The license text is retained as `libs/d3d8/LICENSE.GPL-3.0`. Adapted production files retain
source attribution. This packet is not asserted to be an independent clean-room
reconstruction, and the older Halo repository's CC0 notice must not be treated
as relicensing these adaptations. Third-party reference text was read through the authorized GitHub
connector, not from a later local binary substituted for the January target.

The user-supplied January Halo EXE/PDB, pinned split configuration, instructions,
data references, and ABI are the authority for target behavior. The RXDK revision
is later and differs in layouts and behavior: APIs, sizes, flags and field owners
were reconciled against the target. Agreement with reference source alone never
awards code credit. Build flags remain a hypothesis until original commands are
recovered; exactness here is measured output, not a claimed recovered build recipe.

Only the authored/adapted source and reviewable tooling patches are included
in this repository. User-supplied EXE, XBE, PDB, compiler and SDK archives are
not included.


## Continuation 005 additions

The same pinned RXDK revision was additionally consulted/adapted for
`mphal.cpp` (graphics-context bank layout, 3D initialization and PLL sequencing)
and `mpcore.cpp` (shutdown drain order and tile creation/destruction).
Relevant Git blob IDs: mphal `9b5ffa9697cf38d8cd0e77dc2be8332aaa6b230d`,
mpcore `fbc477fe8f299ad60fa1f7ec2094be14a2b2ec49`.
The generated context-bank names/counts are additionally documented by the
January target write sequence in `libs/d3d8/research/wave5/grctx3d_schema.json`.

Kernel declaration corroboration uses XboxDev/nxdk `lib/xboxkrnl/xboxkrnl.h`
at `58427c078b4ccb0121f359b6b6c536b3b2914976`; it is not a replacement SDK.
Only the necessary ABI declarations were used. See
`libs/d3d8/research/wave5/kernel_import_contracts.json` for declarations and limitations.
