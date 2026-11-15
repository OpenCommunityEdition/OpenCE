# Forge

Multiplayer > **Forge** opens an in-game 3D map editor on any Xbox (`v5`),
Custom Edition (`v609`) or MCC (`v13`) multiplayer map. Players can build alone
or together over system link, internet play and dedicated servers, save maps as
`.txt` sidecar files in `custom_maps/`, and play those saved maps in any
standard gametype.

## Controls

All Forge bindings can be customized in **Settings > Control Setup** (`forge_controls.ini`).

| Action | Keyboard / Mouse | Controller |
| --- | --- | --- |
| Toggle Play / Monitor mode | `F1` | D-Pad Up |
| Open Spawn Palette | `F2` | `X` (when not holding an object) |
| Object Properties | `X` | `X` (when holding or aiming at an object) |
| Toggle 3rd-Person Monitor view | `F4` | — |
| Quick Save / Save Map As | `F5` | Pause > Save Forge Map |
| Grab / Drop object | Left Mouse Button | `A` / `B` |
| Delete object | `Delete` / `Backspace` | `Y` |
| Rotate held object | Hold `R` + Mouse | Hold Right Trigger + Right Stick |
| Cycle rotation snap (`Off`, `15°`, `45°`, `90°`) | `G` | D-Pad Left / Right |
| Snap held object to ground | `T` | Left Stick Click |
| Clone aimed object | `C` | Right Stick Click |
| Undo / Redo | `Ctrl+Z` / `Ctrl+Y` | — |
| Assign / toggle Weld Group (`1..16`) | `Ctrl+G` | Object Properties > Weld Group |
| Monitor vertical flight (Ascend / Descend) | `Space` / `Ctrl` | Right Bumper / Left Bumper |

## Sidecar map files (`custom_maps/<name>.txt`)

Saving a map writes `custom_maps/<name>.txt` beside your existing maps without
altering any `.map` file on disk. Each sidecar records:

- `Base Map:` — the underlying `maps/<name>`, `custom_maps/<name>` or
  `mcc_maps/<folder>/<name>` cache file.
- `DEFAULT_EDIT|` / `DEFAULT_PROPS|` / `DEFAULT_GAMETYPE|` — moved, rotated,
  deleted or reconfigured stock scenario objects, matched by `(group_tag,
  tag_path)` hash and original scenario coordinates.
- `OBJECT|` / `OBJECT_PROPS|` / `OBJECT_GAMETYPE|` — Forge-placed objects,
  including physics mode (`Normal`, `Fixed`, `Phased`), respawn timer, team
  alignment, weld group, dynamic light colour/intensity/radius presets and
  gametype filter bits.
- `SPAWN|`, `NETGAME_FLAG|` and `AI|` — player starting locations, two-way
  teleporter pairs (`Reference ID 1..16`), CTF/Oddball/King/Race markers and AI
  encounters.

Saved `.txt` maps appear under **Forge Maps** in the map list. Launching a saved
map from **Standard** multiplayer loads all placed objects, custom tags, lights,
teleporters and objective markers while keeping Monitor flight and editor
hotkeys off.

## Cross-map tag importing & Tag Options

In the Forge lobby (**Tag Options**), **Add Tags** scans installed `.map` files
across `maps/`, `custom_maps/` and `mcc_maps/`. Highlighting a vehicle, weapon,
biped, equipment or scenery tag renders a 3D software preview (`tag_preview.c`)
before importing.

Importing a tag extracts its root definition and recursive dependency closure
(models, animations, shaders, bitmaps, collision, physics, projectiles, effects
and sounds) into a compressed `.fbrtag` (`FBR2`) bundle in the local Tag Vault
(`OpenCE Forge/tag_vault/`). At runtime, `forge_runtime_tags.c` appends the
cloned tag hierarchy to the active tag table in RAM and converts Custom Edition
(`mod2`) and MCC (`v13`) blocks to the engine's runtime layout.

## Bidirectional custom tag streaming (`FTRQ` / `FTST`)

When a host or joiner spawns an imported tag in a multiplayer session (or when a
joiner connects to a hosted game that uses custom tags), peers that do not have
the tag locally request it over the distributed network channel:

- `_distributed_message_forge_tag_request` (`FTRQ`) and
  `_distributed_message_forge_tag_stream` (`FTST`: `OFFER`, `CHUNK`, `MISS`)
  stream Zlib-compressed `FBR2` bundles in 1,024-byte chunks at up to 256 KB/s
  whenever reliable queue backlog is below 8 KB.
- Every offer carries a `BLAKE2b-256` digest verified before decompression and
  bounds-checked recursive tag installation.
- Received bundles are installed in session RAM (`source_cache_id = 4`) and are
  only written to disk if the player explicitly saves the map (`F5`) or clicks
  **Save Local Tags** in **Tag Options**.
- Setting `network.forge_tag_download = false` in `config.toml` disables
  receiving streamed tags; missing custom objects then fall back to a visible
  placeholder crate.
