# Plan: Run the game on macOS (headless)

Track: `macos_headless_host_20261018`. Spec: [spec.md](./spec.md). Workflow: [../../workflow.md](../../workflow.md).

Prerequisite (the user does this): `sudo xcodebuild -license accept`. Apple's clang and SDK do not run until the licence is accepted.

## Phase 1: Feasibility spike on macOS

Small stand-alone C programs in `port/macos/spikes/`, each one printing PASS or FAIL. Each has a pytest in `tools/test_macos_spikes.py` that builds it, runs it and checks its output. The tests are written first and fail until the programs exist.

- [ ] Task: Write `tools/test_macos_spikes.py`, which builds and runs each spike and expects PASS (red)
- [ ] Task: Spike `low_memory`: try a host linked with `-pagezero_size 0x1000`, then mapping `0x80000000`–`0x8c000000` with fixed `mmap` / `mach_vm_allocate`; if the link or the load is refused, try freeing the page-zero range at start-up. Record which way works
- [ ] Task: Spike `exec_image`: map a small arm64_32 code page at `0x88000000` as executable, call into it through a trampoline that switches between ILP32 and 64-bit, return a value. Note the entitlements a hardened runtime would need
- [ ] Task: Spike `thread_pointer`: set `TPIDR_EL0` on two threads, sleep and yield, and check that each keeps its own value
- [ ] Task: Write `docs/macos_host.md` with each risk's result, the commands and their output, and the chosen approach; update `tech-stack.md` (remove "Unverified") or stop the track if no approach works
- [ ] Task: Conductor - User Manual Verification 'Phase 1: Feasibility spike on macOS' (Protocol in workflow.md)

## Phase 2: Nix flake and the macOS build

- [ ] Task: Write `tools/test_macos_build.py`: the configured build graph has a `macos` target with the guest image and the host, and `ninja -n macos` succeeds (red)
- [ ] Task: Add `flake.nix` and `flake.lock`: `devShells.default` with clang (arm64_32 target), `ld.lld`, ninja, Python 3, Rust stable (`rust-toolchain.toml`), `cargo-llvm-cov` and SDL3, using the impure Apple SDK from Xcode
- [ ] Task: Add `tools/macos_build.py` and the `macos` target to `configure.py`; build the guest image with the Android guest's rules (shared, not copied) and an empty host that links with the Phase 1 memory approach
- [ ] Task: Add the flake packages `guest` and `host`, which run `configure.py` and `ninja macos` in the sandbox
- [ ] Task: Check that `python configure.py` and the existing `linux` and `android` targets are unchanged (`ninja -t commands` before and after)
- [ ] Task: Conductor - User Manual Verification 'Phase 2: Nix flake and the macOS build' (Protocol in workflow.md)

## Phase 3: The Rust runtime crate

`port/macos/runtime/`: the pure-logic pieces of the host, behind a C ABI (cbindgen header), with tests that need no game data.

- [ ] Task: Write tests for `GuestPtr(u32)`: bounds-checked conversion to host slices, rejecting addresses outside the guest region and ranges that wrap (red)
- [ ] Task: Implement `GuestPtr` and the guest-region type
- [ ] Task: Write tests for the address-space plan: the fixed ranges (`0x80000000`–`0x88000000`, image to `0x8c000000`, pools), overlap checks, and the error for a range already taken (red)
- [ ] Task: Implement the address-space plan from `port/android/host/host_memory.c`'s layout, with the Darwin reservation behind a `platform` module
- [ ] Task: Write tests for the ELF image checks: the header, the program headers within the file, segments inside the image range, the import table's bounds, with malformed images (red)
- [ ] Task: Implement the image checks from `host_loader.c`, and export the C ABI with a cbindgen header; `cargo fmt`, `cargo clippy -D warnings`, and coverage >80%
- [ ] Task: Conductor - User Manual Verification 'Phase 3: The Rust runtime crate' (Protocol in workflow.md)

## Phase 4: The macOS host

`port/macos/host/`, adapted from `port/android/host/`. Shared code is moved or included rather than copied where the Android host stays as it is.

- [ ] Task: Write `tools/test_macos_host.py` with a smoke test: the host starts, reserves memory, loads the guest image and reaches the guest's `main` (a log line), and exits with status 0 under `debug.exit_after` (red)
- [ ] Task: Host start-up: reserve memory (Phase 1 approach, through the Rust plan), load and check the image (through the Rust checks), fill the import table
- [ ] Task: Threads: run `main` and each guest thread on guest stacks with the guest's thread pointer, from `host_thread.c`
- [ ] Task: System calls: carry out the guest's Linux-numbered calls with Darwin calls (`host_syscall.c`), with `errno` mapped; unit tests for the number and `errno` mapping
- [ ] Task: Files, sockets and settings: the data root next to the executable (`assets/` in development), sockets for the network test
- [ ] Task: SDL: audio and input through `host_sdl.c`'s imports, with SDL's dummy video driver; the OpenGL ES imports return errors, since only `debug.null_renderer` is supported in this track
- [ ] Task: Conductor - User Manual Verification 'Phase 4: The macOS host' (Protocol in workflow.md)

## Phase 5: A headless game, and the final build

- [ ] Task: Extend `tools/test_macos_host.py`: with `assets/maps`, `HALO_NETWORK_TEST=host:bloodgulch`, `debug.null_renderer`, `debug.hidden_window` and `debug.exit_after=30` log the hosted game's player state each second and exit 0 with no failed assertion; skipped without the maps (red)
- [ ] Task: Fix what the headless run finds until the test passes
- [ ] Task: Join test: two copies on loopback addresses (`HALO_NET_ADDRESS`), one hosting and one joining; both log the other player
- [ ] Task: Document the macOS build in `docs/macos_host.md` (building, running headless, the memory approach, what is not supported yet) and link it from `README.md`
- [ ] Task: Final build: `nix build .#host` from a clean checkout, then the whole test suite inside `nix develop`
- [ ] Task: Conductor - User Manual Verification 'Phase 5: A headless game, and the final build' (Protocol in workflow.md)
