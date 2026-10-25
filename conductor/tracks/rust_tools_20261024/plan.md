# Plan: Rust crates and tools for the engine

Every crate passes `cargo deny check`; every phase ends with the reference
traces matching (`sim/tests/*_reference.rs`) and the timings measured
against the start. The phase checks are automated; the user reads the
report at the end.

## Phase 1: Quick wins [checkpoint: 8812cd52]
In the order of the user's goal (2026-10-25): caches ready, debug builds and tests faster, then the rest.
- [x] Task: Measure the starting point 9ca9137
    - [x] Debug and release build times, the test suite's time, devgulch's release frame time and load time, written to the track's report
- [x] Task: Ready caches for the tests acdd596
    - [x] Write tests: the shared loader gives the same level as `import_level`; a second load in one process reads no file; a pack of another importer version is imported again
    - [x] One test loader for the user's maps, through `load_level` and the shared cache folder (`build/engine/cache`, or `OPENCE_CACHE`), held once per process; the game-data suites use it instead of importing the map each (on 2026-10-25, 33 suites imported Blood Gulch from the map); the importer's own tests still import
- [x] Task: Optimised game-data tests 095a54d
    - [x] Write tests: the test step builds with an optimised test profile that keeps debug assertions and overflow checks; the reference traces match under it
    - [x] A `[profile.test-opt]` (inherits release, `debug-assertions` and `overflow-checks` on, incremental) used by nextest for the workspace run; time each suite before and after (on 2026-10-25 the debug run took about 40 minutes, the slowest suites 984 s, 739 s and four of 200–275 s, all stepping the sim on Blood Gulch)
- [x] Task: A faster debug loop 74539a1a
    - [x] Write tests: the dev profile's opt-level for dependencies is set; sccache judged (left out: no incremental caching)
    - [x] `[profile.dev.package."*"] opt-level = 2`, sccache; measure again
- [x] Task: cargo-nextest and cargo-deny in the test step 7b4d5a3
    - [x] Write tests: `tools/engine_build.py` runs nextest and `cargo deny check`; a forbidden licence fails the step
    - [x] `deny.toml` for the workspace; nextest's profile; the build script's test step
- [x] Task: Deterministic maps eb85df8
    - [x] Write tests: one sim run twice in one process with differently seeded hashers gives the same state each tick (the contrails, run four times: the one map iterated with the game's random numbers)
    - [x] `indexmap` where the sim iterates a map, `rustc-hash` for the hot lookups; clippy's `disallowed_types` forbids std `HashMap` in `sim` (the original's slot order in place of `indexmap`; the ban covers the workspace)
- [x] Task: insta and pretty_assertions 7761dd1
    - [x] Write tests: snapshots of the dev state, the bake report, the import's tag dumps and the shell's command output (the shell's text and the tag index; the dev state and the bake report wait for another session's changes to their code)
    - [x] Move the hand-written expected texts to snapshots
- [x] Task: Conductor - User Manual Verification 'Phase 1: Quick wins' (Protocol in workflow.md) (superseded: automated verification only, at the user's instruction)

## Phase 2: Test strength [checkpoint: 51c7ff1b]
- [x] Task: Mutation testing 8847ba03
    - [x] Run `cargo mutants` over `sim`, `physics` and `script`; list the survivors
    - [x] Write tests that kill the survivors that matter; record the rest with a reason
- [x] Task: Property tests 2fb6670e
    - [x] Write tests (proptest): the tag and cache readers never panic and round-trip; the script compiler rejects or compiles; physics keeps its invariants (no tunnelling at the tested speeds, energy not gained)
- [x] Task: Coverage floors 40d4f953
    - [x] Write tests: the check fails below each crate's floor (its own check at each phase's end, not every test step: instrumented, the tests take half as long again)
    - [x] `cargo llvm-cov` per crate, the floors set from today's numbers
- [x] Task: Fuzz targets for every untrusted reader 5bd8f372
    - [x] Write tests: each target runs a short corpus in the test step
    - [x] Targets for the map, tag, sound and bitmap readers
