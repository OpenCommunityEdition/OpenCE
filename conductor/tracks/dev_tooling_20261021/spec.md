# Spec: The developer shell, engine hooks and a test map

## Overview
Each later track (lighting, the UI, animation, physics, particles, sound, coverage) is quicker to build and check with tools that reach into the running engine. This track builds those tools first. It also tracks down why objects are placed oddly in some maps, because the tools make that visible and every later track depends on objects being in the right place.

It has five parts:
- **The shell:** a terminal-like overlay in the game. It runs HaloScript and engine commands, with completion, history, scrollback and the engine's log.
- **The control socket:** the same commands from outside the app, through a local socket and an `opence` command-line tool. A terminal, a script or an agent can drive the engine, read its state and take frames, with no window in focus.
- **Engine hooks:** a registry of commands and variables that every crate can add to; debug drawing; live reload of shaders; a test driver that runs command scripts.
- **The test map:** a generated level, a flat plane under a sky, that places every object definition found in the user's maps. Every weapon, vehicle, biped, device and piece of scenery can be spawned, used and inspected there.
- **Placement checks:** each object's placement in the engine is compared with the reference game's, and the cause of the odd placements is fixed.

## Functional Requirements

### Commands and variables
1. A `devtools` module in a new crate, `opence_dev`, holds a registry of commands and variables. Each has a name, typed arguments, help text and a handler. Crates register theirs at start-up; nothing is hard-coded in the shell.
2. Variables are typed (bool, int, float, string, enum) with a default. Some are saved to the settings file, and changes to them are announced to listeners. Debug switches are variables, for example `render.wireframe`, `render.lightmaps`, `sim.game_speed`, `ai.enabled` and `debug.draw.collision`.
3. HaloScript runs through the same entry point: anything that is not a registered command is parsed and evaluated by `opence_script` against the running mission, so `(ai_place x30_marines)` and `object_create warthog` both work.
4. The first commands, at least: `map <name> [bsp]`, `bsp <index>`, `spawn <tag> [count]`, `teleport <x y z> | <object>`, `noclip`, `god`, `give <weapon>`, `enter <vehicle> [seat]`, `kill`, `time <scale>`, `pause`, `step [ticks]`, `frame <path>` (save a frame), `objects [filter]` (list with positions), `inspect <object>`, `tags <class> [filter]`, `vars [filter]`, `set <var> <value>`, `bind <key> <command>`, `exec <file>`, `help [name]`, `clear`, `quit`.

