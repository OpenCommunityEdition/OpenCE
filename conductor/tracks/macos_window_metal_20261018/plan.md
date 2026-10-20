# Plan: The game in a macOS window: a Swift app, a Metal renderer, native input and audio

## Phase 1: The Swift app runs the VM host [checkpoint: 0605ccac]
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
- [x] Task: Conductor - User Manual Verification 'Phase 1: The Swift app runs the VM host' (Protocol in workflow.md)

## Phase 2: Window, native input and audio [checkpoint: 9ab0a0c4]
- [x] Task: The window and its Metal layer 01316243
    - [x] Write tests: host_sdl_create_window and host_sdl_window_size_in_pixels report the backing size, and a resize is reported (XCTest with a window off-screen)
    - [x] An AppKit window with a CAMetalLayer at the backing scale; live resize; native fullscreen from the menu bar and a key; a cleared frame each present (manual check: window lifecycle)
- [x] Task: Keyboard and mouse from NSEvent 9d85891f
    - [x] Write tests: key codes map to SDL scancodes (table test); mouse buttons, wheel and relative motion become SDL-shaped events; focus loss releases the capture
    - [x] Implement the event queue behind host_sdl_poll_event, host_sdl_set_relative_mouse and the scancode imports
- [x] Task: Leave gamepads out (keyboard and mouse only) 7f20b49b
    - [x] Write tests: with the app's windows, SDL's joystick and gamepad events are left out of the guest's polls; without them they pass
    - [x] Skip SDL's events 0x600 to 0x6ff in host_input_poll_event (GameController support was dropped at the user's request)
- [x] Task: Sound with AVAudioEngine 84e34cda
    - [x] Write tests: the audio-stream imports' format conversion, buffering and callbacks (host_audio_test.c, native); the engine's source node renders what the host gives (XCTest, offline rendering); vm_sdl's audio test still plays through SDL in the command-line host
    - [x] Implement host_sdl_open_audio_stream and the rest with AVAudioEngine
- [x] Task: Conductor - User Manual Verification 'Phase 2: Window, native input and audio' (Protocol in workflow.md)

## Phase 3: The command ring and the Metal backend [checkpoint: 60cb70b3]
- [x] Task: Define the ring's format 56e08e51
    - [x] Write tests: cargo test decodes valid batches, and rejects truncated commands, out-of-range sizes, indices and guest offsets
    - [x] A shared C header for the commands; the decoder and validator in Rust (port/macos/runtime), exported to Swift
- [x] Task: The guest's encoder and the doorbell 48fa14c6
    - [x] Write tests: a guest program writes batches that the host decodes intact; the exits per batch are counted
    - [x] The guest-side writer and the host_render_submit import
- [x] Task: The Swift Metal backend 15c579bc
    - [x] Write tests: XCTest submits decoded commands (buffers, textures, pipelines, draws) to an offscreen target and reads pixels back
    - [x] Buffers and textures uploaded from guest memory; pipeline and depth-stencil state caches; presenting to the layer
- [x] Task: Offscreen captures and golden-frame checks 1cc1db50
    - [x] Write tests: the compare tool's tolerance (pytest, synthetic images)
    - [x] A capture setting (debug, off by default) writing a frame to a file; tools/test_macos_frames.py comparing to approved references
- [x] Task: Conductor - User Manual Verification 'Phase 3: The command ring and the Metal backend' (Protocol in workflow.md)

## Phase 4: Direct3D 8 on Metal [checkpoint: 987e8e9d]
- [x] Task: NV2A shaders to Metal Shading Language 5de109a4
    - [x] Write tests: harness tests of nv2a_vsh.c / nv2a_psh.c's MSL output for known programs, and that each compiles (newLibraryWithSource in a test tool)
    - [x] An MSL emitter beside the GLSL one; an in-memory library cache by source (Metal's own shader cache keeps them on disk)
- [x] Task: Resources and Xbox texture formats 992de7bd
    - [x] Write tests: each format xbox_textures.c handles maps to a Metal pixel format or conversion, checked by pixel readback
    - [x] Vertex/index buffers, textures, surfaces in d3d8_metal.c
- [x] Task: Render states, draws and render targets 26404ecb
    - [x] Write tests: harness tests of state → pipeline-key packing; a draw test through the ring
    - [x] The rest of D3D8 as d3d8_gl.c implements it, encoded into the ring
- [x] Task: The main menu draws 6b1e9ab9
    - [x] Write tests: the main-menu golden frame (approved by you once)
    - [x] Fix what the frame shows wrong
- [x] Task: Bloodgulch draws 9ca74bf6
    - [x] Write tests: a bloodgulch golden frame from a fixed camera (approved by you once)
    - [x] Fix what the frame shows wrong
- [x] Task: Conductor - User Manual Verification 'Phase 4: Direct3D 8 on Metal' (Protocol in workflow.md)

## Phase 5: Parity with the GL build's extras [checkpoint: 617c6341]
- [x] Task: High-res HUD and text 33fe5b0b
    - [x] Write tests: golden frames with the HUD and text at native resolution
    - [x] hud_hires.c and text_hires.c through the Metal path
- [x] Task: Post-processing, incl. SMAA 3447cec4
    - [x] Write tests: golden frames with each post pass on; stock frames unchanged with them at defaults
    - [x] xgpu_post.c's passes in MSL, same settings and defaults
- [x] Task: The frame-time overlay 69de6c45
    - [x] Write tests: the setting exists, off by default, and documented in the settings table
    - [x] Draw frame time and fps when on
- [x] Task: Conductor - User Manual Verification 'Phase 5: Parity with the GL build's extras' (Protocol in workflow.md)

## Phase 6: 4K at 120fps [checkpoint: ef38b20e]
- [x] Task: Answer the hottest imports in the guest 7e8d0dac
    - [x] Write tests: ticks from the guest's counter (CNTVCT_EL0) agree with the host's; errno kept guest-side; exits/s reported lower than the 500k baseline
    - [x] Move host_sdl_ticks and host_errno into the guest runtime
- [x] Task: A repeatable benchmark 0a25b926
    - [x] Write tests: a pytest run of bloodgulch for 60 s at 3840×2160 logging per-frame times and VM exits per frame, reporting p99
    - [x] The frame-time log and the run
- [x] Task: Optimise until the target is met c0ff161a f4d0aa74
    - [x] Profile with Metal frame capture and Instruments; fix the largest costs one by one, each measured
    - [x] Record the result (p99 ≤ 8.33 ms) in docs/macos_host.md
- [x] Task: Documentation a779e96f
    - [x] docs/macos_host.md (app, renderer, ring, input, audio), the settings table, tech-stack.md
- [x] Task: Conductor - User Manual Verification 'Phase 6: 4K at 120fps' (Protocol in workflow.md)