- [x] Task: Golden frames 79b59225
    - [x] Write tests (XCTest, the tests' own capture and difference rather than swift-snapshot-testing): devgulch's stations rendered headless match their golden images within a tolerance
    - [x] The headless capture and the recorded images (made from the user's map, kept out of the repository)
- [x] Task: Conductor - User Manual Verification 'Phase 2: Test strength' (Protocol in workflow.md) (superseded: automated verification only, at the user's instruction)

## Phase 3: Formats [checkpoint: 11778c82]
- [x] Task: binrw for the tag and cache layouts cfb6a372
    - [x] Write tests: each converted reader gives the same values as the hand-written one over the user's maps (skipped without them) (declined: load_timing measures open, and the inflate dominates; see the report)
    - [x] Convert the readers one layout at a time; zerocopy for the plain ones (declined: the readers stay hand-written, bounds-checked and fuzzed)
- [x] Task: memmap2 and bytemuck a7aea52e
    - [x] Write tests: maps read through a mapping match a read file; the Metal buffer structures are `Pod` (Pod only: memmap2 declined, see the report)
    - [x] Map files mapped, the buffer copies through bytemuck (bytemuck's checks and its zeroed slot; reading the map stays fs::read)
- [x] Task: blake3 and lz4_flex for the bake cache 35faf08d
    - [x] Write tests: a changed input invalidates its baked asset and only it; compressed caches round-trip; load time measured (the level packs, whose key the bakes share)
    - [x] The cache key and the compression
- [x] Task: Conductor - User Manual Verification 'Phase 3: Formats' (Protocol in workflow.md) (superseded: automated verification only, at the user's instruction)

## Phase 4: Performance [checkpoint: 3046fac1]
- [x] Task: tracing and benchmarks d2a62812
    - [x] Write tests: spans nest and the JSON log parses; the benchmarks run (dev/tests/tracing.rs; `cargo test --bench` runs each once)
    - [x] `tracing` in place of `log`, `tracing-tracy` behind a feature, `divan` benchmarks for collision, the AI tick, animation blending and the bake (the log book is a layer; `OPENCE_LOG_JSON` adds the JSON log; the blend and AI benches need assets/maps)
- [x] Task: Allocation in the frame 1269e227
    - [x] Write tests: a counting allocator shows how many allocations a frame makes on devgulch, and the count goes down
    - [x] mimalloc; bumpalo frame arenas; smallvec and arrayvec in the hot paths (mimalloc adopted; the rest declined, see the report)
- [x] Task: The audio thread 0364e2b7
    - [x] Write tests: the audio thread's queue never blocks; resampled 22 kHz sounds match a reference within a tolerance
    - [x] rtrb (declined: one thread), rubato
- [x] Task: Bake and import in parallel ab55e7c1
    - [x] Write tests: the bake's output is the same with one thread and with many
    - [x] rayon in bake and import; meshopt at bake time (rayon in the bake's firefly hold and exposure fit, BLAKE3 hashing on all cores; the import kept serial and meshopt declined, see the report)
- [x] Task: The release build 63e99d02
    - [x] Write tests: the release profile has fat LTO and one codegen unit; the PGO build gives the same reference traces (the profile test holds the measured decision, abort and no LTO; the PGO build's traces are identical)
    - [x] LTO, codegen-units, cargo-pgo profiles recorded on devgulch; cargo-show-asm in the shell (LTO and PGO measured slower and declined; cargo-show-asm deferred to the Nix restructure, see the report)
- [x] Task: Conductor - User Manual Verification 'Phase 4: Performance' (Protocol in workflow.md) (superseded: automated verification only, at the user's instruction)

## Phase 5: Nix tooling (after the crane packages build) [checkpoint: 20ebe3ed]
- [x] Task: Formatting and hooks c1778fd5
    - [x] Write tests: `nix fmt` formats Rust, Swift, Nix and Python; the hook refuses an unformatted commit
    - [x] treefmt-nix, git-hooks.nix (the tree formatted once in 41e8fdba)
- [x] Task: The shell e6395390
    - [x] nix-direnv's `.envrc`, nix-output-monitor in the shell
- [x] Task: The checks c52732aa
    - [x] Write tests: `nix flake check` runs fmt, clippy, nextest, deny and llvm-cov and fails on each (the tests and coverage run as cargo test in the test-opt profile; nextest stays the suite's runner)
    - [x] The crane checks
- [x] Task: The report 9e1fc515
    - [x] Build, test, frame and load times before and after; crates adopted and declined, each with its measurement
- [x] Task: Conductor - User Manual Verification 'Phase 5: Nix tooling' (Protocol in workflow.md) (superseded: automated verification only, at the user's instruction)
