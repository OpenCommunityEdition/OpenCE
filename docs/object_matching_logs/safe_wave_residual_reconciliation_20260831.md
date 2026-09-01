# Safe-wave residual reconciliation (2026-08-31)

## Scope

This audit reconciles packet `03680f0a301ce1d461a2b4dcbdfef0d6a359f788` against canonical base `83e72959bbd55224fc49704acc6036015aa86d83`, the frozen object ledgers, and the current house rules. The packet's immediate donor is local commit `1ed2fd9efa4a322edd654d5fe9caf47ed2fe8e59` (Claude Fable 5 reconstruction wave). A donor is not treated as original January source merely because it improves fuzzy similarity.

## Decisions

| Function | Decision | Reason |
| --- | --- | --- |
| `tiff_export`, `tiff_import` | Reverted | The fixed-point TIFF closeout already exhausted these source-level spellings and requires new source/local-lifetime evidence to reopen. The wave supplied disassembly reconstruction, not authenticated TIFF source or a new live-range donor. Fuzzy gains therefore do not supersede the frozen ruling. |
| `dead_camera_new` | Reverted | The if/else-to-ternary change supplied no closer cache-beta provenance or target-specific lifetime evidence. The frozen topology/freshness rulings remain controlling. |
| `lruv_block_new` | Retained and parked | The local donor's natural typed-C body is independently corroborated at the semantic/control-flow level by Aerocatia/demon commit `edb96c7614b80b4a80a3b0cee5c5fb7c7804a3c7`. Canonical names and types remain authoritative. The body removes the frozen candidate's banned `_ReadWriteBarrier()`, uses one initialized block index, a typed best-hole scan, the named ring macro, and a common return. It reaches the target's 1,312-byte/48-relocation envelope without compiler steering. |
| `weapon_can_be_fired` | Retained and parked | The local typed-C reconstruction agrees with the readable Pastudan disassembly lift in `src/halo/items/weapons.c`: age below one blocks firing; a finite game-engine magazine blocks only when both loaded and reserve rounds are empty. The owner prototype remains in `source/items/weapons.h`. The exact 144-byte/five-relocation envelope is useful fuzzy progress, but the 25% similarity earns no exact credit. |

Demon and Pastudan are semantic/disassembly corroboration, not claims of original January source provenance.

## Measurements after reconciliation

| Function | Target | Base | Objdiff |
| --- | ---: | ---: | ---: |
| `lruv_block_new` | 1,312 bytes / 48 relocs | 1,312 bytes / 48 relocs | 94.18722% |
| `weapon_can_be_fired` | 144 bytes / 5 relocs | 144 bytes / 5 relocs | 25.0% |

The LRUV residual is parked as register allocation because alignment first diverges in the callee-saved register cycle. The weapon residual remains honestly unclassified.

## Validation

- Full Ninja build and report: pass.
- Per-TU campaign gates: TIFF `1E/2R/0U`; dead camera `2E/2R/0U`; LRUV `16E/2R/0U`; weapons `43E/3R/33U`.
- `units.obj`: `189E/0R/0U`.
- Park manifest validator: pass with current COFF snapshots.
- Added-line fake-match scan and `git diff --check`: pass.
- Full verdict comparison against the frozen canonical snapshot: zero exact regressions.
