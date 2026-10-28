# OpenCE for macOS

The Mac build is `OpenCE.app`: the same game, platform layer, settings, menus,
networking and renderer as the other ports, for Intel and Apple Silicon Macs
(one universal app).

## Building

```sh
brew install llvm@20 lld@20 ninja cmake
python configure.py --macos
ninja macos                    # build/macos/OpenCE.app
```

The guests are built with Homebrew's LLVM 20 and lld, and Apple Silicon's
with a pass plugin built against it (`compiler/guest_rebase.cpp`). The first
`configure.py --macos` downloads musl, SDL3 and ANGLE, OpenGL ES on Metal, into
`build/macos/third_party`
(kivy/angle-builder's universal build of Chromium 7151's ANGLE, BSD-3-Clause);
each is pinned by its SHA-256 or commit in `tools/macos_build.py`.

## Running

Open `build/macos/OpenCE.app`, or run `build/macos/OpenCE.app/Contents/MacOS/halo`
from a terminal to see its log. The game's folder is
`~/Library/Application Support/OpenCE`: `config.toml`, `debug.txt`, the maps
(the first start offers to extract them from an Xbox disc image, as the Linux
and Windows builds do) and, under `halo-linux`, the saves.

The Mac build has no updater and no dedicated server, and plays stereo.

## How it operates

macOS runs no 32-bit processes, and the game needs 32-bit pointers, so a
guest image holds the game, the platform layer and musl, compiled with
32-bit pointers, and a 64-bit host, `halo`, loads it and serves it. The app holds a guest for each Mac and a universal host.

- Intel: `halo_guest_x86_64.elf` is x32 code (x86-64 with 32-bit pointers,
  `x86_64-linux-gnux32`), and the host's Intel half is linked with a 4 KB
  `__PAGEZERO`, so that the guest runs at its own addresses below 4 GB.
- Apple Silicon: an arm64 process cannot map the low 4 GB. `halo_guest_arm64.elf`
  is arm64_32 code, whose compiler pass
  (`compiler/guest_rebase.cpp`) moves every address it uses 1 TB up
  (`HALO_GUEST_BIAS`), where the host has the guest's memory.
- At start the host reserves the guest's 0x40000000 to 4 GB
  (`host/host_memory.c`): the Custom Edition tags at 0x40440000, the image at
  0x70000000 (x32 code reaches its symbols in the low 2 GB only), the Xbox
  window at 0x80000000 and the guest's own pages above it. Apple Silicon's
  pages are 16 KB, the guest's 4 KB: a host page a guest mapping covers in
  part keeps its neighbours' data.
- The guest's runtime (`guest/runtime`) starts it, runs its threads and
  passes its SDL calls to the host; `include/halo_guest_abi.h` is the layout
  both halves share. Its musl (with `guest/libc` for arm64_32) makes Linux's
  system calls, which the host performs with Darwin's (`host/host_syscall.c`).
- The host serves SDL (`host/host_sdl.c`, with the desktop's calls bridged by
  `guest/guest_desktop.c`), OpenGL ES (`host/host_gl.c`), HTTPS
  (`host/host_https.c`) and the platform layer's `posix_*.c`.
- The game's main thread is the process's (Cocoa needs it), moved onto a
  stack in guest memory.

A crash in the guest is logged with its address in the image:
`llvm-symbolizer --obj=build/macos/halo_guest_<arch>.elf <address>`.
