# `path.obj`: typed state initializer (2026-08-31)

## Result

`_path_state_new` is strict exact from an ordinary typed implementation. It
adds 56 meaningful / 64 padded bytes and two relocations. The direct campaign
gate advances `path.obj` from 9/32 to 10/32 exact functions, with zero
residual functions and 22 still unwritten.

| Function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_path_state_new` | 56 / 64 | 2 | `fa67a764474c1dbf4d3beaebe0c93dcedd35da168619e913d921be235a538c26` |

The relocations are `REL32` calls to `_csmemset` at section offset `0x12`
and `_global_structure_bsp_get` at `0x1A`. The complete padded section,
normalized bytes, relocation types, offsets, destinations, and addends agree.

## Provenance and source boundary

The January PDB symbol atlas fixes the public name and address. The existing
owner declaration in `source/ai/path.h` supplies the typed three-argument
signature. HCEA commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c`
independently supplies the same initializer semantics and signature: clear a
`path_state`, attach the current structure BSP, copy the complete
`path_input`, and store the optional `path_debug_storage` pointer. The
January PC object is authoritative for its platform-specific BSP accessor;
the relocation names `_global_structure_bsp_get` directly.

The earlier input-leaf wave did not emit this function because its then-current
shared `path_state` definition measured `0x1008C`. The corrected owner header
now proves the January size `0x1408C`; source-local assertions additionally
prove `debug` at `+0x48` and `structure` at `+0x64`. The retained body uses a
typed `sizeof(*state)` clear and ordinary structure assignment. It has no raw
offset access, cast, aliasing pun, assembly, `volatile`, compiler directive,
synthetic carrier, or optimizer-only spelling.

## Frozen sibling boundary

Natural typed candidates for `_path_input_set_attractor` and
`_path_input_set_search_bounds` also happened to match 64 and 32 padded bytes,
respectively, during this audit. They remain rejected and absent. The prior
input-leaf ledger explicitly freezes the exact spellings that would be needed:
an aggregate point copy for the first and flag-before-distance assignment for
the second. The preserved HCEA source instead uses scalar point-component
stores and distance-before-flag order. No new independently preserved source
artifact authenticates the January spellings, so byte equality alone does not
override that source-provenance boundary.

## Validation

- Scratch-safe direct selected gate: 1 exact / 0 residual / 0 unwritten.
- Scratch-safe whole-unit gate: 10 exact / 0 residual / 22 unwritten.
- `tools.coff_compare`: `all_equal: true` for `_path_state_new`.
- `tools/fake_match_scan.py --fail-on-findings source/ai/path.c`: zero review
  leads.
- `git diff --check -- source/ai/path.c`: clean.

No shared header, configuration, object-status label, parked record, build
tree, staging area, or commit is changed by this bounded packet. Full-tree
rebuild and regression adjudication remain with the integrating orchestrator.
