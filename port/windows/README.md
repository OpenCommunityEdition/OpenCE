# Native Windows build

`ninja windows` compiles the decompiled game with clang for 32-bit x86
Windows and links a native executable, `build/windows/halo.exe`, with
`SDL3.dll` next to it. It is built on Windows, for Windows. Like the Linux
build (`port/linux/README.md`), it is a separate graph from the
byte-matching build, and it shares the Linux port's platform layer
(`port/linux/src`): the Xbox SDK implemented over SDL3, OpenGL and the host
operating system. It renders with OpenGL 4.5, plays sound through SDL3 and
takes the same keyboard, mouse and gamepad input as the Linux build.

## Building

Requirements, on Windows (no part of the Xbox SDK):

- Visual Studio (or the Build Tools) with the C++ workload: the 32-bit
  (x86) MSVC libraries and a Windows 10/11 SDK. Only the libraries and
  headers are used; the compiler is clang.
- LLVM (clang and lld), Python 3 and ninja on the PATH (for example
  `scoop install llvm python ninja`).
- Network access the first time: `configure.py` downloads SDL 3.4.16's
  Visual C++ development package into `build/windows/third_party`.

```bat
python configure.py
ninja windows
```

`configure.py` generates the Windows build only when it runs on Windows.

The executable is optimised for the processor of the computer that builds
it (`-march=native`) and may not start on another; `python configure.py
--portable` builds one that runs on any x86-64 processor, for sharing. Full
link-time optimisation makes the final link take a while; see
[Optimisation](../../README.md#optimisation) for this and for the
profile-guided optimisation with `pgo/halo_windows.profdata`, which
`--pgo=train` records again (the instrumented build uses LLVM's profile
runtime, compiled for 32-bit x86 from its sources, since LLVM for Windows
ships it for x86-64 only: `pgo/halo_profile_runtime.c`).

## Running

```bat
build\windows\halo.exe
```

The game data is found as on Linux: `paths.data` in `config.toml` if set, else the
current directory when it has `maps\`, else `assets\` in the current
directory or in the repository that holds the executable. It must be the PAL
data of this build (01.01.14.2342). Saves go to `%APPDATA%\halo`
(`paths.saves` overrides it). The settings are in `config.toml` next to
`halo.exe`, written with the defaults on the first run; they and the
controls are those of the Linux build (`port/linux/README.md`); like it, the game draws
a frame at every refresh of the display, between its 30 Hz ticks ("Frame
rate" there).

## How it works

The game is 32-bit code for the same reason as on Linux: its data formats
embed 32-bit pointers. Clang's `i686-pc-windows-msvc` target gives it the
ABI it was written against natively (MSVC structure layout, 16-bit
`wchar_t`, calling conventions), so much less adaptation is
needed than on Linux. The executable is large-address-aware: the Xbox memory
window the platform layer reserves is at 0x80000000.

### Headers

- Game and platform units see the Xbox SDK declarations, not the Windows
  SDK's of the same names (`winbase.h`, `windef.h`, ...): `port/include/xdk`,
  which stands in for the Xbox SDK's headers, comes ahead of the Windows SDK.
  The C runtime is the Windows one (the static UCRT).
- `include/halo_windows_prefix.h` is force-included into game and platform
  units. It renames the Xbox SDK functions that the platform layer
  implements under Windows names (`CreateFileA`, `ReadFile`, `Sleep`, ...;
  `include/halo_windows_api_names.h`), so that the Windows-facing code
  reaches Windows, and it shares the Linux build's Winsock renames and
  source fixups.
- `include/crt` wraps a few C runtime headers: the game's `fopen`, `open`,
  `mkdir` and similar calls with Xbox paths go through path translation
  (`src/windows_crt.c`); `stdlib.h` does not leak `limits.h` macros that the
  game's `cseries.h` declares itself; the game's own `strnlen` coexists with
  the runtime's.
- `include/posix` declares the POSIX calls the shared platform layer makes
  (threads, clocks, `mmap`, positional I/O, `sysconf`).

### Windows-facing code

The files named `src/win32_*.c` are compiled against the Windows SDK only:

- `win32_files.c` and `win32_net.c` implement the file and socket boundary
  of `port/linux/src/posix.h` (on Linux, `posix_files.c` and `posix_net.c`);
- `win32_posix.c` implements the POSIX calls above over Windows threads,
  critical sections, condition variables, `VirtualAlloc` and the
  performance counter, and sets binary file mode and a 1 ms timer period at
  start-up;
- `win32_memory_watch.c` replaces `memory_watch.c` (texture write tracking
  with a vectored exception handler) and reports crashes.

`port.json` lists the Linux platform files these replace, and the Windows
libraries linked.

### Inline functions

MSVC emits a C `__inline` function with external linkage as a COMDAT
wherever a call to it is not inlined, and some units call such functions
through ordinary prototypes. Clang's Microsoft target has the same COMDAT
linkage, but inlines more, so a function can end up emitted nowhere:

- header inlines reached through prototypes get one weak definition each
  from `port/linux/game/msvc_comdat.c` (COFF allows only one weak
  definition of a name; a COMDAT copy or an outright definition overrides
  it);
- the few `__inline` functions defined in `.c` files are exported by
  compiling their unit through a generated wrapper that takes their
  addresses (`inline_export_wrapper` in `tools/windows_build.py`).

Structure tags first named in a prototype get file scope in MSVC but not in
clang, so the Linux build's generated forward declarations are
force-included too (`build/windows/halo_msvc_tags.h`).

## Known limitations

- Bink video is not supported (as on Linux): the intro movies are skipped.
- Only the narrow (ANSI code page) file APIs are used: data or save paths
  with characters outside the system code page may not work.
