# `network_game_manager.obj` and `rasterizer_debug.obj` admission (2026-09-03)

## Result

Two completed compilands were left `NonMatching` after their implementation
packets.  A current-head COFF ownership audit now proves both can be marked
`Matching` without granting credit to candidate-only support sections:

| Object | Exact functions | Exact meaningful code | Exact target data |
| --- | ---: | ---: | ---: |
| `network_game_manager.obj` | 19 / 19 | 3,125 / 3,125 bytes | 668 / 668 bytes |
| `rasterizer_debug.obj` | 12 / 12 | 3,026 / 3,026 bytes | 705 / 705 bytes |

The strict function report has no semantic exception in either unit.  Every
January-owned code section, relocation identity, literal, and mutable-data
owner is directly exact.

## Candidate-only COMDAT boundary

The rebuilt network manager contains eight select-any read-only COMDATs that
January assigns to other compilands.  Each has the same decorated owner name,
logical size, zero-relocation topology, and SHA-256 as its authenticated
January owner:

| Owner | Bytes | January owner |
| --- | ---: | --- |
| `_D3DTEXTUREDIRECTENCODE` | 16 | `cseries/headers.obj` |
| `_D3DSIMPLERENDERSTATEENCODE` | 328 | `interface/progress_bar.obj` |
| `_D3DPRIMITIVETOVERTEXCOUNT` | 88 | `rasterizer_xbox_decals.obj` |
| `"game"` | 5 | `ui_widget_event_handler_functions.obj` |
| `"player"` | 7 | `ai/actor_types.obj` |
| `"p1 && p2"` | 9 | `cseries/cseries.obj` |
| `"game_load() failed."` | 20 | `main/main.obj` |
| `"network_player_is_valid(player)"` | 32 | `network_client_manager.obj` |

The first three are stock XDK lookup tables emitted by every `xtl.h` consumer;
the other five are compiler-pooled assertion literals.  Their COMDAT names
encode identity, and the linker coalesces them into the same single linked
owners rather than creating additional runtime data.  They receive no data
credit in this object.

The rebuilt rasterizer debug object likewise contains only two candidate-only
select-any support owners: the 32-byte `_dot_product3d` header helper and the
four-byte `__real@00000000` constant.  Both are byte-identical, zero-relocation
copies of the January owners in `ai/action_charge.obj`; the rebuilt tree emits
46 and 116 compatible copies respectively.  No candidate-only mutable data or
non-COMDAT runtime owner exists.

This is the established whole-object admission boundary used for compiler and
XDK support COMDATs: exact target-owned code/data is required, while proven
identical select-any duplicates are recorded but are neither credited nor
treated as extra linked content.

## Verification

- Current-head full build, semantic report, strict progress, aligned-rdata,
  and non-code ownership validation pass.
- Stable whole-tree comparison retains the preceding integration's 23 exact
  function / 2,608 padded-byte gain with zero regressions.
- The object-admission audit has no contradiction or rejection for either
  unit.
- Park validation reports 181 active, zero stale, and zero invalid entries.
- The project tooling suite passes 261 tests; the changed-source fake-match
  scan has zero findings.
- Neither rebuilt object contains a `point_from_line3d` symbol or COMDAT.
- No source, symbol name, semantic exception, or comparator rule is changed by
  this accounting-only admission.
