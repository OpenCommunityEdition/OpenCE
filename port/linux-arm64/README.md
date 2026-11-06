# Linux arm64

`ninja linux_arm64_guest linux_arm64_host` builds the game for 64-bit ARM
Linux and `build/linux-arm64/halo_host` runs it.

## What it is

An AArch64 host with an ILP32 guest. The guest is the game as ILP32 AArch64
code (64-bit ARM instructions with the 32-bit pointers the game's data
needs), the same image that the Android build runs. The host, `halo_host`, is
a native 64-bit program that loads it and gives it what it needs: the
system's SDL3 for sound, input and windows, and the system's OpenGL ES for
graphics.

This port is built from the Android port. The guest image is Android's, and
the host is the code in `host/` here, together with the host sources that
the Android build already uses. The game's own sources, and the platform
layer of the Linux build (`port/linux/src`), are shared with the other
builds; refer to [port/linux/README.md](../linux/README.md).

## Building

The host builds on Linux arm64 only. It needs a C compiler, ninja, Python,
pkg-config and the development packages of SDL3, OpenGL ES and EGL
(`sdl3`, `glesv2` and `egl` to pkg-config).

1. Go to the root folder of the repository.
2. Enter `python3 configure.py --guest-lld-bin /usr/bin`.
3. Enter `ninja linux_arm64_guest linux_arm64_host`.

The build writes `build/linux-arm64/halo_host` and
`build/linux-arm64/halo_guest.elf`. Copy them together.

## Running

Enter `halo_host [DATA_DIR]`. The data root is the first argument if there
is one, else `HALO_DATA_ROOT`, else `$HOME/.local/share/halo`. Saved games
go in `save/` under it. The settings file (`config.toml`) and the broker
list are kept there too.

`HALO_GUEST_ELF` selects a guest image other than the default, which is the
`halo_guest.elf` next to the executable.

A first start without a `maps/` folder asks for an Xbox disc image, the
same flow as the x86 build.

## Tests

Run these with pytest from the `tools` folder:

- `test_linux_arm64_build.py`: the build graph of the guest and the host.
- `test_host_boot.py`: the host's boot helpers.
- `test_host_page_size.py`: the page size handling, Linux only.
- `test_linux_arm64_host.py`: a start-up smoke test, on Linux arm64 with
  the host built.

## CI

`tools/ci_smoke.py` starts the host in CI with no window or game data and
checks that it loads and quits at the missing data.
