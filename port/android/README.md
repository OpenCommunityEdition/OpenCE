# Android build

`ninja android` builds the decompiled game for 64-bit ARM Android
(arm64-v8a), and `ninja android_apk` packages it as an app,
`port/android/app/build/outputs/apk/debug/app-debug.apk`. It renders with
OpenGL ES 3, plays sound through SDL3 (AAudio), and takes input from game
controllers, including a PlayStation 5 DualSense over Bluetooth. It targets
Android 9 (API 28) and later, and is tested against 64-bit-only devices
such as the Pixel 9 Pro XL (Tensor G4, Mali-G715).

Like the Linux build (`port/linux/README.md`), it is a separate graph from
the byte-matching build, and it shares the Linux port's platform layer
(`port/linux/src`): the Xbox SDK implemented over SDL3, OpenGL and POSIX.

## Building

Requirements, in addition to what the Linux build needs (the XDK headers in
`xbox/include`, Python, ninja):

- a clang with the `arm64_32` target (any recent LLVM, e.g. the system
  clang; `--android-guest-cc` selects another);
- the Android NDK (found through `ANDROID_NDK_HOME`, or the newest under
  `$ANDROID_HOME/ndk`, `~/Android/Sdk/ndk` or `/opt/android-sdk/ndk`;
  `--android-ndk` selects one), CMake and a JDK 17+ for Gradle;
- network access the first time: `configure.py` downloads musl 1.2.5 and
  clones SDL 3.4.16 into `build/android/third_party`, and Gradle fetches the
  Android Gradle Plugin.

```sh
python configure.py
ninja android          # the game image and native libraries
ninja android_apk      # the APK (runs Gradle in port/android)
adb install -r port/android/app/build/outputs/apk/debug/app-debug.apk
```

## Game data

The game needs the PAL game data of this build (01.01.14.2342), the folder
that holds `maps/`, exactly as the Linux build does. The app keeps it in its
external files directory, `/sdcard/Android/data/com.halo.decomp/files`.

- On first launch the app shows a screen with a button that opens the
  system folder picker: choose the folder that contains `maps` (or `maps`
  itself) and the app copies it (about 1.8 GB).
- Or, with the app installed and started once (so that it creates its
  directories), push it from a computer:
  `adb push <folder>/. /sdcard/Android/data/com.halo.decomp/files/`

