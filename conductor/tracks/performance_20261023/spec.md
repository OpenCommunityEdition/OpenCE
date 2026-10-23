# Spec: Smooth play on every Apple Silicon Mac

## Overview
The lighting track left the benchmark above its bar. Bloodgulch at 3840×2160 has a p99 of 14.19 ms on the M1 Max against a target of 8.33 ms, and MetalFX at full scale alone takes 6.4 ms. Nothing yet measures the campaign's busy scenes, frame pacing or hitches, and every Mac gets the same settings. This track makes play feel smooth on every Apple Silicon Mac from the base M1 to the M4 Max. It measures first, fixes the lighting track's open problems, removes hitches, and picks quality tiers from the hardware.

## Functional Requirements

### Measuring
1. **GPU and CPU timings:**
   - each render pass has GPU timestamps (Metal counter sample buffers);
   - the simulation, the scripts, the AI, the snapshot and the encoding each have a CPU span;
   - the shell's `perf` overlay shows them live, and `perf.log` writes them to a file.
2. **Hitches** are logged with their cause: any frame over twice the target. The causes are shader compiles, texture uploads, BSP switches, saves, the bake, and allocation.
3. **The benchmark covers the campaign:** it plays recorded camera and input paths through bloodgulch, a10's crash site, b30's beach, the c10 Flood reveal, d40's Warthog run and a 16-player game. It reports the mean, the p99, the p99.9, hitches and frame-pacing variance per path.

### Quality tiers
4. **Tiers** (Low, Medium, High, Ultra) are picked from the GPU's family and core count on first run. Each pass declares its setting at each tier. The settings menu shows the tier, and any single setting can be overridden.

   | Mac | Target |
   | --- | --- |
   | M1 / M2 / M3 (8–10-core GPU) | 1440p at 60 fps |
   | M1 Pro / M2 Pro / M3 Pro | 1440p at 120 fps, or 4K at 60 fps |
   | M1 Max and above | 4K at 120 fps |

5. **Render scale with MetalFX temporal upscaling** is the default at 4K (internal resolution about 0.67), with native scale as a setting. A dynamic resolution mode can hold the target frame rate.

### Frame pacing and latency
6. **Frames are presented in step with the display** (CAMetalDisplayLink): ProMotion and adaptive sync where the display has them, a frame cap and a V-Sync setting, and a bounded number of frames in flight, with no waits between the CPU and the GPU.
7. **The renderer interpolates between the simulation's 60 Hz ticks**, so motion is smooth at 120 Hz and at any refresh rate. The camera uses the latest input, not the latest tick.
8. **Input-to-photon latency** is measured by the shell and kept under two frames plus one tick.

### Removing hitches
9. **No pipeline is compiled during play:** every pipeline state is built at import or first launch and kept in Metal binary archives.
10. **Textures** stream on a background queue with mip residency. BSP switches, checkpoints, saves and the background bake never hitch a frame. The bake runs at a low priority and yields to the frame.
11. **The frame allocates nothing on the heap** in Rust or Swift. A test checks this with a counting allocator.

### GPU and CPU work
12. **Culling:** the BSP's clusters and PVS, then a Hi-Z occlusion pass on the GPU, then indirect draws through argument buffers. Draws with the same material are merged.
13. **Apple's tile memory:** memoryless attachments, and tile shading or imageblocks where passes can share tile memory, such as lighting and the transparent resolve. Half-precision is used where the result is unchanged.
14. **Lower resolution:** screen effects run at half resolution where the golden frames do not change beyond tolerance, such as AO, SSR and volumetrics.
15. **The CPU side:** independent ECS systems run in parallel within the tick, keeping determinism. The snapshot to Swift stays copy-free.

### Lighting and video fixes
16. **The lighting track's problems are fixed:** the problems its deferred manual verifications find, and those this track's measuring finds, such as light leaks, probe seams, shadow acne and swimming, exposure flicker, TAA ghosting and the fallback to stock lightmaps.
17. **A full suite of graphics settings**, as modern games have, in the launcher's Settings:
    - **Presets:** Low, Medium, High, Ultra and Custom. Changing any one setting switches to Custom.
    - **Information on each setting:** a description, a preview picture where it shows a difference, its cost in GPU time measured on this Mac, and a video-memory meter for the whole set.
    - **Applying changes:** they take effect without a restart wherever Metal allows it. A built-in benchmark button runs one path and reports the result.
18. **Display:**
    - display and display mode (windowed, borderless, exclusive full screen);
    - resolution, refresh rate, V-Sync, frame cap, low-latency mode;
    - HDR on or off, peak brightness, paper white, and brightness/gamma calibration screens.
19. **Resolution and anti-aliasing:**
    - render scale;
    - the upscaler: MetalFX temporal or spatial, or native;
    - dynamic resolution with a target frame rate;
    - the anti-aliasing mode (TAA, SMAA, off) and sharpening;
    - MetalFX frame interpolation as an option, off by default, with its added latency shown.
20. **Quality settings**, each with Off and Low through Ultra where that makes sense:
    - textures and anisotropic filtering;
    - level of detail and draw distance;
    - detail objects (grass) density;
    - shadows: resolution, cascades, distance and soft shadows; contact shadows;
    - ambient occlusion (off, GTAO, ray-traced on M3 and later);
    - reflections (off, SSR, ray-traced);
    - dynamic light count and light shadows;
    - lightmap and bake quality;
    - volumetric fog, clouds, water, particles, decals (count and lifetime), weather effects.

    The atmosphere and living-world tracks add their passes to this list as they arrive.
21. **Post-processing:**
    - bloom and lens flares;
    - motion blur (off by default);
    - depth of field in cutscenes (off by default);
    - film grain, chromatic aberration and vignette (off by default);
    - exposure bias.
22. **Camera and accessibility:**
    - the field of view and the first-person weapon's field of view;
    - head bob in the stock amount or reduced;
    - colour-blind filters;
    - the HUD's scale.
23. **The performance overlay** has levels: off, FPS, frame times, and full per-pass timings.

## Non-Functional Requirements
- The simulation's results are unchanged: the replay hash test still passes.
- Memory fits an 8 GB base M1 at the Low tier with every stock map.
- A benchmark regression of more than 5% on any path fails the performance test.

## Acceptance Criteria
- On the M1 Max, every benchmark path meets 4K/120 at the Ultra tier with upscaling: p99 at most 8.33 ms and no hitches.
- The Low and Medium tiers meet their targets on the lower Macs available for manual checks. Until then they are checked by their GPU time on the M1 Max, scaled by the GPU core counts.
- None of the benchmark paths compiles a pipeline during play.
- Every setting changes what it says, and its measured cost is shown (a test switches each one and checks the frame changes).
- A manual check that play feels smooth in a10, b30 and a 16-player bloodgulch game.

## Out of Scope
- Intel Macs.
- New rendering features; those belong to the atmosphere and living-world tracks, which must fit within these budgets and add their settings here.
