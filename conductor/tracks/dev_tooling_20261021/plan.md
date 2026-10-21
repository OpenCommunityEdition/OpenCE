# Plan: The developer shell, engine hooks and a test map

## Phase 1: Commands, variables and the control socket
- [ ] Task: The registry of commands and variables
    - [ ] Write tests: commands with typed arguments parse and run, and give errors naming the bad argument; variables keep their type, default and saved value; listeners hear changes; a name that is not a command falls through to HaloScript
    - [ ] The `opence_dev` crate: the registry, the typed arguments and variables, the HaloScript fall-through to `opence_script`; the FFI calls Swift uses
- [ ] Task: The first commands
    - [ ] Write tests: each command of the spec's list changes the world as it says, in a headless world over a fake level
    - [ ] `map`, `bsp`, `spawn`, `teleport`, `noclip`, `god`, `give`, `enter`, `kill`, `time`, `pause`, `step`, `objects`, `inspect`, `tags`, `vars`, `set`, `bind`, `exec`, `help`, `quit`
- [ ] Task: The headless mode and the control socket
    - [ ] Write tests: requests and replies as JSON lines; oversized, deeply nested and too-frequent requests are refused; the socket is created with the user's permissions only; `--headless` runs and draws a frame offscreen on request
    - [ ] The socket server, the structured requests (`state`, `objects`, `frame`, `subscribe log`), `--headless`
- [ ] Task: The `opence` tool
    - [ ] Write tests: `opence run`, `opence frame`, `opence watch` and `opence script` against a headless engine
    - [ ] The binary, with completion from the engine's registry in `opence shell`
- [ ] Task: Conductor - User Manual Verification 'Phase 1: Commands, variables and the control socket' (Protocol in workflow.md) (deferred to the end)

## Phase 2: The shell overlay
- [ ] Task: Monospace text
    - [ ] Write tests: the glyph atlas at scales 1 and 2; the font's licence and checksum are present
    - [ ] Overpass Mono in `port/assets/fonts` with its licence; a glyph atlas and text pass in the renderer
- [ ] Task: The terminal
    - [ ] Write tests: XCTest of line editing, history search, completion candidates, scrollback, selection and copy, coloured output
    - [ ] The overlay: opening with `~`, input capture, the log stream and its filter, the watch line
- [ ] Task: Conductor - User Manual Verification 'Phase 2: The shell overlay' (Protocol in workflow.md) (deferred to the end)

## Phase 3: Hooks
- [ ] Task: Debug drawing
    - [ ] Write tests: shapes with lifetimes expire on time; the layers switch; nothing is drawn or allocated when they are off
    - [ ] The immediate-mode API in Rust, the snapshot's debug buffer, the Metal pass; the collision, body, AI, trigger, marker, light and sound layers
- [ ] Task: Live shader reload
    - [ ] Write tests: an edited source recompiles and swaps; a broken one keeps the old pipeline and reports the error
    - [ ] Watching the shader sources in a development build
- [ ] Task: The test driver
    - [ ] Write tests: `wait`, `wait_until`, `expect` and `frame` in `.ocs` scripts; a failed `expect` reports its line and state
    - [ ] The script runner, headless and in the window
- [ ] Task: Profiling marks
    - [ ] Write tests: nested scopes add up; the trace file is valid
    - [ ] `profile_scope!`, the Swift equivalent, `stat frame`, the trace writer
- [ ] Task: Conductor - User Manual Verification 'Phase 3: Hooks' (Protocol in workflow.md) (deferred to the end)

## Phase 4: The test map
- [ ] Task: A level made by the engine
    - [ ] Write tests: the generated plane's collision and render geometry; the player spawns on it; the sky is drawn
    - [ ] A builder of `content::Level` values that are not imported from a cache
- [ ] Task: Gathering every object
    - [ ] Write tests: definitions from several packs are gathered once each by tag path, keeping their own data; the galleries are laid out without overlap
    - [ ] The galleries with labels; `--map devmap` and `map devmap`
- [ ] Task: The stations
    - [ ] Write tests: each station's script passes over the user's maps (marked, needs `assets/maps`)
    - [ ] The weapons, vehicles, animation, doors and physics stations
- [ ] Task: Conductor - User Manual Verification 'Phase 4: The test map' (Protocol in workflow.md) (deferred to the end)

## Phase 5: Placement checks
- [ ] Task: Recording the reference game's placements
    - [ ] Write tests: the recorder's output format; a small map's trace is complete
    - [ ] `tools/engine_placement_traces.py`, using the reference app headless
- [ ] Task: Comparing and fixing placements
    - [ ] Write tests: the engine's world after spawning matches each stock map's trace (marked, needs `assets/maps`)
    - [ ] Find and fix the causes of the mismatches in the importer and the simulation
- [ ] Task: The documentation
    - [ ] `docs/engine.md`: the shell, the commands and variables, the socket and `opence`, debug drawing, the test driver, the test map
- [ ] Task: Conductor - User Manual Verification 'Phase 5: Placement checks' (Protocol in workflow.md) (deferred to the end)