Saves go to `files/save` (`z:\` and `u:\` of the Xbox, as on Linux), and the
game's log to `files/debug.txt`; the app keeps them readable over adb
(`adb pull /sdcard/Android/data/com.halo.decomp/files/save` backs them up).

## Controls

Controllers are read through SDL3's gamepad API, so any controller Android
recognises works; the first recognised one is player 1, further ones
players 2-4 (split screen). Buttons map by position to the Xbox controller:

| DualSense | Xbox | Halo (default layout) |
| --- | --- | --- |
| left stick / right stick | left stick / right stick | move / look |
| R2 | right trigger | fire |
| L2 | left trigger | throw grenade |
| Cross | A | jump, accept |
| Circle | B | melee, back |
| Square | X | action, reload |
| Triangle | Y | switch weapon |
| L1 | white | flashlight |
| R1 | black | switch grenade |
| L3 / R3 | left / right stick click | crouch / zoom |
| D-pad | D-pad | |
| Options | Start | pause menu |
| Create | Back | |

Rumble is passed to the controller. The Android back gesture acts as B,
and a Bluetooth or USB keyboard works as described in the Linux README.

## Settings

A text file `files/halo.env` (next to `maps`) sets any of the port's
environment variables, one `NAME=value` per line: the `HALO_*` settings of
the Linux README (volume, language, renderer debugging), plus:

| Variable | Effect |
| --- | --- |
| `HALO_SCREEN_WIDTH` | columns of the 480-line picture; by default the display's aspect ratio (1068 on a 20:9 phone), `640` for the Xbox's 4:3 |
| `HALO_INTERPOLATION` | `0`: the original 30 frames per second instead of one per display refresh (port/linux/README.md, "Frame rate") |
| `HALO_SAMPLE` | see Debugging |

## Widescreen

The game renders 480 lines at the display's aspect ratio instead of the
Xbox's 640x480. Its camera derives the horizontal field of view from the
viewport's shape with a fixed vertical one, so the 3D view simply widens
("Hor+"). The HUD anchors to the title-safe frame, which widens with the
screen; the menus, the loading bar and the post-game screens are laid out
for 640 columns and are drawn centered (the vertex shaders shift them,
`halo_android_ui_offset`); chapter titles keep their place relative to the
screen's sides; letterbox bars and fades cover the whole width. The
changes are in `rasterizer_xbox.c`, `render.c`, `ui_widget.c`,
`cinematics.c`, `main.c` and `rasterizer_xbox_screen_effect.c`, under
`#ifdef HALO_ANDROID`.

## How it works

### Why the game is not an ordinary arm64 library

The game's data formats embed 32-bit pointers: cache files are laid out for
the Xbox's address space, game state is saved as a memory image, and
Direct3D resources carry 32-bit physical addresses. The Linux build
therefore compiles the game as 32-bit x86. Android devices no longer run
32-bit ARM code (the Tensor G4 and other current SoCs have no AArch32 at
all), and compiling the game for 64-bit pointers would change the layout of
every structure it reads from disk.

So the game runs as **ILP32 AArch64 code**: native 64-bit ARM instructions
with 32-bit `int`, `long` and pointers, inside an ordinary 64-bit app.

### The guest image

The game, the platform layer shared with Linux, and a small runtime are the
*guest*. Clang offers ILP32 AArch64 only as Apple's `arm64_32` watchOS
target, so the guest is compiled for `arm64_32-apple-watchos` with the
Darwin environment hidden (`-U__APPLE__`, `-fno-define-target-os-macros`),
to assembly; `tools/android_asm_convert.py` rewrites that Mach-O assembly as
ELF assembly (symbol decoration, sections, `@PAGE`/`@PAGEOFF` relocations,
GOT loads relaxed to direct addresses), which the ordinary AArch64 assembler
turns into objects. `ld.lld` links them with `guest/guest.ld` into a static
image, `build/android/halo_guest.elf`, at a fixed address just above the
Xbox memory window (`include/halo_android_abi.h`). The APK carries it as an
asset.

Its C library is a subset of musl built for a new `arm64_32` arch
(`guest/libc/arch/arm64_32`): ILP32 types, a 32-bit `time_t` as in the MSVC
runtime the game was written against, and a system call layer
(`syscall_arch.h`) that forwards every call to the host. musl's futex-based
locks, condition variables, stdio and malloc are used unchanged; thread
creation, the thread pointer and clang's emulated TLS are in
`guest/runtime/guest_thread.c`.

### The host library

`libmain.so` is an ordinary arm64 NDK library started by SDL3's activity
(`app/.../HaloActivity.java`). It

- reserves the guest's address space below 4 GB: the Xbox window at
  0x80000000, the image's range, and pools for the guest's mappings, which
  it claims above the image first because ART keeps its own low-4 GB heaps
  at the bottom of the address space (`host/host_memory.c`);
- loads the image and fills its import table (`host/host_loader.c`);
- runs the game's `main`, and every guest thread, on a thread whose stack
  is in guest memory, since ILP32 code keeps stack addresses in 32-bit
  registers (`host/host_thread.c`); the stack is given to `pthread_create`,
  so it is also the stack ART knows, and SDL may call into Java from it;
- hands SDL's audio callback, which SDL calls on its own thread, to a
  thread with a guest stack that runs the game's callback
  (`host/host_sdl.c`);
- serves the guest's calls: system calls, converting the few structures
  whose layout differs and keeping mappings below 4 GB
  (`host/host_syscall.c`); SDL, whose objects become small handles
  (`host/host_sdl.c`); OpenGL ES (`host/host_gl.c`); and the file system
  and socket helpers of `port/linux/src/posix_*.c`, compiled into the host.

The guest calls the host through stubs (`tools/android_imports.py`) that
branch through a table of host function pointers; the two ABIs agree on
register use for 32-bit integers, floats and pointers, which arm64_32
passes zero-extended. The OpenGL ES entry points are generated from the list
in `port/linux/src/gl.h` (`tools/android_gl_stubs.py`), widening
`GLsizeiptr` arguments, stack-passed arguments and the string array of
`glShaderSource`. The `posix_*` helpers get wrappers that copy the host's
`errno` back (`tools/android_posix_stubs.py`).

### OpenGL ES

The renderer (`port/linux/src/d3d8_gl.c` and the NV2A shader translators)
targets OpenGL ES 3.0 with optional 3.2 features under `HALO_ANDROID`:

- clip control is emulated in the generated vertex shaders (y flipped and
  depth remapped from 0..1), with the front-face winding inverted to match;
- BGRA texels are uploaded as RGBA with a texture swizzle, and DXT textures
  are decoded on the CPU when the driver lacks S3TC (Mali GPUs do);
- the sampler LOD bias (used by water ripples) is applied in the pixel
  shaders;
- `D3DCOLOR` vertex attributes are byte-swapped on upload;
- vertex and index data stream into a ring of three buffers, one per frame
  in flight, with unsynchronized mapped writes: Mali copies a whole buffer
  for every `glBufferSubData` into one that queued draws still use, and
  orphaning a large buffer each frame costs as much, which exhausted the
  phone's memory within seconds;
- indexed draws are rebased on the CPU instead of using base-vertex draws;
- border clamping, anisotropy and image copies are used where available;
- visibility tests (lens flare occlusion) count samples with a fragment
  shader atomic counter on OpenGL ES 3.1 and later, as the NV2A did; ES 3.0
  only reports whether any sample passed, which reads as fully visible.

### Calling convention hazards

The decompiled sources sometimes declare a function differently in the file
that calls it than where it is defined, or call it with no prototype at
all. Under 32-bit x86 every argument is a stack slot and that goes
unnoticed; under the guest's convention (Darwin's arm64) floating-point
arguments travel in their own registers and variadic arguments on the
stack, so such a call passes garbage. `tools/android_abi_check.py` compares
every declaration with its definition in the LLVM IR of the sources
compiled with the guest's flags; the unsafe ones are fixed: `hs.c` declared
the script fades' red component as a `long` (the cause of a garbage fade
colour at the end of a30's intro), and a dozen files call `error`,
`console_printf` or `terminal_printf` without a prototype
(`include/halo_android_variadic_prototypes.h` is force-included into them).
The same target also turns some libm calls into Darwin library functions
(`__sincos_stret`, `__exp10f`), which `guest/runtime/guest_misc.c`
provides. The guest is compiled without floating-point contraction, as the
x86 original had no fused multiply-add.

