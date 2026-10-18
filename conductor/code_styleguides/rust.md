# Rust Style Guide (OpenCE host runtime)

Rust owns the low-level host runtime: the guest loader, memory reservation, threads and import dispatch. It is exposed to Swift through UniFFI and to the guest through a narrow C ABI.

## General
- Stable Rust, edition 2021 or later, pinned through the Nix flake (`rust-toolchain.toml` kept in step with it).
- `cargo fmt` and `cargo clippy -- -D warnings` must pass. Don't silence a lint without a comment saying why.
- Follow the [Rust API Guidelines](https://rust-lang.github.io/api-guidelines/) for naming and API shape.
- Keep dependencies few and reviewed. Every new crate needs a reason, and its licence must be compatible with a CC0 repository.

## Portability (wasm32 later)
- Keep platform-specific code (Mach VM calls, `pthread`, Darwin syscalls) behind a small `platform` module with a trait or a `cfg`-selected implementation, so a `wasm32` implementation can be added without touching the rest.
- Core crates must build without macOS-only APIs. CI should check `cargo check --target wasm32-unknown-unknown` once the first portable crate exists.

## Unsafe code and the FFI boundary
- `unsafe` lives only in the modules that touch the guest's memory or the C ABI. Every `unsafe` block has a `// SAFETY:` comment stating the invariant it relies on.
- Guest pointers are 32-bit offsets into the reserved guest region. Represent them with a newtype (`GuestPtr(u32)`), never as host pointers, and check bounds when you turn one into a slice.
- `extern "C"` functions never panic across the boundary: catch panics (`std::panic::catch_unwind`) or use `panic = "abort"`, and return error codes the C side understands.
- The C ABI is `#[repr(C)]` types and plain integers only. A generated header (cbindgen) is the single source of truth for C callers.

## UniFFI
- Expose a small, coarse API to Swift (start, stop, pause, settings, input and render hand-off). Don't expose per-frame fine-grained calls through UniFFI.
- Declare interfaces with proc-macros (`#[uniffi::export]`) and generate the Swift bindings in the build, never by hand.

## Errors and logging
- Libraries return `Result` with a `thiserror` error type. No `unwrap()` / `expect()` outside tests, except for invariants documented in a comment.
- Log with the `log` (or `tracing`) facade. The macOS host routes it to `os_log`.

## Tests
- `cargo test` for unit tests. Behaviour that touches the guest's memory model (bounds, `GuestPtr` conversion, import dispatch) gets tests that don't need the game data.
