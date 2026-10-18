# Plan: Run the game on macOS (headless)

Track: `macos_headless_host_20261018`. Spec: [spec.md](./spec.md). Workflow: [../../workflow.md](../../workflow.md).

Prerequisite (the user does this): `sudo xcodebuild -license accept`. Apple's clang and SDK do not run until the licence is accepted.

## Phase 1: Feasibility spike on macOS [checkpoint: cfe80db]

Small stand-alone C programs in `port/macos/spikes/`, each one printing PASS or FAIL. Each has a pytest in `tools/test_macos_spikes.py` that builds it, runs it and checks its output. The tests are written first and fail until the programs exist.

- [x] Task: Write `tools/test_macos_spikes.py`, which builds and runs each spike and expects PASS (red) 83a1f3d
- [x] Task: Spike `low_memory`: try a host linked with `-pagezero_size 0x1000`, then mapping `0x80000000`–`0x8c000000` with fixed `mmap` / `mach_vm_allocate`; if the link or the load is refused, try freeing the page-zero range at start-up. Record which way works 7105e76 (result: refused both ways)
- [x] Task: Spike `exec_image`: map a small arm64_32 code page at `0x88000000` as executable, call into it through a trampoline that switches between ILP32 and 64-bit, return a value. Note the entitlements a hardened runtime would need 867f88c (replaced by the `hypervisor` spike: a VM runs code at `0x80000000`, about 1 µs per exit)
- [x] Task: Spike `thread_pointer`: set `TPIDR_EL0` on two threads, sleep and yield, and check that each keeps its own value 6849eaf (result: overwritten at the first context switch)
- [x] Task: Write `docs/macos_host.md` with each risk's result, the commands and their output, and the chosen approach; update `tech-stack.md` (remove "Unverified") or stop the track if no approach works 94ba2cf (approach pending the user's decision)
- [x] Task: Conductor - User Manual Verification 'Phase 1: Feasibility spike on macOS' (Protocol in workflow.md)

## Phase 2: Nix flake and the macOS build [checkpoint: 1155c87]

Phase 1 settled the approach (`docs/macos_host.md`): the guest runs in a VM under Hypervisor.framework.

- [x] Task: Write `tools/test_macos_build.py`: the configured build graph has a `macos` target with the guest image and the host, and `ninja -n macos` succeeds; the VM build's import stubs are `mov w16, #index; svc #0; ret` (red) 6e19415
- [x] Task: Add `flake.nix` and `flake.lock`: `devShells.default` with clang (arm64_32 target), `ld.lld`, ninja, Python 3 with pytest, Rust stable (`rust-toolchain.toml`), `cargo-llvm-cov` and SDL3, using the impure Apple SDK from Xcode dc40b99
- [x] Task: Add the VM import stubs: a mode of `tools/android_imports.py` (shared with Android, not copied) that writes each stub as `mov w16, #index; svc #0; ret`, and the host's table of import names by index 75634e7
- [x] Task: Add `tools/macos_build.py` and the `macos` target to `configure.py`: the guest image with the Android guest's rules and the VM stubs, and an empty host signed ad hoc with `com.apple.security.hypervisor` a5ed7eb
- [x] Task: Add the flake packages `guest` and `host`, which run `configure.py` and `ninja macos` in the sandbox be4f87a
- [x] Task: Check that `python configure.py` and the existing `linux` and `android` targets are unchanged (`ninja -t commands` before and after) (checked: identical commands, no code change)
- [x] Task: Conductor - User Manual Verification 'Phase 2: Nix flake and the macOS build' (Protocol in workflow.md)

## Phase 3: The Rust runtime crate [checkpoint: e522066]

`port/macos/runtime/`: the pure-logic pieces of the VM host, behind a C ABI (cbindgen header), with tests that need no game data.

- [x] Task: Write tests for `GuestPtr(u32)` and the guest memory: guest addresses map to offsets in one host reservation of 4 GB, with bounds-checked conversion to host slices, and rejection of unmapped addresses and ranges that wrap (red) 7955264
- [x] Task: Implement `GuestPtr` and the guest memory, with the Darwin reservation behind a `platform` module 1a9b22a
- [x] Task: Write tests for the guest's address-space plan, from `port/android/host/host_memory.c`'s layout: the fixed ranges (`0x80000000`–`0x88000000`, image to `0x8c000000`), the pools and stacks, overlap checks, and the error for a range already taken (red) caa8a80
- [x] Task: Implement the address-space plan 9743893
- [x] Task: Write tests for the guest's page tables: a 4 KB-granule, 4 GB stage-1 map (TCR `T0SZ=32`) built from the plan, with normal cacheable memory, EL0 access, execute only where the image's code is, and the descriptor bits checked against the Arm architecture (red) eebe8b7
- [x] Task: Implement the page-table builder and the system-register values the vCPUs start with (`TTBR0_EL1`, `TCR_EL1`, `MAIR_EL1`, `SCTLR_EL1`, `CPACR_EL1`, `VBAR_EL1`) 7df6d14
- [x] Task: Write tests for the ELF image checks: the header, program headers inside the file, segments inside the image range, the import table's bounds, with malformed images (red) f634cca
- [x] Task: Implement the image checks from `host_loader.c`, and export the C ABI with a cbindgen header; `cargo fmt`, `cargo clippy -D warnings`, and coverage >80% b6bd54a 8f580af
- [x] Task: Conductor - User Manual Verification 'Phase 3: The Rust runtime crate' (Protocol in workflow.md)

