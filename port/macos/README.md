# macOS

`ninja macos` builds the game for Macs with Apple silicon (arm64):
`build/macos/Halo.app`, and `build/macos/halo` to run from the build.

The game shows its graphics with OpenGL 4.1. It plays sound and reads the
keyboard, the mouse and game controllers through SDL3. The app needs macOS
13 or later.

The macOS build uses the platform layer of the Linux build
(`port/linux/src`) and the guest runtime of the Android build
(`port/android/guest`). Refer to [port/linux/README.md](../linux/README.md)
and [port/android/README.md](../android/README.md).

## Download

The releases of
[spacemandev-git/OpenCE](https://github.com/spacemandev-git/OpenCE/releases)
contain `halo-macos-release.zip` (to play) and `halo-macos-debug.zip`:

1. Unzip the file and move `Halo.app` to the Applications folder.
2. The app is not notarized by Apple, so macOS does not open the downloaded
   app. Enter `xattr -dr com.apple.quarantine /Applications/Halo.app` one
   time to permit it.
3. Start the app. At the first start, it asks for the disc image (refer to
   "Game data").

## Requirements

You do not need the Xbox SDK. You need the tools of the Linux build (Python,
ninja) and these items:

- The command line tools of Xcode (`xcode-select --install`). Their clang
  has the `arm64_32` target. The option `--macos-guest-cc` of
  `configure.py` selects a different compiler.
- `ld.lld`: `brew install lld`. `configure.py` looks for it on the `PATH`
  and in Homebrew. The option `--macos-lld` selects a different linker.
- CMake: `brew install cmake`.
- A network connection for the first build. `configure.py` downloads musl
  1.2.5, SDL 3.4.16 and the Khronos OpenGL ES headers to
  `build/macos/third_party`.

## Build and start the game

1. Go to the root folder of the repository.
2. Enter `python3 configure.py`.
3. Enter `ninja macos` (or `ninja`: on a Mac, it builds macOS).
4. Enter `open build/macos/Halo.app`, or copy the app to `/Applications`.

The build signs the app for this computer only (ad hoc). A Mac that gets
the app from somewhere else does not open it until you remove the
quarantine attribute: `xattr -dr com.apple.quarantine Halo.app`.

To start the game from the build, with its log in the terminal, enter
`build/macos/halo`.

## Game data

At the first start, the game asks for an Xbox disc image (`.xiso` or
`.iso`) of Halo: Combat Evolved, and extracts its `maps/` folder, as on
Linux. Refer to "Game data" in [README.md](../../README.md#game-data).

| Item | `Halo.app` | `build/macos/halo` |
| --- | --- | --- |
| `maps/`, `config.toml`, the log (`debug.txt`) | `~/Library/Application Support/Halo` | `build/macos` (or `<repository>/assets`, as on Linux) |
| Saved games (`z:\` and `u:\`) | `~/Library/Application Support/Halo/saves` | `~/Library/Application Support/Halo/saves` |

The settings are the settings of Linux. Refer to
[port/linux/README.md](../linux/README.md#settings). The controls are the
controls of Linux. On the Mac keyboard, F11 and F12 can need the Fn key.

## How the port operates

### The guest

The data of the game contains 32-bit pointers (refer to "ILP32 code" in
[port/android/README.md](../android/README.md#ilp32-code)). macOS cannot
execute 32-bit code. Thus, as on Android, the game is ILP32 AArch64 code:
the guest image, `build/macos/halo_guest.elf`. `tools/guest_build.py`
builds it for the two ports.

On Android, the guest image is at its addresses below 4 GB. A macOS process
for Apple silicon cannot map memory below 4 GB: the kernel gives every
process a page zero of 4 GB. Thus the 4 GB of the guest are at a different
address, the base, which is a multiple of 4 GB. The guest address A is the
host address base + A.

`tools/guest_asm_rebase.py` changes the assembly of the guest after
`tools/android_asm_convert.py`:

- The register x28 holds the base. x27 is a scratch register. The compiler
  does not use the two registers (`-ffixed-x27 -ffixed-x28`).
- A load or a store through a register r goes to base + (the low 32 bits
  of r): `ldr w0, [x1]` becomes `ldr w0, [x28, w1, uxtw]`. An access with
  an offset, a pair, an exclusive access or a vector structure first adds
  the base into x27.
- A branch through a register goes to base + (the low 32 bits of the
  register).
- sp and x29 hold host addresses in the 4 GB of the guest. Because the base
  is a multiple of 4 GB, their low 32 bits are guest addresses. A value
  that comes from sp, from x29, from `adrp` or from `adr` is zero-extended,
  thus the other registers hold only guest addresses.
- Jump tables have 32-bit entries
  (`-aarch64-enable-compress-jump-tables=false`), because the changed code
  is longer.

The guest has approximately 18% more instructions than on Android.

### The host

`build/macos/halo` is an arm64 macOS executable (`port/macos/host`). The
host:

- Reserves the 4 GB of the guest, and puts the Xbox memory at guest address
  `0x80000000` and the image at `0x88000000` (`host_memory.c`).
- Keeps the 4 KB pages of the guest on the 16 KB pages of Apple silicon. A
  host page is mapped while one or more of its four guest pages is in use.
  Mappings that the guest does not place get whole host pages.
- Loads the image and fills its import table (`host_loader.c`).
- Starts the `main` of the game on the first thread of the process, which
  Cocoa needs for the windows and the events, on a stack in guest memory.
  Each guest thread also gets its stack in guest memory (`host_thread.c`).
  `host_entry.S` sets x28 when the host calls the guest.
- Does the Linux system calls of the guest with the calls of macOS
  (`host_syscall.c`). It changes the numbers of the flags and the errors,
  and the structures. Futexes are `__ulock_wait` and `__ulock_wake`.
- Adds the base to the pointers that the guest gives to SDL (`host_sdl.c`),
  OpenGL and the `posix_*` functions of `port/linux/src/posix.h`. Generated
  wrappers do the OpenGL and `posix_*` functions
  (`tools/macos_host_wrappers.py`).

### OpenGL

macOS has OpenGL 4.1, not the OpenGL 4.5 of the Linux build. The renderer
uses its OpenGL ES 3 path (`HALO_GLES`), as on Android, on an OpenGL 4.1
core context, with `#version 410 core` shaders. OpenGL 4.1 gives base vertex
draws and border colors. The window, the mouse in the menus, the display
settings and the anti-aliasing are those of the desktop.

### Changes to the shared sources

- `HALO_GUEST`: the code of the ILP32 guest of the Android and macOS ports
  (the `bss_seg` pragmas, the stack walker, `_control87`).
- `HALO_GLES`: the OpenGL ES 3 path of the renderer, for the Android and
  macOS ports.
- `HALO_MACOS`: the OpenGL 4.1 context and capabilities, the folder of the
  saved games, and no self-updater.
- `posix_net.c`: macOS has no `SOCK_CLOEXEC`, `accept4` or `MSG_NOSIGNAL`,
  and its socket addresses start with a length byte.

## Find problems

- Start `build/macos/halo` in a terminal. The log of the host starts with
  `halo-macos:`, the log of the platform layer with `halo-linux:`.
  `debug.txt` is the log of the game.
- If the guest stops, the log shows the registers, the frame chain and the
  guest address of the program counter. To find the functions, enter
  `llvm-symbolizer --obj=build/macos/halo_guest.elf <address>`, or look for
  the address in `build/macos/halo_guest.elf.map`.
- `python3 -m pytest tools/test_macos_port.py` tests the changes to the
  assembly, runs C code through the guest pipeline at a base, and starts
  the game from the build without a window.

## Limits

- Bink video is not available. The game skips the movies.
- The game does not update itself.
- An invite link (`halo://join/...`) does not open the game. Copy the link
  and go to the game.
- Macs with Intel processors cannot run the game.
