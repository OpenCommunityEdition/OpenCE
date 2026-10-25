# Plan: Rust crates and tools for the engine

Every crate passes `cargo deny check`; every phase ends with the reference
traces matching (`sim/tests/*_reference.rs`) and the timings measured
against the start. The phase checks are automated; the user reads the
report at the end.

## Phase 1: Quick wins
In the order of the user's goal (2026-10-25): caches ready, debug builds and tests faster, then the rest.
- [x] Task: Measure the starting point 9ca9137
    - [x] Debug and release build times, the test suite's time, devgulch's release frame time and load time, written to the track's report
- [x] Task: Ready caches for the tests acdd596
    - [x] Write tests: the shared loader gives the same level as `import_level`; a second load in one process reads no file; a pack of another importer version is imported again
    - [x] One test loader for the user's maps, through `load_level` and the shared cache folder (`build/engine/cache`, or `OPENCE_CACHE`), held once per process; the game-data suites use it instead of importing the map each (on 2026-10-25, 33 suites imported Blood Gulch from the map); the importer's own tests still import
- [x] Task: Optimised game-data tests 095a54d
    - [x] Write tests: the test step builds with an optimised test profile that keeps debug assertions and overflow checks; the reference traces match under it
    - [x] A `[profile.test-opt]` (inherits release, `debug-assertions` and `overflow-checks` on, incremental) used by nextest for the workspace run; time each suite before and after (on 2026-10-25 the debug run took about 40 minutes, the slowest suites 984 s, 739 s and four of 200–275 s, all stepping the sim on Blood Gulch)
- [ ] Task: A faster debug loop
    - [ ] Write tests: the dev profile's opt-level for dependencies is set; sccache is in the shell
    - [ ] `[profile.dev.package."*"] opt-level = 2`, sccache; measure again
- [ ] Task: cargo-nextest and cargo-deny in the test step
    - [ ] Write tests: `tools/engine_build.py` runs nextest and `cargo deny check`; a forbidden licence fails the step
    - [ ] `deny.toml` for the workspace; nextest's profile; the build script's test step
- [ ] Task: Deterministic maps
    - [ ] Write tests: one sim run twice in one process with differently seeded hashers gives the same state each tick
    - [ ] `indexmap` where the sim iterates a map, `rustc-hash` for the hot lookups; clippy's `disallowed_types` forbids std `HashMap` in `sim`
- [ ] Task: insta and pretty_assertions
    - [ ] Write tests: snapshots of the dev state, the bake report, the import's tag dumps and the shell's command output
    - [ ] Move the hand-written expected texts to snapshots
- [ ] Task: Conductor - User Manual Verification 'Phase 1: Quick wins' (Protocol in workflow.md)

## Phase 2: Test strength
- [ ] Task: Mutation testing
    - [ ] Run `cargo mutants` over `sim`, `physics` and `script`; list the survivors
    - [ ] Write tests that kill the survivors that matter; record the rest with a reason
- [ ] Task: Property tests
    - [ ] Write tests (proptest): the tag and cache readers never panic and round-trip; the script compiler rejects or compiles; physics keeps its invariants (no tunnelling at the tested speeds, energy not gained)
- [ ] Task: Coverage floors
    - [ ] Write tests: the test step fails below each crate's floor
    - [ ] `cargo llvm-cov` per crate, the floors set from today's numbers
- [ ] Task: Fuzz targets for every untrusted reader
    - [ ] Write tests: each target runs a short corpus in the test step
    - [ ] Targets for the map, tag, sound and bitmap readers
- [ ] Task: Golden frames
    - [ ] Write tests (XCTest with swift-snapshot-testing): devgulch's stations rendered headless match their golden images within a tolerance
    - [ ] The headless capture and the recorded images (made from the user's map, kept out of the repository)
- [ ] Task: Conductor - User Manual Verification 'Phase 2: Test strength' (Protocol in workflow.md)

## Phase 3: Formats
- [ ] Task: binrw for the tag and cache layouts
    - [ ] Write tests: each converted reader gives the same values as the hand-written one over the user's maps (skipped without them)
    - [ ] Convert the readers one layout at a time; zerocopy for the plain ones
- [ ] Task: memmap2 and bytemuck
    - [ ] Write tests: maps read through a mapping match a read file; the Metal buffer structures are `Pod`
    - [ ] Map files mapped, the buffer copies through bytemuck
- [ ] Task: blake3 and lz4_flex for the bake cache
    - [ ] Write tests: a changed input invalidates its baked asset and only it; compressed caches round-trip; load time measured
    - [ ] The cache key and the compression
- [ ] Task: Conductor - User Manual Verification 'Phase 3: Formats' (Protocol in workflow.md)

## Phase 4: Performance
- [ ] Task: tracing and benchmarks
    - [ ] Write tests: spans nest and the JSON log parses; the benchmarks run
    - [ ] `tracing` in place of `log`, `tracing-tracy` behind a feature, `divan` benchmarks for collision, the AI tick, animation blending and the bake
- [ ] Task: Allocation in the frame
    - [ ] Write tests: a counting allocator shows how many allocations a frame makes on devgulch, and the count goes down
    - [ ] mimalloc; bumpalo frame arenas; smallvec and arrayvec in the hot paths
- [ ] Task: The audio thread
    - [ ] Write tests: the audio thread's queue never blocks; resampled 22 kHz sounds match a reference within a tolerance
    - [ ] rtrb, rubato
- [ ] Task: Bake and import in parallel
    - [ ] Write tests: the bake's output is the same with one thread and with many
    - [ ] rayon in bake and import; meshopt at bake time
- [ ] Task: The release build
    - [ ] Write tests: the release profile has fat LTO and one codegen unit; the PGO build gives the same reference traces
    - [ ] LTO, codegen-units, cargo-pgo profiles recorded on devgulch; cargo-show-asm in the shell
- [ ] Task: Conductor - User Manual Verification 'Phase 4: Performance' (Protocol in workflow.md)

## Phase 5: Nix tooling (after the crane packages build)
- [ ] Task: Formatting and hooks
    - [ ] Write tests: `nix fmt` formats Rust, Swift, Nix and Python; the hook refuses an unformatted commit
    - [ ] treefmt-nix, git-hooks.nix
- [ ] Task: The shell
    - [ ] nix-direnv's `.envrc`, nix-output-monitor in the shell
- [ ] Task: The checks
    - [ ] Write tests: `nix flake check` runs fmt, clippy, nextest, deny and llvm-cov and fails on each
    - [ ] The crane checks
- [ ] Task: The report
    - [ ] Build, test, frame and load times before and after; crates adopted and declined, each with its measurement
- [ ] Task: Conductor - User Manual Verification 'Phase 5: Nix tooling' (Protocol in workflow.md)
