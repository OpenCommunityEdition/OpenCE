# Native Linux build

`ninja linux` compiles the decompiled game with clang for 32-bit x86 Linux
and links a native ELF executable, `build/linux/halo`. It is a separate
graph from the byte-matching build: it never touches the MSVC objects,
`objdiff.json`, or progress, and the only game source edits it needed were
verified to leave every MSVC object byte-identical (see below).

## Building

Requirements, in addition to the XDK headers already needed by the matching
build (`xbox/include`):

- clang (any recent version; `--linux-cc` selects another compiler)
- 32-bit glibc development files (`lib32-glibc` on Arch,
  `gcc-multilib`/`libc6-dev-i386` on Debian/Ubuntu)
- 32-bit SDL3 (`lib32-sdl3` on Arch, `libsdl3-dev:i386` on Debian/Ubuntu),
  and at run time 32-bit OpenGL and PipeWire/PulseAudio client libraries
  (`lib32-mesa`, `lib32-pipewire` or `lib32-libpulse`)

```sh
python configure.py
ninja linux
```

The game is built as 32-bit code because its data formats (tag data, cache
files, saved games) embed 32-bit pointers, exactly as on the Xbox.

## Running

```sh
build/linux/halo
```

The game data (the directory holding `maps/`) is found automatically:
`HALO_DATA_ROOT` if set, else the current directory when it has `maps/`,
else `assets/`, looked up in the current directory and in the repository
that holds the executable. It must be the PAL data of this build
(01.01.14.2342); the game rejects cache files from any other build.

