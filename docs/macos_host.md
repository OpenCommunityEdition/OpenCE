# The macOS host

The game runs on Android as ILP32 AArch64 code (`arm64_32`) in a 64-bit
process: 64-bit instructions with 32-bit pointers (`port/android/README.md`,
"ILP32 code"). The guest image, `halo_guest.elf`, does not depend on
Android, so a macOS host can load the same image. It needs three things from
the system:

1. The guest's fixed addresses: the Xbox memory at `0x80000000` to
   `0x88000000` and the image above it, to `0x8c000000`. The cache files
   hold pointers to these addresses, so they cannot move.
2. An executable mapping of the guest image.
3. A thread pointer for each guest thread. On Android the guest does not
   keep it in `TPIDR_EL0` (the host's bionic uses the register): the guest
   asks the host for it (`host_get_tp`, `pthread_arch.h`). The spike checks
   whether macOS would let a guest use the register, because asking the
   host costs an exit in a VM.

The programs in `port/macos/spikes` check each one on arm64 macOS, and
`tools/test_macos_spikes.py` builds and runs them.

## Results

Checked on macOS 26.6.2 (SDK 26.5), Apple Silicon, Apple clang 21.0.0.

### Memory below 4 GB: refused

`low_memory.c` tries to map `0x80000000` to `0x8c000000`.

Linked with a small page-zero (`-Wl,-pagezero_size,0x4000`), the program
is killed as it starts (status 137, SIGKILL), and nothing is logged.
`otool -l` shows the `__PAGEZERO` segment as asked (`vmsize 0x4000`), so
the kernel refuses the binary itself. An arm64 executable must have a 4 GB
page-zero.

Linked as usual, the program starts, but nothing below 4 GB can be mapped:

```
region at 0x80000000: free; next region 0x100958000-0x10095c000
mmap with a hint: placed at 0x105624000 instead
mach_vm_allocate fixed: (os/kern) invalid address
mach_vm_deallocate of the range: (os/kern) successful
mach_vm_allocate fixed: (os/kern) invalid address
FAIL: no way to map 0x80000000-0x8c000000
```

The lowest address the process may use is 4 GB. Freeing the range changes
nothing, because it is outside the process's address space altogether. Thus
the guest cannot live in the host's own address space, as it does on
Android.

### Thread pointer: overwritten

`thread_pointer.c` sets `TPIDR_EL0` on two threads and checks it after each
yield or sleep:

```
TPIDR_EL0 at start: 0x2006
FAIL: thread 0 lost its TPIDR_EL0 at round 1
```

macOS uses `TPIDR_EL0` itself (it holds a value before the program sets
it), and the kernel writes it again at each context switch. A guest in the
host's process keeps asking the host for its thread pointer, as on Android.
Inside a VM the register is the guest's own, so there the guest can read it
directly and save an exit on each access to thread-local data.

### Executable image

`hv_vm_map` maps the guest's memory executable inside a VM (below), so no
separate check of executable mappings in the host process was made. That
check matters only to the in-process approach (option B).

### Hypervisor.framework: works

`hypervisor.c` creates a VM, maps a page of host memory at guest physical
address `0x80000000`, and runs two instructions there at EL1 with the MMU
off: an add, and an `hvc` that exits to the host. It is signed ad hoc with
the `com.apple.security.hypervisor` entitlement
(`hypervisor.entitlements`):

```
hv_vm_create: success
hv_vm_map at 0x80000000: success
guest code at 0x80000000 ran 100000 times; x0 = 100000
round trip guest -> host -> guest: 1004 ns
PASS
```

A VM has an address space and registers of its own, so the fixed addresses
and `TPIDR_EL0` are the guest's. Each exit to the host costs about 1 µs.

## The ways forward

### A. The guest in a VM (Hypervisor.framework)

The guest image is unchanged. The host:

- creates a VM, maps the guest's memory at its fixed addresses, and one
  vCPU for each guest thread (each on a host thread, as Hypervisor.framework
  requires);
- gives the guest a small EL1 runtime: exception vectors, and page tables
  that map the guest's range as normal, cacheable memory (with the MMU off,
  all memory is device memory: slow, and unaligned accesses fault);
- turns the guest's host imports into exits. Every call the guest makes
  to the host, system calls included (`__guest_syscall` calls
  `host_syscall`), goes through one generated stub
  (`tools/android_imports.py`) that branches through
  `__host_import_table`. In the VM build the stub is `mov w16, #index;
  svc #0; ret`, and the EL1 vector turns the `svc` into an `hvc`. The host
  reads the arguments from the vCPU's registers, turns guest addresses into
  host addresses, and carries out the call as the Android host does
  (`host_syscall.c`, `host_sdl.c`). The game's sources do not change;
- keeps the thread pointer in `TPIDR_EL0`, which is the guest's own in a
  VM, instead of the `host_get_tp` import, which would be an exit on each
  access to thread-local data;
