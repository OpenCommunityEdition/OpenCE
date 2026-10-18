# Swift Style Guide (OpenCE macOS host)

Swift owns the macOS app: the app shell, windowing, input, audio, lifecycle, the .app bundle and the Metal renderer.

## General
- Follow the [Swift API Design Guidelines](https://www.swift.org/documentation/api-design-guidelines/): clarity at the point of use, names that read as phrases, no needless words.
- Format with `swift-format` using the project's configuration. Don't hand-format against it.
- Target the current macOS SDK, arm64 only. Don't add availability checks for older systems unless a track asks for them.
- Prefer value types (`struct`, `enum`). Use `final class` when reference semantics or Objective-C interop is needed.

## Concurrency and the game loop
- The game runs on its own guest threads. Swift never calls into the guest from arbitrary threads; it calls through the host runtime's API (Rust via UniFFI, or C).
- UI work happens on the main actor. The render loop is driven by `CAMetalDisplayLink` (or `MTKView`) on its own thread, and it must not block on the main thread.
- Use Swift concurrency (`async`/`await`, actors) for app-level work such as file pickers, updates and settings. Don't use it inside the per-frame path.

## Metal
- Create long-lived objects once: device, queues, pipeline states, samplers and heaps. Never create a pipeline state per frame.
- Keep a fixed number of frames in flight (triple buffering) and guard them with a semaphore. Ring-buffer per-frame data.
- Compile shaders to a `.metallib` at build time. Don't compile from source at run time.
- Label every resource and command encoder, and use debug groups (`pushDebugGroup`) so frame captures are readable.
- Treat a frame-time regression at 4K / 120fps as a bug. Check renderer changes with a Metal frame capture or Instruments, before and after.
- Features that change the stock look, such as ray-traced lighting, sit behind a setting that is off by default.

## Interop
- Talk to the guest/host runtime through the generated UniFFI bindings or a small, documented C header. Don't scatter `UnsafePointer` work across the app; keep it in one interop layer.
- Each `unsafe` / `Unmanaged` use gets a comment that says why it's sound.

## Errors and logging
- Use `throws` and typed errors at API boundaries. Don't `try!` or force-unwrap outside tests and truly impossible cases. When one is impossible, say why in a comment.
- Log with `os.Logger`, one subsystem with a category per area.

## Tests
- XCTest (or Swift Testing) for logic that doesn't need the game data. Tests that need `assets/maps` are marked and skipped when the data is missing.
