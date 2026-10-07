# Windows

`ninja windows` compiles the game with clang for 32-bit x86 Windows. The
result is a native executable, `build/windows/halo.exe`, with `SDL3.dll`
next to it. You must build on Windows.

The Windows build uses the platform layer of the Linux build
(`port/linux/src`). The graphics, the sound, the input, the settings and
the multiplayer functions are the same as on Linux. Refer to
[port/linux/README.md](../linux/README.md).

## Requirements

You do not need the Xbox SDK.

- Visual Studio or the Visual Studio Build Tools, with the C++ workload.
  The build uses the 32-bit (x86) MSVC libraries and a Windows 10 or 11
  SDK. It does not use the MSVC compiler.
- LLVM (clang and lld), Python 3 and ninja in the `PATH`. For example,
  enter `scoop install llvm python ninja`.
- A network connection for the first build. `configure.py` downloads the
  Visual C++ development package of SDL 3.4.16 to
  `build/windows/third_party`.

## Build the game

1. Go to the root folder of the repository.
2. Enter `python configure.py`.
3. Enter `ninja windows`.

`configure.py` generates the Windows build only on Windows.

The build uses all the instructions of the processor of the computer that
builds it (`-march=native`). Such a build does not always start on a
different computer. To make a build for other computers, enter
`python configure.py --portable`. Refer to the build options in the main
[README](../../README.md#build-options).

For a new optimization profile (`--pgo=train`), the build compiles the
profile runtime of LLVM for 32-bit x86 (`pgo/halo_profile_runtime.c`).
LLVM for Windows supplies this runtime only for x86-64.

## Start the game

Enter `build\windows\halo.exe`.

The game finds the game data as on Linux. Refer to "Start the game" in
[port/linux/README.md](../linux/README.md#start-the-game).

| Item | Location |
| --- | --- |
| Settings | `config.toml` next to `halo.exe` |
| Saved games | `%APPDATA%\halo`, or `paths.saves` in `config.toml` |
| Log | `debug.txt` in the data root (the folder that contains `maps\`) |
| Log of the port | The console. The release build has no console: `halo.log` next to `halo.exe` |
| Crash reports | `crashes\` next to `halo.exe`, until the game sends them |

## How the port operates

The game is 32-bit code, as on Linux, because its data contains 32-bit
pointers. The clang target `i686-pc-windows-msvc` gives the ABI of MSVC:
the structure layout, the 16-bit `wchar_t` and the calling conventions.
Thus the Windows build needs fewer changes than the Linux build.

The executable is large-address-aware, because the platform layer reserves
the Xbox memory at `0x80000000`.

### Headers

- The game and the platform layer use the Xbox SDK declarations of
  `port/include/xdk`, not the Windows SDK declarations with the same names.
  The compiler reads `port/include/xdk` before the Windows SDK.
- The C runtime is the static UCRT of Windows.
- `include/halo_windows_prefix.h` is the first header of each file. It gives
  Windows names to the Xbox SDK functions of the platform layer (for example
  `CreateFileA`, `ReadFile`, `Sleep`). Thus the code for Windows gets to
  Windows. The list of names is in `include/halo_windows_api_names.h`.
- `include/crt` changes some C runtime headers. The file functions of the
  game (`fopen`, `open`, `mkdir`) go through the translation of Xbox paths
  (`src/windows_crt.c`).
- `include/posix` declares the POSIX functions that the platform layer uses.

### Code for Windows

These files use only the Windows SDK:

| File | Contents |
| --- | --- |
| `src/win32_files.c`, `src/win32_net.c` | The file and socket functions of `port/linux/src/posix.h`. |
| `src/win32_posix.c` | The POSIX functions on Windows threads, critical sections, condition variables, `VirtualAlloc` and the performance counter. |
| `src/win32_memory_watch.c` | The write tracking of textures, with a vectored exception handler. |
| `src/win32_crash.c` | The crash reports. Refer to "Crash reports". |
| `src/win32_d3d12_*.c` | The Direct3D 12 work of the Direct3D 12 renderer. Refer to "Direct3D 12". |

`port.json` gives the Linux files that these files replace, and the Windows
libraries of the link.

## Direct3D 12

The game draws with OpenGL 4.5 (`port/linux/src/d3d8_gl.c`) or, on
Windows, with Direct3D 12. `display.renderer` in `config.toml` (or the
`HALO_RENDERER` environment variable) chooses: `"d3d12"`, `"gl"` or
`"auto"` (OpenGL for now). A renderer that cannot start falls back to
OpenGL. With Direct3D 12 the game uses no OpenGL.

Both renderers draw what the Xbox Direct3D 8 device holds
(`port/linux/src/xgpu_device.h`), with the same shaders: the NV2A programs
translated to GLSL or to HLSL (`nv2a_vsh.c`, `nv2a_psh.c`). The Direct3D 12
renderer has two halves, because the platform layer sees the Xbox SDK
declarations and Direct3D 12 needs the Windows SDK declarations:

| File | Contents |
| --- | --- |
| `src/d3d12_device.c` | The renderer of the device: it reads the state of the device and describes each draw (shaders, pipeline state, textures, targets, vertices, constants). Xbox SDK. |
| `src/d3d12_renderer.h` | The interface between the halves, in C types only. |
| `src/win32_d3d12_device.c` | The device, the swap chain, the frames in flight, upload memory, descriptors, presentation. Windows SDK. |
| `src/win32_d3d12_resources.c` | Textures, render targets, the buffers of the vertex mirror, samplers. |
| `src/win32_d3d12_draw.c` | Pipeline states, the recording of the draws, clears, visibility tests. |
| `src/win32_d3d12_passes.c` | The window blit, clears of some channels, FXAA, SMAA, mip levels. |

Each draw carries the pass of the game that draws it (the sky, the models,
the lightmaps, the transparents, the HUD...) and the window. PIX and
RenderDoc show the draws under the names of the passes. The camera of each
window and the end of the solid surfaces of each window also reach the
renderer. A raytracer needs these: Direct3D 12 raytracing (DXR) is not
available to a 32-bit process, so a raytracer runs as a 64-bit process of its
own and takes the scene from the draws of the opaque passes.

Settings for debugging:

| Setting | Environment variable | Effect |
| --- | --- | --- |
| `debug.d3d12_debug` | `HALO_D3D12_DEBUG` | The debug layer of Direct3D 12 (the Graphics Tools feature of Windows). Its errors and warnings go to the log. |
| `debug.d3d12_gpu_validation` | `HALO_D3D12_GPU_VALIDATION` | With the debug layer, the validation on the GPU (slow). |
| `debug.gpu_dump_shaders` | `HALO_GPU_DUMP_SHADERS` | The HLSL of each shader is written to this folder. |

`tools/render_test.py` compares the pictures of the two renderers. Refer to
its description.

### Inline functions

MSVC makes a COMDAT copy of an external `__inline` function where it does
not inline a call. Some files call such functions through a prototype.
clang inlines more calls. Thus a function can have no copy. The build
prevents this:

- `port/linux/game/msvc_comdat.c` gives one weak definition of each header
  inline function.
- A wrapper takes the address of each `__inline` function in a `.c` file.
  Thus the compiler makes an external copy (`inline_export_wrapper` in
  `tools/windows_build.py`).

MSVC gives file scope to a structure tag in a prototype. clang does not.
Thus the build also includes the declarations of the Linux build
(`build/windows/halo_msvc_tags.h`).

## Crash reports

A crash writes the faulting address and the calls that led to it to
`debug.txt` and to the log. The builds of the workflow (the releases)
also send a crash report to the Sentry project of the developers:

1. The game starts a second copy of `halo.exe` (`halo.exe --crash-report`).
   This copy writes a minidump of the game to `crashes\` next to
   `halo.exe`, and the game stops. A minidump contains the stacks and the
   registers of the threads, and the list of the modules. It does not
   contain the other memory of the game.
2. At the first crash, the copy asks the player whether to send crash
   reports. `crash_reports.upload` in `config.toml` keeps the answer:
   `"yes"` sends this report and all the reports after it, and `"no"` sends
   no reports and does not ask again. The answer `"no"` deletes the
   minidump.
3. The copy sends the minidump and a copy of `halo.log` to Sentry, then
   deletes them. If it cannot send them (for example, with no network
   connection), the game sends them when it starts the next time
   (`halo.exe --crash-upload`, in the background). `crashes\` keeps at
   most 8 reports.

Sentry finds the function names and the source lines of the minidump in
the PDBs (`halo.pdb`, `SDL3.pdb`). The `release` job of the workflow
uploads the PDBs and the executables of each build of main to Sentry. This
needs the `SENTRY_AUTH_TOKEN` secret of the repository: an organization
auth token from the settings of the Sentry organization (the token gives
the organization and its region). The `SENTRY_PROJECT` variable of the
repository can give the slug of the project; without it, the job uses the
id of the project in the DSN. For a personal token instead, also set the
`SENTRY_ORG` variable (the slug of the organization) and the `SENTRY_URL`
variable (`https://de.sentry.io`).

Each release also has the PDBs as `halo-windows-<configuration>-symbols.zip`.
`tools/symbolize_crash.py` uses them to add the function names and the
source lines to the crash lines of a `debug.txt` or a `halo.log`:

```
python tools/symbolize_crash.py debug.txt halo.exe
```

Give the `halo.exe` of the build that crashed, with its `halo.pdb` next to
it. The tool needs `llvm-symbolizer` (LLVM).

Builds without a build number (a local build, or a build of a branch other
than main) send no crash reports. To test the crash reports with such a
build, set the `HALO_CRASH_REPORTS_ANY_BUILD` environment variable.

The crash reports also include the crashes of `abort()` and of an invalid
argument to a function of the C runtime.

## Limits

- Bink video is not available. The game skips the movies.
- The game uses only the ANSI file functions. A path to the data or the
  saved games with characters that are not in the code page of the system
  does not always operate.
