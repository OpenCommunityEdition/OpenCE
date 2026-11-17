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
map from **Standard** multiplayer loads all placed objects, donor tags, lights,
teleporters and objective markers while keeping Monitor flight and editor
hotkeys off.

## Retail Xbox tag cache & Tag Options

On first launch, Forge indexes stock object tags from the installed retail Xbox
(`v5`) maps in `maps/` into `OpenCE Forge/cached_tags/xbox_catalog.txt` and
`OpenCE Forge/credits.md` (`source_cache_id = 0`), and scans object tags
already present in the active map (`source_cache_id = 3`, shown as `[MAP]`).

- **Installed Tags** in **Tag Options** lists the stock Xbox donor tags by
  category and renders a live 3D software preview (`tag_preview.c`).
- **Reset Stock Tags** rebuilds `xbox_catalog.txt` and `credits.md` from `maps/`
  if retail map files change.
- **Local donor & map tag resolution**: players resolve stock donor tags locally
  from `maps/` and map-local `[MAP]` tags from the active level, while
  lightweight chat-channel messages synchronize spawns, movement and property
  edits.
- **Map tag safety checks**: loaded-map object tags are checked on load and
  before runtime resolution (`forge_validate_safe_object_tag`) to reject corrupt
  definitions, out-of-range bounding spheres or node counts, dangling tag
  references and malicious creation effects.
