Halo 1 xbox decomp, ported to Linux, Windows and Android 
=============

This is a port of the decompilation of Halo: Combat Evolved build 2342 (`cachebeta.exe`, sha256 `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`) to Linux, Windows and Android.

<img width="1289" height="995" alt="image" src="https://github.com/user-attachments/assets/0d3ad50f-f8b8-46cf-aef8-e3661da2a7d7" />

This is based on [bnunu](https://github.com/bnunu/halo)'s decompilation project, which itself is a fork of [punpckhdq/halo](https://github.com/punpckhdq/halo).

## Agent quick references

- [Current campaign house rules and batch/treemap cadence](docs/campaign_house_rules.md)
- [Common constants, types, float patterns, tag IDs, and flag conventions](docs/common_constants.md)
- [Shared assertion macros and byte-matching examples](docs/assertions.md)
- [Supplied CE source maps, recovered names, and next reconstruction packets](docs/user_source_reconstruction_map_20260906.md)
- [Matching methodology and source-credibility rules](docs/matching_methodology.md)

## Build instructions

You need Python and [ninja-build](https://ninja-build.org/) on your PATH; run `configure.py` from the repository root, then `ninja` with the target below (plain `ninja` builds the one for the computer you are on). No part of the Xbox SDK is needed: the SDK declarations the game uses are in [port/include/xdk](port/include/xdk/README.md).

Every pushed commit is built by GitHub Actions ([.github/workflows/build.yml](.github/workflows/build.yml)): debug and release builds for Linux, Windows and Android, made by `tools/ci_build.py` (which also works locally, e.g. `python tools/ci_build.py linux release`) and kept as artifacts for three days.

### Native Linux build

`ninja linux` compiles the game with clang into a native 32-bit Linux executable, `build/linux/halo`. It needs clang, 32-bit glibc development files and 32-bit SDL3. It renders with OpenGL, plays sound through SDL3 audio, and takes keyboard, mouse and gamepad input. Put the PAL game data (build 01.01.14.2342) under `assets/` so that `assets/maps` exists, then run `build/linux/halo`. See [port/linux/README.md](port/linux/README.md) for controls and settings.

### Debug and release

The native builds (Linux, Windows, Android) are debug builds by default: like the build the decompilation reproduces, they stop at the first failed assertion and log it. `python configure.py --release` configures release builds instead, which, like the retail game, do not check assertions. The byte-matching build is unaffected.

### Optimisation

The Linux and Windows builds are optimised for the processor of the computer that builds them (`-march=native`), and may not start on another. `python configure.py --portable` targets the x86-64 baseline instead (SSE2, which every 64-bit x86 processor has): use it for builds you share.

They also use full link-time optimisation by default, which makes the final link take a minute or so; `--lto=thin` links in parallel and incrementally, `--lto=off` not at all.

They are also optimised with profiles of the game at play, recorded by an instrumented build playing the main menu and the opening minute of every campaign level by itself: `pgo/halo_linux.profdata` for Linux (and Android, whose code is almost the same) and `pgo/halo_windows.profdata` for Windows. The profiles need clang 22 or later; with an older clang, and with `--pgo=off`, the builds do without. A profile stays useful as the code changes (functions it does not know simply go without). To record a new one, delete it and configure with `--pgo=train`: `ninja linux` or `ninja windows` then builds the instrumented game and plays the levels (about fifteen minutes, in a window, silently; it needs the game data in `assets/`) before the real build. Over ssh, Windows plays them on the logged-in user's desktop, through a scheduled task. `--pgo-profile <file>` uses another profile.

With Mesa drivers the Linux build makes its GL calls through Mesa's GL thread (`mesa_glthread`), which takes them off the game's thread.

Frames per second at the opening of a30, uncapped (`vsync = false` in `config.toml`, or `HALO_NO_VSYNC=1`), about 510 draws per frame; each row adds one change to the one above (Linux: a laptop with an Intel Core i7-1355U and Iris Xe graphics, median of three runs; Android: a Pixel 9 Pro XL, Tensor G4, median of three runs):

| Change | Linux | Android |
| --- | ---: | ---: |
| Original | 103 | 132 |
| GL state set only when it changes | 139 | 156 |
| Vertex and index buffers from a GL copy of the Xbox memory | 171 | 193 |
| Full link-time optimisation | 189 | not applicable |
| Profile-guided optimisation | 201 | 197 |
| `-march=native` (`--portable`, x86-64: 212; Android: `-mcpu=cortex-x3`, not used) | 224 | 191 |
| Mesa's GL thread | 257 | not applicable |
| Sound obstruction tested once per game tick, cheaper per-draw bookkeeping | 303 | 220 |

Android has no equivalent of `-march=native`: the build runs on any 64-bit phone, and compiling for the Pixel's big cores made it slower there anyway, so its last row builds on the profile-guided one. On the laptop the game's own thread no longer sets the frame rate: it spends about as long waiting for the GL and driver threads and the GPU (which the laptop's power management keeps at a low clock for this load) as they spend waiting for it. The same holds in heavier scenes such as b30, at about 190.

Unity ("jumbo") builds, which compile many files as one, would give the compiler nothing full link-time optimisation does not already see, and the decompiled sources declare too many conflicting local types for it anyway. `-O3` and profile-driven function splitting measured no faster than `-O2`.

### Frame rate

The native builds draw a frame at every refresh of the display (60, 90, 120, 240 Hz, ...), paced by vsync, while the game still simulates at 30 Hz as on the Xbox: each frame blends the last two ticks. To see the frame rate, open the developer console (the \` key) and enter `display_framerate true`; the frames per second, averaged over half a second, appear at the bottom right of the screen. `interpolation = false` in `config.toml` (written next to the executable, or in the data folder on Android, on the first run) restores the original 30 frames per second. See [port/linux/README.md](port/linux/README.md#frame-rate).

### Native Windows build

`ninja windows`, run on Windows, compiles the game with clang into a native 32-bit Windows executable, `build/windows/halo.exe` (with `SDL3.dll`), sharing the Linux build's platform layer. It needs LLVM, Python and ninja, plus Visual Studio's x86 C++ libraries and a Windows SDK. Put the game data under `assets/` as for Linux. See [port/windows/README.md](port/windows/README.md).

To install it on a PC without setting any of this up, run `port/windows/launcher/HaloLauncher.cmd`: you choose your game data, and it downloads, checks and builds the rest, then starts the game. See [port/windows/launcher/README.md](port/windows/launcher/README.md).

### Android build

`ninja android_apk` builds an arm64 Android app (`port/android/app/build/outputs/apk/debug/app-debug.apk`) that runs the game natively on 64-bit ARM phones, with OpenGL ES 3 rendering at the device's aspect ratio, SDL3 audio and game controller support (including a PS5 DualSense over Bluetooth). It needs the Android NDK and a clang with the `arm64_32` target in addition to the Linux build's requirements. The game data goes in the app's storage (the app offers to import it). See [port/android/README.md](port/android/README.md).

### Matching build

This fork builds only the native ports. The upstream project's byte-matching build, which compiles the game with the Xbox SDK's own compiler and compares it with `cachebeta.exe`, needs the August 2001 Xbox SDK, which cannot be redistributed, so `configure.py` no longer writes it. Its sources, configuration and `#ifdef`s are kept as they are, so that upstream's matching work still merges; `SolutionConfig.matching` in `tools/project_x86.py` turns it back on, for a checkout with the SDK's `XDK/xbox` folder extracted to `xbox/` and `cachebeta.exe` in the repository root. See [port/linux/README.md](port/linux/README.md#the-matching-build-on-a-linux-host) for running it on Linux.

## Where's all the type information?

We use debug symbols from later Halo games to help map structures, enums, type definitions, function signatures and some variable names. You should obtain a copy of the Halo CEA beta and run it through [pdb-decompiler](https://github.com/camden-smallwood/pdb-decompiler) to obtain debug info for yourself. Note that CEA is based off of Halo PC and has its own modifications which make it not 100% accurate to the original Xbox title.
