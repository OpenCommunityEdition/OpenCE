# `game.obj` HCEA initialization pair and fuzzy completion (2026-09-08)

## Result

At canonical commit `4ebe0ff6ec889aa7ae5657086afb51b175694aff`, the
focused gate moved `game.obj` from `22 exact / 2 residual / 3 unwritten` to
`24 exact / 3 residual / 0 unwritten`. Two authentic subsystem-order
reconstructions close exactly:

| Function | Meaningful bytes | Padded bytes | Result |
| --- | ---: | ---: | --- |
| `_game_initialize` | `257` | `272` | strict exact |
| `_game_initialize_for_new_map` | `345` | `352` | strict exact |
| **Total new exact credit** | **`602`** | **`624`** | **2 functions** |

All 22 inherited exact functions remain strict exact. The final candidate
contains no unwritten configured `game.obj` function, but the three retained
residuals receive no exact-match credit.

## Evidence and source shape

The initialization bodies follow the complete call order and control flow in
the HCEA reconstruction cache:

- `research-cache/halocea-full-blobs-20260830/src/blam/game/game_initialize.c`
- `research-cache/halocea-full-blobs-20260830/src/blam/game/game_initialize_for_new_map.c`
- `research-cache/halocea-full-blobs-20260830/src/blam/game/game_safe_to_save.c`
- `research-cache/halocea-full-blobs-20260830/src/blam/game/game_load.c`
- `research-cache/halocea-full-blobs-20260830/src/blam/game/game_set_game_variant_from_name.c`

January COFF symbols, relocation order, and disassembly fix the exact
initialization order, both map-state assertions, random-seed restoration,
conditional AI placement, and the terminal UI safe-to-load call. The source
uses project `boolean`/`real` types, public subsystem names, ordinary calls,
and explicit returns. It adds no inline assembly, compiler steering,
address-named identity, raw layout access, or undefined behavior.

The missing declarations were placed in their actual owner headers:

- decal lifecycle declarations in `source/effects/decals.h`;
- `game_statistics_start` in the associated `source/game/game.h`;
- `any_player_is_in_the_air` in `source/game/players.h`;
- `vehicle_moving_near_any_player` in `source/units/vehicles.h`;
- the `debug_game_save` definition contract in `source/main/main.h`.

`game.c` now includes `units/vehicles.h` for the vehicle-owned predicate.
The conflicting private `extern byte debug_game_save[]` shadow in
`hs_globals_external.c` was removed, and the boolean HS global table now uses
`&debug_game_save`. This is owner-contract cleanup and receives no matching
credit.

## Credible fuzzy functions retained without credit

`_game_safe_to_save` retains the previously authenticated HCEA
guarded-result implementation. It covers every predicate and warning in the
January order and matches all 32 relocation identities, but differs in branch
tail placement and Boolean register lifetime:

| Evidence | Meaningful / padded | Instructions | Relocations |
| --- | ---: | ---: | ---: |
| January target | `315 / 320` | `111` | `32` |
| Retained HCEA candidate | `313 / 320` | `112` | `32` |

The detailed immutable first-shot analysis and do-not-repeat boundary remain
in `docs/object_matching_logs/game_obj_jonas_safe_to_save_first_shot_20260829.md`.
No allocator/CFG byte tuning was attempted.

`_game_load` remains a complete semantic candidate at `198 / 208` target
bytes. Its target and candidate have the same padded envelope and relocation
identities; the remaining difference is the final independent global-load
schedule/register choice. `_game_set_game_variant_from_name` remains a
complete semantic candidate against a `92 / 96` target; the compiler folds
the second local variant copy that January retains. A natural pointer-lifetime
expression did not change emitted code and was reverted. Both are parked for
new source/compiler evidence, with no exact credit.

## Validation

- Final focused gate: `24 exact / 3 residual / 0 unwritten`.
- `_point_from_line3d` emitted-symbol guard: pass for `game.obj`.
- Direct-consumer gate: all 93 consumers of the five changed owner headers
  compiled and preserved every inherited exact function; zero regressions.
- Four consumers (`action_charge`, `ai_debug`, `units`, and `vehicles`) have a
  pre-existing `_point_from_line3d` COMDAT. They were recompiled without that
  guard solely to compare their complete exact sets, which remained
  unchanged (`20`, `56`, `189`, and `34`, respectively). The exception is not
  attributed to this packet and receives no credit.
- `hs_globals_external.c` compiles with the corrected owner contract; it has
  no configured function ownership in the focused gate (`0 -> 0`).
- Fake-match scan over all changed production files: 0 review leads.
- `git diff --check`: clean.

The saved final candidate object is
`scratch/game-hcea-initialize-20260908.obj` with SHA-256
`dc4eb5c58850fbc50a35256e3884bb426986bd4e283e97e6498874a57ec955d6`.
No shared build, configuration, commit, or push was performed.
