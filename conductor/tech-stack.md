# Tech Stack: OpenCE (macOS)

## Game Core (kept)
- **C (C89, `gnu89`)**: the decompiled game in `source/` and the shared port layer in `port/linux/src` and `port/linux/game`.
- **Guest ABI: ILP32 AArch64** (`arm64_32-apple-watchos` via Apple clang), the model the Android build already uses. 64-bit instructions with 32-bit pointers, so the decompiled code runs unchanged on Apple Silicon. macOS has no 32-bit support, so the x86 build is not an option.
- **Vendored C libraries** (`port/third_party`): musl maths (for determinism), zlib, expat, opus, stb / stb_vorbis, mbedtls, monocypher, kcp, miniupnpc, tomlc17, smaa, extract-xiso.

## macOS Host (new)
- **Swift + Xcode / Apple SDK (macOS 26, arm64)**: the app shell (AppKit), windowing, input (GameController), audio, lifecycle, the .app bundle, and the **Metal** renderer replacing D3D8-on-OpenGL. The rendering imports become Metal calls.
- **Rust (stable, via Nix) + UniFFI**: the low-level host runtime, moved over from the Android host by degrees (guest loader, memory reservation, threads, import dispatch) behind a narrow C ABI, with UniFFI bindings to Swift. Kept portable so a wasm32 target stays possible.
- **C** where the guest/host boundary needs it (import stubs, the trampolines that switch between ILP32 and 64-bit).
- The guest must live below 4 GB (Xbox memory at `0x80000000`–`0x88000000`, the image up to `0x8c000000`). **Settled (2026-10-18, `docs/macos_host.md`):** an arm64 macOS process cannot map below 4 GB. A small page-zero gets the process killed at exec, and a normal process can't allocate there. The kernel also overwrites `TPIDR_EL0`. So the guest cannot share the host's address space, as it does on Android. Hypervisor.framework works: a VM maps and runs code at `0x80000000`, with an exit to the host costing about 1 µs. The recommended host runs the guest in a VM (entitlement `com.apple.security.hypervisor`), with rendering sent in batches through shared memory. The approach is pending the user's decision.

## Build & Toolchain
- **Nix (flake)**: the primary dev shell (Rust, ninja, Python, lld, clang with the `arm64_32` target) for quick iteration. `nix build` produces the final build that closes out a track.
- **Xcode / xcodebuild**: the Swift app target, Metal shader compilation, code signing and .app packaging. It runs under Nix as the host's impure Apple SDK.
- **configure.py + Ninja**: stays for the guest image during the transition.
- **Python 3**: build scripts, generators (menus, settings, assets) and tests.

## Testing
- **pytest**: the asset-free harness (`tools/harness`) and the map checks.
- **XCTest**: the Swift layer. **cargo test**: the Rust crates.
- **Metal frame capture and Instruments**: performance (4K / 120fps target).

## Removed
- Linux, Windows, Android and web builds and their platform code (`port/windows`, the Android Java app, `port/web`, Emscripten, Gradle), as the migration allows. The Android guest runtime is the exception: it is reused.

## Roadmap
- Metal ray tracing (hardware RT on M3 and later) for the lighting engine, once the Metal renderer is in place.
- A wasm32 target for the Rust layer.
