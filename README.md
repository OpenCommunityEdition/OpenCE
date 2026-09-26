Halo 1 decomp, ported to Linux
=============

This is a port of the decompilation of Halo: Combat Evolved build 2342 (`cachebeta.exe`, sha256 `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`) to Linux.

<img width="1284" height="989" alt="Main_Menu_Screenshot" src="https://github.com/user-attachments/assets/92e03c85-0d96-45f5-bdd9-8e69555c996d" />

This is based on [bnunu](https://github.com/bnunu/halo)'s decompilation project, which itself is a fork of [punpckhdq/halo](https://github.com/punpckhdq/halo).

## Agent quick references

- [Current campaign house rules and batch/treemap cadence](docs/campaign_house_rules.md)
- [Common constants, types, float patterns, tag IDs, and flag conventions](docs/common_constants.md)
- [Shared assertion macros and byte-matching examples](docs/assertions.md)
- [Supplied CE source maps, recovered names, and next reconstruction packets](docs/user_source_reconstruction_map_20260906.md)
- [Matching methodology and source-credibility rules](docs/matching_methodology.md)

## Build instructions

You must source the August 2001 Xbox SDK yourself, and you need Python and [ninja-build](https://ninja-build.org/) on your PATH. Extract the `XDK/xbox` folder from the installer into the repository root such that `xbox/{bin,include}` are valid paths, then run `configure.py` from the repository root.

### Native Linux build

`ninja linux` compiles the game with clang into a native 32-bit Linux executable, `build/linux/halo`. It needs clang, 32-bit glibc development files and 32-bit SDL3. It renders with OpenGL, plays sound through SDL3 audio, and takes keyboard, mouse and gamepad input. Put the PAL game data (build 01.01.14.2342) under `assets/` so that `assets/maps` exists, then run `build/linux/halo`. See [port/linux/README.md](port/linux/README.md) for controls and settings.

### Debug and release

The native builds (Linux, Windows, Android) are debug builds by default: like the build the decompilation reproduces, they stop at the first failed assertion and log it. `python configure.py --release` configures release builds instead, which, like the retail game, do not check assertions. The byte-matching build is unaffected.

### Frame rate

The native builds draw a frame at every refresh of the display (60, 90, 120, 240 Hz, ...), paced by vsync, while the game still simulates at 30 Hz as on the Xbox: each frame blends the last two ticks. To see the frame rate, open the developer console (the \` key) and enter `display_framerate true`; the frames per second, averaged over half a second, appear at the bottom right of the screen. `HALO_INTERPOLATION=0` restores the original 30 frames per second. See [port/linux/README.md](port/linux/README.md#frame-rate).

### Native Windows build

`ninja windows`, run on Windows, compiles the game with clang into a native 32-bit Windows executable, `build/windows/halo.exe` (with `SDL3.dll`), sharing the Linux build's platform layer. It needs LLVM, Python and ninja, plus Visual Studio's x86 C++ libraries and a Windows SDK. Put the game data under `assets/` as for Linux. See [port/windows/README.md](port/windows/README.md).

### Android build

`ninja android_apk` builds an arm64 Android app (`port/android/app/build/outputs/apk/debug/app-debug.apk`) that runs the game natively on 64-bit ARM phones, with OpenGL ES 3 rendering at the device's aspect ratio, SDL3 audio and game controller support (including a PS5 DualSense over Bluetooth). It needs the Android NDK and a clang with the `arm64_32` target in addition to the Linux build's requirements. The game data goes in the app's storage (the app offers to import it). See [port/android/README.md](port/android/README.md).

### Matching build

The byte-matching build also needs `cachebeta.exe` from the Halo 1 PAL debug build in the repository root. Run `ninja` to compile the game and report progress statistics. On Linux it runs the XDK compiler through [wibo](https://github.com/decompals/wibo) (downloaded automatically), assembles the CRT `.asm` units with UASM when no MASM is available, and builds csplit from source; see [port/linux/README.md](port/linux/README.md#the-matching-build-on-a-linux-host).

## Where's all the type information?

We use debug symbols from later Halo games to help map structures, enums, type definitions, function signatures and some variable names. You should obtain a copy of the Halo CEA beta and run it through [pdb-decompiler](https://github.com/camden-smallwood/pdb-decompiler) to obtain debug info for yourself. Note that CEA is based off of Halo PC and has its own modifications which make it not 100% accurate to the original Xbox title.
