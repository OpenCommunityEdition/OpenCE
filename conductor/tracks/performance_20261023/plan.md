# Plan: Smooth play on every Apple Silicon Mac

## Phase 1: Measuring
- [ ] Task: GPU and CPU timings
    - [ ] Write tests: every pass reports a GPU time; the CPU spans nest and sum to the frame; `perf.log` round-trips
    - [ ] Counter sample buffers per pass, CPU spans, the shell's `perf` overlay with its levels
- [ ] Task: The hitch log
    - [ ] Write tests: a forced pipeline compile, upload and allocation are each logged with their cause
    - [ ] Frame-over-budget detection and cause tagging
- [ ] Task: The benchmark paths
    - [ ] Write tests: a recorded path replays the same camera and inputs; the report has mean, p99, p99.9, hitches and pacing variance; a regression over 5% fails
    - [ ] Record the paths (bloodgulch, a10, b30, c10, d40, 16 players); the baseline report in `docs/engine.md`
- [ ] Task: Conductor - User Manual Verification 'Phase 1: Measuring' (Protocol in workflow.md) (deferred to the end)

## Phase 2: Frame pacing and hitches
- [ ] Task: Presenting with the display
    - [ ] Write tests: the frames in flight stay bounded; no CPU wait on the GPU in a frame; the frame cap holds
    - [ ] CAMetalDisplayLink, ProMotion and adaptive sync, V-Sync and the frame cap
- [ ] Task: Interpolating between ticks
    - [ ] Write tests: positions drawn between two ticks interpolate; the camera takes the latest input; the replay hash is unchanged
    - [ ] Render-side interpolation of the snapshot; input-to-photon latency measured
- [ ] Task: No compiles, uploads or allocations in play
    - [ ] Write tests: no pipeline is built after load on any path; a counting allocator sees no heap allocation in a frame; a BSP switch and a save stay under budget
    - [ ] Metal binary archives built at import; background texture streaming with mip residency; the bake at low priority
- [ ] Task: Conductor - User Manual Verification 'Phase 2: Frame pacing and hitches' (Protocol in workflow.md) (deferred to the end)

## Phase 3: GPU and CPU work
- [ ] Task: Culling and indirect draws
    - [ ] Write tests: the drawn set matches a brute-force visibility test; the draw count falls on each path
    - [ ] Cluster and PVS culling, Hi-Z occlusion on the GPU, argument buffers and merged draws
- [ ] Task: Tile memory and precision
    - [ ] Write tests: the golden frames stay within tolerance; the memoryless attachments allocate no memory
    - [ ] Memoryless attachments, tile shading and imageblocks, half precision
- [ ] Task: Upscaling and half-resolution effects
    - [ ] Write tests: the golden frames at scale 0.67 and with half-resolution AO, SSR and volumetrics stay within tolerance; dynamic resolution holds the target
    - [ ] MetalFX temporal and spatial at a render scale; dynamic resolution
- [ ] Task: The simulation in parallel
    - [ ] Write tests: the replay hash is unchanged with parallel systems; the tick's time falls
    - [ ] The ECS schedule run in parallel
- [ ] Task: Conductor - User Manual Verification 'Phase 3: GPU and CPU work' (Protocol in workflow.md) (deferred to the end)

## Phase 4: The lighting track's problems
- [ ] Task: Collect the problems
    - [ ] Run the lighting track's deferred manual checks and the benchmark paths, and list each problem in `docs/engine.md`
- [ ] Task: Fix them
    - [ ] Write tests: a regression test or golden frame for each problem (light leaks, probe seams, shadow acne and swimming, exposure flicker, TAA ghosting, the stock-lightmap fallback)
    - [ ] The fixes
- [ ] Task: Conductor - User Manual Verification 'Phase 4: The lighting track's problems' (Protocol in workflow.md) (deferred to the end)

## Phase 5: Tiers and graphics settings
- [ ] Task: Quality tiers
    - [ ] Write tests: each GPU family and core count picks its tier; each pass declares a setting per tier; an 8 GB budget fits at Low
    - [ ] The tier table and its detection
- [ ] Task: The settings model
    - [ ] Write tests: every setting of the display, resolution, quality, post-processing, camera and accessibility groups exists with its default; presets set them; changing one gives Custom; switching each changes the frame
    - [ ] The settings in Rust and Swift, applied live where Metal allows
- [ ] Task: The settings screens
    - [ ] Write tests (XCTest): the screens list every setting with its description and measured cost; the video-memory meter; the benchmark button; the calibration screens
    - [ ] The SwiftUI screens, the preview pictures, the colour-blind filters, MetalFX frame interpolation (off by default)
- [ ] Task: Meeting the targets
    - [ ] Write tests: each path at Ultra on the M1 Max meets 8.33 ms p99; the Low and Medium estimates meet theirs
    - [ ] The last fixes; `docs/engine.md`: performance, the tiers and the settings
- [ ] Task: Conductor - User Manual Verification 'Phase 5: Tiers and graphics settings' (Protocol in workflow.md) (deferred to the end)
