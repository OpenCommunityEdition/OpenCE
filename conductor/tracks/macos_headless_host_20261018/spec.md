# Spec: Run the game on macOS (headless)

## Goal
Prove the decompiled game runs natively on Apple Silicon macOS. A native arm64 host loads the existing `arm64_32` guest image and runs it far enough to host a network test game, with no window and no renderer. The track also sets up the Nix flake that every later track builds with.

This is the foundation for everything in `product.md`. The Swift app shell, the Metal renderer, the removal of the other platforms and ray tracing are later tracks.

## Background
- macOS has no 32-bit support, so the 32-bit x86 Linux/Windows build cannot run there.
- The Android build already runs the game as ILP32 AArch64 code in a 64-bit process (`port/android/README.md`, "ILP32 code" onwards):
  - `tools/android_build.py` compiles the guest for `arm64_32-apple-watchos`, converts the assembly to ELF, and links a static image (`halo_guest.elf`, `port/android/guest/guest.ld`) at a fixed address.
  - The host (`port/android/host/`, about 3,500 lines of C) reserves the address space below 4 GB, loads the image and fills its import table. It runs guest threads on guest stacks and carries out the guest's calls: system calls (Linux numbers, through musl), SDL, OpenGL ES, files and sockets.
- The guest image itself does not depend on Android. The macOS host can reuse it. A new host has to replace the Android-only parts: JNI, ART reclaim, `host_gl.c`, `host_touch.c` and the Java app.

## Fixed addresses
The guest needs the Xbox memory at `0x80000000`–`0x88000000` and its image above it, up to `0x8c000000`. The cache files hold pointers to these addresses, so they cannot move.

## Phase 1 results (`docs/macos_host.md`)
- **Memory below 4 GB: refused.** An arm64 executable with a small page-zero is killed at exec. A normal process cannot allocate below 4 GB.
- **`TPIDR_EL0`: overwritten** by the kernel. On Android the guest asks the host for its thread pointer anyway (`host_get_tp`).
- **Hypervisor.framework: works.** A VM maps and runs code at `0x80000000`, and each exit to the host costs about 1 µs.

**Decision (2026-10-18): approach A.** The guest runs in a VM under Hypervisor.framework. The guest image keeps its code; only the generated import stubs differ (`svc` instead of a branch through the import table). The host is signed with `com.apple.security.hypervisor` (ad hoc for development).

## Scope
**In:**
- The Phase 1 spike programs and a written result (`docs/macos_host.md`).
- A Nix flake: `devShells.default` (clang with the `arm64_32` target, `ld.lld`, ninja, Python 3, Rust stable, SDL3) and packages `guest` and `host`.
- A macOS build in `configure.py` (`tools/macos_build.py`, `ninja macos`). The guest is built as for Android; the host is a native arm64 executable.
- A VM host under `port/macos/host/`. It runs the guest in a VM: the guest memory mapped at its fixed addresses, stage-1 page tables, EL1 vectors that turn an `svc` into an exit, a vCPU per guest thread, and the thread pointer in `TPIDR_EL0`. It carries out the imports adapted from `port/android/host/`: system calls (Linux numbers carried out with Darwin calls), files, sockets, SDL audio and input with SDL's dummy video driver. The pure-logic pieces (`GuestPtr` and guest memory, the address-space plan, the page tables, the ELF image checks) are a Rust crate, `port/macos/runtime/`, behind a C ABI, so the Rust toolchain is in place for the later tracks.
- A headless run: `debug.null_renderer`, `debug.hidden_window`, `debug.exit_after`, and `HALO_NETWORK_TEST=host:<map>` reach a hosted game and log each player's state.

**Out:**
- The Swift app, Metal, windows and the .app bundle (next track).
- Removing the Linux, Windows, Android and web code (later track; this track does not break them).
- UniFFI bindings (added with the Swift app track; the Rust crate only exposes a C ABI here).
- Performance work, beyond counting the exits as a baseline for the renderer.

## Acceptance criteria
1. `docs/macos_host.md` records the result for each Phase 1 risk, with the commands and output that settle it.
2. `nix develop` gives a shell in which `python configure.py && ninja macos` builds the guest image and the host.
3. `nix build .#host` builds the host (signed ad hoc with `com.apple.security.hypervisor`) and the guest image from a clean checkout.
4. `cargo test` passes for `port/macos/runtime`, with >80% line coverage (`cargo llvm-cov`).
5. With the game data in `assets/maps`, this command logs a hosted game for at least 30 seconds and exits with status 0, with no failed assertion:
   `HALO_NETWORK_TEST=host:bloodgulch` plus `debug.null_renderer`, `debug.hidden_window` and `debug.exit_after=30`.
6. A pytest (`tools/test_macos_host.py`) runs check 5 and is skipped when the maps are missing.
7. The host logs the exits per second by import during the headless run.
8. The existing builds still configure: `python configure.py` succeeds, and `ninja linux` / `ninja android` are not changed by this track.
9. Each change in `source/` (none expected) is marked `port:`. No Xbox SDK headers are read.
