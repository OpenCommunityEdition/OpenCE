# PPC Mac game startup link inventory

The Mac full-startup link at `build/mac-audit/game-startup-link-ppc.txt` initially failed on these 29 undefined symbols. This is a provider inventory for the link boundary only. It does not establish that `game_initialize`, a map load, a simulation tick, or biped movement runs successfully on PPC. The names come from engine initialization call sites, not from proof that the initialization order or providers are ready.

## Engine providers to include and validate

These are genuine engine services already implemented in `source/`; the next source-link batch should include their implementation units and any dependencies exposed by the next link.

| Missing symbol | Engine provider |
| --- | --- |
| `bink_playback_initialize` | `source/bink/bink_playback.c` |
| `input_abstraction_initialize` | `source/input/input_abstraction.c` |
| `initialize_network_game_packets` | `source/networking/network_messages.c` |
| `weather_particle_systems_initialize` | `source/effects/weather_particle_systems.c` |
| `event_manager_initialize` | `source/interface/event_manager.c` |
| `ui_widgets_initialize` | `source/interface/ui_widget.c` |
| `cheats_initialize` | `source/game/cheats.c` |
| `hs_initialize` | `source/hs/hs.c` |
| `player_ui_initialize` | `source/interface/player_ui.c` |
| `saved_game_files_initialize` | `source/saved games/saved_game_files.c` |
| `structure_detail_objects_initialize` | `source/structures/structure_detail_objects.c` |
| `structure_decals_initialize` | `source/structures/structure_runtime_decals.c` |
| `hud_initialize_weapon_interface` | `source/interface/hud_weapon.c` |
| `hud_initialize_unit_interface` | `source/interface/hud_unit.c` |
| `hud_initialize_nav_points` | `source/interface/hud_nav_points.c` |
| `motion_sensor_initialize` | `source/interface/motion_sensor.c` |
| `cache_files_map_directory` | `source/cache/cache_files.c` |
| `game_engines` | `source/game/game_engine_list.c`; its referenced engine objects must also be linked |

The matching call sites are the real `game_initialize` in `source/game/game.c`, `hud_initialize` in `source/interface/hud.c`, `structures_initialize` in `source/structures/structures.c`, and progress-bar initialization in `source/interface/progress_bar.c`. Keep these real calls; do not replace them with empty startup callbacks.

## Native Mac boundary work

These symbols are Windows API or Windows socket names referenced by shared source. Provide/complete the Mac platform boundary with actual filesystem and socket semantics; do not link the Winsock implementation as a Mac provider.

| Missing symbol | Source caller | Required Mac provider |
| --- | --- | --- |
| `CopyFileA` | `source/interface/progress_bar.c` (also saved-game code) | Native file copy compatibility implementation |
| `CreateDirectoryA` | `source/interface/progress_bar.c` | Native directory creation compatibility implementation |
| `halo_ws_bind` | `source/bungie_net/network/transport_endpoint_winsock.c` | Mac socket endpoint implementation or typed POSIX mapping |
| `halo_ws_socket` | same | Mac socket creation |
| `halo_ws_listen` | same | Mac socket listen |
| `halo_ws_getsockopt` | same | Mac socket option query |
| `halo_ws_setsockopt` | same | Mac socket option setting |
| `__control87` | `source/math/real_math.c` | Mac floating-point precision control matching the engine's required precision |

`transport_initialize` is declared/implemented by `source/bungie_net/network/transport_endpoint_set_winsock.c`, but that file is a Winsock endpoint provider and is not sufficient for a native Mac runtime. Add a Mac transport implementation that owns `transport_initialized` and initializes the transport services consumed by the native endpoints. Preserve the shared transport contract.

## Native storage for engine-owned globals

These names are globals expected by shared engine source, rather than callable services. The Mac link currently lacks their storage; define them once with the exact declared types in the Mac native common-storage boundary, and let their real engine initialization functions populate or reset them.

| Missing symbol | Declaration / expected owner |
| --- | --- |
| `scenario_globals` | `source/scenario/scenario.h`; allocated and initialized by `scenario_initialize` in `source/scenario/scenario.c` |
| `game_variant_global` | `source/game/game.c`; reset/used by real game and game-engine initialization |

`game_engines` above is different: it is an initialized pointer table, so link `source/game/game_engine_list.c` and its actual game-engine providers rather than supplying storage.

## Suggested batching

1. Add the missing source units for game, HUD, structures, networking packets, and cache path services. Link again to reveal each unit's real dependency closure.
2. Add the exact-type Mac storage for `scenario_globals` and `game_variant_global`; verify genuine `game_initialize` allocates scenario globals and initializes the variant consumers.
3. Implement native filesystem and socket boundaries, then wire `transport_initialize` to the Mac transport lifecycle. Keep Winsock-specific endpoint code out of the Mac build.
4. Resolve floating-point control using the native Mac API and verify the precision mode around `real_math_reset_precision`.
5. Only after the full link closes, run genuine startup on PPC and record each subsystem's runtime initialization separately. A successful link is not a startup or gameplay result.

The current 29-symbol list is a snapshot from one PPC link attempt. Closing these names may reveal more dependencies; this inventory should be revised from the next link rather than treated as a complete startup census.

## Core-state seam implemented

The native build now owns the exact declared `game_variant_global` and a
checked complete compilation view of game.c. `tools/mac_game_state_boundary.py`
factors only the original core allocation/zeroing prefix into one function used
by full startup and the isolated native core lifecycle. It rejects source drift.
This leaves the game's active/map-loaded flags untouched and preserves full
startup service order. `engine_game_runtime.c` refuses duplicate core ownership,
restores the previous variant and drops the private pointer before arena teardown.
This addresses the observed biped getter fault without a replacement getter or
constant result. The original 29-symbol census predates this variant provider;
it is not a current full-startup link result.
