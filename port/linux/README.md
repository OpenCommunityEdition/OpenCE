# Native Linux build

`ninja linux` compiles the decompiled game with clang for 32-bit x86 Linux
and links a native ELF executable, `build/linux/halo`. It is a separate
graph from the byte-matching build: it never touches the MSVC objects,
`objdiff.json`, or progress, and the only game source edits it needed were
verified to leave every MSVC object byte-identical (see below).

## Building

Requirements (no part of the Xbox SDK: the declarations the game uses from
it are in `port/include/xdk`):

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
`paths.data` in `config.toml` if set (see Settings), else the current
directory when it has `maps/`, else `assets/`, looked up in the current
directory and in the repository that holds the executable. It must be the PAL data of this build
(01.01.14.2342); the game rejects cache files from any other build.

`d:\` is the data root. Every other Xbox drive `X:\` is the directory `X/`
below the save root, which is `paths.saves`, else
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
| tab, mouse wheel | Y (switch weapon; one scroll of the wheel switches once, and scrolling again after a moment's pause switches again) |
| Q | white (flashlight) |
| X | black |
| left ctrl, C | left stick click (crouch) |
| Z, middle button | right stick click (zoom) |
| arrows | D-pad |
| escape | start (pause menu) |
| F1 | back |
| \` | opens the developer console (typing then goes to the console) |
| F12 | releases or recaptures the mouse |
| F11 | switches between fullscreen and the window |

In the menus (the main menu, the pause menu and the dialogs), the mouse is released and drives a pointer: the item under it takes the focus, a left click selects it (on a setting with a value, clicking its left or right half steps the value; on the rows of profiles and levels, clicking one moves to it and selects it; on a button in a screen's key, such as "B = Back", presses that button), a right click goes back and the wheel moves through the items. The keyboard keeps working alongside it. When the game resumes the mouse aims again; a button held from the click that resumed it does not fire until pressed again.

### Settings

The settings are in `config.toml` next to the executable
(`build/linux/config.toml`). The game writes it with the defaults and a
comment on each setting the first time it runs; delete it to get the
defaults back. It is read once at start-up, with
[tomlc17](../third_party/tomlc17) (`src/port_config.c`): a misspelt key or a
value of the wrong type is reported in the log (with its line) and the
default used. Each setting can also be set for one run with its environment
variable, which wins over the file; the tools use those
(`tools/pgo_train.py`).

| Setting | Default | Environment | Effect |
| --- | --- | --- | --- |
| `display.fullscreen` | `true` | `HALO_FULLSCREEN` | fullscreen at the display's resolution and shape: 480 of the game's lines, as wide as the display (the view widens, the HUD keeps to the screen's edges, menus stay centered), each drawn at as many pixels as the display has; `false` opens a window, which draws the Xbox's 640x480. F11 switches between the two |
| `display.window_scale` | `2` | `HALO_WINDOW_SCALE` | window size as a multiple of 640x480; the window is resizable and the picture is letterboxed |
| `display.vsync` | `true` | `HALO_NO_VSYNC=1` turns it off | wait for the display between frames |
| `display.interpolation` | `true` | `HALO_INTERPOLATION` | a frame at every display refresh; `false` the original 30 frames per second (see Frame rate) |
| `audio.enabled` | `true` | `HALO_NO_AUDIO=1` turns it off | open an audio device (with `false`, sound still runs, silently) |
| `audio.volume` | `1.0` | `HALO_VOLUME` | master volume |
| `input.mouse_sensitivity` | `1.0` | `HALO_MOUSE_SENSITIVITY` | mouse aim multiplier |
| `input.invert_mouse` | `false` | `HALO_MOUSE_INVERT=1` turns it on | invert vertical mouse aim |
| `game.language` | `""` | `HALO_LANGUAGE` | dashboard language: `ja`, `de`, `fr`, `es`, `it`; empty for English |
| `paths.data`, `paths.saves` | `""` | `HALO_DATA_ROOT`, `HALO_SAVE_ROOT` | see Running |
| `network.address` | `""` | `HALO_NET_ADDRESS` | this machine's system link IPv4 address: sockets bind to it instead of to every address, other machines see games at it, and traffic to 127.0.0.1 goes to it. Lets several copies of the game play together on one computer, each on its own loopback address (see System link) |
| `network.broadcast` | `""` | `HALO_NET_BROADCAST` | comma-separated IPv4 addresses to send the game's broadcasts (a client's game search, a host's game advertisement) to instead of 255.255.255.255, for example the other copies' loopback addresses on the same computer (listing 255.255.255.255 too still broadcasts). Machines with an address receive no broadcasts, so they find each other only through these lists |
| `debug.exit_after` | `0.0` | `HALO_EXIT_AFTER` | quit that many seconds after the window opens (profile training, benchmarks) |
| `debug.screenshot_directory`, `debug.screenshot_every` | `""`, `0` | `HALO_SCREENSHOT_DIR`, `HALO_SCREENSHOT_EVERY` | write every Nth presented frame as a BMP |
| `debug.gpu_stats`, `debug.gpu_trace_frame` (with `debug.gpu_trace_constants`), `debug.gpu_dump_shaders`, `debug.texture_dump_directory`, `debug.texture_log`, `debug.gl_debug`, `debug.texture_no_cache` | off | `HALO_GPU_STATS`, `HALO_GPU_TRACE`, `HALO_GPU_TRACE_CONSTANTS`, `HALO_GPU_DUMP_SHADERS`, `HALO_TEXTURE_DUMP`, `HALO_TEXTURE_LOG`, `HALO_GL_DEBUG`, `HALO_TEXTURE_NO_CACHE` | renderer debugging: per-frame counts, a full state trace of one frame, the generated GLSL, uploaded textures |
| `debug.gpu_skip_vertex_shaders`, `debug.gpu_debug_expression`, `debug.gpu_debug_flat`, `debug.gpu_debug_texture0` | off | `HALO_GPU_SKIP_VS`, `HALO_GPU_DEBUG_EXPR`, `HALO_GPU_DEBUG_FLAT`, `HALO_GPU_DEBUG_T0` | renderer debugging: drop draws by vertex shader, or replace every pixel shader's output with a GLSL expression (for example `t0.rgb` or `xD0.rgb`) |
| `debug.hidden_window`, `debug.null_renderer` | `false` | `HALO_HIDDEN_WINDOW`, `HALO_NULL_RENDERER` | run with the window hidden, or with none |

One environment variable is not the game's: `mesa_glthread=false`, with Mesa
drivers, makes the GL calls on the game's own thread (the game turns Mesa's
GL thread on otherwise).

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

`interpolation = false` in `config.toml` restores the original behaviour: one frame per
tick, throttled to 30 per second.

`display_framerate true` in the developer console shows the frame rate at
the bottom right of the screen: in the native builds the frames per second
averaged over half a second (the Xbox showed each frame's own rate, which
cannot read above 100).

### System link

The native builds play system link games of up to 128 players on up to
128 machines, where the Xbox game allows 16 players on up to 4. Split
screen stays at 4 players per machine. The two limits are constants in
`include/halo_port_limits.h`; the memory they need, a 16 MB game state at
`0x81A00000` instead of 3.3 MB and larger pools of objects, effects,
particles, contrails, lights and sounds, is set in
`include/halo_port_capacity.h`. The byte-matching build keeps the Xbox
limits: every change is under `#ifdef HALO_LINUX`.

