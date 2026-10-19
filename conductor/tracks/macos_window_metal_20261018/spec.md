# Spec: The game in a macOS window: a Swift app, a Metal renderer, native input and audio

## Overview
The macOS host runs the game only headless today (debug.null_renderer, no window, no input, SDL's dummy audio). This track makes OpenCE.app: a Swift/AppKit application that runs the existing VM host (the guest in Hypervisor.framework) and gives the game a window, a Metal renderer, keyboard and mouse input and sound, until bloodgulch plays at 4K and 120fps on Apple Silicon.

The renderer is cut in the guest: a new guest-side Direct3D 8 implementation (built from d3d8_gl.c's structure) turns the game's D3D8 state and draws into Metal-level commands written to a ring in shared guest memory; the NV2A shader translators (nv2a_vsh.c, nv2a_psh.c) emit Metal Shading Language as well as GLSL. The host decodes the ring and submits it to Metal, so a frame costs a few VM exits, not one per draw.

## Functional Requirements

### App shell
1. `OpenCE.app`: a Swift (AppKit) app built through Nix (`nix build .#app`), signed ad hoc with the hypervisor entitlement, bundling the guest image.
2. The C VM host (`port/macos/host`) is linked into the app as a library; the guest's vCPUs run on their own threads, while the process's main thread runs AppKit. Imports that need the main thread (window creation, events, cursor capture) are carried out there and their results returned to the vCPU that asked.
3. Swift calls the Rust runtime (`port/macos/runtime`) through UniFFI where it needs it; the C ABI stays for the C host.
4. The game data root is chosen as today (`HALO_DATA_ROOT`, else a default folder in Application Support), and a missing `maps` folder is reported in a dialog, not only in debug.txt.

### Window and display
5. One window, rendered at its backing (Retina) resolution and following live resizes; the game's view and the HUD are laid out for the new size as the GL build does.
6. Native macOS fullscreen (its own Space), from the menu bar and a key binding.
7. A frame-time overlay (frame time and fps), a setting that is off by default.

### Renderer (Metal)
8. A guest-side D3D8 implementation for Metal covering everything d3d8_gl.c implements: resources (vertex/index buffers, textures incl. swizzled/compressed Xbox formats via xbox_textures.c), render states, vertex and pixel shaders (NV2A programs translated to MSL once per shader / per combination, cached), render targets and the frame's post passes.
9. Commands cross to the host through a ring in guest memory with a doorbell exit at most per batch; the host validates every command (sizes, indices, offsets inside guest memory) before using it, as untrusted input.
10. Shaders are compiled at run time with `MTLDevice newLibraryWithSource`, cached in memory and on disk.
11. Parity with the GL build's extras: the high-res HUD and text (hud_hires.c, text_hires.c) and the post-processing passes (xgpu_post.c, incl. SMAA), with the same settings and defaults.
12. Stock look by default: with the extras at their defaults, frames match the stock game's rendering.

### Input (native)
13. Keyboard and mouse from NSEvent, including relative mouse for looking (cursor hidden and captured while playing, released in menus and on focus loss).
14. No gamepads: the app is played with the keyboard and mouse only, and SDL's joystick and gamepad events are left out of the guest's polls.
15. These reach the guest through its existing SDL-shaped imports (host_sdl_poll_event and the scancode imports), so xinput_sdl.c and sdl_platform.c's input handling stay unchanged.

### Audio (native)
16. AVAudioEngine plays the game's sound: the guest's DirectSound layer (dsound_sdl.c) is unchanged and its audio-stream imports are implemented with AVAudioEngine in the host.

## Non-Functional Requirements
- **Performance:** bloodgulch at 3840×2160, 120fps sustained on the development Mac (Apple Silicon): 99th-percentile frame time ≤ 8.33 ms over a fixed 60 s run. The renderer adds at most a handful of VM exits per frame; the time calls that dominate today (host_sdl_ticks) are answered in the guest.
- **Determinism and netcode:** the simulation is untouched; no fused multiply-add or system maths in shared code; HALO_PORT_NETWORK_VERSION unchanged (nothing sent between machines changes).
- **Safety:** guest-supplied commands, sizes and pointers are bounds-checked on the host; a bad command is dropped and logged, never trusted.
- **Faithful:** departures from the stock look are settings, stock by default, documented in the settings table.
- **Coverage:** >80% for new Swift and Rust code.

## Acceptance Criteria
1. `nix build .#app` produces OpenCE.app; opening it shows the main menu at the window's native resolution.
2. The menus navigate with keyboard and mouse; a bloodgulch game starts and plays (move, look, shoot, HUD) with sound.
3. Resizing and fullscreen work without restarting; focus loss releases the mouse.
4. Golden frames: offscreen captures of fixed scenes (main menu; bloodgulch from a fixed camera) match approved references within a tolerance (pytest, skipped without game data).
5. Unit tests cover the NV2A→MSL translation, the command ring's encoding/decoding and its validation (bad commands rejected).
6. The performance target above is met and recorded (frame-time log), and the VM exits per frame are reported.
7. The headless runs (tools/test_macos_host.py) and the rest of the suite still pass; `nix build` succeeds from a clean checkout.

## Out of Scope
- ProMotion / CAMetalDisplayLink adaptive pacing (the layer presents with vsync).
- Metal ray tracing; a wasm target.
- Intel Macs and macOS before 26; Developer ID signing and notarization.
- Removing the Linux/Windows/Android/web code (d3d8_gl.c stays for now).
- Touch controls, voice chat audio input, and Bink movies (still the null player).
