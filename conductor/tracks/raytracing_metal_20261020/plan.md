# Plan: Ray-traced sun shadows and ambient occlusion with Metal

## Phase 1: The spike (go / no-go)
- [ ] Task: The level's geometry in an acceleration structure
    - [ ] Write tests: the ring's BSP geometry command decodes and rejects bad sizes and indices (cargo test); XCTest builds a structure from a known mesh and checks hits and misses
    - [ ] The guest sends the structure BSP's render triangles when a BSP loads or switches; the Swift backend builds a primitive acceleration structure
- [ ] Task: A half-resolution sun shadow pass
    - [ ] Write tests: XCTest traces a synthetic scene (a plane and a box, known sun) and checks the mask's shadowed pixels
    - [ ] The camera's matrices and the sun's direction through the ring; a compute pass reconstructing positions from depth and tracing toward the sun; a debug-only multiply into the frame
- [ ] Task: Measure and decide
    - [ ] Write tests: the benchmark script reports the build time and the trace pass's GPU time
    - [ ] Measure bloodgulch and a campaign level at 3840×2160 on the M1 Max; check whether macOS 26's MetalFX has a suitable denoiser; record the numbers and the go / no-go in `docs/macos_host.md`
- [ ] Task: Conductor - User Manual Verification 'Phase 1: The spike (go / no-go)' (Protocol in workflow.md)

## Phase 2: Objects in the scene
- [ ] Task: Models as instances
    - [ ] Write tests: XCTest of the model cache (one primitive structure per model, reused) and of an instance structure refitted when transforms change
    - [ ] Each drawn object's model and transform through the ring each frame; an instance acceleration structure refitted each frame
- [ ] Task: Skinned models
    - [ ] Write tests: a posed biped's structure (or its bone capsules) follows the pose (XCTest)
    - [ ] Refitted posed vertices or bone capsules, whichever the spike measured cheaper
- [ ] Task: Alpha-tested surfaces
    - [ ] Write tests: a ray through a texture's transparent texels passes; through opaque ones it hits (XCTest)
    - [ ] An intersection function reading the texture's alpha
- [ ] Task: Conductor - User Manual Verification 'Phase 2: Objects in the scene' (Protocol in workflow.md)

## Phase 3: Shadows, AO and denoising
- [ ] Task: The settings
    - [ ] Write tests: `video.ray_traced_shadows`, `video.ray_traced_ao`, `video.ambient_occlusion` and `debug.raytrace_view` parse from config and environment, off by default; the settings screen lists them
    - [ ] Entries in `port_config.c`, the settings table in `port/linux/README.md`, `tools/port_settings.py` run again
- [ ] Task: The shadow mask in the lighting
    - [ ] Write tests: golden frame of a Warthog's shadow on bloodgulch at noon; the existing goldens unchanged with the settings off
    - [ ] Apply the mask to the sun's share of the lighting before transparent geometry and the HUD
- [ ] Task: Ray-traced AO
    - [ ] Write tests: golden frame of AO in a campaign corridor
    - [ ] Short-range rays at half resolution, accumulated over frames
- [ ] Task: Denoising
    - [ ] Write tests: the filter converges on a static synthetic scene and rejects history across a camera cut; output repeats with the frame clock
    - [ ] A depth-aware upscale and a spatial and temporal filter with motion vectors (or MetalFX's denoiser if the spike chose it)
- [ ] Task: Screen-space AO
    - [ ] Write tests: golden frame with `video.ambient_occlusion = "screen-space"`
    - [ ] A screen-space AO pass from the depth buffer
- [ ] Task: Conductor - User Manual Verification 'Phase 3: Shadows, AO and denoising' (Protocol in workflow.md)

## Phase 4: Performance and documentation
- [ ] Task: The benchmark with every effect on
    - [ ] Write tests: the benchmark runs with every effect on and reports p99 and each pass's GPU time
    - [ ] Meet p99 ≤ 8.33 ms at 3840×2160 on the M1 Max, or add the half- and quarter-resolution modes that do; record the results in `docs/macos_host.md`
- [ ] Task: `debug.raytrace_view` and documentation
    - [ ] Write tests: each view draws only with the setting on
    - [ ] Describe the ray tracing passes in `docs/macos_host.md`
- [ ] Task: Conductor - User Manual Verification 'Phase 4: Performance and documentation' (Protocol in workflow.md)
