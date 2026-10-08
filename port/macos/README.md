# macOS

`ninja macos` builds the game for macOS as an app, `build/macos/Halo.app`.

The app is an x86-64 program. On a Mac with Apple silicon it runs under
Rosetta 2, Apple's translator for Intel programs. On an Intel Mac it runs
natively. It needs macOS 12 or later.

The game draws with OpenGL 4.1, the newest version macOS has. It plays sound
and reads keyboards, mice and controllers through SDL3. It uses the platform
layer of the Linux build (`port/linux/src`) and the design of the Android
build ([port/android/README.md](../android/README.md)).

## What to know first

- The game's code needs 32-bit pointers. macOS runs only 64-bit programs, so
  the game runs as a guest inside a 64-bit host program, as on Android.
- The guest is x32 code: x86-64 instructions with 32-bit pointers. The host
  is an ordinary x86-64 Mac program.
- Rosetta 2 is needed on Apple silicon. Apple has said that from macOS 28
  Rosetta will run only some older games, so this build may stop working
  on Apple silicon then.
- The app is signed ad hoc. It is not notarised, so the first start needs
  Control-click and Open.

## Requirements

You do not need the Xbox SDK. You need:

- the Xcode command line tools (Apple's clang, `codesign`);
- Homebrew's `llvm` and `lld` (`brew install llvm lld`), for the guest's
  compiler, `llvm-ar` and `ld.lld`;
- `ninja`, `cmake` and Python 3. `configure.py` takes Homebrew's `cmake` in
  `/opt/homebrew` first, then the one on the `PATH`;
- Rosetta 2 on Apple silicon, to run the app (`softwareupdate
  --install-rosetta`);
- a network connection for the first build. `configure.py` downloads musl
  1.2.5, SDL 3.4.16 and the OpenGL ES headers to `build/macos/third_party`.

`configure.py --macos-guest-cc` names another clang with the x32 target.
`--macos-lld` names another `ld.lld`.

## Build the app

1. Go to the root folder of the repository.
2. Enter `python3 configure.py`.
3. Enter `ninja`. On a Mac the default target is `macos`.
4. Start the app with `open build/macos/Halo.app`, or move it to
   Applications first.

`--release`, `--pgo` and `--pgo-profile` work as on the other ports. The
guest uses the Linux profile (`pgo/halo_linux.profdata`), which needs
clang 22 or later. Link-time optimisation is not used.

## Game data and files

The game needs the `maps/` folder from an Xbox disc image of the game. At
the first start the app offers to extract it from a disc image (`.xiso` or
`.iso`) you pick.

The app keeps everything in `~/Library/Application Support/OpenCE`, the data
folder. It never writes inside the app.

| Item | Location in the data folder |
| --- | --- |
| Game data | `maps` |
| Saved games | `save` |
| Settings | `config.toml` |
| Game log | `debug.txt` |
| Host log | `host.log` |
| Internet play brokers | `brokers.txt`, copied from the app at each start |

`HALO_DATA_ROOT` and `HALO_SAVE_ROOT` in the environment name other
folders. Every `HALO_` variable the app starts with reaches the game, so the
settings' environment variables work as on Linux.

## How it works

The macOS build is the Android build with x86 in place of ARM. This section
says what differs from Android, and why.

### Two programs in one process

The guest is the game, the platform layer, the guest runtime
(`port/android/guest/runtime`, shared with Android) and a subset of musl,
the C library. Homebrew's clang compiles it for `x86_64-linux-muslx32` into
ELF objects, and `ld.lld` links it into `build/macos/halo_guest.elf`.

The host is `port/macos/host`, built by Apple's clang into an x86-64 Mach-O
program, with SDL3 linked in statically. It loads the guest image, fills in
the guest's import table and serves the guest's calls.

The guest calls the host through a stub per import. Each stub is one
instruction, `jmpq *__host_import_table+8*i(%rip)`, which jumps to the
host function with the arguments untouched.

### Why x32

The game's data formats hold 32-bit pointers, so the game needs code with
32-bit pointers that a 64-bit Mac can run. Android uses clang's `arm64_32`
target for this. A Mac with Apple silicon cannot map memory below 4 GB in a
native ARM program, but an x86-64 program under Rosetta can, with 4 KB
pages. x32 is clang's x86-64 target with 32-bit pointers.

The calling conventions of x32 and x86-64 agree for integers, floats and
pointers. x86-64 has six integer argument registers, not eight as AArch64
has. So the OpenGL stub generator widens pointer and 64-bit arguments from
the seventh on (`tools/android_gl_stubs.py --integer-registers 6`).

### The image lies below 2 GB

x32 code built without position independence keeps absolute addresses in
sign-extended 32-bit fields. So the image must end below 2 GB. It is linked
at 0x10000000 (`port/macos/guest/guest.ld`), not at Android's 0x88000000.
That leaves the Xbox memory window its full desktop size, 512 MB at
0x80000000 (Android's is 128 MB).

### The host claims the low 4 GB first

The host is linked with a 64 KB page zero instead of the usual 4 GB, so that
memory below 4 GB can be mapped at all. But then Rosetta and the system
frameworks map into that range too.

So the first thing the host does is reserve every free page below 4 GB
(`host_memory_claim` in `host_memory.c`). From then on it hands that space
out itself: the image, the Xbox window, thread stacks and the guest's
`mmap` calls. Nothing else can land below 4 GB. This replaces Android's
memory pools.

### Write faults arrive as SIGBUS

The renderer write-protects the memory behind cached textures and records
the first write to each page. Linux reports such a write as SIGSEGV. macOS
reports it as SIGBUS, with the exact address. The host handles both.

### Crashes end with _exit

Every other fault is a crash. The host logs the registers and the guest's
frame chain to `host.log` and ends the process with `_exit`, so the crash
does not reach the system's crash reporting. A crash report of a Rosetta
process has been seen to hang every later Rosetta launch until `oahd` was
restarted. Every thread the host starts has an alternate signal stack, so a
stack overflow is still reported. A fault the handlers cannot see (a stack
overflow on a thread of SDL or Cocoa, or a kill without a signal) still
reaches the system.

### The game runs on the main thread

Cocoa, under SDL, needs the process's main thread for windows and events.
Android runs the game on a second thread, but x32 code needs its stack below
4 GB and the main thread's stack is above. So the host moves the main thread
onto a 16 MB stack in guest memory (`host_stack.S`) and starts the game
there. SDL, Cocoa and OpenGL then run on that stack too.

### System calls are translated

The guest's musl is configured as on Android (`port/macos/guest/libc/arch/x32`
is Android's `arm64_32` with the CPU files swapped). It sends every system
call to the host with AArch64 Linux's numbers, flags and structures. No call
reaches a kernel, so these numbers are a private protocol between guest and
host.

Darwin numbers almost every constant differently. So `host_syscall.c`
performs each call with Darwin's C library and translates both ways
(`host_linux.c`):

- errno values, open flags, `*at` flags, clock ids, `mmap` flags, `fcntl`
  commands, signals and resource limits;
- the structures for `stat`, `getdents64`, `uname` and `sysinfo`;
- futexes, built on `os_sync_wait_on_address` (macOS 14.4 and later) or the
  private `__ulock` calls it is made of (earlier);
- absolute sleeps, which Darwin does not have.

The host never uses Darwin's `<sys/syscall.h>`, whose numbers mean other
calls. The guest's numbers are generated from its own list into
`guest_syscall_numbers.h`.

### Threads and thread ids

macOS lets no program set the `%fs` base that x86 code uses as its thread
pointer. So the guest's thread pointer stays a host import, as on Android,
and the guest uses emulated TLS (`-femulated-tls`). Guest thread ids are
small numbers from a host counter, as musl keeps them in 30 bits.

## Find problems

`host.log` in the data folder has the host's messages, the guest's standard
output and any crash report. A crash report names guest addresses to
symbolize with the image:

```
llvm-symbolizer --obj=build/macos/halo_guest.elf 0x10123456
```

`debug.sample_seconds` in `config.toml` makes the host interrupt every guest
thread that often and log where it is, as on Android. lldb under Rosetta does
not know the ELF guest, so it shows guest frames as bare addresses.

`tools/test_macos_host_linux.py` checks the system call layer. It builds
the translation tables and `host_syscall.c` for the Mac's own processor and
makes the guest's calls against Darwin, so it needs no Rosetta.

## Limits

- Apple silicon needs Rosetta 2, which Apple will restrict from macOS 28.
- x32 code runs about 1.4 times slower than ordinary x86-64 code under
  Rosetta, in a measured address-heavy loop. Intel Macs run it at full
  speed.
- OpenGL stops at 4.1 on macOS. The renderer uses its OpenGL ES code path,
  with a desktop profile, and never calls functions newer than 4.1.
- The app is signed ad hoc and not notarised. The hardened runtime would
  need the `com.apple.security.cs.allow-unsigned-executable-memory`
  entitlement, as the guest's code lives in anonymous memory.
- The self-updater is compiled out.
- Guest file offsets returned by `lseek` are 32-bit, as on Android.