- Every machine in a game must run a build with the same limits and
  capacities: the game runs in lockstep on every machine, and a pool that
  fills on one machine and not on another changes the game. The native
  builds' messages differ from the Xbox game's (longer arrays, and the
  13 KB game settings record sent in four pieces), so they search for
  games with protocol version 2: they see neither the Xbox game nor older
  native builds, and those do not see them.
- The host sends every machine every player's input each tick: 3.9 KB per
  tick with 128 players, about 0.9 Mbit/s to each machine and 118 Mbit/s
  of upload for a host of 128 machines (measured). The traffic grows with
  the square of the session; a host of 32 machines with one player each
  sends about 8 Mbit/s.
  A 100 Mbit/s network carries about 100 machines, Wi-Fi far fewer.
- The host waits up to 60 seconds (15 on the Xbox) for slower machines to
  load the map, and keeps the machines that have loaded connected
  meanwhile. A machine that stops reading the host's messages for two
  seconds is dropped from the game rather than holding everyone up.
- The game state is saved whole, so checkpoints and saved games are 16 MB
  (3.4 MB on the Xbox), and saves from earlier native builds do not carry
  over. Garbage (bodies, dropped weapons) is collected as on the Xbox in
  campaign and games of up to 16 players, and in proportion to the players
  in larger games.
- The lobby has panels for the local machine and three remote machines,
  and shows the first three remote machines to join; the others are in
  the game all the same. Finishing places past 16th, which the game's
  string lists lack, are written out in English (17th, 21st, 22nd, ...),
  and in free-for-all games every player is a team of one.

Several copies of the game can play together on one computer. The host
tells machines apart by address, so every copy needs its own loopback
address, the host included. A copy bound to one address receives no
broadcasts, so each lists the others in `network.broadcast`: the clients
send their game search to the host, and the host its game advertisement
to the clients. For a host and two clients (the environment variables
override `config.toml`'s `[network]` for one run):

```sh
HALO_NET_ADDRESS=127.0.0.200 HALO_NET_BROADCAST=127.0.0.201,127.0.0.202 build/linux/halo
HALO_NET_ADDRESS=127.0.0.201 HALO_NET_BROADCAST=127.0.0.200 build/linux/halo
HALO_NET_ADDRESS=127.0.0.202 HALO_NET_BROADCAST=127.0.0.200 build/linux/halo
```

Never give a copy 127.0.0.1: every copy reaches its own address through
127.0.0.1. Linux and Windows route all of 127.0.0.0/8 to the loopback
interface without configuration. Pinning a machine to a network card's
address with `network.address` works the same way: every machine must list
the others' addresses.

`tools/system_link_bots.py` fills a session without a hundred copies of the
game. It joins a host with lightweight stand-in machines, one player each,
that speak the system link protocol, acknowledge every tick and send input,
but do not simulate the game. On the host's computer (each stand-in binds
its own loopback address, 127.0.0.2 and up), create a game on the host and
run

