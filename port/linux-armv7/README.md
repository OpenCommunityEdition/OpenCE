# Linux armv7

`ninja linux_armv7` builds the game for 32-bit ARM Linux and
`build/linux-armv7/halo` runs it.

## What it is

The game as native 32-bit ARM (AArch32) code, in one process: there is no
host and guest. It is the same 32-bit game as the x86 Linux build, compiled
for ARMv7-A with hard-float VFPv3-D16 (`armv7-linux-gnueabihf`, which Debian
calls armhf and `uname -m` calls `armv7l`) with clang and lld. On a 64-bit
ARM CPU it runs in the CPU's 32-bit mode (AArch32). It uses the OpenGL ES
renderer path (`HALO_GLES`) that the Android and web builds already use:
OpenGL ES 3.2, with a fallback to 3.0, requested from the platform layer.
SDL3 comes from the system root.

This port is built from the x86 Linux port. The game's sources and the
platform layer (`port/linux/src`) are shared with the other Linux builds;
refer to [port/linux/README.md](../linux/README.md). The crash report
records the ARM registers.

## Building

It needs clang, lld, ninja, Python and a root of armhf libraries.

1. Go to the root folder of the repository.
2. Enter `python3 configure.py --linux-arch armv7 --linux-sysroot DIR --lto=off --pgo=off`.
3. Enter `ninja linux_armv7`.

The build writes `build/linux-armv7/halo`.

`DIR` is an armhf Debian root with `libc6-dev`, `libgcc-14-dev`,
`libgles-dev`, and the headers and libraries of SDL 3.4. SDL 3.2 from Debian
trixie may lack calls the game uses. Without `--linux-sysroot` the build
uses the machine's own libraries, which suits an armhf machine.

`tools/linux_armv7.cmake` is a toolchain file for building SDL3 for this
target.

## Floating point

The game sets and reads the rounding mode and the exception flags through
`_controlfp`, `_statusfp` and `_clearfp` (`port/linux/src/msvc_crt.c`). On
armv7 these are the VFP unit's: the rounding mode is RMode in FPSCR, the
flags are its sticky exception bits. Precision control and exception
unmasking have no equivalent on ARM; the rest of the MSVC control word is only
remembered. AArch64 has the same bits in FPCR and FPSR and shares the code.

## Alignment

32-bit ARM loads and stores a single word at any address, but faults (a bus
error, `SIGBUS`) on a multiple-register load or store, and on a floating-point
load, at an address that is not a multiple of 4. x86 and AArch64 take those in
their stride, and the game's data has places where a float or a structure of
floats sits on a multiple of 2: a byte stream in a tag, or the scripts' stack.
So on this build:

- the scripts' stack frames start their data on a multiple of 4, and every
  allocation from it is a multiple of 4 (`hs_stack_frame` and
  `hs_stack_allocate` in `source/hs/hs_runtime.c`, under `HALO_LINUX_ARMV7`:
  the other builds keep the Xbox's layout);
- a compressed animation's keyframes and defaults are read through `memcpy`
  (`source/models/model_animations.c`).

The crash report of a fault has the program counter and the address:
`llvm-symbolizer` on the build's `halo` names the function. The game's own
halt screen lists the call chain as addresses, which it decodes the same way.

## Custom Edition tag cache

Custom Edition maps need 23 MB at the fixed address 0x40440000. On 32-bit ARM
Linux the dynamic loader maps the shared libraries from 0x40000000 upwards in
the legacy memory layout (and qemu-user always does), on top of that window,
so a reservation at start-up fails. The armv7 executable therefore carries the
window as a segment of its own, which the kernel maps before the loader runs
(`tools/linux_armv7_tag_cache.ld`; `port/linux/src/xbox_memory.c` uses it).

## Running on a 64-bit ARM computer

To run the game on a 64-bit ARM computer that has the 32-bit mode but no armhf
libraries, use an armhf root: for example a Debian 13 armhf container
(`podman run --platform linux/arm/v7 debian:trixie`) with the libraries named
under "Building" and `libwayland-client0`, `libwayland-egl1`,
`libxkbcommon0` and Mesa's drivers (`libgl1-mesa-dri`, `libegl-mesa0`). Start
`build/linux-armv7/halo` in the container, with the container's libraries, the
Wayland socket and `/dev/dri`.

SDL3 is built with Wayland and X11 support (the build needs
`wayland-scanner` on the build computer, and `wayland-protocols` and
`libdecor` in the armhf root: without them SDL leaves Wayland out and the game
needs XWayland).

## Limits

- No `--portable` build.
- No PGO profile yet, so use `--pgo=off`.
- Run on a 64-bit ARM CPU in its 32-bit mode (a Snapdragon 8 Gen 2, in an
  armhf container, with Wayland and the Adreno GPU: OpenGL ES 3.2) and under
  qemu-user; not on a CPU that is 32-bit only (a Raspberry Pi 3 or 4 with a
  32-bit system, for example). A build made in the container loaded and drew
  all 23 maps (the 10 campaign levels and the 13 multiplayer maps) twice in
  one run, with no fault; it ran at 88 to 120 frames a second against the
  arm64 build's 102 to 120 on the same maps, since only the cores with a
  32-bit mode (the Cortex-A510 and A710 there) can run it. A controller
  worked in play.
- The game needs a GPU with OpenGL ES 3.0 or later.
- Custom Edition maps are not tested on this build.

## Tests

Run this with pytest from the `tools` folder:

- `test_linux_armv7_build.py`: the build graph of the armv7 target.

`tools/ci_smoke.py` starts the built game headless under `qemu-arm-static`, in
the `linux-armv7` job, and checks its start-up log, among it that the Custom
Edition window was reserved.

`port/linux-armv7/tests/fenv_test.c` checks the floating-point control (the
rounding modes and the exception flags) on the VFP unit. The `linux-armv7`
job of the workflow links it with the build's `msvc_crt.o` and runs it with
`qemu-arm-static`; it also passes natively on a Snapdragon 8 Gen 2 (all 20
checks, where the older code that only remembered the control word fails 10). To run it yourself, build `msvc_crt.o` with `ninja`, then
link the test statically with it
(`-static -Wl,-z,muldefs -Wl,--unresolved-symbols=ignore-all`) and run it on
an armv7 machine or with `qemu-arm-static`.