## Phase 4: The VM host [checkpoint: 2b0b0d1]

`port/macos/host/`: the VM, and the imports adapted from `port/android/host/`. Shared code is moved or included rather than copied where the Android host stays as it is.

- [x] Task: Write `tools/test_macos_host.py` with a smoke test: the host creates the VM, loads the guest image, runs to the guest's `main` (a log line through `host_log`) and exits with status 0 under `debug.exit_after` (red) 61722a9
- [x] Task: The VM: reserve the guest memory (Rust), map it into the VM, build the page tables, load and check the image (Rust), and start a vCPU at EL0 at the guest's entry 76bead7
- [x] Task: The guest's EL1 vectors (assembly, mapped by the host): an `svc` from EL0 becomes an `hvc` and then an `eret`; any other exception exits with `ESR_EL1`, `FAR_EL1` and `ELR_EL1`, and the host reports it as a crash with the guest address 76bead7
- [x] Task: The import dispatcher: generated from `guest_host.h`'s prototypes, it reads each import's integer and floating-point arguments from the vCPU's registers, turns guest addresses into host addresses, calls the host function and writes the result back; tests for the generator 76bead7
- [x] Task: Threads: a vCPU for each guest thread, on its own host thread (`host_thread_create`); host calls into the guest (thread entry, callbacks) set the vCPU's `pc`, `sp` and `x0`, with `lr` at a stub that exits back to the host 76bead7 aff181e
- [x] Task: The thread pointer in `TPIDR_EL0`: under the VM build, `__guest_get_tp` reads the register and `host_set_tp` sets it on the vCPU, so thread-local data costs no exit 7cce78a
- [x] Task: System calls: `host_syscall` carries out the guest's Linux-numbered calls with Darwin calls (from `host_syscall.c`), with guest addresses turned into host addresses, futexes on guest memory, and `errno` mapped; unit tests for the number and `errno` mapping 76bead7 d4c8684
- [x] Task: Files, sockets and settings: the data root next to the executable (`assets/` in development), sockets for the network test 8295d2f
- [x] Task: SDL: audio and input through `host_sdl.c`'s imports, with SDL's dummy video driver (the audio callback runs on a vCPU of its own); the OpenGL ES imports return errors, since only `debug.null_renderer` is supported in this track 76bead7 a74f5ed
- [x] Task: Run the asset-free harness (`tools/harness`) in the VM: on macOS it compiles each test for the arm64_32 guest, links it as a guest image with the guest runtime, and runs it in the VM host, which reports the test's output and status; the harness's tests pass on macOS as on Linux (on macOS every test fails today to compile, as on `main`) 93dd075
- [x] Task: Conductor - User Manual Verification 'Phase 4: The VM host' (Protocol in workflow.md)

## Phase 5: A headless game, and the final build [checkpoint: 9f7ddd3]

- [x] Task: Extend `tools/test_macos_host.py`: with `assets/maps`, `HALO_NETWORK_TEST=host:bloodgulch`, `debug.null_renderer`, `debug.hidden_window` and `debug.exit_after=30` log the hosted game's player state each second and exit 0 with no failed assertion; skipped without the maps (red) `7460d36`
- [x] Task: Fix what the headless run finds until the test passes `7460d36`
- [x] Task: Count the exits: the host logs the exits per second by import, as the baseline for the renderer's batching in the next track `f2c905a`
- [x] Task: Join test: two copies on loopback addresses (`HALO_NET_ADDRESS`), one hosting and one joining; both log the other player `9ead8ee` (skipped until lo0 has the aliases)
- [x] Task: Document the macOS build in `docs/macos_host.md` (building, signing, running headless, what is not supported yet) and link it from `README.md` `bb35180`
- [x] Task: Final build: `nix build .#host` from a clean checkout, then the whole test suite inside `nix develop` (nix build .#host ok; suite: 678 passed, 3 failed in tools/test_profile.py as on main on a Mac)
- [x] Task: Conductor - User Manual Verification 'Phase 5: A headless game, and the final build' (Protocol in workflow.md)

## Phase: Review Fixes
- [x] Task: Apply review suggestions 6aeb40a2