```sh
python tools/system_link_bots.py --host 127.0.0.200 --machines 127 --start
```

`--start` starts the game once every stand-in is in the lobby. A host
without `network.address` is found at the default `--host 127.0.0.1`.

## What works

| Area | Status |
| --- | --- |
| Game code | All 466 C translation units of the game project, unmodified apart from the edits listed below. |
| Graphics | Direct3D 8 on OpenGL 4.5 core through SDL3 (`src/d3d8_gl.c`): NV2A vertex shader microcode and register combiner pixel shaders are translated to GLSL, Xbox textures (swizzled, linear, DXT, palettized, cube and volume) are decoded and cached with page-protection write tracking, vertex and index buffers are drawn from a copy of the Xbox's contiguous memory in GL buffers kept current the same way, GL state is set only when it changes, render targets are framebuffer objects, and the picture is presented letterboxed in a resizable window. |
| Sound | Xbox DirectSound over SDL3 audio (`src/dsound_sdl.c`): PCM and Xbox ADPCM streams mixed at 48 kHz with volume, pitch, mix bins, distance rolloff, stereo panning and I3DL2 occlusion/obstruction levels. Doppler, cones and reverb are not modelled. |
| Input | XInput over SDL3 (`src/xinput_sdl.c`): keyboard and mouse as controller 1, SDL gamepads with rumble, and the debug keyboard for the console. |
| Files | Win32 file API over POSIX (`CreateFile`, overlapped/`ReadFileEx` with completion APCs, find, attributes, times, free space), MSVC `fopen`/`open`/`_stat` families with Xbox path translation. |
| Threads and synchronisation | Threads (including `CREATE_SUSPENDED`), events, mutexes, critical sections, interlocked operations, alertable waits. |
| Memory | The Xbox contiguous-memory window is reserved at `0x80000000`, so `XPhysicalAlloc` returns the fixed game-state and tag-cache addresses the game asserts, and Direct3D physical addresses keep their meaning. |
| Time | Tick count, performance counter (1 MHz), system time, x87 control word (`_control87`). |
| Save games and signatures | `XCreateSaveGame` & co. with the Xbox `UDATA` layout; SHA-1 content signatures. |
| C runtime | MSVC-only functions, and a 16-bit `wchar_t` runtime (UTF-16 like the Xbox) including MSVC-style wide `printf`. |
| Networking | Winsock over BSD sockets; XNet addresses collapse to plain IPv4 (system link on a LAN), with games of up to 128 players on up to 128 machines (see System link). |
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
- The Xbox SDK declarations come from `port/include/xdk` (after every other
  include directory), which stands in for the SDK's headers; the C runtime
  headers they include are glibc's and the shims above.
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
different layout under `-malign-double`. With link-time optimisation they
stay native objects, as LLVM will not optimise code with glibc's 32-bit
`wchar_t` together with the game's 16-bit one. Everything else includes the
SDK declarations (`port/include/xdk`) through `platform.h`, so each definition
is type-checked against the SDK prototype it implements, calling convention
included.
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

Some changes exist only under `#ifdef HALO_LINUX` (defined by the Linux
prefix header, never by the matching build):

| File | Change |
| --- | --- |
| `scenario/scenario.c` | the structure BSP connection tables are named directly instead of being addressed at MSVC's offsets from `global_structure_bsp_index` |
| `rasterizer/xbox/rasterizer_xbox_environment_fog.c` | a local pointer initialized from the file-scope array of the same name; MSVC resolved the name in the initializer to the array, standard C to the new local |
| `game/player_control.c` | adds direct mouse aim (`halo_linux_mouse_look`) to the facing change of the player on controller 1 |
| `sound/game_sound.c` | `compute_sound_obstruction` (a collision test from the camera to each audible sound) runs once per game tick and its result is reused by the tick's other frames: the sound manager refreshes sounds every frame, which on the Xbox was once per tick |
| `networking/`, `game/` (players, player queues, game engine and its game types), `interface/` (lobby, HUD, motion sensor), `bungie_net/network/`, and the pools in `objects/`, `effects/`, `render/`, `sound/`, `hs/`, `structures/`, `cache/physical_memory_map.c` and `saved games/` | the system link limits and the memory they need (see System link); sizes and offsets that followed from the Xbox limits come from `include/halo_port_limits.h` and `include/halo_port_capacity.h` |
| `cseries/errors.c` | `debug.txt` stays open between lines (opening and closing it for each line took milliseconds on Windows, and a large session logs thousands of lines at once) |

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

This fork no longer generates the byte-matching build (see the main
README), but with it turned back on and the Xbox SDK in `xbox/`, it (`ninja`,
`ninja all_source`) also works on Linux:

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
