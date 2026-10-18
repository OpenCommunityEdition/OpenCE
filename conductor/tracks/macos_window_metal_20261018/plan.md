# Plan: The game in a macOS window: a Swift app, a Metal renderer, native input and audio

## Phase 1: The Swift app runs the VM host
- [x] Task: Split the C host into a library and its command-line front end 45cee3fa
    - [x] Write tests: the build graph has `libhalo_host.a` and `halo_host` links it (tools/test_macos_build.py); the headless runs stay green
    - [x] Move host_main.c's start-up into `halo_host_run(image, options)`, keeping `halo_host` as a thin main
- [x] Task: Build OpenCE.app with Nix (`nix build .#app`) 71b7cb62
    - [x] Write tests: pytest checks the bundle layout, Info.plist, the embedded guest image and the hypervisor entitlement in the signature
    - [x] A Swift app target (SwiftPM or xcodebuild under Nix, settled here; tech-stack.md updated first if it differs), signed ad hoc with sigtool
- [x] Task: UniFFI bindings from the Rust runtime to Swift ee85ebca
    - [x] Write tests: cargo test for the exported API; a Swift test that maps a guest range through the bindings
    - [x] Export what Swift needs (the guest memory view and its range checks)
- [x] Task: Run main-thread imports on the main thread 988639b1
    - [x] Write tests: a guest program (port/macos/tests) whose thread asks for a main-thread import and gets its result; a request during shutdown fails cleanly
    - [x] A request queue: the vCPU posts and waits, the AppKit run loop carries it out
- [x] Task: The data root, and a dialog for missing maps d7b772b6
    - [x] Write tests: the root is HALO_DATA_ROOT, else Application Support/OpenCE; missing maps give the dialog's message
    - [x] Implement it in the Swift shell
- [ ] Task: Conductor - User Manual Verification 'Phase 1: The Swift app runs the VM host' (Protocol in workflow.md)

## Phase 2: Window, native input and audio
- [ ] Task: The window and its Metal layer
    - [ ] Write tests: host_sdl_create_window and host_sdl_window_size_in_pixels report the backing size, and a resize is reported (XCTest with a window off-screen)
    - [ ] An AppKit window with a CAMetalLayer at the backing scale; live resize; native fullscreen from the menu bar and a key; a cleared frame each present (manual check: window lifecycle)
- [ ] Task: Keyboard and mouse from NSEvent
    - [ ] Write tests: key codes map to SDL scancodes (table test); mouse buttons, wheel and relative motion become SDL-shaped events; focus loss releases the capture
    - [ ] Implement the event queue behind host_sdl_poll_event, host_sdl_set_relative_mouse and the scancode imports
- [ ] Task: Gamepads from GameController
    - [ ] Write tests: a fake controller's buttons and axes map to the gamepad imports; connect/disconnect events; rumble requests reach the haptics engine
    - [ ] Implement it with GCController, incl. hot-plug and rumble
- [ ] Task: Sound with AVAudioEngine
    - [ ] Write tests: the audio-stream imports' format conversion and buffering (XCTest); vm_sdl's audio test plays through the new backend
    - [ ] Implement host_sdl_open_audio_stream and the rest with AVAudioEngine
- [ ] Task: Conductor - User Manual Verification 'Phase 2: Window, native input and audio' (Protocol in workflow.md)

## Phase 3: The command ring and the Metal backend
- [ ] Task: Define the ring's format
    - [ ] Write tests: cargo test decodes valid batches, and rejects truncated commands, out-of-range sizes, indices and guest offsets
    - [ ] A shared C header for the commands; the decoder and validator in Rust (port/macos/runtime), exported to Swift
- [ ] Task: The guest's encoder and the doorbell
    - [ ] Write tests: a guest program writes batches that the host decodes intact; the exits per batch are counted
    - [ ] The guest-side writer and the host_render_submit import
- [ ] Task: The Swift Metal backend
    - [ ] Write tests: XCTest submits decoded commands (buffers, textures, pipelines, draws) to an offscreen target and reads pixels back
    - [ ] Buffers and textures uploaded from guest memory; pipeline and depth-stencil state caches; presenting to the layer
- [ ] Task: Offscreen captures and golden-frame checks
    - [ ] Write tests: the compare tool's tolerance (pytest, synthetic images)
    - [ ] A capture setting (debug, off by default) writing a frame to a file; tools/test_macos_frames.py comparing to approved references
- [ ] Task: Conductor - User Manual Verification 'Phase 3: The command ring and the Metal backend' (Protocol in workflow.md)

## Phase 4: Direct3D 8 on Metal
- [ ] Task: NV2A shaders to Metal Shading Language
    - [ ] Write tests: harness tests of nv2a_vsh.c / nv2a_psh.c's MSL output for known programs, and that each compiles (newLibraryWithSource in a test tool)
    - [ ] An MSL emitter beside the GLSL one; in-memory and on-disk shader caches
- [ ] Task: Resources and Xbox texture formats
    - [ ] Write tests: each format xbox_textures.c handles maps to a Metal pixel format or conversion, checked by pixel readback
    - [ ] Vertex/index buffers, textures, surfaces in d3d8_metal.c
- [ ] Task: Render states, draws and render targets
    - [ ] Write tests: harness tests of state → pipeline-key packing; a draw test through the ring
    - [ ] The rest of D3D8 as d3d8_gl.c implements it, encoded into the ring
- [ ] Task: The main menu draws
    - [ ] Write tests: the main-menu golden frame (approved by you once)
    - [ ] Fix what the frame shows wrong
- [ ] Task: Bloodgulch draws
    - [ ] Write tests: a bloodgulch golden frame from a fixed camera (approved by you once)
    - [ ] Fix what the frame shows wrong
- [ ] Task: Conductor - User Manual Verification 'Phase 4: Direct3D 8 on Metal' (Protocol in workflow.md)

## Phase 5: Parity with the GL build's extras
- [ ] Task: High-res HUD and text
    - [ ] Write tests: golden frames with the HUD and text at native resolution
    - [ ] hud_hires.c and text_hires.c through the Metal path
- [ ] Task: Post-processing, incl. SMAA
    - [ ] Write tests: golden frames with each post pass on; stock frames unchanged with them at defaults
    - [ ] xgpu_post.c's passes in MSL, same settings and defaults
- [ ] Task: The frame-time overlay
    - [ ] Write tests: the setting exists, off by default, and documented in the settings table
    - [ ] Draw frame time and fps when on
- [ ] Task: Conductor - User Manual Verification 'Phase 5: Parity with the GL build's extras' (Protocol in workflow.md)

## Phase 6: 4K at 120fps
- [ ] Task: Answer the hottest imports in the guest
    - [ ] Write tests: ticks from the guest's counter (CNTVCT_EL0) agree with the host's; errno kept guest-side; exits/s reported lower than the 500k baseline
    - [ ] Move host_sdl_ticks and host_errno into the guest runtime
- [ ] Task: A repeatable benchmark
    - [ ] Write tests: a pytest run of bloodgulch for 60 s at 3840×2160 logging per-frame times and VM exits per frame, reporting p99
    - [ ] The frame-time log and the run
- [ ] Task: Optimise until the target is met
    - [ ] Profile with Metal frame capture and Instruments; fix the largest costs one by one, each measured
    - [ ] Record the result (p99 ≤ 8.33 ms) in docs/macos_host.md
- [ ] Task: Documentation
    - [ ] docs/macos_host.md (app, renderer, ring, input, audio), the settings table, tech-stack.md
- [ ] Task: Conductor - User Manual Verification 'Phase 6: 4K at 120fps' (Protocol in workflow.md)
