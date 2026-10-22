# Plan: The developer shell, engine hooks and a test map

## Phase 1: Commands, variables and the control socket [checkpoint: 2fa9bbf2]
- [x] Task: The registry of commands and variables 1ae39b0e
    - [x] Write tests: commands with typed arguments parse and run, and give errors naming the bad argument; variables keep their type, default and saved value; listeners hear changes; a name that is not a command falls through to HaloScript
    - [x] The `opence_dev` crate: the registry, the typed arguments and variables, the HaloScript fall-through to `opence_script` (the FFI calls Swift uses moved to the next task, where the engine's context exists)
- [x] Task: The first commands 91a102ad
    - [x] Write tests: each command of the spec's list changes the world as it says, in a headless world over a fake level
    - [x] The FFI calls Swift uses: running a line, completion, the variables
    - [x] `map`, `bsp`, `spawn`, `teleport`, `noclip`, `god`, `give`, `enter`, `kill`, `time`, `pause`, `step`, `objects`, `inspect`, `tags`, `vars`, `set`, `bind`, `exec`, `help`, `quit`
- [x] Task: The headless mode and the control socket 91a102ad
    - [x] Write tests: requests and replies as JSON lines; oversized, deeply nested and too-frequent requests are refused; the socket is created with the user's permissions only; `--headless` runs and draws a frame offscreen on request
    - [x] The socket server, the structured requests (`state`, `objects`, `frame`, `subscribe log`), `--headless`
- [x] Task: The `opence` tool 87593c75
    - [x] Write tests: `opence run`, `opence frame`, `opence watch` and `opence script` against a headless engine
    - [x] The binary, with completion from the engine's registry in `opence shell`
- [ ] Task: Conductor - User Manual Verification 'Phase 1: Commands, variables and the control socket' (Protocol in workflow.md) (deferred to the end)

## Phase 2: The shell overlay [checkpoint: 2b9e935e]
- [x] Task: Monospace text ae764736
    - [x] Write tests: the glyph atlas at scales 1 and 2; the font's licence and checksum are present
    - [x] Overpass Mono in `port/assets/fonts` with its licence; a glyph atlas and text pass in the renderer
- [x] Task: The terminal acd64406
    - [x] Write tests: XCTest of line editing, history search, completion candidates, scrollback, selection and copy, coloured output
    - [x] The overlay: opening with `~`, input capture, the log stream and its filter, the watch line
- [ ] Task: Conductor - User Manual Verification 'Phase 2: The shell overlay' (Protocol in workflow.md) (deferred to the end)

## Phase 3: Hooks [checkpoint: 68071ae2]
- [x] Task: Debug drawing d66e7195
    - [x] Write tests: shapes with lifetimes expire on time; the layers switch; nothing is drawn or allocated when they are off
    - [x] The immediate-mode API in Rust, the snapshot's debug buffer, the Metal pass; the collision, body, AI, trigger, marker, light and sound layers
- [x] Task: Live shader reload c7ccbda5
    - [x] Write tests: an edited source recompiles and swaps; a broken one keeps the old pipeline and reports the error
    - [x] Watching the shader sources in a development build
- [x] Task: The test driver 7971f7a5
    - [x] Write tests: `wait`, `wait_until`, `expect` and `frame` in `.ocs` scripts; a failed `expect` reports its line and state
    - [x] The script runner, headless and in the window
- [x] Task: Profiling marks 7971f7a5
    - [x] Write tests: nested scopes add up; the trace file is valid
    - [x] `profile_scope!`, the Swift equivalent, `stat frame`, the trace writer
- [ ] Task: Conductor - User Manual Verification 'Phase 3: Hooks' (Protocol in workflow.md) (deferred to the end)

## Phase 4: The test map [checkpoint: 3d6df48a]
- [x] Task: A level made by the engine 94fe7da8
    - [x] Write tests: the generated plane's collision and render geometry; the player spawns on it; the sky is drawn
    - [x] A builder of `content::Level` values that are not imported from a cache
- [x] Task: Gathering every object b93bbd80
    - [x] Write tests: definitions from several packs are gathered once each by tag path, keeping their own data; the galleries are laid out without overlap
    - [x] The galleries with labels; `--map devmap` and `map devmap`
- [x] Task: The stations 9f3b2c79
    - [x] Write tests: each station's script passes over the user's maps (marked, needs `assets/maps`)
    - [x] The weapons, vehicles, animation, doors and physics stations
- [ ] Task: Conductor - User Manual Verification 'Phase 4: The test map' (Protocol in workflow.md) (deferred to the end)

## Phase 5: Placement checks [checkpoint: 8b23535d]
- [x] Task: Recording the reference game's placements d2f2db02
    - [x] Write tests: the recorder's output format; a small map's trace is complete
    - [x] `tools/engine_placement_traces.py`, using the reference app headless
- [x] Task: Comparing and fixing placements 238ad5be
    - [x] Write tests: the engine's world after spawning matches each stock map's trace (marked, needs `assets/maps`)
    - [x] Find and fix the causes of the mismatches in the importer and the simulation
- [x] Task: The documentation 3ec72c52
    - [x] `docs/engine.md`: the shell, the commands and variables, the socket and `opence`, debug drawing, the test driver, the test map
- [ ] Task: Conductor - User Manual Verification 'Phase 5: Placement checks' (Protocol in workflow.md) (deferred to the end)
