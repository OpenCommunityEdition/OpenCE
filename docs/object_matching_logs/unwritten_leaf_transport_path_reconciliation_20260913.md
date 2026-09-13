# Unwritten transport/path leaf reconciliation (2026-09-13)

## Disposition

This bounded wave audited four small unwritten leaves in
`main.obj`, `model_animations.obj`, `path_obstacle_avoidance.obj`, and
`transport_endpoint_set_winsock.obj`. It admits one natural strict-exact
transport owner and one coherent zero-credit obstacle-path reconstruction.
The two remaining leaves are withheld at authentic owner boundaries rather
than filled with private layout guesses or address-derived names.

## Exact transport lifecycle leaf

`transport_dispose` is strict exact on its natural status-returning source:

| Owner | Meaningful / padded bytes | Relocations | Result |
| --- | ---: | ---: | --- |
| `_transport_dispose` | 41 / 48 | 4 / 4 | strict exact |

January tests `transport_initialized`, calls `WSACleanup` and then
`XNetCleanup` only when active, clears the named boolean, and returns
`_transport_error_none`; the inactive path returns
`_transport_error_not_initialized`. The ordinary `short result` source
naturally reproduces January's retained zero in ESI and its AX return without
register, volatile, inline, or code-generation pressure.

The ABI repair is owned by `bungie_net/network/transport.h` as
`short transport_dispose(void)`. The stale consumer-local
`void transport_dispose(void)` declaration was removed from `game.c`.
Every direct includer of the owner header was compiled with the universal
gate and the forbidden-emitted-symbol guard. Their final exact counts are:

- `transport_endpoint_winsock` 17;
- `transport_endpoint_set_winsock` 18;
- `transport_address` 5;
- `telnet_console` 5;
- `game` 25;
- `network_server_message_handler` 20;
- `network_server_manager` 70;
- `network_game_globals` 26;
- `network_connection` 11;
- `network_client_message_handler` 17;
- `network_client_manager` 52;
- `ui_widget_game_data_input_functions` 39;
- `ui_widget_event_handler_functions` 100.

The header declaration was also temporarily removed and the stale Game-local
declaration restored to check the historically sensitive UI consumer. Its
current 39 exact owners and Game's 25 exact owners were identical in both
states, so the proper owner correction loses no inherited current exact
function. The transport object advances from 17 to **18 exact functions**, a
genuine gain of **41 meaningful / 48 padded bytes**.

## Coherent obstacle-path reconstruction, zero exact credit

`render_debug_obstacle_path` is now source-complete from the January control
flow and the existing typed Path APIs. It handles the R/G/N/I debug keys,
updates the named debug flags, calls `path_new`/`path_iterate`, and renders the
named obstacle/path snapshots. Its candidate is instruction-for-instruction
and relocation-position/type identical to January across all **224 padded
bytes**.

It remains an honest `[reloc-identity]` residual and earns **zero exact
credit**. January's split labels the private aggregate at offset zero as
`_current_traverse_index`, with the obstacle snapshot at addend `0x1538`.
The coherent source owns those regions as `debug_path` and
`debug_obstacles`, consistent with their types and use. Renaming either
semantic global to the split artifact or spelling raw BSS offsets would
violate the project's naming and authenticity rules. The focused object gate
is therefore **19 exact / 5 residual / 0 unwritten**, preserving all 19
inherited exact owners while recording the best complete reconstruction for
future data-boundary research.

## Withheld leaves

- `main_framerate_render` (672 padded bytes) is behaviorally understood, but
  January reads the real HUD-global font reference at offset `0x54`. The
  owning `hud.h` does not yet expose that genuine schema. A TU-private prefix,
  raw offset access, or substituting `interface_get_tag_index` would be fake
  ownership/source, so the function remains unwritten.
- `animation_update_internal` (304 padded bytes) has HCEA-authenticated logic
  and a 32-bit update-kind ABI, but the current shared declaration uses a
  16-bit parameter. Prior natural work reached 98.432205%; correcting the
  owner schema perturbed exact consumers, while duplicating its types in the
  implementation would be false ownership. It remains unwritten pending a
  coordinated header packet.
- The target-only 16-byte empty transport leaf has no authenticated semantic
  name. No `code_00070420` definition or invented private name was added.

## Verification

Focused universal gates finish at:

- `transport_endpoint_set_winsock`: 18 exact / 7 residual / 1 unwritten;
- `path_obstacle_avoidance`: 19 exact / 5 residual / 0 unwritten;
- `main`: 82 exact / 11 residual / 2 unwritten;
- `model_animations`: 25 exact / 6 residual / 1 unwritten.

All four gates and all direct transport-header consumer gates pass the
`_point_from_line3d` emitted-symbol guard. `tools/fake_match_scan.py` reports
zero review leads in the changed C/header packet, and `git diff --check`
reports no errors. No global configure, Ninja build, commit, publication, or
park/config edit was performed by this wave.
