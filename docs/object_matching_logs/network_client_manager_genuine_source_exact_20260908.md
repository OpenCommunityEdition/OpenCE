# `network_client_manager.obj` genuine-source exact closure (2026-09-08)

## Result

`source/networking/network_client_manager.c` now reproduces all 52 January code
functions exactly. The strict baseline contained 47 exact functions and five residuals;
the final PID-scratch gate reports 52 exact, zero residual, and zero unwritten functions.
No inherited exact function regressed.

| Function closed | Meaningful code bytes | Padded bytes |
| --- | ---: | ---: |
| `_network_game_client_add_advertised_game` | 706 | 720 |
| `_network_game_client_idle_ingame` | 392 | 400 |
| `_network_game_client_initiate_join_game` | 246 | 256 |
| `_network_game_client_leave_game` | 605 | 608 |
| `_network_game_client_remove_player` | 357 | 368 |
| **Confirmed gain** | **2,306** | **2,352** |

Only strict byte-and-relocation equality is counted above. No fuzzy result receives credit.

## Reconstruction evidence

The primary donor is the user-supplied genuine Bungie source at
`C:/Users/isabe/Downloads/haloleak2024/halo1/network_client_manager.c`, SHA-256
`B2815FAF5E0F34A28A10360314BC8EB1C15B17F5F2ADA2BAEB9FFFC4A5344907`.
January COFF disassembly and relocation identities were used to distinguish build-specific
names and layouts from source-shape evidence.

The closure restores these genuine source characteristics:

- the persistent `success` and player-index flow in player removal, rather than early-return
  restructuring;
- advertised-game garbage collection, slot selection, success accumulation, field assignment
  order, and the original conditional expressions used by its diagnostic event;
- the negative connection guard, split-screen `else if`, scoped stale-state temporary, and
  post-idle failure flow in the in-game idle routine;
- assertion expressions without redundant casts after restoring the authentic boolean
  `network_connection_connected` contract in its owner;
- named UI error enum constants in the reconstructed in-game idle path;
- `word` advertised-game machine/player counts, corroborated by January's unsigned default
  promotions (`movzx`) at the genuine variadic diagnostic call.

The result does not use address-named identifiers, fabricated branches, undefined behavior,
inline assembly, artificial compiler steering, or emitted helper COMDATs.

## Verification

The final isolated command was:

```text
python -B tools/campaign/gate.py source/networking/network_client_manager --all --forbid-emitted-symbol _point_from_line3d
```

It reports `exact 52 residual 0 unwritten 0`; the forbidden-symbol guard passes.

```text
python -B tools/fake_match_scan.py --fail-on-findings source/networking/network_client_manager.c
```

The fake-match scan reports zero review leads, and `git diff --check` passes. Canonical
configure, full build, and project-wide regression sweep remain the orchestrator's admission
steps; this isolated packet did not run or modify the shared build.