- sends rendering in batches: a command buffer in shared memory, handed to
  the host once per buffer or per frame, not one exit per call. At about
  1 µs per exit, the thousands of draw calls of a frame cannot each be an
  exit at 120 fps (8.3 ms per frame).

The app needs the `com.apple.security.hypervisor` entitlement. Ad hoc
signing is enough for development. A distributed app is signed with it.

### B. The guest in the host's process, with rewritten code

The guest's code would be changed so that it does not need low addresses.
Every memory access would add a base register (`[x28, w1, uxtw]`). The place for
this is the step that already converts the guest's assembly
(`tools/android_asm_convert.py`). There would be no VM exits, but every
load, store, atomic and address computation in the game must be found and
rewritten, and one that is missed corrupts memory. The image would also need
an executable mapping in the host process (`MAP_JIT` or the hardened
runtime's exceptions).

### Recommendation

Option A, which the host now carries out (below): the guest's code stays as it is, the host carries over most of
the Android host, and the cost (an exit per host call) is limited by
batching the renderer, which the Metal renderer needs in any case.

## Building

The macOS build is for Apple Silicon Macs with macOS 26 or later. Its tools
come from Nix (`flake.nix`): clang 22 and lld for the guest, the macOS 26
SDK and SDL3 for the host, and Rust (`rust-toolchain.toml`) for the
runtime crate, `port/macos/runtime`.

```sh
nix develop                    # the shell: every tool the build needs
python configure.py            # on a Mac, the default target is macos
ninja macos                    # build/macos/halo_host and build/macos/halo_guest.elf
nix build .#host               # the final build: result/bin/halo_host, result/share/opence/halo_guest.elf
ninja macos_app                # build/macos/OpenCE.app (needs Xcode)
nix build .#app                # result/Applications/OpenCE.app
ninja macos_app_test           # the Swift package's tests (port/macos/app/Tests)
```

The app, `OpenCE.app`, is the Swift package `port/macos/app` built with
Xcode's Swift toolchain (`xcrun swift build`; nixpkgs' Swift cannot link
against the macOS 26 SDK) and linked with `libhalo_host.a`, the runtime
crate and SDL3. `tools/macos_app.py` lays out the bundle
(`Contents/MacOS/OpenCE`, `Contents/Info.plist`,
`Contents/Resources/halo_guest.elf`) and signs it ad hoc with the
hypervisor entitlement, using Apple's `codesign`. AppKit keeps the main
thread; the guest's main runs on a thread of its own. Being outside the
store, Xcode makes `nix build .#app` impure: it needs a Nix without the
sandbox.

Swift calls the runtime crate through UniFFI (`port/macos/runtime/src/swift.rs`).
For now that is a view of the guest's memory, `GuestMemoryView`. Its
`hostAddress`, `read` and `write` accept only ranges that are wholly mapped,
and throw `GuestMemoryError` for any other. `runningGuestMemory()` returns the
memory of the guest the C host is running. The memory is mapped when the
guest is made and never again, so the view needs no lock; it keeps the memory
alive while the view exists. The build makes the crate's `uniffi-bindgen`
(`--features bindgen`, in a target folder of its own) and runs it on
`libopence_runtime.a`. It writes the bindings into the package's ignored
folders `Sources/OpenCERuntime` and `Sources/opence_runtimeFFI`.

`ninja macos` builds:

- the guest image, `build/macos/halo_guest.elf`: the Android guest's
  sources with `HALO_GUEST_VM` defined (its imports are `svc` stubs, and
  it keeps its thread pointer in `TPIDR_EL0`);
- the runtime crate, a static library with a C header made by cbindgen
  (`build/macos/runtime/opence_runtime.h`): the guest's memory plan, its
  page tables and the image's checks;
- the host library, `build/macos/libhalo_host.a`: `port/macos/host`, the
  Android host's SDL bridge (`host_sdl.c`), the platform layer's files and
  sockets (`posix_files.c`, `posix_net.c`), UPnP and DTLS, and the import
  dispatcher that `tools/macos_dispatch.py` writes from
  `port/android/host_imports.list`. Its entry point is `halo_host_run`
  (`port/macos/host/halo_host.h`);
- the command-line host, `build/macos/halo_host`: `port/macos/cli/main.c`
  linked with the library.

### Signing

The host needs the `com.apple.security.hypervisor` entitlement
(`port/macos/host/halo_host.entitlements`). The build signs it ad hoc
with the entitlement, which is enough to run it on the Mac that built it.
A copy for other Macs must be signed with a Developer ID that has it.
The Nix package skips the fixup phase, whose signing would drop the
entitlement.

## Running

```sh
build/macos/halo_host [build/macos/halo_guest.elf] [arguments for the game]
```

