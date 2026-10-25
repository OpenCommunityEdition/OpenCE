# Spec: Rust crates and tools for the engine

## Overview

The engine (`engine/`, Rust with a Swift app) grew on a small set of
dependencies: `bevy_ecs`, `rkyv`, `glam` with libm, `libm`, `uniffi`,
`lewton`, `miniz_oxide`, `sha2`, `thiserror` and `log`. The dev shell
already has `cargo-nextest`, `cargo-llvm-cov`, `cargo-deny` and
`cbindgen`, and `engine/fuzz` has a `cargo-fuzz` shell.

This track adopts the crates and tools reviewed on 2026-10-24 that serve
three goals:

1. **Faster development**: less hand-written parsing and layout code,
   shorter build and test loops.
2. **Help for Claude**: tests whose failures say what changed, a measure
   of how strong the tests are, and logs and timings that can be read
   without the user's screen.
3. **A faster game on the user's Mac**: fewer allocations and hashes in
   the frame, a better optimised build, baked assets that load faster.

Each crate replaces code or a step that exists already, or gives a check
that the engine does not have. None is taken for its own sake.

## The rule for every crate

- **Determinism.** A crate whose floating-point maths runs in the
  simulation must use libm (or `glam`'s libm feature) and must not fuse
  multiply-adds. The simulation's tests that compare against the
  original's traces (`sim/tests/*_reference.rs`) must give the same
  results before and after.
- **Licence.** Each crate passes `cargo deny check` (licences compatible
  with a CC0 repository, no advisories, no duplicate versions without a
  reason). GPL crates are not used.
- **Where it runs.** Threads (`rayon`) only in bake and import, never in
  the simulation. The audio thread gets no locks and no allocations.
- Each adoption is one commit that says what the crate replaced and what
  was measured.

## Functional requirements

### 1. Quick wins
- `insta` snapshot tests for text and structured outputs (the dev state,
  the bake's reports, the import's tag dumps, the shell's command output).
- `pretty_assertions` in the tests that compare large structures.
- `cargo nextest` as the way tests run, in the dev shell and in
  `tools/engine_build.py`'s test step.
- A `deny.toml` for the workspace, with `cargo deny check` in the test
  step.
- `rustc-hash` (`FxHashMap`) for hot maps, and `indexmap` wherever a map
  is iterated in the simulation, so that iteration order no longer depends
  on std `HashMap`'s random seed. A test runs one sim twice in one process
  with different hasher seeds and compares the state.
- `[profile.dev.package."*"] opt-level = 2` so that dependencies are fast
  in debug builds, and `sccache` in the dev shell.
- An optimised test profile for the workspace's test run: the suites that
  step the simulation on the user's maps are the engine's own crates, which
  the dependency opt-level does not reach, and in debug they take most of
  a 40-minute run. The profile keeps debug assertions and overflow checks,
  and the clippy lints stay as they are (they are checked when compiling).

### 2. Test strength
- `cargo-mutants` over `sim`, `physics` and `script`, with its survivors
  listed in a report; tests are added for the survivors that matter.
- `proptest` for the parsers (tag readers, cache readers, script
  compiler) and for the physics' invariants.
- `cargo llvm-cov` with a coverage floor per crate, checked by the test
  step.
- Fuzz targets in `engine/fuzz` for every reader of untrusted files (map,
  tag, sound, bitmap readers).
- `swift-snapshot-testing` golden frames for the app: a fixed station on
  devgulch rendered headless and compared with a tolerance.

### 3. Formats
- `binrw` (or `zerocopy` where the layout is plain) for the tag and cache
  layouts the import reads by hand.
- `memmap2` for reading `.map` files.
- `bytemuck` for the structures copied into Metal buffers.
- `blake3` to know when a baked asset is out of date (the bake cache's
  key).
- `lz4_flex` for the baked caches.

### 4. Performance
- `tracing` with JSON logs replacing `log`, and `tracing-tracy` behind a
  feature for timelines.
- `divan` benchmarks for the frame's hot functions (collision queries,
  the AI tick, animation blending, the bake).
- `mimalloc` as the global allocator.
- `bumpalo` frame arenas, `smallvec` and `arrayvec` where the hot paths
  allocate small vectors.
- `rtrb` for the queue to the audio thread, and `rubato` for resampling
  the 22 kHz sounds.
- `rayon` for bake and import.
- `meshopt` at bake time.
- `lto = "fat"` and `codegen-units = 1` for release, then `cargo-pgo`
  profiles recorded on devgulch.
- `cargo-show-asm` in the dev shell for checking hot loops.

### 5. Nix tooling
- `treefmt-nix` (rustfmt, swift-format, nixfmt, ruff) behind `nix fmt`.
- `git-hooks.nix` running the formatter and clippy before a commit.
- `nix-direnv` with an `.envrc`, and `nix-output-monitor` in the shell.
- The crane checks in `nix flake check`: fmt, clippy, nextest, deny and
  llvm-cov.
- This phase starts only once the crane packages (another session's work)
  build.

## Non-functional requirements
- Release frame time on devgulch, measured with the dev panel's timings,
  is not worse after any phase, and better after Phase 4.
- Debug build and test times are measured before Phase 1 and after it.
- The reference traces match after every phase.

## Acceptance criteria
- Every crate above is either adopted with a measured reason or recorded
  in the plan as declined, with the measurement that declined it.
- `cargo deny check`, the mutation report, the coverage floor and the
  fuzz targets run from the test step without manual steps.
- `nix flake check` runs the checks.
- A report at the end of the track compares build times, test times,
  frame times and load times before and after.

## Out of scope
- The UI crates (`taffy`, `lyon`, `fdsm`, `ttf-parser`, `rustybuzz`):
  they are adopted with the menus' Phase 2 (`menu_redesign_20261024`).
- Replacing our own physics, netcode or the Xbox ADPCM decoder.
- `str0m` (only if internet play returns), `parry3d` (only as a test
  oracle, when needed), `bitcode` or `postcard` (when snapshots need a
  second format).
- MetalFX (a renderer track).
- flake-parts and fenix (std and rust-overlay stay).