### Game source changes

The game's x86 inline assembly has C equivalents under `#ifdef HALO_LINUX`,
shared by all native ports ([port/linux/README.md](../linux/README.md#game-source-edits)),
and so has the XDK `winnt.h`'s (`tools/linux_sdk_overlay.py`). Under
`#ifdef HALO_ANDROID` are the seven `#pragma bss_seg(".bss")` lines
Darwin's section syntax rejects, and a stack walker for the assertion
handler that follows AArch64 frame records, so an assertion's log
(`debug.txt`) lists the call sites. The MSVC build defines neither.

The guest's musl uses its generic C math rather than the AArch64 inline
assembly versions, leaving the choice of instructions to the compiler. The
only assembly the port itself contains is necessary: the generated import
stubs through which the guest calls the host (a 32-bit guest cannot hold
or branch to a 64-bit host address), and the symbol aliases in
`guest/libc/src_include/features.h` (the Darwin target rejects alias
attributes).

## Debugging

- `adb logcat -s halo` shows the port's log and the game's error output;
  `files/debug.txt` is the game's own log.
- A crash in guest code is logged with its registers and frame chain;
  `llvm-symbolizer --obj=build/android/halo_guest.elf <address>` names the
  functions.
- `HALO_SAMPLE=<seconds>` in `halo.env` logs every guest thread's program
  counter and frame chain that often, which finds hangs on devices without
  root.
- `HALO_GL_DEBUG=1` reports OpenGL ES errors around draws.

## Known limitations

- Bink video is not supported (as on Linux): the intro movies are skipped.
- The device must allow the fixed guest address ranges (0x80000000 up to
  about 0x89000000) to be reserved; the app reports it if they are taken.
- Screen touches do nothing: the game is played with a controller (or a
  keyboard).
- Kernels with 16 KB pages (an Android 15 developer option) are not
  supported: the Xbox memory emulation works in 4 KB pages.