The image defaults to `halo_guest.elf` beside the host, or
`../share/opence/halo_guest.elf` (the Nix package's layout). The game's data
folder is `HALO_DATA_ROOT`, else the current folder: its `maps` folder holds
the maps, and `config.toml` and `debug.txt` are written there. Saves and
profiles go to `XDG_DATA_HOME`, by default `~/Library/Application
Support/OpenCE`.

Without a person at it, as on Linux (`AGENTS.md`):

```sh
HALO_DATA_ROOT=assets HALO_NULL_RENDERER=1 HALO_HIDDEN_WINDOW=1 \
SDL_VIDEO_DRIVER=dummy SDL_AUDIO_DRIVER=dummy \
HALO_NETWORK_TEST=host:bloodgulch HALO_NET_ADDRESS=127.0.0.1 HALO_EXIT_AFTER=30 \
build/macos/halo_host
```

hosts a game on bloodgulch and logs its player's state each second
(`network test: tick ...`), then exits with status 0.

The app's data folder is `HALO_DATA_ROOT` too, else
`~/Library/Application Support/OpenCE` (`DataRoot` in
`port/macos/app/Sources/OpenCEShell`). The app makes its `maps` folder.
If `ui.map` is not in it, the app does not start the game: a dialog says
where to copy the maps, offers to show the folder in the Finder, and quits.

Two copies on one Mac need loopback addresses of their own, and macOS has
only 127.0.0.1 on `lo0`. Add others with
`sudo ifconfig lo0 alias 127.0.0.2 up` (and `127.0.0.3`). They last until
the Mac restarts.

The host's own settings:

| Variable | Meaning |
| --- | --- |
| `HALO_HOST_QUIET` | Logs only the host's warnings and errors |
| `HALO_HOST_IMPORT_COUNTS` | At exit, reports the VM's exits per second, in all and by import (busiest first) |
| `HALO_HOST_SAMPLE=<seconds>` | Every that many seconds, prints each vCPU's guest address and its frames |
| `HALO_HOST_MAIN_LOOP=1` | Runs the guest on a thread of its own and the main dispatch queue on the main thread, as the app does |
| `HALO_HOST_TRACE_MAIN_THREAD` | Reports each import the first time it is carried out on the main thread |

### The main thread

AppKit, and SDL's windows and events with it, must be used on the main
thread. The app's main thread runs AppKit, so the guest's threads cannot
call those imports there themselves. The imports that need it are listed in
`tools/macos_dispatch.py` (`MAIN_THREAD`): SDL's initialisation, the window
and its size, relative mouse mode, the event queue, message boxes and the
clipboard. The OpenGL context's imports are not among them, because they
belong to the thread that made the context current.

When the guest asks for one of these imports, its vCPU thread queues the
request (`port/macos/host/host_main_thread.c`). The main dispatch queue is
asked to run it, and the thread waits. The run loop drains that queue in its
common modes, so a request also runs during a modal alert or a live resize.
The import's result and errno are brought back to the vCPU.

The app turns this on with `halo_host_use_main_queue` before it starts the
guest. Before that, and on the main thread itself, an import runs where it
is asked for. As the app terminates, `halo_host_shutdown` fails any queued
request with `ECANCELED`, and any made later, so that no guest thread
waits for a run loop that has stopped.

SDL keeps its error message per thread. After a failed import on the main
thread, `SDL_GetError` on the guest's thread does not have that import's
message.

A guest crash is reported with its registers, the exception's syndrome and
the guest's frames, by address in the image (`llvm-symbolizer` on
`halo_guest.elf` names them).

### Exits

Each call the guest makes to the host is an exit from the VM, at about
1 µs. A hosted bloodgulch game, headless (null renderer) for 30 seconds,
made 503 000 exits a second, half of the time of one core:

| Import | Calls a second |
| --- | --- |
| `host_syscall` | 248 000 |
| `host_sdl_ticks` | 164 000 |
| `host_errno` | 32 000 |
| `hostposix_socket_last_error` | 12 000 |
| `hostposix_socket_recv` | 9 000 |

This is the baseline for the next steps: the renderer's commands go in
batches, and the time and error calls that run most can be answered in
the guest.

## Tests

In `nix develop`, after `ninja macos`:

```sh
python -m pytest -q tools/test_macos_spikes.py tools/test_macos_build.py tools/test_macos_vm.py tools/test_macos_host.py
cargo test --manifest-path port/macos/runtime/Cargo.toml
python -m pytest -q tools/harness         # the asset-free harness, in the VM
```

`tools/test_macos_vm.py` runs guest programs (`port/macos/tests`) in the
host: threads, futexes, time, files, memory, sockets and SDL, headless, and
a window made from a guest thread in the main-loop mode. It also builds two
of the host's files natively and tests them: the system calls, and the
main thread's queue with its shutdown.
`tools/test_macos_host.py` runs the game: without maps, with them, a
hosted network game, and a second copy that joins it (when `lo0` has the
aliases).

## Not supported yet

- Drawing: OpenGL ES is refused while headless, and there is no Metal
  renderer yet. The game runs only with `debug.null_renderer`.
- A window, and input from it.
- Intel Macs, and macOS before 26.
- An app bundle.
