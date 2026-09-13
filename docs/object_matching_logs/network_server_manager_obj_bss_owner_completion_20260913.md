# `network_server_manager.obj` BSS-owner completion (2026-09-13)

## Result

`network_server_manager.obj` is admitted as complete.  Its 70 January
functions are exact under the hardened COFF comparator, including 12,464
meaningful code bytes.  Its 7,688 bytes of read-only data are ordinarily exact,
and the last eight reported BSS bytes are now covered by a fail-closed semantic
owner record.  No function or data credit is inferred from a percentage.

## Authenticated private static

The January split BSS contains two owners in one eight-byte section:

| owner | offset | storage | source meaning |
| --- | ---: | ---: | --- |
| `_network_game_server_memory_do_not_use_directly_in_use` | 0 | external | allocation guard byte |
| `_network_game_server_next_team_index` | 4 | static | alternating team index |

The rebuilt object has the same size, flags, logical all-zero payload and owner
positions.  VC7 decorates the function-local static as
`?network_game_server_next_team_index@?1??network_game_server_add_player_to_game@@9@9`;
csplit instead records the recovered source name.  Both owners have value 4,
type 0 and static storage class 3.  The complete BSS normalized SHA-256 is
`af5570f5a1810b7af78caf4bc70a660f0df51e42baf91d4de5b2328de0e83dfc`.

This is not a guessed alias.  The user-supplied Bungie
`C:/Users/isabe/Downloads/haloleak2024/halo1/network_server_manager.c` places
the alternating team value in a function-local static (`flipflop`) inside
`network_game_server_add_player_to_game`.  January's symbol map supplies the
build-specific semantic name.  Keeping the static in that genuine scope also
preserves all 70 exact function sections; moving it to file scope reverses the
VC7 BSS order and is therefore rejected.

## Candidate-only duplicate sections

The rebuilt translation unit emits three unreferenced XDK lookup-table copies:

| owner | bytes | selection | selected January provider |
| --- | ---: | ---: | --- |
| `_D3DTEXTUREDIRECTENCODE` | 16 | SELECT_ANY | `source/cseries/headers.obj` |
| `_D3DSIMPLERENDERSTATEENCODE` | 328 | SELECT_ANY | `source/interface/progress_bar.obj` |
| `_D3DPRIMITIVETOVERTEXCOUNT` | 88 | SELECT_ANY | `source/rasterizer/xbox/rasterizer_xbox_decals.obj` |

Each copy has zero incoming relocations in this translation unit and exactly
the selected provider's size, zero-relocation shape and normalized SHA-256.
The ordinary-link discard behavior for these same stock copies is already
verified by the Event Manager completion audit.  Compiler directive/debug
sections and locally materialized SELECT_ANY string providers receive no
additional target ownership or data credit.

## Verification boundary

- PID-isolated gate: 70 exact, zero residual, zero unwritten.
- `_point_from_line3d` emitted-symbol guard: passed.
- Target and rebuilt BSS: eight bytes, flags `0xC0300080`, zero relocations,
  identical logical payload, and identical semantic owner positions/classes.
- The orchestrator's regenerated full build and semantic report pass. The
  rename-stable whole-tree sweep remains at 6,925 ordinary exact owners with
  zero regressions, the object-admission audit reports no candidate or
  contradiction, and the complete `tools/` suite passes 1,151 tests plus 26
  subtests (five skipped). Canonical therefore records this object among its
  392 admitted `Matching` units.