`d:\` is the data root. Every other Xbox drive `X:\` is the directory `X/`
below the save root, which is `HALO_SAVE_ROOT`, else
`$XDG_DATA_HOME/halo-linux` (`~/.local/share/halo-linux`): `z:\` holds the
cache partition (about 800 MB of copied map data) and saves, `u:\` user
data. Directories are created on first use and path components are matched
case-insensitively, like the Xbox's FATX volumes. The game writes its log to
`d:\debug.txt` as usual. A `d:\init.txt` runs console commands at start-up,
for example `map_name levels\a10\a10` to go straight into a level.

### Controls

Keyboard and mouse drive controller 1; SDL gamepads are merged into it, and
further gamepads become controllers 2-4.

| Key | Controller |
| --- | --- |
| W A S D | left stick (move) |
| mouse | aim (direct, not through the right stick) |
| left button | right trigger (fire) |
| right button, G | left trigger (grenade) |
| space, enter | A (jump, accept) |
| F, backspace, mouse button 4 | B (melee, back) |
| E, R | X (action, reload) |
| tab, mouse wheel | Y (switch weapon) |
| Q | white (flashlight) |
| X | black |
| left ctrl, C | left stick click (crouch) |
| Z, middle button | right stick click (zoom) |
| arrows | D-pad |
| escape | start (pause menu) |
| F1 | back |
| \` | opens the developer console (typing then goes to the console) |
| F12 | releases or recaptures the mouse |

### Settings

| Variable | Effect |
| --- | --- |
| `HALO_DATA_ROOT`, `HALO_SAVE_ROOT` | see above |
| `HALO_WINDOW_SCALE` | initial window size as a multiple of 640x480 (default 2); the window is resizable and the picture is letterboxed |
| `HALO_MOUSE_SENSITIVITY` | mouse aim multiplier (default 1.0) |
| `HALO_MOUSE_INVERT` | set to invert vertical mouse aim |
| `HALO_VOLUME` | master volume (default 1.0) |
| `HALO_NO_AUDIO` | do not open an audio device (sound still runs, silently) |
| `HALO_LANGUAGE` | dashboard language: `en`, `ja`, `de`, `fr`, `es`, `it` |
| `HALO_INTERPOLATION=0` | the original 30 frames per second (see Frame rate) |
| `HALO_NO_VSYNC` | do not wait for the display between frames |
| `HALO_SCREENSHOT_DIR`, `HALO_SCREENSHOT_EVERY` | write every Nth presented frame as a BMP |
| `HALO_GPU_STATS`, `HALO_GPU_TRACE=<frame>` (with `HALO_GPU_TRACE_CONSTANTS`), `HALO_GPU_DUMP_SHADERS=<dir>`, `HALO_TEXTURE_DUMP=<dir>`, `HALO_TEXTURE_LOG`, `HALO_GL_DEBUG`, `HALO_TEXTURE_NO_CACHE` | renderer debugging: per-frame counts, a full state trace of one frame, the generated GLSL, uploaded textures |
| `HALO_GPU_SKIP_VS=<id>,...`, `HALO_GPU_DEBUG_EXPR=<glsl>`, `HALO_GPU_DEBUG_FLAT`, `HALO_GPU_DEBUG_T0` | renderer debugging: drop draws by vertex shader, or replace every pixel shader's output with a GLSL expression (for example `t0.rgb` or `xD0.rgb`) |

### Frame rate

The game simulates in 30 Hz ticks and originally drew one frame per tick.
The native builds (Linux, Windows, Android) draw a frame at every refresh
of the display instead, paced by vsync: 60, 90, 120, 240 Hz or whatever the
display runs at. Each frame shows the world between the last two ticks
(`game/render_interpolation.c`): after every tick the camera, every
object's node matrices and the first-person weapon's pose are kept, and a
frame blends the previous and the latest by how far the clock has run into
the next tick. Rotations are blended as quaternions (normalised lerp, the
shorter way round), positions and scales linearly; teleports, respawns and
camera cuts snap. What is drawn is therefore one tick (33 ms) behind the
simulation. Particles, contrails and other effects already moved every
frame. The simulation itself is unchanged: 30 Hz, as on the Xbox.

`HALO_INTERPOLATION=0` restores the original behaviour: one frame per
tick, throttled to 30 per second.

`display_framerate true` in the developer console shows the frame rate at
the bottom right of the screen: in the native builds the frames per second
averaged over half a second (the Xbox showed each frame's own rate, which
cannot read above 100).

## What works

| Area | Status |
| --- | --- |
| Game code | All 466 C translation units of the game project, unmodified apart from the edits listed below. |
| Graphics | Direct3D 8 on OpenGL 4.5 core through SDL3 (`src/d3d8_gl.c`): NV2A vertex shader microcode and register combiner pixel shaders are translated to GLSL, Xbox textures (swizzled, linear, DXT, palettized, cube and volume) are decoded and cached with page-protection write tracking, render targets are framebuffer objects, and the picture is presented letterboxed in a resizable window. |
| Sound | Xbox DirectSound over SDL3 audio (`src/dsound_sdl.c`): PCM and Xbox ADPCM streams mixed at 48 kHz with volume, pitch, mix bins, distance rolloff, stereo panning and I3DL2 occlusion/obstruction levels. Doppler, cones and reverb are not modelled. |
| Input | XInput over SDL3 (`src/xinput_sdl.c`): keyboard and mouse as controller 1, SDL gamepads with rumble, and the debug keyboard for the console. |
| Files | Win32 file API over POSIX (`CreateFile`, overlapped/`ReadFileEx` with completion APCs, find, attributes, times, free space), MSVC `fopen`/`open`/`_stat` families with Xbox path translation. |
| Threads and synchronisation | Threads (including `CREATE_SUSPENDED`), events, mutexes, critical sections, interlocked operations, alertable waits. |
| Memory | The Xbox contiguous-memory window is reserved at `0x80000000`, so `XPhysicalAlloc` returns the fixed game-state and tag-cache addresses the game asserts, and Direct3D physical addresses keep their meaning. |
| Time | Tick count, performance counter (1 MHz), system time, x87 control word (`_control87`). |
| Save games and signatures | `XCreateSaveGame` & co. with the Xbox `UDATA` layout; SHA-1 content signatures. |
| C runtime | MSVC-only functions, and a 16-bit `wchar_t` runtime (UTF-16 like the Xbox) including MSVC-style wide `printf`. |
| Networking | Winsock over BSD sockets; XNet addresses collapse to plain IPv4 (system link on a LAN). |
| Bink video | Not supported (the RAD SDK is proprietary); `BinkOpen` fails and the game skips the movie. |
| Debug monitor (`xbdm`) | Empty module lists. |

A fatal signal prints the faulting address and a backtrace to standard
error (`addr2line -e build/linux/halo <address>` symbolises it).

## How it works

### Compiling MSVC-era code with clang

`tools/linux_build.py` compiles the game with
`--target=i686-linux-gnu -fms-extensions -fshort-wchar -malign-double
-fcommon` and the other flags listed there, which reproduce the ABI the
source was written for: MSVC extensions, 16-bit `wchar_t`, 8-byte alignment
of 64-bit struct members, and C89 tentative definitions. The game's inline
assembly is not compiled (see [Game source edits](#game-source-edits)).
glibc is restricted to ISO C (`__STRICT_ANSI__`) so POSIX names such as
`random` and `strnlen` cannot collide with the game's own.

Everything MSVC-specific that clang lacks a switch for is supplied without
editing the game:

- `include/halo_linux_prefix.h` is force-included first: XDK architecture
  macros, MSVC `__inline` semantics, SEH keywords (`__try`/`__except` run the
  guarded block), `__declspec(selectany)`.
- `include/` shims extend or replace C runtime headers: MSVC names in
  `stdio.h`/`stdlib.h`/`string.h`/`math.h`/`float.h`, a complete 16-bit
  `wchar.h`, `io.h`, `direct.h`, `sys/stat.h` with the MSVC `struct _stat`.
- The XDK's own headers are used through a case-insensitive symlink overlay
  (`tools/linux_sdk_overlay.py`), which leaves out the XDK's C runtime
  headers in favour of glibc. `winnt.h` is a copy whose three 64-bit shift
  helpers are C instead of x86 assembly; the SDK itself is not modified.
- `tools/linux_msvc_semantics.py` generates a header that forward-declares
  every struct/union tag at file scope (MSVC gives a tag first seen in a
  prototype file scope; C gives it prototype scope) and marks header inline
  functions `#pragma weak`, the ELF analogue of MSVC's COMDAT inline
  functions. `game/msvc_comdat.c` then provides one external copy of each
  for units that call them through a plain prototype.
