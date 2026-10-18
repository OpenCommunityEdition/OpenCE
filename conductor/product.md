# Product Guide: OpenCE (macOS)

## Vision
Bring Halo: Combat Evolved (Xbox build 2342) to modern macOS as a fast, native application. Starting from the existing decompilation port, we narrow scope to macOS only and lean into Apple hardware: a native .app driven by a Metal renderer, targeting 4K at 120fps+. Dropping other platforms shrinks the codebase and lets the engine specialize for Apple Silicon.

## Target Platform
- **macOS only** (Apple Silicon-first). Linux, Windows, Android, and web targets are removed going forward to reduce scope.
- Shipped as a **native macOS application package** (.app).

## Target Users
- **Mac players** wanting Halo: CE running natively and fast on modern Apple hardware.
- **Contributors & preservationists** interested in a clean, CC0 macOS decompilation port.

## Core Value
- **Native & performant:** Metal renderer exploiting Apple Silicon; 4K + 120fps+ target.
- **Faithful:** game logic stays byte-close to the decompiled C original.
- **Focused:** one platform, a smaller/sharper codebase and toolchain.
- **Free & open:** CC0; game data from the user's own disc image; no commercial assets shipped.

## Architecture & Languages
- **C core (`source/`):** the decompiled game, kept nearly verbatim; wrapped/extended, not rewritten.
- **Rust (via UniFFI):** low-level, portable engine-adjacent logic where memory safety and a clean FFI boundary matter. UniFFI generates the Swift bindings. Chosen partly to keep a **wasm32 build target reachable later**.
- **Swift + Xcode:** the macOS app shell, windowing/lifecycle, packaging, and the **Metal harness**.
- Rationale: Swift is the native path to AppKit/Metal/Xcode; Rust gives low-level control and a smooth future wasm on-ramp. Both are used at different layers rather than choosing one.

## Toolchain & Workflow
- **Nix** is the primary shell and harness for native builds — the Nix dev shell for quick iterative changes, `nix build` for final builds to close out a track.
- Migration toward the **Swift/Xcode** toolchain for the Apple app layer and the Metal harness, alongside the **Rust** toolchain for the low-level layer.

## Key Features
- Native macOS .app running Halo: CE at performant speeds.
- Metal-based rendering path replacing the D3D8-on-OpenGL layer.
- 4K + 120fps+ on modern Apple displays/hardware.

## Roadmap / Sequenced Later
- **Hardware-accelerated ray tracing** for the lighting engine — unlocked once the Swift/Metal harness is in place (Metal ray tracing on Apple Silicon).
- **wasm build target** — enabled by keeping the low-level layer in Rust; revisited after the macOS app is solid.

## Parked (far-future, viability TBD)
- **Ragdoll & extended physics:** extend Halo 1's native physics, studying how Halo 2 / Halo 3 handled player–environment interaction and building a native extension inspired by them. Explicitly parked; viability (including whether decompiled later engines can inform it) assessed later.

## Non-Goals
- Shipping game data or commercial fonts/art.
- OpenSauce feature support.
- Reading from or copying the Xbox SDK headers.
- Supporting Linux, Windows, Android, or web (web via wasm is a later roadmap item, not a current target).