### The shell overlay
5. The shell opens with the `~` key (the stock developer console's key) and slides down over the game. The game keeps running unless `pause` is set. While the shell is open, input goes to it.
6. It behaves like a modern terminal:
   - a prompt with line editing (cursor movement, word jumps, kill and yank);
   - history kept between runs, searched with Ctrl-R;
   - Tab completion of commands, variables, tag paths, object names and HaloScript functions and globals, with a list of candidates;
   - scrollback with the mouse wheel, selection and copy;
   - coloured output: errors, warnings, the engine's log and the results of expressions.
7. The engine's log (the `log` crate's records from Rust, and Swift's log) streams into the shell. It can be filtered by level and target (`log filter render=debug`).
8. Text is drawn with a monospace font. Overpass Mono, from the same OFL-licensed Overpass family the project already uses, is added under `port/assets/fonts` with its licence and checksum. It is drawn with a glyph atlas at the display's scale, so it is sharp at any resolution.
9. A one-line overlay of watched values (`watch <expression>`) stays on screen when the shell is closed.

### The control socket and the `opence` tool
10. When run with `--control` (or `debug.control_socket`), the engine listens on a Unix domain socket in the user's runtime folder, readable only by the user. It never listens on a network address.
11. Each request is one JSON line: a command line, or a structured request (`state`, `objects`, `frame`, `screenshot`, `subscribe log`). Each reply is one JSON line. Every result that the shell shows as text is also available as structured data.
12. `opence` is a small Rust binary in the workspace:
    - `opence run '<command>'` runs one command;
    - `opence shell` is an interactive terminal on the socket, with the same completion;
    - `opence frame out.png` saves a frame;
    - `opence watch` streams the log;
    - `opence script file.ocs` runs a command script.
13. With `--headless` the engine runs with no window, at a fixed or unthrottled tick, drawing frames only when asked (offscreen, at a given size). Tests and agents use this mode; it never opens a window on the user's desktop.

### Hooks for quick development
14. **Live reload:** the Metal shader sources are watched in a development build, and an edited shader is recompiled and swapped without restarting. A failed compile keeps the old pipeline and prints the error to the shell.
15. **Debug drawing:** an immediate-mode API in Rust (`debug_line`, `debug_box`, `debug_sphere`, `debug_text_3d`, each with a lifetime in ticks) that any system can call. The renderer draws it after the world. It has layers that can be switched on and off: collision, physics bodies, AI paths and firing positions, trigger volumes, object origins and markers, lights, sound emitters.
16. **The test driver:** command scripts (`.ocs`) are lists of commands with `wait <ticks>`, `wait_until <expression> [timeout]`, `expect <expression>` and `frame <name>`. They run in the headless mode in tests, and in the window for a person to watch. A failed `expect` stops the script and reports the line, the expression and the state.
17. **Profiling marks:** named timers in Rust and Swift (`profile_scope!`) shown as a frame-time breakdown overlay (`stat frame`), and written as a trace for Instruments or Perfetto on request.

### The test map
18. `devmap` is a level made by the engine, not imported from a cache. Its pieces:
    - a flat, square plane with collision, a grid texture and a generated sky (or, when given, the sky of an imported map);
    - the player's start;
    - galleries, one per object kind, laid out in rows with a label above each object: weapons on racks, vehicles in bays, bipeds on pedestals (playing their idle), devices (doors, lifts, switches, control panels), scenery, equipment and projectiles.
19. It gathers its objects from every map in the data folders: each distinct definition (by tag path) is placed once, from the map whose `.ocpak` holds it. Its definitions keep the tag data they came from, so each one draws, animates and behaves as it does in its own map.
20. Devices in the test map can be opened, closed and powered from their control panels and from commands. Vehicles can be entered. Weapons can be picked up from the racks.
21. Test stations, each started from a pad on the floor or a command (`station <name>`), run a test script on the spot:
    - `station weapons`: fire each weapon at targets and show its rate of fire, damage and heat;
    - `station vehicles`: a loop course with ramps;
    - `station animation`: a biped that plays each animation of its graph in turn, with its name shown;
    - `station doors`: every device cycling through its states;
    - `station physics`: a pile of crates and a slope.
22. The test map can be loaded with `--map devmap` or `map devmap`. It is not offered in the game's menus.

### Placement checks
23. A tool records the reference game's world after spawning (each object's tag, position, rotation, permutation, BSP and parent), for each stock map, using the reference app headless as the movement and animation traces do (`tools/engine_movement_traces.py`). The traces are kept in `build/`, never committed.
24. A test compares the engine's world after spawning with the trace. Positions must match within 1 mm and rotations within 0.01°. Each mismatch is reported with its object, its placement block and the difference.
25. The causes found are fixed in the importer or the simulation. Suspects include:
    - the order of yaw, pitch and roll;
    - objects that belong to only some BSPs;
    - objects attached to their parent's markers;
    - device groups and their initial positions;
    - placements flagged to not spawn, or to spawn only on some difficulties;
    - scenery that drops to the ground at spawn.

## Non-Functional Requirements
- The shell and the debug drawing cost nothing when closed and switched off: no allocation and no draw calls each frame.
- Nothing in the release build listens on a socket unless `--control` is given.
- The control socket's requests are checked like any other input: sizes, nesting and rates.
- Every command and variable has help text, and `docs/engine.md` gets a section on the tools.

## Acceptance Criteria
- `~` opens the shell. `map a30`, `spawn warthog`, `(ai_place x)` and `set render.wireframe true` work. Completion and history work.
- `opence run 'objects biped'` lists a30's bipeds as JSON from another terminal, and `opence frame` saves a frame of the headless engine.
- A shader edit reloads while the game runs.
- `map devmap` loads the test map with every object definition from the user's maps, and every station's script passes.
- The placement test passes on every stock map. The odd placements are gone.

## Out of Scope
- A gamepad (the engine is keyboard and mouse only).
- Editing levels and saving them as maps.
- Remote access over a network.