- `include/halo_linux_winsock_names.h` renames the XDK's `__stdcall`
  Winsock functions (`socket`, `bind`, `select`, ...) so they cannot bind to
  glibc's cdecl functions of the same names.
- `include/halo_linux_source_fixups.h` handles the one declaration conflict
  that could not be fixed in source without changing MSVC output
  (`rasterizer_debug_drawing_begin`).

`tools/linux_link_check.py` fails the link if any weak reference lacks a
definition, since the linker would otherwise resolve it to address 0.

### The platform layer (`src/`)

Files named `posix_*.c` talk to glibc and are compiled with the host ABI:
glibc structures with 64-bit members (`struct stat`, `struct dirent`) have a
different layout under `-malign-double`. Everything else includes the XDK
headers through `platform.h`, so each definition is type-checked against the
SDK prototype it implements, calling convention included.
`src/halo_linker_common.c` holds weak, zero-filled storage for globals that
the January link pooled from tentative definitions in units not yet
reconstructed, plus stand-ins for `fast_ftol_C` and `main_crash`. Being
weak, each gives way automatically once the real definition exists.

`main/d3d_intimacy.cpp`, which reads the Xbox Direct3D runtime's private
device structure, is left out of the Linux build; `src/d3d8_gl.c` provides
`d3d_find_flipcount` from its 60 Hz vertical blank thread.

Small structures and unions are returned in registers
(`-freg-struct-return`), as on Win32: `hs_runtime.c` calls union-returning
conversion functions through pointers typed as returning `long`.

### Game source edits

Five game files needed changes for clang. Each was checked by rebuilding
every MSVC object from a pristine checkout and comparing all 612 C objects:
all are identical apart from data that depends on the checkout's location
(the `.debug$S` path record and its section checksum) and MSVC's internal
local label numbers.

