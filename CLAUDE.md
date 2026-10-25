# Working on OpenCE

These are the practices the project has settled on while building the new
engine. They come before `AGENTS.md` where the two differ: `AGENTS.md`
describes the old port (Linux, Windows, Android and the web, the
decompilation compiled as it is), and some of its rules no longer apply.

## What the project is now

- **macOS only.** The game is a new engine in Rust (`engine/crates/`) with a
  Swift app (`engine/app/`), drawn with Metal. There is no VM, no guest and
  no 32-bit addressing. `port/macos/` is the old hosted port, kept until the
  engine replaces it.
- **The decompilation is a reference.** `source/` says how the original
  behaves; the engine is not compiled from it, and no new C is written.
  The rules in `AGENTS.md` about changing `source/` with `port:` comments
  apply only if `source/` itself is changed.
- **Port, match, then extend.** Lighting, shaders and graphics are
  rewritten and enhanced. AI, particles, animation, sound, script logic
  and gameplay are ported from `source/` and matched to it (reference
  traces) before anything is extended.
- **Keyboard and mouse only.** No gamepad or controller support; pad
  events are filtered out.
- **Departures from stock** need not be off by default in the engine
  (the `AGENTS.md` settings rule was the old port's). The lighting is the
  enhanced lighting only. The UI is built from the user's own `ui.map` at
  native resolution (4K), not held to the original's low resolution.
- **The launcher** shows Play, Settings and Exit, and its window can be
  resized. Its Developer tab is for development.
- **Nix is all the user needs**: `nix run .#engine` runs the game and
  `nix build .#engine` builds it; `nix develop` is the shell to work in.

## Still in force from `AGENTS.md`

- Never read the Xbox SDK's headers or anything derived from them; do not
  read `xbox/include`.
- The repository is CC0. GPL code (OpenSauce, Reclaimer) is read as
  documentation of formats, never copied. Vendored code goes in
  `port/third_party/<name>` with a README.
- Map, tag and other game files are untrusted input: read them through
  bounds-checked readers, and fuzz the readers.
- Art: hand-drawn redraws; fonts: Overpass and OpenCE only.
- Nothing from the user's game data (maps, `ui.map`, the xbe, Bungie's
  sounds) goes into the repository or the app bundle. Generated data
  derived from it (devgulch, golden frames, bakes) stays on the user's
  machine.
- Do not imitate Bungie's or Microsoft's branding.

## Building and testing

- `nix develop`, then `python tools/engine_build.py` (or the crane
  packages, `nix build .#engine`). Do not leave `nix flake check` broken.
- Rust lints for the workspace (`engine/Cargo.toml`): `unsafe_code` is
  denied, `missing_docs` warns, clippy's `unwrap_used` and `expect_used`
  are denied, and release builds abort on panic. Write code that passes
  them rather than allowing them.
- Tests that need the user's data (`assets/maps`, `Haloce_dump`) skip
  themselves when it is missing; they never fail for want of it.
- Write the test first (`conductor/workflow.md`), and run the whole
  workspace's tests before a commit.

## Matching the original

- **Reference traces.** Behaviour is checked against the original's own
  functions: a `tools/engine_*_reference.py` script compiles the needed
  functions from `source/` against a small fake world, records a trace,
  and `engine/crates/sim/tests/*_reference.rs` compares the engine with
  it. `--check` fails when the trace is stale.
- **Determinism.** The simulation runs at 60 Hz, scripts at 30 Hz, in a
  fixed `SimSet` order. Maths goes through `libm` (and `glam` with its
  libm feature), with no fused multiply-add. Maps the simulation iterates
  must iterate in a fixed order. A new crate that does floating-point maths
  in the simulation must follow the same rules.
- **Measure, don't assume.** Compare with the stock game by measurement
  (lightmap seams, shadow edges, timings), in systematic sweeps. The
  user's own look found blocky shadows that the tests had missed; a check
  that passes is not proof that the result looks right.

## Running the game

- Run headless only, never on the user's desktop, and stop only the
  processes you started.
- **devgulch** is the test level: a copy of Blood Gulch made from the
  user's map at import, with a station for each system
  (`--map devgulch --station vehicles`).
- Every dev panel control is also a shell command and a control socket
  call (the `opence` tool), so anything the user can do in the panel can
  be scripted and tested.
- Bakes and probes never run during play: at import, or behind the loading
  screen, cached by the map's hash.

## How the work runs

- **Claude writes all the code.** The user reads it. Do not leave
  `TODO(human)` or ask the user to write code; explanatory insight notes
  are welcome.
- **Conductor tracks** (`conductor/tracks.md`) hold the plans. A fidelity
  track runs all its phases to the end without stopping for manual
  checks, with automated checks at each phase's end, and then a
  data-backed report that sets up the next fix track. New features come
  only after the user is satisfied with fidelity.
- **Designs** (screens, menus, layouts) are drawn as static prototypes in a
  Claude design artifact, not as markdown.
- **Commits.** The subject is a plain sentence saying what changed; the
  body says why and what was checked. Stage only your own files: other
  sessions often have uncommitted work in the same checkout. Do not push
  unless asked.

## Nix

- The flake's outputs are grown with std from the cells in `nix/` (`repo`,
  `macos`, `engine`). std copies each cell into the store on its own, so
  refer to the repository through the toolchain's `root` path, never
  `../..`.
- Pass paths, not `"${inputs.self}/..."` strings, to `importCargoLock` and
  crane: a path copies only the named file, so vendored crates build again
  only when the lock file changes.