| File | Change |
| --- | --- |
| `cseries/cseries.c` | the naked `stristr` addresses its parameters as `[ebp+8]`/`[ebp+12]` (clang rejects named parameters in naked functions; MSVC emits the same operands) |
| `bitmaps/bitmap_drawing.c` | `*((word *)p)++` lvalue casts written as `*(*(word **)&p)++` |
| `rasterizer/xbox/rasterizer_xbox_hardware_bitmaps.c` | `&(T *)x` written as `(T **)&x` |
| `hs/hs.c` | removed local prototypes that contradicted `ai_script.h` |
| `units/vehicles.c` | local prototype of `unit_update_animation` uses the struct pointer type `units.h` declares |

`math/real_math.h` also had a header copy of `plane2d_from_points` that
disagreed with the real definition in `effects/decals.c` (it used the edge
direction instead of its perpendicular as the line normal). MSVC always
called the out-of-line function, so the matching build never saw it, but
clang inlines the header copy, which broke the portal clipping behind
structure visibility (whole areas of a level vanished depending on the view).
The header copy now agrees with the real function; the matching report is
unchanged.

Three more changes exist only under `#ifdef HALO_LINUX` (defined by the
Linux prefix header, never by the matching build):

| File | Change |
| --- | --- |
| `scenario/scenario.c` | the structure BSP connection tables are named directly instead of being addressed at MSVC's offsets from `global_structure_bsp_index` |
| `rasterizer/xbox/rasterizer_xbox_environment_fog.c` | a local pointer initialized from the file-scope array of the same name; MSVC resolved the name in the initializer to the array, standard C to the new local |
| `game/player_control.c` | adds direct mouse aim (`halo_linux_mouse_look`) to the facing change of the player on controller 1 |

The game's x86 inline assembly is also replaced under `#ifdef HALO_LINUX`,
which every native port (Linux, Windows, Android) defines, so the compiler
optimizes and vectorizes that code for each target like any other C:

| File | Assembly | Replacement |
| --- | --- | --- |
| `cseries/cseries.h` | x87 `fistp` float to integer conversion (`fast_ftol`) | `__builtin_rint` |
| `bitmaps/bitmaps_inlines.h` | x87 float to integer conversions | C conversions |
| `math/matrix_math.c` | SSE `matrix4x3_multiply` | the C loop |
| `effects/decals.c` | x87 float to integer conversion | C conversion |
| `cseries/profile.c` | `rdtsc` | `QueryPerformanceCounter`, at its own frequency |
| `cseries/cseries.c` | naked `stristr` | a C `stristr` |
| `cseries/stack_walk_windows.c` | reads EBP | `__builtin_frame_address` |
| `interface/hud_draw.c` | reads the caller's return address from `[ebp+4]` | `__builtin_return_address(1)` |
| `bink/bink_playback.c` | `int 3` | `__builtin_trap` |

The C runtime's x87 control and status words (`_control87`, `_statusfp`,
`_clearfp`, `src/msvc_crt.c`) go through `fenv.h`, or the FPCR and FPSR
builtins on Android.

## The matching build on a Linux host

The byte-matching build (`ninja`, `ninja all_source`) also works on Linux:

- The nine vendor-assembly CRT units are assembled with UASM (downloaded
  automatically) when no MASM is available. Their code sections are
  byte-identical, section flags included, to the MASM-built objects in the
  XDK's `libcmt.lib`; UASM adds one extra empty `.text` section. Pass
  `--ml path/to/ml.exe` to use real MASM through wibo/wine instead.
- csplit is built from source at a pinned upstream commit, because the
  v0.0.2 Linux release writes corrupt placeholder objects that objdiff
  rejects. The pinned commit is v0.0.2 plus that fix.
- `tools/msvc_deps_filter.py` rewrites the compiler's `/showIncludes` paths
  (`source/cseries\cseries.h`, `z:\home\...`) to their on-disk spelling, so
  ninja's header dependencies work and rebuilds are incremental.

One verification in `ninja progress` still fails on this host:
`config/semantic_data_matches.json` expects the compiler temporary
`$T18302` for `shell_xbox`'s `.rdata` scope table, but CL.exe run through
either wibo or wine names it `$T18301` from the same, unchanged sources. The
code and data are identical, so this looks like an environment-dependent
counter in the compiler; the ledger entry was left as is.
